#include "avaudio.h"
#include "codeccontext.h"
#include "playframe.h"
#include "averror.h"

extern "C"{
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
}

namespace Ffmpeg {

AVAudio::AVAudio(CodecContext *codecCtx)
{
    AVCodecContext *ctx = codecCtx->avCodecCtx();
    //qDebug() << AV_CH_LAYOUT_5POINT1 << AV_CH_LAYOUT_STEREO;
    m_swrContext = swr_alloc_set_opts(NULL, AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16, SAMPLE_RATE/*ctx->sample_rate*/,
                                      ctx->channel_layout, ctx->sample_fmt, ctx->sample_rate, NULL, NULL);
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

QByteArray AVAudio::convert(PlayFrame *frame, CodecContext *codecCtx)
{
    QByteArray buf;
    AVCodecContext *ctx = codecCtx->avCodecCtx();
    // 解码器数据流参数
    int size = av_samples_get_buffer_size(0, ctx->channels, ctx->sample_rate, ctx->sample_fmt, 0);
    uint8_t *audio_out_buffer = (uint8_t *)av_malloc(size);
    Q_ASSERT(audio_out_buffer != nullptr);
    int len = swr_convert(m_swrContext, &audio_out_buffer, size,
                          (const uint8_t **)frame->avFrame()->data, frame->avFrame()->nb_samples);
    if(len == size){
        qWarning() << "audio_out_buffer is too small.";
    }

    if(len > 0){
        // 重采样后的数据流参数
        int dst_bufsize = av_samples_get_buffer_size(0,  CHANNELS, len, AV_SAMPLE_FMT_S16, 1);
        buf = QByteArray((char*)audio_out_buffer, dst_bufsize);
    }else{
        qWarning() << AVError::avErrorString(len);
    }

    av_free(audio_out_buffer);
    return buf;
}

}
