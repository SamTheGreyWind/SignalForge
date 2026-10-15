#include <cstdio>
#include <cstdint>

#if defined(_WIN32)
#define RTL_EXPORT extern "C" __declspec(dllexport)
#else
#define RTL_EXPORT extern "C" __attribute__((visibility("default")))
#endif

// A standalone mock of the public RTL-SDR discovery API. The DLL/SO itself
// does not use radio hardware and is built only as a test fixture.
RTL_EXPORT uint32_t rtlsdr_get_device_count() {
    return 3;
}

RTL_EXPORT const char* rtlsdr_get_device_name(uint32_t index) {
    if (index < 3) return "Mock RTL-SDR Blog V4";
    return "";
}

RTL_EXPORT int rtlsdr_get_device_usb_strings(
    uint32_t index, char* manufacturer, char* product, char* serial) {
    if (index >= 3 || !manufacturer || !product || !serial) return -1;
    std::snprintf(manufacturer, 256, "%s", "RTLSDRBlog");
    std::snprintf(product, 256, "%s", "Blog V4");
    // The last two pretend to have overlapping EEPROM serials.
    std::snprintf(serial, 256, "%s", index == 0 ? "SF-V4-0001" : "SF-V4-0002");
    return 0;
}
