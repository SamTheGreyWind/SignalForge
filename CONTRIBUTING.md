# Contributing to SignalForge

Thanks for helping build **SignalForge — The Open-Source Radio Multitool**.

> Forged by Operators. Built for Everyone.

**Project state:** Early development. SignalForge-authored software is licensed under `GPL-3.0-or-later`. Technical conventions and third-party dependency audits are still evolving. Please discuss substantial features or third-party code integrations in an issue before investing heavily in changes.

## How we collaborate

- Open an issue for bugs, feature ideas, hardware compatibility reports or decoder behavior.
- Keep issues focused: observed behavior, expected behavior, OS, SDR device, signal/standard, and reproducible steps where possible.
- Propose changes in a feature branch and submit a pull request targeting `main`.
- Describe tests performed, performance implications, operating-system impact and documentation changes.
- Avoid committing credentials, API keys, sensitive captures, private location histories or copyrighted sample materials without authorization.

## Code direction

- C++20, Qt 6 and CMake are the current direction; keep radio/DSP logic independent of GUI.
- Favor unit-testable, modular code and interfaces that do not require physical radios for every test.
- Maintain Windows 11 support and keep Linux build compatibility in view.
- Ensure callbacks and decoder workers cannot block the GUI thread.
- Explain new library/code dependencies and identify their licenses.

## Responsible use

Respect spectrum regulations, privacy, content rights and the service rules for OpenMHz and other uploads. Transmit-capable features require explicit operator control and tested hardware safeguards. Do not submit features intended to bypass encryption or lawful access controls.

## Licensing note

The SignalForge software project is licensed under **GPL-3.0-or-later**; see [LICENSE](LICENSE). Submit code contributions only if you have the rights to offer them under these terms. By submitting a contribution for inclusion, you agree that your contribution may be distributed under GPL-3.0-or-later, unless a different license for a specific asset has been explicitly agreed to and documented. You retain ownership of your contributions; no contributor license agreement or copyright assignment is currently required. Document the license and provenance of third-party material before adding it. Public-domain map data and third-party Qt runtime libraries are not relicensed by SignalForge's repository license.
