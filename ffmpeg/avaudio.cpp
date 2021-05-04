#include "avaudio.h"
#include "codeccontext.h"
#include "playframe.h"
#include "averror.h"

extern "C"{
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
}

namespace Ffmpeg {

AVAudio::AVAudio(CodecContext *codecCtx, AVSampleFormat format)
{
    m_format = format;
    AVCodecContext *ctx = codecCtx->avCodecCtx();
    m_swrContext = swr_alloc_set_opts(nullptr, int64_t(ctx->channel_layout), format,
                                      ctx->sample_rate, int64_t(ctx->channel_layout),
                                      ctx->sample_fmt, ctx->sample_rate, 0, nullptr);

    int ret = swr_init(m_swrContext);
    if(ret < 0){
        qWarning() << AVError::avErrorString(ret);
    }
    Q_ASSERT(m_swrContext != nullptr);
}

AVAudio::~AVAudio()
{
    Q_ASSERT(m_swrContext != nullptr);
    swr_free(&m_swrContext);
}

QByteArray AVAudio::convert(PlayFrame *frame)
{
    QByteArray data;
    int size = av_samples_get_buffer_size(nullptr, frame->avFrame()->channels,
                                          frame->avFrame()->nb_samples, m_format, 0);
    uint8_t *buf = new uint8_t[size];
    int len = swr_convert(m_swrContext, &buf, frame->avFrame()->nb_samples,
                          const_cast<const uint8_t**>(frame->avFrame()->data), frame->avFrame()->nb_samples);
    data += QByteArray((const char *)(buf), size);
    delete[] buf;

    if(len <= 0){
        qWarning() << AVError::avErrorString(len);
    }

    return data;
}

}
