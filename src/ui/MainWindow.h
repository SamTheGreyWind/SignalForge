#pragma once

#include <QByteArray>
#include <QMainWindow>
#include <QString>
#include <QVector>

class QCloseEvent;
class QDockWidget;
class QWidget;

namespace signalforge::ui {

// A workspace is a collection of views; no radio hardware belongs to a window.
// Each window has an independent set of dock panels and can start empty.
class MainWindow final : public QMainWindow {
public:
    explicit MainWindow(bool primaryWindow = true,
                        const QString& initialPanel = {},
                        QWidget* parent = nullptr);

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

    bool primaryWindow_ = true;
    QVector<QDockWidget*> panels_;
    QByteArray defaultState_;
    QByteArray defaultGeometry_;
};

}  // namespace signalforge::ui
