#include "scalewidget.h"

#include "mainwindow.h"
#include "sessionrecording.h"
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QLabel>
#include <QResizeEvent>
#include <QVBoxLayout>

// @TODO graphicsitem z-level for lines and items

ScaleItem::ScaleItem(KeyframeCamera* keyframe, SessionRecording* recording, QColor color, 
                     double size)
    : _keyframe(keyframe)
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

void ScaleItem::hoverMoveEvent(QGraphicsSceneHoverEvent* event) {

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

    if (_pickedItem) {
        if (_pickedItem->_leftNeighbor && pt.x() <= _pickedItem->_leftNeighbor->scenePos().x()) return;
        if (_pickedItem->_rightNeighbor && pt.x() >= _pickedItem->_rightNeighbor->scenePos().x()) return;

        // If the first or last point is selected, we don't want to allow a change in x
        if (_pickedItem->_leftNeighbor == nullptr || _pickedItem->_rightNeighbor == nullptr)  pt.rx() = _pickedItem->scenePos().x();

        // Update item position
        _pickedItem->setPos(pt);

        // Update connected lines
        if (_pickedItem->_leftLine) {
            QGraphicsLineItem* left = _pickedItem->_leftLine;
            QLineF leftLineOld = left->line();
            QLineF leftLineNew = QLineF(leftLineOld.p1(), pt);
            _pickedItem->_leftLine->setLine(leftLineNew);
        }

        if (_pickedItem->_rightLine) {
            QGraphicsLineItem* right = _pickedItem->_rightLine;
            QLineF rightLineOld = right->line();
            QLineF rightLineNew = QLineF(pt, rightLineOld.p2());
            _pickedItem->_rightLine->setLine(rightLineNew);
        }
    }
    else {
        if (_recording) {
            QPointF pt2 = mapToScene(pt.x(), pt.y());
            double length = _recording->recordingLength;
            std::pair<double, double> minMax = _recording->minMaxScale;

            double x = pt2.x() * length;
            double y = minMax.first + pt2.y() * (minMax.second - minMax.first);

            _parent->_hoverInfo->setText(QString::number(x) + ", " + QString::number(y));
        }
    }
}

void ScaleView::mouseDoubleClickEvent(QMouseEvent* event) {
    QPointF pt = mapToScene(event->pos());

    ScaleItem* prev = nullptr;
    ScaleItem* next = nullptr;
    for (size_t i = 0; i < _parent->_items.size(); i += 1) {
        ScaleItem* c = _parent->_items[i];
        if (c->scenePos().x() > pt.x()) {
            next = c;
            prev = next->_leftNeighbor;
            break;
        }
    }

    assert(prev && next);
    QPen pen;
    pen.setColor(Qt::black);
    pen.setWidthF(0.0025f);
    QGraphicsLineItem* line = scene()->addLine(QLineF(pt, next->scenePos()), pen);

    ScaleItem* item = new ScaleItem(nullptr, _recording, Qt::white, 5.0);
    item->setPos(pt);
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
            }
            scene()->update(sceneRect());
            return;
        }
    }

    // Because of the early return, if we get here, we didn't pick anything
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
    _view->invalidateScene();

    layout->addWidget(_view);

    _hoverInfo = new QLabel;
    layout->addWidget(_hoverInfo);

    setLayout(layout);
}

void ScaleWidget::setSessionRecording(SessionRecording* recording) {
    _recording = recording;
    _view->_recording = recording;

    _items.clear();
    _scene->clear();

    struct Line {
        QGraphicsLineItem* line;
        KeyframeCamera* start;
        KeyframeCamera* end;
    };
    std::vector<Line> lines;
    for (size_t i = 0; i < recording->normalizedLinearizedScale.size() - 1; i += 1) {
        ScaleInfo current = recording->normalizedLinearizedScale[i];
        ScaleInfo next = recording->normalizedLinearizedScale[i + 1];

        QPen pen;
        pen.setColor(Qt::black);
        pen.setWidthF(0.0025f);
        QGraphicsLineItem* line = _scene->addLine(current.x, current.y, next.x, next.y, pen);
        lines.push_back({ line, current.kf, next.kf });
    }
    for (ScaleInfo p : recording->normalizedLinearizedScale) {
        ScaleItem* item = new ScaleItem(p.kf, _recording, Qt::white, 5.0);
        item->setPos(p.x, p.y);
        _scene->addItem(item);
        _items.push_back(item);
    }

    for (Line line : lines) {
        for (ScaleItem* si : _items) {
            KeyframeCamera* kf = si->_keyframe;
            if (kf == line.start) {
                assert(si->_rightLine == nullptr);
                si->_rightLine = line.line;
            }
            else if (kf == line.end) {
                assert(si->_leftLine == nullptr);
                si->_leftLine = line.line;
            }
        }
    }

    for (size_t i = 0; i < _items.size(); i += 1) {
        if (i > 0)  _items[i]->_leftNeighbor = _items[i-1];
        if (i < _items.size() - 1) _items[i]->_rightNeighbor = _items[i+1];
    }

    _view->fitInView(_scene->sceneRect());
    _view->invalidateScene();
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
