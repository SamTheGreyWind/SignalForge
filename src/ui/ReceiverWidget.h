#pragma once
#include <QWidget>

class QLabel;
class QTableWidget;
class QDoubleSpinBox;

namespace signalforge::ui {

// A view/controller for a shared, independent receive-only RTL engine.
// Closing this dock never stops the active hardware session.
class ReceiverWidget final : public QWidget {
public:
    explicit ReceiverWidget(QWidget* parent = nullptr);

private:
    void chooseDriver();
    void refreshDevices();
    void startSelected();
    void refreshStatus();

    QTableWidget* table_ = nullptr;
    QDoubleSpinBox* frequency_ = nullptr;
    QLabel* status_ = nullptr;
    QLabel* streamingStatus_ = nullptr;
    QLabel* warning_ = nullptr;
};

} // namespace signalforge::ui
