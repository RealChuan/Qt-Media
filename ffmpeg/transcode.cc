#include "transcode.hpp"
#include "avcontextinfo.h"
#include "averror.h"
#include "codeccontext.h"
#include "formatcontext.h"
#include "frame.hpp"

#include <QSharedPointer>

extern "C" {
#include <libavdevice/avdevice.h>
}

namespace Ffmpeg {

struct TranscodeContext
{
    QSharedPointer<AVContextInfo> decContextInfo;
    QSharedPointer<AVContextInfo> encContextInfo;

    QSharedPointer<Frame> dec_frame;
};

class Transcode::TranscodePrivate
{
public:
    TranscodePrivate(QObject *parent)
        : owner(parent)
        , inFormatContext(new FormatContext(owner))
        , outFormatContext(new FormatContext(owner))
    {}

    bool setInMediaIndex(AVContextInfo *contextInfo, int index)
    {
        contextInfo->setIndex(index);
        contextInfo->setStream(inFormatContext->stream(index));
        return contextInfo->initDecoder(inFormatContext->guessFrameRate(index));
    }

    QObject *owner;

    FormatContext *inFormatContext;
    FormatContext *outFormatContext;
    QVector<TranscodeContext *> transcodeContexts;
    AVCodecID enCodecId = AV_CODEC_ID_NONE;

    bool gpuDecode = false;

    AVError error;
};

Transcode::Transcode(QObject *parent)
    : QObject{parent}
    , d_ptr(new TranscodePrivate(this))
{}

Transcode::~Transcode() {}

bool Transcode::openInputFile(const QString &filepath)
{
    auto ret = d_ptr->inFormatContext->openFilePath(filepath);
    if (!ret) {
        return ret;
    }
    d_ptr->inFormatContext->findStream();
    auto stream_num = d_ptr->inFormatContext->streams();
    for (int i = 0; i < stream_num; i++) {
        QSharedPointer<AVContextInfo> contextInfoPtr;
        auto codec_type = d_ptr->inFormatContext->stream(i)->codecpar->codec_type;
        switch (codec_type) {
        case AVMEDIA_TYPE_AUDIO:
        case AVMEDIA_TYPE_VIDEO:
        case AVMEDIA_TYPE_SUBTITLE: contextInfoPtr.reset(new AVContextInfo); break;
        default: break;
        }
        if (contextInfoPtr.isNull()) {
            continue;
        }
        if (!d_ptr->setInMediaIndex(contextInfoPtr.data(), i)) {
            return false;
        }
        if (codec_type == AVMEDIA_TYPE_VIDEO || codec_type == AVMEDIA_TYPE_AUDIO) {
            contextInfoPtr->openCodec(d_ptr->gpuDecode);
        }
        auto transContext = new TranscodeContext;
        transContext->decContextInfo = contextInfoPtr;
        transContext->dec_frame.reset(new Frame);
        d_ptr->transcodeContexts.append(transContext);
    }
    d_ptr->inFormatContext->dumpFormat();

    return true;
}

bool Transcode::openOutputFile(const QString &filepath)
{
    auto ret = d_ptr->outFormatContext->openFilePath(filepath, FormatContext::WriteOnly);
    if (!ret) {
        return ret;
    }
    auto stream_num = d_ptr->inFormatContext->streams();
    for (int i = 0; i < stream_num; i++) {
        auto stream = d_ptr->outFormatContext->createStream();
        if (!stream) {
            return false;
        }
        auto transContext = d_ptr->transcodeContexts.at(i);
        auto decContextInfo = transContext->decContextInfo;
        switch (decContextInfo->mediaType()) {
        case AVMEDIA_TYPE_AUDIO:
        case AVMEDIA_TYPE_VIDEO: {
            QSharedPointer<AVContextInfo> contextInfoPtr(new AVContextInfo);
            contextInfoPtr->initEncoder(d_ptr->enCodecId);
            decContextInfo->copyToCodecParameters(contextInfoPtr.data());
            if (d_ptr->outFormatContext->avFormatContext()->oformat->flags & AVFMT_GLOBALHEADER) {
                contextInfoPtr->codecCtx()->setFlags(contextInfoPtr->codecCtx()->flags()
                                                     | AV_CODEC_FLAG_GLOBAL_HEADER);
            }
            contextInfoPtr->openCodec(d_ptr->gpuDecode);
            auto ret = avcodec_parameters_from_context(stream->codecpar,
                                                       contextInfoPtr->codecCtx()->avCodecCtx());
            if (ret < 0) {
                setError(ret);
                return ret;
            }
            stream->time_base = decContextInfo->timebase();
            transContext->encContextInfo = contextInfoPtr;
        } break;
        case AVMEDIA_TYPE_UNKNOWN:
            qFatal("Elementary stream #%d is of unknown type, cannot proceed\n", i);
            return false;
        default: {
            auto inStream = d_ptr->inFormatContext->stream(i);
            auto ret = avcodec_parameters_copy(stream->codecpar, inStream->codecpar);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "Copying parameters for stream #%u failed\n", i);
                return ret;
            }
            stream->time_base = inStream->time_base;
        } break;
        }
    }

    d_ptr->outFormatContext->dumpFormat();

    d_ptr->outFormatContext->avio_open();

    return d_ptr->outFormatContext->writeHeader();
}

void Transcode::setError(int errorCode)
{
    d_ptr->error.setError(errorCode);
    emit error(d_ptr->error);
}

} // namespace Ffmpeg
