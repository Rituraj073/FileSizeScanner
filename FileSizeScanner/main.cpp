#include "stdafx.h"
#include "FileSizeScanner.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    FileSizeScanner window;
    window.show();
    return app.exec();
}
