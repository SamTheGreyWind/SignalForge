#pragma once

#include <QWidget>

namespace signalforge::ui {

// A stylized drawing for previewing map-layer controls.
// It deliberately uses fictional positions rather than geographic data.
class DemoMapWidget final : public QWidget {
public:
    explicit DemoMapWidget(bool showAircraft,
                           bool showAprs,
                           QWidget* parent = nullptr);
};

}  // namespace signalforge::ui
