#include "core/Frequency.h"

#include <limits>

namespace signalforge::core {
namespace {

using Hertz = std::int64_t;

std::optional<Hertz> checkedAdd(Hertz left, Hertz right) noexcept {
    constexpr Hertz max = std::numeric_limits<Hertz>::max();
    constexpr Hertz min = std::numeric_limits<Hertz>::min();
    if ((right > 0 && left > max - right) ||
        (right < 0 && left < min - right)) {
        return std::nullopt;
    }
    return left + right;
}

std::optional<Hertz> checkedSubtract(Hertz left, Hertz right) noexcept {
    constexpr Hertz max = std::numeric_limits<Hertz>::max();
    constexpr Hertz min = std::numeric_limits<Hertz>::min();
    if ((right > 0 && left < min + right) ||
        (right < 0 && left > max + right)) {
        return std::nullopt;
    }
    return left - right;
}

}  // namespace

std::optional<Hertz> toHardwareHz(
    Hertz displayedRfHz,
    Hertz converterOffsetHz) noexcept {
    if (displayedRfHz < 0) {
        return std::nullopt;
    }
    auto result = checkedSubtract(displayedRfHz, converterOffsetHz);
    return result && *result >= 0 ? result : std::nullopt;
}

std::optional<Hertz> toDisplayedRfHz(
    Hertz hardwareHz,
    Hertz converterOffsetHz) noexcept {
    if (hardwareHz < 0) {
        return std::nullopt;
    }
    auto result = checkedAdd(hardwareHz, converterOffsetHz);
    return result && *result >= 0 ? result : std::nullopt;
}

}  // namespace signalforge::core
