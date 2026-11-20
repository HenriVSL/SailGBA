#include "emulator.h"

#include "audiooutput.h"
#include "settings.h"

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>

#include <mgba/core/blip_buf.h>
#include <mgba/core/config.h>
#include <mgba/core/core.h>
#include <mgba/core/log.h>
#include <mgba/core/serialize.h>
#include <mgba-util/vfs.h>

#include <cstdio>
#include <cstdlib>
#include <vector>

namespace {

// GBA: 16.78 MHz / 280896 cycles per frame. The GB runs at the same rate.
const double FrameRate = 16777216.0 / 280896.0;
const int MaxWidth = 256;
const int MaxHeight = 224;
// Audio frames to keep queued ahead of the device: about two video frames.
const int AudioTargetFrames = 1600;

void quietLog(mLogger *, int category, mLogLevel level, const char *format, va_list args)
{
    if (!(level & (mLOG_ERROR | mLOG_FATAL)))
        return;
    char message[256];
    vsnprintf(message, sizeof(message), format, args);
    qWarning("mGBA %s: %s", mLogCategoryName(category), message);
}

mLogger logger = { quietLog, nullptr };

} // namespace

struct RumbleBridge : mRumble {
    std::atomic<bool> *seen;
    bool level = false;
};

namespace {

void setRumble(mRumble *rumble, int enable)
{
    RumbleBridge *bridge = static_cast<RumbleBridge *>(rumble);
    bridge->level = enable;
    if (enable)
        bridge->seen->store(true);
}

QString dataSubdir(const char *name)
{
    QDir dir(Settings::dataDir());
    dir.mkpath(QLatin1String(name));
    return dir.filePath(QLatin1String(name));
}

} // namespace

// ---------------------------------------------------------------------------
// FrameBuffer

bool FrameBuffer::take(QImage *out, quint64 *serial) const
{
    QMutexLocker lock(&m_mutex);
    if (m_serial == *serial || m_image.isNull())
        return false;
    *serial = m_serial;
    if (out->size() != m_image.size())
        *out = QImage(m_image.size(), QImage::Format_RGBA8888);
    memcpy(out->bits(), m_image.constBits(), size_t(m_image.byteCount()));
    return true;
}

void FrameBuffer::publish(const quint32 *pixels, int width, int height, int stride)
{
    QMutexLocker lock(&m_mutex);
    if (m_image.width() != width || m_image.height() != height)
        m_image = QImage(width, height, QImage::Format_RGBA8888);
    for (int y = 0; y < height; ++y) {
        const quint32 *src = pixels + y * stride;
        quint32 *dst = reinterpret_cast<quint32 *>(m_image.scanLine(y));
        // mGBA writes 0x00BBGGRR (red in the low byte, as RGBA8888 in
        // memory) and leaves alpha at zero; the compositor needs it opaque.
        for (int x = 0; x < width; ++x)
            dst[x] = src[x] | 0xFF000000u;
    }
    ++m_serial;
}

QImage FrameBuffer::snapshot() const
{
    QMutexLocker lock(&m_mutex);
    return m_image.copy();
}

void FrameBuffer::clear()
{
    QMutexLocker lock(&m_mutex);
    m_image = QImage();
    ++m_serial;
}

QSize FrameBuffer::size() const
{
    QMutexLocker lock(&m_mutex);
    return m_image.size();
}

// ---------------------------------------------------------------------------
// Emulator: main thread side

Emulator::Emulator(Settings *settings, QObject *parent)
    : QObject(parent)
    , m_settings(settings)
    , m_audio(new AudioOutput)
    , m_rumbleBridge(new RumbleBridge)
{
    mLogSetDefaultLogger(&logger);
    m_rumbleBridge->setRumble = setRumble;
    m_rumbleBridge->seen = &m_rumbleSeen;

    connect(settings, &Settings::volumeChanged, this, &Emulator::applySettings);
    connect(settings, &Settings::audioEnabledChanged, this, &Emulator::applySettings);
    connect(settings, &Settings::fastForwardSpeedChanged, this, &Emulator::applySettings);
    connect(settings, &Settings::autoSaveStateChanged, this, &Emulator::applySettings);
    applySettings();

    connect(this, &Emulator::workerStarted, this, &Emulator::onStarted, Qt::QueuedConnection);
    connect(this, &Emulator::workerFailed, this, &Emulator::onFailed, Qt::QueuedConnection);
    connect(this, &Emulator::workerFrame, this, &Emulator::onFrame, Qt::QueuedConnection);
    connect(this, &Emulator::workerStateSaved, this, [this](int generation, int slot, bool ok) {
        if (generation != m_generation)
            return;
        emit stateSaved(slot, ok);
        emit statesChanged();
    }, Qt::QueuedConnection);
    connect(this, &Emulator::workerStateLoaded, this, [this](int generation, int slot, bool ok) {
        if (generation == m_generation)
            emit stateLoaded(slot, ok);
    }, Qt::QueuedConnection);
}

