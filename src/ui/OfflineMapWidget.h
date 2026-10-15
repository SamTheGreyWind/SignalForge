#pragma once

#include <QWidget>

class QComboBox;
class QLabel;
class QString;

namespace signalforge::ui {

class OfflineMapCanvas;

// Offline raster MBTiles viewer. Map selection persists across sessions,
// but the map files are read from their original paths (including USB/SSD).
// ADS-B/APRS markers remain simulated until decoders are connected.
class OfflineMapWidget final : public QWidget {
public:
    explicit OfflineMapWidget(bool aircraft,
                              bool aprs,
                              QWidget* parent = nullptr);

private:
    void refreshPackages(const QString& selected = {});
    void selectPackage(int index);
    void addPackage();
    void removePackage();
    void addTestPin();

    QComboBox* packages_ = nullptr;
    QLabel* status_ = nullptr;
    OfflineMapCanvas* canvas_ = nullptr;
};

}  // namespace signalforge::ui
