#include "core/Frequency.h"

#include <cstdint>
#include <iostream>
#include <limits>

namespace {

void check(bool condition, const char* description, int& failures) {
    if (!condition) {
        std::cerr << "FAIL: " << description << '\n';
        ++failures;
    }
}

}  // namespace

int main() {
    using signalforge::core::toDisplayedRfHz;
    using signalforge::core::toHardwareHz;

    int failures = 0;
    check(toHardwareHz(145'500'000, 0) == 145'500'000,
          "No converter changes nothing", failures);
    check(toHardwareHz(7'000'000, -125'000'000) == 132'000'000,
          "125 MHz upconverter tunes higher", failures);
    check(toDisplayedRfHz(132'000'000, -125'000'000) == 7'000'000,
          "Upconverter RF display is correct", failures);
    check(toHardwareHz(10'000'000'000LL, 9'000'000'000LL) == 1'000'000'000LL,
          "Downconverter tunes lower", failures);
    check(toDisplayedRfHz(1'000'000'000LL, 9'000'000'000LL) == 10'000'000'000LL,
          "Downconverter RF display is correct", failures);
    check(!toHardwareHz(-1, 0), "Reject negative RF", failures);
    check(!toDisplayedRfHz(-1, 0), "Reject negative hardware tune", failures);
    check(!toHardwareHz(1, 2), "Reject negative hardware result", failures);
    check(!toDisplayedRfHz(1, -2), "Reject negative RF result", failures);
    check(!toHardwareHz(std::numeric_limits<std::int64_t>::max(), -1),
          "Reject subtraction overflow", failures);
    check(!toDisplayedRfHz(std::numeric_limits<std::int64_t>::max(), 1),
          "Reject addition overflow", failures);
    check(toHardwareHz(0, 0) == 0, "Zero is representable", failures);

    if (failures == 0) {
        std::cout << "All frequency converter tests passed.\n";
    }
    return failures == 0 ? 0 : 1;
}
