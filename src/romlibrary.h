#ifndef ROMLIBRARY_H
#define ROMLIBRARY_H

#include <QAbstractListModel>
#include <QFutureWatcher>
#include <QVector>

class Settings;

struct RomEntry {
    QString path;
    QString name;        // file name without extension, tidied for display
    QString headerTitle; // title stored in the cartridge header
    QString platform;    // "GBA", "GBC" or "GB"
    qint64 size = 0;
};

// The games found in the configured folders plus recently opened files,
// most recently played first.
class RomLibrary : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(bool scanning READ scanning NOTIFY scanningChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged)

public:
    enum Roles {
        PathRole = Qt::UserRole + 1,
        NameRole,
        HeaderTitleRole,
        PlatformRole,
        ThumbnailRole,
        LastPlayedRole,
        HasResumeRole,
        FolderRole
    };

    RomLibrary(Settings *settings, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    bool scanning() const { return m_watcher.isRunning(); }
    QString filter() const { return m_filter; }
    void setFilter(const QString &filter);

    Q_INVOKABLE void rescan();
    // Re-sorts and refreshes thumbnails/resume state after a game was played.
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void forget(const QString &path);
    Q_INVOKABLE static bool isRom(const QString &path);

    static RomEntry inspect(const QString &path);

signals:
    void scanningChanged();
    void countChanged();
    void filterChanged();

private:
    void onScanFinished();
    void rebuild();

    Settings *m_settings;
    QFutureWatcher<QVector<RomEntry>> m_watcher;
    QVector<RomEntry> m_all;
    QVector<RomEntry> m_visible;
    QString m_filter;
    bool m_rescanQueued = false;
};

#endif // ROMLIBRARY_H
