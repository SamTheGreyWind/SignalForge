#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace signalforge::core {

struct RtlIqSnapshot {
    bool receiving = false;
    std::string status = "Receiver stopped.";
    unsigned int deviceIndex = 0;
    std::uint32_t centerHz = 0;
    std::uint32_t sampleRate = 2'400'000;
    std::uint64_t fftFrames = 0;
    // Ordered from -sampleRate/2 through +sampleRate/2, normalized preview
    // levels. Not calibrated dBm.
    std::vector<float> power;
};

// First live RF slice: one active receive-only RTL2832U session, potentially
// observed by many view panels. Never owned by a dock/view widget.
// Multi-device simultaneous session management comes in a later increment.
class RtlIqReceiver final {
public:
    RtlIqReceiver() = default;
    ~RtlIqReceiver();
    RtlIqReceiver(const RtlIqReceiver&) = delete;
    RtlIqReceiver& operator=(const RtlIqReceiver&) = delete;

    void start(unsigned int index, std::uint32_t centerHz,
               const std::string& libraryPath = {});
    void stop();
    void retune(std::uint32_t centerHz);
    [[nodiscard]] RtlIqSnapshot snapshot() const;

private:
    void run(unsigned int index, std::uint32_t frequency,
             std::string libraryPath);
    void receiveBytes(const unsigned char* iq, std::uint32_t byteCount);
    static void onAsyncData(unsigned char* iq, std::uint32_t bytes, void* ctx);
    void updateStatus(std::string status, bool running);

    mutable std::mutex stateMutex_;
    RtlIqSnapshot latest_;
    std::mutex driverMutex_;
    void* activeDevice_ = nullptr;
    int (*cancelAsync_)(void*) = nullptr;
    std::thread worker_;
    std::atomic<bool> requestedRunning_{false};
    std::atomic<std::uint32_t> requestedCenterHz_{0};
    std::uint32_t appliedCenterHz_ = 0;
    std::uint64_t callbackCount_ = 0;
};

// Shared GUI-independent engine. Closing or floating any panel never stops RX.
// The engine stops when the application exits, or when the user presses Stop.
[[nodiscard]] std::shared_ptr<RtlIqReceiver> sharedRtlIqReceiver();

} // namespace signalforge::core
