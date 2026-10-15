# SignalForge — Development Roadmap (Draft)

**Strategy:** Ship useful, testable increments. Windows 11 is the initial user-facing target; test Linux portability from the first engine milestone. This roadmap expresses sequence and dependency, not calendar promises.

## Phase 0 — Establish the project

- Publish README, requirements, architecture and contribution process.
- Select a specific open-source license after reviewing third-party dependencies.
- Set up CMake, formatting, tests, CI and issue templates.
- Agree on a small reference hardware list and sample I/Q fixtures.

**Exit:** Reproducible Windows build instructions and a minimal Linux build path.

## Phase 1 — SDR receiver foundation

- SDR device abstraction and enumeration, including simulated receiver.
- Real sample acquisition, FFT spectrum, waterfall and interactive VFO.
- Tuning, converter offsets, gain, filters, basic AM/FM/SSB/CW receive modes.
- Frequency lists/range scan and band-plan overlays.
- Docking, floating panels, saved workspace layouts.
- Hardware and DSP diagnostics.

**Exit:** Operator can monitor an analog channel on Windows, see a responsive waterfall and save/restore a workspace. Core compiles on Linux.

## Phase 2 — Monitoring, archives and multi-receiver

- Independent receiver assignments and safe resource scheduling.
- Call/audio-triggered recording and 24-hour default retention configuration.
- Per-call archive indexing, instant playback and media integrity/recovery.
- Local rules engine, optional notifications and credential management.
- Raw I/Q capture/replay for tests and support.

**Exit:** Several supported SDRs can run concurrently, with stable recording and playback while UI panels move/close.

## Phase 3 — Trunked public-safety monitoring

- Digital demodulator interfaces; P25 control/traffic channel processing, Phase I then Phase II research and implementation.
- Talkgroup metadata, active-call panels and call archive.
- Local speech transcription with accuracy/error handling and configurable email alerts.
- OpenMHz-compatible upload adapter with queuing/retry, tested against documented API and service policy.

**Exit:** Demonstrated end-to-end lawful, unencrypted compatible P25 call capture -> metadata/audio -> local transcription -> optional upload or alert. Different system types require compatibility testing.

## Phase 4 — Conventional alerts and decoding

- NOAA SAME alert decoding and alert rules.
- ATIS monitoring and archive/transcription.
- Improved SSB/CW reception, Morse decoding and scanning controls.

**Exit:** Each protocol module is independently startable, testable and observable.

## Phase 5 — Maps, aircraft, packet radio and satellites

- ADS-B positions/tracks with independent data store.
- APRS stations, packets and weather data with independent data store.
- Separate/combined map layers; multiple map panels and optional offline maps.
- Weather-satellite pass planning, reception and imagery for current supported formats.

**Exit:** Aircraft and APRS can be viewed separately or together without merging their original databases; a compatible weather-satellite data product can be generated from test captures.

## Phase 6 — Advanced SDR capabilities

- Transmit workflow for supported hardware with safety controls and local compliance documentation.
- OTA broadcast TV decoding research/prototype, per-standard compatibility reporting.
- Additional devices, codecs, protocols, profiles and community-authored modules.
- Consider remote/headless service operation and macOS builds.

**Exit:** Advanced modules can be deployed without destabilizing core receiving and archive services.

## Cross-cutting work at every phase

Documentation, license review, privacy/security review, test fixtures, performance profiling, accessibility, Windows/Linux testing, hardware compatibility matrix, and issue-driven community feedback.

## Definition of done (per feature)

1. Document operator-facing behavior, limitations and configuration.
2. Add repeatable unit/integration tests (and device/manual tests when applicable).
3. Confirm module can be disabled without affecting unrelated receivers/jobs.
4. Test failure/recovery paths and resource usage.
5. Note supported SDR devices, OS versions, signal formats and any licensing requirements.
6. Update release notes and user documentation.
