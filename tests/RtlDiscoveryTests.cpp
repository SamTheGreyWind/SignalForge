#include "core/RtlDiscovery.h"

#include <filesystem>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Expected absolute path to mock rtlsdr DLL/SO\n";
        return 2;
    }
    const auto result = signalforge::core::enumerateRtlDevices(argv[1]);
    if (!result.driverLoaded || result.devices.size() != 3) {
        std::cerr << "Failed to enumerate the mock receiver library: "
                  << result.diagnostic << '\n';
        return 1;
    }
    if (!result.devices[0].uniqueSerial ||
        result.devices[1].uniqueSerial ||
        result.devices[2].uniqueSerial ||
        result.devices[0].serial != "SF-V4-0001" ||
        result.devices[1].product != "Blog V4" ||
        result.devices[2].deviceIndex != 2) {
        std::cerr << "Incorrect per-device serial/identity reporting\n";
        return 1;
    }
    // Absent libraries must fail cleanly, without opening a device.
    const auto absent = signalforge::core::enumerateRtlDevices(
        (std::filesystem::temp_directory_path() /
         "signalforge-this-rtl-driver-does-not-exist-9802.dll").string());
    if (absent.driverLoaded || absent.diagnostic.empty()) {
        std::cerr << "Expected a graceful diagnostic for a missing DLL\n";
        return 1;
    }
    std::cout << "RTL discovery: valid mock, duplicate identity and missing "
                 "library error cases passed.\n";
    return 0;
}
