#include "ui/MainWindow.h"

#include "ui/OfflineMapWidget.h"
#include "ui/SpectrumWidget.h"

#include <QAbstractItemView>
#include <QAction>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFont>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPoint>
#include <QSettings>
#include <QSignalBlocker>
#include <QStatusBar>
#include <QStringList>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

#include <cmath>
#include <memory>

namespace signalforge::ui {
namespace {

// Display-only sample data. Tables do not represent radio decoding or a
// persistence layer and should never be confused with live traffic.
QWidget* demoTable(const QString& caption,
                   const QStringList& headers,
                   const QList<QStringList>& rows) {
    auto* container = new QWidget;
    auto* layout = new QVBoxLayout(container);
    auto* note = new QLabel("DEMO DATA — NOT LIVE\n" + caption, container);
    note->setWordWrap(true);
    layout->addWidget(note);

    auto* table = new QTableWidget(rows.size(), headers.size(), container);
    table->setHorizontalHeaderLabels(headers);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setAlternatingRowColors(true);
    table->verticalHeader()->hide();
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->setShowGrid(false);
    for (int row = 0; row < rows.size(); ++row) {
        for (int column = 0; column < headers.size(); ++column) {
            const QString value = column < rows[row].size() ?
                rows[row][column] : QString();
            table->setItem(row, column, new QTableWidgetItem(value));
        }
    }
    layout->addWidget(table, 1);
    return container;
}



QWidget* spectrumPanel() {
    auto* panel = new QWidget;
    auto* layout = new QVBoxLayout(panel);
    layout->setContentsMargins(5, 5, 5, 5);

    auto* controls = new QHBoxLayout;
    auto* animate = new QCheckBox("Animate demo", panel);
    animate->setChecked(true);
    controls->addWidget(animate);
    controls->addWidget(new QLabel("Mode", panel));
    auto* modes = new QComboBox(panel);
    modes->addItem("WFM", 180.0);
    modes->addItem("NFM", 12.5);
    modes->addItem("AM", 9.0);
    modes->addItem("USB", 3.0);
    modes->addItem("LSB", 3.0);
    modes->addItem("CW", 1.0);
    modes->addItem("Custom", 0.0);
    controls->addWidget(modes);
    controls->addWidget(new QLabel("Filter", panel));
    auto* bandwidth = new QDoubleSpinBox(panel);
    bandwidth->setRange(1.0, 300.0);
    bandwidth->setSingleStep(2.5);
    bandwidth->setDecimals(1);
    bandwidth->setSuffix(" kHz");
    bandwidth->setValue(180.0);
    bandwidth->setToolTip("Simulated VFO demodulation filter width, not RF capture span");
    controls->addWidget(bandwidth);
    controls->addStretch();
    layout->addLayout(controls);

    auto* widget = new SpectrumWidget(panel);
    layout->addWidget(widget, 1);
    auto* hint = new QLabel(
        "DEMO: click/drag to tune; drag amber filter edges to adjust passband; "
        "scroll to move center frequency. No live RF.",
        panel);
    hint->setWordWrap(true);
    layout->addWidget(hint);
    QObject::connect(animate, &QCheckBox::toggled, widget,
                     &SpectrumWidget::setRunning);
    const auto applyingModePreset = std::make_shared<bool>(false);
    QObject::connect(modes, QOverload<int>::of(&QComboBox::currentIndexChanged),
                     panel, [modes, bandwidth, applyingModePreset](int index) {
        if (index < 0) return;
        const double preset = modes->itemData(index).toDouble();
        if (preset > 0) {
            // A mode preset is not a manual filter-width change.
            *applyingModePreset = true;
            bandwidth->setValue(preset);
            *applyingModePreset = false;
        }
    });
    QObject::connect(bandwidth, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                     panel, [widget, modes, bandwidth, applyingModePreset](double khz) {
        widget->setPassbandHz(static_cast<qint64>(std::llround(khz * 1000.0)));
        const double preset = modes->currentData().toDouble();
        if (!*applyingModePreset && std::abs(preset - bandwidth->value()) > 0.06)
            modes->setCurrentIndex(modes->count() - 1);
    });
    widget->setBandwidthChangedHandler([bandwidth, modes, applyingModePreset](qint64 hz) {
        const QSignalBlocker blocker(bandwidth);
        bandwidth->setValue(static_cast<double>(hz) / 1000.0);
        if (!*applyingModePreset)
            modes->setCurrentIndex(modes->count() - 1);
    });
    return panel;
}

}  // namespace

MainWindow::MainWindow(bool primaryWindow,
                       const QString& initialPanel,
                       QWidget* parent)
    : QMainWindow(parent), primaryWindow_(primaryWindow) {
    setObjectName("signalforge.main");
    setWindowTitle(primaryWindow_
        ? "SignalForge — The Open-Source Radio Multitool"
        : "SignalForge — Workspace");
    resize(1380, 840);
    setDockOptions(QMainWindow::AllowNestedDocks |
                   QMainWindow::AllowTabbedDocks |
                   QMainWindow::AnimatedDocks |
                   QMainWindow::GroupedDragging);

    // The central widget is deliberately NOT the waterfall: views can be
    // closed, floated or moved without leaving an immutable drawing behind.
    auto* central = new QWidget(this);
    auto* centerLayout = new QVBoxLayout(central);
    auto* heading = new QLabel("SignalForge", central);
    QFont headingFont = heading->font();
    headingFont.setPointSize(20);
    headingFont.setBold(true);
    heading->setFont(headingFont);
    heading->setAlignment(Qt::AlignCenter);
    auto* instructions = new QLabel(
        "Forged by Operators. Built for Everyone.\n\n"
        "Use Panels > Show/Hide (or the toolbar) to build this workspace.\n"
        "Drag panels to dock, float, resize, or move them to another monitor.",
        central);
    instructions->setAlignment(Qt::AlignCenter);
    instructions->setWordWrap(true);
    centerLayout->addStretch();
    centerLayout->addWidget(heading);
    centerLayout->addWidget(instructions);
    centerLayout->addStretch();
    setCentralWidget(central);

    // Each top-level window gets independent VIEW instances. Later RF engines
    // will be separate from all of these GUI widgets.
    addPanel("spectrum", "Spectrum / Waterfall", spectrumPanel(),
             Qt::LeftDockWidgetArea);
    addPanel("receivers", "Receivers",
             demoTable("Synthetic devices — no physical hardware connected.",
                 {"Receiver", "Center", "State"},
                 {{"SIM-01", "155.000", "Demo"},
                  {"SIM-02", "1090.000", "Demo"},
                  {"SIM-03", "144.390", "Demo"}}),
             Qt::RightDockWidgetArea);
    addPanel("scanner", "Frequency Scanner",
             demoTable("Sample scan-list channels (MHz).",
                 {"Frequency", "Mode", "State"},
                 {{"154.480", "NFM", "Idle"},
                  {"154.840", "NFM", "Activity"},
                  {"155.320", "NFM", "Hold"},
                  {"155.770", "NFM", "Idle"}}),
             Qt::RightDockWidgetArea);
    auto* p25 = addPanel("p25", "P25 Calls",
             demoTable("Fictional talkgroups. No P25 received or decoded.",
                 {"Time", "Talkgroup", "Status"},
                 {{"12:04:18", "DEMO-FIRE", "Complete"},
                  {"12:03:51", "DEMO-OPS", "Complete"},
                  {"12:03:16", "DEMO-EMS", "Complete"},
                  {"12:02:04", "DEMO-OPS", "Complete"}}),
             Qt::RightDockWidgetArea);
    auto* recordings = addPanel("recordings", "Recordings",
             demoTable("Illustrative archive only. Playback is unavailable.",
                 {"Time", "Source", "Length"},
                 {{"12:04:18", "DEMO-FIRE", "00:12"},
                  {"12:03:51", "DEMO-OPS", "00:08"},
                  {"12:03:16", "DEMO-EMS", "00:23"}}),
             Qt::RightDockWidgetArea);
    tabifyDockWidget(p25, recordings);
    p25->raise();

    auto* adsb = addPanel("adsb", "ADS-B Map",
                          new OfflineMapWidget(true, false),
                          Qt::BottomDockWidgetArea);
    auto* aprs = addPanel("aprs", "APRS Map",
                          new OfflineMapWidget(false, true),
                          Qt::BottomDockWidgetArea);
    tabifyDockWidget(adsb, aprs);
    adsb->raise();

    createMenus();

    if (!primaryWindow_) {
        for (auto* panel : panels_) {
            const bool selected = panel->objectName() ==
                                  "signalforge.panel." + initialPanel;
            panel->setVisible(selected);
        }
    }
    defaultState_ = saveState(2);
    defaultGeometry_ = saveGeometry();
    if (primaryWindow_) restoreSession();
    statusBar()->showMessage(
        "SIMULATION ONLY: all calls and markers are fictional. "
        "Real offline MBTiles supported. No SDR device connected.");
}

QDockWidget* MainWindow::addPanel(const QString& id,
                                 const QString& title,
                                 QWidget* contents,
                                 Qt::DockWidgetArea area) {
    auto* dock = new QDockWidget(title, this);
    dock->setObjectName("signalforge.panel." + id);
    dock->setAllowedAreas(Qt::AllDockWidgetAreas);
    dock->setFeatures(QDockWidget::DockWidgetClosable |
                      QDockWidget::DockWidgetMovable |
                      QDockWidget::DockWidgetFloatable);
    dock->setWidget(contents);
    addDockWidget(area, dock);
    panels_.append(dock);
    return dock;
}

void MainWindow::openNewWindow() {
    const QStringList names = {
        "Empty Workspace", "Spectrum / Waterfall",
        "Receivers", "Frequency Scanner", "P25 Calls", "Recordings",
        "ADS-B Map", "APRS Map"
    };
    const QStringList ids = {
        "", "spectrum", "receivers", "scanner", "p25", "recordings",
        "adsb", "aprs"
    };
    bool accepted = false;
    const QString choice = QInputDialog::getItem(
        this, "Create SignalForge Window", "Start this window with:",
        names, 0, false, &accepted);
    if (!accepted) return;
    const int index = names.indexOf(choice);
    if (index < 0) return;

    auto* window = new MainWindow(false, ids.at(index));
    window->setAttribute(Qt::WA_DeleteOnClose);
    window->move(pos() + QPoint(56, 56));
    window->show();
    window->raise();
}

void MainWindow::createMenus() {
    auto* file = menuBar()->addMenu("&File");
    auto* newWindow = file->addAction("&New Window...");
    newWindow->setShortcut(QKeySequence::New);
    connect(newWindow, &QAction::triggered, this,
            [this] { openNewWindow(); });
    file->addSeparator();
    auto* closeAction = file->addAction("&Close Window");
    closeAction->setShortcut(QKeySequence::Close);
    connect(closeAction, &QAction::triggered, this,
            [this] { close(); });

    auto* panelMenu = menuBar()->addMenu("&Panels");
    for (auto* dock : panels_) panelMenu->addAction(dock->toggleViewAction());

    // Duplicate the menu in a toolbar so an empty window has an obvious
    // way to select more panels without hiding the menu bar.
    auto* toolbar = new QToolBar("Workspace Toolbar", this);
    toolbar->setObjectName("signalforge.workspace.toolbar");
    toolbar->setMovable(false);
    auto* picker = new QToolButton(toolbar);
    picker->setText("Panels  ▾");
    picker->setPopupMode(QToolButton::InstantPopup);
    picker->setMenu(panelMenu);
    toolbar->addWidget(picker);
    addToolBar(Qt::TopToolBarArea, toolbar);

    auto* layouts = menuBar()->addMenu("&Workspaces");
    auto* save = layouts->addAction("&Save Layout As...");
    connect(save, &QAction::triggered, this,
            [this] { saveWorkspaceAs(); });
    auto* load = layouts->addAction("&Load Layout...");
    connect(load, &QAction::triggered, this,
            [this] { loadWorkspace(); });
    layouts->addSeparator();
    auto* reset = layouts->addAction("&Reset to Default");
    connect(reset, &QAction::triggered, this,
            [this] { resetWorkspace(); });

    auto* help = menuBar()->addMenu("&Help");
    auto* about = help->addAction("&About SignalForge");
    connect(about, &QAction::triggered, this, [this] {
        QMessageBox::about(this, "About SignalForge",
            "SignalForge 0.1.0 — Operator Interface Preview\n\n"
            "The Open-Source Radio Multitool\n"
            "Forged by Operators. Built for Everyone.\n\n"
            "Spectrum, waterfall, calls and station positions are simulated. "
            "User-supplied offline MBTiles basemaps are real. "
            "SDR hardware, receiver filters and protocol decoders "
            "are not implemented yet.");
    });
}

void MainWindow::saveSession() const {
    QSettings settings;
    settings.setValue("session/geometry", saveGeometry());
    settings.setValue("session/state", saveState(2));
}

void MainWindow::restoreSession() {
    QSettings settings;
    if (settings.contains("session/geometry")) {
        restoreGeometry(settings.value("session/geometry").toByteArray());
    }
    if (settings.contains("session/state")) {
        restoreState(settings.value("session/state").toByteArray(), 2);
    }
}

void MainWindow::saveWorkspaceAs() {
    bool accepted = false;
    const QString name = QInputDialog::getText(
        this, "Save Workspace", "Workspace name:", QLineEdit::Normal,
        QString(), &accepted).trimmed();
    if (!accepted || name.isEmpty()) return;

    if (name.contains('/') || name.contains('\\')) {
        QMessageBox::warning(this, "Invalid name",
                             "Workspace names cannot contain slashes.");
        return;
    }

    QSettings settings;
    settings.beginGroup("workspaces");
    settings.beginGroup(name);
    settings.setValue("geometry", saveGeometry());
    settings.setValue("state", saveState(2));
    settings.endGroup();
    settings.endGroup();
    statusBar()->showMessage("Saved workspace: " + name, 4000);
}

void MainWindow::loadWorkspace() {
    QSettings settings;
    settings.beginGroup("workspaces");
    const QStringList names = settings.childGroups();
    settings.endGroup();
    if (names.isEmpty()) {
        QMessageBox::information(this, "Workspaces",
                                 "No saved workspace layouts yet.");
        return;
    }

    bool accepted = false;
    const QString name = QInputDialog::getItem(
        this, "Load Workspace", "Workspace:", names, 0, false, &accepted);
    if (!accepted || name.isEmpty()) return;

    settings.beginGroup("workspaces");
    settings.beginGroup(name);
    const QByteArray geometry = settings.value("geometry").toByteArray();
    const QByteArray state = settings.value("state").toByteArray();
    settings.endGroup();
    settings.endGroup();

    if (!restoreState(state, 2)) {
        QMessageBox::warning(this, "Workspace",
                             "Could not restore this saved layout.");
        return;
    }
    restoreGeometry(geometry);
    statusBar()->showMessage("Loaded workspace: " + name, 4000);
}

void MainWindow::resetWorkspace() {
    restoreState(defaultState_, 2);
    restoreGeometry(defaultGeometry_);
    statusBar()->showMessage("Restored default workspace layout.", 4000);
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (primaryWindow_) saveSession();
    QMainWindow::closeEvent(event);
}

}  // namespace signalforge::ui
