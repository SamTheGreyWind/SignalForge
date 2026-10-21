#include "ui/MainWindow.h"
#include "core/RtlDiscovery.h"

#include <QApplication>
#include <QCoreApplication>

#include <string_view>

int main(int argc, char* argv[]) {
    // Headless self-test for Windows preview packaging. It verifies that the
    // bundled native rtl-sdr DLL and any dependencies are loadable without
    // opening or retuning an actual SDR. Device count may legitimately be 0.
    if (argc == 2 && std::string_view(argv[1]) == "--rtl-driver-self-test") {
        const auto result = signalforge::core::enumerateRtlDevices();
        return result.driverLoaded ? 0 : 1;
    }
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("SignalForge");
    QCoreApplication::setApplicationName("SignalForge");
    QCoreApplication::setApplicationVersion("0.1.0");

    signalforge::ui::MainWindow window;
    window.show();
    return app.exec();
}
