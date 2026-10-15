#include "core/WebMercator.h"

#include <algorithm>
#include <cmath>

namespace signalforge::core {
namespace {
constexpr double kPi = 3.14159265358979323846;
}

double worldSize(int zoom) noexcept {
    return static_cast<double>(kMapTileSize) * std::ldexp(1.0, std::clamp(zoom, 0, 22));
}

double normalizeLongitude(double longitude) noexcept {
    if (!std::isfinite(longitude)) return 0.0;
    double wrapped = std::fmod(longitude + 180.0, 360.0);
    if (wrapped < 0) wrapped += 360.0;
    return wrapped - 180.0;
}

MapPixel toWorldPixel(double latitude, double longitude, int zoom) noexcept {
    if (!std::isfinite(latitude)) latitude = 0.0;
    latitude = std::clamp(latitude, -kMercatorMaxLatitude, kMercatorMaxLatitude);
    const double lat = latitude * kPi / 180.0;
    const double size = worldSize(zoom);
    const double x = (normalizeLongitude(longitude) + 180.0) / 360.0 * size;
    const double y = (1.0 - std::asinh(std::tan(lat)) / kPi) * 0.5 * size;
    return { x, std::clamp(y, 0.0, size) };
}

}  // namespace signalforge::core
