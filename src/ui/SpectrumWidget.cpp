#include "ui/SpectrumWidget.h"

#include <QColor>
#include <QFont>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>
#include <cstring>

namespace signalforge::ui {
namespace {

double gaussian(double position, double center, double width) {
    const double d = (position - center) / width;
    return std::exp(-0.5 * d * d);
}

}  // namespace

SpectrumWidget::SpectrumWidget(QWidget* parent)
    : QWidget(parent),
      history_(kBins, kRows, QImage::Format_RGB32) {
    setMinimumSize(440, 300);
    setMouseTracking(true);
    setAccessibleName("Simulated RF spectrum and waterfall with interactive VFO");
    setToolTip("Synthetic signals only. Click or drag to tune the VFO; use the mouse wheel to pan the spectrum.");
    history_.fill(QColor(7, 17, 31));

    connect(&timer_, &QTimer::timeout, this, [this] {
        ++frame_;
        addScanline();
        update();
    });
    timer_.start(85);
    for (int i = 0; i < 48; ++i) {
        ++frame_;
        addScanline();
    }
}

void SpectrumWidget::setRunning(bool running) {
    running_ = running;
    if (running_) {
        timer_.start(85);
    } else {
        timer_.stop();
    }
    update();
}

double SpectrumWidget::powerAt(int bin) const {
    const double fraction = static_cast<double>(bin) / (kBins - 1);
    const double frequencyMHz =
        (static_cast<double>(centerHz_) +
         (fraction - 0.5) * static_cast<double>(spanHz_)) / 1'000'000.0;
    const double noise = 0.055
        + 0.015 * std::sin(bin * 0.63 + frame_ * 0.31)
        + 0.010 * std::sin(bin * 1.83 - frame_ * 0.13);
    const double signal =
        0.83 * gaussian(frequencyMHz, 154.48, 0.019) +
        0.62 * gaussian(frequencyMHz, 154.84, 0.032) +
        0.93 * gaussian(frequencyMHz, 155.32, 0.015) +
        0.73 * gaussian(frequencyMHz, 155.77, 0.045) +
        0.30 * gaussian(frequencyMHz,
                        155.05 + 0.08 * std::sin(frame_ * 0.015), 0.028);
    return std::clamp(noise + signal, 0.0, 1.0);
}

void SpectrumWidget::addScanline() {
    const auto bytes = static_cast<std::size_t>(history_.bytesPerLine())
                       * static_cast<std::size_t>(kRows - 1);
    std::memmove(history_.scanLine(1), history_.constScanLine(0), bytes);
    auto* row = reinterpret_cast<QRgb*>(history_.scanLine(0));

    for (int x = 0; x < kBins; ++x) {
        const double level = powerAt(x);
        // Dark blue -> cyan -> yellow -> orange (demo heat palette).
        const int hue = static_cast<int>(215.0 - level * 180.0);
        const int value = static_cast<int>(27.0 + level * 228.0);
        row[x] = QColor::fromHsv(hue, 220, value).rgb();
    }
}

QRect SpectrumWidget::plotBounds() const {
    const int top = 55;
    const int height = std::max(85, (this->height() - 85) / 3);
    return QRect(18, top, std::max(1, width() - 36), height);
}

QRect SpectrumWidget::waterfallBounds() const {
    const QRect spectrum = plotBounds();
    const int top = spectrum.bottom() + 9;
    return QRect(spectrum.left(), top, spectrum.width(),
                 std::max(1, height() - top - 28));
}

void SpectrumWidget::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), QColor(9, 19, 34));

    const QRect plot = plotBounds();
    const QRect water = waterfallBounds();

    QFont labelFont = font();
    labelFont.setPointSize(10);
    labelFont.setBold(true);
    painter.setFont(labelFont);
    painter.setPen(QColor(100, 235, 188));
    painter.drawText(18, 23, "DEMO MODE  -  SIMULATED RF / NO DEVICE CONNECTED");
    painter.setPen(QColor(219, 229, 242));
    painter.drawText(18, 43, QString("VFO: %1 MHz   |   Center: %2 MHz   |   Span: 2.40 MHz")
        .arg(static_cast<double>(vfoHz_) / 1'000'000, 0, 'f', 5)
        .arg(static_cast<double>(centerHz_) / 1'000'000, 0, 'f', 3));

    painter.fillRect(plot, QColor(13, 32, 53));
    painter.setPen(QPen(QColor(51, 83, 103), 1));
    for (int i = 1; i <= 4; ++i) {
        const int y = plot.top() + i * plot.height() / 5;
        painter.drawLine(plot.left(), y, plot.right(), y);
    }
    for (int i = 1; i <= 7; ++i) {
        const int x = plot.left() + i * plot.width() / 8;
        painter.drawLine(x, plot.top(), x, plot.bottom());
    }

    QPainterPath trace;
    for (int i = 0; i < kBins; ++i) {
        const double power = powerAt(i);
        const double x = plot.left() + plot.width() * i / (kBins - 1.0);
        const double y = plot.bottom() - power * (plot.height() - 4);
        if (i == 0) trace.moveTo(x, y);
        else trace.lineTo(x, y);
    }
    painter.setPen(QPen(QColor(99, 241, 179), 2));
    painter.drawPath(trace);

    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
    painter.drawImage(water, history_);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    const double fraction =
        0.5 + static_cast<double>(vfoHz_ - centerHz_) / spanHz_;
    if (fraction >= 0.0 && fraction <= 1.0) {
        const int x = plot.left() + static_cast<int>(fraction * plot.width());
        painter.setPen(QPen(QColor(255, 190, 62), 2));
        painter.drawLine(x, plot.top(), x, water.bottom());
        painter.setBrush(QColor(255, 190, 62));
        painter.setPen(Qt::NoPen);
        painter.drawPolygon(QPolygon{
            QPoint(x - 6, plot.top()), QPoint(x + 6, plot.top()),
            QPoint(x, plot.top() + 8)
        });
    }

    painter.setPen(QColor(170, 194, 211));
    for (int i = 0; i <= 4; ++i) {
        const double mhz = (static_cast<double>(centerHz_) +
            (i / 4.0 - 0.5) * spanHz_) / 1'000'000.0;
        const int x = plot.left() + i * plot.width() / 4;
        const QRect textRect(x - 42, water.bottom() + 3, 84, 19);
        painter.drawText(textRect, Qt::AlignCenter,
                         QString::number(mhz, 'f', 2));
    }
    if (!running_) {
        painter.fillRect(water, QColor(4, 12, 21, 120));
        painter.setPen(Qt::white);
        painter.drawText(water, Qt::AlignCenter, "DEMO PAUSED");
    }
}

