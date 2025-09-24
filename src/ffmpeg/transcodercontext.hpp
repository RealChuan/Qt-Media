#pragma once

#include "frame.hpp"

#include <QSharedPointer>

namespace Ffmpeg {

class AVContextInfo;
class Filter;
class AudioFifo;

struct TranscoderContext
{
    [[nodiscard]] auto initFilter(const QString &filter_spec, const FramePtr &framePtr) const
        -> bool;

    QSharedPointer<AVContextInfo> decContextInfoPtr;
    QSharedPointer<AVContextInfo> encContextInfoPtr;

    QSharedPointer<Filter> filterPtr;

    QSharedPointer<AudioFifo> audioFifoPtr;
    qint64 audioPts = 0;

    bool vaild = false;
    int outStreamIndex = -1;
};

} // namespace Ffmpeg
