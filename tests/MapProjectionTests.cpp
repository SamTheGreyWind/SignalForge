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
    const auto atlanta = toWorldPixel(33.749, -84.388, 12);
    const auto restored = fromWorldPixel(atlanta, 12);
    assertClose(restored.latitude, 33.749, "latitude round-trip", failures);
    assertClose(restored.longitude, -84.388, "longitude round-trip", failures);
    const auto cursor = fromWorldPixel({atlanta.x + 256.0,
                                       atlanta.y + 256.0}, 12);
    if (!(cursor.latitude < restored.latitude &&
          cursor.longitude > restored.longitude)) {
        std::cerr << "FAIL cursor southeast direction\n";
        ++failures;
    }
    const auto wrapped = fromWorldPixel({worldSize(2) + 256, 512}, 2);
    assertClose(wrapped.longitude, -90, "wrapped inverse longitude", failures);
    const auto anti = toWorldPixel(0, 180, 2);
    assertClose(anti.x, 0, "antimeridian wrap", failures);
    return failures == 0 ? 0 : 1;
}
