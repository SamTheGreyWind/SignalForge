#include "ui/OfflineMapWidget.h"
#include "core/WebMercator.h"

#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QSizePolicy>
#include <QSettings>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <QHash>
#include <QImage>
#include <QPointF>
#include <QStringList>
#include <QUuid>

#include <algorithm>
#include <cmath>

namespace signalforge::ui {
namespace {
using signalforge::core::MapPixel;
using signalforge::core::toWorldPixel;
using signalforge::core::worldSize;
using signalforge::core::normalizeLongitude;

constexpr double kPi = 3.14159265358979323846;
constexpr int kTile = signalforge::core::kMapTileSize;
constexpr int kMaxCachedTiles = 256;

double latitudeFromY(double y, int zoom) {
    const double n = kPi * (1.0 - 2.0 * y / worldSize(zoom));
    return std::atan(std::sinh(n)) * 180.0 / kPi;
}

int wrapTile(int x, int size) {
    return ((x % size) + size) % size;
}
}

class OfflineMapCanvas final : public QWidget {
public:
    explicit OfflineMapCanvas(bool aircraft, bool aprs, QWidget* parent = nullptr)
        : QWidget(parent),
          connectionName_(QUuid::createUuid().toString(QUuid::WithoutBraces)),
          database_(QSqlDatabase::addDatabase("QSQLITE", connectionName_)),
          aircraft_(aircraft),
          aprs_(aprs) {
        setMinimumSize(250, 185);
        setFocusPolicy(Qt::StrongFocus);
        setMouseTracking(true);
        setAccessibleName("Offline MBTiles geographic map with simulated position layers");
    }

    ~OfflineMapCanvas() override {
        cache_.clear();
        database_.close();
        database_ = QSqlDatabase();
        QSqlDatabase::removeDatabase(connectionName_);
    }

    void setAircraft(bool enabled) { aircraft_ = enabled; update(); }
    void setAprs(bool enabled) { aprs_ = enabled; update(); }

    bool openPackage(const QString& path, QString* failure) {
        database_.close();
        cache_.clear();
        hasMap_ = false;
        mapName_.clear();
        if (path.isEmpty()) { update(); return true; }

        const QFileInfo info(path);
        if (!info.isFile() || !info.isReadable()) {
            if (failure) *failure = "Map file is missing or unreadable.";
            update();
            return false;
        }
        database_.setConnectOptions("QSQLITE_OPEN_READONLY");
        database_.setDatabaseName(path);
        if (!database_.open()) {
            if (failure) *failure = database_.lastError().text();
            update();
            return false;
        }
        if (!database_.tables().contains("tiles")) {
            if (failure) *failure = "This file does not contain an MBTiles tiles table.";
            database_.close();
            update();
            return false;
        }

        QHash<QString, QString> meta;
        if (database_.tables().contains("metadata")) {
            QSqlQuery q(database_);
            if (q.exec("SELECT name, value FROM metadata")) {
                while (q.next())
                    meta.insert(q.value(0).toString(), q.value(1).toString());
            }
        }
        const QString format = meta.value("format").toLower();
        if (format == "pbf" || format == "mvt") {
            if (failure)
                *failure = "Vector MBTiles are not supported yet. Export raster PNG/JPEG tiles.";
            database_.close();
            update();
            return false;
        }

        QSqlQuery zoomQuery(database_);
        if (!zoomQuery.exec("SELECT MIN(zoom_level), MAX(zoom_level) FROM tiles") ||
            !zoomQuery.next() || zoomQuery.value(0).isNull()) {
            if (failure) *failure = "No readable map tiles were found.";
            database_.close();
            update();
            return false;
        }
        minZoom_ = std::clamp(zoomQuery.value(0).toInt(), 0, 19);
        maxZoom_ = std::clamp(zoomQuery.value(1).toInt(), minZoom_, 19);
        tileSchemeXyz_ = meta.value("scheme").compare("xyz", Qt::CaseInsensitive) == 0;

        double lat = 39.7392;
        double lon = -104.9903;
        int proposedZoom = std::clamp(11, minZoom_, maxZoom_);
        const QStringList center = meta.value("center").split(',');
        const QStringList bounds = meta.value("bounds").split(',');
        bool okLat = false;
        bool okLon = false;
        if (center.size() >= 2) {
            lon = center[0].toDouble(&okLon);
            lat = center[1].toDouble(&okLat);
            if (center.size() >= 3) {
                bool okZoom = false;
                const int z = center[2].toInt(&okZoom);
                if (okZoom) proposedZoom = std::clamp(z, minZoom_, maxZoom_);
            }
        }
        if (!(okLon && okLat) && bounds.size() == 4) {
            bool a = false, b = false, c = false, d = false;
            const double west = bounds[0].toDouble(&a);
            const double south = bounds[1].toDouble(&b);
            const double east = bounds[2].toDouble(&c);
            const double north = bounds[3].toDouble(&d);
            if (a && b && c && d) {
                lat = (south + north) / 2.0;
                lon = normalizeLongitude(west + normalizeLongitude(east - west) / 2.0);
            }
        }
        if (!std::isfinite(lat) || !std::isfinite(lon) ||
            lat < -90 || lat > 90) {
            lat = 39.7392;
            lon = -104.9903;
        }

        mapName_ = meta.value("name", info.completeBaseName());
        centerLatitude_ = lat;
        centerLongitude_ = normalizeLongitude(lon);
        zoom_ = proposedZoom;
        markerLatitude_ = centerLatitude_;
        markerLongitude_ = centerLongitude_;
        customPin_ = false;
        hasMap_ = true;
        update();
        return true;
    }

