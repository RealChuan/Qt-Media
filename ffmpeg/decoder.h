#ifndef DECODER_H
#define DECODER_H

#include <QThread>

#include <utils/taskqueue.h>

#include "packet.h"

namespace Ffmpeg {

class PlayFrame;
class FormatContext;
class AVContextInfo;
class DecoderPrivate;
class Decoder : public QThread
{
    Q_OBJECT
public:
    Decoder(QObject *parent = nullptr);
    ~Decoder() override;

    void setFormatContext(FormatContext *formatContext) { m_formatContext = formatContext; }
    FormatContext *formatContext() { return m_formatContext; }

    void startDecoder(AVContextInfo *contextInfo);
    void stopDecoder();

    void append(const Packet& packet) { m_packetQueue.append(packet); }

    int size() { return m_packetQueue.size(); };

protected:
    virtual void runDecoder() = 0;
    void run() override;

protected:
    Utils::Queue<Packet> m_packetQueue;
    AVContextInfo *m_contextInfo;
    FormatContext *m_formatContext;
    bool m_runing = true;
};

}

#endif // DECODER_H
