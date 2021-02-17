#ifndef CODECCONTEXT_H
#define CODECCONTEXT_H

#include <QObject>

struct AVCodecContext;
struct AVCodecParameters;
struct AVRational;
struct AVCodec;

namespace Ffmpeg {

class Subtitle;
class Packet;
class PlayFrame;
class CodecContext : public QObject
{
    Q_OBJECT
public:
    explicit CodecContext(const AVCodec *codec, QObject *parent = nullptr);
    ~CodecContext();

    AVCodecContext *avCodecCtx();

    bool setParameters(const AVCodecParameters *par);
    void setTimebase(const AVRational &timebase);
    bool open(AVCodec *codec);

    bool sendPacket(Packet *packet);
    bool receiveFrame(PlayFrame *frame);
    bool decodeSubtitle2(Subtitle *subtitle, Packet *packet);

    unsigned char *imageBuffer(PlayFrame &frame);
    void clearImageBuffer();

    int width();
    int height();

    void flush();

private:
    AVCodecContext *m_codecCtx; //解码器上下文
    unsigned char *m_out_buffer;
};

}

#endif // CODECCONTEXT_H
