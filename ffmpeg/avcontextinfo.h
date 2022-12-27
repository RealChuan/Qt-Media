#ifndef AVCONTEXTINFO_H
#define AVCONTEXTINFO_H

#include <QObject>

extern "C" {
#include <libavcodec/codec_id.h>
#include <libavutil/avutil.h>
}

struct AVStream;
struct AVRational;

namespace Ffmpeg {

class AVError;
class Subtitle;
class Packet;
class Frame;
class CodecContext;
class AVContextInfo : public QObject
{
    Q_OBJECT
public:
    explicit AVContextInfo(QObject *parent = nullptr);
    ~AVContextInfo();

    void copyToCodecParameters(AVContextInfo *dst);

    void resetIndex();
    void setIndex(int index);
    int index();

    bool isIndexVaild();

    void setStream(AVStream *stream);
    AVStream *stream();

    bool initDecoder(const AVRational &frameRate);
    bool initEncoder(AVCodecID codecId);
    bool initEncoder(const QString &name);
    bool openCodec(bool useGpu = false);

    bool sendPacket(Packet *packet);
    bool receiveFrame(Frame *frame);
    bool decodeSubtitle2(Subtitle *subtitle, Packet *packet);
    // sendPacket and receiveFrame
    Frame *decodeFrame(Packet *packet);

    void flush();

    double cal_timebase() const;
    AVRational timebase() const;
    double fps() const;
    qint64 fames() const;
    QSize resolutionRatio() const;
    AVMediaType mediaType() const;
    QString mediaTypeString() const;
    bool isDecoder() const;

    bool isGpuDecode();

    CodecContext *codecCtx();

signals:
    void error(const Ffmpeg::AVError &avError);

private:
    void showCodecpar();
    void showMetaData();

    struct AVContextInfoPrivate;
    QScopedPointer<AVContextInfoPrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // AVCONTEXTINFO_H
