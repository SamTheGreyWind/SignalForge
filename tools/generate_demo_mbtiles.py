#!/usr/bin/env python3
"""Create a tiny georeferenced raster MBTiles basemap for offline UI previews.

The simplified North America coastline/country polygons are derived from
Natural Earth low-resolution country boundaries (public domain):
https://www.naturalearthdata.com/about/terms-of-use/

This is a coarse demonstrator, NOT a navigational/GIS analysis product.
No downloads, APIs, pip packages, or internet access are required.
"""
from __future__ import annotations

import argparse
import base64
import json
import math
from pathlib import Path
import sqlite3
import struct
import zlib

SIZE = 256
MIN_ZOOM = 3
MAX_ZOOM = 5
LON_WEST, LAT_SOUTH, LON_EAST, LAT_NORTH = -140, 15, -55, 60
OCEAN = (16, 45, 68)
LAND = (49, 101, 83)
GRID = (35, 78, 105)
COAST = (141, 179, 153)

# Public-domain Natural Earth low-res geographic outlines, simplified to ~0.4
# degrees, clipped to the sample region. zlib/json/base64 for compactness.
# The original .shp is not needed at build time.
GEOGRAPHY = (
"eNptV22u5ToI21BfFfIFWcto9r+NweCk7b3vz5EsCDHBQM+fP3/+k9rucvV1l78X0LjnNcrdE+ndgWyjdY16t0TrFiDbqF2j3TNQK/e4Rqeno3KNca9Ecus15vZsuMF4Xxs4txizKZDtc07xmiV5ru40X0CP32p3BYgQq4KV3hKggL4mKRvJKUG4jXRzIMgkLLpwRu4awPA2NZm7xZCiBphg6j8BFNEso/mZcegonnOznuuVwoznYxGm4h7LS6e/4uMmeC09wHBpEB1xaU9unoqdFBwoWAedGaUoDD2zupmceNp9JtFpfqZbBpgDlnXoVFj0sTBTtwwESDdUoQ+6CSys/4Q0OlNAqR/LcDq95SM6NwGYBB6t09Lhxsr5UwmAENhzaQCW3p+3AnRmGnSCgXYv8AETZ3ipxhs8wN1qEnWFOAPJe6yBQc0AoZ19xsE8RM1whi+6BBa+20KB94s6MLw12++0psOoZIcYs8YzPY1ATiXHyFPtAUoLVOYgAxgCkNFYWdZBLYUulAF4aXBg+/bkLq1CT43Dw20VUqu7tSts77bHuYzjuY1zo1TLBMpG77ETai577DRUYvH+J0pETQ0u6qmmp55m2ccet32qQ7n6nJpZgYn0XiAs5VEhzjCdmvHLryp+6/uu/EcTX7V8dESF7emkR3sfVX71+lHyR+Nv9X/64tsxP3qpoP9yiJXkpgQGBo2ggFte2kCnkJuAwQYdbqxucGuc3Yrx73N+EAgsncDdKGPF3GqspYMGN+GALQD1segJrZevl3xRgUUytCGfOtnavvkuF1ylmwIsVsEBpWXg5iCjBR2uTMNUb+xEBx1u9WypyuQW5pbvUyMYuCcB1lyl5KQI0J4HpcJxr8fSkkTZaAFllxQsJM8wPQVRHKVNQMQbVxOBiaPsQ8G6an33YUEyXCpSQ/LfPhy7f3sujLptsRcYJZStu2NPo2Tj9PJa6z9X/vdz4Pvh8P2oKK91fj4/7DWjNlNPR852xn0GlLaHS3BTXCgcIAnsbI0BkLLH14OwwRVDT9aRPQPseP0Sjl/FJNiOvm8jxGQbxU17WfFMhkDHb3WoJtjfFgbwrKtwy1Oampp0rEfVeYrb+HHbp9bpBQ2N8osuT7UzhDos86GEM1ndH+r6Ku+jyq9if2hbsnV+N0j2EZNZcXmlBRND9ncgpCt74UqCxY/CdlXh+o5okmwNScqzsQvKYAQDoDGAg3lC6wMwxIWTdmFuH9DTLefChBA4aZH4oywpaIAtEqDg2veDFWTLFq+o6n4wwfM1frbi2e01DKIInFBSYkS17Vl+x+y7XJB5nfuGkQNMd0z3pH5hG2c6viQQkrAoBYedRbZ8FcMyOgC1lD17karw6idAxNuv/D/v/6Myr5q9q2n4H/MC9bnWEugvds+lmVPYOBsMnzK7gmbvND7Bz5n8Bhmpts4vYTvxJvpVuMbie1c4r54zu1/t2HS+TinHSz3NK+eTyUvQIIByHCvbVaOo5dXyZ65Fn3DjxXdHlfPNW08LGr65Kv8vxR8ud9vLlJf+/fsPzxwtPA=="
)


def mercator(lon: float, lat: float, zoom: int) -> tuple[float, float]:
    lat = max(-85.0511287798066, min(85.0511287798066, lat))
    x = (lon + 180.0) / 360.0 * (SIZE * (1 << zoom))
    r = math.radians(lat)
    y = (1.0 - math.asinh(math.tan(r)) / math.pi) / 2.0 * (
        SIZE * (1 << zoom)
    )
    return x, y


