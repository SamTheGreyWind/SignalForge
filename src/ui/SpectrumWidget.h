#pragma once

#include <QImage>
#include <QTimer>
#include <QWidget>

class QMouseEvent;
class QPaintEvent;
class QWheelEvent;

namespace signalforge::ui {

// Interactive *synthetic* RF display. Generates no I/Q data and uses no SDR.
// The interface is deliberately a widget so a later DSP spectrum source can
// replace the demonstration sampler without changing the workspace manager.
class SpectrumWidget final : public QWidget {
public:
    explicit SpectrumWidget(QWidget* parent = nullptr);

    void setRunning(bool running);
    [[nodiscard]] bool isRunning() const noexcept { return running_; }

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    static constexpr int kBins = 600;
    static constexpr int kRows = 240;

    [[nodiscard]] double powerAt(int bin) const;
    void addScanline();
    void tuneToPixel(double x);
    void panToFrequency(qint64 newCenter);
    [[nodiscard]] QRect plotBounds() const;
    [[nodiscard]] QRect waterfallBounds() const;

    QTimer timer_;
    QImage history_;
    qint64 centerHz_ = 155'000'000;
    qint64 vfoHz_ = 155'000'000;
    qint64 spanHz_ = 2'400'000;
    int frame_ = 0;
    bool running_ = true;
};

}  // namespace signalforge::ui
