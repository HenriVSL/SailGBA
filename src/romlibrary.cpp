#include "romlibrary.h"

#include "emulator.h"
#include "settings.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>
#include <QUrl>
#include <QtConcurrent>

#include <algorithm>

namespace {

const int MaxScanDepth = 4;

QString tidyName(const QString &baseName)
{
    QString name = baseName;
    name.replace(QLatin1Char('_'), QLatin1Char(' '));
    return name.simplified();
}

QString printable(const QByteArray &bytes)
{
    QString out;
    for (char c : bytes) {
        if (c == '\0')
            break;
        out.append(c >= 0x20 && c < 0x7F ? QLatin1Char(c) : QLatin1Char(' '));
    }
    return out.simplified();
}

void scanDir(const QString &path, int depth, QSet<QString> *seen, QVector<RomEntry> *out)
{
    QDir dir(path);
    const QFileInfoList entries = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Readable);
    for (const QFileInfo &info : entries) {
        if (info.isDir()) {
            if (depth < MaxScanDepth && !info.fileName().startsWith(QLatin1Char('.')))
                scanDir(info.filePath(), depth + 1, seen, out);
        } else if (RomLibrary::isRom(info.filePath())) {
            const QString canonical = info.canonicalFilePath();
            if (seen->contains(canonical))
                continue;
            seen->insert(canonical);
            const RomEntry entry = RomLibrary::inspect(info.filePath());
            if (!entry.platform.isEmpty())
                out->append(entry);
        }
    }
}

QVector<RomEntry> scan(const QStringList &folders, const QStringList &recent)
{
    QVector<RomEntry> result;
    QSet<QString> seen;
    for (const QString &path : recent) {
        QFileInfo info(path);
        if (!info.isFile() || seen.contains(info.canonicalFilePath()))
            continue;
        seen.insert(info.canonicalFilePath());
        const RomEntry entry = RomLibrary::inspect(path);
        if (!entry.platform.isEmpty())
            result.append(entry);
    }
    QStringList roots = folders;
    // Memory cards are mounted under /run/media/<user>/<label>.
    QDir media(QStringLiteral("/run/media/") + qgetenv("USER"));
    for (const QFileInfo &card : media.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot))
        roots.append(card.filePath());
    for (const QString &root : roots)
        scanDir(root, 0, &seen, &result);
    return result;
}

} // namespace

RomLibrary::RomLibrary(Settings *settings, QObject *parent)
    : QAbstractListModel(parent)
    , m_settings(settings)
{
    connect(&m_watcher, &QFutureWatcherBase::finished, this, &RomLibrary::onScanFinished);
    connect(settings, &Settings::romFoldersChanged, this, &RomLibrary::rescan);
    rescan();
}

bool RomLibrary::isRom(const QString &path)
{
    const QString suffix = QFileInfo(path).suffix().toLower();
    return suffix == QLatin1String("gba") || suffix == QLatin1String("gb")
        || suffix == QLatin1String("gbc") || suffix == QLatin1String("agb")
        || suffix == QLatin1String("sgb");
}

RomEntry RomLibrary::inspect(const QString &path)
{
    RomEntry entry;
    QString local = path;
    if (local.startsWith(QLatin1String("file://")))
        local = QUrl(local).toLocalFile();
    entry.path = local;
    QFileInfo info(local);
    entry.name = tidyName(info.completeBaseName());
    entry.size = info.size();

    QFile file(local);
    if (!file.open(QIODevice::ReadOnly))
        return entry;
    const QByteArray header = file.read(0x150);
    if (header.size() >= 0xC0 && quint8(header[0xB2]) == 0x96) {
        entry.platform = QStringLiteral("GBA");
        entry.headerTitle = printable(header.mid(0xA0, 12));
    } else if (header.size() >= 0x150 && quint8(header[0x104]) == 0xCE && quint8(header[0x105]) == 0xED) {
        const quint8 cgb = quint8(header[0x143]);
        entry.platform = (cgb & 0x80) ? QStringLiteral("GBC") : QStringLiteral("GB");
        entry.headerTitle = printable(header.mid(0x134, (cgb & 0x80) ? 11 : 16));
    }
    return entry;
}

