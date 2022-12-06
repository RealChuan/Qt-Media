#ifndef DECODER_H
#define DECODER_H

#include <QDebug>
#include <QThread>

#include <utils/countdownlatch.hpp>
#include <utils/taskqueue.h>

#include "avcontextinfo.h"
#include "formatcontext.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/time.h>
}

#define Sleep_Queue_Full_Milliseconds 100
#define Sleep_Queue_Empty_Milliseconds 10
#define Max_Frame_Size 25
#define UnWait_Milliseconds 50

namespace Ffmpeg {

void calculateTime(Frame *frame, AVContextInfo *contextInfo, FormatContext *formatContext);
void calculateTime(Packet *packet, AVContextInfo *contextInfo);

template<typename T>
class Decoder : public QThread
{
public:
    Decoder(QObject *parent = nullptr)
        : QThread(parent)
    {}
    virtual ~Decoder() override { stopDecoder(); }

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
        if (isRunning()) {
            quit();
            wait();
        }
        clear();
        m_seekTime = 0;
    }

    void append(const T &t) { m_queue.enqueue(t); }

    int size() { return m_queue.size(); }

    void clear() { m_queue.clearPoints(); }

    virtual void pause(bool state) = 0;

    void seek(qint64 seekTime, QSharedPointer<Utils::CountDownLatch> latchPtr)
    {
        assertVaild();
        if (!m_contextInfo->isIndexVaild()) {
            latchPtr->countDown();
            return;
        }
        m_seek = true;
        m_seekTime = seekTime;
        m_latchPtr = latchPtr;
        pause(false);
    }

    bool isSeek() { return m_seek; }

    virtual void setSpeed(double speed)
    {
        Q_ASSERT(speed > 0);
        m_speed.store(speed);
    }

    double speed() { return m_speed.load(); }

    static void setClock(double value) { m_clock.store(value); }
    static double clock() { return m_clock.load(); }

protected:
    virtual void runDecoder() = 0;

    void run() override final
    {
        assertVaild();
        if (!m_contextInfo->isIndexVaild()) {
            return;
        }
        runDecoder();
    }

    void seekCodec(qint64 seekTime)
    {
        m_formatContext->seek(m_contextInfo->index(), seekTime / m_contextInfo->timebase());
        m_contextInfo->flush();
    }

    void seekFinish() { m_seek = false; }

    void assertVaild()
    {
        Q_ASSERT(m_formatContext != nullptr);
        Q_ASSERT(m_contextInfo != nullptr);
    }

    Utils::Queue<T> m_queue;
    AVContextInfo *m_contextInfo = nullptr;
    FormatContext *m_formatContext = nullptr;
    volatile bool m_runing = true;
    volatile bool m_seek = false;
    qint64 m_seekTime = 0; // seconds
    std::atomic<double> m_speed = 1.0;
    QWeakPointer<Utils::CountDownLatch> m_latchPtr;

private:
    static std::atomic<double> m_clock;
};

template<typename T>
std::atomic<double> Decoder<T>::m_clock = 0;

} // namespace Ffmpeg

#endif // DECODER_H
