#pragma once

#include <QWidget>

class QLabel;
class QTableWidget;
class QLineEdit;

namespace signalforge::ui {

// Phase 1: read-only hardware discovery. Never opens, tunes or records from
// an SDR; the eventual per-device receiver sessions live outside UI widgets.
class ReceiverWidget final : public QWidget {
public:
    explicit ReceiverWidget(QWidget* parent = nullptr);

private:
    void chooseDriver();
    void refreshDevices();
    QTableWidget* table_ = nullptr;
    QLabel* status_ = nullptr;
    QLabel* warning_ = nullptr;
};

} // namespace signalforge::ui
