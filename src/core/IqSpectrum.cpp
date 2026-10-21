#include "core/IqSpectrum.h"
#include <algorithm>
#include <cmath>
#include <complex>
#include <numbers>
#include <utility>

namespace signalforge::core {

std::vector<float> fftPowerFromU8Iq(
    const unsigned char* iq, std::size_t byteCount) {
    if (!iq || byteCount < 2 * kRtlFftSize) return {};
    constexpr std::size_t n = kRtlFftSize;
    std::complex<double> bins[n];
    const double meanWindow = 0.5 * (n - 1);
    for (std::size_t i = 0; i < n; ++i) {
        const double w = 0.5 - 0.5 * std::cos(2.0 * std::numbers::pi *
                                              static_cast<double>(i) / (n - 1));
        bins[i] = std::complex<double>(
            (static_cast<double>(iq[2 * i]) - 127.5) / 128.0,
            (static_cast<double>(iq[2 * i + 1]) - 127.5) / 128.0) * w;
    }
    // In-place radix-2 FFT. No external FFT libraries or network dependency.
    for (std::size_t i = 1, j = 0; i < n; ++i) {
        std::size_t bit = n >> 1;
        while (j & bit) { j ^= bit; bit >>= 1; }
        j ^= bit;
        if (i < j) std::swap(bins[i], bins[j]);
    }
    for (std::size_t length = 2; length <= n; length <<= 1) {
        const double angle = -2.0 * std::numbers::pi / static_cast<double>(length);
        const std::complex<double> phaseStep(std::cos(angle), std::sin(angle));
        for (std::size_t start = 0; start < n; start += length) {
            std::complex<double> phase(1, 0);
            for (std::size_t j = 0; j < length / 2; ++j) {
                const auto even = bins[start + j];
                const auto odd = bins[start + j + length / 2] * phase;
                bins[start + j] = even + odd;
                bins[start + j + length / 2] = even - odd;
                phase *= phaseStep;
            }
        }
    }
    std::vector<float> spectrum(n);
    for (std::size_t k = 0; k < n; ++k) {
        const std::size_t shifted = (k + n / 2) % n;
        const double amplitude = std::abs(bins[shifted]) / meanWindow;
        const double dbfs = 20.0 * std::log10(std::max(1e-9, amplitude));
        // Preview visual level, not a calibrated power measurement.
        spectrum[k] = static_cast<float>(
            std::clamp((dbfs + 85.0) / 80.0, 0.0, 1.0));
    }
    return spectrum;
}

} // namespace signalforge::core
