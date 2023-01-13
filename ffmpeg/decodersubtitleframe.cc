#include "decodersubtitleframe.hpp"
#include "codeccontext.h"

#include <subtitle/ass.hpp>
#include <videorender/videorender.hpp>

#include <QDebug>
#include <QImage>
#include <QWaitCondition>

extern "C" {
#include <libavcodec/avcodec.h>
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
    QSize videoResolutionRatio = QSize(1280, 720);

    QMutex mutex_render;
    QVector<VideoRender *> videoRenders = {};
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

void DecoderSubtitleFrame::setVideoResolutionRatio(const QSize &size)
{
    if (!size.isValid()) {
        return;
    }
    d_ptr->videoResolutionRatio = size;
}

void DecoderSubtitleFrame::setVideoRenders(QVector<VideoRender *> videoRenders)
{
    d_ptr->videoRenders = videoRenders;
}

void DecoderSubtitleFrame::runDecoder()
{
    auto ctx = m_contextInfo->codecCtx()->avCodecCtx();
    QScopedPointer<Ass> assPtr(new Ass);
    assPtr->init(ctx->subtitle_header, ctx->subtitle_header_size);
    assPtr->setWindowSize(d_ptr->videoResolutionRatio);
    SwsContext *swsContext = nullptr;
    quint64 dropNum = 0;
    while (m_runing) {
        checkPause();
        checkSeek(assPtr.data());

        QSharedPointer<Subtitle> subtitlePtr(m_queue.dequeue());
        if (subtitlePtr.isNull()) {
            msleep(Sleep_Queue_Empty_Milliseconds);
            continue;
        }
        subtitlePtr->setVideoResolutionRatio(d_ptr->videoResolutionRatio);
        subtitlePtr->parse(swsContext);
        auto pts = subtitlePtr->pts();
        auto duration = subtitlePtr->duration();
        if (m_seekTime > (pts + duration)) {
            continue;
        }
        if (subtitlePtr->type() == Subtitle::Type::ASS) {
            subtitlePtr->resolveAss(assPtr.data());
            subtitlePtr->generateImage();
        }
        double diffPts = (pts - mediaClock()) * 1000;
        double difDuration = diffPts + duration * 1000;
        if (difDuration < Drop_Milliseconds
            || (mediaSpeed() > 1.0 && qAbs(difDuration) > UnWait_Milliseconds)) {
            dropNum++;
            continue;
        } else if (diffPts > UnWait_Milliseconds && !m_seek && !d_ptr->pause) {
            QMutexLocker locker(&d_ptr->mutex);
            d_ptr->waitCondition.wait(&d_ptr->mutex, diffPts);
        }
        // 略慢于音频
        renderFrame(subtitlePtr);
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

void DecoderSubtitleFrame::checkSeek(Ass *ass)
{
    if (!m_seek) {
        return;
    }
    clear();
    ass->flushASSEvents();
    auto latchPtr = m_latchPtr.lock();
    if (latchPtr) {
        latchPtr->countDown();
    }
    seekFinish();
}

void DecoderSubtitleFrame::renderFrame(const QSharedPointer<Subtitle> &subtitlePtr)
{
    QMutexLocker locker(&d_ptr->mutex_render);
    for (auto render : d_ptr->videoRenders) {
        render->setSubTitleFrame(subtitlePtr);
    }
}

} // namespace Ffmpeg
