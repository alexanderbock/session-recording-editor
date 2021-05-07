#pragma once

#include <QMainWindow>

#include "sessionrecording.h"
#include "scalewidget.h"

class QLineEdit;

class MainWindow : public QMainWindow {
Q_OBJECT
public:
    MainWindow();

    void loadFile(std::string path);
    
    virtual void dragEnterEvent(QDragEnterEvent* event) override;
    virtual void dropEvent(QDropEvent* event) override;
    void saveRecording();

    ScaleWidget* _scaleWidget = nullptr;
    SessionRecording* _sessionRecording = nullptr;

    QLineEdit* _sourceFile;
    QLineEdit* _destinationFile;
};
