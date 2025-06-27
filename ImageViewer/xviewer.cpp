#include "XViewer.h"
#include "View.h"
#include "ui_XViewer.h"
#include <QImageReader>
#include <QElapsedTimer>
#include <QMimeData>
#include <QDebug>
#include <QFileDialog>
#include <QFileInfo>
#include <QFileIconProvider>
#pragma execution_character_set("utf-8")


constexpr int ITEM_WIDTH = 400;
constexpr int ITEM_HEIGHT = 80;

constexpr int ICON_WIDTH = 100;
constexpr int ICON_HEIGHT = 64;

constexpr int FLOD_BTN_HEIGHT = 20;
constexpr int LISTWIDGET_HEIGHT = 100;

cv::Mat QImage2Mat(const QImage& image)
{
    QString format = image.format();
    switch (image.format()) {
    case QImage::Format_ARGB32:
    case QImage::Format_ARGB32_Premultiplied: {
        cv::Mat mat(image.height(), image.width(), CV_8UC4, const_cast<uchar*>(image.bits()), image.bytesPerLine());
        return mat.clone(); // Clone to ensure proper memory management
    }
    case QImage::Format_RGB32: {
        cv::Mat mat(image.height(), image.width(), CV_8UC4, const_cast<uchar*>(image.bits()), image.bytesPerLine());
        cv::Mat matBGR;
        cv::cvtColor(mat, matBGR, cv::COLOR_BGRA2BGR);
        return matBGR;
    }
    case QImage::Format_RGB888: {
        cv::Mat mat(image.height(), image.width(), CV_8UC3, const_cast<uchar*>(image.bits()), image.bytesPerLine());
        cv::Mat matBGR;
        cv::cvtColor(mat, matBGR, cv::COLOR_RGB2BGR);
        return matBGR;
    }
    case QImage::Format_Indexed8: {
        return cv::Mat(image.height(), image.width(), CV_8UC1, const_cast<uchar*>(image.bits()), image.bytesPerLine()).clone();
    }
    default:
        return cv::Mat();
    }
}

QImage Mat2QImage(const cv::Mat& mat, bool clone = false)
{
    if (mat.empty()) {
        throw std::invalid_argument("Input cv::Mat is empty.");
    }

    QImage image;
    switch (mat.type()) {
    case CV_8UC1: {
        image = QImage(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_Grayscale8);
        break;
    }
    case CV_8UC3: { // BGR image
        cv::Mat matRGB;
        cv::cvtColor(mat, matRGB, cv::COLOR_BGR2RGB); // Convert BGR to RGB
        image = QImage(matRGB.data, matRGB.cols, matRGB.rows, matRGB.step, QImage::Format_RGB888);
        if (clone) {
            return image.copy(); // Deep copy
        }
        else {
            return image;
        }
    }
    case CV_8UC4: { // BGRA image
        image = QImage(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_ARGB32);
        break;
    }
    default:
        throw std::runtime_error("Unsupported cv::Mat format for conversion to QImage.");
    }

    return clone ? image.copy() : image; // Return deep copy or shallow copy
}

QRect GetRealShowRect(const QRect& avail_roi, const QSize& image_size)
{
    if (image_size.isEmpty() || avail_roi.isEmpty()) {
        return QRect(); // 返回空矩形，表示无效
    }

    // 获取图像的宽高比
    double image_aspect = static_cast<double>(image_size.width()) / image_size.height();
    // 获取可用区域的宽高比
    double roi_aspect = static_cast<double>(avail_roi.width()) / avail_roi.height();

    // 根据宽高比调整矩形大小
    QRect real_show_rect;
    if (image_aspect > roi_aspect) {
        // 图像更宽，宽度填满，调整高度
        int new_width = avail_roi.width();
        int new_height = static_cast<int>(new_width / image_aspect);
        int y_offset = (avail_roi.height() - new_height) / 2; // 垂直居中
        real_show_rect = QRect(avail_roi.x(), avail_roi.y() + y_offset, new_width, new_height);
    }
    else {
        // 图像更高，高度填满，调整宽度
        int new_height = avail_roi.height();
        int new_width = static_cast<int>(new_height * image_aspect);
        int x_offset = (avail_roi.width() - new_width) / 2; // 水平居中
        real_show_rect = QRect(avail_roi.x() + x_offset, avail_roi.y(), new_width, new_height);
    }

    return real_show_rect;
}

