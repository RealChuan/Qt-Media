#pragma once

#include <ffmpeg/frame.hpp>

#include <QObject>

extern "C" {
#include <libavutil/pixfmt.h>
}

struct AVCodec;

namespace Ffmpeg {

class CodecContext;
class HardWareEncode : public QObject
{
public:
    explicit HardWareEncode(QObject *parent = nullptr);
    ~HardWareEncode() override;

    auto initEncoder(const AVCodec *encoder) -> bool;
    auto initHardWareDevice(CodecContext *codecContext) -> bool;
    auto transToGpu(CodecContext *codecContext, FramePtr inPtr, bool &ok) -> FramePtr;

    [[nodiscard]] auto swFormat() const -> AVPixelFormat;

    auto isVaild() -> bool;

private:
    class HardWareEncodePrivate;
    QScopedPointer<HardWareEncodePrivate> d_ptr;
};

} // namespace Ffmpeg
