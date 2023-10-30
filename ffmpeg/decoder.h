#ifndef DECODER_H
#define DECODER_H

#include <QSharedPointer>
#include <QThread>

#include <event/event.hpp>
#include <utils/boundedblockingqueue.hpp>
#include <utils/threadsafequeue.hpp>

#include "avcontextinfo.h"
#include "formatcontext.h"

extern "C" {
#include <libavformat/avformat.h>
}

namespace Ffmpeg {

static constexpr auto s_waitQueueEmptyMilliseconds = 50;
static constexpr auto s_videoQueueSize = 10;
// For truehd audio, the queue size should be large enough
static constexpr auto s_audioQueueSize = 200;

template<typename T>
class Decoder : public QThread
{
public:
    explicit Decoder(QObject *parent = nullptr)
        : QThread(parent)
        , m_queue(s_videoQueueSize)
    {}
    ~Decoder() override = default;

    void startDecoder(FormatContext *formatContext, AVContextInfo *contextInfo)
    {
        stopDecoder();
        m_formatContext = formatContext;
        m_contextInfo = contextInfo;
        m_runing = true;
        if (!m_contextInfo->isIndexVaild()) {
            return;
        }
        if (m_contextInfo->stream()->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            m_queue.setMaxSize(s_audioQueueSize);
        } else {
            m_queue.setMaxSize(s_videoQueueSize);
        }
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
        m_eventQueue.clear();
    }

    void append(const T &t)
    {
        assertVaild();
        m_queue.append(t);
    }
    void append(T &&t)
    {
        assertVaild();
        m_queue.append(t);
    }

    auto size() -> size_t { return m_queue.size(); }

    void clear() { m_queue.clear(); }

    void wakeup()
    {
        if (m_queue.empty()) {
            m_queue.insertHead(T());
        }
    }

    void addEvent(const EventPtr &event)
    {
        if (!m_contextInfo->isIndexVaild()) {
            return;
        }
        m_eventQueue.append(event);
        wakeup();
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
    Utils::ThreadSafeQueue<EventPtr> m_eventQueue;
    AVContextInfo *m_contextInfo = nullptr;
    FormatContext *m_formatContext = nullptr;
    std::atomic_bool m_runing = true;
};

} // namespace Ffmpeg

#endif // DECODER_H