QString GetImageType(const QImage& image)
{
    if (image.isNull()) {
        return "Null Image";
    }

    switch (image.format()) {
    case QImage::Format_ARGB32:
        return "ARGB32 (8 bits per channel, 4 channels)";
    case QImage::Format_ARGB32_Premultiplied:
        return "ARGB32 Premultiplied (8 bits per channel, 4 channels)";
    case QImage::Format_RGB32:
        return "RGB32 (8 bits per channel, 4 channels with alpha ignored)";
    case QImage::Format_RGB888:
        return "RGB888 (8 bits per channel, 3 channels)";
    case QImage::Format_Indexed8:
        return "Indexed8 (8 bits per pixel, palette-based)";
    case QImage::Format_Grayscale8:
        return "Grayscale8 (8 bits per pixel, single channel)";
    case QImage::Format_Mono:
        return "Mono (1 bit per pixel, black and white)";
    case QImage::Format_MonoLSB:
        return "Mono LSB (1 bit per pixel, black and white, least significant bit)";
    case QImage::Format_RGBA8888:
        return "RGBA8888 (8 bits per channel, 4 channels)";
    case QImage::Format_RGBA8888_Premultiplied:
        return "RGBA8888 Premultiplied (8 bits per channel, 4 channels)";
    default:
        return "Unknown Format";
    }
}

XViewer::XViewer(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::XViewerClass())
    , m_isRun(true)
{
    initUi();
    std::thread(&XViewer::run, this).detach();
    //test();
}

XViewer::~XViewer()
{
    delete ui;
    m_isRun = false;
}

void XViewer::inputImage(QString path)
{
    open(path);
}

void XViewer::initUi()
{
    ui->setupUi(this);

    // 设置窗体可响应 Mouse Move
    this->setMouseTracking(true);

    // 设置窗口图标
    //this->setWindowIcon(QIcon(":/icon/misv-v.ico"));

    //QPalette palette;
    //palette.setColor(QPalette::Background, QColor(33, 33, 33));  // 这里设置背景颜色
    //this->statusBar()->setAutoFillBackground(true);
    //this->statusBar()->setPalette(palette);

    m_view = new View();
    m_view->setBackgroundBrush(QBrush(QColor(33, 33, 33)));

    this->setCentralWidget(m_view);
    m_view->installEventFilter(this);

    //右键菜单
    m_menu = new QMenu(this);
    //添加按钮
    auto open_action = m_menu->addAction("打开");
    open_action->setShortcut(Qt::CTRL + Qt::Key_O);

    auto previous_action = m_menu->addAction("上一张");
    previous_action->setShortcut(Qt::Key_Left);

    auto next_action = m_menu->addAction("下一张");
    next_action->setShortcut(Qt::Key_Right);

    m_menu->addAction("删除这张");
    m_menu->addAction("清空");
    m_menu->addAction("链接视图")->setCheckable(true);
    m_menu->addAction("自动最大化对比度")->setCheckable(true);
    m_menu->addAction("1通道伪色彩")->setCheckable(true);

    QMenu* his_menu = new QMenu("查看直方图");
    his_menu->addAction("局部直方图");
    his_menu->addAction("全图直方图");
    m_menu->addMenu(his_menu);
    m_menu->addAction("快速定位")->setCheckable(true);
    m_menu->addAction("顺时针旋转90°");
    m_menu->addAction("逆时针旋转90°");

    //connect(his_menu, &QMenu::triggered, this, &XViewer::actionTriggered);

    m_listWidget = new QListWidget(this);
    // 控件外观
    QString backgroundColorStyle = "background-color: rgba(80, 80, 80, 0.8);";
    QString frontColorStyle = "color: rgb(210, 210, 210);";
    m_listWidget->setStyleSheet(backgroundColorStyle + frontColorStyle);
    m_listWidget->setFrameShape(QListWidget::NoFrame);
    m_listWidget->setIconSize(QSize(ITEM_WIDTH, ITEM_HEIGHT));
    m_listWidget->setViewMode(QListView::ListMode);	// 设置为图标显示模式
    m_listWidget->setResizeMode(QListView::Adjust);	// 设置为自适应
    m_listWidget->setMovement(QListView::Static);		// 设置为不可拖动
    m_listWidget->setFlow(QListView::LeftToRight);
    //m_listWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_listWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    //m_listWidget->setContextMenuPolicy(Qt::CustomContextMenu);

    m_foldButton = new QPushButton(this);
    m_foldButton->setFlat(false);
    m_foldButton->setCheckable(true);
    m_foldButton->setChecked(true);

    m_foldButton->setStyleSheet(
        "QPushButton{background-color:transparent;image: url(:/icon/up.png);border:none;}"
        "QPushButton:hover{background-color:qlineargradient(x1:-1, y1:0 , x2:1 ,y2:0 stop:0 rgba(20,20,20,0.6) ,stop:1 transparent);image: url(:/icon/up.png);border:none;}"
        "QPushButton:checked{image: url(:/icon/down.png);border:none;};"
    );

    m_propertyAnimation1 = new QPropertyAnimation(m_foldButton, "geometry");
    m_propertyAnimation2 = new QPropertyAnimation(m_listWidget, "geometry");

    m_posLabel = new QLabel();
    m_pixelLabel = new QLabel();
    m_scaleLabel = new QLabel();
    m_picSizeLabel = new QLabel();
    m_picFormatLabel = new QLabel();

    m_posLabel->setFixedWidth(100);
    m_pixelLabel->setFixedWidth(100);
    m_scaleLabel->setFixedWidth(100);
    m_picSizeLabel->setFixedWidth(100);
    m_picFormatLabel->setFixedWidth(100);

    auto pstatusBar = this->statusBar();
    pstatusBar->setSizeGripEnabled(false);//去掉状态栏右下角的三角
    pstatusBar->addWidget(m_posLabel);
    pstatusBar->addWidget(m_pixelLabel);
    pstatusBar->addWidget(m_scaleLabel);
    //pstatusBar->addWidget();
    pstatusBar->addWidget(m_picSizeLabel);
    pstatusBar->addWidget(m_picFormatLabel);

    initConnection();

    //m_view->inputImage("D:/sourceMaterial/png/panel_image.png");

// 	Tool& tool = Tool::GetInstance();
// 	tool.loadingStart(this);
}

