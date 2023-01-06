#ifndef CODECCONTEXT_H
#define CODECCONTEXT_H

#include <QObject>

extern "C" {
#include <libavutil/avutil.h>
#include <libavutil/samplefmt.h>
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
    explicit CodecContext(const AVCodec *codec, QObject *parent = nullptr);
    ~CodecContext();

    void copyToCodecParameters(CodecContext *dst);

    bool setParameters(const AVCodecParameters *par);
    void setFrameRate(const AVRational &frameRate);

    void setPixfmt(AVPixelFormat pixfmt);
    AVPixelFormat pixfmt() const;

    void setSampleRate(int sampleRate);
    int sampleRate() const;

    void setSampleFmt(AVSampleFormat sampleFmt);
    AVSampleFormat sampleFmt() const;

    void setChannelLayout(uint64_t channelLayout);
    uint64_t channelLayout() const;

    void setSize(const QSize &size);
    QSize size() const;

    void setQuailty(int quailty);
    QPair<int, int> quantizer() const;

    void setMinBitrate(int64_t bitrate);
    void setMaxBitrate(int64_t bitrate);
    void setCrf(int crf);
    void setPreset(const QString &preset);
    void setTune(const QString &tune);
    void setProfile(const QString &profile);

    QVector<AVPixelFormat> supportPixFmts() const;
    QVector<AVSampleFormat> supportSampleFmts() const;

    void setTimebase(const AVRational &timebase);
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

    const AVCodec *codec();
    AVCodecContext *avCodecCtx();

signals:
    void error(const Ffmpeg::AVError &avError);

private:
    void setError(int errorCode);

    class CodecContextPrivate;
    QScopedPointer<CodecContextPrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // CODECCONTEXT_H
