#pragma once

#include <string>
#include <vector>

namespace signalforge::core {

// An RTL-SDR USB device reported by a V4-compatible librtlsdr runtime.
// Enumeration does not claim/open a device or tune a receiver.
struct RtlDeviceInfo {
    unsigned int deviceIndex = 0;
    std::string name;
    std::string manufacturer;
    std::string product;
    std::string serial;
    bool uniqueSerial = false;
};

struct RtlDiscoveryResult {
    bool driverLoaded = false;
    std::string diagnostic;
    std::vector<RtlDeviceInfo> devices;
};

// Dynamically loads librtlsdr: no hardware/development headers are required
// to build SignalForge. On Windows, uses rtlsdr.dll next to the executable or
// in the OS safe DLL search paths. A user-supplied path must be absolute.
// Driver releases from RTL-SDR Blog support the special V4 R828D tuner.
//
// A driver compatible with the RTL-SDR Blog V4 is crucial: older DLLs may
// enumerate the device yet tune it incorrectly. Never enable bias tee here.
[[nodiscard]] RtlDiscoveryResult enumerateRtlDevices(
    const std::string& driverPathUtf8 = {});

} // namespace signalforge::core
