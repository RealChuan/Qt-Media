#include "decoderaudioframe.h"
#include "audioframeconverter.h"
#include "clock.hpp"
#include "codeccontext.h"

#include <QAudioDevice>
#include <QAudioSink>
#include <QCoreApplication>
#include <QMediaDevices>
#include <QMediaFormat>
#include <QTime>
#include <QWaitCondition>

extern "C" {
#include <libavutil/time.h>
}

namespace Ffmpeg {

class DecoderAudioFrame::DecoderAudioFramePrivate
{
public:
    explicit DecoderAudioFramePrivate(DecoderAudioFrame *q)
        : q_ptr(q)
    {
        clock = new Clock(q);
    }
    ~DecoderAudioFramePrivate() = default;

    DecoderAudioFrame *q_ptr;

    QScopedPointer<QAudioSink> audioSinkPtr;
    QIODevice *audioDevice = nullptr;
    qreal volume = 0.5;

    Clock *clock;
    QMutex mutex;
    QWaitCondition waitCondition;

    QScopedPointer<QMediaDevices> mediaDevicesPtr;
    std::atomic_bool audioOutputsChanged = false;
};

DecoderAudioFrame::DecoderAudioFrame(QObject *parent)
    : Decoder<FramePtr>(parent)
    , d_ptr(new DecoderAudioFramePrivate(this))
{
    buildConnect();
}

DecoderAudioFrame::~DecoderAudioFrame() = default;

void DecoderAudioFrame::setVolume(qreal volume)
{
    d_ptr->volume = volume;
    if (d_ptr->audioSinkPtr) {
        d_ptr->audioSinkPtr->setVolume(volume);
    }
}

void DecoderAudioFrame::setMasterClock()
{
    Clock::setMaster(d_ptr->clock);
}

void DecoderAudioFrame::onStateChanged(QAudio::State state)
{
    Q_UNUSED(state)
    if (d_ptr->audioSinkPtr.isNull()) {
        return;
    }
    if (d_ptr->audioSinkPtr->error() != QAudio::NoError) {
        qWarning() << tr("QAudioSink Error:") << state << d_ptr->audioSinkPtr->error();
    }
    //    switch (state) {
    //    case QAudio::StoppedState:
    //        // Stopped for other reasons
    //        if (d_ptr->audioSink->error() != QAudio::NoError) {
    //            qWarning() << tr("QAudioSink Error:") << d_ptr->audioSink->error();
    //        }
    //        break;
    //    default:
    //        // ... other cases as appropriate
    //        break;
    //    }
}

void DecoderAudioFrame::onAudioOutputsChanged()
{
    d_ptr->audioOutputsChanged = true;
}

void DecoderAudioFrame::runDecoder()
{
    quint64 dropNum = 0;
    QAudioDevice audioDevice(QMediaDevices::defaultAudioOutput());
    auto format = resetAudioOutput();
    AudioFrameConverter audioConverter(m_contextInfo->codecCtx(), format);
    d_ptr->clock->reset();
    while (m_runing.load()) {
        checkDefaultAudioOutput(audioDevice);

        auto framePtr(m_queue.take());
        if (framePtr.isNull()) {
            continue;
        }
        auto audioBuf = audioConverter.convert(framePtr.data());
        auto pts = framePtr->pts();
        auto emitPosition = qScopeGuard([=]() { emit positionChanged(pts); });
        d_ptr->clock->update(pts, av_gettime_relative());
        qint64 delay = 0;
        if (!d_ptr->clock->getDelayWithMaster(delay)) {
            continue;
        }
        if (!Clock::adjustDelay(delay)) {
            dropNum++;
            continue;
        }
        delay /= 1000;
        if (delay > 0) {
            QMutexLocker locker(&d_ptr->mutex);
            d_ptr->waitCondition.wait(&d_ptr->mutex, delay);
        }
        // qDebug() << "Audio PTS:"
        //          << QTime::fromMSecsSinceStartOfDay(pts / 1000).toString("hh:mm:ss.zzz");

        writeToDevice(audioBuf);
    }
    d_ptr->audioSinkPtr.reset();
    qInfo() << "Audio Drop Num:" << dropNum;
}

void DecoderAudioFrame::buildConnect()
{
    d_ptr->mediaDevicesPtr.reset(new QMediaDevices);
    connect(d_ptr->mediaDevicesPtr.data(),
            &QMediaDevices::audioOutputsChanged,
            this,
            &DecoderAudioFrame::onAudioOutputsChanged);
}

void DecoderAudioFrame::checkDefaultAudioOutput(QAudioDevice &audioDevice)
{
    if (!d_ptr->audioOutputsChanged) {
        return;
    }
    d_ptr->audioOutputsChanged = false;
    if (audioDevice == QMediaDevices::defaultAudioOutput()) {
        return;
    }
    resetAudioOutput();
    audioDevice = QMediaDevices::defaultAudioOutput();
}

void DecoderAudioFrame::writeToDevice(QByteArray &audioBuf)
{
    if (!d_ptr->audioDevice) {
        return;
    }
    while (audioBuf.size() > 0) {
        int byteFree = d_ptr->audioSinkPtr->bytesFree();
        if (byteFree > 0 && byteFree < audioBuf.size()) {
            d_ptr->audioDevice->write(audioBuf.data(), byteFree); // Memory leak
            audioBuf = audioBuf.sliced(byteFree);
        } else {
            d_ptr->audioDevice->write(audioBuf);
            break;
        }
    }
    qApp->processEvents();
}

void printAudioOuputDevice()
{
    QList<QAudioDevice> audioDevices = QMediaDevices::audioOutputs();
    for (const QAudioDevice &audioDevice : qAsConst(audioDevices)) {
        qDebug() << audioDevice.id() << audioDevice.description() << audioDevice.isDefault();
    }
}

QAudioFormat DecoderAudioFrame::resetAudioOutput()
{
    printAudioOuputDevice();

    int sampleSzie = 0;
    auto format = getAudioFormatFromCodecCtx(m_contextInfo->codecCtx(), sampleSzie);
    d_ptr->audioSinkPtr.reset(new QAudioSink(format));
    d_ptr->audioSinkPtr->setBufferSize(format.sampleRate() * sampleSzie / 8);
    d_ptr->audioSinkPtr->setVolume(d_ptr->volume);
    d_ptr->audioDevice = d_ptr->audioSinkPtr->start();
    if (!d_ptr->audioDevice) {
        qWarning() << tr("Create AudioDevice Failed!");
    }
    connect(d_ptr->audioSinkPtr.data(),
            &QAudioSink::stateChanged,
            this,
            &DecoderAudioFrame::onStateChanged);

    return format;
}

} // namespace Ffmpeg
