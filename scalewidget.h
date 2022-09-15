#pragma once

#include <QWidget>

#include <QGraphicsItem>
#include <QGraphicsView>

struct KeyframeCamera;
class MainWindow;
class QGraphicsScene;
class QGraphicsView;
class QLabel;
class QLineEdit;
class QResizeEvent;
class QSlider;
class ScaleWidget;
struct SessionRecording;

struct ScaleItem : public QGraphicsItem {
    ScaleItem(KeyframeCamera* kf, SessionRecording* recording, QColor color = Qt::white, double size = 10.0);

    virtual QRectF boundingRect() const override;

    virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
        QWidget* widget) override;

    KeyframeCamera* _keyframe;
    SessionRecording* _recording = nullptr;
    QGraphicsLineItem* _leftLine = nullptr;
    QGraphicsLineItem* _rightLine = nullptr;

    ScaleItem* _leftNeighbor = nullptr;
    ScaleItem* _rightNeighbor = nullptr;

    QColor _color;
    double _size;
    bool _picked = false;
};

class ScaleView : public QGraphicsView {
Q_OBJECT
public:
    ScaleView(QGraphicsScene* scene, QWidget* parent, ScaleWidget* scale, SessionRecording* recording);

    virtual void mouseDoubleClickEvent(QMouseEvent* event) override;
    virtual void mouseMoveEvent(QMouseEvent* event) override;
    virtual void mousePressEvent(QMouseEvent* event) override;
    virtual void mouseReleaseEvent(QMouseEvent* event) override;

    ScaleWidget* _parent;
    SessionRecording* _recording = nullptr;

    ScaleItem* _pickedItem = nullptr;
};

class ScaleWidget : public QWidget {
Q_OBJECT
public:
    ScaleWidget(MainWindow* mainWindow, QWidget* parent = nullptr);

    void setSessionRecording(SessionRecording* recording);
    void updateSessionRecording();

    virtual void dragEnterEvent(QDragEnterEvent* event) override;
    virtual void dropEvent(QDropEvent* event) override;
    virtual void resizeEvent(QResizeEvent* event) override;

public slots:
    void rescaleItems();

public:
    MainWindow* _mainWindow;
    QGraphicsScene* _scene = nullptr;
    ScaleView* _view = nullptr;

    QLabel* _hoverInfo;
    QSlider* _minValue;
    QLabel* _minValueText;
    QSlider* _maxValue;
    QLabel* _maxValueText;

    std::vector<ScaleItem*> _items;
    SessionRecording* _recording = nullptr;
};