    void addTestPin(double latitude, double longitude) {
        customPinLatitude_ = latitude;
        customPinLongitude_ = normalizeLongitude(longitude);
        customPin_ = true;
        centerLatitude_ = latitude;
        centerLongitude_ = customPinLongitude_;
        update();
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.fillRect(rect(), QColor(13, 29, 45));
        painter.setRenderHint(QPainter::SmoothPixmapTransform, false);

        const double mapSize = worldSize(zoom_);
        const MapPixel center = toWorldPixel(centerLatitude_, centerLongitude_, zoom_);
        const int tilesPerAxis = 1 << zoom_;
        const double originX = center.x - width() / 2.0;
        const double originY = center.y - height() / 2.0;
        const int x0 = static_cast<int>(std::floor(originX / kTile));
        const int y0 = static_cast<int>(std::floor(originY / kTile));
        const int x1 = static_cast<int>(std::ceil((originX + width()) / kTile));
        const int y1 = static_cast<int>(std::ceil((originY + height()) / kTile));

        for (int y = y0; y <= y1; ++y) {
            if (y < 0 || y >= tilesPerAxis) continue;
            for (int x = x0; x <= x1; ++x) {
                const QRectF target(x * kTile - originX, y * kTile - originY,
                                    kTile, kTile);
                const QImage tile = hasMap_ ?
                    tileImage(zoom_, wrapTile(x, tilesPerAxis), y) : QImage();
                if (!tile.isNull()) {
                    painter.drawImage(target, tile);
                } else {
                    painter.fillRect(target, QColor(18, 42, 60));
                    painter.setPen(QPen(QColor(51, 80, 96), 1));
                    painter.drawRect(target);
                }
            }
        }

        if (!hasMap_) {
            painter.setPen(QColor(220, 233, 240));
            painter.drawText(rect().adjusted(14, 32, -14, -25),
                             Qt::AlignCenter | Qt::TextWordWrap,
                             "NO OFFLINE MAP LOADED\n\n"
                             "Choose 'Add MBTiles…' to use a georeferenced map "
                             "from your computer, USB drive, or external SSD.");
        }

        auto project = [&](double lat, double lon) {
            const MapPixel pixel = toWorldPixel(lat, lon, zoom_);
            double dx = pixel.x - center.x;
            if (dx > mapSize / 2.0) dx -= mapSize;
            if (dx < -mapSize / 2.0) dx += mapSize;
            return QPointF(width() * .5 + dx,
                           height() * .5 + pixel.y - center.y);
        };

        // Explicitly synthetic geographic coordinates relative to the
        // imported map center. Screen positions come from Web Mercator.
        const double delta = std::clamp(90.0 / (1 << zoom_), 0.001, 0.09);
        if (aircraft_) {
            const QPointF offsets[] = { {-.6, .32}, {.38, -.42}, {.7, .7} };
            for (int i = 0; i < 3; ++i) {
                const QPointF point = project(markerLatitude_ + offsets[i].y() * delta,
                    markerLongitude_ + offsets[i].x() * delta);
                if (!rect().contains(point.toPoint())) continue;
                painter.setPen(QPen(QColor(10, 48, 66), 2));
                painter.setBrush(QColor(91, 220, 255));
                painter.drawEllipse(point, 5, 5);
                painter.setPen(QColor(255, 255, 255));
                painter.drawText(point + QPointF(9, -9),
                                 QString("DEMO-A%1").arg(i + 1));
            }
        }
        if (aprs_) {
            const QPointF offsets[] = { {-.28, -.1}, {.15, .65}, {-.8, -.62} };
            for (int i = 0; i < 3; ++i) {
                const QPointF point = project(markerLatitude_ + offsets[i].y() * delta,
                    markerLongitude_ + offsets[i].x() * delta);
                if (!rect().contains(point.toPoint())) continue;
                painter.setPen(QPen(QColor(7, 43, 24), 2));
                painter.setBrush(QColor(88, 243, 146));
                painter.drawRect(QRectF(point.x() - 5, point.y() - 5, 10, 10));
                painter.setPen(Qt::white);
                painter.drawText(point + QPointF(9, -9),
                                 QString("DEMO-%1").arg(i + 1));
            }
        }
        if (customPin_) {
            const QPointF point = project(customPinLatitude_, customPinLongitude_);
            if (rect().contains(point.toPoint())) {
                painter.setBrush(QColor(255, 194, 60));
                painter.setPen(QPen(QColor(49, 34, 10), 2));
                painter.drawEllipse(point, 7, 7);
                painter.setPen(Qt::white);
                painter.drawText(point + QPointF(11, -11), "TEST PIN");
            }
        }
        painter.fillRect(QRect(0, 0, width(), 27), QColor(5, 16, 26, 223));
        painter.setPen(QColor(240, 230, 166));
        painter.setFont(QFont("Sans Serif", 9, QFont::Bold));
        painter.drawText(QRect(8, 2, width() - 16, 22), Qt::AlignVCenter,
                         "OFFLINE MAP  |  DEMO MARKERS ONLY");
        painter.fillRect(QRect(0, height() - 22, width(), 22),
                         QColor(5, 16, 26, 215));
        painter.setPen(QColor(240, 244, 246));
        painter.setFont(QFont("Sans Serif", 8));
        painter.drawText(QRect(7, height() - 20, width() - 14, 18),
                         Qt::AlignVCenter,
                         QString("Lat %1  Lon %2   Zoom %3  %4")
                           .arg(centerLatitude_, 0, 'f', 4)
                           .arg(centerLongitude_, 0, 'f', 4)
                           .arg(zoom_)
                           .arg(hasMap_ ? mapName_ : "No base map"));
    }

