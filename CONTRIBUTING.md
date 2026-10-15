# Contributing to SignalForge

Thanks for helping build **SignalForge — The Open-Source Radio Multitool**.

> Forged by Operators. Built for Everyone.

**Project state:** Early planning. Technical conventions and an open-source license are not final yet. Please discuss substantial features or third-party code integrations in an issue before investing heavily in changes.

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

SignalForge intends to use an open-source license, but a LICENSE file has not yet been adopted. Contributions are not being solicited under an undefined license: before accepting nontrivial external code contributions, maintainers will select and publish the governing license and contribution terms.