Emulator::~Emulator()
{
    stop();
    delete m_audio;
    delete m_rumbleBridge;
}

void Emulator::applySettings()
{
    m_volume.store(m_settings->volume() * 256 / 100);
    if (m_running) {
        if (m_settings->audioEnabled())
            m_audio->start();
        else
            m_audio->stop();
    }
    m_ffSpeed.store(m_settings->fastForwardSpeed());
    m_autoSave.store(m_settings->autoSaveState());
}

void Emulator::start(const QString &path, bool resume)
{
    stop();

    QString local = path;
    if (local.startsWith(QLatin1String("file://")))
        local = QUrl(local).toLocalFile();

    StartOptions options;
    options.path = local;
    options.biosPath = m_settings->useBios() ? m_settings->biosPath() : QString();
    options.resume = resume && m_settings->autoSaveState();
    options.skipBios = m_settings->skipBiosIntro();
    options.generation = ++m_generation;

    m_romPath = local;
    m_gameTitle = QFileInfo(local).completeBaseName();
    m_platform.clear();
    emit gameChanged();

    m_quit = false;
    m_pauseRequested = m_paused;
    m_ffRequested = m_fastForward;
    m_keys = 0;
    m_framePending = false;
    m_frame.clear();
    {
        // Commands queued for the previous game must not run on this one.
        std::lock_guard<std::mutex> lock(m_commandMutex);
        m_commands.clear();
    }
    m_running = true;
    emit runningChanged();
    m_thread = std::thread(&Emulator::run, this, options);
}

void Emulator::stop()
{
    if (!m_thread.joinable())
        return;
    {
        std::lock_guard<std::mutex> lock(m_commandMutex);
        m_quit = true;
    }
    m_wake.notify_all();
    m_thread.join();
    m_audio->stop();

    if (m_running) {
        m_running = false;
        emit runningChanged();
    }
    if (m_fps != 0) {
        m_fps = 0;
        emit fpsChanged();
    }
    if (m_rumble) {
        m_rumble = false;
        emit rumbleChanged();
    }
    emit statesChanged();
}

void Emulator::setPaused(bool paused)
{
    if (m_paused == paused)
        return;
    m_paused = paused;
    {
        std::lock_guard<std::mutex> lock(m_commandMutex);
        m_pauseRequested = paused;
    }
    m_wake.notify_all();
    m_audio->setSuspended(paused);
    emit pausedChanged();
}

void Emulator::setFastForward(bool on)
{
    if (m_fastForward == on)
        return;
    m_fastForward = on;
    m_ffRequested = on;
    emit fastForwardChanged();
}

void Emulator::post(std::function<void()> command)
{
    if (!m_thread.joinable())
        return;
    {
        std::lock_guard<std::mutex> lock(m_commandMutex);
        m_commands.push_back(std::move(command));
    }
    m_wake.notify_all();
}

void Emulator::reset()
{
    post([this] { m_core->reset(m_core); });
}

void Emulator::saveState(int slot)
{
    post([this, slot] {
        emit workerStateSaved(m_runGeneration, slot, saveStateNow(slot));
    });
}

void Emulator::loadState(int slot)
{
    post([this, slot] {
        emit workerStateLoaded(m_runGeneration, slot, loadStateNow(slot));
    });
}

void Emulator::deleteState(int slot)
{
    QFile::remove(statePath(slot));
    QFile::remove(statePath(slot) + QStringLiteral(".png"));
    emit statesChanged();
}

QString Emulator::baseName(const QString &romPath)
{
    return QFileInfo(romPath).completeBaseName();
}

QString Emulator::thumbnailPath(const QString &romPath)
{
    return QDir(dataSubdir("thumbs")).filePath(baseName(romPath) + QStringLiteral(".png"));
}

QString Emulator::statePath(int slot) const
{
    // Matches the name mCoreGetState() derives: <savestatePath>/<base>.ss<N>
    return QDir(dataSubdir("states")).filePath(baseName(m_romPath) + QStringLiteral(".ss") + QString::number(slot));
}

bool Emulator::stateExists(int slot) const
{
    return !m_romPath.isEmpty() && QFileInfo::exists(statePath(slot));
}

