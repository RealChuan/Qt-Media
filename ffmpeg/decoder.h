#ifndef DECODER_H
#define DECODER_H

#include <QSharedPointer>
#include <QThread>

#include <utils/countdownlatch.hpp>
#include <utils/taskqueue.h>

#include "avcontextinfo.h"
#include "codeccontext.h"
#include "formatcontext.h"

#define Sleep_Queue_Full_Milliseconds 50
#define Sleep_Queue_Empty_Milliseconds 10
#define Max_Frame_Size 25
#define UnWait_Microseconds (50 * 1000)
#define Drop_Microseconds (-100 * 1000)

namespace Ffmpeg {

void calculateTime(Frame *frame, AVContextInfo *contextInfo, FormatContext *formatContext);
void calculateTime(Packet *packet, AVContextInfo *contextInfo);

void setMediaClock(qint64 value);
auto mediaClock() -> qint64;

void setMediaSpeed(double speed);
auto mediaSpeed() -> double;

template<typename T>
class Decoder : public QThread
{
public:
    explicit Decoder(QObject *parent = nullptr)
        : QThread(parent)
    {}
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
        if (isRunning()) {
            quit();
            wait();
        }
        clear();
        m_seekTime = 0;
    }

    void append(const T &t) { m_queue.enqueue(t); }

    auto size() -> int { return m_queue.size(); }

    void clear() { m_queue.clearPoints(); }

    virtual void pause(bool state) = 0;

    void seek(qint64 seekTime, QSharedPointer<Utils::CountDownLatch> latchPtr) // microsecond
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

    auto isSeek() -> bool { return m_seek; }

protected:
    virtual void runDecoder() = 0;

    void run() final
    {
        assertVaild();
        if (!m_contextInfo->isIndexVaild()) {
            return;
        }
        runDecoder();
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
    QWeakPointer<Utils::CountDownLatch> m_latchPtr;
};

} // namespace Ffmpeg

#endif // DECODER_H
