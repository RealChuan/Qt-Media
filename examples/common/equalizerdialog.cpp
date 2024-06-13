#include "equalizerdialog.h"

#include <examples/common/slider.h>

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

void init(QSlider *slider,
          QSpinBox *spinBox,
          const MediaConfig::Equalizer::EqualizerRange &equalizerRange)
{
    slider->setRange(equalizerRange.min(), equalizerRange.max());
    slider->setValue(equalizerRange.value());

    spinBox->setKeyboardTracking(false);
    spinBox->setRange(slider->minimum(), slider->maximum());
    spinBox->setValue(slider->value());
}

class EqualizerDialog::EqualizerDialogPrivate
{
public:
    explicit EqualizerDialogPrivate(EqualizerDialog *q)
        : q_ptr(q)
    {
        MediaConfig::Equalizer equalizer;

        contrastSlider = new Slider(q_ptr);
        contrastSpinBox = new QSpinBox(q_ptr);
        init(contrastSlider, contrastSpinBox, equalizer.contrastRange());

        saturationSlider = new Slider(q_ptr);
        saturationSpinBox = new QSpinBox(q_ptr);
        init(saturationSlider, saturationSpinBox, equalizer.saturationRange());

        brightnessSlider = new Slider(q_ptr);
        brightnessSpinBox = new QSpinBox(q_ptr);
        init(brightnessSlider, brightnessSpinBox, equalizer.brightnessRange());

        gammaSlider = new Slider(q_ptr);
        gammaSpinBox = new QSpinBox(q_ptr);
        init(gammaSlider, gammaSpinBox, equalizer.gammaRange());

        hueSlider = new Slider(q_ptr);
        hueSpinBox = new QSpinBox(q_ptr);
        init(hueSlider, hueSpinBox, equalizer.hueRange());

        resetButton = new QToolButton(q_ptr);
        resetButton->setText("Reset");
    }

    void setupUI() const
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
        layout->addWidget(new QLabel(QObject::tr("Gamma:"), q_ptr), 3, 0);
        layout->addWidget(gammaSlider, 3, 1);
        layout->addWidget(gammaSpinBox, 3, 2);
        layout->addWidget(new QLabel(QObject::tr("Hue:"), q_ptr), 4, 0);
        layout->addWidget(hueSlider, 4, 1);
        layout->addWidget(hueSpinBox, 4, 2);
        layout->addWidget(resetButton, 5, 2, Qt::AlignRight);
    }

    EqualizerDialog *q_ptr;

    Slider *contrastSlider;
    QSpinBox *contrastSpinBox;

    Slider *saturationSlider;
    QSpinBox *saturationSpinBox;

    Slider *brightnessSlider;
    QSpinBox *brightnessSpinBox;

    Slider *gammaSlider;
    QSpinBox *gammaSpinBox;

    Slider *hueSlider;
    QSpinBox *hueSpinBox;

    QToolButton *resetButton;
};

EqualizerDialog::EqualizerDialog(QWidget *parent)
    : QDialog(parent)
    , d_ptr(new EqualizerDialogPrivate(this))
{
    d_ptr->setupUI();
    buildConnect();
    resize(400, 250);
}

EqualizerDialog::~EqualizerDialog() = default;

void EqualizerDialog::setEqualizer(const MediaConfig::Equalizer &equalizer)
{
    setBlockValue(d_ptr->contrastSlider, equalizer.contrastRange().value());
    setBlockValue(d_ptr->contrastSpinBox, equalizer.contrastRange().value());

    setBlockValue(d_ptr->saturationSlider, equalizer.saturationRange().value());
    setBlockValue(d_ptr->saturationSpinBox, equalizer.saturationRange().value());

    setBlockValue(d_ptr->brightnessSlider, equalizer.brightnessRange().value());
    setBlockValue(d_ptr->brightnessSpinBox, equalizer.brightnessRange().value());

    setBlockValue(d_ptr->gammaSlider, equalizer.gammaRange().value());
    setBlockValue(d_ptr->gammaSpinBox, equalizer.gammaRange().value());

    setBlockValue(d_ptr->hueSlider, equalizer.hueRange().value());
    setBlockValue(d_ptr->hueSpinBox, equalizer.hueRange().value());
}

