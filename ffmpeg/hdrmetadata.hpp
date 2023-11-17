#ifndef HDRMETADATA_HPP
#define HDRMETADATA_HPP

#include <QtCore>

namespace Ffmpeg {

class Frame;

struct RawPrimaries
{
    QPointF red = {0, 0}, green = {0, 0}, blue = {0, 0}, white = {0, 0};
};

struct HdrBezier
{
    float targetLuma = 0;    // target luminance (cd/m²) for this OOTF
    QPointF knee = {0, 0};   // cross-over knee point (0-1)
    float anchors[15] = {0}; // intermediate bezier curve control points (0-1)
    uint8_t numAnchors = 0;
};

struct HdrMetaData
{
public:
    HdrMetaData() = default;
    explicit HdrMetaData(Frame *frame);

    float minLuma = 0, maxLuma = 0;
    RawPrimaries primaries;

    unsigned MaxCLL = 0, MaxFALL = 0;

    float sceneMax[3] = {0}, sceneAvg = 0;
    HdrBezier ootf;
};

} // namespace Ffmpeg

#endif // HDRMETADATA_HPP
