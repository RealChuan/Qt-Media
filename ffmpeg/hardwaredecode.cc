#include "hardwaredecode.hpp"
#include "averror.h"
#include "codeccontext.h"
#include "playframe.h"

#include <functional>

extern "C" {
#include <libavformat/avformat.h>
}

namespace Ffmpeg {

QVector<AVHWDeviceType> getHardWareDeviceTypes()
{
    static QVector<AVHWDeviceType> types{AV_HWDEVICE_TYPE_VDPAU,
                                         AV_HWDEVICE_TYPE_CUDA,
                                         AV_HWDEVICE_TYPE_VAAPI,
                                         AV_HWDEVICE_TYPE_DXVA2,
                                         AV_HWDEVICE_TYPE_QSV,
                                         AV_HWDEVICE_TYPE_VIDEOTOOLBOX,
                                         AV_HWDEVICE_TYPE_D3D11VA,
                                         AV_HWDEVICE_TYPE_DRM,
                                         AV_HWDEVICE_TYPE_OPENCL,
                                         AV_HWDEVICE_TYPE_MEDIACODEC,
                                         AV_HWDEVICE_TYPE_VULKAN};
    return types;
    //    const QStringList list{"cuda",
    //                           "drm",
    //                           "dxva2",
    //                           "d3d11va",
    //                           "opencl",
    //                           "qsv",
    //                           "vaapi",
    //                           "vdpau",
    //                           "videotoolbox",
    //                           "mediacodec"};
    //    AVHWDeviceType type = AV_HWDEVICE_TYPE_NONE;
    //    for (const QString &hw : qAsConst(list)) {
    //        type = av_hwdevice_find_type_by_name(hw.toLocal8Bit().constData());
    //        if (type != AV_HWDEVICE_TYPE_NONE) {
    //            types.append(type);
    //            continue;
    //        }
    //        qWarning() << QString("Device type %1 is not supported.").arg(hw);
    //        qWarning() << "Available device types:";
    //        while ((type = av_hwdevice_iterate_types(type)) != AV_HWDEVICE_TYPE_NONE) {
    //            qWarning() << av_hwdevice_get_type_name(type);
    //        }
    //    }
}

AVPixelFormat getPixelFormat(const AVCodec *decoder, AVHWDeviceType type)
{
    AVPixelFormat hw_pix_fmt = AV_PIX_FMT_NONE;
    for (int i = 0;; i++) {
        const AVCodecHWConfig *config = avcodec_get_hw_config(decoder, i);
        if (!config) {
            qWarning() << QString("Decoder %1 does not support device type %2.")
                              .arg(decoder->name, av_hwdevice_get_type_name(type));
            return hw_pix_fmt;
        }
        if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX
            && config->device_type == type) {
            qInfo() << QString("Decoder %1 support device type %2.")
                           .arg(decoder->name, av_hwdevice_get_type_name(type));
            hw_pix_fmt = config->pix_fmt;
            break;
        }
    }
    return hw_pix_fmt;
}

AVPixelFormat hw_pix_fmt = AV_PIX_FMT_NONE;

AVPixelFormat get_hw_format(AVCodecContext *ctx, const enum AVPixelFormat *pix_fmts)
{
    Q_UNUSED(ctx)
    const enum AVPixelFormat *p;
    for (p = pix_fmts; *p != -1; p++) {
        if (*p == hw_pix_fmt)
            return *p;
    }
    qWarning() << "Failed to get HW surface format.";
    return AV_PIX_FMT_NONE;
}

struct HardWareDecode::HardWareDecodePrivate
{
    ~HardWareDecodePrivate() { av_buffer_unref(&bufferRef); }

    QVector<AVHWDeviceType> hwDeviceTypes = getHardWareDeviceTypes();
    AVHWDeviceType hwDeviceType = AV_HWDEVICE_TYPE_NONE;
    AVBufferRef *bufferRef = nullptr;
    AVError error;
    bool vaild = true;
};

HardWareDecode::HardWareDecode(QObject *parent)
    : QObject(parent)
    , d_ptr(new HardWareDecodePrivate)
{}

HardWareDecode::~HardWareDecode() {}

bool HardWareDecode::initPixelFormat(const AVCodec *decoder)
{
    if (d_ptr->hwDeviceTypes.isEmpty()) {
        return false;
    }
    for (AVHWDeviceType type : qAsConst(d_ptr->hwDeviceTypes)) {
        hw_pix_fmt = getPixelFormat(decoder, type);
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
    codecContext->avCodecCtx()->get_format = get_hw_format;
    int ret = av_hwdevice_ctx_create(&d_ptr->bufferRef, d_ptr->hwDeviceType, nullptr, nullptr, 0);
    if (ret < 0) {
        qWarning() << "Failed to create specified HW device.";
        setError(ret);
        return false;
    }
    codecContext->avCodecCtx()->hw_device_ctx = av_buffer_ref(d_ptr->bufferRef);
    d_ptr->vaild = true;
    return true;
}

PlayFrame *HardWareDecode::transforFrame(PlayFrame *playFrame, bool &ok)
{
    ok = true;
    if (!isVaild()) {
        return playFrame;
    }

    if (playFrame->avFrame()->format != hw_pix_fmt) {
        return playFrame;
    }
    PlayFrame *out = new PlayFrame;
    int ret = av_hwframe_transfer_data(out->avFrame(), playFrame->avFrame(), 0);
    if (ret < 0) {
        qWarning() << "Error transferring the data to system memory";
        setError(ret);
        ok = false;
    }
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

AVError HardWareDecode::avError()
{
    return d_ptr->error;
}

void HardWareDecode::setError(int errorCode)
{
    d_ptr->error.setError(errorCode);
    emit error(d_ptr->error);
}

} // namespace Ffmpeg
