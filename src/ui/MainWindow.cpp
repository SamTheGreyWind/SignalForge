#include "ui/MainWindow.h"

#include "ui/DemoMapWidget.h"
#include "ui/SpectrumWidget.h"

#include <QAbstractItemView>
#include <QAction>
#include <QCheckBox>
#include <QCloseEvent>
#include <QDockWidget>
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
#include <QStatusBar>
#include <QStringList>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QWidget>

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

}  // namespace

MainWindow::MainWindow(bool primaryWindow, QWidget* parent)
    : QMainWindow(parent), primaryWindow_(primaryWindow) {
    setObjectName("signalforge.main");
    setWindowTitle(primaryWindow_
        ? "SignalForge — The Open-Source Radio Multitool"
        : "SignalForge — Additional Workspace (Demo)");
    resize(1360, 820);
    setDockOptions(QMainWindow::AllowNestedDocks |
                   QMainWindow::AllowTabbedDocks |
                   QMainWindow::AnimatedDocks);

    auto* workspace = new QWidget(this);
    auto* layout = new QVBoxLayout(workspace);
    layout->setContentsMargins(12, 12, 12, 12);

    auto* headline = new QHBoxLayout();
    auto* title = new QLabel("SignalForge", workspace);
    QFont titleFont = title->font();
    titleFont.setPointSize(19);
    titleFont.setBold(true);
    title->setFont(titleFont);
    headline->addWidget(title);

    headline->addStretch();
    auto* runDemo = new QCheckBox("Animate demo", workspace);
    runDemo->setChecked(true);
    headline->addWidget(runDemo);
    layout->addLayout(headline);

    auto* subtitle = new QLabel(
        "Forged by Operators. Built for Everyone.   |   "
        "Simulation only — click/drag the VFO; scroll to tune and pan.", workspace);
    subtitle->setWordWrap(true);
    layout->addWidget(subtitle);

    spectrum_ = new SpectrumWidget(workspace);
    layout->addWidget(spectrum_, 1);
    connect(runDemo, &QCheckBox::toggled,
            spectrum_, &SpectrumWidget::setRunning);
    setCentralWidget(workspace);

    addPanel("receivers", "Receivers",
             demoTable("Synthetic devices — no physical hardware connected.",
                 {"Receiver", "Center", "State"},
                 {{"SIM-01", "155.000", "Demo"},
                  {"SIM-02", "1090.000", "Demo"},
                  {"SIM-03", "144.390", "Demo"}}),
             Qt::LeftDockWidgetArea);

    addPanel("scanner", "Frequency Scanner",
             demoTable("Sample scan-list channels (MHz).",
                 {"Frequency", "Mode", "State"},
                 {{"154.480", "NFM", "Idle"},
                  {"154.840", "NFM", "Activity"},
                  {"155.320", "NFM", "Hold"},
                  {"155.770", "NFM", "Idle"}}),
             Qt::LeftDockWidgetArea);

    auto* p25 = addPanel("p25", "P25 Calls",
             demoTable("Fictional talkgroups. No P25 received or decoded.",
                 {"Time", "Talkgroup", "Status"},
                 {{"12:04:18", "DEMO-FIRE", "Complete"},
                  {"12:03:51", "DEMO-OPS", "Complete"},
                  {"12:03:16", "DEMO-EMS", "Complete"},
                  {"12:02:04", "DEMO-OPS", "Complete"}}),
             Qt::RightDockWidgetArea);

    auto* recordings = addPanel("recordings", "Recordings",
             demoTable("Illustrative archive only. Playback is not available.",
                 {"Time", "Source", "Length"},
                 {{"12:04:18", "DEMO-FIRE", "00:12"},
                  {"12:03:51", "DEMO-OPS", "00:08"},
                  {"12:03:16", "DEMO-EMS", "00:23"}}),
             Qt::RightDockWidgetArea);
    tabifyDockWidget(p25, recordings);
    p25->raise();

    auto* adsb = addPanel("adsb", "ADS-B Map",
             new DemoMapWidget(true, false),
             Qt::BottomDockWidgetArea);
    auto* aprs = addPanel("aprs", "APRS Map",
             new DemoMapWidget(false, true),
             Qt::BottomDockWidgetArea);
    tabifyDockWidget(adsb, aprs);
    adsb->raise();

    createMenus();
    statusBar()->showMessage(
        "Demo mode: synthetic RF, fictional calls and illustrated map positions. "
        "No SDR or GPS input.");

    defaultState_ = saveState(1);
    defaultGeometry_ = saveGeometry();
    if (primaryWindow_) restoreSession();
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
    // No parent: independently movable, closable top-level application window.
    // Closing the additional window frees it and does not close this workspace.
    auto* window = new MainWindow(false);
    window->setAttribute(Qt::WA_DeleteOnClose);
    window->move(pos() + QPoint(56, 56));
    window->show();
    window->raise();
}

