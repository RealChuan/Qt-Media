#include "hdrmetadata.hpp"

extern "C" {
#include <libavutil/frame.h>
#include <libavutil/hdr_dynamic_metadata.h>
#include <libavutil/mastering_display_metadata.h>
}

namespace Ffmpeg {

HdrMetaData::HdrMetaData(const FramePtr &framePtr)
{
    auto *avFrame = framePtr->avFrame();
    auto *mdm = av_frame_get_side_data(avFrame, AV_FRAME_DATA_MASTERING_DISPLAY_METADATA);
    auto *clm = av_frame_get_side_data(avFrame, AV_FRAME_DATA_CONTENT_LIGHT_LEVEL);
    auto *dhp = av_frame_get_side_data(avFrame, AV_FRAME_DATA_DYNAMIC_HDR_PLUS);

    if (mdm != nullptr) {
        auto *mdmPtr = reinterpret_cast<AVMasteringDisplayMetadata *>(mdm);
        if (mdmPtr != nullptr) {
            if (mdmPtr->has_luminance != 0) {
                maxLuma = av_q2d(mdmPtr->max_luminance);
                minLuma = av_q2d(mdmPtr->min_luminance);
                if (maxLuma < 10.0 || minLuma >= maxLuma) {
                    maxLuma = minLuma = 0; /* sanity */
                }
            }
            if (mdmPtr->has_primaries != 0) {
                primaries.red.setX(av_q2d(mdmPtr->display_primaries[0][0]));
                primaries.red.setY(av_q2d(mdmPtr->display_primaries[0][1]));
                primaries.green.setX(av_q2d(mdmPtr->display_primaries[1][0]));
                primaries.green.setY(av_q2d(mdmPtr->display_primaries[1][1]));
                primaries.blue.setX(av_q2d(mdmPtr->display_primaries[2][0]));
                primaries.blue.setY(av_q2d(mdmPtr->display_primaries[2][1]));
                primaries.white.setX(av_q2d(mdmPtr->white_point[0]));
                primaries.white.setY(av_q2d(mdmPtr->white_point[1]));
            }
        }
    }
    if (clm != nullptr) {
        auto *clmPtr = reinterpret_cast<AVContentLightMetadata *>(clm);
        if (clmPtr != nullptr) {
            MaxCLL = clmPtr->MaxCLL;
            MaxFALL = clmPtr->MaxFALL;
        }
    }
    if (dhp != nullptr) {
        auto *dhpPtr = reinterpret_cast<AVDynamicHDRPlus *>(dhp);
        if ((dhpPtr != nullptr) && dhpPtr->application_version < 2) {
            float hist_max = 0;
            const auto *pars = &dhpPtr->params[0];
            Q_ASSERT(dhpPtr->num_windows > 0);
            for (int i = 0; i < 3; i++) {
                sceneMax[i] = 10000 * av_q2d(pars->maxscl[i]);
            }
            sceneAvg = 10000 * av_q2d(pars->average_maxrgb);

            // Calculate largest value from histogram to use as fallback for clips
            // with missing MaxSCL information. Note that this may end up picking
            // the "reserved" value at the 5% percentile, which in practice appears
            // to track the brightest pixel in the scene.
            for (int i = 0; i < pars->num_distribution_maxrgb_percentiles; i++) {
                float hist_val = av_q2d(pars->distribution_maxrgb[i].percentile);
                if (hist_val > hist_max) {
                    hist_max = hist_val;
                }
            }
            hist_max *= 10000;
            if (sceneMax[0] == 0.0f) {
                sceneMax[0] = hist_max;
            }
            if (sceneMax[1] == 0.0f) {
                sceneMax[1] = hist_max;
            }
            if (sceneMax[2] == 0.0f) {
                sceneMax[2] = hist_max;
            }

            if (pars->tone_mapping_flag == 1) {
                ootf.targetLuma = av_q2d(dhpPtr->targeted_system_display_maximum_luminance);
                ootf.knee.setX(av_q2d(pars->knee_point_x));
                ootf.knee.setY(av_q2d(pars->knee_point_y));
                assert(pars->num_bezier_curve_anchors < 16);
                for (int i = 0; i < pars->num_bezier_curve_anchors; i++) {
                    ootf.anchors[i] = av_q2d(pars->bezier_curve_anchors[i]);
                }
                ootf.numAnchors = pars->num_bezier_curve_anchors;
            }
        }
    }
}

} // namespace Ffmpeg
