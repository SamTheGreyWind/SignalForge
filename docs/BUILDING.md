# Building SignalForge (early application shell)

## Windows preview without development tools

For pull requests that include the GitHub Actions workflow, you can download a packaged Windows preview:

1. Open the pull request's **Checks** tab, or go to the repository's **Actions** tab.
2. Open the most recent successful **Build and Test** workflow run for the pull request.
3. Scroll to **Artifacts** and download **SignalForge-Windows-Preview** (GitHub may require sign-in).
4. Extract the downloaded ZIP to a folder; preserve its Qt DLLs and `platforms` subfolder.
5. Double-click `SignalForge.exe`.

This preview is **not an installer** and is not a signed release. Windows SmartScreen may warn about unfamiliar unsigned applications. Only run artifacts from workflow runs you trust. Its panels are currently placeholders, and no SDR hardware functions are implemented.

The current build provides a **Qt 6 desktop shell** with dockable/floating panels, named layouts and a small independently testable C++ core. **No hardware SDR reception, spectrum plotting or protocol decoding is implemented yet.**

## Requirements

- CMake 3.21 or newer.
- A compiler with C++20 support (MSVC 2022, GCC or Clang).
- Qt 6.4 or newer with the **Widgets** module for the desktop application.
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
sudo apt-get install -y build-essential cmake qt6-base-dev
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

## Workspaces and panels

- Drag panel title bars to dock, tab or float; floating panels can move to other monitors.
- Use **Panels** to show/hide a dock.
- Use **Workspaces > Save Layout As** to name a layout, and **Load Layout** to restore it.
- The last session's layout is saved on exit.
- Named layouts currently save panel docking, visibility and geometry, not device-task assignments.

## Known limitations

No SDR device access, live waterfall, audio, maps, TX or decoders are present. Panels are deliberately marked as placeholders. Frequency offset utility tests exist but are not wired to drivers. Licensing and release packaging are pending.