void MainWindow::createMenus() {
    auto* file = menuBar()->addMenu("&File");
    auto* newWindow = file->addAction("&New Window");
    newWindow->setShortcut(QKeySequence::New);
    connect(newWindow, &QAction::triggered,
            this, [this] { openNewWindow(); });

    file->addSeparator();
    auto* close = file->addAction(primaryWindow_
        ? "&Quit SignalForge" : "&Close Window");
    close->setShortcut(primaryWindow_
        ? QKeySequence::Quit : QKeySequence::Close);
    connect(close, &QAction::triggered, this, [this] { close(); });

    auto* panelsMenu = menuBar()->addMenu("&Panels");
    for (auto* panel : panels_) {
        panelsMenu->addAction(panel->toggleViewAction());
    }

    auto* workspaces = menuBar()->addMenu("&Workspaces");
    auto* save = workspaces->addAction("&Save Layout As...");
    connect(save, &QAction::triggered, this, [this] { saveWorkspaceAs(); });
    auto* load = workspaces->addAction("&Load Layout...");
    connect(load, &QAction::triggered, this, [this] { loadWorkspace(); });
    workspaces->addSeparator();
    auto* reset = workspaces->addAction("&Reset to Default");
    connect(reset, &QAction::triggered, this, [this] { resetWorkspace(); });

    auto* help = menuBar()->addMenu("&Help");
    auto* about = help->addAction("&About SignalForge");
    connect(about, &QAction::triggered, this, [this] {
        QMessageBox::about(this, "About SignalForge",
            "SignalForge 0.1.0 — Interactive Demo\n\n"
            "The Open-Source Radio Multitool\n"
            "Forged by Operators. Built for Everyone.\n\n"
            "All displayed signals, maps, receiver lists and calls are "
            "simulated. SDR hardware and decoder backends are not implemented.");
    });
}

void MainWindow::saveSession() const {
    QSettings settings;
    settings.setValue("session/geometry", saveGeometry());
    settings.setValue("session/state", saveState(1));
}

void MainWindow::restoreSession() {
    QSettings settings;
    if (settings.contains("session/geometry")) {
        restoreGeometry(settings.value("session/geometry").toByteArray());
    }
    if (settings.contains("session/state")) {
        restoreState(settings.value("session/state").toByteArray(), 1);
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
    settings.setValue("state", saveState(1));
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

    if (!restoreState(state, 1)) {
        QMessageBox::warning(this, "Workspace",
                             "Could not restore this saved layout.");
        return;
    }
    restoreGeometry(geometry);
    statusBar()->showMessage("Loaded workspace: " + name, 4000);
}

void MainWindow::resetWorkspace() {
    restoreState(defaultState_, 1);
    restoreGeometry(defaultGeometry_);
    statusBar()->showMessage("Restored default workspace layout.", 4000);
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (primaryWindow_) saveSession();
    QMainWindow::closeEvent(event);
}

}  // namespace signalforge::ui
