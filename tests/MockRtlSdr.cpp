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


#include <atomic>
#include <chrono>
#include <cmath>
#include <thread>
#include <vector>

// Fake USB stream: deliberately uses NO RTL-SDR hardware. FFT should show
// a positive-frequency +Fs/8 carrier after frequency shifting.
struct MockDevice {
    std::atomic<bool> cancelled{false};
    uint32_t frequency = 100'000'000;
};
RTL_EXPORT int rtlsdr_open(void** out, uint32_t index) {
    if (!out || index >= 3) return -1;
    *out = new MockDevice();
    return 0;
}
RTL_EXPORT int rtlsdr_close(void* dev) {
    delete static_cast<MockDevice*>(dev);
    return 0;
}
RTL_EXPORT int rtlsdr_set_center_freq(void* dev, uint32_t frequency) {
    if (!dev) return -1;
    static_cast<MockDevice*>(dev)->frequency = frequency;
    return 0;
}
RTL_EXPORT int rtlsdr_set_sample_rate(void* dev, uint32_t sampleRate) {
    return dev && sampleRate == 2'400'000 ? 0 : -1;
}
RTL_EXPORT int rtlsdr_set_tuner_gain_mode(void* dev, int) {
    return dev ? 0 : -1;
}
RTL_EXPORT int rtlsdr_reset_buffer(void* dev) {
    return dev ? 0 : -1;
}
RTL_EXPORT int rtlsdr_cancel_async(void* dev) {
    if (!dev) return -1;
    static_cast<MockDevice*>(dev)->cancelled.store(true);
    return 0;
}
RTL_EXPORT int rtlsdr_read_async(void* dev,
     void (*cb)(unsigned char*, uint32_t, void*), void* ctx,
     uint32_t, uint32_t bufferLength) {
    if (!dev || !cb || bufferLength < 2048) return -1;
    auto* receiver = static_cast<MockDevice*>(dev);
    receiver->cancelled = false;
    std::vector<unsigned char> iq(bufferLength);
    while (!receiver->cancelled.load()) {
        for (std::size_t i = 0; i < iq.size() / 2; ++i) {
            const double phase = 2.0 * 3.14159265358979323846 * (i % 8) / 8;
            iq[2 * i] = static_cast<unsigned char>(128 + 78 * std::cos(phase));
            iq[2 * i + 1] = static_cast<unsigned char>(128 + 78 * std::sin(phase));
        }
        cb(iq.data(), bufferLength, ctx);
        std::this_thread::sleep_for(std::chrono::milliseconds(6));
    }
    return 0;
}
