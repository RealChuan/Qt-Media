#ifndef CODECCONTEXT_H
#define CODECCONTEXT_H

extern "C"{
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
}

class Packet;
class PlayFrame;
class CodecContext
{
public:
    explicit CodecContext(const AVCodec *codec);
    ~CodecContext();

    AVCodecContext *avCodecCtx();

    bool isOk();

    bool setParameters(const AVCodecParameters *par);
    void setTimebase(const AVRational &timebase);
    bool open(AVCodec *codec);

    bool sendPacket(Packet *packet);
    bool receiveFrame(PlayFrame *frame);

    unsigned char *imageBuffer(PlayFrame &frame);

    int width();
    int height();

private:
    bool m_ok = false;
    AVCodecContext *m_codecCtx; //解码器上下文
};

#endif // CODECCONTEXT_H
