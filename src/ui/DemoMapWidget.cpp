#include "ui/DemoMapWidget.h"

#include <QCheckBox>
#include <QColor>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPolygonF>
#include <QVBoxLayout>

namespace signalforge::ui {
namespace {

class DemoMapCanvas final : public QWidget {
public:
    explicit DemoMapCanvas(bool aircraft, bool aprs, QWidget* parent = nullptr)
        : QWidget(parent), aircraft_(aircraft), aprs_(aprs) {
        setMinimumSize(230, 155);
        setAccessibleName("Illustrative map grid with fictional demo aircraft and APRS stations");
    }

    void showAircraft(bool value) { aircraft_ = value; update(); }
    void showAprs(bool value) { aprs_ = value; update(); }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.fillRect(rect(), QColor(15, 34, 47));

        p.setPen(QPen(QColor(48, 79, 93), 1));
        for (int x = 0; x < width(); x += 32) p.drawLine(x, 0, x, height());
        for (int y = 0; y < height(); y += 32) p.drawLine(0, y, width(), y);
        p.setPen(QPen(QColor(55, 100, 91), 2));
        p.drawPolyline(QPolygonF{
            QPointF(0, height() * .25), QPointF(width() * .2, height() * .45),
            QPointF(width() * .5, height() * .30),
            QPointF(width(), height() * .52)
        });
        p.drawPolyline(QPolygonF{
            QPointF(width() * .12, 0), QPointF(width() * .34, height() * .22),
            QPointF(width() * .42, height()), QPointF(width() * .75, height())
        });

        // Intentionally fixed *screen-space* positions; not actual geography.
        if (aircraft_) {
            const QPointF planes[] = {
                QPointF(width() * .18, height() * .24),
                QPointF(width() * .66, height() * .45),
                QPointF(width() * .81, height() * .73)
            };
            const int headings[] = { -20, 45, -70 };
            const char* names[] = { "TEST-A1", "TEST-A2", "TEST-A3" };
            p.setFont(QFont("Sans Serif", 8));
            for (int i = 0; i < 3; ++i) {
                p.save();
                p.translate(planes[i]);
                p.rotate(headings[i]);
                p.setPen(Qt::NoPen);
                p.setBrush(QColor(87, 204, 255));
                p.drawPolygon(QPolygonF{
                    QPointF(0, -10), QPointF(4, 5), QPointF(0, 2),
                    QPointF(-4, 5)
                });
                p.restore();
                p.setPen(QColor(173, 225, 244));
                p.drawText(planes[i] + QPointF(8, -7), names[i]);
            }
        }

        if (aprs_) {
            const QPointF sites[] = {
                QPointF(width() * .31, height() * .77),
                QPointF(width() * .52, height() * .19),
                QPointF(width() * .86, height() * .33)
            };
            const char* names[] = { "DEMO-1", "WX-DEMO", "DEMO-3" };
            p.setFont(QFont("Sans Serif", 8));
            for (int i = 0; i < 3; ++i) {
                p.setPen(QPen(QColor(98, 230, 137), 2));
                p.setBrush(QColor(20, 71, 53));
                p.drawEllipse(sites[i], 5, 5);
                p.setPen(QColor(175, 240, 188));
                p.drawText(sites[i] + QPointF(8, -7), names[i]);
            }
        }

        if (!aircraft_ && !aprs_) {
            p.setPen(Qt::white);
            p.drawText(rect(), Qt::AlignCenter, "No demo layers selected");
        }
        p.setPen(QColor(218, 204, 136));
        p.setFont(QFont("Sans Serif", 8));
        p.drawText(QRect(8, height() - 24, width() - 16, 18),
                   Qt::AlignLeft | Qt::AlignVCenter,
                   "ILLUSTRATIVE GRID - NOT A REAL MAP");
    }

private:
    bool aircraft_;
    bool aprs_;
};

}  // namespace

DemoMapWidget::DemoMapWidget(bool showAircraft, bool showAprs, QWidget* parent)
    : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    auto* banner = new QLabel("SIMULATED POSITIONS - NO LIVE RF OR GPS", this);
    banner->setWordWrap(true);
    layout->addWidget(banner);

    auto* toggles = new QHBoxLayout();
    auto* aircraft = new QCheckBox("ADS-B", this);
    auto* aprs = new QCheckBox("APRS", this);
    aircraft->setChecked(showAircraft);
    aprs->setChecked(showAprs);
    toggles->addWidget(aircraft);
    toggles->addWidget(aprs);
    toggles->addStretch();
    layout->addLayout(toggles);

    auto* canvas = new DemoMapCanvas(showAircraft, showAprs, this);
    layout->addWidget(canvas, 1);
    connect(aircraft, &QCheckBox::toggled,
            canvas, [canvas](bool checked) { canvas->showAircraft(checked); });
    connect(aprs, &QCheckBox::toggled,
            canvas, [canvas](bool checked) { canvas->showAprs(checked); });
}

}  // namespace signalforge::ui
