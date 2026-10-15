#pragma once

#include <QByteArray>
#include <QMainWindow>
#include <QVector>

class QCloseEvent;
class QDockWidget;
class QString;

namespace signalforge::ui {

// The desktop is a view layer. No RF device is owned by this window.
class MainWindow final : public QMainWindow {
public:
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    QDockWidget* addPanel(const QString& id,
                          const QString& title,
                          const QString& description,
                          Qt::DockWidgetArea area);
    void createMenus();
    void saveSession() const;
    void restoreSession();
    void saveWorkspaceAs();
    void loadWorkspace();
    void resetWorkspace();

    QVector<QDockWidget*> panels_;
    QByteArray defaultState_;
    QByteArray defaultGeometry_;
};

}  // namespace signalforge::ui
