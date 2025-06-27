#include "Scene.h"
#include <QGraphicsItem>

Scene::Scene(QObject *parent)
	: QGraphicsScene(parent)
{}

Scene::~Scene()
{}

void Scene::drawBackground(QPainter* painter, const QRectF& rect)
{
	QGraphicsScene::drawBackground(painter, rect);
}