QUrl Emulator::stateThumbnail(int slot) const
{
    const QString png = statePath(slot) + QStringLiteral(".png");
    if (m_romPath.isEmpty() || !QFileInfo::exists(png))
        return QUrl();
    // The query defeats QML's image cache when the slot is overwritten.
    QUrl url = QUrl::fromLocalFile(png);
    url.setQuery(QStringLiteral("t=") + QString::number(QFileInfo(png).lastModified().toMSecsSinceEpoch()));
    return url;
}

qint64 Emulator::stateTime(int slot) const
{
    QFileInfo info(statePath(slot));
    return info.exists() ? info.lastModified().toMSecsSinceEpoch() : 0;
}

bool Emulator::hasAutoSave(const QString &romPath) const
{
    return QFileInfo::exists(QDir(dataSubdir("states")).filePath(baseName(romPath) + QStringLiteral(".ss0")));
}

void Emulator::deleteAutoSave(const QString &romPath)
{
    const QString path = QDir(dataSubdir("states")).filePath(baseName(romPath) + QStringLiteral(".ss0"));
    QFile::remove(path);
    QFile::remove(path + QStringLiteral(".png"));
}

void Emulator::onStarted(int generation, const QString &title, const QString &platform)
{
    if (generation != m_generation)
        return;
    if (!title.isEmpty())
        m_gameTitle = title;
    m_platform = platform;
    if (m_settings->audioEnabled())
        m_audio->start();
    m_audio->setSuspended(m_paused);
    m_settings->touchRecentGame(m_romPath);
    emit gameChanged();
    emit statesChanged();
    emit started();
}

void Emulator::onFailed(int generation, const QString &message)
{
    if (generation != m_generation || !m_running)
        return;
    if (m_thread.joinable())
        m_thread.join();
    m_audio->stop();
    m_running = false;
    emit runningChanged();
    emit failed(message);
}

void Emulator::onFrame(int generation)
{
    m_framePending = false;
    if (generation != m_generation)
        return;
    const QSize size = m_frame.size();
    if (size != m_frameSize && !size.isEmpty()) {
        m_frameSize = size;
        emit frameSizeChanged();
    }
    const qreal fps = m_fpsTimes100.load() / 100.0;
    if (!qFuzzyCompare(fps + 1, m_fps + 1)) {
        m_fps = fps;
        emit fpsChanged();
    }
    const bool rumble = m_rumbleState.load();
    if (rumble != m_rumble) {
        m_rumble = rumble;
        emit rumbleChanged();
    }
    emit frameReady();
}

// ---------------------------------------------------------------------------
// Emulation thread

void Emulator::run(StartOptions options)
{
    m_runGeneration = options.generation;
    QString error;
    if (!loadGame(options, &error)) {
        emit workerFailed(m_runGeneration, error);
        return;
    }

    char title[17] = {};
    m_core->getGameTitle(m_core, title);
    const QString platform = m_core->platform(m_core) == mPLATFORM_GBA ? QStringLiteral("GBA") : QStringLiteral("GB");
    const QString gameTitle = QString::fromLatin1(title).trimmed();
    emit workerStarted(m_runGeneration, gameTitle, platform);

    runFrames();
    unloadGame(m_autoSave.load());
}

