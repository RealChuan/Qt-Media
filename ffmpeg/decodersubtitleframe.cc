#include "decodersubtitleframe.hpp"
#include "codeccontext.h"

#include <subtitle/ass.hpp>
#include <videorender/videorender.hpp>

#include <QDir>
#include <QImage>
#include <QStandardPaths>
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
    QSize videoResolutionRatio = QSize(1280, 720);

    QVector<VideoRender *> videoRenders;
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

void DecoderSubtitleFrame::setVideoOutputRenders(QVector<VideoRender *> videoRenders)
{
    d_ptr->videoRenders = videoRenders;
}

void DecoderSubtitleFrame::runDecoder()
{
    //--------------------------test-------------------------------------------------------
    auto ctx = m_contextInfo->codecCtx()->avCodecCtx();
    QScopedPointer<Ass> assPtr(new Ass);
    assPtr->init(ctx->extradata, ctx->extradata_size);
    assPtr->setWindowSize(d_ptr->videoResolutionRatio);
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
        subtitlePtr->setVideoResolutionRatio(d_ptr->videoResolutionRatio);
        subtitlePtr->parse(swsContext);
        double pts = subtitlePtr->pts();
        if (m_seekTime > pts) {
            continue;
        }
        //--------------------------test-------------------------------------------------------
        if (subtitlePtr->type() == Subtitle::Type::ASS) {
            subtitlePtr->resolveAss(assPtr.data());
            //            auto list = subtitlePtr->list();
            //            for (const auto &data : qAsConst(list)) {
            //                auto rect = data.rect();
            //                QImage image((uchar *) data.rgba().constData(),
            //                             rect.width(),
            //                             rect.height(),
            //                             QImage::Format_RGBA8888);
            //                if (image.isNull()) {
            //                    qWarning() << "image is null";
            //                }
            //                auto path = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation)
            //                                .value(0, QDir::homePath());
            //                static int num = 0;
            //                path = QString("%1/1/%2.png").arg(path, QString::number(++num));
            //                qDebug() << rect << image.save(path);
            //            }
        }
        //--------------------------test-------------------------------------------------------
        double diff = (pts - mediaClock()) * 1000;
        if (diff < Drop_Milliseconds || (mediaSpeed() > 1.0 && qAbs(diff) > UnWait_Milliseconds)) {
            dropNum++;
            continue;
        } else if (diff > UnWait_Milliseconds && !m_seek && !d_ptr->pause) {
            QMutexLocker locker(&d_ptr->mutex);
            d_ptr->waitCondition.wait(&d_ptr->mutex, diff);
        }
        // 略慢于音频
        for (auto render : d_ptr->videoRenders) {
            render->setSubTitleFrame(subtitlePtr);
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
