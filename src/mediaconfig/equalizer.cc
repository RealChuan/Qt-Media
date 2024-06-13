#include "equalizer.hpp"

#include <utils/utils.h>

namespace MediaConfig {

class Equalizer::EqualizerPrivate
{
public:
    EqualizerPrivate(Equalizer *q)
        : q_ptr(q)
        , contrastRange(-100, 100)
        , saturationRange(-100, 100)
        , brightnessRange(-100, 100)
        , gammaRange(-100, 100)
        , hueRange(-100, 100)
    {
        contrastRange.setValue(0);
        saturationRange.setValue(0);
        brightnessRange.setValue(0);
        gammaRange.setValue(0);
        hueRange.setValue(0);
    }

    Equalizer *q_ptr;

    EqualizerRange contrastRange;
    EqualizerRange saturationRange;
    EqualizerRange brightnessRange;
    EqualizerRange gammaRange;
    EqualizerRange hueRange;
};

Equalizer::Equalizer()
    : d_ptr(new EqualizerPrivate(this))
{}

Equalizer::Equalizer(const Equalizer &other)
    : d_ptr(new EqualizerPrivate(this))
{
    d_ptr->contrastRange = other.d_ptr->contrastRange;
    d_ptr->saturationRange = other.d_ptr->saturationRange;
    d_ptr->brightnessRange = other.d_ptr->brightnessRange;
    d_ptr->gammaRange = other.d_ptr->gammaRange;
    d_ptr->hueRange = other.d_ptr->hueRange;
}

Equalizer &Equalizer::operator=(const Equalizer &other)
{
    d_ptr->contrastRange = other.d_ptr->contrastRange;
    d_ptr->saturationRange = other.d_ptr->saturationRange;
    d_ptr->brightnessRange = other.d_ptr->brightnessRange;
    d_ptr->gammaRange = other.d_ptr->gammaRange;
    d_ptr->hueRange = other.d_ptr->hueRange;

    return *this;
}

Equalizer::~Equalizer() {}

bool Equalizer::operator==(const Equalizer &other) const
{
    return d_ptr->contrastRange == other.d_ptr->contrastRange
           && d_ptr->saturationRange == other.d_ptr->saturationRange
           && d_ptr->brightnessRange == other.d_ptr->brightnessRange
           && d_ptr->gammaRange == other.d_ptr->gammaRange
           && d_ptr->hueRange == other.d_ptr->hueRange;
}

bool Equalizer::operator!=(const Equalizer &other) const
{
    return !(*this == other);
}

Equalizer::EqualizerRange &Equalizer::contrastRange() const
{
    return d_ptr->contrastRange;
}

float Equalizer::ffContrast() const
{
    return Utils::rangeMap(d_ptr->contrastRange.value(),
                           d_ptr->contrastRange.min(),
                           d_ptr->contrastRange.max(),
                           0.0,
                           2.0);
}

float Equalizer::eqContrast() const
{
    return Utils::rangeMap(d_ptr->contrastRange.value(),
                           d_ptr->contrastRange.min(),
                           d_ptr->contrastRange.max(),
                           0.0,
                           2.0);
    // The value must be a float value in range -1000.0 to 1000.0. The default value is "1".
}

Equalizer::EqualizerRange &Equalizer::saturationRange() const
{
    return d_ptr->saturationRange;
}

float Equalizer::ffSaturation() const
{
    return Utils::rangeMap(d_ptr->saturationRange.value(),
                           d_ptr->saturationRange.min(),
                           d_ptr->saturationRange.max(),
                           0.0,
                           2.0);
}

float Equalizer::eqSaturation() const
{
    // The value must be a float in range 0.0 to 3.0. The default value is "1".
    auto value = d_ptr->saturationRange.value();
    if (value == 0) {
        return 1.0;
    }
    if (value > 1) {
        return Utils::rangeMap(value, 0, d_ptr->saturationRange.max(), 1.0, 3.0);
    }
    return Utils::rangeMap(value, d_ptr->saturationRange.min(), 0, 0, 1.0);
}

Equalizer::EqualizerRange &Equalizer::brightnessRange() const
{
    return d_ptr->brightnessRange;
}

float Equalizer::ffBrightness() const
{
    return Utils::rangeMap(d_ptr->brightnessRange.value(),
                           d_ptr->brightnessRange.min(),
                           d_ptr->brightnessRange.max(),
                           -1,
                           1.0);
}

float Equalizer::eqBrightness() const
{
    return Utils::rangeMap(d_ptr->brightnessRange.value(),
                           d_ptr->brightnessRange.min(),
                           d_ptr->brightnessRange.max(),
                           -1,
                           1);
    // The value must be a float value in range -1000.0 to 1000.0. The default value is "1".
}

Equalizer::EqualizerRange &Equalizer::gammaRange() const
{
    return d_ptr->gammaRange;
}

Equalizer::EqualizerRange &Equalizer::hueRange() const
{
    return d_ptr->hueRange;
}

} // namespace MediaConfig
