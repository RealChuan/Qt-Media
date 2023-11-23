#ifndef COLORSPACEDIALOG_HPP
#define COLORSPACEDIALOG_HPP

#include <ffmpeg/videorender/videorender.hpp>

#include <QDialog>

class ColorSpaceDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ColorSpaceDialog(QWidget *parent = nullptr);
    ~ColorSpaceDialog() override;

    void setColorSpace(const Ffmpeg::ColorUtils::ColorSpaceTrc &colorTrc);
    [[nodiscard]] auto colorSpace() const -> Ffmpeg::ColorUtils::ColorSpaceTrc;

signals:
    void colorSpaceChanged();

private slots:
    void onContrastSliderChanged(int value);
    void onSaturationSliderChanged(int value);
    void onBrightnessSliderChanged(int value);
    void onContrastSpinBoxChanged(int value);
    void onSaturationSpinBoxChanged(int value);
    void onBrightnessSpinBoxChanged(int value);
    void onReset();

private:
    void buildConnect();

    class ColorSpaceDialogPrivate;
    QScopedPointer<ColorSpaceDialogPrivate> d_ptr;
};

#endif // COLORSPACEDIALOG_HPP
