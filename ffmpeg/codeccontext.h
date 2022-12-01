#ifndef CODECCONTEXT_H
#define CODECCONTEXT_H

#include <QObject>
#include <QSize>

struct AVCodecContext;
struct AVCodecParameters;
struct AVRational;
struct AVCodec;

namespace Ffmpeg {

class AVError;
class Subtitle;
class Packet;
class Frame;
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
    bool receiveFrame(Frame *frame);
    bool decodeSubtitle2(Subtitle *subtitle, Packet *packet);

    int width();
    int height();

    void flush();

    AVError avError();

signals:
    void error(const Ffmpeg::AVError &avError);

private:
    void setError(int errorCode);

    struct CodecContextPrivate;
    QScopedPointer<CodecContextPrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // CODECCONTEXT_H
