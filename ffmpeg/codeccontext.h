#ifndef CODECCONTEXT_H
#define CODECCONTEXT_H

#include <QObject>

extern "C" {
#include <libavutil/avutil.h>
}

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
    explicit CodecContext(AVCodec *codec, QObject *parent = nullptr);
    ~CodecContext();

    void copyToCodecParameters(CodecContext *dst);

    bool setParameters(const AVCodecParameters *par);
    void setTimebase(const AVRational &timebase);
    void setFrameRate(const AVRational &frameRate);
    // Set before open, Soft solution is effective
    void setThreadCount(int threadCount);
    bool open();

    void setFlags(int flags);
    int flags() const;

    bool sendPacket(Packet *packet);
    bool receiveFrame(Frame *frame);
    bool decodeSubtitle2(Subtitle *subtitle, Packet *packet);

    bool sendFrame(Frame *frame);
    bool receivePacket(Packet *packet);

    int width() const;
    int height() const;
    AVMediaType mediaType() const;
    QString mediaTypeString() const;
    bool isDecoder() const;

    void flush();

    AVError avError();

    AVCodec *codec();
    AVCodecContext *avCodecCtx();

signals:
    void error(const Ffmpeg::AVError &avError);

private:
    void setError(int errorCode);

    struct CodecContextPrivate;
    QScopedPointer<CodecContextPrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // CODECCONTEXT_H
