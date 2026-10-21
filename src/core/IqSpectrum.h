#pragma once
#include <complex>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace signalforge::core {

// Read-only FFT power mapping of interleaved unsigned 8-bit RTL I/Q.
// Result is FFT-shifted: bin zero = -Fs/2; middle bin = DC.
inline constexpr std::size_t kRtlFftSize = 1024;
[[nodiscard]] std::vector<float> fftPowerFromU8Iq(
    const unsigned char* interleavedIq, std::size_t byteCount);

} // namespace signalforge::core