bool Emulator::loadGame(const StartOptions &options, QString *error)
{
    m_framesRun = 0;
    const QByteArray path = QFile::encodeName(options.path);
    m_core = mCoreFind(path.constData());
    if (!m_core) {
        *error = tr("Not a Game Boy or Game Boy Advance game");
        return false;
    }
    if (!m_core->init(m_core)) {
        // init() frees what it allocated; only the core struct itself is left.
        free(m_core);
        m_core = nullptr;
        *error = tr("Could not start the emulator core");
        return false;
    }

    mCoreInitConfig(m_core, nullptr);
    const QByteArray saveDir = QFile::encodeName(dataSubdir("saves"));
    const QByteArray stateDir = QFile::encodeName(dataSubdir("states"));
    const QByteArray screenshotDir = QFile::encodeName(dataSubdir("screenshots"));
    mCoreOptions opts = {};
    opts.useBios = !options.biosPath.isEmpty();
    opts.skipBios = options.skipBios;
    opts.volume = 0x100;
    opts.audioBuffers = 1024;
    opts.sampleRate = 48000;
    mCoreConfigLoadDefaults(&m_core->config, &opts);
    // LoadDefaults() skips the directory options; mCoreLoadConfig() maps
    // these onto core->dirs before the ROM's own folder is attached.
    mCoreConfigSetDefaultValue(&m_core->config, "savegamePath", saveDir.constData());
    mCoreConfigSetDefaultValue(&m_core->config, "savestatePath", stateDir.constData());
    mCoreConfigSetDefaultValue(&m_core->config, "screenshotPath", screenshotDir.constData());
    mCoreConfigSetDefaultValue(&m_core->config, "patchPath", saveDir.constData());
    mCoreConfigSetDefaultValue(&m_core->config, "cheatsPath", saveDir.constData());
    // Keep Super Game Boy games at the plain 160x144 screen.
    mCoreConfigSetDefaultIntValue(&m_core->config, "sgb.borders", 0);
    mCoreLoadConfig(m_core);

    m_videoBuffer = new quint32[MaxWidth * MaxHeight]();
    m_core->setVideoBuffer(m_core, m_videoBuffer, MaxWidth);
    m_core->setAudioBufferSize(m_core, 1024);

    if (!mCoreLoadFile(m_core, path.constData())) {
        unloadGame(false);
        *error = tr("Could not read the game file");
        return false;
    }

    if (!options.biosPath.isEmpty() && m_core->platform(m_core) == mPLATFORM_GBA) {
        VFile *bios = VFileOpen(QFile::encodeName(options.biosPath).constData(), O_RDONLY);
        if (bios && !m_core->loadBIOS(m_core, bios, 0))
            bios->close(bios);
    }

    // Saves live in the app's data directory. A .sav next to the ROM (as
    // other emulators leave it) is adopted the first time the game starts.
    const QString base = baseName(options.path);
    const QString ownSave = QDir(QString::fromLocal8Bit(saveDir)).filePath(base + QStringLiteral(".sav"));
    const QString romSave = QFileInfo(options.path).dir().filePath(base + QStringLiteral(".sav"));
    if (!QFileInfo::exists(ownSave) && QFileInfo::exists(romSave))
        QFile::copy(romSave, ownSave);
    mCoreAutoloadSave(m_core);

    m_rumbleBridge->level = false;
    m_rumbleSeen = false;
    m_core->setPeripheral(m_core, mPERIPH_RUMBLE, m_rumbleBridge);

    m_core->reset(m_core);

    if (options.resume)
        loadStateNow(AutoSaveSlot);

    return true;
}

void Emulator::unloadGame(bool autoSave)
{
    if (!m_core)
        return;
    // m_framesRun, not the core's frame counter: a resumed state restores that
    // counter, and quitting before any frame ran would save a blank picture.
    if (autoSave && m_framesRun > 0) {
        saveStateNow(AutoSaveSlot);
        const QImage frame = m_frame.snapshot();
        if (!frame.isNull())
            frame.save(thumbnailPath(m_romPath), "PNG");
    } else if (!autoSave) {
        // A resume state left from before the setting was turned off would
        // otherwise come back, out of date, when it is turned on again.
        QFile::remove(statePath(AutoSaveSlot));
        QFile::remove(statePath(AutoSaveSlot) + QStringLiteral(".png"));
    }
    mCoreConfigDeinit(&m_core->config);
    m_core->deinit(m_core);
    m_core = nullptr;
    delete[] m_videoBuffer;
    m_videoBuffer = nullptr;
    m_audioPlaying = false;
}

bool Emulator::saveStateNow(int slot)
{
    // No SAVESTATE_SAVEDATA: mGBA would restore an embedded battery save as a
    // write-back mask on load, so an old state could overwrite newer in-game
    // saves. Battery saves live only in the .sav file.
    const bool ok = mCoreSaveState(m_core, slot, SAVESTATE_RTC | SAVESTATE_METADATA);
    if (ok) {
        const QImage frame = m_frame.snapshot();
        if (!frame.isNull())
            frame.save(statePath(slot) + QStringLiteral(".png"), "PNG");
    }
    return ok;
}

bool Emulator::loadStateNow(int slot)
{
    // The video buffer is refreshed by the next frame that runs; states don't
    // carry a screenshot (that needs libpng).
    return mCoreLoadState(m_core, slot, SAVESTATE_RTC);
}

void Emulator::publishFrame()
{
    unsigned width = 0, height = 0;
    m_core->desiredVideoDimensions(m_core, &width, &height);
    m_frame.publish(m_videoBuffer, int(width), int(height), MaxWidth);
    if (!m_framePending.exchange(true))
        emit workerFrame(m_runGeneration);
}

