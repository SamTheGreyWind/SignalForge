#include "core/RtlDiscovery.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <map>
#include <string>

#if defined(_WIN32)
#define NOMINMAX
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace signalforge::core {
namespace {

class Library final {
public:
    explicit Library(const std::string& path) {
        if (!path.empty() && !std::filesystem::path(path).is_absolute()) {
            diagnostic_ = "Choose an absolute path to the RTL-SDR driver library.";
            return;
        }
#if defined(_WIN32)
        auto load = [this](const wchar_t* name, DWORD flags) {
            handle_ = LoadLibraryExW(name, nullptr, flags);
        };
        if (!path.empty()) {
            const int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                                 path.c_str(), -1, nullptr, 0);
            if (size == 0) {
                diagnostic_ = "The selected driver path is not valid UTF-8.";
                return;
            }
            std::wstring wide(static_cast<std::size_t>(size), L'\0');
            MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path.c_str(),
                                -1, wide.data(), size);
            load(wide.c_str(), LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR |
                                LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
        } else {
            // Restrict implicit DLL lookup to trusted Windows search paths,
            // including the application's own directory (not CWD).
            load(L"rtlsdr.dll", LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
            if (!handle_)
                load(L"librtlsdr.dll", LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
        }
        if (!handle_)
            diagnostic_ = "Could not load a V4-compatible RTL-SDR DLL (Windows error " +
                          std::to_string(GetLastError()) + ").";
#else
        if (!path.empty()) {
            handle_ = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
        } else {
            handle_ = dlopen("librtlsdr.so.0", RTLD_NOW | RTLD_LOCAL);
            if (!handle_) handle_ = dlopen("librtlsdr.so", RTLD_NOW | RTLD_LOCAL);
        }
        if (!handle_) {
            const char* error = dlerror();
            diagnostic_ = error ? error : "Could not load librtlsdr.";
        }
#endif
    }

    ~Library() {
#if defined(_WIN32)
        if (handle_) FreeLibrary(handle_);
#else
        if (handle_) dlclose(handle_);
#endif
    }

    Library(const Library&) = delete;
    Library& operator=(const Library&) = delete;

    [[nodiscard]] bool loaded() const { return handle_ != nullptr; }
    [[nodiscard]] const std::string& diagnostic() const { return diagnostic_; }

    template <typename FunctionPointer>
    FunctionPointer symbol(const char* name) const {
        if (!handle_) return nullptr;
#if defined(_WIN32)
        return reinterpret_cast<FunctionPointer>(GetProcAddress(handle_, name));
#else
        // dlsym returns void*; function pointer conversion is supported by
        // the target POSIX implementations for dynamically loaded functions.
        return reinterpret_cast<FunctionPointer>(dlsym(handle_, name));
#endif
    }

private:
#if defined(_WIN32)
    HMODULE handle_ = nullptr;
#else
    void* handle_ = nullptr;
#endif
    std::string diagnostic_;
};

using DeviceCount = uint32_t (*)();
using DeviceName = const char* (*)(uint32_t);
using UsbStrings = int (*)(uint32_t, char*, char*, char*);

std::string safeString(const char* text) {
    return text ? std::string(text) : std::string();
}

} // namespace

RtlDiscoveryResult enumerateRtlDevices(const std::string& driverPathUtf8) {
    RtlDiscoveryResult result;
    Library library(driverPathUtf8);
    if (!library.loaded()) {
        result.diagnostic = library.diagnostic();
        return result;
    }

    const auto getCount = library.symbol<DeviceCount>("rtlsdr_get_device_count");
    const auto getName = library.symbol<DeviceName>("rtlsdr_get_device_name");
    const auto getStrings = library.symbol<UsbStrings>("rtlsdr_get_device_usb_strings");
    if (!getCount || !getName || !getStrings) {
        result.diagnostic =
            "The loaded library is missing required RTL-SDR discovery functions.";
        return result;
    }
    result.driverLoaded = true;

    constexpr uint32_t kMaxEnumerated = 64;
    const uint32_t reported = getCount();
    const uint32_t count = std::min(reported, kMaxEnumerated);
    std::map<std::string, unsigned> serialCounts;

    for (uint32_t index = 0; index < count; ++index) {
        RtlDeviceInfo device;
        device.deviceIndex = index;
        device.name = safeString(getName(index));

        std::array<char, 256> maker{};
        std::array<char, 256> product{};
        std::array<char, 256> serial{};
        if (getStrings(index, maker.data(), product.data(), serial.data()) == 0) {
            maker.back() = '\0';
            product.back() = '\0';
            serial.back() = '\0';
            device.manufacturer = maker.data();
            device.product = product.data();
            device.serial = serial.data();
        }
        if (!device.serial.empty())
            ++serialCounts[device.serial];
        result.devices.push_back(std::move(device));
    }
    for (auto& device : result.devices) {
        device.uniqueSerial = !device.serial.empty() &&
                              serialCounts[device.serial] == 1;
    }
    if (reported > kMaxEnumerated) {
        result.diagnostic = "Driver reported more than 64 devices; "
                            "showing only the first 64.";
    } else if (result.devices.empty()) {
        result.diagnostic = "RTL-SDR driver loaded, but no receivers were detected. "
                            "Check USB/WinUSB access and that no other SDR program "
                            "has claimed the device.";
    } else {
        result.diagnostic = "Found " + std::to_string(result.devices.size()) +
                            " RTL-SDR receiver(s). Enumeration only; no IQ "
                            "sampling or radio tuning has started.";
    }
    return result;
}

} // namespace signalforge::core