void XViewer::initConnection()
{
    connect(this, &XViewer::newImageArrived, this, [&](QString path, QImage image) {
        // 创建 QFileInfo 对象，并传入文件路径
        QFileInfo fileInfo(path);
        // 使用 fileName() 方法获取文件名
        QString fileName = fileInfo.fileName();

        QPalette pal = m_listWidget->palette();
        QBrush back_brush = pal.background();
        QBrush font_brush = pal.foreground();
        QPixmap icon_map = generateIcon(image, QSize(ICON_WIDTH, ICON_HEIGHT));
        // 为单元项添加图标对象
        QListWidgetItem* item = new QListWidgetItem(QIcon(icon_map), "");
        item->setToolTip(path);
        m_listWidget->addItem(item);
        m_listWidget->setCurrentItem(item);
    });

    connect(m_view, SIGNAL(positionChanged(int, int)), this, SLOT(positionChanged(int, int)));

    connect(m_view, &View::loadImageSignal, this, [&](bool isStart) {
    });

    connect(m_view, &View::scaleChanged, this, [&](double scale) {
        int value = scale * 100;
        QString str = QString("%1%").arg(value);
        m_scaleLabel->setText(str);
    });

    connect(m_view, &View::pixelChanged, this, [&](QString str) {
        m_pixelLabel->setText(str);
    });

    connect(m_view, &View::hasDrop, this, [&](QDropEvent * event) {
        if (event->mimeData()->hasUrls()) {	//是否是文件路径
            QList<QUrl> urls = event->mimeData()->urls();

            if (urls.isEmpty())
                return;

            QString file_name;

            for (auto i : urls) {
                file_name = i.toLocalFile();
                if (file_name.isEmpty())
                    return;

                open(file_name);
            }
        }
    });

    connect(m_view, &View::rectCropFinished, this, [&](QRect rect) {
        viewHistogram(rect);
    });

    connect(m_menu, &QMenu::triggered, this, &XViewer::actionTriggered);

    connect(m_foldButton, &QPushButton::toggled, this, [ = ](bool flg) {
        if (flg) {
            m_propertyAnimation1->setStartValue(QRect(0, this->height() - m_foldButton->height() - this->statusBar()->height(), m_foldButton->width(), m_foldButton->height()));
            m_propertyAnimation1->setEndValue(QRect(0, this->height() - m_listWidget->height() - m_foldButton->height() - this->statusBar()->height(), m_foldButton->width(), m_foldButton->height()));
            m_propertyAnimation2->setStartValue(QRect(0, this->height(), m_listWidget->width(), m_listWidget->height()));
            m_propertyAnimation2->setEndValue(QRect(0, this->height() - m_listWidget->height() - this->statusBar()->height(), m_listWidget->width(), m_listWidget->height()));
        }
        else {
            m_propertyAnimation1->setStartValue(QRect(0, this->height() - m_listWidget->height() - m_foldButton->height() - this->statusBar()->height(), m_foldButton->width(), m_foldButton->height()));
            m_propertyAnimation1->setEndValue(QRect(0, this->height() - m_foldButton->height() - this->statusBar()->height(), m_foldButton->width(), m_foldButton->height()));
            m_propertyAnimation2->setStartValue(QRect(0, this->height() - m_listWidget->height() - this->statusBar()->height(), m_listWidget->width(), m_listWidget->height()));
            m_propertyAnimation2->setEndValue(QRect(0, this->height(), m_listWidget->width(), m_listWidget->height()));
        }
        m_propertyAnimation1->setEasingCurve(QEasingCurve::OutCubic);
        m_propertyAnimation2->setEasingCurve(QEasingCurve::OutCubic);

        m_propertyAnimation1->start();
        m_propertyAnimation2->start();
    });

    connect(m_listWidget, &QListWidget::itemSelectionChanged, this, &XViewer::itemChangedSlot);
}

