#include "core/IqSpectrum.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>

int main() {
    constexpr std::size_t n = signalforge::core::kRtlFftSize;
    std::vector<unsigned char> iq(n * 2);
    for (std::size_t i = 0; i < n; ++i) {
        // Positive frequency +Fs/8 is FFT-shifted to bin 5/8 of spectrum.
        const double p = 2.0 * 3.14159265358979323846 * i / 8;
        iq[2*i] = static_cast<unsigned char>(128 + 90 * std::cos(p));
        iq[2*i+1] = static_cast<unsigned char>(128 + 90 * std::sin(p));
    }
    const auto spectrum = signalforge::core::fftPowerFromU8Iq(iq.data(), iq.size());
    if (spectrum.size() != n) return 1;
    const auto peak = static_cast<std::size_t>(
        std::max_element(spectrum.begin(), spectrum.end()) - spectrum.begin());
    if (peak != n * 5 / 8 || spectrum[peak] < 0.7f ||
        spectrum[n / 4] > 0.4f) {
        std::cerr << "Incorrect shifted complex IQ peak at " << peak
                  << ": " << spectrum[peak] << '\n';
        return 1;
    }
    if (!signalforge::core::fftPowerFromU8Iq(iq.data(), 40).empty() ||
        !signalforge::core::fftPowerFromU8Iq(nullptr, iq.size()).empty())
        return 1;
    std::cout << "IQ FFT peak, frequency shift, and input guards passed.\n";
    return 0;
}
