#include "colorspacedialog.hpp"
#include "slider.h"

#include <QtWidgets>

void setBlockValue(QSpinBox *spinBox, int value)
{
    spinBox->blockSignals(true);
    spinBox->setValue(value);
    spinBox->blockSignals(false);
}

void setBlockValue(QSlider *slider, int value)
{
    slider->blockSignals(true);
    slider->setValue(value);
    slider->blockSignals(false);
}

class ColorSpaceDialog::ColorSpaceDialogPrivate
{
public:
    explicit ColorSpaceDialogPrivate(ColorSpaceDialog *q)
        : q_ptr(q)
    {
        Ffmpeg::ColorUtils::ColorSpaceTrc colorTrc;

        contrastSlider = new Slider(q_ptr);
        contrastSlider->setRange(colorTrc.contrast_min * multiple, colorTrc.contrast_max * multiple);
        contrastSlider->setValue(colorTrc.contrast() * multiple);
        saturationSlider = new Slider(q_ptr);
        saturationSlider->setRange(colorTrc.saturation_min * multiple,
                                   colorTrc.saturation_max * multiple);
        saturationSlider->setValue(colorTrc.saturation() * multiple);
        brightnessSlider = new Slider(q_ptr);
        brightnessSlider->setRange(colorTrc.brightness_min * multiple,
                                   colorTrc.brightness_max * multiple);
        brightnessSlider->setValue(colorTrc.brightness() * multiple);

        contrastSpinBox = new QSpinBox(q_ptr);
        contrastSpinBox->setKeyboardTracking(false);
        contrastSpinBox->setRange(colorTrc.contrast_min * multiple,
                                  colorTrc.contrast_max * multiple);
        contrastSpinBox->setValue(colorTrc.contrast() * multiple);
        saturationSpinBox = new QSpinBox(q_ptr);
        saturationSpinBox->setKeyboardTracking(false);
        saturationSpinBox->setRange(colorTrc.saturation_min * multiple,
                                    colorTrc.saturation_max * multiple);
        saturationSpinBox->setValue(colorTrc.saturation() * multiple);
        brightnessSpinBox = new QSpinBox(q_ptr);
        brightnessSpinBox->setKeyboardTracking(false);
        brightnessSpinBox->setRange(colorTrc.brightness_min * multiple,
                                    colorTrc.brightness_max * multiple);
        brightnessSpinBox->setValue(colorTrc.brightness() * multiple);

        resetButton = new QToolButton(q_ptr);
        resetButton->setText("Reset");
    }

    void setupUI()
    {
        auto *layout = new QGridLayout(q_ptr);
        layout->addWidget(new QLabel(QObject::tr("Contrast:"), q_ptr), 0, 0);
        layout->addWidget(contrastSlider, 0, 1);
        layout->addWidget(contrastSpinBox, 0, 2);
        layout->addWidget(new QLabel(QObject::tr("Saturation:"), q_ptr), 1, 0);
        layout->addWidget(saturationSlider, 1, 1);
        layout->addWidget(saturationSpinBox, 1, 2);
        layout->addWidget(new QLabel(QObject::tr("Brightness:"), q_ptr), 2, 0);
        layout->addWidget(brightnessSlider, 2, 1);
        layout->addWidget(brightnessSpinBox, 2, 2);
        layout->addWidget(resetButton, 3, 2, Qt::AlignRight);
    }

    ColorSpaceDialog *q_ptr;

    Slider *contrastSlider;
    Slider *saturationSlider;
    Slider *brightnessSlider;
    QSpinBox *contrastSpinBox;
    QSpinBox *saturationSpinBox;
    QSpinBox *brightnessSpinBox;
    QToolButton *resetButton;

    const float multiple = 100.0;
};

ColorSpaceDialog::ColorSpaceDialog(QWidget *parent)
    : QDialog(parent)
    , d_ptr(new ColorSpaceDialogPrivate(this))
{
    d_ptr->setupUI();
    buildConnect();
    resize(400, 250);
}

ColorSpaceDialog::~ColorSpaceDialog() = default;

