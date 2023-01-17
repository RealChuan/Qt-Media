#include "hardwareencode.hpp"
#include "averrormanager.hpp"
#include "codeccontext.h"
#include "ffmpegutils.hpp"
#include "frame.hpp"

#include <QDebug>

extern "C" {
#include <libavcodec/avcodec.h>
}

namespace Ffmpeg {

class HardWareEncode::HardWareEncodePrivate
{
public:
    HardWareEncodePrivate(QObject *parent)
        : owner(parent)
    {}

    ~HardWareEncodePrivate() { av_buffer_unref(&bufferRef); }

    void setError(int errorCode) { AVErrorManager::instance()->setErrorCode(errorCode); }

    QObject *owner;
    QVector<AVHWDeviceType> hwDeviceTypes = Utils::getCurrentHWDeviceTypes();
    AVHWDeviceType hwDeviceType = AV_HWDEVICE_TYPE_NONE;
    AVBufferRef *bufferRef = nullptr;
    AVPixelFormat sw_format = AV_PIX_FMT_NV12;
    AVPixelFormat hw_pix_fmt = AV_PIX_FMT_NONE;
    QVector<AVPixelFormat> hw_pix_fmts = {AV_PIX_FMT_VDPAU,
                                          AV_PIX_FMT_QSV,
                                          AV_PIX_FMT_MMAL,
                                          AV_PIX_FMT_CUDA};
    bool vaild = false;
};

HardWareEncode::HardWareEncode(QObject *parent)
    : QObject{parent}
    , d_ptr(new HardWareEncodePrivate(this))
{}

HardWareEncode::~HardWareEncode() {}

bool HardWareEncode::initEncoder(const AVCodec *encoder)
{
    if (av_codec_is_encoder(encoder) <= 0) {
        return false;
    }
    if (d_ptr->hwDeviceTypes.isEmpty()) {
        return false;
    }
    auto hw_pix_fmt = AV_PIX_FMT_NONE;
    for (AVHWDeviceType type : qAsConst(d_ptr->hwDeviceTypes)) {
        hw_pix_fmt = Utils::getPixelFormat(encoder, type);
        if (hw_pix_fmt != AV_PIX_FMT_NONE) {
            d_ptr->hwDeviceType = type;
            break;
        }
    }
    if (encoder->pix_fmts) {
        for (auto pix_fmt = encoder->pix_fmts; *pix_fmt != -1; pix_fmt++) {
            if (d_ptr->hw_pix_fmts.contains(*pix_fmt)) {
                d_ptr->hw_pix_fmt = *pix_fmt;
                break;
            }
        }
    }
    return (hw_pix_fmt != AV_PIX_FMT_NONE);
}

bool HardWareEncode::initHardWareDevice(CodecContext *codecContext)
{
    if (d_ptr->hwDeviceType == AV_HWDEVICE_TYPE_NONE) {
        return false;
    }
    auto ret = av_hwdevice_ctx_create(&d_ptr->bufferRef, d_ptr->hwDeviceType, nullptr, nullptr, 0);
    if (ret < 0) {
        qWarning() << "Failed to create specified HW device.";
        d_ptr->setError(ret);
        return false;
    }

    auto hw_frames_ref = av_hwframe_ctx_alloc(d_ptr->bufferRef);
    if (!hw_frames_ref) {
        qWarning() << "Failed to create VAAPI frame context.";
        return false;
    }
    auto ctx = codecContext->avCodecCtx();
    ctx->pix_fmt = d_ptr->hw_pix_fmt;
    auto frames_ctx = (AVHWFramesContext *) (hw_frames_ref->data);
    frames_ctx->format = d_ptr->hw_pix_fmt;
    frames_ctx->sw_format = d_ptr->sw_format;
    frames_ctx->width = ctx->width;
    frames_ctx->height = ctx->height;
    frames_ctx->initial_pool_size = 20;
    auto err = av_hwframe_ctx_init(hw_frames_ref);
    auto clean = qScopeGuard([&] { av_buffer_unref(&hw_frames_ref); });
    if (err < 0) {
        d_ptr->setError(err);
        return false;
    }
    ctx->hw_frames_ctx = av_buffer_ref(hw_frames_ref);
    d_ptr->vaild = ctx->hw_frames_ctx != nullptr;
    return d_ptr->vaild;
}

Frame *HardWareEncode::transToGpu(CodecContext *codecContext, Frame *in, bool &ok, bool &needFree)
{
    ok = true;
    if (!isVaild()) {
        return in;
    }
    auto avctx = codecContext->avCodecCtx();
    auto sw_frame = in->avFrame();
    std::unique_ptr<Frame> out(new Frame);
    auto hw_frame = out->avFrame();
    auto err = av_hwframe_get_buffer(avctx->hw_frames_ctx, hw_frame, 0);
    if (err < 0) {
        ok = false;
        d_ptr->setError(err);
        return in;
    }
    if (!hw_frame->hw_frames_ctx) {
        ok = false;
        return in;
    }
    if ((err = av_hwframe_transfer_data(hw_frame, sw_frame, 0)) < 0) {
        ok = false;
        d_ptr->setError(err);
        return in;
    }
    needFree = true;
    out->copyPropsFrom(in);
    return out.release();
}

AVPixelFormat HardWareEncode::swFormat() const
{
    return d_ptr->sw_format;
}

bool HardWareEncode::isVaild()
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