void RomLibrary::rescan()
{
    if (m_watcher.isRunning()) {
        m_rescanQueued = true;
        return;
    }
    m_watcher.setFuture(QtConcurrent::run(scan, m_settings->romFolders(), m_settings->recentGames()));
    emit scanningChanged();
}

void RomLibrary::onScanFinished()
{
    m_all = m_watcher.result();
    rebuild();
    emit scanningChanged();
    if (m_rescanQueued) {
        m_rescanQueued = false;
        rescan();
    }
}

void RomLibrary::refresh()
{
    // A game opened from a picker may not be in the list yet.
    const QStringList recent = m_settings->recentGames();
    if (!recent.isEmpty()) {
        const QString newest = recent.first();
        const bool known = std::any_of(m_all.cbegin(), m_all.cend(),
                                       [&](const RomEntry &e) { return e.path == newest; });
        if (!known) {
            const RomEntry entry = inspect(newest);
            if (!entry.platform.isEmpty())
                m_all.append(entry);
        }
    }
    rebuild();
}

void RomLibrary::forget(const QString &path)
{
    m_settings->forgetRecentGame(path);
    m_all.erase(std::remove_if(m_all.begin(), m_all.end(), [&](const RomEntry &e) { return e.path == path; }), m_all.end());
    rebuild();
}

void RomLibrary::setFilter(const QString &filter)
{
    if (m_filter == filter)
        return;
    m_filter = filter;
    rebuild();
    emit filterChanged();
}

void RomLibrary::rebuild()
{
    QVector<RomEntry> visible;
    for (const RomEntry &entry : m_all) {
        if (m_filter.isEmpty() || entry.name.contains(m_filter, Qt::CaseInsensitive)
            || entry.headerTitle.contains(m_filter, Qt::CaseInsensitive))
            visible.append(entry);
    }
    QHash<QString, qint64> played;
    for (const RomEntry &entry : visible)
        played.insert(entry.path, m_settings->lastPlayed(entry.path));
    std::sort(visible.begin(), visible.end(), [&](const RomEntry &a, const RomEntry &b) {
        const qint64 pa = played.value(a.path), pb = played.value(b.path);
        if (pa != pb)
            return pa > pb;
        return QString::localeAwareCompare(a.name, b.name) < 0;
    });

    const int oldCount = m_visible.size();
    beginResetModel();
    m_visible = visible;
    endResetModel();
    if (oldCount != m_visible.size())
        emit countChanged();
}

int RomLibrary::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_visible.size();
}

QVariant RomLibrary::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_visible.size())
        return QVariant();
    const RomEntry &entry = m_visible.at(index.row());
    switch (role) {
    case PathRole:
        return entry.path;
    case NameRole:
        return entry.name;
    case HeaderTitleRole:
        return entry.headerTitle;
    case PlatformRole:
        return entry.platform;
    case ThumbnailRole: {
        const QString thumb = Emulator::thumbnailPath(entry.path);
        QFileInfo info(thumb);
        if (!info.exists())
            return QUrl();
        QUrl url = QUrl::fromLocalFile(thumb);
        url.setQuery(QStringLiteral("t=") + QString::number(info.lastModified().toMSecsSinceEpoch()));
        return url;
    }
    case LastPlayedRole:
        return m_settings->lastPlayed(entry.path);
    case HasResumeRole:
        return QFileInfo::exists(QDir(Settings::dataDir()).filePath(
            QStringLiteral("states/") + Emulator::baseName(entry.path) + QStringLiteral(".ss0")));
    case FolderRole:
        return QFileInfo(entry.path).absolutePath();
    }
    return QVariant();
}

QHash<int, QByteArray> RomLibrary::roleNames() const
{
    return {
        { PathRole, "path" },
        { NameRole, "name" },
        { HeaderTitleRole, "headerTitle" },
        { PlatformRole, "platform" },
        { ThumbnailRole, "thumbnail" },
        { LastPlayedRole, "lastPlayed" },
        { HasResumeRole, "hasResume" },
        { FolderRole, "folder" },
    };
}
