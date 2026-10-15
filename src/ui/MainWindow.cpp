#include "ui/MainWindow.h"

#include <QAction>
#include <QCloseEvent>
#include <QDockWidget>
#include <QFont>
#include <QInputDialog>
#include <QKeySequence>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QSettings>
#include <QStatusBar>
#include <QStringList>
#include <QVBoxLayout>
#include <QWidget>

namespace signalforge::ui {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent) {
    setObjectName("signalforge.main");
    setWindowTitle("SignalForge — The Open-Source Radio Multitool");
    resize(1360, 820);
    setDockOptions(QMainWindow::AllowNestedDocks |
                   QMainWindow::AllowTabbedDocks |
                   QMainWindow::AnimatedDocks);

    auto* workspace = new QWidget(this);
    auto* layout = new QVBoxLayout(workspace);
    layout->setContentsMargins(24, 24, 24, 24);

    auto* title = new QLabel("SignalForge", workspace);
    QFont titleFont = title->font();
    titleFont.setPointSize(22);
    titleFont.setBold(true);
    title->setFont(titleFont);

    auto* subtitle = new QLabel("Forged by Operators. Built for Everyone.", workspace);
    auto* spectrum = new QLabel(
        "Spectrum / Waterfall\n\nNo SDR connected.\n"
        "Live RF acquisition and the visual VFO are planned for a future milestone.",
        workspace);
    spectrum->setAlignment(Qt::AlignCenter);
    spectrum->setWordWrap(true);
    spectrum->setMinimumSize(480, 260);
    spectrum->setFrameShape(QFrame::StyledPanel);
    spectrum->setAccessibleName("Spectrum and waterfall placeholder");

    layout->addWidget(title);
    layout->addWidget(subtitle);
    layout->addWidget(spectrum, 1);
    setCentralWidget(workspace);

    addPanel("receivers", "Receivers",
             "Receiver management will enumerate SDR devices and independently assigned tasks.",
             Qt::LeftDockWidgetArea);
    addPanel("scanner", "Frequency Scanner",
             "Scan lists, range scans, and activity detection will appear here.",
             Qt::LeftDockWidgetArea);

    auto* p25 = addPanel("p25", "P25 Calls",
             "P25 control/traffic channels, talkgroups, and live calls are not implemented yet.",
             Qt::RightDockWidgetArea);
    auto* recordings = addPanel("recordings", "Recordings",
             "Future call archive, search, retention and instant playback.",
             Qt::RightDockWidgetArea);
    tabifyDockWidget(p25, recordings);
    p25->raise();

    auto* adsb = addPanel("adsb", "ADS-B Map",
             "Independent aircraft tracking map; no aircraft data source is connected.",
             Qt::BottomDockWidgetArea);
    auto* aprs = addPanel("aprs", "APRS Map",
             "Independent APRS station and weather map; no packet decoder is connected.",
             Qt::BottomDockWidgetArea);
    tabifyDockWidget(adsb, aprs);
    adsb->raise();

    createMenus();
    statusBar()->showMessage(
        "Application foundation — SDR reception, decoding and mapping are not yet implemented.");

    // Save a known default before restoring the previously used layout.
    defaultState_ = saveState(1);
    defaultGeometry_ = saveGeometry();
    restoreSession();
}

QDockWidget* MainWindow::addPanel(const QString& id,
                                 const QString& title,
                                 const QString& description,
                                 Qt::DockWidgetArea area) {
    auto* dock = new QDockWidget(title, this);
    dock->setObjectName("signalforge.panel." + id);
    dock->setAllowedAreas(Qt::AllDockWidgetAreas);
    dock->setFeatures(QDockWidget::DockWidgetClosable |
                      QDockWidget::DockWidgetMovable |
                      QDockWidget::DockWidgetFloatable);
    auto* label = new QLabel(description, dock);
    label->setWordWrap(true);
    label->setAlignment(Qt::AlignCenter);
    label->setMargin(16);
    label->setMinimumWidth(170);
    dock->setWidget(label);
    addDockWidget(area, dock);
    panels_.append(dock);
    return dock;
}

void MainWindow::createMenus() {
    auto* file = menuBar()->addMenu("&File");
    auto* quit = file->addAction("&Quit");
    quit->setShortcut(QKeySequence::Quit);
    connect(quit, &QAction::triggered, this, [this] { close(); });

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
            "SignalForge 0.1.0 — Application Foundation\n\n"
            "The Open-Source Radio Multitool\n"
            "Forged by Operators. Built for Everyone.\n\n"
            "This is an early desktop shell. SDR hardware and decoder "
            "backends have not been implemented.");
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
    QString name = QInputDialog::getText(
        this, "Save Workspace", "Workspace name:", QLineEdit::Normal,
        QString(), &accepted).trimmed();
    if (!accepted || name.isEmpty()) {
        return;
    }
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
    if (!accepted || name.isEmpty()) {
        return;
    }
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
    saveSession();
    QMainWindow::closeEvent(event);
}

}  // namespace signalforge::ui
