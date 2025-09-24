#pragma once

#include "colorutils.hpp"
#include "frame.hpp"

namespace Ffmpeg {

struct HdrBezier
{
    float targetLuma = 0;                // target luminance (cd/m²) for this OOTF
    QPointF knee = {0, 0};               // cross-over knee point (0-1)
    std::array<float, 15> anchors = {0}; // intermediate bezier curve control points (0-1)
    uint8_t numAnchors = 0;
};

struct HdrMetaData
{
public:
    HdrMetaData() = default;
    explicit HdrMetaData(const FramePtr &framePtr);

    float minLuma = 0, maxLuma = 0;
    ColorUtils::RawPrimaries primaries;

    unsigned MaxCLL = 0, MaxFALL = 0;

    std::array<float, 3> sceneMax = {0};
    float sceneAvg = 0;
    HdrBezier ootf;
};

} // namespace Ffmpeg
