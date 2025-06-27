#pragma once

#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>

class Scene  : public QGraphicsScene
{
	Q_OBJECT

public:
	Scene(QObject *parent = nullptr);
	~Scene();

	void setView(QGraphicsView* view) { m_view = view; }
	QGraphicsView* view() { return m_view; }

protected:
	void drawBackground(QPainter* painter, const QRectF& rect) override;

private:
	QGraphicsView* m_view = nullptr;

};
