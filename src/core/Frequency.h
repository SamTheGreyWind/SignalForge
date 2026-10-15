#pragma once

#include <cstdint>
#include <optional>

namespace signalforge::core {

// Converter offset convention:
// displayed RF frequency = hardware tune frequency + converter offset.
// 7 MHz RF -> 132 MHz hardware (upconverter) uses -125 MHz offset.
// 10 GHz RF -> 1 GHz hardware (downconverter) uses +9 GHz offset.
// Reject negative frequencies and signed integer overflow.
[[nodiscard]] std::optional<std::int64_t> toHardwareHz(
    std::int64_t displayedRfHz,
    std::int64_t converterOffsetHz) noexcept;

[[nodiscard]] std::optional<std::int64_t> toDisplayedRfHz(
    std::int64_t hardwareHz,
    std::int64_t converterOffsetHz) noexcept;

}  // namespace signalforge::core
