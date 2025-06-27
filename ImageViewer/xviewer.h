#pragma once

#include <QMap>
#include <QMutex>
#include <QMainWindow>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QPropertyAnimation>
#include <QShowEvent>
#include <QResizeEvent>
#include <QDropEvent>
#include <QMouseEvent>
#include <QGraphicsView>
#include <QKeyEvent>
#include <thread>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

QT_BEGIN_NAMESPACE
namespace Ui {
class XViewerClass;
};
QT_END_NAMESPACE

class View;
class HistogramWidget;
class InputPonitWidget;

class XViewer : public QMainWindow {
    Q_OBJECT

public:
    XViewer(QWidget* parent = nullptr);
    ~XViewer();

    void inputImage(QString path);

protected:
    void resizeEvent(QResizeEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event);
    void showEvent(QShowEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    QImage MatToImage(const cv::Mat& m);
    void initUi();
    void initConnection();
    void run();//�߳���ִ�к���
    void open(QString path);
    void remove();
    void clear();
    void previous();
    void next();

    QImage autoMaxContrastAndPseudoColor(QImage image);

    QPixmap generateIcon(QImage& image, QSize iconSize, QString str, QColor backC, QColor fontC);
    QPixmap generateIcon(QImage& image, QSize iconSize);

    void viewHistogram(QRect rect = QRect());

    void test();

private slots:
    void positionChanged(int x, int y);
    void actionTriggered(QAction* triggeredAction);
    void itemChangedSlot();
    void updateStatus();

private:
    Ui::XViewerClass* ui;
    //��ͼ����
    View* m_view = nullptr;

    HistogramWidget* m_histogramWidget = nullptr;
    InputPonitWidget* m_inputPonitWidget = nullptr;

    //�Զ���󻯶Աȶ�
    bool m_isAutoMax = false;
    //1ͨ��αɫ��
    bool m_isPseudo = false;

    //ͼƬ����
    QMap<QString, QImage> m_images;

    //�̼߳���
    //std::thread m_thread;
    QStringList m_loadList;
    QMutex m_mutex;
    bool m_isRun = true;

    //�Ҽ��˵�
    QMenu* m_menu = nullptr;

    QListWidget* m_listWidget = nullptr;
    QPushButton* m_foldButton = nullptr;
    QPropertyAnimation* m_propertyAnimation1 = nullptr;
    QPropertyAnimation* m_propertyAnimation2 = nullptr;

    QLabel* m_posLabel = nullptr;
    QLabel* m_pixelLabel = nullptr;
    QLabel* m_scaleLabel = nullptr;
    QLabel* m_picSizeLabel = nullptr;
    QLabel* m_picFormatLabel = nullptr;

signals:
    void newImageArrived(QString, QImage);

    void startLoad();
    void stopLoad();
};
