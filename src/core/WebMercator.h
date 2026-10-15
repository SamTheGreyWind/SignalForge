#pragma once

namespace signalforge::core {

// EPSG:3857 / slippy-map world pixel coordinates, with 256 px tiles.
// longitude is degrees east, latitude degrees north.
// Coordinates are clamped to the representable Web Mercator bounds.
struct MapPixel {
    double x = 0;
    double y = 0;
};

struct GeoCoordinate {
    double latitude = 0;
    double longitude = 0;
};

inline constexpr double kMercatorMaxLatitude = 85.0511287798066;
inline constexpr int kMapTileSize = 256;

[[nodiscard]] MapPixel toWorldPixel(double latitude,
                                    double longitude,
                                    int zoom) noexcept;
[[nodiscard]] GeoCoordinate fromWorldPixel(MapPixel pixel, int zoom) noexcept;
[[nodiscard]] double worldSize(int zoom) noexcept;
[[nodiscard]] double normalizeLongitude(double longitude) noexcept;

}  // namespace signalforge::core
