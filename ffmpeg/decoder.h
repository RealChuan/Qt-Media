#ifndef DECODER_H
#define DECODER_H

#include <QThread>
#include <QDebug>
#include <QWaitCondition>

#include <utils/taskqueue.h>

#include "avcontextinfo.h"
#include "formatcontext.h"

extern "C"{
#include <libavutil/time.h>
#include <libavformat/avformat.h>
}

#define Seek_Error_Time 0.99

namespace Ffmpeg {

template<typename T>
class Decoder : public QThread
{
public:
    Decoder(QObject *parent = nullptr) : QThread(parent) {}
    ~Decoder() override { stopDecoder(); }

    void startDecoder(FormatContext *formatContext, AVContextInfo *contextInfo)
    {
        stopDecoder();
        m_formatContext = formatContext;
        m_contextInfo = contextInfo;
        m_runing = true;
        start();
    }

    virtual void stopDecoder()
    {
        m_runing = false;
        if(isRunning()){
            quit();
            wait();
        }
        clear();
    }

    void append(const T& t) { m_queue.append(t); }

    int size() { return m_queue.size(); };

    void clear() { m_queue.clear(); }

    void seek(qint64 seekTime)
    {
        m_seek = true;
        m_seekTime = seekTime;
        clear();
        while (m_seek) {
            QMutexLocker locker(&m_mutex);
            m_waitCondition.wait(&m_mutex);
        }
    }

    bool isSeek() { return m_seek; }

protected:
    virtual void runDecoder() = 0;

    void run() override
    {
        Q_ASSERT(m_formatContext != nullptr);
        Q_ASSERT(m_contextInfo != nullptr);
        runDecoder();
    }

    void calculateTime(AVFrame *frame, double &duration, double &pts)
    {
        AVRational tb = m_contextInfo->stream()->time_base;
        AVRational frame_rate = av_guess_frame_rate(m_formatContext->avFormatContext(), m_contextInfo->stream(), NULL);
        // 当前帧播放时长
        duration = (frame_rate.num && frame_rate.den ? av_q2d(AVRational{frame_rate.den, frame_rate.num}) : 0);
        // 当前帧显示时间戳
        pts = (frame->pts == AV_NOPTS_VALUE) ? NAN : frame->pts * av_q2d(tb);
        //qDebug() << duration << pts;
    }

    void seekCodec(qint64 seekTime)
    {
        m_formatContext->seek(m_contextInfo->index(), seekTime / m_contextInfo->timebase());
        m_contextInfo->flush();
    }

    void seekFinish()
    {
        m_seek = false;
        m_waitCondition.wakeOne();
    }

    Utils::Queue<T> m_queue;
    AVContextInfo *m_contextInfo;
    FormatContext *m_formatContext;
    bool m_runing = true;
    bool m_seek = false;
    qint64 m_seekTime = 0; // seconds
    QMutex m_mutex;
    QWaitCondition m_waitCondition;
};

}

#endif // DECODER_H
