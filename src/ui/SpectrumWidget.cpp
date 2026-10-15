#include "ui/SpectrumWidget.h"

#include <QColor>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QPolygonF>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iterator>
#include <utility>

namespace signalforge::ui {
namespace {

double hashedNoise(int bin, int frame) {
    std::uint32_t x = static_cast<std::uint32_t>(bin + 1) * 0x9e3779b9u;
    x ^= static_cast<std::uint32_t>(frame + 41) * 0x85ebca6bu;
    x ^= x >> 16;
    x *= 0x7feb352du;
    x ^= x >> 15;
    x *= 0x846ca68bu;
    x ^= x >> 16;
    return static_cast<double>(x & 0xffffu) / 65535.0;
}

double shoulder(double deltaHz, double halfBandwidth, double transitionHz) {
    const double excess = std::abs(deltaHz) - halfBandwidth;
    return std::clamp(0.5 - excess / transitionHz, 0.0, 1.0);
}

double carrier(double deltaHz, double halfBandwidth) {
    const double relative = std::abs(deltaHz) / halfBandwidth;
    if (relative > 1.0) return 0.0;
    return 1.0 - relative * 0.45;
}

QColor waterfallColor(double power) {
    struct Stop { double threshold; int r; int g; int b; };
    static constexpr Stop stops[] = {
        {0.00,  4, 15, 51},
        {0.14,  6, 55, 123},
        {0.32,  0, 166, 219},
        {0.51,  0, 242, 210},
        {0.68, 239, 245,  49},
        {0.83, 255, 128,  18},
        {1.00, 217,  10,   9}
    };
    power = std::clamp(power, 0.0, 1.0);
    for (std::size_t i = 1; i < std::size(stops); ++i) {
        const Stop& right = stops[i];
        if (power <= right.threshold) {
            const Stop& left = stops[i - 1];
            const double mix = (power - left.threshold) /
                               (right.threshold - left.threshold);
            return QColor(
                static_cast<int>(left.r + (right.r - left.r) * mix),
                static_cast<int>(left.g + (right.g - left.g) * mix),
                static_cast<int>(left.b + (right.b - left.b) * mix));
        }
    }
    return QColor(217, 10, 9);
}

} // namespace

SpectrumWidget::SpectrumWidget(QWidget* parent)
    : QWidget(parent), history_(kBins, kRows, QImage::Format_RGB32) {
    setMinimumSize(290, 185);
    setMouseTracking(true);
    setAccessibleName("Simulated FFT-style spectrum and waterfall with bandwidth-aware VFO");
    setToolTip("DEMO ONLY: click or drag the VFO to tune; drag orange filter edges "
               "to change bandwidth; scroll to pan in 25 kHz increments.");
    history_.fill(QColor(4, 15, 51));
    connect(&timer_, &QTimer::timeout, this, [this] {
        ++frame_;
        addScanline();
        update();
    });
    timer_.start(85);
    for (int i = 0; i < 50; ++i) { ++frame_; addScanline(); }
}

void SpectrumWidget::setRunning(bool running) {
    running_ = running;
    if (running_) timer_.start(85);
    else timer_.stop();
    update();
}

void SpectrumWidget::setPassbandHz(qint64 hz) {
    const qint64 limited = std::clamp<qint64>(hz, 2'500, 300'000);
    if (limited == passbandHz_) return;
    passbandHz_ = limited;
    if (bandwidthChanged_) bandwidthChanged_(passbandHz_);
    update();
}

void SpectrumWidget::setBandwidthChangedHandler(std::function<void(qint64)> onChange) {
    bandwidthChanged_ = std::move(onChange);
}

double SpectrumWidget::powerAt(int bin) const {
    const double fraction = (bin + 0.5) / static_cast<double>(kBins);
    const double hz = static_cast<double>(centerHz_) +
                      (fraction - 0.5) * spanHz_;
    const double coarse = hashedNoise(bin, frame_);
    const double fine = hashedNoise(bin * 9 + 13, frame_ / 3);
    double level = 0.11 + 0.09 * coarse + 0.035 * (fine - 0.5);

    // Distinct illustrative signal structures: narrow carriers, flat-topped
    // channels, a changing multi-carrier group, and bursty wideband activity.
    level += 0.53 * carrier(hz - 154'480'000.0, 6'000.0);
    level += 0.50 * shoulder(hz - 154'840'000.0, 65'000.0, 4'500.0)
             * (0.90 + 0.10 * hashedNoise(bin, frame_ / 8));
    for (int i = -4; i <= 4; ++i) {
        const double spacing = 22'500.0 * i;
        level += 0.74 * carrier(hz - (155'320'000.0 + spacing), 3'200.0)
                 * (0.88 + 0.12 * std::sin(frame_ * .031 + i));
    }
    const bool wideBurst = (frame_ / 36) % 5 != 0;
    level += (wideBurst ? 0.63 : 0.23) *
             shoulder(hz - 155'770'000.0, 48'000.0, 3'500.0)
             * (0.89 + 0.11 * hashedNoise(bin, frame_ / 3));
    level += 0.29 * carrier(
        hz - (155'050'000.0 + 90'000.0 * std::sin(frame_ * .016)), 4'800.0);

    return std::clamp(level, 0.0, 1.0);
}

void SpectrumWidget::addScanline() {
    const auto bytes = static_cast<std::size_t>(history_.bytesPerLine()) *
                       static_cast<std::size_t>(kRows - 1);
    std::memmove(history_.scanLine(1), history_.constScanLine(0), bytes);
    auto* row = reinterpret_cast<QRgb*>(history_.scanLine(0));
    for (int x = 0; x < kBins; ++x) {
        row[x] = waterfallColor(powerAt(x)).rgb();
    }
}

QRect SpectrumWidget::plotBounds() const {
    const int top = 60;
    const int available = std::max(1, height() - 82);
    const int height = std::clamp(available / 3, 64, available);
    return QRect(12, top, std::max(1, width() - 24), height);
}

QRect SpectrumWidget::waterfallBounds() const {
    const QRect spectrum = plotBounds();
    const int top = spectrum.bottom() + 7;
    return QRect(spectrum.left(), top, spectrum.width(),
                 std::max(1, height() - top - 23));
}

double SpectrumWidget::frequencyToX(qint64 hz) const {
    const QRect plot = plotBounds();
    return plot.left() + (0.5 + static_cast<double>(hz - centerHz_) /
                         static_cast<double>(spanHz_)) * plot.width();
}

void SpectrumWidget::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
    painter.fillRect(rect(), QColor(7, 13, 21));
    const QRect plot = plotBounds();
    const QRect water = waterfallBounds();

    painter.setPen(QColor(93, 219, 247));
    painter.drawText(12, 19, "SIMULATED FFT / WATERFALL  -  NO RECEIVER CONNECTED");
    painter.setPen(QColor(220, 235, 248));
    painter.drawText(12, 42,
        QString("VFO %1 MHz  |  Filter %2 kHz  |  Span %3 MHz")
            .arg(static_cast<double>(vfoHz_) / 1'000'000.0, 0, 'f', 5)
            .arg(static_cast<double>(passbandHz_) / 1'000.0, 0, 'f', 1)
            .arg(static_cast<double>(spanHz_) / 1'000'000.0, 0, 'f', 2));

    painter.fillRect(plot, QColor(5, 8, 12));
    painter.setPen(QPen(QColor(57, 67, 73), 1));
    for (int i = 1; i <= 4; ++i)
        painter.drawLine(plot.left(), plot.top() + i * plot.height() / 5,
                         plot.right(), plot.top() + i * plot.height() / 5);
    for (int i = 1; i <= 9; ++i)
        painter.drawLine(plot.left() + i * plot.width() / 10, plot.top(),
                         plot.left() + i * plot.width() / 10, plot.bottom());

    QPolygonF trace;
    trace.reserve(kBins + 2);
    for (int i = 0; i < kBins; ++i) {
        const double power = powerAt(i);
        const double x = plot.left() + (i + 0.5) * plot.width() / kBins;
        const double y = plot.bottom() - power * (plot.height() - 3);
        trace << QPointF(x, y);
    }
    QPolygonF fill = trace;
    fill << QPointF(plot.right(), plot.bottom())
         << QPointF(plot.left(), plot.bottom());
    painter.setBrush(QColor(25, 136, 160, 60));
    painter.setPen(Qt::NoPen);
    painter.drawPolygon(fill);
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(QColor(93, 220, 247), 1));
    painter.drawPolyline(trace);

    painter.drawImage(water, history_);

    const double centerX = frequencyToX(vfoHz_);
    const double halfPixelWidth = static_cast<double>(passbandHz_) /
                                  static_cast<double>(spanHz_) * plot.width() * .5;
    const double left = centerX - halfPixelWidth;
    const double right = centerX + halfPixelWidth;
    if (right >= plot.left() && left <= plot.right()) {
        const QRectF band(std::max(left, static_cast<double>(plot.left())),
                          plot.top(),
                          std::min(right, static_cast<double>(plot.right())) -
                          std::max(left, static_cast<double>(plot.left())),
                          water.bottom() - plot.top() + 1);
        if (band.width() > 0) painter.fillRect(band, QColor(255, 179, 42, 35));
        painter.setPen(QPen(QColor(255, 188, 64), 1));
        if (left >= plot.left() && left <= plot.right())
            painter.drawLine(QPointF(left, plot.top()), QPointF(left, water.bottom()));
        if (right >= plot.left() && right <= plot.right())
            painter.drawLine(QPointF(right, plot.top()), QPointF(right, water.bottom()));
        painter.setPen(QPen(QColor(255, 206, 90), 2));
        painter.drawLine(QPointF(centerX, plot.top()), QPointF(centerX, water.bottom()));
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(255, 206, 90));
        painter.drawPolygon(QPolygonF{
            QPointF(centerX - 6, plot.top()), QPointF(centerX + 6, plot.top()),
            QPointF(centerX, plot.top() + 9)
        });
        painter.setBrush(Qt::NoBrush);
    }

    painter.setPen(QColor(175, 194, 208));
    for (int i = 0; i <= 4; ++i) {
        const double mhz = (static_cast<double>(centerHz_) +
             (i / 4.0 - 0.5) * spanHz_) / 1'000'000.0;
        const int x = plot.left() + i * plot.width() / 4;
        painter.drawText(QRect(x - 40, water.bottom() + 3, 80, 18),
                         Qt::AlignCenter, QString::number(mhz, 'f', 2));
    }
    if (!running_) {
        painter.fillRect(water, QColor(0, 0, 0, 128));
        painter.setPen(Qt::white);
        painter.drawText(water, Qt::AlignCenter, "SIMULATION PAUSED");
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

void SpectrumWidget::resizeEdge(double x) {
    const double halfWidthHz = std::abs(x - frequencyToX(vfoHz_)) /
                               std::max(1, plotBounds().width()) * spanHz_;
    setPassbandHz(static_cast<qint64>(2 * halfWidthHz));
}

void SpectrumWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() != Qt::LeftButton ||
        !(plotBounds().contains(event->pos()) ||
          waterfallBounds().contains(event->pos()))) {
        QWidget::mousePressEvent(event);
        return;
    }
    const double x = event->position().x();
    const double centerX = frequencyToX(vfoHz_);
    const double halfWidth = static_cast<double>(passbandHz_) / spanHz_ *
                             plotBounds().width() * .5;
    dragMode_ = DragMode::Tune;
    if (halfWidth >= 9.0) {
        if (std::abs(x - (centerX - halfWidth)) <= 7.0)
            dragMode_ = DragMode::LeftEdge;
        else if (std::abs(x - (centerX + halfWidth)) <= 7.0)
            dragMode_ = DragMode::RightEdge;
    }
    if (dragMode_ == DragMode::Tune) tuneToPixel(x);
    event->accept();
}

void SpectrumWidget::mouseMoveEvent(QMouseEvent* event) {
    if ((event->buttons() & Qt::LeftButton) && dragMode_ != DragMode::None) {
        if (dragMode_ == DragMode::Tune) tuneToPixel(event->position().x());
        else resizeEdge(event->position().x());
        event->accept();
        return;
    }
    QWidget::mouseMoveEvent(event);
}

void SpectrumWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        dragMode_ = DragMode::None;
        event->accept();
        return;
    }
    QWidget::mouseReleaseEvent(event);
}

void SpectrumWidget::panToFrequency(qint64 newCenter) {
    if (newCenter < spanHz_ / 2) return;
    const qint64 previous = centerHz_;
    centerHz_ = newCenter;
    const int shift = static_cast<int>(
        static_cast<double>(previous - newCenter) / spanHz_ * kBins);
    const QImage existing = history_;
    history_.fill(QColor(4, 15, 51));
    QPainter painter(&history_);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
    painter.drawImage(shift, 0, existing);
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

} // namespace signalforge::ui
