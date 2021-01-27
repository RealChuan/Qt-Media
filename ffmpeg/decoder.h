#ifndef DECODER_H
#define DECODER_H

#include <QThread>
#include <QDebug>

#include <utils/taskqueue.h>

#include "avcontextinfo.h"
#include "formatcontext.h"

extern "C"{
#include <libavutil/time.h>
#include <libavformat/avformat.h>
}

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

    void stopDecoder()
    {
        m_runing = false;
        clear();
        if(isRunning()){
            quit();
            wait();
        }
    }

    void append(const T& t) { m_queue.append(t); }

    int size() { return m_queue.size(); };

    void clear() { m_queue.clear(); }

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

    Utils::Queue<T> m_queue;
    AVContextInfo *m_contextInfo;
    FormatContext *m_formatContext;
    bool m_runing = true;
};

}

#endif // DECODER_H
