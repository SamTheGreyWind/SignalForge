# Building SignalForge (early application shell)

## Windows preview without development tools

For pull requests that include the GitHub Actions workflow, you can download a packaged Windows preview:

1. Open the pull request's **Checks** tab, or go to the repository's **Actions** tab.
2. Open the most recent successful **Build and Test** workflow run for the pull request.
3. Scroll to **Artifacts** and download **SignalForge-Windows-Preview** (GitHub may require sign-in).
4. Extract the downloaded ZIP to a folder; keep its Qt DLLs, `platforms`, `sqldrivers`, and **`maps`** subfolders together.
5. Double-click `SignalForge.exe`.

This preview is **not an installer** and is not a signed release. Windows SmartScreen may warn about unfamiliar unsigned applications. Only run artifacts from workflow runs you trust. It includes synthetic spectrum/waterfall activity, fictional monitoring tables, a **bundled offline Natural Earth MBTiles sample**, and the ability to import your own maps. No SDR hardware functions are implemented.

The current build provides a **Qt 6 desktop shell** with dockable/floating panels, named layouts, a moving **synthetic** spectrum/waterfall, sample monitoring tables, a genuine **offline raster MBTiles map reader**, and a small independently testable C++ core. **No hardware SDR reception or protocol decoding is implemented yet.**

## Requirements

- CMake 3.21 or newer.
- A compiler with C++20 support (MSVC 2022, GCC or Clang).
- Qt 6.4 or newer with the **Widgets and Sql** modules (including the QSQLITE plugin) for the desktop application.
- Windows 11 is the first target; Linux is a supported build target. macOS has not been validated.

## Windows 11 (Visual Studio)

1. Install Visual Studio 2022 or newer with the **Desktop development with C++** workload and CMake tools.
2. Install an MSVC 64-bit Qt 6 Widgets kit through the official Qt installer, matching the installed compiler.
3. Open a Developer PowerShell with MSVC tools available.
4. Configure, build and test (replace the example Qt path with your actual installation):

~~~powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:/Qt/6.8.3/msvc2022_64" -DBUILD_TESTING=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
~~~

The executable is typically `build/Release/SignalForge.exe` with the Visual Studio generator. Deploy required Qt runtime libraries/plugins using Qt's `windeployqt` tool for machines without a Qt development environment.

## Linux (Ubuntu/Debian example)

~~~bash
sudo apt-get update
sudo apt-get install -y build-essential cmake qt6-base-dev libqt6sql6-sqlite
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/SignalForge
~~~

The GUI requires a graphical desktop session. Wayland window-placement restrictions may affect floating panel restoration.

## Headless core tests (Qt not required)

~~~bash
cmake -S . -B build-core -DSIGNALFORGE_BUILD_GUI=OFF -DBUILD_TESTING=ON
cmake --build build-core --parallel
ctest --test-dir build-core --output-on-failure
~~~

This proves the core can build without Qt.

## Exploring the demonstration interface

- The waterfall animates generated RF-like signals. **No radio is connected or sampled.**
- Click/drag across the spectrum or waterfall to move the VFO marker.
- Use the mouse wheel over the spectrum to tune in 25 kHz steps and pan the simulated frequency span.
- Toggle **Animate demo** to pause/resume updates.
- Scanner, P25, receivers, recordings, ADS-B and APRS contain **fictional preview data** only.
- ADS-B and APRS panels can display imported offline georeferenced raster maps with independent synthetic marker overlays; markers do not represent real received locations.
- Choose **File > New Window** (Ctrl+N) to open a separate top-level workspace on any monitor. Closing a secondary window does not stop the others; only the main window auto-saves the last-session layout.

## Offline GIS map support (preview)

**No internet connection or tile service is used.** The Windows preview now includes a small, coarse public-domain Natural Earth raster MBTiles map at `maps/SignalForge_Offline_GIS_Sample.mbtiles`. It auto-opens when no previous map selection is saved; you can always select another file. The generator is in `tools/generate_demo_mbtiles.py` and uses only Python's standard library. This preview supports **raster MBTiles** (SQLite tiles with PNG or JPEG imagery, including suitably licensed satellite imagery). Vector MBTiles / PBF, GeoTIFF, GeoPackage, Shapefile, WMS/WMTS and elevation rasters are **not implemented yet**.

1. Try the **bundled Natural Earth sample** first; it covers parts of North America at zoom levels 3–5 and is a low-resolution geographic orientation map, **not suitable for navigation**. For useful street-level detail, create or obtain a georeferenced **raster MBTiles** package covering your area, using a GIS tool such as QGIS and data you have permission to store offline. The map should use standard Web Mercator XYZ/MBTiles tiling; MBTiles default TMS tile-row convention is handled automatically.
2. Put any **additional** map file somewhere accessible, including an external SSD or USB drive.
3. Open **ADS-B Map** or **APRS Map**. Select the bundled sample from the map list, or click **Add MBTiles…** for your own file.
4. Select the file. Your imported map loads **in place** without copying it into SignalForge. If you disconnect the drive, the file won't be available until it is reconnected.
5. Drag with the left mouse button to pan, and scroll to zoom. **ADS-B** and **APRS** checkboxes control separate example marker overlays on the same map.
6. Click **Test Lat/Lon Pin…** to enter a test coordinate (decimal degrees, WGS84) and center the view on the resulting georeferenced marker.
7. Change or remove **imported** entries using the map selector and **Remove**. Removing an entry only changes SignalForge's list; it **does not delete the original file**. The bundled sample stays available while the preview is installed; selecting **(No offline base map)** hides it.

The base map's pixel positions and test markers use the same WGS84 latitude/longitude to Web Mercator projection. The sample ADS-B/APRS markers are intentionally **fictional**, anchored near the imported map center so they can be evaluated visually. They are **not live GPS or RF data**. Outside the geographic tile coverage of the current package, the map displays an empty grid; no online fallback is attempted.

The bundled example uses generalized Natural Earth public-domain coastline/boundary data. Other map packages are user-supplied files with their own licenses. Users deploying this in commercial or governmental settings are responsible for reviewing map/imagery attribution, distribution rights, and retention/security policy. Avoid importing untrusted large files; future versions will add richer format support and map-package validation.

## Workspaces and panels

- Drag panel title bars to dock, tab or float; floating panels can move to other monitors.
- Use **Panels** to show/hide a dock.
- Use **Workspaces > Save Layout As** to name a layout, and **Load Layout** to restore it.
- The last session's layout is saved on exit.
- Named layouts currently save panel docking, visibility and geometry, not device-task assignments.

## Known limitations

No SDR device access, **real RF** waterfall, audio, live aircraft/APRS locations, TX or decoders are present. An imported MBTiles basemap is genuine georeferenced map data, but all RF/position overlays remain synthetic. Frequency offset utility tests exist but are not wired to drivers. Licensing and release packaging are pending.