// QImage XViewer::MatToImage(const cv::Mat& m)
// {
// 	// 输入数据有效性检查
// 	if (m.empty())
// 	{
// 		qDebug() << "Error: Input cv::Mat is empty.";
// 		return QImage(); // 返回一个空的 QImage
// 	}
//
// 	int chane = m.type();
// 	qDebug() << chane << "++";
//
// 	switch (m.type())
// 	{
// 	case CV_8UC1:
// 	{
// 		QImage img((uchar*)m.data, m.cols, m.rows, m.cols * 1, QImage::Format_Grayscale8);
// 		return img;
// 	}
// 	break;
// 	case CV_8UC3:
// 	{
// 		QImage img((uchar*)m.data, m.cols, m.rows, m.cols * 3, QImage::Format_RGB888);
// 		return img.rgbSwapped();
// 	}
// 	break;
// 	case CV_8UC4:
// 	{
// 		QImage img((uchar*)m.data, m.cols, m.rows, m.cols * 4, QImage::Format_ARGB32);
// 		return img;
// 	}
// 	break;
// 	default:
// 	{
// 		qDebug() << "Error: Unsupported cv::Mat data type. Defaulting to RGB888 format.";
// 		QImage img((uchar*)m.data, m.cols, m.rows, m.cols * 3, QImage::Format_RGB888);
// 		return img.rgbSwapped();
// 	}
// 	}
// }

// QImage XViewer::MatToImage(const cv::Mat& mat)
// {
// 	cv::Mat matCopy;
// 	QImage image;
//
// 	switch (mat.type()) {
// 	case CV_8UC1:
// 		//cv::cvtColor(mat, matCopy, cv::COLOR_GRAY2RGB);
// 		image = QImage(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_Grayscale8);
// 		break;
// 	case CV_8UC3:
// 		cv::cvtColor(mat, matCopy, cv::COLOR_BGR2RGB);
// 		image = QImage(matCopy.data, matCopy.cols, matCopy.rows, matCopy.step, QImage::Format_RGB888);
// 		break;
// 	case CV_8UC4:
// 		cv::cvtColor(mat, matCopy, cv::COLOR_BGRA2RGB);
// 		image = QImage(matCopy.data, matCopy.cols, matCopy.rows, matCopy.step, QImage::Format_RGB888);
// 		break;
// 	case CV_16UC1: {
// 		/*double minVal, maxVal;
// 		cv::minMaxLoc(mat, &minVal, &maxVal);
// 		cv::Mat normalizedMat;
// 		mat.convertTo(normalizedMat, CV_16U, 65535.0 / (maxVal - minVal), -minVal * 65535.0 / (maxVal - minVal));
// 		image = QImage(normalizedMat.data, normalizedMat.cols, normalizedMat.rows, normalizedMat.step, QImage::Format_Grayscale16);*/
//
// 		image = QImage((const uchar*)mat.data, mat.cols, mat.rows, mat.step, QImage::Format_Grayscale16);
// 		break;
// 	}
// 	default:
// 		return QImage();
// 	}
//
// 	//QImage image(matCopy.data, matCopy.cols, matCopy.rows, matCopy.step, QImage::Format_RGB888);
// 	return image.copy();
// }

