#include "decoder.h"

namespace Ffmpeg {

void calculateTime(AVFrame *frame,
                   double &duration,
                   double &pts,
                   AVContextInfo *contextInfo,
                   FormatContext *formatContext)
{
    AVRational tb = contextInfo->stream()->time_base;
    AVRational frame_rate = av_guess_frame_rate(formatContext->avFormatContext(),
                                                contextInfo->stream(),
                                                NULL);
    // 当前帧播放时长
    duration = (frame_rate.num && frame_rate.den
                    ? av_q2d(AVRational{frame_rate.den, frame_rate.num})
                    : 0);
    // 当前帧显示时间戳
    pts = (frame->pts == AV_NOPTS_VALUE) ? NAN : frame->pts * av_q2d(tb);
    //qDebug() << duration << pts;
}

} // namespace Ffmpeg
