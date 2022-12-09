#include "decodersubtitleframe.hpp"
#include "codeccontext.h"

#include <subtitle/ass.hpp>

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
    //--------------------------test-------------------------------------------------------
    int num = 0;
    auto ctx = m_contextInfo->codecCtx()->avCodecCtx();
    QScopedPointer<Ass> assPtr(new Ass);
    assPtr->init(ctx->extradata, ctx->extradata_size);
    assPtr->setWindowSize(QSize(1280, 720));
    //--------------------------test-------------------------------------------------------
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
        //--------------------------test-------------------------------------------------------
        auto list = subtitlePtr->texts();
        for (const auto &data : qAsConst(list)) {
            assPtr->addSubtitleData(data);
        }
        AssDataInfoList assDataInfoList;
        assPtr->getRGBAData(assDataInfoList, pts);
        qDebug() << assDataInfoList.size() << pts;
        for (const auto &data : qAsConst(assDataInfoList)) {
            auto rect = data.rect();
            QImage image((uchar *) data.rgba().constData(),
                         rect.width(),
                         rect.height(),
                         QImage::Format_RGBA8888);
            if (image.isNull()) {
                qWarning() << "image is null";
            }
            const QString path = QString("C:/Users/Administrator/Pictures/zzz/%1.png").arg(++num);
            qDebug() << rect << image.save(path);
        }
        //--------------------------test-------------------------------------------------------
        double diff = (pts - mediaClock()) * 1000;
        if (diff < Drop_Milliseconds || (m_speed > 1.0 && qAbs(diff) > UnWait_Milliseconds)) {
            dropNum++;
            continue;
        } else if (diff > UnWait_Milliseconds && !m_seek && !d_ptr->pause) {
            QMutexLocker locker(&d_ptr->mutex);
            d_ptr->waitCondition.wait(&d_ptr->mutex, diff);
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
