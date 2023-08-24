#ifndef DECODER_H
#define DECODER_H

#include <QSharedPointer>
#include <QThread>

#include <utils/boundedblockingqueue.hpp>

#include "avcontextinfo.h"
#include "codeccontext.h"
#include "formatcontext.h"

#define Sleep_Queue_Full_Milliseconds 50
#define UnWait_Microseconds (50 * 1000)
#define Drop_Microseconds (-100 * 1000)

namespace Ffmpeg {

static const auto g_frameQueueSize = 25;

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
        , m_queue(g_frameQueueSize)
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
        clear();
        wakeup();
        if (isRunning()) {
            quit();
            wait();
        }
        m_seekTime = 0;
    }

    void append(const T &t) { m_queue.put(t); }
    void append(const T &&t) { m_queue.put(t); }

    auto size() -> size_t { return m_queue.size(); }

    void clear() { m_queue.clear(); }

    void wakeup() { m_queue.put(T()); }

    virtual void pause(bool state) = 0;

    virtual void seek(qint64 seekTime) // microsecond
    {
        clear();
        m_seekTime = seekTime;
        assertVaild();
        if (!m_contextInfo->isIndexVaild()) {
            return;
        }
        pause(false);
    }

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

    void assertVaild()
    {
        Q_ASSERT(m_formatContext != nullptr);
        Q_ASSERT(m_contextInfo != nullptr);
    }

    Utils::BoundedBlockingQueue<T> m_queue;
    AVContextInfo *m_contextInfo = nullptr;
    FormatContext *m_formatContext = nullptr;
    std::atomic_bool m_runing = true;
    qint64 m_seekTime = 0; // microsecond
};

} // namespace Ffmpeg

#endif // DECODER_H
