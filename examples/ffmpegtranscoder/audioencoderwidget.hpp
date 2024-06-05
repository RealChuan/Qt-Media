#ifndef AUDIOENCODERWIDGET_HPP
#define AUDIOENCODERWIDGET_HPP

#include <ffmpeg/encodecontext.hpp>

#include <QWidget>

class AudioEncoderWidget : public QWidget
{
    Q_OBJECT
public:
    explicit AudioEncoderWidget(QWidget *parent = nullptr);
    ~AudioEncoderWidget() override;

    void setDecodeContext(const Ffmpeg::EncodeContexts &decodeContexts);
    [[nodiscard]] auto encodeContexts() const -> Ffmpeg::EncodeContexts;

private slots:
    void onReset();

private:
    void setupUI();

    class AudioEncoderWidgetPrivate;
    QScopedPointer<AudioEncoderWidgetPrivate> d_ptr;
};

#endif // AUDIOENCODERWIDGET_HPP
