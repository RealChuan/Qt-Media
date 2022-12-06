#include "subtitledecoder.h"
#include "decoderaudioframe.h"

namespace Ffmpeg {

class SubtitleDecoder::SubtitleDecoderPrivate
{
public:
    SubtitleDecoderPrivate(QObject *parent)
        : owner(parent)
    {}

    QObject *owner;
    bool pause = false;
    QMutex mutex;
    QWaitCondition waitCondition;
};

SubtitleDecoder::SubtitleDecoder(QObject *parent)
    : Decoder<Packet *>(parent)
    , d_ptr(new SubtitleDecoderPrivate(this))
{
    qRegisterMetaType<QVector<Ffmpeg::SubtitleImage>>("QVector<Ffmpeg::SubtitleImage>");
}

SubtitleDecoder::~SubtitleDecoder() {}

void SubtitleDecoder::stopDecoder()
{
    pause(false);
    Decoder<Packet *>::stopDecoder();
}

void SubtitleDecoder::pause(bool state)
{
    if (!isRunning()) {
        return;
    }
    d_ptr->pause = state;
    if (state) {
        return;
    }
    d_ptr->waitCondition.wakeOne();
}

void SubtitleDecoder::runDecoder()
{
    Subtitle subtitle;

    while (m_runing) {
        checkPause();
        checkSeek();

        QScopedPointer<Packet> packetPtr(m_queue.dequeue());
        if (packetPtr.isNull()) {
            msleep(Sleep_Queue_Empty_Milliseconds);
            continue;
        }
        if (!m_contextInfo->decodeSubtitle2(&subtitle, packetPtr.data())) {
            continue;
        }

        Ffmpeg::calculateTime(packetPtr.data(), m_contextInfo);
        subtitle.setDefault(packetPtr->pts(),
                            packetPtr->duration(),
                            (const char *) packetPtr->avPacket()->data);
        QVector<SubtitleImage> subtitles = subtitle.subtitleImages();
        subtitle.clear();
        if (subtitles.isEmpty()) {
            continue;
        }

        double audioPts = clock() * 1000;
        double diff = subtitles.at(0).startDisplayTime - audioPts;
        if (diff > 0) {
            QMutexLocker locker(&d_ptr->mutex);
            d_ptr->waitCondition.wait(&d_ptr->mutex, diff);
            //msleep(diff);
        } else if (audioPts > subtitles.at(0).endDisplayTime) {
            continue;
        }

        emit subtitleImages(subtitles);
    }
}

void SubtitleDecoder::checkPause()
{
    while (d_ptr->pause) {
        QMutexLocker locker(&d_ptr->mutex);
        d_ptr->waitCondition.wait(&d_ptr->mutex);
    }
}

void SubtitleDecoder::checkSeek()
{
    if (!m_seek) {
        return;
    }
    clear();
    auto latchPtr = m_latchPtr.lock();
    if (latchPtr) {
        latchPtr->countDown();
    }
    seekFinish();
}

} // namespace Ffmpeg
