# Third-party SDR runtime notices

The Windows preview of SignalForge (licensed GPL-3.0-or-later) includes
**third-party dynamically linked driver libraries**; this notice is also
copied into each Windows preview ZIP. The DLLs are deliberately not committed
into the SignalForge source repository.

## RTL-SDR Blog `rtlsdr.dll`

- Upstream: https://github.com/rtlsdrblog/rtl-sdr-blog
- Source tag: `V1.4.0`; binary from:
  https://github.com/rtlsdrblog/rtl-sdr-blog/releases/download/V1.4.0/Release.zip
- Release archive SHA-256:
  `7ef33f1304647f65e5e0fde43637a73d54f076e91e651a3cecc4f55a17fd9815`
- License: **GPL-2.0-or-later** as indicated in the upstream source.
  Full upstream GPLv2 text: `licenses/rtl-sdr-blog/COPYING`.
- Source availability: the public release-tag repository above.
- The driver is *not* a USB kernel driver. It is the application-facing
  userspace library that uses the existing WinUSB/libusb device interface.
  It supports standard RTL2832U-class devices and RTL-SDR Blog V4/V4L.
  Its presence does not prove that any attached SDR is available.

## `pthreadVC2.dll`

- Distributed as an unmodified x64 runtime dependency in the **same
  RTL-SDR Blog V1.4.0 archive**.
- This runtime is commonly known as pthreads-win32. The pthreads-win32
  project identifies the relevant older distributions as **LGPL-2.1**.
- Source and copyright information:
  https://sourceware.org/pthreads-win32/
  https://github.com/grumpycoders/pthreads-win32
- Copyright-specific notices: `licenses/pthreads-win32/COPYING`.
- Full LGPL-2.1 license:
  `licenses/pthreads-win32/COPYING.LIB`.
- You may replace dynamically linked libraries with compatible versions.
  Redistribution and reverse-engineering for debugging library modifications
  are governed by the applicable LGPL terms.

## User control and future adapters

SignalForge discovers `rtlsdr.dll` in the executable's trusted default
Windows DLL lookup paths. The Receivers panel also allows an explicit
absolute path to a different native RTL-SDR library. Selecting an unknown
DLL executes code from it: only choose a trusted build.

Other SDR backends will be added individually, each with its own upstream
source, version, checksum, license notices and Windows loading tests.
A manufacturer's Windows **USB driver** is distinct from these application
runtime libraries; installing this ZIP will never silently change the
operating system's USB driver.
