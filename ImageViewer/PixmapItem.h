#pragma once

#include <QGraphicsItem>
#include <QPainter>

class PixmapItem  : public QGraphicsPixmapItem
{
public:
	PixmapItem();
	~PixmapItem();

	void setImage(QImage image) { m_image = image; }
	QString getPixelValue(QPoint pos);

protected:
	void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget /* = nullptr */) override;

private:
	QString getPixelValue(int x, int y);

private:
	QImage m_image;

};
