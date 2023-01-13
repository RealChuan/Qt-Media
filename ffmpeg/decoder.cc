#include "decoder.h"
#include "frame.hpp"
#include "packet.h"

extern "C" {
#include <libavformat/avformat.h>
}

namespace Ffmpeg {

void calculateTime(Frame *frame, AVContextInfo *contextInfo, FormatContext *formatContext)
{
    auto tb = contextInfo->stream()->time_base;
    auto frame_rate = formatContext->guessFrameRate(contextInfo->stream());
    // 当前帧播放时长
    auto duration = (frame_rate.num && frame_rate.den
                         ? av_q2d(AVRational{frame_rate.den, frame_rate.num})
                         : 0);
    // 当前帧显示时间戳
    auto avFrame = frame->avFrame();
    auto pts = (avFrame->pts == AV_NOPTS_VALUE) ? NAN : avFrame->pts * av_q2d(tb);
    frame->setDuration(duration);
    frame->setPts(pts);
    //qDebug() << duration << pts;
}

void calculateTime(Packet *packet, AVContextInfo *contextInfo)
{
    auto tb = contextInfo->stream()->time_base;
    // 当前帧播放时长
    auto avPacket = packet->avPacket();
    auto duration = avPacket->duration * av_q2d(tb);
    // 当前帧显示时间戳
    auto pts = (avPacket->pts == AV_NOPTS_VALUE) ? NAN : avPacket->pts * av_q2d(tb);
    packet->setDuration(duration);
    packet->setPts(pts);
    //qDebug() << duration << pts;
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

std::atomic<double> g_mediaSpeed = 1.0;

void setMediaSpeed(double speed)
{
    Q_ASSERT(speed > 0);
    g_mediaSpeed.store(speed);
}

double mediaSpeed()
{
    return g_mediaSpeed.load();
}

} // namespace Ffmpeg
