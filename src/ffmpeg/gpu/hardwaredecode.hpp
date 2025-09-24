#pragma once

#include <ffmpeg/frame.hpp>

#include <QObject>

struct AVCodec;

namespace Ffmpeg {

class CodecContext;
class HardWareDecode : public QObject
{
public:
    explicit HardWareDecode(QObject *parent = nullptr);
    ~HardWareDecode() override;

    auto initPixelFormat(const AVCodec *decoder) -> bool;
    auto initHardWareDevice(CodecContext *codecContext) -> bool;
    auto transFromGpu(const FramePtr &inPtr, bool &ok) -> FramePtr;

    auto isVaild() -> bool;

private:
    class HardWareDecodePrivate;
    QScopedPointer<HardWareDecodePrivate> d_ptr;
};

} // namespace Ffmpeg
