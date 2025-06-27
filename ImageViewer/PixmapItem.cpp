#include "PixmapItem.h"
#include "View.h"
#include <opencv2/core.hpp>

PixmapItem::PixmapItem()
{}

PixmapItem::~PixmapItem()
{}

void PixmapItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget /* = nullptr */)
{
    auto transform = painter->transform();
    double zoom = transform.m11();
    auto oldItemTransMode = transformationMode();
    if (zoom >= DEFAULT_DRAW_LINE_SCALE) {
        setTransformationMode(Qt::FastTransformation);
    }

    QGraphicsPixmapItem::paint(painter, option, widget);
    if (zoom >= DEFAULT_DRAW_LINE_SCALE) {
        setTransformationMode(oldItemTransMode);
    }
    bool dealed = false;
    if (zoom >= DEFAULT_DRAW_LINE_SCALE) {
        auto itemRect = boundingRect();
        painter->save();
        painter->setPen(QPen(QColor(128, 128, 128), 2.0 / zoom));

        auto view = dynamic_cast<QGraphicsView*>(widget->parent());
        auto viewportTransform = view->viewportTransform();
        QRectF viewRect = view->viewport()->rect();
        auto viewRectInScene = viewportTransform.inverted().mapRect(viewRect);
        auto itemRectInScene = mapRectToScene(itemRect);

        auto itemRectDisped = viewRectInScene.intersected(itemRectInScene);
        for (int iy = itemRectDisped.top(); iy <= itemRectDisped.bottom(); iy++) {
            painter->drawLine(0, iy, itemRect.width(), iy);
        }
        for (int ix = itemRectDisped.left(); ix <= itemRectDisped.right(); ix++) {
            painter->drawLine(ix, 0, ix, itemRect.height());
        }
        if (zoom >= DEFAULT_MAX_SCALE) {
            //设置灰色刷，黑色笔
            painter->setBrush(QBrush(QColor(128, 128, 128)));
            painter->setPen(QPen(QColor(0, 0, 0), 1.0 / zoom));
            QFont font("Arial");
            font.setPointSizeF(10 * zoom / 64);
            font.setWeight(QFont::Normal * zoom / 64);
            painter->scale(1.0 / zoom, 1.0 / zoom);
            painter->setFont(font);

            //给显示出来的每一个像素写上像素值
            for (int iy = itemRectDisped.top(); iy < itemRectDisped.bottom(); iy++) {
                for (int ix = itemRectDisped.left(); ix < itemRectDisped.right(); ix++) {
                    QString rgb = getPixelValue(ix, iy);

                    QRectF rect((ix + 0.1) * zoom, (iy + 0.05) * zoom, 0.8 * zoom, 0.9 * zoom);
                    QRectF bkGround = painter->boundingRect(rect, Qt::AlignCenter, rgb);
                    bkGround.adjust(-0.1 * zoom, 0, 0.1 * zoom, 0);
                    bkGround = bkGround.intersected(rect);
                    painter->drawRect(bkGround);
                    painter->drawText(rect, Qt::AlignCenter, rgb);
                }
            }
        }
        painter->restore();
    }
}

QString PixmapItem::getPixelValue(int x, int y)
{
    QString ret_str;
    QRgb pixelValue;
    uint grayValue;
    cv::Mat test_image;

    switch (m_image.format()) {
    case QImage::Format_ARGB32:
    case QImage::Format_RGBA8888:
    case QImage::Format_RGB32:
        // 获取某个像素的 RGBA 值
        pixelValue = m_image.pixel(x, y);
        if (qAlpha(pixelValue) == 255)
            ret_str = QString("%1\n%2\n%3").arg(qRed(pixelValue)).arg(qGreen(pixelValue)).arg(qBlue(pixelValue));
        else
            ret_str = QString("%1\n%2\n%3\n%4").arg(qRed(pixelValue)).arg(qGreen(pixelValue)).arg(qBlue(pixelValue)).arg(qAlpha(pixelValue));
        break;
    case QImage::Format_RGB888:
        // 获取某个像素的 RGB 值
        pixelValue = m_image.pixel(x, y);
        ret_str = QString("%1\n%2\n%3").arg(qRed(pixelValue)).arg(qGreen(pixelValue)).arg(qBlue(pixelValue));
        break;
    case QImage::Format_Indexed8:
        // 获取某个像素的灰度值
        ret_str = QString("%1").arg(m_image.pixelIndex(x, y));
        break;
    case QImage::Format_Grayscale8:
        // 获取某个像素的灰度值
        ret_str = QString("%1").arg(qGray(m_image.pixel(x, y)));
        break;
    case QImage::Format_Grayscale16:
        // 获取某个像素的灰度值
// 		test_image = QImage2Mat(m_image);
// 		grayValue = test_image.at<uint16_t>(cv::Point(x, y));
        pixelValue = m_image.pixel(x, y);
        grayValue = qGreen(pixelValue) * 256 + qBlue(pixelValue);
        ret_str = QString("%1").arg(grayValue);
        break;
    case QImage::Format_Mono:
        // 获取某个像素的黑白值
        ret_str = m_image.pixel(x, y) > 0 ? "1" : "0";
        break;
    default:
        break;
    }

    return ret_str;
}

QString PixmapItem::getPixelValue(QPoint pos)
{
    QString ret_str;
    QRgb pixelValue;
    uint grayValue;

    if (!m_image.rect().contains(pos)) {
        return ret_str;
    }

    switch (m_image.format()) {
// 	case QImage::Format_ARGB32:
// 	case QImage::Format_RGBA8888:
// 	case QImage::Format_RGB32:
// 		// 获取某个像素的 RGBA 值
// 		pixelValue = m_image.pixel(pos.x(), pos.y());
// 		if (qAlpha(pixelValue) == 255)
// 			ret_str = QString("%1\n%2\n%3").arg(qRed(pixelValue)).arg(qGreen(pixelValue)).arg(qBlue(pixelValue));
// 		else
// 			ret_str = QString("%1\n%2\n%3\n%4").arg(qRed(pixelValue)).arg(qGreen(pixelValue)).arg(qBlue(pixelValue)).arg(qAlpha(pixelValue));
// 		break;
    case QImage::Format_RGB888:
        // 获取某个像素的 RGB 值
        pixelValue = m_image.pixel(pos.x(), pos.y());
        ret_str = QString("[%1, %2, %3]").arg(qRed(pixelValue)).arg(qGreen(pixelValue)).arg(qBlue(pixelValue));
        break;
    case QImage::Format_Indexed8:
        // 获取某个像素的灰度值
        ret_str = QString("[%1]").arg(m_image.pixelIndex(pos.x(), pos.y()));
        break;
    case QImage::Format_Grayscale8:
        // 获取某个像素的灰度值
        ret_str = QString("[%1]").arg(qGray(m_image.pixel(pos.x(), pos.y())));
        break;
    case QImage::Format_Grayscale16:
        // 获取某个像素的灰度值
        pixelValue = m_image.pixel(pos.x(), pos.y());
        grayValue = qGreen(pixelValue) * 256 + qBlue(pixelValue);
        ret_str = QString("%1").arg(grayValue);
        break;
    case QImage::Format_Mono:
        // 获取某个像素的黑白值
        ret_str = m_image.pixel(pos.x(), pos.y()) > 0 ? "[1]" : "[0]";
        break;
    default:
        break;
    }

    return ret_str;
}
