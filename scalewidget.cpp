#include "scalewidget.h"

#include "mainwindow.h"
#include "sessionrecording.h"
#include <QDoubleValidator>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QResizeEvent>
#include <QSlider>
#include <QVBoxLayout>

namespace {
    constexpr const int MaximumValue = 1000;
} // namespace

ScaleItem::ScaleItem(KeyframeCamera* kf, SessionRecording* recording, QColor color, 
                     double size)
    : _keyframe(kf)
    , _recording(recording)
    , _color(color)
    , _size(size)
{
    setFlags(QGraphicsItem::ItemIgnoresTransformations);
}

QRectF ScaleItem::boundingRect() const {
    return QRectF(-_size/2.0, -_size/2.0, _size, _size);

    //qreal penWidth = 1;
    //return QRectF(-10 - penWidth / 2, -10 - penWidth / 2,
    //    20 + penWidth, 20 + penWidth);
}

void ScaleItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
                      QWidget* widget)
{
    if (_picked) {
        painter->setPen(Qt::red);
        painter->setBrush(Qt::red);
    }
    else {
        painter->setPen(_color);
        painter->setBrush(_color);
    }
    //painter->drawPoint(0, 0);
    painter->drawRoundedRect(-_size/2.0, -_size/2.0, _size, _size, 5, 5);

}

//////////////////////////////////////////////////////////////////////////////////////////

ScaleView::ScaleView(QGraphicsScene* scene, QWidget* parent, ScaleWidget* scale,
                     SessionRecording* recording)
    : QGraphicsView(scene, parent)
    , _parent(scale)
    , _recording(recording)
{
    setMouseTracking(true);
}

void ScaleView::mouseMoveEvent(QMouseEvent* event) {
    assert(
        std::is_sorted(
            _parent->_items.begin(), _parent->_items.end(),
            [](ScaleItem* lhs, ScaleItem* rhs) {
                return lhs->scenePos().x() < rhs->scenePos().x();
            }
        )
    );

    QPointF pt = event->pos();
    QPointF scenePt = mapToScene(pt.x(), pt.y());

    if (_pickedItem) {
        // We don't want the x coordinate to change
        scenePt.rx() = _pickedItem->scenePos().x();

        // Update item position
        _pickedItem->setPos(scenePt);

        // Update connected lines
        if (_pickedItem->_leftLine) {
            QGraphicsLineItem* left = _pickedItem->_leftLine;
            QLineF leftLineOld = left->line();
            QLineF leftLineNew = QLineF(leftLineOld.p1(), scenePt);
            _pickedItem->_leftLine->setLine(leftLineNew);
        }

        if (_pickedItem->_rightLine) {
            QGraphicsLineItem* right = _pickedItem->_rightLine;
            QLineF rightLineOld = right->line();
            QLineF rightLineNew = QLineF(scenePt, rightLineOld.p2());
            _pickedItem->_rightLine->setLine(rightLineNew);
        }
    }
    else {
        if (_recording) {
            double length = _recording->recordingLength;
            std::pair<double, double> minMax = _recording->minMaxScale;

            double x = scenePt.x() * length;
            double y = minMax.first + scenePt.y() * (minMax.second - minMax.first);

            _parent->_hoverInfo->setText(QString::number(x) + ", " + QString::number(y));
        }
    }
}

