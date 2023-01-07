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

class Subtitle;
class Packet;
class Frame;
class CodecContext;
class AVContextInfo : public QObject
{
public:
    explicit AVContextInfo(QObject *parent = nullptr);
    ~AVContextInfo();

    void copyToCodecParameters(AVContextInfo *dst);

    // encoder
    void setSize(const QSize &size);
    void setQuailty(int quailty);
    void setMinBitrate(int64_t bitrate);
    void setMaxBitrate(int64_t bitrate);
    void setCrf(int crf);
    void setPreset(const QString &preset);
    void setTune(const QString &tune);
    void setProfile(const QString &profile);

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

    bool decodeSubtitle2(Subtitle *subtitle, Packet *packet);
    // sendPacket and receiveFrame
    QVector<Frame *> decodeFrame(Packet *packet);
    QVector<Packet *> encodeFrame(Frame *frame);

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

private:
    void showCodecpar();
    void showMetaData();

    struct AVContextInfoPrivate;
    QScopedPointer<AVContextInfoPrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // AVCONTEXTINFO_H