QImage XViewer::MatToImage(const cv::Mat& mat)
{
    // 检查输入 Mat 是否为空
    if (mat.empty()) {
        qDebug() << "Input Mat is empty!";
        return QImage();
    }

    cv::Mat matCopy;  // 用于存储可能需要调整的数据
    QImage image;

    switch (mat.type()) {
    case CV_8UC1:  // 单通道灰度图
        image = QImage(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_Grayscale8);
        break;

    case CV_8UC3:  // 三通道彩色图
        cv::cvtColor(mat, matCopy, cv::COLOR_BGR2RGB);  // 转换为 RGB 格式
        image = QImage(matCopy.data, matCopy.cols, matCopy.rows, matCopy.step, QImage::Format_RGB888);
        break;

    case CV_8UC4:  // 四通道彩色图
        cv::cvtColor(mat, matCopy, cv::COLOR_BGRA2RGB);  // 转换为 RGB 格式
        image = QImage(matCopy.data, matCopy.cols, matCopy.rows, matCopy.step, QImage::Format_RGB888);
        break;

    case CV_16UC1: {  // 16 位单通道灰度图
        cv::Mat normalizedMat;
        // 将 16 位灰度图像转换为 8 位灰度图像
        mat.convertTo(normalizedMat, CV_8UC1, 255.0 / 65535.0);
        image = QImage(normalizedMat.data, normalizedMat.cols, normalizedMat.rows, normalizedMat.step, QImage::Format_Grayscale8);
        break;
    }

    default:  // 不支持的格式
        qDebug() << "Unsupported Mat format:" << mat.type();
        return QImage();
    }

    // 如果 QImage 构造完成，可以直接返回，无需 copy
    return image.copy();
}


