# SignalForge — Product Requirements (Draft)

**Project tagline:** Forged by Operators. Built for Everyone.  
**Status:** Vision and requirements baseline; no feature below should be interpreted as implemented.  
**Target users:** SDR enthusiasts, radio operators, researchers, and lawful radio-monitoring stations.

## Primary design requirements

1. **Windows-first; cross-platform by design:** Windows 11 is the initial test target. Avoid Windows-only business logic so Linux ports use the same SDR/DSP engine; macOS is a future possibility.
2. **True modularity:** A module owns functions and data acquisition. UI panels are separately instantiated views. Hiding/closing a panel must not stop monitoring.
3. **CAD-style customizable UI:** Dock, undock, tab, hide, resize and arrange panels across multiple windows/monitors. Save and restore named workspace layouts.
4. **Multiple receivers:** Independent SDR device configuration and scheduling; several dongles may serve concurrent tasks. A single captured bandwidth may support multiple channelized demodulators.
5. **Operator control and transparency:** Profiles and automation are customizable, reversible and observable. Do not unexpectedly retune a receiver dedicated to a high-priority task.
6. **Domain-separated data by default:** P25, APRS, ADS-B, weather and other services have their own data stores. Optionally present a unified events/search view, with a separately enabled index only if needed.
7. **Offline-capable core:** Essential receiving, viewing, decoding, archival playback and configuration should operate without an account or internet. Network services are optional.
8. **Responsible operation:** Clearly identify encrypted/unsupported signals, require explicit user-initiated transmission controls, and protect sensitive data (credentials, location, call archives).

## Original 18 feature requests

| ID | Requirement | Intended behavior / acceptance direction |
| --- | --- | --- |
| F01 | Spectrum waterfall and visual VFO | Fast spectral display with a draggable VFO. Support both movable-VFO/stationary-spectrum and center-locked-VFO/scrolled-spectrum tuning modes. |
| F02 | Digital radio and trunking | Architecture for digital protocols including APCO P25 Phase I/II and talkgroup/control-channel tracking, subject to hardware, codecs, simulcast constraints, and lawful access. |
| F03 | Frequency scanning | Named scan lists and user-defined frequency-range scanning, steps, dwell, squelch, activity hold and lockouts. |
| F04 | Band-plan overlays | Optional frequency/channel annotations superimposed on spectrum/waterfall; configurable by region and user. |
| F05 | Audio transcription and email alerts | Timestamped radio-call transcripts linked to audio and metadata; rule-driven email delivery. Display accuracy/confidence limits. |
| F06 | OpenMHz integration | Upload compatible public-safety calls with associated system/talkgroup metadata, configurable authentication, and retry queue. Validate against current service specifications. |
| F07 | NOAA SAME decoding | Decode Specific Area Message Encoding alerts and support locally configured text/email notifications. |
| F08 | Software demodulation | Implement common receiver modes (e.g., AM, narrow/wide FM, SSB, CW) and extensible modulation plug-ins. |
| F09 | Instant playback and rolling archive | Retain approximately 24 hours of monitored radio traffic by default, organized by call/channel/system; adjustable hold/retention and immediate playback. |
| F10 | ADS-B mapping | Decode compatible aircraft broadcasts and plot their reported positions/tracks on an aircraft map. |
| F11 | APRS mapping and weather | Decode APRS packets, stations, position reports and weather observations, with collection/history. |
| F12 | Concurrent dongles | Assign multiple SDR devices to distinct bands, trunked systems, scans, or decoding jobs simultaneously. |
| F13 | Weather satellites | Track compatible passes, capture weather satellite data and generate imagery/products with suitable antennas/hardware; decoder formats updated as operational satellites change. |
| F14 | Frequency converters | Per-device up/downconverter toggle and offset/calibration; distinguish RF frequency from tuned hardware frequency. |
| F15 | SDR transmission | Explicitly enabled TX workflow for supported transmit-capable devices (e.g., HackRF), within lawful limits and hardware capabilities. |
| F16 | ATIS monitoring | Tune, monitor, record and optionally transcribe supported aviation ATIS broadcasts; avoid assuming every station is receivable. |
| F17 | CW decode and transcribe | SSB reception and configurable Morse-code decoding/transcription with appropriate filtering. |
| F18 | Over-the-air television | Explore reception/decoding of supported broadcast-TV standards; hardware sample-rate, codec and content protection support vary. |

## Additional agreed requirement

| ID | Requirement | Intended behavior / acceptance direction |
| --- | --- | --- |
| F19 | Modular dockable multi-window UI | GeoConex CAD-inspired operator workspace: independently docked/floating/tabbed panels, multiple monitors, presets and persistent layouts, without coupling views to decoder uptime. |

## Clarifications and user choices

- **Separate vs. combined event history:** Separate module-owned databases remain the default. An optional federated/combined view must not force data duplication or physical database consolidation.
- **APRS vs. ADS-B maps:** Each owns its data and can be viewed independently. A shared map panel may show either/both through simple layer toggles; multiple map panels can be open at once.
- **Recording model:** Store per-call audio and metadata for speech traffic, with configurable retention/hold; support raw I/Q capture separately for troubleshooting.
- **P25 limitations:** Encrypted traffic is not assumed to be recoverable. Simulcast environments and decoder/codecs require specialized testing.
- **Alert model:** Notifications need user-defined matching rules, rate limits, opt-in network credentials, and test modes.
- **No ATAK or Android dependency:** This desktop project is separate from the earlier FieldSDR Android concept.

## Candidate future enhancements (not baseline commitments)

Priority scanning; programmable monitoring schedule; event-based automation rules; RF I/Q capture and replay; optional federated search index; system health telemetry; headless monitoring service; accessible keyboard-driven workflows; receiver/resource scheduler; remote read-only monitoring dashboards.

## Nonfunctional quality criteria

- **Stability:** Receiving/recording continues even if panels are closed, minimized or rearranged.
- **Performance:** DSP and disk writers are off the GUI thread; bounded queues, backpressure and dropped-sample indicators are required.
- **Recoverability:** Interrupted sessions and upload failures are recoverable; retention cleanup is predictable.
- **Interoperability:** Hardware modules have capability negotiation for RX/TX, sample rate, tuning range and gain.
- **Portability:** Build and test shared code on Windows and Linux early.
- **Privacy:** Store local data locally unless the operator enables uploads/notifications; protect secrets from logs.
- **Verifiability:** Test using synthetic signals and recordings before requiring physical hardware for every test.

## Open questions

Licensing compatibility; first hardware reference devices; first SDR bandwidth/sample-rate targets; P25 and TV implementation/library choices; map base-layer and offline support; audio storage formats; user-data retention policies; operational limits for TX modules.
