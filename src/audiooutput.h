#ifndef AUDIOOUTPUT_H
#define AUDIOOUTPUT_H

#include <QIODevice>
#include <QObject>

#include <atomic>
#include <mutex>
#include <vector>

class QAudioOutput;

// Stereo 16-bit PCM ring buffer that QAudioOutput pulls from. The emulation
// thread writes into it and watches the fill level to steer its resampling
// rate, which keeps latency low without the audio clock dictating speed.
class AudioRing : public QIODevice
{
    Q_OBJECT
public:
    explicit AudioRing(QObject *parent = nullptr);

    void write(const qint16 *frames, int count);
    int bufferedFrames() const;
    void clear();

    bool isSequential() const override { return true; }
    qint64 bytesAvailable() const override;

protected:
    qint64 readData(char *data, qint64 maxlen) override;
    qint64 writeData(const char *, qint64) override { return -1; }

private:
    mutable std::mutex m_mutex;
    std::vector<qint16> m_buffer; // interleaved L/R
    size_t m_read = 0;
    size_t m_fill = 0; // in frames
};

class AudioOutput : public QObject
{
    Q_OBJECT
public:
    explicit AudioOutput(QObject *parent = nullptr);
    ~AudioOutput() override;

    // Opens the PulseAudio stream. Returns false if no usable device exists;
    // emulation is then paced with a timer instead. Main thread only: that
    // is where Sailfish's audio resource policy expects the stream to live.
    bool start();
    void stop();
    void setSuspended(bool suspended);

    // Thread-safe.
    bool isActive() const { return m_active.load(); }
    int sampleRate() const { return m_sampleRate.load(); }

    // Thread-safe: called from the emulation thread.
    void write(const qint16 *frames, int count) { m_ring->write(frames, count); }
    int bufferedFrames() const { return m_ring->bufferedFrames(); }
    void clear() { m_ring->clear(); }

private:
    AudioRing *m_ring;
    QAudioOutput *m_output = nullptr;
    std::atomic<bool> m_active { false };
    std::atomic<int> m_sampleRate { 48000 };
};

#endif // AUDIOOUTPUT_H
