#include "ui/ReceiverWidget.h"
#include "core/RtlDiscovery.h"
#include "core/RtlIqReceiver.h"

#include <QAbstractItemView>
#include <QFileDialog>
#include <QFileInfo>
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QSignalBlocker>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTimer>
#include <QVBoxLayout>

namespace signalforge::ui {

ReceiverWidget::ReceiverWidget(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    auto* heading = new QLabel(
        "RTL-SDR USB receiver • live I/Q spectrum (receive only)", this);
    heading->setWordWrap(true);
    layout->addWidget(heading);

    auto* toolbar = new QHBoxLayout;
    auto* refresh = new QPushButton("Scan USB Devices", this);
    auto* choose = new QPushButton("Select RTL-SDR Library…", this);
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

    auto* receiveControls = new QHBoxLayout;
    receiveControls->addWidget(new QLabel("Center", this));
    frequency_ = new QDoubleSpinBox(this);
    frequency_->setRange(24.0, 1700.0);
    frequency_->setDecimals(4);
    frequency_->setSingleStep(0.025);
    frequency_->setValue(100.0);
    frequency_->setSuffix(" MHz");
    frequency_->setToolTip("Center tuning for a real 2.4 MSPS RTL-SDR IQ capture");
    receiveControls->addWidget(frequency_);
    auto* start = new QPushButton("Start RX", this);
    auto* stop = new QPushButton("Stop RX", this);
    receiveControls->addWidget(start);
    receiveControls->addWidget(stop);
    layout->addLayout(receiveControls);

    streamingStatus_ = new QLabel("Receiver stopped.", this);
    streamingStatus_->setWordWrap(true);
    streamingStatus_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(streamingStatus_);

    status_ = new QLabel(this);
    status_->setWordWrap(true);
    status_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(status_);
    warning_ = new QLabel(
        "Live IQ only: 2.4 MSPS, automatic gain, no audio or decoder yet. "
        "Bias tee remains off. Closing this panel does not stop RX; press "
        "Stop RX first if you need to release the USB device.", this);
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
    connect(start, &QPushButton::clicked, this, [this] { startSelected(); });
    connect(stop, &QPushButton::clicked, this, [] {
        signalforge::core::sharedRtlIqReceiver()->stop();
    });
    // While a receiver is active, frequency controls retune via the core
    // worker thread rather than making USB calls on the GUI thread.
    connect(frequency_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, [](double mhz) {
        const auto engine = signalforge::core::sharedRtlIqReceiver();
        if (engine->snapshot().receiving) {
            engine->retune(static_cast<std::uint32_t>(mhz * 1'000'000.0));
        }
    });
    auto* poll = new QTimer(this);
    poll->setInterval(150);
    connect(poll, &QTimer::timeout, this, [this] { refreshStatus(); });
    poll->start();
    QTimer::singleShot(0, this, [this] { refreshDevices(); refreshStatus(); });
}

void ReceiverWidget::chooseDriver() {
#if defined(Q_OS_WIN)
    const QString filter = "RTL-SDR library (rtlsdr.dll librtlsdr.dll);;All files (*)";
#else
    const QString filter = "RTL-SDR shared library (*.so *.so.*);;All files (*)";
#endif
    const QString path = QFileDialog::getOpenFileName(
        this, "Choose RTL-SDR Library", {}, filter);
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
    if (table_->rowCount() > 0) table_->selectRow(0);
    QString text = QString::fromStdString(report.diagnostic);
    if (!library.isEmpty()) {
        text += "\nDriver: " + QFileInfo(library).absoluteFilePath();
    } else if (!report.driverLoaded) {
        text += "\nFor Windows, the preview normally includes x64 rtlsdr.dll. If missing, place a compatible DLL "
                "beside SignalForge.exe, or choose one using the button above.";
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


void ReceiverWidget::startSelected() {
    const int row = table_->currentRow();
    if (row < 0 || !table_->item(row, 0)) {
        streamingStatus_->setText("Select a receiver row before pressing Start RX.");
        return;
    }
    const unsigned index = table_->item(row, 0)->text().toUInt();
    const auto mhz = frequency_->value();
    const std::uint32_t freqHz = static_cast<std::uint32_t>(
        mhz * 1'000'000.0);
    const auto path = QSettings().value("receivers/rtlDriverPath")
        .toString().toUtf8().toStdString();
    signalforge::core::sharedRtlIqReceiver()->start(index, freqHz, path);
    refreshStatus();
}

void ReceiverWidget::refreshStatus() {
    const auto state = signalforge::core::sharedRtlIqReceiver()->snapshot();
    if (state.receiving && !frequency_->hasFocus()) {
        const QSignalBlocker blocker(frequency_);
        frequency_->setValue(static_cast<double>(state.centerHz) / 1'000'000.0);
    }
    streamingStatus_->setText(QString::fromStdString(state.status) +
        QString(" | FFT frames: %1").arg(state.fftFrames));
}

} // namespace signalforge::ui
