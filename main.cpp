#include <QApplication>
#include <QWidget>

#include "mainwindow.h"
#include <iostream>

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    MainWindow w;
    w.show();

    if (argc == 2) {
        std::string file = argv[1];

    }

    app.exec();
}
