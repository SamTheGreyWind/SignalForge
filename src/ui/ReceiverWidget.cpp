#include "ui/ReceiverWidget.h"
#include "core/RtlDiscovery.h"

#include <QAbstractItemView>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTimer>
#include <QVBoxLayout>

namespace signalforge::ui {

ReceiverWidget::ReceiverWidget(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    auto* heading = new QLabel(
        "RTL-SDR hardware discovery (read-only) — no RF acquisition yet", this);
    heading->setWordWrap(true);
    layout->addWidget(heading);

    auto* toolbar = new QHBoxLayout;
    auto* refresh = new QPushButton("Scan USB Devices", this);
    auto* choose = new QPushButton("Select V4 Driver DLL…", this);
    auto* reset = new QPushButton("Use Installed Driver", this);
    toolbar->addWidget(refresh);
    toolbar->addWidget(choose);
    toolbar->addWidget(reset);
    layout->addLayout(toolbar);

    table_ = new QTableWidget(0, 5, this);
    table_->setHorizontalHeaderLabels(
        {"Index", "Driver Name", "USB Product", "USB Serial", "Identity"});
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setAlternatingRowColors(true);
    table_->verticalHeader()->hide();
    table_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    layout->addWidget(table_, 1);

    status_ = new QLabel(this);
    status_->setWordWrap(true);
    status_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(status_);
    warning_ = new QLabel(
        "RTL-SDR Blog V4 requires an updated compatible RTL-SDR driver. "
        "Discovery does NOT prove the DLL can tune the V4 correctly. "
        "Bias tee stays off; no hardware is opened by this panel.", this);
    warning_->setWordWrap(true);
    layout->addWidget(warning_);

    connect(refresh, &QPushButton::clicked, this,
            [this] { refreshDevices(); });
    connect(choose, &QPushButton::clicked, this,
            [this] { chooseDriver(); });
    connect(reset, &QPushButton::clicked, this, [this] {
        QSettings().remove("receivers/rtlDriverPath");
        refreshDevices();
    });
    QTimer::singleShot(0, this, [this] { refreshDevices(); });
}

void ReceiverWidget::chooseDriver() {
#if defined(Q_OS_WIN)
    const QString filter = "RTL-SDR library (rtlsdr.dll librtlsdr.dll);;All files (*)";
#else
    const QString filter = "RTL-SDR shared library (*.so *.so.*);;All files (*)";
#endif
    const QString path = QFileDialog::getOpenFileName(
        this, "Choose V4-Compatible RTL-SDR Library", {}, filter);
    if (path.isEmpty()) return;
    QSettings().setValue("receivers/rtlDriverPath", path);
    refreshDevices();
}

void ReceiverWidget::refreshDevices() {
    table_->setRowCount(0);
    const QString library = QSettings()
        .value("receivers/rtlDriverPath").toString();
    const auto report = signalforge::core::enumerateRtlDevices(
        library.toUtf8().toStdString());
    for (const auto& item : report.devices) {
        const int row = table_->rowCount();
        table_->insertRow(row);
        const QStringList values = {
            QString::number(item.deviceIndex),
            QString::fromUtf8(item.name.c_str()),
            QString::fromUtf8(item.product.c_str()),
            QString::fromUtf8(item.serial.c_str()),
            item.uniqueSerial ? "Unique USB serial" : "Missing/duplicate serial"
        };
        for (int col = 0; col < values.size(); ++col) {
            table_->setItem(row, col, new QTableWidgetItem(values[col]));
        }
    }
    QString text = QString::fromStdString(report.diagnostic);
    if (!library.isEmpty()) {
        text += "\nDriver: " + QFileInfo(library).absoluteFilePath();
    } else if (!report.driverLoaded) {
        text += "\nFor Windows, place a **V4-compatible x64 rtlsdr.dll** "
                "beside SignalForge.exe, or choose it using the button above.";
    }
    if (report.driverLoaded) {
        int ambiguous = 0;
        for (const auto& item : report.devices)
            ambiguous += !item.uniqueSerial;
        if (ambiguous) {
            text += "\nWarning: " + QString::number(ambiguous) +
                    " device(s) have missing/duplicate serials. "
                    "Assign distinct EEPROM serials before persistent "
                    "multi-device profiles; don't rely on changing USB indices.";
        }
    }
    status_->setText(text);
}

} // namespace signalforge::ui