void ScaleView::mouseDoubleClickEvent(QMouseEvent* event) {
    QPointF pt = mapToScene(event->pos());

    ScaleItem* prev = nullptr;
    ScaleItem* next = nullptr;
    for (ScaleItem* c : _parent->_items) {
        if (c->scenePos().x() > pt.x()) {
            next = c;
            prev = next->_leftNeighbor;
            break;
        }
    }

    assert(prev && next);

    ScaleInfo s;
    for (ScaleInfo si : _parent->_recording->originalNormalizedScale) {
        if (si.x >= pt.x()) {
            s = si;
            break;
        }
    }
    
    QPen pen;
    pen.setColor(Qt::black);
    pen.setWidthF(0.0025f);
    QGraphicsLineItem* line = scene()->addLine(QLineF(pt, next->scenePos()), pen);
    line->setZValue(0);

    ScaleItem* item = new ScaleItem(s.kf, _recording, Qt::white, 5.0);
    item->setPos(s.x, pt.y());
    item->setZValue(1);
    scene()->addItem(item);
    _parent->_items.push_back(item);
    std::sort(
        _parent->_items.begin(), _parent->_items.end(),
        [](ScaleItem* lhs, ScaleItem* rhs) { return lhs->scenePos().x() < rhs->scenePos().x(); }
    );


    QLineF newLineLeft = QLineF(prev->scenePos(), pt);
    prev->_rightLine->setLine(newLineLeft);
    item->_leftLine = prev->_rightLine;

    item->_rightLine = line;
    next->_leftLine = line;

    item->_leftNeighbor = prev;
    prev->_rightNeighbor = item;
    item->_rightNeighbor = next;
    next->_leftNeighbor = item;
}

void ScaleView::mousePressEvent(QMouseEvent* event) {
    QPointF pt = mapToScene(event->pos());

    // Mark the previous one (if it exists) as unpicked
    if (_pickedItem) _pickedItem->_picked = false;

    for (size_t idx = 0; idx < _parent->_items.size(); idx += 1) {
        ScaleItem* i = _parent->_items[idx];
        QPointF p = i->scenePos();
        float distance = (pt - p).manhattanLength();
        if (distance < 0.0075) {
            if (event->button() == Qt::MouseButton::LeftButton) {
                // Select it
                _pickedItem = i;
                _pickedItem->_picked = true;
            }
            else if (event->button() == Qt::MouseButton::RightButton) {
#if 0
                // We dont' want to delete the border points
                if (idx == 0 || idx == _parent->_items.size() - 1)  continue;

                _parent->_items.erase(std::find(_parent->_items.begin(), _parent->_items.end(), i));
                
                i->_leftNeighbor->_rightNeighbor = i->_rightNeighbor;
                i->_rightNeighbor->_leftNeighbor = i->_leftNeighbor;

                QLineF newLine = QLineF(i->_leftLine->line().p1(), i->_rightLine->line().p2());
                i->_leftLine->setLine(newLine);

                i->_rightNeighbor->_leftLine = i->_leftNeighbor->_rightLine;

                delete i->_rightLine;
                delete i;
                
                if (idx > 0)  idx -= 1;
#endif
            }
            scene()->update(sceneRect());
            return;
        }
    }

    // Because of the early return, if we get here, we didn't pick anything
    _pickedItem = nullptr;
    scene()->update(sceneRect());
}

void ScaleView::mouseReleaseEvent(QMouseEvent* event) {
    if (_pickedItem)  _pickedItem->_picked = false;
    _pickedItem = nullptr;
    scene()->update(sceneRect());
}


//////////////////////////////////////////////////////////////////////////////////////////

ScaleWidget::ScaleWidget(MainWindow* mainWindow, QWidget* parent)
    : QWidget(parent)
    , _mainWindow(mainWindow)
{
    QBoxLayout* layout = new QVBoxLayout(this);

    _scene = new QGraphicsScene(this);
    _scene->setBackgroundBrush(Qt::darkGray);
    _scene->setSceneRect(0, 0, 1, 1);

    _view = new ScaleView(_scene, this, this, _recording);
    //_view->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    _view->fitInView(_scene->sceneRect());
    _view->scale(1, -1);
    _view->invalidateScene();
    layout->addWidget(_view);

    {
        QWidget* container = new QWidget;
        QBoxLayout* containerLayout = new QHBoxLayout;

        _hoverInfo = new QLabel;
        containerLayout->addWidget(_hoverInfo);

        _minValue = new QSlider(Qt::Orientation::Horizontal);
        _minValue->setMinimum(-MaximumValue);
        _minValue->setValue(0);
        _minValue->setMaximum(MaximumValue);
        connect(_minValue, &QSlider::valueChanged, this, &ScaleWidget::rescaleItems);
        containerLayout->addWidget(_minValue);

        _minValueText = new QLabel;
        containerLayout->addWidget(_minValueText);

        _maxValue = new QSlider(Qt::Orientation::Horizontal);
        _maxValue->setMinimum(-MaximumValue);
        _maxValue->setValue(0);
        _maxValue->setMaximum(MaximumValue);
        connect(_maxValue, &QSlider::valueChanged, this, &ScaleWidget::rescaleItems);
        containerLayout->addWidget(_maxValue);

        _maxValueText = new QLabel;
        containerLayout->addWidget(_maxValueText);

        container->setLayout(containerLayout);
        layout->addWidget(container);
    }

    setLayout(layout);
}

