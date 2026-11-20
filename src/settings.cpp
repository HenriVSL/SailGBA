#include "settings.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QUrl>

namespace {
const int MaxRecentGames = 50;
const qint64 GbaBiosSize = 16384;
}

Settings::Settings(QObject *parent)
    : QObject(parent)
    , m_settings(QDir(dataDir()).filePath(QStringLiteral("settings.ini")), QSettings::IniFormat)
{
}

QString Settings::dataDir()
{
    static const QString dir = [] {
        QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir().mkpath(path);
        return path;
    }();
    return dir;
}

void Settings::set(const char *key, const QVariant &value, void (Settings::*signal)())
{
    const QString k = QLatin1String(key);
    if (m_settings.value(k) == value)
        return;
    m_settings.setValue(k, value);
    emit (this->*signal)();
}

bool Settings::audioEnabled() const { return m_settings.value("audio/enabled", true).toBool(); }
void Settings::setAudioEnabled(bool on) { set("audio/enabled", on, &Settings::audioEnabledChanged); }

int Settings::volume() const { return m_settings.value("audio/volume", 100).toInt(); }
void Settings::setVolume(int volume) { set("audio/volume", qBound(0, volume, 100), &Settings::volumeChanged); }

bool Settings::hapticsEnabled() const { return m_settings.value("controls/haptics", true).toBool(); }
void Settings::setHapticsEnabled(bool on) { set("controls/haptics", on, &Settings::hapticsEnabledChanged); }

qreal Settings::controlsOpacity() const { return m_settings.value("controls/opacity", 0.7).toReal(); }
void Settings::setControlsOpacity(qreal opacity) { set("controls/opacity", qBound(qreal(0.1), opacity, qreal(1.0)), &Settings::controlsOpacityChanged); }

qreal Settings::controlsScale() const { return m_settings.value("controls/scale", 1.0).toReal(); }
void Settings::setControlsScale(qreal scale) { set("controls/scale", qBound(qreal(0.6), scale, qreal(1.5)), &Settings::controlsScaleChanged); }

bool Settings::smoothScaling() const { return m_settings.value("video/smooth", false).toBool(); }
void Settings::setSmoothScaling(bool on) { set("video/smooth", on, &Settings::smoothScalingChanged); }

bool Settings::integerScaling() const { return m_settings.value("video/integer", false).toBool(); }
void Settings::setIntegerScaling(bool on) { set("video/integer", on, &Settings::integerScalingChanged); }

bool Settings::showFps() const { return m_settings.value("video/showFps", false).toBool(); }
void Settings::setShowFps(bool on) { set("video/showFps", on, &Settings::showFpsChanged); }

int Settings::orientation() const { return m_settings.value("video/orientation", OrientationAuto).toInt(); }
void Settings::setOrientation(int orientation) { set("video/orientation", orientation, &Settings::orientationChanged); }

bool Settings::useBios() const { return m_settings.value("system/useBios", true).toBool(); }
void Settings::setUseBios(bool on) { set("system/useBios", on, &Settings::useBiosChanged); }

bool Settings::skipBiosIntro() const { return m_settings.value("system/skipBiosIntro", true).toBool(); }
void Settings::setSkipBiosIntro(bool on) { set("system/skipBiosIntro", on, &Settings::skipBiosIntroChanged); }

bool Settings::autoSaveState() const { return m_settings.value("system/autoSaveState", true).toBool(); }
void Settings::setAutoSaveState(bool on) { set("system/autoSaveState", on, &Settings::autoSaveStateChanged); }

int Settings::fastForwardSpeed() const { return m_settings.value("system/fastForwardSpeed", 3).toInt(); }
void Settings::setFastForwardSpeed(int speed) { set("system/fastForwardSpeed", qBound(0, speed, 8), &Settings::fastForwardSpeedChanged); }

QStringList Settings::romFolders() const
{
    if (!m_settings.contains("library/folders")) {
        const QString home = QDir::homePath();
        return { QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
                 QStandardPaths::writableLocation(QStandardPaths::DownloadLocation),
                 home + QStringLiteral("/android_storage/Download") };
    }
    return m_settings.value("library/folders").toStringList();
}

void Settings::setRomFolders(const QStringList &folders)
{
    if (folders == romFolders())
        return;
    m_settings.setValue("library/folders", folders);
    emit romFoldersChanged();
}

void Settings::addRomFolder(const QString &folder)
{
    QStringList folders = romFolders();
    const QString clean = QDir::cleanPath(folder);
    if (!folders.contains(clean)) {
        folders.append(clean);
        setRomFolders(folders);
    }
}

void Settings::removeRomFolder(const QString &folder)
{
    QStringList folders = romFolders();
    folders.removeAll(folder);
    setRomFolders(folders);
}

QString Settings::biosPath() const
{
    const QString path = QDir(dataDir()).filePath(QStringLiteral("bios/gba_bios.bin"));
    return QFileInfo::exists(path) ? path : QString();
}

bool Settings::hasBios() const
{
    return !biosPath().isEmpty();
}

QString Settings::importBios(const QString &path)
{
    QString local = path;
    if (local.startsWith(QLatin1String("file://")))
        local = QUrl(local).toLocalFile();
    QFileInfo info(local);
    if (!info.isFile())
        return tr("File not found");
    if (info.size() != GbaBiosSize)
        return tr("Not a GBA BIOS: the file should be exactly 16 KiB");

    QDir dir(dataDir());
    dir.mkpath(QStringLiteral("bios"));
    const QString target = dir.filePath(QStringLiteral("bios/gba_bios.bin"));
    QFile::remove(target);
    if (!QFile::copy(local, target))
        return tr("Could not copy the BIOS file");
    emit biosPathChanged();
    return QString();
}

void Settings::removeBios()
{
    const QString path = biosPath();
    if (!path.isEmpty()) {
        QFile::remove(path);
        emit biosPathChanged();
    }
}

QStringList Settings::recentGames() const
{
    return m_settings.value("library/recent").toStringList();
}

void Settings::touchRecentGame(const QString &path)
{
    QStringList recent = recentGames();
    recent.removeAll(path);
    recent.prepend(path);
    while (recent.size() > MaxRecentGames)
        recent.removeLast();
    m_settings.setValue("library/recent", recent);
    m_settings.setValue(QStringLiteral("played/") + QString::fromLatin1(path.toUtf8().toBase64(QByteArray::Base64UrlEncoding)),
                        QDateTime::currentMSecsSinceEpoch());
    emit recentGamesChanged();
}

void Settings::forgetRecentGame(const QString &path)
{
    QStringList recent = recentGames();
    if (recent.removeAll(path)) {
        m_settings.setValue("library/recent", recent);
        emit recentGamesChanged();
    }
}

qint64 Settings::lastPlayed(const QString &path) const
{
    return m_settings.value(QStringLiteral("played/") + QString::fromLatin1(path.toUtf8().toBase64(QByteArray::Base64UrlEncoding)), 0).toLongLong();
}
