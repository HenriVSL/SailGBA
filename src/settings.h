#ifndef SETTINGS_H
#define SETTINGS_H

#include <QObject>
#include <QSettings>
#include <QStringList>

// Persistent user preferences, exposed to QML as the "settings" context property.
class Settings : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool audioEnabled READ audioEnabled WRITE setAudioEnabled NOTIFY audioEnabledChanged)
    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(bool hapticsEnabled READ hapticsEnabled WRITE setHapticsEnabled NOTIFY hapticsEnabledChanged)
    Q_PROPERTY(qreal controlsOpacity READ controlsOpacity WRITE setControlsOpacity NOTIFY controlsOpacityChanged)
    Q_PROPERTY(qreal controlsScale READ controlsScale WRITE setControlsScale NOTIFY controlsScaleChanged)
    Q_PROPERTY(bool smoothScaling READ smoothScaling WRITE setSmoothScaling NOTIFY smoothScalingChanged)
    Q_PROPERTY(bool integerScaling READ integerScaling WRITE setIntegerScaling NOTIFY integerScalingChanged)
    Q_PROPERTY(bool showFps READ showFps WRITE setShowFps NOTIFY showFpsChanged)
    Q_PROPERTY(int orientation READ orientation WRITE setOrientation NOTIFY orientationChanged)
    Q_PROPERTY(bool useBios READ useBios WRITE setUseBios NOTIFY useBiosChanged)
    Q_PROPERTY(bool skipBiosIntro READ skipBiosIntro WRITE setSkipBiosIntro NOTIFY skipBiosIntroChanged)
    Q_PROPERTY(bool autoSaveState READ autoSaveState WRITE setAutoSaveState NOTIFY autoSaveStateChanged)
    Q_PROPERTY(int fastForwardSpeed READ fastForwardSpeed WRITE setFastForwardSpeed NOTIFY fastForwardSpeedChanged)
    Q_PROPERTY(QStringList romFolders READ romFolders WRITE setRomFolders NOTIFY romFoldersChanged)
    Q_PROPERTY(QString biosPath READ biosPath NOTIFY biosPathChanged)
    Q_PROPERTY(bool hasBios READ hasBios NOTIFY biosPathChanged)

public:
    enum Orientation { OrientationAuto = 0, OrientationPortrait = 1, OrientationLandscape = 2 };
    Q_ENUM(Orientation)

    explicit Settings(QObject *parent = nullptr);

    bool audioEnabled() const;
    void setAudioEnabled(bool on);
    int volume() const;
    void setVolume(int volume);
    bool hapticsEnabled() const;
    void setHapticsEnabled(bool on);
    qreal controlsOpacity() const;
    void setControlsOpacity(qreal opacity);
    qreal controlsScale() const;
    void setControlsScale(qreal scale);
    bool smoothScaling() const;
    void setSmoothScaling(bool on);
    bool integerScaling() const;
    void setIntegerScaling(bool on);
    bool showFps() const;
    void setShowFps(bool on);
    int orientation() const;
    void setOrientation(int orientation);
    bool useBios() const;
    void setUseBios(bool on);
    bool skipBiosIntro() const;
    void setSkipBiosIntro(bool on);
    bool autoSaveState() const;
    void setAutoSaveState(bool on);
    int fastForwardSpeed() const;
    void setFastForwardSpeed(int speed);
    QStringList romFolders() const;
    void setRomFolders(const QStringList &folders);

    // The BIOS is copied into the app's data directory, so it keeps working
    // after the original file is moved and stays inside the sandbox.
    QString biosPath() const;
    bool hasBios() const;
    Q_INVOKABLE QString importBios(const QString &path);
    Q_INVOKABLE void removeBios();

    Q_INVOKABLE void addRomFolder(const QString &folder);
    Q_INVOKABLE void removeRomFolder(const QString &folder);

    // Recently played files, newest first. Files opened from a picker appear
    // in the library through this list even when outside the scanned folders.
    QStringList recentGames() const;
    void touchRecentGame(const QString &path);
    void forgetRecentGame(const QString &path);
    qint64 lastPlayed(const QString &path) const;

    static QString dataDir();

signals:
    void audioEnabledChanged();
    void volumeChanged();
    void hapticsEnabledChanged();
    void controlsOpacityChanged();
    void controlsScaleChanged();
    void smoothScalingChanged();
    void integerScalingChanged();
    void showFpsChanged();
    void orientationChanged();
    void useBiosChanged();
    void skipBiosIntroChanged();
    void autoSaveStateChanged();
    void fastForwardSpeedChanged();
    void romFoldersChanged();
    void biosPathChanged();
    void recentGamesChanged();

private:
    void set(const char *key, const QVariant &value, void (Settings::*signal)());

    QSettings m_settings;
};

#endif // SETTINGS_H
