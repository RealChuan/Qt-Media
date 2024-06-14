#ifndef EQUALIZER_H
#define EQUALIZER_H

#include "mediaconfig_global.hpp"

#include <utils/range.hpp>

#include <QScopedPointer>

namespace MediaConfig {

class MEDIACONFIG_EXPORT Equalizer
{
public:
    using EqualizerRange = Utils::Range<float>;

    Equalizer();
    Equalizer(const Equalizer &other);
    auto operator=(const Equalizer &other) -> Equalizer &;
    ~Equalizer();

    auto operator==(const Equalizer &other) const -> bool;
    auto operator!=(const Equalizer &other) const -> bool;

    [[nodiscard]] auto contrastRange() const -> EqualizerRange &;
    [[nodiscard]] auto ffContrast() const -> float;
    [[nodiscard]] auto eqContrast() const -> float;

    [[nodiscard]] auto saturationRange() const -> EqualizerRange &;
    [[nodiscard]] auto ffSaturation() const -> float;
    [[nodiscard]] auto eqSaturation() const -> float;

    [[nodiscard]] auto brightnessRange() const -> EqualizerRange &;
    [[nodiscard]] auto ffBrightness() const -> float;
    [[nodiscard]] auto eqBrightness() const -> float;

    [[nodiscard]] auto gammaRange() const -> EqualizerRange &;
    [[nodiscard]] auto ffGamma() const -> float;
    [[nodiscard]] auto eqGamma() const -> float;

    [[nodiscard]] auto hueRange() const -> EqualizerRange &;
    [[nodiscard]] auto ffHue() const -> float;
    [[nodiscard]] auto eqHue() const -> float;

private:
    class EqualizerPrivate;
    QScopedPointer<EqualizerPrivate> d_ptr;
};

} // namespace MediaConfig

#endif // EQUALIZER_H