void XViewer::run()
{
    while (m_isRun) {
        if (m_loadList.isEmpty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        m_mutex.lock();
        QString path = m_loadList.first();
        m_loadList.pop_front();
        m_mutex.unlock();

        if (m_images.contains(path))
            continue;

        emit startLoad();
        std::string std_path = path.toLocal8Bit();
        cv::Mat image_mat;
        if (std_path.compare(std_path.size() - 3, 3, "img") == 0) {
        }
        else {
            image_mat = imread(std_path, cv::IMREAD_UNCHANGED);
            if (image_mat.empty()) {
                emit stopLoad();
                continue;
            }
        }
        QImage image = MatToImage(image_mat);
        //QImage image(path);

        if (!image.isNull()) {
            QImage convertedImage;
            if ((image.format() == QImage::Format_RGB32) ||
                    (image.format() == QImage::Format_ARGB32) ||
                    (image.format() == QImage::Format_ARGB32_Premultiplied)) {
                convertedImage = image.convertToFormat(QImage::Format_RGB888);
                m_images.insert(path, convertedImage);
                emit newImageArrived(path, convertedImage);
            }
            else {
                m_images.insert(path, image);
                emit newImageArrived(path, image);
            }
        }
    }
}

void XViewer::open(QString path)
{
    if (path.isEmpty())
        return;

    m_mutex.lock();
    m_loadList << path;
    m_mutex.unlock();
}

void XViewer::remove()
{
    int current_row = m_listWidget->currentRow();
    QListWidgetItem* item = m_listWidget->item(current_row);
    if (item) {
        QString path = item->toolTip();
        m_images.remove(path);
        QListWidgetItem* removedItem = m_listWidget->takeItem(current_row);
        delete removedItem;
    }
}

void XViewer::clear()
{
    m_listWidget->clear();
    m_images.clear();

    m_view->inputImage(QImage());
    updateStatus();
}

void XViewer::previous()
{
    int current_row = m_listWidget->currentRow();

    if (current_row < 0)
        return;
    else if (current_row == 0) {
        return;
    }

    m_listWidget->setCurrentRow(current_row - 1);
}

void XViewer::next()
{
    int current_row = m_listWidget->currentRow();

    if (current_row < 0)
        return;

    if (current_row > (m_listWidget->count() - 1))
        return;
    else if (current_row == (m_listWidget->count() - 1)) {
        return;
    }

    m_listWidget->setCurrentRow(current_row + 1);
}

QImage XViewer::autoMaxContrastAndPseudoColor(QImage image)
{
    if (image.isNull())
        return image;

    if (!m_isAutoMax && !m_isPseudo)
        return image;

    // 单通道的时候才有效
    if (!(image.format() == QImage::Format_Indexed8 || image.format() == QImage::Format_Grayscale8))
        return image;

    QImage image_copy;

    // 非伪色彩，对比度增强
    if (!m_isPseudo && m_isAutoMax) {
        cv::Mat cv_image = QImage2Mat(image);
        cv::Mat normalize_mat;
        normalize(cv_image, normalize_mat, 0, 255, cv::NORM_MINMAX, -1);
        if (normalize_mat.type() != CV_8UC1)
            normalize_mat.convertTo(normalize_mat, CV_8UC1);

        image_copy = Mat2QImage(normalize_mat, true);
    }

    // 伪色彩，非对比度增强
    if (m_isPseudo && !m_isAutoMax) {
        cv::Mat cv_image = QImage2Mat(image);
        cv::Mat cv_8u_image;
        if (cv_image.type() == CV_8UC1)
            cv_8u_image = cv_image;
        else {
            cv::Mat normalize_mat;
            normalize(cv_image, normalize_mat, 0, 128, cv::NORM_MINMAX, -1);
            normalize_mat.convertTo(cv_8u_image, CV_8UC1);
        }
        cv::Mat dst;
        cv::applyColorMap(cv_8u_image, dst, cv::COLORMAP_JET);

        image_copy = Mat2QImage(dst, true);
    }

    // 伪色彩，对比度增强
    if (m_isPseudo && m_isAutoMax) {
        cv::Mat cv_image = QImage2Mat(image);
        cv::Mat normalize_mat;
        normalize(cv_image, normalize_mat, 0, 255, cv::NORM_MINMAX, -1);
        if (normalize_mat.type() != CV_8UC1)
            normalize_mat.convertTo(normalize_mat, CV_8UC1);

        cv::Mat dst;
        cv::applyColorMap(normalize_mat, dst, cv::COLORMAP_JET);

        image_copy = Mat2QImage(dst, true);
    }

    return image_copy;
}

QPixmap XViewer::generateIcon(QImage& image, QSize iconSize, QString str, QColor backC, QColor fontC)
{
    int length = 7 * str.length();
    iconSize.setWidth(cv::max(iconSize.width(), length + 10));

    QColor back_color = backC;
    QColor pen_color = fontC;

    // 生成图标对象
    QPixmap iconPixmap(iconSize);
    iconPixmap.fill(back_color);

    // 在图标上生成QPainter对象
    QPainter painter(&iconPixmap);
    painter.setPen(pen_color);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);//反走样

    // 设置字体：SimSun、大小15
    QFont font;
    font.setFamily("Calibri");
    font.setPointSize(10);
    painter.setFont(font);

    // 绘制图片名称
    QRect name_roi = QRect(2, 0, iconSize.width() - 10, 18);
    painter.drawText(name_roi, Qt::AlignLeft, str);

    // 绘制图像
    QPixmap image_pixmap = QPixmap::fromImage(image);
    QRect avail_roi(0, name_roi.y() + name_roi.height(), iconSize.width() / 5, iconSize.height() - (name_roi.y() + name_roi.height()));
    painter.fillRect(avail_roi, QColor(255, 255, 255));
    QRect draw_roi = GetRealShowRect(avail_roi, image.size());
    painter.drawPixmap(draw_roi, image_pixmap);

    // 绘制 image info
    std::stringstream ss;
    ss << image.width() << " x " << image.height() << "\n";

    QString info = QString::fromLocal8Bit((ss.str()).data()) + GetImageType(image);
    QRect info_roi(avail_roi.width() + 5, avail_roi.y(), iconSize.width() - avail_roi.width() - avail_roi.x() - 5, avail_roi.height());
    painter.drawText(info_roi, Qt::AlignLeft, info);

    return iconPixmap;
}

