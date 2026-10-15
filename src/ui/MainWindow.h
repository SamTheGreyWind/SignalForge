#pragma once

#include <QByteArray>
#include <QMainWindow>
#include <QVector>

class QCloseEvent;
class QDockWidget;
class QString;
class QWidget;

namespace signalforge::ui {

class SpectrumWidget;

// A window displays views but never owns an actual RF receiver.
// Only the primary window stores the "last session" layout.
class MainWindow final : public QMainWindow {
public:
    explicit MainWindow(bool primaryWindow = true, QWidget* parent = nullptr);

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    QDockWidget* addPanel(const QString& id,
                          const QString& title,
                          QWidget* contents,
                          Qt::DockWidgetArea area);
    void createMenus();
    void openNewWindow();
    void saveSession() const;
    void restoreSession();
    void saveWorkspaceAs();
    void loadWorkspace();
    void resetWorkspace();

    bool primaryWindow_;
    SpectrumWidget* spectrum_ = nullptr;
    QVector<QDockWidget*> panels_;
    QByteArray defaultState_;
    QByteArray defaultGeometry_;
};

}  // namespace signalforge::ui
