#pragma once

#include "ffmepg_global.h"
#include "frame.hpp"

#include <QObject>

extern "C" {
#include <libavcodec/codec.h>
}

struct AVCodecParameters;
struct AVCodecContext;

namespace Ffmpeg {

struct EncodeContext;
class Subtitle;
class Packet;
class FFMPEG_EXPORT CodecContext : public QObject
{
public:
    explicit CodecContext(const AVCodec *codec, QObject *parent = nullptr);
    ~CodecContext() override;

    void copyToCodecParameters(CodecContext *dst);

    auto setParameters(const AVCodecParameters *par) -> bool;

    [[nodiscard]] auto supportedFrameRates() const -> QList<AVRational>;
    void setFrameRate(const AVRational &frameRate);

    [[nodiscard]] auto supportedPixFmts() const -> QList<AVPixelFormat>;
    void setPixfmt(AVPixelFormat pixfmt);

    [[nodiscard]] auto supportedSampleRates() const -> QList<int>;
    void setSampleRate(int sampleRate);

    [[nodiscard]] auto supportedSampleFmts() const -> QList<AVSampleFormat>;
    void setSampleFmt(AVSampleFormat sampleFmt);

    [[nodiscard]] auto supportedProfiles() const -> QList<AVProfile>;
    void setProfile(int profile);

    [[nodiscard]] auto supportedChLayouts() const -> QList<AVChannelLayout>;
    [[nodiscard]] auto chLayout() const -> AVChannelLayout;
    void setChLayout(const AVChannelLayout &chLayout);

    void setEncodeParameters(const EncodeContext &encodeContext);

    void setSize(const QSize &size);
    [[nodiscard]] auto size() const -> QSize;

    [[nodiscard]] auto quantizer() const -> QPair<int, int>;

    // Set before open, Soft solution is effective
    void setThreadCount(int threadCount);
    auto open() -> bool;

    auto sendPacket(Packet *packet) -> bool;
    auto receiveFrame(const FramePtr &framePtr) -> bool;
    auto decodeSubtitle2(Subtitle *subtitle, Packet *packet) -> bool;

    auto sendFrame(const FramePtr &framePtr) -> bool;
    auto receivePacket(Packet *packet) -> bool;

    [[nodiscard]] auto mediaTypeString() const -> QString;
    [[nodiscard]] auto isDecoder() const -> bool;

    void flush();

    auto avCodecCtx() -> AVCodecContext *;

private:
    class CodecContextPrivate;
    QScopedPointer<CodecContextPrivate> d_ptr;
};

} // namespace Ffmpeg
