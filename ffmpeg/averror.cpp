#include "averror.h"

extern "C"{
#include <libavutil/error.h>
}

namespace Ffmpeg {

QString AVError::errorString() const
{
    char buf[1024] = {0};
    av_strerror(m_error, buf, sizeof buf);
    return buf;
}

QString AVError::avErrorString(int error)
{
    return AVError(error).errorString();
}


}
