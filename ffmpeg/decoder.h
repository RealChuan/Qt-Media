#ifndef DECODER_H
#define DECODER_H

#include <QThread>

#include <utils/taskqueue.h>

namespace Ffmpeg {

class FormatContext;
class AVContextInfo;

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
        if(isRunning()){
            quit();
            wait();
        }
    }

    void append(const T& t) { m_queue.append(t); }

    int size() { return m_queue.size(); };

protected:
    virtual void runDecoder() = 0;

    void run() override
    {
        Q_ASSERT(m_formatContext != nullptr);
        Q_ASSERT(m_contextInfo != nullptr);
        runDecoder();
    }

    Utils::Queue<T> m_queue;
    AVContextInfo *m_contextInfo;
    FormatContext *m_formatContext;
    bool m_runing = true;
};

}

#endif // DECODER_H