    void wheelEvent(QWheelEvent* event) override {
        if (!hasMap_) { event->accept(); return; }
        const int delta = event->angleDelta().y();
        if (delta) {
            zoom_ = std::clamp(zoom_ + (delta > 0 ? 1 : -1), minZoom_, maxZoom_);
            update();
        }
        event->accept();
    }

    void mousePressEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton) {
            dragging_ = true;
            lastMouse_ = event->position();
            setCursor(Qt::ClosedHandCursor);
            event->accept();
        }
    }

    void mouseMoveEvent(QMouseEvent* event) override {
        if (!dragging_) return;
        const QPointF difference = event->position() - lastMouse_;
        lastMouse_ = event->position();
        const MapPixel old = toWorldPixel(centerLatitude_, centerLongitude_, zoom_);
        const double size = worldSize(zoom_);
        const double x = std::fmod(old.x - difference.x() + size, size);
        const double y = std::clamp(old.y - difference.y(), 0.0, size);
        centerLongitude_ = normalizeLongitude(x / size * 360.0 - 180.0);
        centerLatitude_ = latitudeFromY(y, zoom_);
        update();
        event->accept();
    }

    void mouseReleaseEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton) {
            dragging_ = false;
            unsetCursor();
            event->accept();
        }
    }

private:
    QImage tileImage(int z, int x, int y) {
        const quint64 key = (quint64(z) << 42) |
                            (quint64(x) << 21) | quint64(y);
        const auto it = cache_.constFind(key);
        if (it != cache_.cend()) return it.value();
        QSqlQuery query(database_);
        if (!query.prepare(
            "SELECT tile_data FROM tiles WHERE zoom_level = ? "
            "AND tile_column = ? AND tile_row = ?")) return {};
        query.addBindValue(z);
        query.addBindValue(x);
        query.addBindValue(tileSchemeXyz_ ? y : (1 << z) - y - 1);
        QImage image;
        if (query.exec() && query.next())
            image = QImage::fromData(query.value(0).toByteArray());
        if (cache_.size() >= kMaxCachedTiles) cache_.clear();
        cache_.insert(key, image);
        return image;
    }

    QString connectionName_;
    QSqlDatabase database_;
    QHash<quint64, QImage> cache_;
    QString mapName_;
    int zoom_ = 7;
    int minZoom_ = 0;
    int maxZoom_ = 19;
    bool tileSchemeXyz_ = false;
    bool hasMap_ = false;
    bool aircraft_ = true;
    bool aprs_ = false;
    bool customPin_ = false;
    bool dragging_ = false;
    QPointF lastMouse_;
    double centerLatitude_ = 39.7392;
    double centerLongitude_ = -104.9903;
    double markerLatitude_ = 39.7392;
    double markerLongitude_ = -104.9903;
    double customPinLatitude_ = 0;
    double customPinLongitude_ = 0;
};