void ColorSpaceDialog::setColorSpace(const Ffmpeg::ColorUtils::ColorSpaceTrc &colorTrc)
{
    setBlockValue(d_ptr->contrastSpinBox, colorTrc.contrast() * d_ptr->multiple);
    setBlockValue(d_ptr->saturationSpinBox, colorTrc.saturation() * d_ptr->multiple);
    setBlockValue(d_ptr->brightnessSpinBox, colorTrc.brightness() * d_ptr->multiple);
    setBlockValue(d_ptr->contrastSlider, colorTrc.contrast() * d_ptr->multiple);
    setBlockValue(d_ptr->saturationSlider, colorTrc.saturation() * d_ptr->multiple);
    setBlockValue(d_ptr->brightnessSlider, colorTrc.brightness() * d_ptr->multiple);
}

Ffmpeg::ColorUtils::ColorSpaceTrc ColorSpaceDialog::colorSpace() const
{
    Ffmpeg::ColorUtils::ColorSpaceTrc colorTrc;
    colorTrc.setContrast(d_ptr->contrastSlider->value() / d_ptr->multiple);
    colorTrc.setSaturation(d_ptr->saturationSlider->value() / d_ptr->multiple);
    colorTrc.setBrightness(d_ptr->brightnessSlider->value() / d_ptr->multiple);
    return colorTrc;
}

void ColorSpaceDialog::onContrastSliderChanged(int value)
{
    setBlockValue(d_ptr->contrastSpinBox, value);
    emit d_ptr->q_ptr->colorSpaceChanged();
}

void ColorSpaceDialog::onSaturationSliderChanged(int value)
{
    setBlockValue(d_ptr->saturationSpinBox, value);
    emit d_ptr->q_ptr->colorSpaceChanged();
}

void ColorSpaceDialog::onBrightnessSliderChanged(int value)
{
    setBlockValue(d_ptr->brightnessSpinBox, value);
    emit d_ptr->q_ptr->colorSpaceChanged();
}

void ColorSpaceDialog::onContrastSpinBoxChanged(int value)
{
    setBlockValue(d_ptr->contrastSlider, value);
    emit d_ptr->q_ptr->colorSpaceChanged();
}

void ColorSpaceDialog::onSaturationSpinBoxChanged(int value)
{
    setBlockValue(d_ptr->saturationSlider, value);
    emit d_ptr->q_ptr->colorSpaceChanged();
}

void ColorSpaceDialog::onBrightnessSpinBoxChanged(int value)
{
    setBlockValue(d_ptr->brightnessSlider, value);
    emit d_ptr->q_ptr->colorSpaceChanged();
}

void ColorSpaceDialog::onReset()
{
    Ffmpeg::ColorUtils::ColorSpaceTrc colorTrc;
    setBlockValue(d_ptr->contrastSlider, colorTrc.contrast_default * d_ptr->multiple);
    setBlockValue(d_ptr->saturationSlider, colorTrc.saturation_default * d_ptr->multiple);
    setBlockValue(d_ptr->brightnessSlider, colorTrc.brightness_default * d_ptr->multiple);
    setBlockValue(d_ptr->contrastSpinBox, colorTrc.contrast_default * d_ptr->multiple);
    setBlockValue(d_ptr->saturationSpinBox, colorTrc.saturation_default * d_ptr->multiple);
    setBlockValue(d_ptr->brightnessSpinBox, colorTrc.brightness_default * d_ptr->multiple);
    emit d_ptr->q_ptr->colorSpaceChanged();
}

void ColorSpaceDialog::buildConnect()
{
    connect(d_ptr->contrastSlider,
            &Slider::valueChanged,
            this,
            &ColorSpaceDialog::onContrastSliderChanged);
    connect(d_ptr->saturationSlider,
            &Slider::valueChanged,
            this,
            &ColorSpaceDialog::onSaturationSliderChanged);
    connect(d_ptr->brightnessSlider,
            &Slider::valueChanged,
            this,
            &ColorSpaceDialog::onBrightnessSliderChanged);
    connect(d_ptr->contrastSpinBox,
            &QSpinBox::valueChanged,
            this,
            &ColorSpaceDialog::onContrastSpinBoxChanged);
    connect(d_ptr->saturationSpinBox,
            &QSpinBox::valueChanged,
            this,
            &ColorSpaceDialog::onSaturationSpinBoxChanged);
    connect(d_ptr->brightnessSpinBox,
            &QSpinBox::valueChanged,
            this,
            &ColorSpaceDialog::onBrightnessSpinBoxChanged);

    connect(d_ptr->resetButton, &QToolButton::clicked, this, &ColorSpaceDialog::onReset);
}
