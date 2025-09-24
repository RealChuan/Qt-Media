#include "hardwareencode.hpp"
#include "bufferref.hpp"

#include <ffmpeg/averrormanager.hpp>
#include <ffmpeg/codeccontext.h>
#include <ffmpeg/ffmpegutils.hpp>

#include <QDebug>

extern "C" {
#include <libavcodec/avcodec.h>
}

namespace Ffmpeg {

class HardWareEncode::HardWareEncodePrivate
{
public:
    explicit HardWareEncodePrivate(HardWareEncode *q)
        : q_ptr(q)
    {
        bufferRef = new BufferRef(q_ptr);
    }

    ~HardWareEncodePrivate() = default;

    HardWareEncode *q_ptr;

    QList<AVHWDeviceType> hwDeviceTypes = getCurrentHWDeviceTypes();
    AVHWDeviceType hwDeviceType = AV_HWDEVICE_TYPE_NONE;
    BufferRef *bufferRef;
    AVPixelFormat sw_format = AV_PIX_FMT_NV12;
    AVPixelFormat hw_pix_fmt = AV_PIX_FMT_NONE;
    QList<AVPixelFormat> hw_pix_fmts = {AV_PIX_FMT_VDPAU,
                                        AV_PIX_FMT_QSV,
                                        AV_PIX_FMT_MMAL,
                                        AV_PIX_FMT_CUDA};
    bool vaild = false;
};

HardWareEncode::HardWareEncode(QObject *parent)
    : QObject{parent}
    , d_ptr(new HardWareEncodePrivate(this))
{}

HardWareEncode::~HardWareEncode() = default;

auto HardWareEncode::initEncoder(const AVCodec *encoder) -> bool
{
    if (av_codec_is_encoder(encoder) <= 0) {
        return false;
    }
    if (d_ptr->hwDeviceTypes.isEmpty()) {
        return false;
    }
    auto hw_pix_fmt = AV_PIX_FMT_NONE;
    for (auto type : std::as_const(d_ptr->hwDeviceTypes)) {
        hw_pix_fmt = getPixelFormat(encoder, type);
        if (hw_pix_fmt != AV_PIX_FMT_NONE) {
            d_ptr->hwDeviceType = type;
            break;
        }
    }

    int nb_pix_fmts;
    const void *pix_fmts = nullptr;
    avcodec_get_supported_config(nullptr,
                                 encoder,
                                 AV_CODEC_CONFIG_PIX_FORMAT,
                                 0,
                                 &pix_fmts,
                                 &nb_pix_fmts);
    if (nb_pix_fmts > 0) {
        const auto *supported_pix_fmts = static_cast<const AVPixelFormat *>(pix_fmts);
        for (int i = 0; i < nb_pix_fmts; ++i) {
            auto pix_fmt = supported_pix_fmts[i];
            if (d_ptr->hw_pix_fmts.contains(pix_fmt)) {
                d_ptr->hw_pix_fmt = pix_fmt;
                break;
            }
        }
    }
    return (hw_pix_fmt != AV_PIX_FMT_NONE);
}

auto HardWareEncode::initHardWareDevice(CodecContext *codecContext) -> bool
{
    if (d_ptr->hwDeviceType == AV_HWDEVICE_TYPE_NONE) {
        return false;
    }
    if (!d_ptr->bufferRef->hwdeviceCtxCreate(d_ptr->hwDeviceType)) {
        return false;
    }
    auto *hw_frames_ref = d_ptr->bufferRef->hwframeCtxAlloc();
    if (hw_frames_ref == nullptr) {
        return false;
    }
    auto *ctx = codecContext->avCodecCtx();
    ctx->pix_fmt = d_ptr->hw_pix_fmt;
    auto *frames_ctx = reinterpret_cast<AVHWFramesContext *>(hw_frames_ref->avBufferRef()->data);
    frames_ctx->format = d_ptr->hw_pix_fmt;
    frames_ctx->sw_format = d_ptr->sw_format;
    frames_ctx->width = ctx->width;
    frames_ctx->height = ctx->height;
    frames_ctx->initial_pool_size = 20;
    if (!hw_frames_ref->hwframeCtxInit()) {
        return false;
    }
    ctx->hw_frames_ctx = hw_frames_ref->ref();
    d_ptr->vaild = ctx->hw_frames_ctx != nullptr;
    return d_ptr->vaild;
}

auto HardWareEncode::transToGpu(CodecContext *codecContext, FramePtr inPtr, bool &ok) -> FramePtr
{
    ok = true;
    if (!isVaild()) {
        return inPtr;
    }
    auto *avctx = codecContext->avCodecCtx();
    auto *sw_frame = inPtr->avFrame();
    FramePtr outPtr(new Frame);
    auto *hw_frame = outPtr->avFrame();
    auto err = av_hwframe_get_buffer(avctx->hw_frames_ctx, hw_frame, 0);
    if (err < 0) {
        ok = false;
        SET_ERROR_CODE(err);
        return inPtr;
    }
    if (hw_frame->hw_frames_ctx == nullptr) {
        ok = false;
        return inPtr;
    }
    if ((err = av_hwframe_transfer_data(hw_frame, sw_frame, 0)) < 0) {
        ok = false;
        SET_ERROR_CODE(err);
        return inPtr;
    }
    outPtr->copyPropsFrom(inPtr.get());
    return outPtr;
}

auto HardWareEncode::swFormat() const -> AVPixelFormat
{
    return d_ptr->sw_format;
}

auto HardWareEncode::isVaild() -> bool
{
    if (d_ptr->hwDeviceType == AV_HWDEVICE_TYPE_NONE) {
        return false;
    }
    if (d_ptr->hw_pix_fmt == AV_PIX_FMT_NONE) {
        return false;
    }
    return d_ptr->vaild;
}

} // namespace Ffmpeg
