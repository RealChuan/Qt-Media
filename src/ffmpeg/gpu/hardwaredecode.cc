#include "hardwaredecode.hpp"
#include "bufferref.hpp"

#include <ffmpeg/averrormanager.hpp>
#include <ffmpeg/codeccontext.h>
#include <ffmpeg/ffmpegutils.hpp>

#include <QDebug>

extern "C" {
#include <libavcodec/avcodec.h>
}

namespace Ffmpeg {

AVPixelFormat hw_pix_fmt = AV_PIX_FMT_NONE;

auto get_hw_format(AVCodecContext *ctx, const enum AVPixelFormat *pix_fmts) -> AVPixelFormat
{
    Q_UNUSED(ctx)
    const enum AVPixelFormat *p;
    for (p = pix_fmts; *p != -1; p++) {
        if (*p == hw_pix_fmt) {
            return *p;
        }
    }
    qWarning() << "Failed to get HW surface format.";
    return AV_PIX_FMT_NONE;
}

class HardWareDecode::HardWareDecodePrivate
{
public:
    explicit HardWareDecodePrivate(HardWareDecode *q)
        : q_ptr(q)
    {
        bufferRef = new BufferRef(q_ptr);
    }

    ~HardWareDecodePrivate() { hw_pix_fmt = AV_PIX_FMT_NONE; }

    HardWareDecode *q_ptr;

    QList<AVHWDeviceType> hwDeviceTypes = getCurrentHWDeviceTypes();
    AVHWDeviceType hwDeviceType = AV_HWDEVICE_TYPE_NONE;
    BufferRef *bufferRef;
    bool vaild = false;
};

HardWareDecode::HardWareDecode(QObject *parent)
    : QObject(parent)
    , d_ptr(new HardWareDecodePrivate(this))
{}

HardWareDecode::~HardWareDecode() = default;

auto HardWareDecode::initPixelFormat(const AVCodec *decoder) -> bool
{
    if (av_codec_is_decoder(decoder) <= 0) {
        return false;
    }
    if (d_ptr->hwDeviceTypes.isEmpty()) {
        return false;
    }
    for (AVHWDeviceType type : std::as_const(d_ptr->hwDeviceTypes)) {
        hw_pix_fmt = getPixelFormat(decoder, type);
        if (hw_pix_fmt != AV_PIX_FMT_NONE) {
            d_ptr->hwDeviceType = type;
            break;
        }
    }
    return (hw_pix_fmt != AV_PIX_FMT_NONE);
}

auto HardWareDecode::initHardWareDevice(CodecContext *codecContext) -> bool
{
    if (hw_pix_fmt == AV_PIX_FMT_NONE) {
        return false;
    }
    if (!d_ptr->bufferRef->hwdeviceCtxCreate(d_ptr->hwDeviceType)) {
        return false;
    }
    auto *ctx = codecContext->avCodecCtx();
    ctx->hw_device_ctx = d_ptr->bufferRef->ref();
    ctx->get_format = get_hw_format;
    d_ptr->vaild = ctx->hw_device_ctx != nullptr;
    return d_ptr->vaild;
}

auto HardWareDecode::transFromGpu(const FramePtr &inPtr, bool &ok) -> FramePtr
{
    ok = true;
    if (!isVaild()) {
        return inPtr;
    }
    if (inPtr->avFrame()->format != hw_pix_fmt) {
        return inPtr;
    }
    FramePtr outPtr(new Frame);
    // 超级吃CPU 巨慢
    auto ret = av_hwframe_transfer_data(outPtr->avFrame(), inPtr->avFrame(), 0);
    // 如果把映射后的帧存起来，接下去解码会出问题；
    // 相当于解码出来的帧要在下一次解码前必须清除。
    //auto ret = av_hwframe_map(out->avFrame(), in->avFrame(), 0);
    if (ret < 0) {
        qWarning() << "Error transferring the data to system memory";
        SET_ERROR_CODE(ret);
        ok = false;
        return inPtr;
    }
    // 拷贝其他信息
    outPtr->copyPropsFrom(inPtr.get());
    return outPtr;
}

auto HardWareDecode::isVaild() -> bool
{
    if (d_ptr->hwDeviceType == AV_HWDEVICE_TYPE_NONE) {
        return false;
    }
    if (hw_pix_fmt == AV_PIX_FMT_NONE) {
        return false;
    }
    return d_ptr->vaild;
}

} // namespace Ffmpeg