OfflineMapWidget::OfflineMapWidget(bool aircraft, bool aprs, QWidget* parent)
    : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(5, 5, 5, 5);
    auto* title = new QLabel("OFFLINE GIS  •  ADS-B/APRS positions are simulated", this);
    title->setWordWrap(true);
    layout->addWidget(title);

    packages_ = new QComboBox(this);
    packages_->setMinimumWidth(140);
    packages_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    auto* tools = new QHBoxLayout;
    auto* add = new QPushButton("Add MBTiles…", this);
    auto* remove = new QPushButton("Remove", this);
    tools->addWidget(packages_, 1);
    tools->addWidget(add);
    tools->addWidget(remove);
    layout->addLayout(tools);

    auto* layers = new QHBoxLayout;
    auto* aircraftToggle = new QCheckBox("ADS-B", this);
    auto* aprsToggle = new QCheckBox("APRS", this);
    aircraftToggle->setChecked(aircraft);
    aprsToggle->setChecked(aprs);
    auto* pinButton = new QPushButton("Test Lat/Lon Pin…", this);
    layers->addWidget(aircraftToggle);
    layers->addWidget(aprsToggle);
    layers->addStretch();
    layers->addWidget(pinButton);
    layout->addLayout(layers);

    canvas_ = new OfflineMapCanvas(aircraft, aprs, this);
    layout->addWidget(canvas_, 1);
    status_ = new QLabel("Map files remain on disk; no network connection is used.", this);
    status_->setWordWrap(true);
    layout->addWidget(status_);

    connect(aircraftToggle, &QCheckBox::toggled, canvas_,
            [this](bool value) { canvas_->setAircraft(value); });
    connect(aprsToggle, &QCheckBox::toggled, canvas_,
            [this](bool value) { canvas_->setAprs(value); });
    connect(add, &QPushButton::clicked, this,
            [this] { addPackage(); });
    connect(remove, &QPushButton::clicked, this,
            [this] { removePackage(); });
    connect(pinButton, &QPushButton::clicked, this,
            [this] { addTestPin(); });
    connect(packages_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) { selectPackage(index); });

    refreshPackages(QSettings().value("maps/lastSelected").toString());
}

