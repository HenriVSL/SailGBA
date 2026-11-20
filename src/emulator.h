#ifndef EMULATOR_H
#define EMULATOR_H

#include <QImage>
#include <QMutex>
#include <QObject>
#include <QUrl>

#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>

class AudioOutput;
class Settings;
struct mCore;
struct RumbleBridge;

// The most recent video frame, shared between the emulation thread (writer)
// and the scene graph render thread (reader).
class FrameBuffer
{
public:
    // Returns true and copies the frame if it changed since `serial`.
    bool take(QImage *out, quint64 *serial) const;
    void publish(const quint32 *pixels, int width, int height, int stride);
    QImage snapshot() const;
    QSize size() const;

private:
    mutable QMutex m_mutex;
    QImage m_image;
    quint64 m_serial = 0;
};

// Runs one mGBA core on its own thread. All mCore calls happen on that thread;
// the QML-facing methods queue commands to it.
class Emulator : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)
    Q_PROPERTY(bool paused READ paused WRITE setPaused NOTIFY pausedChanged)
    Q_PROPERTY(bool fastForward READ fastForward WRITE setFastForward NOTIFY fastForwardChanged)
    Q_PROPERTY(QString gameTitle READ gameTitle NOTIFY gameChanged)
    Q_PROPERTY(QString romPath READ romPath NOTIFY gameChanged)
    Q_PROPERTY(QString platform READ platform NOTIFY gameChanged)
    Q_PROPERTY(int frameWidth READ frameWidth NOTIFY frameSizeChanged)
    Q_PROPERTY(int frameHeight READ frameHeight NOTIFY frameSizeChanged)
    Q_PROPERTY(qreal fps READ fps NOTIFY fpsChanged)
    Q_PROPERTY(bool rumble READ rumble NOTIFY rumbleChanged)

public:
    enum Key {
        KeyA = 1 << 0,
        KeyB = 1 << 1,
        KeySelect = 1 << 2,
        KeyStart = 1 << 3,
        KeyRight = 1 << 4,
        KeyLeft = 1 << 5,
        KeyUp = 1 << 6,
        KeyDown = 1 << 7,
        KeyR = 1 << 8,
        KeyL = 1 << 9
    };
    Q_ENUM(Key)

    static const int AutoSaveSlot = 0;
    static const int StateSlots = 6;

    explicit Emulator(Settings *settings, QObject *parent = nullptr);
    ~Emulator() override;

    bool running() const { return m_running; }
    bool paused() const { return m_paused; }
    void setPaused(bool paused);
    bool fastForward() const { return m_fastForward; }
    void setFastForward(bool on);
    QString gameTitle() const { return m_gameTitle; }
    QString romPath() const { return m_romPath; }
    QString platform() const { return m_platform; }
    int frameWidth() const { return m_frameSize.width(); }
    int frameHeight() const { return m_frameSize.height(); }
    qreal fps() const { return m_fps; }
    bool rumble() const { return m_rumble; }

    const FrameBuffer &frameBuffer() const { return m_frame; }

    // Starts `path`. With `resume`, the automatic save state from the last
    // session is loaded if one exists. Emits started() or failed().
    Q_INVOKABLE void start(const QString &path, bool resume = true);
    Q_INVOKABLE void stop();
    Q_INVOKABLE void reset();
    Q_INVOKABLE void saveState(int slot);
    Q_INVOKABLE void loadState(int slot);
    Q_INVOKABLE void deleteState(int slot);
    Q_INVOKABLE bool stateExists(int slot) const;
    Q_INVOKABLE QUrl stateThumbnail(int slot) const;
    Q_INVOKABLE qint64 stateTime(int slot) const;
    Q_INVOKABLE void setKeys(int keys) { m_keys.store(quint32(keys)); }

    // Library helpers that don't need a running core.
    Q_INVOKABLE bool hasAutoSave(const QString &romPath) const;
    Q_INVOKABLE void deleteAutoSave(const QString &romPath);

    static QString baseName(const QString &romPath);
    static QString thumbnailPath(const QString &romPath);

signals:
    void runningChanged();
    void pausedChanged();
    void fastForwardChanged();
    void gameChanged();
    void frameSizeChanged();
    void fpsChanged();
    void rumbleChanged();
    void frameReady();
    void started();
    void failed(const QString &message);
    void stateSaved(int slot, bool ok);
    void stateLoaded(int slot, bool ok);
    void statesChanged();

    // Emitted on the emulation thread, delivered queued to the main thread.
    void workerStarted(const QString &title, const QString &platform);
    void workerFailed(const QString &message);
    void workerFrame();
    void workerStateSaved(int slot, bool ok);
    void workerStateLoaded(int slot, bool ok);

private:
    struct StartOptions {
        QString path;
        QString biosPath;
        bool resume;
        bool skipBios;
    };

    void run(StartOptions options);
    bool loadGame(const StartOptions &options, QString *error);
    void unloadGame(bool autoSave);
    void runFrames();
    void publishFrame();
    void pumpAudio(bool play);
    void post(std::function<void()> command);
    bool saveStateNow(int slot);
    bool loadStateNow(int slot);
    QString statePath(int slot) const;

    // Main-thread side
    void onStarted(const QString &title, const QString &platform);
    void onFailed(const QString &message);
    void onFrame();
    void applySettings();

    Settings *m_settings;
    AudioOutput *m_audio;
    FrameBuffer m_frame;

    // Emulation thread
    std::thread m_thread;
    std::mutex m_commandMutex;
    std::condition_variable m_wake;
    std::deque<std::function<void()>> m_commands;
    std::atomic<bool> m_quit { false };
    std::atomic<bool> m_pauseRequested { false };
    std::atomic<bool> m_ffRequested { false };
    std::atomic<int> m_ffSpeed { 3 };
    std::atomic<int> m_volume { 256 };
    std::atomic<quint32> m_keys { 0 };
    std::atomic<bool> m_framePending { false };
    std::atomic<bool> m_autoSave { true };
    std::atomic<bool> m_rumbleSeen { false };
    std::atomic<bool> m_rumbleState { false };
    RumbleBridge *m_rumbleBridge;
    mCore *m_core = nullptr;
    quint32 *m_videoBuffer = nullptr;
    bool m_audioPlaying = false;

    // Main thread state mirrored for QML
    bool m_running = false;
    bool m_paused = false;
    bool m_fastForward = false;
    bool m_rumble = false;
    QString m_gameTitle;
    QString m_romPath;
    QString m_platform;
    QSize m_frameSize { 240, 160 };
    qreal m_fps = 0;
    std::atomic<int> m_fpsTimes100 { 0 };
};

#endif // EMULATOR_H
