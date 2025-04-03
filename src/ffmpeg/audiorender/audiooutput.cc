#include "audiooutput.hpp"

#include <ffmpeg/audioframeconverter.h>
#include <ffmpeg/avcontextinfo.h>
#include <utils/concurrentqueue.hpp>

#include <QApplication>
#include <QAudioSink>
#include <QDebug>
#include <QMediaDevices>

namespace Ffmpeg {

void printAudioOuputDevice()
{
    const auto audioDevices = QMediaDevices::audioOutputs();
    for (const QAudioDevice &audioDevice : std::as_const(audioDevices)) {
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

    void reset()
    {
        printAudioOuputDevice();
        audioDevice = QMediaDevices::defaultAudioOutput();

        int sampleSize = 0;
        auto format = getAudioFormatFromCodecCtx(contextInfo->codecCtx(), sampleSize);
        if (!audioDevice.isFormatSupported(format)) {
            qWarning() << "Raw audio format not supported by backend, cannot play audio.";
            return;
        }
        audioConverterPtr.reset(new AudioFrameConverter(contextInfo->codecCtx(), format));

        audioSinkPtr.reset(new QAudioSink(format));
        audioSinkPtr->setBufferSize(format.sampleRate() * sampleSize / 8);
        audioSinkPtr->setVolume(volume);
        ioDevice = audioSinkPtr->start();
        if (ioDevice == nullptr) {
            qWarning() << "Create AudioDevice Failed!";
        }
        q_ptr->connect(audioSinkPtr.data(),
                       &QAudioSink::stateChanged,
                       q_ptr,
                       &AudioOutput::onStateChanged);
    }

    AudioOutput *q_ptr;

    AVContextInfo *contextInfo;

    QScopedPointer<AudioFrameConverter> audioConverterPtr;

    qreal volume = 0.5;
    QScopedPointer<QAudioSink> audioSinkPtr;
    QIODevice *ioDevice = nullptr;
    QMediaDevices *mediaDevices;
    QAudioDevice audioDevice;
    QByteArray audioBuf;
};

AudioOutput::AudioOutput(AVContextInfo *contextInfo, qreal volume, QObject *parent)
    : QObject{parent}
    , d_ptr(new AudioOutputPrivate(this))
{
    d_ptr->contextInfo = contextInfo;
    d_ptr->volume = volume;
    d_ptr->reset();
    buildConnect();
}

AudioOutput::~AudioOutput() = default;

void AudioOutput::onConvertData(const QSharedPointer<Ffmpeg::Frame> &framePtr)
{
    if (d_ptr->ioDevice == nullptr) {
        return;
    }

    auto audioBuf = d_ptr->audioConverterPtr->convert(framePtr.data());
    d_ptr->audioBuf += audioBuf;
}

void AudioOutput::onWrite()
{
    if (d_ptr->ioDevice == nullptr) {
        return;
    }
    while (d_ptr->audioBuf.size() > 0) {
        auto byteFree = d_ptr->audioSinkPtr->bytesFree();
        if (byteFree > 0 && byteFree < d_ptr->audioBuf.size()) {
            d_ptr->ioDevice->write(d_ptr->audioBuf.data(), byteFree);
            d_ptr->audioBuf = d_ptr->audioBuf.sliced(byteFree);
        } else {
            d_ptr->ioDevice->write(d_ptr->audioBuf);
            d_ptr->audioBuf.clear();
            break;
        }
    }
}

void AudioOutput::onSetVolume(qreal value)
{
    d_ptr->volume = value;
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
    qInfo() << "AudioDevice Change: " << d_ptr->audioDevice.description() << "->"
            << QMediaDevices::defaultAudioOutput().description();
    d_ptr->reset();
}

void AudioOutput::buildConnect()
{
    connect(d_ptr->mediaDevices,
            &QMediaDevices::audioOutputsChanged,
            this,
            &AudioOutput::onAudioOutputsChanged);
}

} // namespace Ffmpeg
