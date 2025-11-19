#include <QApplication>
#include "QtMainWindow.h"

int main(int argc,char *argv[]){
    QApplication app(argc, argv);
    QtMainWindow window;
    window.resize(1200,800);
    window.setWindowTitle("CPU Pipeline Visualizer - Advanced");
    window.show();
    return app.exec();
}
