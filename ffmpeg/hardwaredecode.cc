#include "hardwaredecode.hpp"
#include "averrormanager.hpp"
#include "codeccontext.h"
#include "ffmpegutils.hpp"
#include "frame.hpp"

#include <QDebug>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

namespace Ffmpeg {

AVPixelFormat hw_pix_fmt = AV_PIX_FMT_NONE;

AVPixelFormat get_hw_format(AVCodecContext *ctx, const enum AVPixelFormat *pix_fmts)
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
    HardWareDecodePrivate(QObject *parent)
        : owner(parent)
    {}

    ~HardWareDecodePrivate()
    {
        hw_pix_fmt = AV_PIX_FMT_NONE;
        av_buffer_unref(&bufferRef);
    }

    void setError(int errorCode) { AVErrorManager::instance()->setErrorCode(errorCode); }

    QObject *owner;
    QVector<AVHWDeviceType> hwDeviceTypes = Utils::getCurrentHWDeviceTypes();
    AVHWDeviceType hwDeviceType = AV_HWDEVICE_TYPE_NONE;
    AVBufferRef *bufferRef = nullptr;
    bool vaild = false;
};

HardWareDecode::HardWareDecode(QObject *parent)
    : QObject(parent)
    , d_ptr(new HardWareDecodePrivate(this))
{}

HardWareDecode::~HardWareDecode() {}

bool HardWareDecode::initPixelFormat(const AVCodec *decoder)
{
    if (av_codec_is_decoder(decoder) <= 0) {
        return false;
    }
    if (d_ptr->hwDeviceTypes.isEmpty()) {
        return false;
    }
    for (AVHWDeviceType type : qAsConst(d_ptr->hwDeviceTypes)) {
        hw_pix_fmt = Utils::getPixelFormat(decoder, type);
        if (hw_pix_fmt != AV_PIX_FMT_NONE) {
            d_ptr->hwDeviceType = type;
            break;
        }
    }
    return (hw_pix_fmt != AV_PIX_FMT_NONE);
}

bool HardWareDecode::initHardWareDevice(CodecContext *codecContext)
{
    if (hw_pix_fmt == AV_PIX_FMT_NONE) {
        return false;
    }
    auto ret = av_hwdevice_ctx_create(&d_ptr->bufferRef, d_ptr->hwDeviceType, nullptr, nullptr, 0);
    if (ret < 0) {
        qWarning() << "Failed to create specified HW device.";
        d_ptr->setError(ret);
        return false;
    }
    codecContext->avCodecCtx()->hw_device_ctx = av_buffer_ref(d_ptr->bufferRef);
    codecContext->avCodecCtx()->get_format = get_hw_format;
    d_ptr->vaild = true;
    return true;
}

Frame *HardWareDecode::transFromGpu(Frame *in, bool &ok)
{
    ok = true;
    if (!isVaild()) {
        return in;
    }

    if (in->format() != hw_pix_fmt) {
        return in;
    }
    auto out = new Frame;
    // 超级吃CPU 巨慢
    int ret = av_hwframe_transfer_data(out->avFrame(), in->avFrame(), 0);
    // 如果把映射后的帧存起来，接下去解码会出问题；
    // 相当于解码出来的帧要在下一次解码前必须清除。
    //int ret = av_hwframe_map(out->avFrame(), in->avFrame(), 0);
    if (ret < 0) {
        qWarning() << "Error transferring the data to system memory";
        d_ptr->setError(ret);
        ok = false;
    }
    // 拷贝其他信息
    out->copyPropsFrom(in);
    return out;
}

bool HardWareDecode::isVaild()
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
