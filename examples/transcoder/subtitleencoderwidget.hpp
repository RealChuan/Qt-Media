#ifndef SUBTITLEENCODERWIDGET_HPP
#define SUBTITLEENCODERWIDGET_HPP

#include <ffmpeg/encodecontext.hpp>

#include <QWidget>

class SubtitleEncoderWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SubtitleEncoderWidget(QWidget *parent = nullptr);
    ~SubtitleEncoderWidget() override;

    void setDecodeContext(const Ffmpeg::EncodeContexts &decodeContexts);
    [[nodiscard]] auto encodeContexts() const -> Ffmpeg::EncodeContexts;

private slots:
    void onReset();
    void onLoad();

private:
    void setupUI();

    class SubtitleEncoderWidgetPrivate;
    QScopedPointer<SubtitleEncoderWidgetPrivate> d_ptr;
};

#endif // SUBTITLEENCODERWIDGET_HPP