MediaConfig::Equalizer EqualizerDialog::equalizer() const
{
    MediaConfig::Equalizer equalizer;
    equalizer.contrastRange().setValue(d_ptr->contrastSlider->value());
    equalizer.saturationRange().setValue(d_ptr->saturationSlider->value());
    equalizer.brightnessRange().setValue(d_ptr->brightnessSlider->value());
    equalizer.gammaRange().setValue(d_ptr->gammaSlider->value());
    equalizer.hueRange().setValue(d_ptr->hueSlider->value());
    return equalizer;
}

void EqualizerDialog::onContrastSliderChanged(int value)
{
    setBlockValue(d_ptr->contrastSpinBox, value);
    emit equalizerChanged();
}

void EqualizerDialog::onSaturationSliderChanged(int value)
{
    setBlockValue(d_ptr->saturationSpinBox, value);
    emit equalizerChanged();
}

void EqualizerDialog::onBrightnessSliderChanged(int value)
{
    setBlockValue(d_ptr->brightnessSpinBox, value);
    emit equalizerChanged();
}

void EqualizerDialog::onContrastSpinBoxChanged(int value)
{
    setBlockValue(d_ptr->contrastSlider, value);
    emit equalizerChanged();
}

void EqualizerDialog::onSaturationSpinBoxChanged(int value)
{
    setBlockValue(d_ptr->saturationSlider, value);
    emit equalizerChanged();
}

void EqualizerDialog::onBrightnessSpinBoxChanged(int value)
{
    setBlockValue(d_ptr->brightnessSlider, value);
    emit equalizerChanged();
}

void EqualizerDialog::onGammaSliderChanged(int value)
{
    setBlockValue(d_ptr->gammaSpinBox, value);
    emit equalizerChanged();
}

void EqualizerDialog::onGammaSpinBoxChanged(int value)
{
    setBlockValue(d_ptr->gammaSlider, value);
    emit equalizerChanged();
}

void EqualizerDialog::onHueSliderChanged(int value)
{
    setBlockValue(d_ptr->hueSpinBox, value);
    emit equalizerChanged();
}

void EqualizerDialog::onHueSpinBoxChanged(int value)
{
    setBlockValue(d_ptr->hueSlider, value);
    emit equalizerChanged();
}

void EqualizerDialog::onReset()
{
    setEqualizer({});
    emit equalizerChanged();
}

void EqualizerDialog::buildConnect()
{
    connect(d_ptr->contrastSlider,
            &Slider::valueChanged,
            this,
            &EqualizerDialog::onContrastSliderChanged);
    connect(d_ptr->contrastSpinBox,
            &QSpinBox::valueChanged,
            this,
            &EqualizerDialog::onContrastSpinBoxChanged);

    connect(d_ptr->saturationSlider,
            &Slider::valueChanged,
            this,
            &EqualizerDialog::onSaturationSliderChanged);
    connect(d_ptr->saturationSpinBox,
            &QSpinBox::valueChanged,
            this,
            &EqualizerDialog::onSaturationSpinBoxChanged);

    connect(d_ptr->brightnessSlider,
            &Slider::valueChanged,
            this,
            &EqualizerDialog::onBrightnessSliderChanged);
    connect(d_ptr->brightnessSpinBox,
            &QSpinBox::valueChanged,
            this,
            &EqualizerDialog::onBrightnessSpinBoxChanged);

    connect(d_ptr->gammaSlider, &Slider::valueChanged, this, &EqualizerDialog::onGammaSliderChanged);
    connect(d_ptr->gammaSpinBox,
            &QSpinBox::valueChanged,
            this,
            &EqualizerDialog::onGammaSpinBoxChanged);

    connect(d_ptr->hueSlider, &Slider::valueChanged, this, &EqualizerDialog::onHueSliderChanged);
    connect(d_ptr->hueSpinBox, &QSpinBox::valueChanged, this, &EqualizerDialog::onHueSpinBoxChanged);

    connect(d_ptr->resetButton, &QToolButton::clicked, this, &EqualizerDialog::onReset);
}
