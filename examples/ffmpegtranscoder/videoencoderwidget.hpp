#ifndef VIDEOENCODERWIDGET_HPP
#define VIDEOENCODERWIDGET_HPP

#include <ffmpeg/encodecontext.hpp>

#include <QWidget>

extern "C" {
#include <libavcodec/codec_id.h>
}

class VideoEncoderWidget : public QWidget
{
    Q_OBJECT
public:
    explicit VideoEncoderWidget(QWidget *parent = nullptr);
    ~VideoEncoderWidget() override;

    void setDecodeContext(const Ffmpeg::EncodeContext &decodeContext);
    [[nodiscard]] auto encodeContext() const -> Ffmpeg::EncodeContext;

    [[nodiscard]] auto isGpuDecode() const -> bool;

private slots:
    void onEncoderChanged();
    void onVideoWidthChanged();
    void onVideoHeightChanged();
    void onQualityTypeChanged();

private:
    void setupUI();
    void buildConnect();

    class VideoEncoderWidgetPrivate;
    QScopedPointer<VideoEncoderWidgetPrivate> d_ptr;
};

#endif // VIDEOENCODERWIDGET_HPP
