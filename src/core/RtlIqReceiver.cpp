#include "core/RtlIqReceiver.h"
#include "core/IqSpectrum.h"

#include <algorithm>
#include <filesystem>
#include <string>
#include <utility>

#if defined(_WIN32)
#define NOMINMAX
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace signalforge::core {
namespace {

class NativeLibrary {
public:
    explicit NativeLibrary(const std::string& path) {
        if (!path.empty() && !std::filesystem::path(path).is_absolute()) {
            error_ = "RTL driver library path must be absolute.";
            return;
        }
#if defined(_WIN32)
        if (path.empty()) {
            handle_ = LoadLibraryExW(L"rtlsdr.dll", nullptr,
                                     LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
            if (!handle_)
                handle_ = LoadLibraryExW(L"librtlsdr.dll", nullptr,
                                         LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
        } else {
            const int len = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                                path.c_str(), -1, nullptr, 0);
            if (len > 0) {
                std::wstring wide(static_cast<std::size_t>(len), L'\0');
                MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                    path.c_str(), -1, wide.data(), len);
                handle_ = LoadLibraryExW(wide.c_str(), nullptr,
                    LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
            }
        }
        if (!handle_) error_ = "Unable to load rtlsdr.dll and its dependencies "
                                "(Windows error " + std::to_string(GetLastError()) + ").";
#else
        if (!path.empty()) handle_ = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
        else {
            handle_ = dlopen("librtlsdr.so.0", RTLD_NOW | RTLD_LOCAL);
            if (!handle_)
                handle_ = dlopen("librtlsdr.so", RTLD_NOW | RTLD_LOCAL);
        }
        if (!handle_) {
            if (const char* why = dlerror()) error_ = why;
        }
#endif
    }
    ~NativeLibrary() {
#if defined(_WIN32)
        if (handle_) FreeLibrary(handle_);
#else
        if (handle_) dlclose(handle_);
#endif
    }
    [[nodiscard]] bool loaded() const { return handle_ != nullptr; }
    [[nodiscard]] std::string error() const { return error_; }
    template <typename Fn> Fn get(const char* symbol) {
#if defined(_WIN32)
        return handle_ ? reinterpret_cast<Fn>(GetProcAddress(handle_, symbol)) : nullptr;
#else
        return handle_ ? reinterpret_cast<Fn>(dlsym(handle_, symbol)) : nullptr;
#endif
    }

private:
#if defined(_WIN32)
    HMODULE handle_ = nullptr;
#else
    void* handle_ = nullptr;
#endif
    std::string error_;
};

using Open = int (*)(void**, std::uint32_t);
using Close = int (*)(void*);
using SetFreq = int (*)(void*, std::uint32_t);
using SetRate = int (*)(void*, std::uint32_t);
using SetGain = int (*)(void*, int);
using Reset = int (*)(void*);
using ReadAsync = int (*)(void*, void (*)(unsigned char*, std::uint32_t, void*),
                         void*, std::uint32_t, std::uint32_t);
using CancelAsync = int (*)(void*);

} // namespace

RtlIqReceiver::~RtlIqReceiver() { stop(); }

void RtlIqReceiver::updateStatus(std::string status, bool receiving) {
    std::lock_guard lock(stateMutex_);
    latest_.status = std::move(status);
    latest_.receiving = receiving;
}

void RtlIqReceiver::start(unsigned int index, std::uint32_t centerHz,
                          const std::string& driver) {
    stop();
    if (centerHz < 24'000'000 || centerHz > 1'700'000'000) {
        updateStatus("Frequency must be 24–1700 MHz for this preview.", false);
        return;
    }
    requestedCenterHz_ = centerHz;
    requestedRunning_ = true;
    {
        std::lock_guard lock(stateMutex_);
        latest_ = {};
        latest_.deviceIndex = index;
        latest_.centerHz = centerHz;
        latest_.status = "Opening RTL-SDR device " + std::to_string(index) + "...";
    }
    worker_ = std::thread([this, index, centerHz, driver] {
        run(index, centerHz, driver);
    });
}

void RtlIqReceiver::stop() {
    requestedRunning_ = false;
    {
        std::lock_guard lock(driverMutex_);
        if (activeDevice_ && cancelAsync_) cancelAsync_(activeDevice_);
    }
    if (worker_.joinable()) worker_.join();
    {
        std::lock_guard lock(driverMutex_);
        activeDevice_ = nullptr;
        cancelAsync_ = nullptr;
    }
    updateStatus("Receiver stopped.", false);
}

void RtlIqReceiver::retune(std::uint32_t frequency) {
    if (frequency < 24'000'000 || frequency > 1'700'000'000) return;
    requestedCenterHz_ = frequency;
    // Interrupt async capture so ONLY the owning worker thread reconfigures
    // the device. Never retune the dongle from a GUI callback.
    std::lock_guard lock(driverMutex_);
    if (activeDevice_ && cancelAsync_) cancelAsync_(activeDevice_);
}

RtlIqSnapshot RtlIqReceiver::snapshot() const {
    std::lock_guard lock(stateMutex_);
    return latest_;
}

void RtlIqReceiver::onAsyncData(unsigned char* iq, std::uint32_t len, void* ctx) {
    if (ctx)
        static_cast<RtlIqReceiver*>(ctx)->receiveBytes(iq, len);
}

void RtlIqReceiver::receiveBytes(const unsigned char* iq, std::uint32_t byteCount) {
    if (!requestedRunning_ || !iq || byteCount < kRtlFftSize * 2) return;
    // At 2.4MSPS there are hundreds of USB callback buffers per second.
    // Bound FFT CPU work; keep one newest power frame for all GUI subscribers.
    if ((++callbackCount_ % 12) != 0) return;
    auto fft = fftPowerFromU8Iq(iq, byteCount);
    if (fft.empty()) return;
    std::lock_guard lock(stateMutex_);
    latest_.power = std::move(fft);
    ++latest_.fftFrames;
}

void RtlIqReceiver::run(unsigned int index, std::uint32_t initial,
                         std::string driver) {
    NativeLibrary library(driver);
    if (!library.loaded()) {
        updateStatus(library.error(), false);
        requestedRunning_ = false;
        return;
    }
    const auto open = library.get<Open>("rtlsdr_open");
    const auto close = library.get<Close>("rtlsdr_close");
    const auto setFreq = library.get<SetFreq>("rtlsdr_set_center_freq");
    const auto setRate = library.get<SetRate>("rtlsdr_set_sample_rate");
    const auto setGain = library.get<SetGain>("rtlsdr_set_tuner_gain_mode");
    const auto reset = library.get<Reset>("rtlsdr_reset_buffer");
    const auto read = library.get<ReadAsync>("rtlsdr_read_async");
    const auto cancel = library.get<CancelAsync>("rtlsdr_cancel_async");
    if (!open || !close || !setFreq || !setRate || !setGain ||
        !reset || !read || !cancel) {
        updateStatus("RTL-SDR library missing a streaming API symbol.", false);
        requestedRunning_ = false;
        return;
    }

    void* device = nullptr;
    const int opened = open(&device, index);
    if (opened != 0 || !device) {
        updateStatus("Could not open RTL-SDR index " + std::to_string(index) +
            " (driver error " + std::to_string(opened) +
            "). Close other SDR programs and check USB access.", false);
        requestedRunning_ = false;
        return;
    }

    // The worker owns open/configure/close. Other threads may only request
    // async cancellation (under driverMutex_), never access device directly.
    {
        std::lock_guard lock(driverMutex_);
        activeDevice_ = device;
        cancelAsync_ = cancel;
    }

    const std::uint32_t rate = 2'400'000;
    int rc = setRate(device, rate);
    if (rc == 0) rc = setGain(device, 0); // automatic tuner gain
    if (rc == 0) rc = setFreq(device, initial);
    if (rc == 0) rc = reset(device);
    if (rc != 0) {
        updateStatus("Failed to configure RTL-SDR (driver error " +
                     std::to_string(rc) + ").", false);
        requestedRunning_ = false;
    }

    while (requestedRunning_) {
        const auto desired = requestedCenterHz_.load();
        if (desired != initial) {
            rc = setFreq(device, desired);
            if (rc == 0) rc = reset(device);
            if (rc != 0) {
                updateStatus("Failed to retune RTL-SDR (driver error " +
                             std::to_string(rc) + ").", false);
                break;
            }
            initial = desired;
        }
        appliedCenterHz_ = initial;
        {
            std::lock_guard lock(stateMutex_);
            latest_.centerHz = initial;
            latest_.sampleRate = rate;
            latest_.power.clear(); // previous history cannot be relabeled
            latest_.receiving = true;
            latest_.status = "LIVE I/Q • RTL index " + std::to_string(index) +
                             " • " + std::to_string(initial) +
                             " Hz • 2.4 MSPS • automatic gain";
        }
        // Async I/Q capture runs until Stop, Retune or driver failure.
        // The USB callback computes periodic FFT frames for the UI.
        rc = read(device, &RtlIqReceiver::onAsyncData, this, 0, 16'384);
        if (!requestedRunning_) break;
        if (requestedCenterHz_.load() == initial) {
            updateStatus("RTL-SDR capture ended unexpectedly (code " +
                         std::to_string(rc) + "). Unplug/replug and restart.", false);
            break;
        }
    }

    requestedRunning_ = false;
    // Close only after async capture has returned and callbacks have ended.
    {
        std::lock_guard lock(driverMutex_);
        activeDevice_ = nullptr;
        cancelAsync_ = nullptr;
    }
    close(device);
    {
        std::lock_guard lock(stateMutex_);
        latest_.receiving = false;
        if (latest_.status.rfind("LIVE", 0) == 0)
            latest_.status = "Receiver stopped.";
    }
}

std::shared_ptr<RtlIqReceiver> sharedRtlIqReceiver() {
    static auto engine = std::make_shared<RtlIqReceiver>();
    return engine;
}

} // namespace signalforge::core
