#include "audiooutput.hpp"

#include <QApplication>
#include <QAudioSink>
#include <QDebug>
#include <QMediaDevices>

namespace Ffmpeg {

void printAudioOuputDevice()
{
    const auto audioDevices = QMediaDevices::audioOutputs();
    for (const QAudioDevice &audioDevice : qAsConst(audioDevices)) {
        qDebug() << audioDevice.id() << audioDevice.description() << audioDevice.isDefault();
    }
}

class AudioOutput::AudioOutputPrivate
{
public:
    explicit AudioOutputPrivate(AudioOutput *q)
        : q_ptr(q)
    {
        mediaDevices = new QMediaDevices(q_ptr);
    }
    ~AudioOutputPrivate() = default;

    void reset(const Audio::Config &config)
    {
        this->config = config;
        printAudioOuputDevice();
        audioDevice = QMediaDevices::defaultAudioOutput();

        audioSinkPtr.reset(new QAudioSink(config.format));
        audioSinkPtr->setBufferSize(config.bufferSize);
        audioSinkPtr->setVolume(config.volume);
        ioDevice = audioSinkPtr->start();
        if (!ioDevice) {
            qWarning() << "Create AudioDevice Failed!";
        }
        QObject::connect(audioSinkPtr.data(),
                         &QAudioSink::stateChanged,
                         q_ptr,
                         &AudioOutput::onStateChanged);
    }

    AudioOutput *q_ptr;

    Audio::Config config;
    QScopedPointer<QAudioSink> audioSinkPtr;
    QIODevice *ioDevice = nullptr;
    QMediaDevices *mediaDevices;
    QAudioDevice audioDevice;
};

AudioOutput::AudioOutput(const Audio::Config &config, QObject *parent)
    : QObject{parent}
    , d_ptr(new AudioOutputPrivate(this))
{
    d_ptr->reset(config);

    buildConnect();
}

AudioOutput::~AudioOutput() = default;

void AudioOutput::onWrite(const QByteArray &audioBuf)
{
    if (!d_ptr->ioDevice) {
        return;
    }
    auto buf = audioBuf;
    while (buf.size() > 0) {
        int byteFree = d_ptr->audioSinkPtr->bytesFree();
        if (byteFree > 0 && byteFree < buf.size()) {
            d_ptr->ioDevice->write(buf.data(), byteFree); // memory leak
            buf = buf.sliced(byteFree);
        } else {
            d_ptr->ioDevice->write(buf);
            break;
        }
    }
    qApp->processEvents(); // fix memory leak
}

void AudioOutput::onSetVolume(qreal value)
{
    d_ptr->config.volume = value;
    if (d_ptr->audioSinkPtr.isNull()) {
        return;
    }
    d_ptr->audioSinkPtr->setVolume(value);
}

void AudioOutput::onStateChanged(QAudio::State state)
{
    Q_UNUSED(state)
    if (d_ptr->audioSinkPtr.isNull()) {
        return;
    }
    if (d_ptr->audioSinkPtr->error() != QAudio::NoError) {
        qWarning() << tr("QAudioSink Error: ") << state << d_ptr->audioSinkPtr->error();
    }
}

void AudioOutput::onAudioOutputsChanged()
{
    if (d_ptr->audioDevice == QMediaDevices::defaultAudioOutput()) {
        return;
    }
    d_ptr->reset(d_ptr->config);
}

void AudioOutput::buildConnect()
{
    connect(d_ptr->mediaDevices,
            &QMediaDevices::audioOutputsChanged,
            this,
            &AudioOutput::onAudioOutputsChanged);
}

} // namespace Ffmpeg