void ScaleWidget::setSessionRecording(SessionRecording* recording) {
    _recording = recording;
    _view->_recording = recording;

    _items.clear();
    _scene->clear();

    _minValueText->setText(QString::number(recording->minMaxScale.first, 'f', 12));
    _maxValueText->setText(QString::number(recording->minMaxScale.second, 'f', 12));

    for (ScaleInfo p : recording->normalizedLinearizedScale) {
        ScaleItem* item = new ScaleItem(p.kf, _recording, Qt::white, 5.0);
        item->setPos(p.x, p.y);
        item->setZValue(1);
        _scene->addItem(item);
        _items.push_back(item);
    }
    
    for (size_t i = 0; i < _items.size() - 1; i += 1) {
        ScaleItem* curr = _items[i];
        ScaleItem* next = _items[i+1];

        curr->_rightNeighbor = next;
        next->_leftNeighbor = curr;

        QPen pen;
        pen.setColor(Qt::black);
        pen.setWidthF(0.0025f);
        QGraphicsLineItem* line = _scene->addLine(curr->pos().x(), curr->pos().y(), next->pos().x(), next->pos().y(), pen);
        line->setZValue(0);

        curr->_rightLine = line;
        next->_leftLine = line;
    }

    _view->fitInView(_scene->sceneRect());
    _view->invalidateScene();
}

void ScaleWidget::updateSessionRecording() {
    std::pair<double, double> minMax = _recording->minMaxScale;

    for (ScaleItem* i : _items) {
        assert(i->_keyframe);
        QPointF p = i->scenePos();
        double y = minMax.first + p.y() * (minMax.second - minMax.first);
        i->_keyframe->scale = y;

    }
    _view->fitInView(_scene->sceneRect());
    _view->invalidateScene();
}

void ScaleWidget::rescaleItems() {
    if (!_recording)  return;

    double delta = (_recording->minMaxScale.second - _recording->minMaxScale.first) / MaximumValue;

    std::pair<double, double> newMinMax;
    newMinMax.first = _recording->minMaxScale.first + _minValue->value() * delta;
    newMinMax.second = _recording->minMaxScale.second + _maxValue->value() * delta;

    _minValueText->setText(QString::number(newMinMax.first, 'f', 15));
    _maxValueText->setText(QString::number(newMinMax.second, 'f', 15));

    if (newMinMax.first >= newMinMax.second)  return;

    std::pair<double, double> oldMinMax = _recording->minMaxScale;
    for (ScaleItem* item : _items) {
        QPointF p = item->pos();
        double y = oldMinMax.first + p.y() * (oldMinMax.second - oldMinMax.first);
        double y2 = (y - newMinMax.first) / (newMinMax.second - newMinMax.first);
        item->setPos(p.x(), y2);

        if (item->_leftLine) {
            QLineF line = item->_leftLine->line();
            line.setP2(QPointF(p.x(), y2));
            item->_leftLine->setLine(line);
        }

        if (item->_rightLine) {
            QLineF line = item->_rightLine->line();
            line.setP1(QPointF(p.x(), y2));
            item->_rightLine->setLine(line);
        }
    }

    _minValue->setValue(0);
    _maxValue->setValue(0);
}

void ScaleWidget::dragEnterEvent(QDragEnterEvent* event) {
    _mainWindow->dragEnterEvent(event);
}

void ScaleWidget::dropEvent(QDropEvent* event) {
    _mainWindow->dropEvent(event);
}

void ScaleWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    _view->fitInView(_scene->sceneRect());
    _view->invalidateScene();
}