QPixmap XViewer::generateIcon(QImage& image, QSize iconSize)
{
    // 生成图标对象
    QPixmap iconPixmap(iconSize);

    // 在图标上生成QPainter对象
    QPainter painter(&iconPixmap);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);//反走样

    // 绘制图像
    QPixmap image_pixmap = QPixmap::fromImage(image);
    QRect avail_roi(0, 0, iconSize.width(), iconSize.height());
    painter.fillRect(avail_roi, QColor(255, 255, 255));
    QRect draw_roi = GetRealShowRect(avail_roi, image.size());
    painter.drawPixmap(draw_roi, image_pixmap);

    return iconPixmap;
}

void XViewer::viewHistogram(QRect rect)
{
    int row = m_listWidget->currentRow();
    if (row >= 0 && row < m_images.size()) {
        QListWidgetItem* item = m_listWidget->item(row);
        if (item) {
            QImage image = m_images.value(item->toolTip());
            cv::Mat mat = QImage2Mat(image);

            if (rect.isEmpty()) {
            }
            else {
                if (rect.x() > image.width() || rect.y() > image.height())
                    return;

                if (rect.x() < 0)
                    rect.setX(0);

                if (rect.y() < 0)
                    rect.setY(0);

                if (rect.x() + rect.width() > image.width())
                    rect.setWidth(image.width() - rect.x());

                if (rect.y() + rect.height() > image.height())
                    rect.setHeight(image.height() - rect.y());

                cv::Rect cvRect(rect.x(), rect.y(), rect.width(), rect.height());
                cv::Mat roiMat(mat, cvRect);
            }
        }
    }
}

void XViewer::test()
{
    // 创建一个 QElapsedTimer 对象
    QElapsedTimer timer;

    QString path = "D:\\code\\ImageViewer\\ImageViewer\\1 (1).bmp";
    open(path);
    //QImage image(path);//1478

    //QPixmap pixmap(path);//2235

//  	QImageReader reader;//2285 2214
// 	reader.setFileName(path);
// 	reader.setAutoTransform(true);
//  	auto src_size = reader.size();
// 	auto scale_size = src_size.scaled(QSize(400, 100), Qt::KeepAspectRatio);
//  	reader.setScaledSize(scale_size);

    //QImage image = reader.read();

    // 开始计时
    timer.start();

    QFileInfo fileInfo(path);
    QFileIconProvider iconProvider;

    QIcon fileIcon = iconProvider.icon(fileInfo);

    QListWidgetItem* item = new QListWidgetItem();
    item->setIcon(fileIcon);
    m_listWidget->addItem(item);

    //QPixmap pixmap = fileIcon.pixmap(100, 100);

    //QImage image = pixmap.toImage();
    //QPixmap pixmap = QPixmap::fromImageReader(&reader);

    // 停止计时并获取耗时时间
    qint64 elapsedTime = timer.elapsed(); // 返回纳秒级别的耗时时间
    // 输出耗时时间
    qDebug() << "Elapsed time:" << elapsedTime << "ms";
}

void XViewer::positionChanged(int x, int y)
{
    //char buf[255];
    //sprintf(buf, "(%d, %d)", x, y);
    //statusBar()->showMessage(buf);

    QString str = QString("(%1, %2)").arg(x).arg(y);
    m_posLabel->setText(str);
}

void XViewer::actionTriggered(QAction* triggeredAction)
{
    if (triggeredAction == nullptr)
        return;

    auto key = triggeredAction->text();

    if (key == "打开") {
        QString path = QFileDialog::getOpenFileName(this, "选择", "");
        open(path);
    }
    else if (key == "上一张") {
        previous();
    }
    else if (key == "下一张") {
        next();
    }
    else if (key == "删除这张") {
        remove();
    }
    else if (key == "清空") {
        clear();
    }
    else if (key == "链接视图") {
        m_view->setLinkView(triggeredAction->isChecked());
    }
    else if (key == "自动最大化对比度") {
        m_isAutoMax = triggeredAction->isChecked();
        itemChangedSlot();
    }
    else if (key == "1通道伪色彩") {
        m_isPseudo = triggeredAction->isChecked();
        itemChangedSlot();
    }
    else if (key == "局部直方图") {
        m_view->setMode(View::Rect);
    }
    else if (key == "全图直方图") {
        viewHistogram();
    }
    else if (key == "快速定位") {
        if (triggeredAction->isChecked()) {

        }
        else {

        }
    }
    else if (key == "顺时针旋转90°") {
        double revolve = m_view->getRevolve();
        m_view->setRevolve(revolve + 90);
    }
    else if (key == "逆时针旋转90°") {
        double revolve = m_view->getRevolve();
        if (revolve <= 0) {
            revolve = 360 + revolve;
        }
        m_view->setRevolve(revolve - 90);
    }
}

