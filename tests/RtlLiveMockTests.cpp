#include "core/RtlIqReceiver.h"
#include "core/IqSpectrum.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <thread>

int main(int argc, char** argv) {
    if (argc != 2) return 2;
    auto receiver = std::make_shared<signalforge::core::RtlIqReceiver>();
    receiver->start(0, 100'000'000, argv[1]);
    bool gotFrame = false;
    for (int i = 0; i < 70; ++i) {
        auto state = receiver->snapshot();
        if (state.receiving &&
            state.fftFrames > 0 &&
            state.power.size() == signalforge::core::kRtlFftSize) {
            const auto peak = std::max_element(state.power.begin(),
                                                state.power.end()) -
                              state.power.begin();
            if (peak != 640) {
                std::cerr << "Incorrect FFT peak in mocked async IQ " << peak << '\n';
                return 1;
            }
            gotFrame = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    if (!gotFrame) {
        std::cerr << "Never received FFT frame from mock RTL USB stream: "
                  << receiver->snapshot().status << '\n';
        return 1;
    }
    receiver->retune(101'000'000);
    bool retuned = false;
    for (int i = 0; i < 70; ++i) {
        const auto state = receiver->snapshot();
        if (state.receiving && state.centerHz == 101'000'000 && !state.power.empty()) {
            retuned = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    receiver->stop();
    if (!retuned || receiver->snapshot().receiving) {
        std::cerr << "Retune or stop failed\n";
        return 1;
    }
    receiver->start(1, 102'000'000, argv[1]);
    receiver->stop();
    if (receiver->snapshot().receiving) return 1;
    std::cout << "Mock RTL async IQ, retune, stop, alternate device passed.\n";
    return 0;
}
