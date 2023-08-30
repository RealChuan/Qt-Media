#ifndef DECODER_H
#define DECODER_H

#include <QSharedPointer>
#include <QThread>

#include <utils/boundedblockingqueue.hpp>

#include "avcontextinfo.h"
#include "codeccontext.h"
#include "formatcontext.h"

#define Sleep_Queue_Full_Milliseconds 50

namespace Ffmpeg {

static const auto s_frameQueueSize = 25;

template<typename T>
class Decoder : public QThread
{
public:
    explicit Decoder(QObject *parent = nullptr)
        : QThread(parent)
        , m_queue(s_frameQueueSize)
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
    }

    void append(const T &t) { m_queue.put(t); }
    void append(T &&t) { m_queue.put(t); }

    auto size() -> size_t { return m_queue.size(); }

    void clear() { m_queue.clear(); }

    void wakeup() { m_queue.put(T()); }

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
};

void calculatePts(Frame *frame, AVContextInfo *contextInfo, FormatContext *formatContext);
void calculatePts(Packet *packet, AVContextInfo *contextInfo);

} // namespace Ffmpeg

#endif // DECODER_H
