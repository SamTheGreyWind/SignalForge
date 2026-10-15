# SignalForge

**The Open-Source Radio Multitool**

> Forged by Operators. Built for Everyone.

SignalForge is a community-oriented, modular software-defined radio (SDR) workstation. Its goal is to combine interactive spectrum exploration, multi-receiver monitoring, protocol decoding, recording and replay, alerts, and mapping in a customizable desktop environment.

**Status:** Early development. A minimal Qt desktop shell, basic core utilities and tests are available; SDR reception, decoding and mapping remain planned features.

## What we are building

- Interactive spectrum/waterfall and visual VFO with configurable band-plan overlays.
- Analog and digital demodulation, scanning, and trunked radio monitoring (including P25).
- Searchable call recordings, configurable retention, transcription, NOAA SAME alerts, and OpenMHz integration.
- Independent ADS-B and APRS mapping with optional combined map layers.
- Multi-SDR receiver coordination, satellite weather decoding, and compatible SDR transmission.
- Expandable, dockable, detachable panels and multi-monitor workspace layouts.
- A shared platform with **separate module data stores by default**, plus optional combined search/views.

See [Product requirements](docs/PRODUCT_REQUIREMENTS.md) for the full feature inventory and acceptance targets.

## Platform and technical direction

| Area | Initial direction |
| --- | --- |
| Primary desktop target | Windows 11 |
| Cross-platform goal | Linux; macOS considered later |
| Language | C++20 |
| UI | Qt 6 Widgets, detachable/dockable panels |
| Builds | CMake |
| SDR integration | Hardware abstraction (SoapySDR under evaluation), vendor-specific adapters where needed |
| Storage | Separate SQLite-backed stores per feature domain, with optional federated views |

These are initial architecture decisions; dependencies, licenses and compatibility must be validated before being adopted.

## Building the early desktop shell

See [Building SignalForge](docs/BUILDING.md) for Windows/Linux instructions. The current UI demonstrates detachable panels, independent windows, a synthetic spectrum/VFO, saved workspaces, a bundled offline raster map sample, and support for locally imported raster MBTiles maps without RF reception or live-position decoding.

## Project documents

- [Product requirements](docs/PRODUCT_REQUIREMENTS.md)
- [Architecture and technical principles](docs/ARCHITECTURE.md)
- [Development roadmap](docs/ROADMAP.md)
- [Contributing](CONTRIBUTING.md)

## Open-source commitment

SignalForge-authored software is licensed under the **GNU General Public License, version 3 or any later version** (`GPL-3.0-or-later`). See [LICENSE](LICENSE) for the complete GPLv3 terms. You may use, modify, and redistribute the software under those terms, including their obligations when distributing modified versions. Third-party components and geographic datasets retain their own licenses; the bundled Natural Earth sample uses public-domain geographic data. Compatibility and distribution notices for dependencies such as Qt, radio codecs, and other libraries must be audited before broader releases.

## Responsible use

SignalForge is intended for lawful reception, analysis and transmission. Authorized transmission, local regulations, private communications, third-party services and content rights remain the operator's responsibility. Encrypted traffic is not treated as decodable without legitimate authorization. Transcripts and automatic alerts may contain errors and should not be considered official public-safety records.

## Current milestone

Document the vision and technical boundaries first, then deliver a functioning Windows SDR receiver and incrementally add independently testable capabilities. See the roadmap for stages and quality gates.