void OfflineMapWidget::refreshPackages(const QString& selected) {
    QSettings settings;
    QStringList paths = settings.value("maps/packages").toStringList();
    // The preview archive ships with a local public-domain demo. It is
    // discovered automatically, never copied into user settings or storage.
    const QString bundled = QCoreApplication::applicationDirPath() +
        "/maps/SignalForge_Offline_GIS_Sample.mbtiles";
    const bool haveBundled = QFileInfo::exists(bundled);
    if (haveBundled && !paths.contains(bundled)) paths.prepend(bundled);
    packages_->blockSignals(true);
    packages_->clear();
    packages_->addItem("(No offline base map)", QString());
    for (const QString& path : paths) {
        const QString label = (haveBundled && path == bundled)
            ? "Bundled sample — Natural Earth (offline)"
            : QFileInfo(path).fileName();
        packages_->addItem(label, path);
    }
    int index = packages_->findData(selected);
    if (index < 0) index = 0;
    // Auto-open bundled sample on FIRST use, not when an operator has
    // explicitly chosen the empty map or an imported external package.
    if (!settings.contains("maps/lastSelected") && haveBundled)
        index = packages_->findData(bundled);
    packages_->setCurrentIndex(index);
    packages_->blockSignals(false);
    selectPackage(index);
}

void OfflineMapWidget::selectPackage(int index) {
    if (index < 0) return;
    const QString path = packages_->itemData(index).toString();
    QSettings().setValue("maps/lastSelected", path);
    QString failure;
    if (canvas_->openPackage(path, &failure)) {
        status_->setText(path.isEmpty()
            ? "Select an offline MBTiles file. No external map service is contacted."
            : "Opened locally: " + QDir::toNativeSeparators(path) +
              "  |  Scroll to zoom; drag to pan.");
    } else {
        status_->setText("Cannot load map: " + failure);
    }
}

void OfflineMapWidget::addPackage() {
    const QString path = QFileDialog::getOpenFileName(
        this, "Open Offline Raster MBTiles", QString(),
        "MBTiles maps (*.mbtiles);;All files (*)");
    if (path.isEmpty()) return;
    QSettings settings;
    QStringList paths = settings.value("maps/packages").toStringList();
    if (!paths.contains(path)) {
        paths.append(path);
        settings.setValue("maps/packages", paths);
    }
    refreshPackages(path);
}

void OfflineMapWidget::removePackage() {
    const QString path = packages_->currentData().toString();
    if (path.isEmpty()) return;
    const QString bundled = QCoreApplication::applicationDirPath() +
        "/maps/SignalForge_Offline_GIS_Sample.mbtiles";
    if (path == bundled && QFileInfo::exists(bundled)) {
        status_->setText("The bundled map is part of the preview. "
                         "Select 'No offline base map' to hide it.");
        return;
    }
    QSettings settings;
    QStringList paths = settings.value("maps/packages").toStringList();
    paths.removeAll(path);
    settings.setValue("maps/packages", paths);
    refreshPackages();
}

void OfflineMapWidget::addTestPin() {
    bool accepted = false;
    const double lat = QInputDialog::getDouble(
        this, "Test Geographic Coordinate", "Latitude (-85 to +85):",
        39.7392, -85.0, 85.0, 6, &accepted);
    if (!accepted) return;
    const double lon = QInputDialog::getDouble(
        this, "Test Geographic Coordinate", "Longitude (-180 to +180):",
        -104.9903, -180.0, 180.0, 6, &accepted);
    if (!accepted) return;
    canvas_->addTestPin(lat, lon);
    status_->setText(QString("Test coordinate: %1°, %2°  (no external data)")
        .arg(lat, 0, 'f', 6).arg(lon, 0, 'f', 6));
}

}  // namespace signalforge::ui