void XViewer::itemChangedSlot()
{
    int row = m_listWidget->currentRow();
    if (row >= 0 && row < m_images.size()) {
        QListWidgetItem* item = m_listWidget->item(row);
        if (item) {
            QImage image = m_images.value(item->toolTip());
            QImage img = autoMaxContrastAndPseudoColor(image);

            //			cv::Mat show = QImage2Mat(img);

            m_view->inputImage(img);
        }
    }
    else {
        m_view->inputImage(QImage());
    }

    updateStatus();
}

void XViewer::updateStatus()
{
    QString pic_size;
    QString pic_format;

    int value = m_view->getCurrentScale() * 100;
    QString strview_scale = QString("%1%").arg(value);
    m_scaleLabel->setText(strview_scale);

    int current_row = m_listWidget->currentRow();

    if (current_row >= 0 && current_row < m_images.size()) {
        QListWidgetItem* item = m_listWidget->item(current_row);
        if (item) {
            QImage image = m_images.value(item->toolTip());
            pic_size = QString("%1 x %2").arg(image.width()).arg(image.height());
        }
    }

    m_picSizeLabel->setText(pic_size);
    m_picFormatLabel->setText(pic_format);
}

void XViewer::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);

    if (m_foldButton->isChecked()) {
        m_foldButton->setGeometry(0, this->height() - LISTWIDGET_HEIGHT - FLOD_BTN_HEIGHT - this->statusBar()->height(), this->width(), FLOD_BTN_HEIGHT);
        m_listWidget->setGeometry(0, this->height() - LISTWIDGET_HEIGHT - this->statusBar()->height(), this->width(), LISTWIDGET_HEIGHT);
    }
    else {
        m_foldButton->setGeometry(0, this->height() - FLOD_BTN_HEIGHT - this->statusBar()->height(), this->width(), FLOD_BTN_HEIGHT);
        m_listWidget->setGeometry(0, this->height() + LISTWIDGET_HEIGHT, this->width(), LISTWIDGET_HEIGHT);
    }
}

void XViewer::contextMenuEvent(QContextMenuEvent* event)
{
    QMainWindow::contextMenuEvent(event);

    m_menu->popup(event->globalPos());
}

void XViewer::showEvent(QShowEvent* event)
{
    QMainWindow::showEvent(event);

    m_foldButton->setGeometry(0, this->height() - LISTWIDGET_HEIGHT - FLOD_BTN_HEIGHT - this->statusBar()->height(), this->width(), FLOD_BTN_HEIGHT);
    m_listWidget->setGeometry(0, this->height() - LISTWIDGET_HEIGHT - this->statusBar()->height(), this->width(), LISTWIDGET_HEIGHT);
}

void XViewer::mouseMoveEvent(QMouseEvent* event)
{
    // 处理鼠标移动事件
    QPoint pos = event->pos();

    // 在控制台输出鼠标位置
    qDebug() << "Mouse moved to:" << pos;

    QMainWindow::mouseMoveEvent(event);
}

bool XViewer::eventFilter(QObject* obj, QEvent* event)
{
    QGraphicsView* view = dynamic_cast<QGraphicsView*>(m_view);
    if (obj == view) {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent* key_event = dynamic_cast<QKeyEvent*>(event);
            if (key_event) {
                if (key_event->key() == Qt::Key_Left) {
                    previous();
                }
                else if (key_event->key() == Qt::Key_Right) {
                    next();
                }
                //获取组合键方式 Ctrl + O
                else if (key_event->modifiers() == (Qt::ControlModifier) && key_event->key() == Qt::Key_O) {
                    QString path = QFileDialog::getOpenFileName(this, "选择", "");
                    open(path);
                }

                // 返回 true 表示事件已经被处理
                return true;
            }
        }
    }

    return QWidget::eventFilter(obj, event);
}
