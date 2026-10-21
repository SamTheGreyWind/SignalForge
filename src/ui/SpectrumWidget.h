#pragma once

#include <QImage>
#include <QTimer>
#include <QWidget>

#include <cstdint>
#include <functional>
#include <vector>

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
    // Visible display span is independent of VFO filter width. An anchor of
    // 0.5 zooms about the view center; Ctrl+wheel zooms at the pointer.
    void setSpanHz(qint64 span, double anchor = 0.5);
    [[nodiscard]] qint64 spanHz() const noexcept { return spanHz_; }
    void setSpanChangedHandler(std::function<void(qint64)> onChange);
    // Live IQ is fed from the GUI-independent shared receiver service.
    // Pass only new FFT frames; no hardware handles belong to this widget.
    void setLiveFrame(const std::vector<float>& bins, qint64 center,
                      qint64 sampleRate);
    void leaveLiveMode();
    [[nodiscard]] bool liveMode() const noexcept { return liveMode_; }
    void setRetuneHandler(std::function<void(qint64)> onRetune);

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
    static constexpr qint64 kMinSpanHz = 25'000;
    static constexpr qint64 kMaxSpanHz = 12'000'000;
    qint64 passbandHz_ = 180'000;
    int frame_ = 0;
    bool running_ = true;
    bool liveMode_ = false;
    qint64 liveCenterHz_ = 0;
    qint64 liveSampleRate_ = 0;
    std::vector<float> liveBins_;
    DragMode dragMode_ = DragMode::None;
    std::function<void(qint64)> bandwidthChanged_;
    std::function<void(qint64)> spanChanged_;
    std::function<void(qint64)> retuneRequested_;
};

}  // namespace signalforge::ui
