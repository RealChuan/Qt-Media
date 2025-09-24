#pragma once

#include "ffmepg_global.h"
#include "frame.hpp"
#include "packet.h"

#include <QObject>

extern "C" {
#include <libavcodec/codec.h>
}

struct AVStream;

namespace Ffmpeg {

class Subtitle;
class CodecContext;

class FFMPEG_EXPORT AVContextInfo : public QObject
{
public:
    enum GpuType { NotUseGpu, GpuDecode, GpuEncode };

    explicit AVContextInfo(QObject *parent = nullptr);
    ~AVContextInfo() override;

    void resetIndex();
    void setIndex(int index);
    auto index() -> int;

    auto isIndexVaild() -> bool;

    void setStream(AVStream *stream);
    auto stream() -> AVStream *;

    auto initDecoder(const AVRational &frameRate) -> bool;
    auto initEncoder(AVCodecID codecId) -> bool;
    auto initEncoder(const QString &name) -> bool;
    auto openCodec(GpuType type = NotUseGpu) -> bool;

    // sendPacket and receiveFrame
    auto decodeFrame(const QSharedPointer<Packet> &packetPtr) -> FramePtrList;
    auto encodeFrame(const FramePtr &framePtr) -> std::vector<QSharedPointer<Packet>>;
    auto decodeSubtitle2(const QSharedPointer<Subtitle> &subtitlePtr,
                         const QSharedPointer<Packet> &packetPtr) -> bool;

    [[nodiscard]] auto calTimebase() const -> double;
    [[nodiscard]] auto timebase() const -> AVRational;
    [[nodiscard]] auto fps() const -> double;
    [[nodiscard]] auto fames() const -> qint64;
    [[nodiscard]] auto resolutionRatio() const -> QSize;
    [[nodiscard]] auto mediaType() const -> AVMediaType;
    [[nodiscard]] auto mediaTypeString() const -> QString;
    [[nodiscard]] auto isDecoder() const -> bool;

    [[nodiscard]] auto gpuType() const -> GpuType;
    [[nodiscard]] auto pixfmt() const -> AVPixelFormat;

    auto codecCtx() -> CodecContext *;

private:
    class AVContextInfoPrivate;
    QScopedPointer<AVContextInfoPrivate> d_ptr;
};

using AVContextInfoPtr = QSharedPointer<AVContextInfo>;

} // namespace Ffmpeg