void SpectrumWidget::tuneToPixel(double x) {
    const QRect plot = plotBounds();
    const double fraction = std::clamp(
        (x - plot.left()) / std::max(1, plot.width()), 0.0, 1.0);
    vfoHz_ = centerHz_ + static_cast<qint64>(
        (fraction - 0.5) * static_cast<double>(spanHz_));
    update();
}

void SpectrumWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton &&
        (plotBounds().contains(event->pos()) ||
         waterfallBounds().contains(event->pos()))) {
        tuneToPixel(event->position().x());
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void SpectrumWidget::mouseMoveEvent(QMouseEvent* event) {
    if (event->buttons() & Qt::LeftButton) {
        tuneToPixel(event->position().x());
        event->accept();
        return;
    }
    QWidget::mouseMoveEvent(event);
}

void SpectrumWidget::panToFrequency(qint64 newCenter) {
    if (newCenter < spanHz_ / 2) return;
    const qint64 previousCenter = centerHz_;
    centerHz_ = newCenter;
    // Slide existing mock history horizontally when tuning so operators
    // can explore how a center-locked waterfall might feel.
    const int shift = static_cast<int>(
        (static_cast<double>(previousCenter - newCenter) / spanHz_) * kBins);
    const QImage previous = history_;
    history_.fill(QColor(7, 17, 31));
    QPainter painter(&history_);
    painter.drawImage(shift, 0, previous);
    update();
}

void SpectrumWidget::wheelEvent(QWheelEvent* event) {
    const int steps = event->angleDelta().y() / 120;
    if (steps != 0) {
        vfoHz_ = std::max<qint64>(0, vfoHz_ + steps * 25'000LL);
        panToFrequency(vfoHz_);
        event->accept();
        return;
    }
    QWidget::wheelEvent(event);
}

}  // namespace signalforge::ui
