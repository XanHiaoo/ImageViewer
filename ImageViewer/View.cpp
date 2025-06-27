#include "View.h"
#include "Scene.h"
#include "PixmapItem.h"
#include <QElapsedTimer>
#include <QMimeData>
#include <QtMath>
#include <QDebug>
#include <future>
#include <thread>

View::View(QWidget* parent)
    : QGraphicsView(parent)
{
    // ���ô���ɽ�����ק
    this->setAcceptDrops(true);



    // ����ˮƽ/��ֱ������
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorUnderMouse);

    // �����
    setRenderHints(QPainter::Antialiasing);

    setCacheMode(QGraphicsView::CacheBackground);

    setAlignment(Qt::AlignCenter);
// 	setDragMode(QGraphicsView::ScrollHandDrag);
// 	setInteractive(true);

    m_scene = new QGraphicsScene();
    setScene(m_scene);
    setSceneRect(INT_MIN / 2, INT_MIN / 2, INT_MAX, INT_MAX);

    m_pixmapItem = new PixmapItem();
    m_scene->addItem(m_pixmapItem);

    m_rectItem = new QGraphicsRectItem();
    // ���������ʽΪ��ɫ��͸��
    QBrush brush(QColor(99, 184, 255, 160));  // ʹ�� RGBA ��ʽ������ alpha ֵΪ 128
    m_rectItem->setBrush(brush);
    m_scene->addItem(m_rectItem);

    connect(this, SIGNAL(loadImageFinished(QImage)), this, SLOT(inputImage(QImage)));
}

View::~View()
{}

void View::inputImage(QString path)
{
    if (path.isEmpty()) {
        return;
    }

    std::thread t1(&View::loadImage, this, path);
    t1.detach();
}

void View::inputImage(QPixmap pixmap)
{
    if (m_pixmapItem) {
        int old_w = m_pixmapItem->pixmap().width();
        int old_h = m_pixmapItem->pixmap().height();

        m_pixmapItem->setPixmap(pixmap);

        if (m_isLinkView) {
            if (old_w != pixmap.width() || old_h != pixmap.height()) {
                adaptiveWidget();
                setRevolve(0);
            }
        }
        else {
            adaptiveWidget();
            setRevolve(0);
        }
    }
}

void View::inputImage(QImage image)
{
    if (m_pixmapItem) {
        m_pixmapItem->setImage(image);
        QPixmap pixmap = QPixmap::fromImage(image);
        inputImage(pixmap);
    }
}

QRectF View::buildRect(QPointF start, QPointF end)
{
    QRectF rect;
    if (start.x() < end.x() && start.y() < end.y()) {
        rect.setTopLeft(start);
        rect.setBottomRight(end);
    }
    else if (start.x() < end.x() && start.y() > end.y()) {
        rect.setBottomLeft(start);
        rect.setTopRight(end);
    }
    else if (start.x() > end.x() && start.y() < end.y()) {
        rect.setTopRight(start);
        rect.setBottomLeft(end);
    }
    else if (start.x() > end.x() && start.y() > end.y()) {
        rect.setBottomRight(start);
        rect.setTopLeft(end);
    }

    return rect;
}

void View::adaptiveWidget()
{
    QPixmap pixmap = m_pixmapItem->pixmap();

    if (pixmap.isNull()) {
        centerOn(this->width() / 2, this->height() / 2);
        return;
    }

    int pixmap_w = pixmap.width();
    int pixmap_h = pixmap.height();
    int graphicsView_w = this->width();
    int graphicsView_h = this->height();

    double w = graphicsView_w / (pixmap_w * 1.0);
    double h = graphicsView_h / (pixmap_h * 1.0);

    m_currentScale = w < h ? w : h;

    QMatrix matrix;
    matrix.scale(m_currentScale, m_currentScale);
    this->setMatrix(matrix);
    centerOn(pixmap.width() / 2, pixmap.height() / 2);

    //emit scaleChanged(m_currentScale);
}

void View::setRevolve(double degrees)
{
    QMatrix matrix;
    if (degrees >= 360) {
        degrees = 0;
    }

    if (degrees < 0) {
        degrees = 0;
    }
    matrix.rotate(degrees);
    matrix.scale(m_currentScale, m_currentScale);
    this->setMatrix(matrix);
}

