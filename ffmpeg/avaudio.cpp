#include "avaudio.h"
#include "codeccontext.h"
#include "playframe.h"

extern "C"{
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
}

namespace Ffmpeg {

AVAudio::AVAudio(CodecContext *codecCtx)
{
    AVCodecContext *ctx = codecCtx->avCodecCtx();
    m_swrContext = swr_alloc_set_opts(NULL, av_get_default_channel_layout(2), AV_SAMPLE_FMT_S16, 44100,
                                      ctx->channel_layout, ctx->sample_fmt, ctx->sample_rate, NULL, NULL);
    swr_init(m_swrContext);
    Q_ASSERT(m_swrContext != nullptr);
}

AVAudio::~AVAudio()
{
    Q_ASSERT(m_swrContext != nullptr);
    swr_free(&m_swrContext);
}

QByteArray AVAudio::convert(PlayFrame *frame, CodecContext *codecCtx)
{
    QByteArray buf;
    uint8_t *audio_out_buffer = (uint8_t *)av_malloc(frame->avFrame()->nb_samples * 2 * 2);
    int len = swr_convert(m_swrContext, &audio_out_buffer, frame->avFrame()->nb_samples,
                          (const uint8_t **)frame->avFrame()->data, frame->avFrame()->nb_samples);
    if(len > 0){
        int dst_bufsize = av_samples_get_buffer_size(0,  codecCtx->avCodecCtx()->channels, len, AV_SAMPLE_FMT_S16, 1);
        buf = QByteArray((char*)audio_out_buffer, dst_bufsize);
    }

    av_free(audio_out_buffer);
    return buf;
}

}
