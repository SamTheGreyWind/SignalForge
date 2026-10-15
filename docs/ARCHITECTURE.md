# SignalForge — Architecture (Draft)

This document records initial choices for the modular Windows-first, Linux-compatible SDR workstation. It is not a finished implementation specification.

## Guiding decisions

- **C++20 core, Qt 6 Widgets UI, CMake builds.** Compile native binaries per operating system; avoid assuming a Windows executable will run directly on Linux.
- **Engine is independent of GUI.** Headless mode or a background service is a later deployment option, enabled by separating process/UI boundaries early.
- **Receiver resources are shared intentionally, not implicitly.** A device manager owns physical SDR handles. Channelizers distribute IQ streams within each receiver's usable instantaneous bandwidth.
- **Modules own their processing and data.** P25, ADS-B, APRS, SAME, satellites, scanner, CW, TV and TX are independent capabilities behind defined interfaces.
- **Panels are views, not receivers.** Multiple panels can observe a single decoder without duplicating processing or retuning hardware.
- **Separate persistence by default.** Domain-specific SQLite databases and external media files (audio/IQ/images) live behind storage adapters. A combined read-only query interface can federate events when enabled.
- **Maps are composable, offline-first views.** ADS-B and APRS have separate source data and layers. Map panels can independently choose layers and locally available map packages. Initial raster MBTiles and WGS84/Web Mercator support is a preview; future GIS drivers should support users' own data and georeferenced mapping in commercial/government deployments.
- **No mandatory network requirement.** Remote uploads and email are opt-in integrations, not prerequisites to receive and decode.
- **Explicit legal and safety constraints.** TX must require manual enabling, capability and frequency checks; encrypted traffic is not bypassed.

## Logical component model

```text
Windows / Linux Qt application
  ├─ Workspace manager (profiles, dock/tab/float, multi-monitor state)
  ├─ Panels (waterfall, VFO, scanner, calls, maps, playback, diagnostics)
  └─ UI-facing controller / observable event subscriptions
                │
             typed APIs
                │
SDR engine (GUI-independent)
  ├─ Device manager & capability adapters (SoapySDR candidate, vendor SDKs)
  ├─ Sample streaming / bounded buffering / clock and metadata
  ├─ FFT, spectrum & waterfall data pipeline
  ├─ Channelizer, tuning and converter-offset logic
  ├─ Demodulators, scanner and decode plug-in interfaces
  ├─ Scheduler / priority allocation for devices and channels
  ├─ Recorder & media playback service
  ├─ Notification rules / transcription queue / upload workers
  └─ Event publication (versioned events, per-module topics)
                │
Module-owned stores and files
  ├─ P25: call metadata + recordings + transcripts
  ├─ APRS: station / packet / weather
  ├─ ADS-B: aircraft / position history
  ├─ Weather: SAME alerts / satellite passes / imagery
  └─ Other modules: their own store(s) as appropriate
                │
Optional federated event-search API and map-layer composition
```

## Receiver and channel lifecycle

1. Device manager enumerates SDR hardware, reporting native tune ranges, sample rates, RX/TX support, channel count and gain controls.
2. A receiver session obtains exclusive control of the **physical device**, while multiple virtual channels can share a sampled RF span.
3. A resource scheduler evaluates requested center frequencies, occupied bandwidth and task priority; it refuses unsafe auto-retunes.
4. Source samples stream through processing stages with bounded queues, timestamps and telemetry.
5. Modules publish domain events and store their own persistent records; subscribers display information or perform notifications.
6. Closing or moving a UI panel only unsubscribes that view; it does not terminate the receiver session.

Receiver reconfiguration must clearly indicate which jobs will be interrupted. A device may run a single trunking task or several channelized tasks, depending on available RF bandwidth, processing capacity, and decoder requirements.

## Event contracts and independent databases

Define a small common envelope for intermodule communication, for example:

```text
event_id, kind, producer, observed_utc, receiver_id,
channel_id, payload_schema_version, payload
```

This **does not require a universal database**. Each module stores the complete records that matter to it. The optional combined timeline queries module-level read APIs or a rebuildable derived index. Avoid writing the same authoritative event independently to multiple domain stores.

For recorded calls, use stable media identifiers and store file paths/URIs and metadata in the owning module's database. Use atomic writes, recovery handling and retention cleanup coordinated with active playback and upload jobs.

## Customizable workspace

- Dock, float, tab, resize and rearrange panels, including moving floating panels to another display.
- Multiple instances of eligible views (e.g., one ADS-B map and one APRS map), sharing a processing module.
- Workspace profile files include layout version, visible panels, data-source selections, layer toggles and device/task associations.
- Qt docking alternatives require license, maintenance and Windows/Linux/X11/Wayland behavior review.
- On some Linux Wayland compositors, exact floating-window positioning is restricted; restore as much layout as the compositor permits.

## Third-party component evaluation (not selected yet)

| Domain | Candidate / concern |
| --- | --- |
| Hardware | SoapySDR with explicit adapters for devices or capabilities not covered |
| FFT/DSP | Benchmark appropriate maintained FFT and DSP libraries |
| Dockable UI | Qt docking infrastructure or an appropriately licensed advanced docking library |
| Speech-to-text | Local transcription via a C/C++ runtime, with optional user-configured alternatives |
| P25 / trunking | Assess decoding libraries/codecs, licensing, sample recordings and simulcast handling |
| Satellite | Research current downlinks and formats; evaluate interoperability with existing open-source tooling |
| Mapping | Assess Qt-compatible map layers, offline tiles and licensing/attribution |
| TV | Investigate ATSC standards and hardware processing needs before scope commitment |
| Email / OpenMHz | Separate network adapters with authenticated, retryable job queues |

Do not copy third-party source or select libraries without a recorded license and distribution compatibility assessment.

## Testing and engineering conventions

- Unit tests: RF-to-hardware frequency translation, scan scheduling, decoder state machines, event serialization, retention.
- Integration tests: prerecorded I/Q fixtures, multiple simulated receivers, simultaneous decoder jobs, dropped-sample recovery.
- End-to-end smoke tests: Windows 11 initially, Linux CI introduced at foundation phase.
- Benchmark high-throughput pipelines; preserve UI responsiveness.
- Code should build with warnings and static analysis, with generated documentation for public APIs.
- Version module contracts and provide graceful fallback for unsupported hardware or data formats.

## First engineering slice

Create CMake scaffolding and a Qt shell; enumerate mock and initial real SDR hardware; implement one waterfall with a tunable VFO; provide a simple AM/NFM demodulator; save/restore a multi-panel workspace; exercise a Linux build early. This is the first runnable target, not the entire feature set.
