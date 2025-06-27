#include "imageviewer.h"
#include "xviewer.h"
#include <QtWidgets/QApplication>

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);
    //ImageViewer w;
    XViewer x;
    x.show();
    return a.exec();
}
