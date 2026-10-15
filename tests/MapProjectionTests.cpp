#include "core/WebMercator.h"

#include <cmath>
#include <iostream>

namespace {
void assertClose(double actual, double expected, const char* what, int& failures) {
    if (std::abs(actual - expected) > 1e-6) {
        std::cerr << "FAIL " << what << ": " << actual
                  << " expected " << expected << '\n';
        ++failures;
    }
}
}

int main() {
    using namespace signalforge::core;
    int failures = 0;
    const auto origin = toWorldPixel(0, 0, 0);
    assertClose(origin.x, 128, "equator x", failures);
    assertClose(origin.y, 128, "equator y", failures);
    const auto quarter = toWorldPixel(0, 90, 2);
    assertClose(quarter.x, 768, "90E at zoom 2", failures);
    assertClose(quarter.y, 512, "equator at zoom 2", failures);
    assertClose(normalizeLongitude(190), -170, "longitude wrapping", failures);
    const auto north = toWorldPixel(90, 0, 1);
    const auto south = toWorldPixel(-90, 0, 1);
    assertClose(north.y, 0, "north clamp", failures);
    assertClose(south.y, 512, "south clamp", failures);
    const auto anti = toWorldPixel(0, 180, 2);
    assertClose(anti.x, 0, "antimeridian wrap", failures);
    return failures == 0 ? 0 : 1;
}
