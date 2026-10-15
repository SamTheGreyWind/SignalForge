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

## Phase 1: RTL-SDR Blog V4 receiver discovery preview

The **Receivers** dock now performs **real read-only device enumeration** using
the public \`librtlsdr\` driver API. It can list multiple RTL-SDR receivers,
including two RTL-SDR Blog V4 units, and reports their USB index, device name,
USB product and serial number. This **does not open a device**, stream I/Q,
tune frequencies, calculate real RF FFT samples or alter USB device settings.

**V4 driver compatibility is essential.** The Blog V4 uses an R828D tuner and
nonstandard RF routing that requires updated driver support. A generic/old DLL
may successfully enumerate your dongles **yet receive incorrect signals**.
Follow the manufacturer's guide:
https://www.rtl-sdr.com/v4/
RTL-SDR Blog's driver source/releases:
https://github.com/rtlsdrblog/rtl-sdr-blog

On Windows 11:

1. Connect either or both V4 receivers to the computer. If they already work
   in other SDR software, do not change their USB driver configuration.
2. Use the **x64** RTL-SDR Blog V4-compatible driver runtime. The preview
   intentionally **does not bundle** the third-party \`rtlsdr.dll\`. You may
   place \`rtlsdr.dll\` plus any required matching dependency DLLs beside
   \`SignalForge.exe\`, or click **Select V4 Driver DLL…** in Receivers and
   browse to the full path of a trustworthy V4-compatible DLL. Do not mix x86
   DLLs with the x64 SignalForge executable.
3. Open **Receivers → Scan USB Devices**. Two connected devices should appear
   as two rows, each with an index and a hardware USB serial number.
4. If no devices appear despite a successfully loaded driver, check the
   receiver's existing WinUSB/libusb setup, USB cable/hub, and whether another
   application has exclusive access. Always follow RTL-SDR Blog's guidance
   before changing operating-system USB drivers.
5. If SignalForge reports duplicate/missing serials, the driver cannot safely
   use the serial strings to restore persistent per-device assignments.
   Distinguish the physical dongles before building persistent profiles.
   Never automatically change EEPROM serials. Changes using \`rtl_eeprom\`
   are a separate, explicit hardware-maintenance operation.

On Linux, install a V4-compatible \`librtlsdr.so\` and set appropriate
udev permissions before using discovery. **Select V4 Driver DLL…** can also
load an explicitly chosen shared library file, despite the generic button
label.

**Security/licensing:** The driver path is user-configured and persisted;
the application loads native executable code from that location. Only use
a trusted driver build. The core's Qt-free discovery adapter uses runtime
dynamic library lookup so CI does not download or redistribute a proprietary
or unreviewed driver binary. Its C API and dual-device serial identification
are exercised through a **fake library fixture in CTest**, without any RF
hardware. Any actual driver bundling is subject to a further licensing audit.

**Next planned increment:** separate per-dongle receiver sessions, sample
rate and tuning controls, bounded I/Q streams, actual FFT/waterfall rendering,
and clean start/stop/disconnect logic; analog demodulation follows afterward.

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
5. Drag with the left mouse button to pan, and scroll to zoom. The map now permits zooming beyond the highest-resolution tile present (up to zoom 19); **this enlarges existing raster pixels and does not reveal additional roads or satellite detail**. The map displays **UPSCALED from Z#** when doing so. Import a higher-resolution MBTiles package to obtain genuine detail. **ADS-B** and **APRS** checkboxes control separate example marker overlays on the same map.
6. Move the mouse over the map: the bottom-left **Cursor** coordinate readout shows WGS84 latitude/longitude under the pointer; after moving off the canvas, it reverts to the **Center** coordinate. Click **Test Lat/Lon Pin…** to enter a test coordinate (decimal degrees, WGS84) and center the view on the resulting georeferenced marker.
7. Change or remove **imported** entries using the map selector and **Remove**. Removing an entry only changes SignalForge's list; it **does not delete the original file**. The bundled sample stays available while the preview is installed; selecting **(No offline base map)** hides it.

The base map's pixel positions and test markers use the same WGS84 latitude/longitude to Web Mercator projection. The bundled sample only has native tile zoom levels 3–5; zooming beyond that scale is useful for inspecting the viewer or positioning markers, but the visual basemap will become blocky. For street-level features, import a region-specific package with higher native zoom levels. The sample ADS-B/APRS markers are intentionally **fictional**, anchored near the imported map center so they can be evaluated visually. They are **not live GPS or RF data**. Outside the geographic tile coverage of the current package, the map displays an empty grid; no online fallback is attempted.

The bundled example uses generalized Natural Earth public-domain coastline/boundary data. Other map packages are user-supplied files with their own licenses. Users deploying this in commercial or governmental settings are responsible for reviewing map/imagery attribution, distribution rights, and retention/security policy. Avoid importing untrusted large files; future versions will add richer format support and map-package validation.

## Workspace-first GUI and spectrum zoom

The prototype no longer displays an immovable project name, slogan, or welcome panel in the middle of an otherwise empty workspace. The **window title** still identifies the running application as SignalForge. All operator data views are movable/resizable Qt dock widgets; close and reopen them with **Panels** or the workspace toolbar. The **View** menu can hide the status bar or workspace toolbar, and the workspace toolbar itself can now float or dock at another window edge. The menu bar remains available so panes can always be restored.

**Spectrum zoom is different from filter bandwidth:**

- **Ctrl + mouse wheel over the spectrum/waterfall** zooms the *visible frequency span* around the mouse position. The frequency under the pointer remains stationary.
- **Zoom +**, **Zoom -**, and **Reset** buttons operate about the view center. **Span** enters the displayed width directly in MHz (25 kHz to 12 MHz).
- **Normal mouse wheel** retains the prior tune/pan behavior.
- **Filter** and amber passband edges adjust only the visible VFO filter width; they do not change the spectrum's display span.
- Zoom resets the *simulated* historical waterfall lines because their columns referred to the previous frequency scale; new matching scanlines appear immediately. No RF samples are involved yet.

Qt application window controls and the menu bar are structural navigation, not operator panels. Full panel-instance management and automatic recreation of multiple top-level windows at startup remain later architectural improvements.

## Operator UI preview updates

- **Sharpened spectrum/waterfall:** the RF demo now draws jagged, bin-like synthetic power samples instead of rounded Gaussian-only hills. The waterfall uses a hard-pixel dark-blue → cyan → yellow → orange/red heat palette. These are simulated RF shapes, **not captured I/Q FFT samples**.
- **VFO filter band:** the amber center marker has a translucent passband; left/right amber edges change the filter width symmetrically when dragged. Mode presets (WFM, NFM, AM, USB, LSB, CW) provide **illustrative** bandwidths, and the **Filter** spinbox accepts a custom value. The filter is **visual only**, not yet connected to a demodulator. The 2.4 MHz screen span is separate from the VFO filter width. Very narrow bands are easier to adjust numerically.
- **Moveable/removable waterfall:** Spectrum / Waterfall is a normal dockable panel, not the permanent central widget. Drag to dock or float, resize, close/hide, and restore with **Panels → Spectrum / Waterfall** or the **Panels** toolbar.
- **New window picker:** use **File → New Window…** or **Ctrl+N** to choose **Empty Workspace** or a starting panel. Each additional top-level window is independently moveable and has its own set of panel views; turn additional views on from its **Panels** menu or toolbar. Secondary windows do not overwrite the primary window's last-session layout.
- **Window presets:** the current named workspace layouts can be saved/loaded from any window. Automatically recreating all secondary windows at startup and creating multiple copies of the *same panel type within one window* remain future enhancements.

Note: saved layouts from the earlier preview may not restore after the central-waterfall → dockable-waterfall migration. Use **Workspaces → Reset to Default**, then save a new named layout. The existing offline MBTiles viewer and cursor-coordinate behavior remain available.

## Workspaces and panels

- Drag panel title bars (including Spectrum / Waterfall) to dock, tab or float; floating panels can move to other monitors.
- Use **Panels** to show/hide a dock.
- Use **Workspaces > Save Layout As** to name a layout, and **Load Layout** to restore it.
- The last session's layout is saved on exit.
- Named layouts currently save panel docking, visibility and geometry, not device-task assignments.

## Known limitations

No SDR device access, **real RF** waterfall, audio, live aircraft/APRS locations, TX or decoders are present. An imported MBTiles basemap is genuine georeferenced map data, but all RF/position overlays remain synthetic. Frequency offset utility tests exist but are not wired to drivers. SignalForge-authored code is `GPL-3.0-or-later`; release packaging and third-party dependency/license notices need additional review before a general release.
