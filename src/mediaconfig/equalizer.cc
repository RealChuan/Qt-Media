#include "equalizer.hpp"

#include <utils/utils.h>

namespace MediaConfig {

class Equalizer::EqualizerPrivate
{
public:
    explicit EqualizerPrivate(Equalizer *q)
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

auto Equalizer::operator=(const Equalizer &other) -> Equalizer &
{
    d_ptr->contrastRange = other.d_ptr->contrastRange;
    d_ptr->saturationRange = other.d_ptr->saturationRange;
    d_ptr->brightnessRange = other.d_ptr->brightnessRange;
    d_ptr->gammaRange = other.d_ptr->gammaRange;
    d_ptr->hueRange = other.d_ptr->hueRange;

    return *this;
}

Equalizer::~Equalizer() = default;

auto Equalizer::operator==(const Equalizer &other) const -> bool
{
    return d_ptr->contrastRange == other.d_ptr->contrastRange
           && d_ptr->saturationRange == other.d_ptr->saturationRange
           && d_ptr->brightnessRange == other.d_ptr->brightnessRange
           && d_ptr->gammaRange == other.d_ptr->gammaRange
           && d_ptr->hueRange == other.d_ptr->hueRange;
}

auto Equalizer::operator!=(const Equalizer &other) const -> bool
{
    return !(*this == other);
}

auto Equalizer::contrastRange() const -> Equalizer::EqualizerRange &
{
    return d_ptr->contrastRange;
}

auto Equalizer::ffContrast() const -> float
{
    return eqContrast();
}

auto Equalizer::eqContrast() const -> float
{
    // The value must be a float value in range -1000.0 to 1000.0. The default value is "1".
    return Utils::rangeMap(d_ptr->contrastRange.value(),
                           d_ptr->contrastRange.min(),
                           d_ptr->contrastRange.max(),
                           0.0,
                           2.0);
}

auto Equalizer::saturationRange() const -> Equalizer::EqualizerRange &
{
    return d_ptr->saturationRange;
}

auto Equalizer::ffSaturation() const -> float
{
    return eqSaturation();
}

auto Equalizer::eqSaturation() const -> float
{
    // The value must be a float in range 0.0 to 3.0. The default value is "1".
    auto value = d_ptr->saturationRange.value();
    if (value == 0) {
        return 1.0;
    }
    if (value > 0) {
        return Utils::rangeMap(value, 0, d_ptr->saturationRange.max(), 1.0, 3.0);
    }
    return Utils::rangeMap(value, d_ptr->saturationRange.min(), 0, 0, 1.0);
}

auto Equalizer::brightnessRange() const -> Equalizer::EqualizerRange &
{
    return d_ptr->brightnessRange;
}

auto Equalizer::ffBrightness() const -> float
{
    return eqBrightness();
}

auto Equalizer::eqBrightness() const -> float
{
    // The value must be a float value in range -1.0 to 1.0. The default value is "0".
    return Utils::rangeMap(d_ptr->brightnessRange.value(),
                           d_ptr->brightnessRange.min(),
                           d_ptr->brightnessRange.max(),
                           -1.0,
                           1.0);
}

auto Equalizer::gammaRange() const -> Equalizer::EqualizerRange &
{
    return d_ptr->gammaRange;
}

auto Equalizer::ffGamma() const -> float
{
    return eqGamma();
}

auto Equalizer::eqGamma() const -> float
{
    // The value must be a float in range 0.1 to 10.0. The default value is "1".
    auto value = d_ptr->gammaRange.value();
    if (value == 0) {
        return 1.0;
    }
    if (value > 0) {
        return Utils::rangeMap(value, 0, d_ptr->gammaRange.max(), 1.0, 10.0);
    }
    return Utils::rangeMap(value, d_ptr->gammaRange.min(), 0, 0.1, 1.0);
}

auto Equalizer::hueRange() const -> Equalizer::EqualizerRange &
{
    return d_ptr->hueRange;
}

auto Equalizer::ffHue() const -> float
{
    return eqHue();
}

auto Equalizer::eqHue() const -> float
{
    // Set the hue shift in degrees to apply. Default is 0. Allowed range is from -180 to 180.
    return Utils::rangeMap(d_ptr->hueRange.value(),
                           d_ptr->hueRange.min(),
                           d_ptr->hueRange.max(),
                           -180.0,
                           180.0);
}

} // namespace MediaConfig
