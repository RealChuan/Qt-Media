#ifndef VIDEORENDER_HPP
#define VIDEORENDER_HPP

#include "tonemap.hpp"

#include <ffmpeg/colorutils.hpp>
#include <ffmpeg/ffmepg_global.h>

#include <QWidget>

extern "C" {
#include <libavutil/pixfmt.h>
}

namespace Ffmpeg {

class Frame;
class Subtitle;

struct ColorSpaceTrc
{
    auto operator=(const ColorSpaceTrc &other) -> ColorSpaceTrc &;

    const float contrast_min = 0.0;
    const float contrast_max = 2.0;
    const float contrast_default = 1.0;
    float contrast = 1.0;

    const float saturation_min = 0.0;
    const float saturation_max = 2.0;
    const float saturation_default = 1.0;
    float saturation = 1.0;

    const float brightness_min = -1.0;
    const float brightness_max = 1.0;
    const float brightness_default = 0.0;
    float brightness = 0.0;
};

class FFMPEG_EXPORT VideoRender
{
    Q_DISABLE_COPY_MOVE(VideoRender)
public:
    VideoRender();
    virtual ~VideoRender();

    virtual auto isSupportedOutput_pix_fmt(AVPixelFormat pix_fmt) -> bool = 0;
    virtual auto supportedOutput_pix_fmt() -> QVector<AVPixelFormat> = 0;
    virtual auto convertSupported_pix_fmt(QSharedPointer<Frame> framePtr) -> QSharedPointer<Frame>
        = 0;
    void setFrame(QSharedPointer<Frame> framePtr);
    void setImage(const QImage &image);
    void setSubTitleFrame(QSharedPointer<Subtitle> framePtr);
    virtual void resetAllFrame() = 0;

    void setColorSpaceTrc(const ColorSpaceTrc &colorTrc) { m_colorSpaceTrc = colorTrc; }
    [[nodiscard]] auto colorSpaceTrc() const -> ColorSpaceTrc { return m_colorSpaceTrc; }

    virtual void setTonemapType(Tonemap::Type type) { m_tonemapType = type; }
    [[nodiscard]] auto tonemapType() const -> Tonemap::Type { return m_tonemapType; }

    virtual void setDestPrimaries(ColorUtils::Primaries::Type type) { m_destPrimaries = type; }
    [[nodiscard]] auto destPrimaries() const -> ColorUtils::Primaries::Type
    {
        return m_destPrimaries;
    }

    virtual auto widget() -> QWidget * = 0;

    auto fps() -> float;
    void resetFps();

protected:
    // may use in anthoer thread, suggest use QMetaObject::invokeMethod(Qt::QueuedConnection)
    virtual void updateFrame(QSharedPointer<Frame> frame) = 0;
    virtual void updateSubTitleFrame(QSharedPointer<Subtitle> frame) = 0;

    ColorSpaceTrc m_colorSpaceTrc;
    Tonemap::Type m_tonemapType = Tonemap::Type::AUTO;
    ColorUtils::Primaries::Type m_destPrimaries = ColorUtils::Primaries::AUTO;

private:
    class VideoRenderPrivate;
    QScopedPointer<VideoRenderPrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // VIDEORENDER_HPP
