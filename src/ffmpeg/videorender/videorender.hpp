#pragma once

#include "tonemapping.hpp"

#include <ffmpeg/colorutils.hpp>
#include <ffmpeg/ffmepg_global.h>
#include <ffmpeg/frame.hpp>
#include <mediaconfig/equalizer.hpp>

#include <QWidget>

extern "C" {
#include <libavutil/pixfmt.h>
}

namespace Ffmpeg {

class Subtitle;

class FFMPEG_EXPORT VideoRender
{
    Q_DISABLE_COPY_MOVE(VideoRender)
public:
    VideoRender();
    virtual ~VideoRender();

    virtual auto isSupportedOutput_pix_fmt(AVPixelFormat pix_fmt) -> bool = 0;
    virtual auto supportedOutput_pix_fmt() -> QList<AVPixelFormat> = 0;
    virtual auto convertSupported_pix_fmt(const FramePtr &framePtr) -> FramePtr = 0;
    void setFrame(FramePtr framePtr);
    void setImage(const QImage &image);
    void setSubTitleFrame(const QSharedPointer<Subtitle> &framePtr);
    virtual void resetAllFrame() = 0;

    void setEqualizer(const MediaConfig::Equalizer &equalizer) { m_equalizer = equalizer; }
    [[nodiscard]] auto equalizer() const -> MediaConfig::Equalizer { return m_equalizer; }

    virtual void setToneMappingType(ToneMapping::Type type) { m_tonemapType = type; }
    [[nodiscard]] auto tonemapType() const -> ToneMapping::Type { return m_tonemapType; }

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
    virtual void updateFrame(const FramePtr &framePtr) = 0;
    virtual void updateSubTitleFrame(const QSharedPointer<Subtitle> &framePtr) = 0;

    MediaConfig::Equalizer m_equalizer;
    ToneMapping::Type m_tonemapType = ToneMapping::Type::AUTO;
    ColorUtils::Primaries::Type m_destPrimaries = ColorUtils::Primaries::AUTO;

    QColor m_backgroundColor = Qt::black;

private:
    class VideoRenderPrivate;
    QScopedPointer<VideoRenderPrivate> d_ptr;
};

} // namespace Ffmpeg
