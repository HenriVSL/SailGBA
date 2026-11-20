#include "audiooutput.h"

#include <QAudioOutput>
#include <QDebug>

#include <cstring>

namespace {
const size_t RingFrames = 16384;
// If the device consumes slower than real time (seen on the emulator), keep
// latency bounded by dropping the oldest audio beyond about 100 ms.
const size_t MaxQueuedFrames = 4800;
const int BufferMs = 50;
}

AudioRing::AudioRing(QObject *parent)
    : QIODevice(parent)
    , m_buffer(RingFrames * 2)
{
}

void AudioRing::write(const qint16 *frames, int count)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    for (int i = 0; i < count; ++i) {
        if (m_fill >= MaxQueuedFrames) {
            // Overrun: drop the oldest frame rather than the newest.
            m_read = (m_read + 1) % RingFrames;
            --m_fill;
        }
        const size_t pos = (m_read + m_fill) % RingFrames;
        m_buffer[pos * 2] = frames[i * 2];
        m_buffer[pos * 2 + 1] = frames[i * 2 + 1];
        ++m_fill;
    }
}

int AudioRing::bufferedFrames() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return int(m_fill);
}

void AudioRing::clear()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_read = 0;
    m_fill = 0;
}

qint64 AudioRing::bytesAvailable() const
{
    // Always report data: on an underrun readData pads with silence, which
    // keeps the stream running instead of letting it fall into IdleState.
    return 1 << 16;
}

qint64 AudioRing::readData(char *data, qint64 maxlen)
{
    qint16 *out = reinterpret_cast<qint16 *>(data);
    const size_t wanted = size_t(maxlen) / 4;
    size_t copied = 0;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        while (copied < wanted && m_fill > 0) {
            const size_t chunk = std::min({ wanted - copied, m_fill, RingFrames - m_read });
            std::memcpy(out + copied * 2, &m_buffer[m_read * 2], chunk * 4);
            m_read = (m_read + chunk) % RingFrames;
            m_fill -= chunk;
            copied += chunk;
        }
    }
    if (copied < wanted)
        std::memset(out + copied * 2, 0, (wanted - copied) * 4);
    return qint64(wanted * 4);
}

AudioOutput::AudioOutput(QObject *parent)
    : QObject(parent)
    , m_ring(new AudioRing(this))
{
}

AudioOutput::~AudioOutput()
{
    stop();
}

bool AudioOutput::start()
{
    if (m_active)
        return true;

    QAudioFormat format;
    format.setSampleRate(48000);
    format.setChannelCount(2);
    format.setSampleSize(16);
    format.setSampleType(QAudioFormat::SignedInt);
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setCodec(QStringLiteral("audio/pcm"));

    QAudioDeviceInfo device = QAudioDeviceInfo::defaultOutputDevice();
    if (device.isNull()) {
        qWarning() << "No audio output device";
        return false;
    }
    if (!device.isFormatSupported(format)) {
        format.setSampleRate(44100);
        if (!device.isFormatSupported(format)) {
            qWarning() << "Audio device supports neither 48 kHz nor 44.1 kHz stereo S16";
            return false;
        }
    }
    m_sampleRate = format.sampleRate();

    m_ring->clear();
    m_ring->open(QIODevice::ReadOnly);
    m_output = new QAudioOutput(device, format, this);
    m_output->setCategory(QStringLiteral("game"));
    m_output->setBufferSize(m_sampleRate * 4 * BufferMs / 1000);
    m_output->start(m_ring);
    if (m_output->error() != QAudio::NoError) {
        qWarning() << "Audio output failed to start:" << m_output->error();
        delete m_output;
        m_output = nullptr;
        m_ring->close();
        return false;
    }
    m_active = true;
    return true;
}

void AudioOutput::stop()
{
    if (m_output) {
        m_output->stop();
        delete m_output;
        m_output = nullptr;
    }
    m_ring->close();
    m_ring->clear();
    m_active = false;
}

void AudioOutput::setSuspended(bool suspended)
{
    if (!m_output)
        return;
    if (suspended) {
        m_output->suspend();
        m_ring->clear();
    } else {
        m_output->resume();
    }
}
