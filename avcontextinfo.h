#ifndef AVCONTEXTINFO_H
#define AVCONTEXTINFO_H

#include <QObject>

extern "C"{
#include <libavformat/avformat.h>
}

class Packet;
class PlayFrame;
class CodecContext;
struct AVContextInfoPrivate;
class AVContextInfo : public QObject
{
public:
    AVContextInfo(QObject *parent = nullptr);
    ~AVContextInfo();

    QString error() const;
    CodecContext *codecCtx();

    void setIndex(int index);
    int index();

    void setStream(AVStream *stream);
    bool findDecoder();

    bool sendPacket(Packet *packet);
    bool receiveFrame(PlayFrame *frame);

    unsigned char *imageBuffer(PlayFrame &frame);

private:
    QScopedPointer<AVContextInfoPrivate> d_ptr;
};

#endif // AVCONTEXTINFO_H
