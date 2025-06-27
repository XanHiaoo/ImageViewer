#pragma once

#include <QGraphicsView>
#include <QGraphicsItem>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QResizeEvent>
#include <QKeyEvent>

//#define DEFAULT_MIN_SCALE		0.001		//默认最小缩放比例
//#define DEFAULT_MAX_SCALE		60			//默认最大缩放比例
#define DEFAULT_MIN_SCALE		0.05		//默认最小缩放比例
#define DEFAULT_MAX_SCALE		60			//默认最大缩放比例
#define DEFAULT_DRAW_LINE_SCALE	40			//默认画线比例

class Scene;
class PixmapItem;

class View  : public QGraphicsView {
    Q_OBJECT

public:
    View(QWidget* parent = nullptr);
    ~View();

    enum MODE {
        Normal,     //常规模式--移动缩放
        Rect,       //矩形
    };

    void inputImage(QString path);

    void setMode(MODE mode)
    {
        m_mode = mode;
    }
    MODE getMode()
    {
        return m_mode;
    }

    void setLinkView(bool linkView)
    {
        m_isLinkView = linkView;
    }
    bool getLinkView()
    {
        return m_isLinkView;
    }

    void setRevolve(double degrees);
    double getRevolve();

    double getCurrentScale()
    {
        return m_currentScale;
    }

public slots:
    void inputImage(QImage image);
    void adaptiveWidget();//自适应窗口

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;

private slots:
    void loadImage(QString path);
    void inputImage(QPixmap pixmap);
    QRectF buildRect(QPointF start, QPointF end);

private:
    QGraphicsScene* m_scene = nullptr;
    PixmapItem* m_pixmapItem = nullptr;
    QGraphicsRectItem* m_rectItem = nullptr;

    double m_minScale = DEFAULT_MIN_SCALE;
    double m_maxScale = DEFAULT_MAX_SCALE;

    double m_currentScale = 1.0;

    bool m_isLinkView = false;

    MODE m_mode = Normal;

    QPointF m_start;
    QPointF m_end;

signals:
    void scaleChanged(double scale);
    void positionChanged(int x, int y);
    void pixelChanged(QString pixStr);
    void loadImageSignal(bool isStart); //true:开始加载 false:完成加载
    void loadImageFinished(QImage image);
    void hasDrop(QDropEvent* event);
    void rectCropFinished(QRect rect);
};
