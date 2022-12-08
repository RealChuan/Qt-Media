#include "decoder.h"
#include "frame.hpp"
#include "packet.h"

namespace Ffmpeg {

void calculateTime(Frame *frame, AVContextInfo *contextInfo, FormatContext *formatContext)
{
    AVRational tb = contextInfo->stream()->time_base;
    AVRational frame_rate = av_guess_frame_rate(formatContext->avFormatContext(),
                                                contextInfo->stream(),
                                                NULL);
    // 当前帧播放时长
    double duration = (frame_rate.num && frame_rate.den
                           ? av_q2d(AVRational{frame_rate.den, frame_rate.num})
                           : 0);
    // 当前帧显示时间戳
    auto avFrame = frame->avFrame();
    double pts = (avFrame->pts == AV_NOPTS_VALUE) ? NAN : avFrame->pts * av_q2d(tb);
    frame->setDuration(duration);
    frame->setPts(pts);
    //qDebug() << duration << pts;
}

void calculateTime(Packet *packet, AVContextInfo *contextInfo)
{
    AVRational tb = contextInfo->stream()->time_base;
    //AVRational frame_rate = av_guess_frame_rate(m_formatContext->avFormatContext(), m_contextInfo->stream(), NULL);
    // 当前帧播放时长
    auto avPacket = packet->avPacket();
    //duration = (frame_rate.num && frame_rate.den ? av_q2d(AVRational{frame_rate.den, frame_rate.num}) : 0);
    double duration = avPacket->duration * av_q2d(tb);
    // 当前帧显示时间戳
    double pts = (avPacket->pts == AV_NOPTS_VALUE) ? NAN : avPacket->pts * av_q2d(tb);
    packet->setDuration(duration);
    packet->setPts(pts);
    //qDebug() << duration << pts;}
}

static std::atomic<double> g_meidaClock;

void setMediaClock(double value)
{
    g_meidaClock.store(value);
}

double mediaClock()
{
    return g_meidaClock.load();
}

} // namespace Ffmpeg