def row_spans(points: list[tuple[float, float]], row: int):
    scan_y = row + 0.5
    intersections = []
    for a, b in zip(points, points[1:]):
        if (a[1] <= scan_y < b[1]) or (b[1] <= scan_y < a[1]):
            intersections.append(a[0] +
                                 (scan_y - a[1]) * (b[0] - a[0]) /
                                 (b[1] - a[1]))
    intersections.sort()
    for i in range(0, len(intersections) - 1, 2):
        left = max(0, math.ceil(intersections[i] - .5))
        right = min(SIZE, math.ceil(intersections[i + 1] - .5))
        if right > left:
            yield left, right


def png_chunk(tag: bytes, data: bytes) -> bytes:
    body = tag + data
    return (struct.pack("!I", len(data)) + body +
            struct.pack("!I", zlib.crc32(body) & 0xFFFFFFFF))


def make_png(rows: list[bytearray]) -> bytes:
    raw = b"".join(b"\x00" + bytes(row) for row in rows)
    return (b"\x89PNG\r\n\x1a\n" +
            png_chunk(b"IHDR", struct.pack("!2I5B",
                      SIZE, SIZE, 8, 2, 0, 0, 0)) +
            png_chunk(b"IDAT", zlib.compress(raw, 6)) +
            png_chunk(b"IEND", b""))


def draw_tile(z: int, tx: int, ty: int, geography: list) -> bytes:
    rows = [bytearray(OCEAN * SIZE) for _ in range(SIZE)]
    ox, oy = tx * SIZE, ty * SIZE
    for poly in geography:
        points = [(mercator(lon, lat, z)[0] - ox,
                   mercator(lon, lat, z)[1] - oy)
                  for lon, lat in poly]
        low = max(0, math.floor(min(y for x, y in points)))
        high = min(SIZE, math.ceil(max(y for x, y in points)))
        for y in range(low, high):
            row = rows[y]
            for a, b in row_spans(points, y):
                row[a * 3:b * 3] = bytes(LAND) * (b - a)

    # Geographic graticule in the same WGS84/Web Mercator projection.
    for lon in range(-140, -49, 10):
        x = round(mercator(lon, 0, z)[0] - ox)
        if 0 <= x < SIZE:
            for row in rows:
                row[x * 3:x * 3 + 3] = bytes(GRID)
    for lat in range(20, 61, 10):
        y = round(mercator(0, lat, z)[1] - oy)
        if 0 <= y < SIZE:
            rows[y][:] = bytes(GRID) * SIZE

    # A contrasting (coarse) coastline/boundary outline.
    for poly in geography:
        previous = None
        for lon, lat in poly:
            x, y = mercator(lon, lat, z)
            current = (round(x - ox), round(y - oy))
            if previous is not None:
                dx = abs(current[0] - previous[0])
                dy = abs(current[1] - previous[1])
                n = max(dx, dy, 1)
                for i in range(n + 1):
                    px = round(previous[0] + (current[0] - previous[0]) * i / n)
                    py = round(previous[1] + (current[1] - previous[1]) * i / n)
                    if 0 <= px < SIZE and 0 <= py < SIZE:
                        rows[py][px * 3:px * 3 + 3] = bytes(COAST)
            previous = current

    return make_png(rows)


def generate(destination: Path) -> None:
    geo = json.loads(zlib.decompress(base64.b64decode(GEOGRAPHY)))
    destination.parent.mkdir(parents=True, exist_ok=True)
    if destination.exists():
        destination.unlink()
    with sqlite3.connect(destination) as connection:
        connection.executescript("""
            CREATE TABLE metadata (name TEXT, value TEXT);
            CREATE TABLE tiles (
              zoom_level INTEGER NOT NULL,
              tile_column INTEGER NOT NULL,
              tile_row INTEGER NOT NULL,
              tile_data BLOB NOT NULL,
              UNIQUE (zoom_level, tile_column, tile_row)
            );
        """)
        meta = {
            "name": "SignalForge Natural Earth Offline Demo (coarse)",
            "type": "baselayer",
            "format": "png",
            "minzoom": str(MIN_ZOOM),
            "maxzoom": str(MAX_ZOOM),
            "center": "-95,37,4",
            "bounds": "-140,15,-55,60",
            "attribution": "Natural Earth public domain, simplified for demo",
            "description": "Offline-only geographic preview; not for navigation."
        }
        connection.executemany("INSERT INTO metadata VALUES (?, ?)", meta.items())

        count = 0
        for zoom in range(MIN_ZOOM, MAX_ZOOM + 1):
            west, north = mercator(LON_WEST, LAT_NORTH, zoom)
            east, south = mercator(LON_EAST, LAT_SOUTH, zoom)
            for y in range(int(north // SIZE), int(south // SIZE) + 1):
                for x in range(int(west // SIZE), int(east // SIZE) + 1):
                    image = draw_tile(zoom, x, y, geo)
                    # MBTiles default tile_row uses TMS (south-origin).
                    tms_row = (1 << zoom) - y - 1
                    connection.execute("INSERT INTO tiles VALUES (?,?,?,?)",
                                       (zoom, x, tms_row, image))
                    count += 1
        connection.commit()
    with sqlite3.connect(destination) as connection:
        assert connection.execute("PRAGMA quick_check").fetchone()[0] == "ok"
        assert connection.execute("SELECT COUNT(*) FROM tiles").fetchone()[0] == count
        assert connection.execute(
            "SELECT substr(tile_data,1,8) FROM tiles LIMIT 1"
        ).fetchone()[0] == b"\x89PNG\r\n\x1a\n"
    print(f"Created {destination}: {count} georeferenced raster tiles "
          f"({destination.stat().st_size} bytes)")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("destination", type=Path,
                        help="Output raster .mbtiles path")
    generate(parser.parse_args().destination)
