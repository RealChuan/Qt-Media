#include "decodersubtitleframe.hpp"

#include <QWaitCondition>

extern "C" {
#include <libswscale/swscale.h>
}

namespace Ffmpeg {

class DecoderSubtitleFrame::DecoderSubtitleFramePrivate
{
public:
    DecoderSubtitleFramePrivate(QObject *parent)
        : owner(parent)
    {}

    QObject *owner;
    bool pause = false;
    QMutex mutex;
    QWaitCondition waitCondition;
};

DecoderSubtitleFrame::DecoderSubtitleFrame(QObject *parent)
    : Decoder<Subtitle *>(parent)
    , d_ptr(new DecoderSubtitleFramePrivate(this))
{}

DecoderSubtitleFrame::~DecoderSubtitleFrame() {}

void DecoderSubtitleFrame::stopDecoder()
{
    pause(false);
    Decoder<Subtitle *>::stopDecoder();
}

void DecoderSubtitleFrame::pause(bool state)
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

void DecoderSubtitleFrame::runDecoder()
{
    SwsContext *swsContext = nullptr;
    quint64 dropNum = 0;
    while (m_runing) {
        checkPause();
        checkSeek();

        QSharedPointer<Subtitle> subtitlePtr(m_queue.dequeue());
        if (subtitlePtr.isNull()) {
            msleep(Sleep_Queue_Empty_Milliseconds);
            continue;
        }
        subtitlePtr->parse(swsContext);
        double pts = subtitlePtr->pts();
        if (m_seekTime > pts) {
            continue;
        }

        double diff = (pts - mediaClock()) * 1000;
        if (qAbs(diff) < UnWait_Milliseconds) {
        } else if (diff > UnWait_Milliseconds && !m_seek && !d_ptr->pause) {
            QMutexLocker locker(&d_ptr->mutex);
            d_ptr->waitCondition.wait(&d_ptr->mutex, diff);
            //msleep(diff);
        } else {
            dropNum++;
            continue;
        }
    }
    sws_freeContext(swsContext);
    qInfo() << dropNum;
}

void DecoderSubtitleFrame::checkPause()
{
    while (d_ptr->pause) {
        QMutexLocker locker(&d_ptr->mutex);
        d_ptr->waitCondition.wait(&d_ptr->mutex);
    }
}

void DecoderSubtitleFrame::checkSeek()
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
