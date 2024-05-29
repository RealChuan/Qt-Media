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

class FFMPEG_EXPORT VideoRender
{
    Q_DISABLE_COPY_MOVE(VideoRender)
public:
    VideoRender();
    virtual ~VideoRender();

    virtual auto isSupportedOutput_pix_fmt(AVPixelFormat pix_fmt) -> bool = 0;
    virtual auto supportedOutput_pix_fmt() -> QVector<AVPixelFormat> = 0;
    virtual auto convertSupported_pix_fmt(const QSharedPointer<Frame> &framePtr)
        -> QSharedPointer<Frame>
        = 0;
    void setFrame(QSharedPointer<Frame> framePtr);
    void setImage(const QImage &image);
    void setSubTitleFrame(const QSharedPointer<Subtitle> &framePtr);
    virtual void resetAllFrame() = 0;

    void setColorSpaceTrc(const ColorUtils::ColorSpaceTrc &colorTrc) { m_colorSpaceTrc = colorTrc; }
    [[nodiscard]] auto colorSpaceTrc() const -> ColorUtils::ColorSpaceTrc
    {
        return m_colorSpaceTrc;
    }

    virtual void setTonemapType(Tonemap::Type type) { m_tonemapType = type; }
    [[nodiscard]] auto tonemapType() const -> Tonemap::Type { return m_tonemapType; }

    virtual void setDestPrimaries(ColorUtils::Primaries::Type type) { m_destPrimaries = type; }
    [[nodiscard]] auto destPrimaries() const -> ColorUtils::Primaries::Type
    {
        return m_destPrimaries;
    }

    void setBackgroundColor(const QColor &color) { m_backgroundColor = color; }
    [[nodiscard]] auto backgroundColor() const -> QColor { return m_backgroundColor; }

    virtual auto widget() -> QWidget * = 0;

    auto fps() -> float;
    void resetFps();

protected:
    // may use in anthoer thread, suggest use QMetaObject::invokeMethod(Qt::QueuedConnection)
    virtual void updateFrame(const QSharedPointer<Frame> &framePtr) = 0;
    virtual void updateSubTitleFrame(const QSharedPointer<Subtitle> &framePtr) = 0;

    ColorUtils::ColorSpaceTrc m_colorSpaceTrc;
    Tonemap::Type m_tonemapType = Tonemap::Type::AUTO;
    ColorUtils::Primaries::Type m_destPrimaries = ColorUtils::Primaries::AUTO;

    QColor m_backgroundColor = Qt::black;

private:
    class VideoRenderPrivate;
    QScopedPointer<VideoRenderPrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // VIDEORENDER_HPP
