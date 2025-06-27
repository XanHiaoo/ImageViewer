#include "imageviewer.h"

ImageViewer::ImageViewer(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::ImageViewerClass())
{
    ui->setupUi(this);
}

ImageViewer::~ImageViewer()
{
    delete ui;
}
