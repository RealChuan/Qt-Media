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

class Subtitle;
class Packet;
class Frame;
class CodecContext : public QObject
{
public:
    explicit CodecContext(const AVCodec *codec, QObject *parent = nullptr);
    ~CodecContext() override;

    void copyToCodecParameters(CodecContext *dst);

    auto setParameters(const AVCodecParameters *par) -> bool;
    void setPixfmt(AVPixelFormat pixfmt);
    void setSampleRate(int sampleRate);
    void setSampleFmt(AVSampleFormat sampleFmt);

    void setMinBitrate(int64_t bitrate);
    void setMaxBitrate(int64_t bitrate);
    void setCrf(int crf);
    void setPreset(const QString &preset);
    void setTune(const QString &tune);
    void setProfile(const QString &profile);

    void setChannelLayout(uint64_t channelLayout);
    [[nodiscard]] auto channels() const -> int;

    void setSize(const QSize &size);
    [[nodiscard]] auto size() const -> QSize;

    void setQuailty(int quailty);
    [[nodiscard]] QPair<int, int> quantizer() const;

    [[nodiscard]] QVector<AVPixelFormat> supportPixFmts() const;
    [[nodiscard]] QVector<AVSampleFormat> supportSampleFmts() const;

    // Set before open, Soft solution is effective
    void setThreadCount(int threadCount);
    auto open() -> bool;

    auto sendPacket(Packet *packet) -> bool;
    auto receiveFrame(Frame *frame) -> bool;
    auto decodeSubtitle2(Subtitle *subtitle, Packet *packet) -> bool;

    auto sendFrame(Frame *frame) -> bool;
    auto receivePacket(Packet *packet) -> bool;

    [[nodiscard]] auto mediaTypeString() const -> QString;
    [[nodiscard]] auto isDecoder() const -> bool;

    void flush();

    auto codec() -> const AVCodec *;
    auto avCodecCtx() -> AVCodecContext *;

private:
    class CodecContextPrivate;
    QScopedPointer<CodecContextPrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // CODECCONTEXT_H
