#include "mainwindow.h"

#include "scalewidget.h"
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMimeData>
#include <QPushButton>
#include <QVBoxLayout>
#include <iostream>

MainWindow::MainWindow() {
    setWindowTitle("Session Recording Editor");
    setMinimumSize(1024, 500);
    setAcceptDrops(true);

    QWidget* central = new QWidget;
    QBoxLayout* layout = new QVBoxLayout;

    _scaleWidget = new ScaleWidget(this);
    layout->addWidget(_scaleWidget);

    {
        QWidget* container = new QWidget;
        QBoxLayout* containerLayout = new QHBoxLayout;

        _sourceFile = new QLineEdit;
        _sourceFile->setPlaceholderText("Source recording path");
        containerLayout->addWidget(_sourceFile);

        _destinationFile = new QLineEdit;
        _destinationFile->setPlaceholderText("Destination recording path");
        containerLayout->addWidget(_destinationFile);

        QPushButton* save = new QPushButton("Save");
        connect(save, &QPushButton::clicked, [this]() { saveRecording(); });
        containerLayout->addWidget(save);

        container->setLayout(containerLayout);
        layout->addWidget(container);
    }

    central->setLayout(layout);
    setCentralWidget(central);
}

void MainWindow::loadFile(std::string path) {
    _sessionRecording = loadSessionRecording(path);

    if (_sessionRecording)  _scaleWidget->setSessionRecording(_sessionRecording);
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event) {
    event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent* event) {
    std::string_view prefix = "file:///";

    std::string path = event->mimeData()->text().toStdString().substr(prefix.size());
    loadFile(path);
}

void MainWindow::saveRecording() {

}
