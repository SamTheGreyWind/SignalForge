#pragma once

#include <QImage>
#include <QTimer>
#include <QWidget>

#include <cstdint>
#include <functional>

class QMouseEvent;
class QPaintEvent;
class QWheelEvent;

namespace signalforge::ui {

// Simulation-only spectrum/waterfall. The VFO passband is a visible filter
// control, NOT a real demodulation filter or an occupied-bandwidth estimate.
class SpectrumWidget final : public QWidget {
public:
    explicit SpectrumWidget(QWidget* parent = nullptr);

    void setRunning(bool running);
    [[nodiscard]] bool isRunning() const noexcept { return running_; }
    void setPassbandHz(qint64 hz);
    [[nodiscard]] qint64 passbandHz() const noexcept { return passbandHz_; }
    void setBandwidthChangedHandler(std::function<void(qint64)> onChange);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    enum class DragMode { None, Tune, LeftEdge, RightEdge };
    static constexpr int kBins = 900;
    static constexpr int kRows = 280;

    [[nodiscard]] double powerAt(int bin) const;
    void addScanline();
    void tuneToPixel(double x);
    void panToFrequency(qint64 newCenter);
    void resizeEdge(double x);
    [[nodiscard]] double frequencyToX(qint64 hz) const;
    [[nodiscard]] QRect plotBounds() const;
    [[nodiscard]] QRect waterfallBounds() const;

    QTimer timer_;
    QImage history_;
    qint64 centerHz_ = 155'000'000;
    qint64 vfoHz_ = 155'000'000;
    qint64 spanHz_ = 2'400'000;
    qint64 passbandHz_ = 180'000;
    int frame_ = 0;
    bool running_ = true;
    DragMode dragMode_ = DragMode::None;
    std::function<void(qint64)> bandwidthChanged_;
};

}  // namespace signalforge::ui