void Emulator::pumpAudio(bool play)
{
    blip_t *left = m_core->getAudioChannel(m_core, 0);
    blip_t *right = m_core->getAudioChannel(m_core, 1);
    int available = blip_samples_avail(left);
    if (available <= 0)
        return;
    static thread_local std::vector<qint16> samples;
    samples.resize(size_t(available) * 2);
    const int read = blip_read_samples(left, samples.data(), available, true);
    blip_read_samples(right, samples.data() + 1, available, true);
    if (!play)
        return;
    const int volume = m_volume.load();
    if (volume != 256) {
        for (int i = 0; i < read * 2; ++i)
            samples[size_t(i)] = qint16(samples[size_t(i)] * volume / 256);
    }
    m_audio->write(samples.data(), read);
}

void Emulator::runFrames()
{
    QElapsedTimer clock;
    clock.start();
    qint64 nextFrameNs = 0;
    int framesThisSecond = 0;
    qint64 secondStartNs = 0;
    int frameCounter = 0;
    bool blipRatesSet = false;

    while (!m_quit) {
        // Run queued commands and honour pause.
        {
            std::unique_lock<std::mutex> lock(m_commandMutex);
            if (m_pauseRequested && m_commands.empty()) {
                m_fpsTimes100 = 0;
                m_wake.wait(lock, [this] { return m_quit || !m_pauseRequested || !m_commands.empty(); });
                nextFrameNs = clock.nsecsElapsed();
                secondStartNs = nextFrameNs;
                framesThisSecond = 0;
            }
            while (!m_commands.empty()) {
                auto command = std::move(m_commands.front());
                m_commands.pop_front();
                lock.unlock();
                command();
                lock.lock();
            }
            if (m_quit)
                break;
            if (m_pauseRequested)
                continue;
        }

        // The main thread opens and closes the audio stream; follow it.
        const bool audioActive = m_audio->isActive();
        if (audioActive != m_audioPlaying) {
            m_audioPlaying = audioActive;
            blipRatesSet = false;
        }
        // Dynamic rate control: a clock paces the frames, and the audio is
        // resampled up to 0.5% faster or slower to hold the device buffer at
        // its target. This absorbs the drift between the emulated and the
        // real audio clock without ever stalling video on the sound card.
        if (m_audioPlaying) {
            const double fill = m_audio->bufferedFrames();
            const double error = (AudioTargetFrames - fill) / AudioTargetFrames;
            const double ratio = 1.0 + qBound(-1.0, error, 1.0) * 0.005;
            const double rate = m_audio->sampleRate() * ratio;
            blip_set_rates(m_core->getAudioChannel(m_core, 0), m_core->frequency(m_core), rate);
            blip_set_rates(m_core->getAudioChannel(m_core, 1), m_core->frequency(m_core), rate);
            blipRatesSet = false;
        } else if (!blipRatesSet) {
            blip_set_rates(m_core->getAudioChannel(m_core, 0), m_core->frequency(m_core), 48000);
            blip_set_rates(m_core->getAudioChannel(m_core, 1), m_core->frequency(m_core), 48000);
            blipRatesSet = true;
        }

        const bool ff = m_ffRequested.load();
        const int ffSpeed = m_ffSpeed.load();

        m_core->setKeys(m_core, m_keys.load());
        m_core->runFrame(m_core);
        ++m_framesRun;
        ++frameCounter;
        ++framesThisSecond;

        pumpAudio(m_audioPlaying && !ff);

        // While fast-forwarding, only every few frames reach the screen.
        if (!ff || frameCounter % 4 == 0)
            publishFrame();

        // Pacing
        const qint64 now = clock.nsecsElapsed();
        if (ff && ffSpeed == 0) {
            nextFrameNs = now;
        } else {
            const double rate = ff ? FrameRate * ffSpeed : FrameRate;
            nextFrameNs += qint64(1e9 / rate);
            const qint64 behind = now - nextFrameNs;
            if (behind > 100000000) {
                nextFrameNs = now; // fell far behind (e.g. a stall); don't try to catch up
            } else if (behind < 0) {
                std::unique_lock<std::mutex> lock(m_commandMutex);
                m_wake.wait_for(lock, std::chrono::nanoseconds(-behind),
                                [this] { return m_quit || !m_commands.empty(); });
            }
        }

        const qint64 elapsed = clock.nsecsElapsed() - secondStartNs;
        if (elapsed >= 1000000000) {
            m_fpsTimes100 = int(framesThisSecond * 100.0 * 1e9 / elapsed);
            framesThisSecond = 0;
            secondStartNs = clock.nsecsElapsed();
        }

        // Report rumble as on if the game switched it on at any point in
        // this frame; games drive the motor with rapid on/off pulses.
        m_rumbleState = m_rumbleSeen.exchange(false) || m_rumbleBridge->level;
    }
    m_audioPlaying = false;
}
