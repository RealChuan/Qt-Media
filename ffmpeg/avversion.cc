#include "avversion.hpp"

#include <QDebug>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

namespace Ffmpeg {

void printFfmpegInfo()
{
    qInfo() << "ffmpeg version:" << av_version_info();
    qInfo() << "libavfilter version:" << avfilter_version();
    qInfo() << "libavformat version:" << avformat_version();
    qInfo() << "libavcodec version:" << avcodec_version();
    qInfo() << "libswresample version:" << swresample_version();
    qInfo() << "libswscale version:" << swscale_version();
    qInfo() << "libavutil version:" << swscale_version();
    qInfo() << avutil_license();
    qInfo() << avutil_configuration();
}

} // namespace Ffmpeg