double View::getRevolve()
{
    QTransform transform = this->transform(); // ��ȡ��һ�����������ͼ�ı任����

    qreal rotation = qRadiansToDegrees(qAtan2(transform.m12(), transform.m11()));

    return rotation;
}

void View::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        //QGraphicsView����
        QPoint view_pos = event->pos();
        //QGraphicsScene����
        QPointF scene_pos = mapToScene(view_pos);
        m_start = scene_pos;

        if (m_mode == Normal) {
            setDragMode(QGraphicsView::ScrollHandDrag);
        }
        else if (m_mode == Rect) {
        }
    }

    QGraphicsView::mousePressEvent(event);
}

void View::mouseMoveEvent(QMouseEvent* event)
{
    if (event->buttons() == Qt::LeftButton) {
        //QGraphicsView����
        QPoint view_pos = event->pos();
        //QGraphicsScene����
        QPointF scene_pos = mapToScene(view_pos);
        m_end = scene_pos;

        if (m_mode == Normal) {
        }
        else if (m_mode == Rect) {
            m_rectItem->setRect(buildRect(m_start, m_end));
        }
    }
    else {
        QPointF pt = mapToScene(event->pos());

        QString pix_str = m_pixmapItem->getPixelValue(pt.toPoint());
        emit positionChanged(pt.x(), pt.y());
        emit pixelChanged(pix_str);
    }

    QGraphicsView::mouseMoveEvent(event);
}

void View::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        if (m_mode == Normal) {
            setDragMode(QGraphicsView::NoDrag);
        }
        else if (m_mode == Rect) {
            emit rectCropFinished(m_rectItem->rect().toRect());
            m_rectItem->setRect(QRectF());
            setMode(Normal);
        }
    }

    QGraphicsView::mouseReleaseEvent(event);
}

void View::wheelEvent(QWheelEvent* event)
{
    // ���� �Ŵ���С
    qreal scaleFactor = pow(2.0, event->angleDelta().y() / 240.0);  // ��������

    // ���ű���
    qreal currentScale = transform().scale(scaleFactor, scaleFactor).mapRect(QRectF(0, 0, 1, 1)).width();

    if (currentScale <= m_minScale) {
        if (currentScale < m_minScale)
            currentScale = m_minScale;
        else
            return;
    }
    else if (currentScale >= m_maxScale) {
        if (currentScale > m_maxScale)
            currentScale = m_maxScale;
        else
            return;
    }

    m_currentScale = currentScale;
    QMatrix matrix;
    matrix.scale(currentScale, currentScale);
    double revolve = getRevolve();
    matrix.rotate(revolve);
    this->setMatrix(matrix);

    emit scaleChanged(m_currentScale);
}

void View::resizeEvent(QResizeEvent* event)
{
    //fitInView(m_pixmapItem, Qt::KeepAspectRatio);
    QGraphicsView::resizeEvent(event);
}

void View::loadImage(QString path)
{
    emit loadImageSignal(true);

    // ����һ�� QElapsedTimer ����
    QElapsedTimer timer;
    // ��ʼ��ʱ
    timer.start();

    //QPixmap pixmap(path);
    QImage image(path);

    // ֹͣ��ʱ����ȡ��ʱʱ��
    qint64 elapsedTime = timer.elapsed(); // �������뼶��ĺ�ʱʱ��
    // �����ʱʱ��
    qDebug() << "Elapsed time:" << elapsedTime << "ms";

    emit loadImageFinished(image);
    emit loadImageSignal(false);
}

void View::dropEvent(QDropEvent* event)
{
    QGraphicsView::dropEvent(event);
    emit hasDrop(event);
}

void View::dragEnterEvent(QDragEnterEvent* event)
{
    QGraphicsView::dragEnterEvent(event);
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void View::dragMoveEvent(QDragMoveEvent* event)
{
    QGraphicsView::dragMoveEvent(event);
    event->acceptProposedAction();
}

void View::dragLeaveEvent(QDragLeaveEvent* event)
{
    QGraphicsView::dragLeaveEvent(event);
    event->accept();
}
