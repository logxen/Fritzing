/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2011 Fachhochschule Potsdam - http://fh-potsdam.de

Fritzing is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Fritzing is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Fritzing.  If not, see <http://www.gnu.org/licenses/>.

********************************************************************

$Revision: 1886 $:
$Author: merunga $:
$Date: 2008-12-18 19:17:13 +0100 (Thu, 18 Dec 2008) $

********************************************************************/

#include "terminalpointitem.h"
#include "../partseditor/partseditorconnectorsconnectoritem.h"
#include "../sketch/zoomablegraphicsview.h"
#include "../debugdialog.h"

QHash<ConnectorRectangle::State, QPixmap> TerminalPointItem::m_pixmapHash;

TerminalPointItem::TerminalPointItem(PartsEditorConnectorsConnectorItem *parent, bool visible)
	: QGraphicsRectItem(parent)
{
	init(parent,visible,rect().center(),false);
	reset(); // the first time
}

TerminalPointItem::TerminalPointItem(PartsEditorConnectorsConnectorItem *parent, bool visible, const QPointF &point)
	: QGraphicsRectItem(parent)
{
	init(parent,visible,point,true);
	m_isInTheCenter = false;
}

void TerminalPointItem::init(PartsEditorConnectorsConnectorItem *parent, bool visible, const QPointF &point, bool loadedFromFile) {
	if (parent == NULL) {
		throw "TerminalPointItem::init no parent";
	}

	m_parent = parent;
	m_point = point;
	m_loadedFromFile = loadedFromFile;
	m_hasBeenMoved = false;

	initPixmapHash();

	QPen pen = QPen();
	pen.setWidth(0);
	pen.setBrush(QBrush());
	setPen(pen);

	bool editable = parent->attachedTo()->viewIdentifier() != ViewIdentifierClass::PCBView;
	m_cross = new TerminalPointItemPrivate(this,editable);
	setMovable(editable);
	setVisible(visible);
	updatePoint();

	setFlag(QGraphicsItem::ItemIsMovable,false);
}

void TerminalPointItem::initPixmapHash() {
	if(m_pixmapHash.isEmpty()) {
		m_pixmapHash[ConnectorRectangle::Normal] =
			QPixmap(":/resources/images/itemselection/crosshairHandlerNormal.png");
		m_pixmapHash[ConnectorRectangle::Hover] =
		 	QPixmap(":/resources/images/itemselection/crosshairHandlerHover.png");
		m_pixmapHash[ConnectorRectangle::Selected] =
		 	QPixmap(":/resources/images/itemselection/crosshairHandlerActive.png");
	}
}

void TerminalPointItem::updatePoint() {
	setRect(m_parent->boundingRect());
	setCrossPos();
}

void TerminalPointItem::setCrossPos() {
	QRectF pRect = parentItem()->boundingRect();
	QPointF correction = transformedCrossCenter();

	QPointF point = m_point-correction+pRect.topLeft();
	m_cross->setPos(point);
}

void TerminalPointItem::setMovable(bool movable) {
	m_cross->setFlag(QGraphicsItem::ItemIsMovable, movable);
}

double TerminalPointItem::currentScale() {
	if(scene()) {
		ZoomableGraphicsView *sw = dynamic_cast<ZoomableGraphicsView*>(scene()->parent());
		if(sw) {
			return sw->currentZoom()/100;
		}
	}
	return 1;
}

void TerminalPointItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
	QGraphicsRectItem::paint(painter,option,widget);
}

QPointF TerminalPointItem::mappedPoint() {
	//bool result = m_cross->hasBeenMoved();
	//Q_ASSERT(result);

	QPointF p = m_cross->mapToItem(m_parent,transformedCrossCenter());
	const PartsEditorConnectorsConnectorItem *pci = parentConnectorItem();
	//if(pci->hasBeenMoved()) {
		p += m_cross->mapToItem(m_parent,pci->pos())
			-m_cross->mapToItem(m_parent,pci->initialPos());
	//}
	return p;
}

void TerminalPointItem::setPoint(const QPointF &point) {
	m_point = point;
}

// because the cross pixmap doesn't accepts transformations
QPointF TerminalPointItem::transformedCrossCenter() {
	return QMatrix()
			.scale(currentScale(),currentScale())
			.inverted()
			.map(m_cross->boundingRect().center());
}

bool TerminalPointItem::isInTheCenter() {
	return m_isInTheCenter;
}


void TerminalPointItem::reset() {
	if(!m_isInTheCenter) {
		QRectF pRect = parentItem()->boundingRect();
		setRect(pRect);
		setPoint(pRect.center()-pRect.topLeft());
		setHasBeenMoved(m_loadedFromFile);
		m_loadedFromFile = false;
		setCrossPos();
		if(scene()) scene()->update();
		m_isInTheCenter = true;
	}
}

void TerminalPointItem::doSetVisible(bool visible) {
	if(visible && !hasBeenMoved()) {
		setCrossPos();
	}
	setVisible(visible);
}

bool TerminalPointItem::isOutsideConnector() {
	return m_cross->isOutsideConnector();
}

const PartsEditorConnectorsConnectorItem* TerminalPointItem::parentConnectorItem() {
	return m_parent;
}

bool TerminalPointItem::loadedFromFile() {
	return m_loadedFromFile;
}

bool TerminalPointItem::hasBeenMoved() {
	return m_hasBeenMoved;
}

void TerminalPointItem::setHasBeenMoved(bool moved) {
	m_hasBeenMoved = moved;
	if(moved) m_isInTheCenter = false;
}

/////////////////////////////////////////////////////////////////////////

TerminalPointItemPrivate::TerminalPointItemPrivate(TerminalPointItem *parent, bool editable)
	: QGraphicsPixmapItem(parent)
{
	m_parent = parent;
	m_pressed = false;
	m_editable = editable;
	setAcceptHoverEvents(true);
	setFlag(QGraphicsItem::ItemIgnoresTransformations);
	setPixmap(m_parent->m_pixmapHash[ConnectorRectangle::Normal]);
	setFlag(QGraphicsItem::ItemIsMovable, m_editable);
}

bool TerminalPointItemPrivate::isPressed() {
	return m_pressed;
}

bool TerminalPointItemPrivate::isOutsideConnector() {
	bool outside = !m_parent->rect().contains(
		mapToParent(m_parent->transformedCrossCenter())
	);
	return outside;
}

void TerminalPointItemPrivate::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
	QGraphicsPixmapItem::paint(painter,option,widget);
}

void TerminalPointItemPrivate::hoverEnterEvent(QGraphicsSceneHoverEvent *event) {
	if(isVisible()) {
		if(m_editable) {
			setPixmap(m_parent->m_pixmapHash[ConnectorRectangle::Hover]);
			setCursor(QCursor(Qt::SizeAllCursor));
		} else {
			setCursor(QCursor(Qt::ForbiddenCursor));
		}
	}
	QGraphicsPixmapItem::hoverEnterEvent(event);
}

void TerminalPointItemPrivate::hoverLeaveEvent(QGraphicsSceneHoverEvent *event) {
	if(isVisible()) {
		if(m_editable) {
			setPixmap(m_parent->m_pixmapHash[ConnectorRectangle::Normal]);
		} else {
			unsetCursor();
		}
	}
	QGraphicsPixmapItem::hoverLeaveEvent(event);
}

void TerminalPointItemPrivate::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
	if(m_editable && isVisible()) {
		if(isOutsideConnector()) {
			setCursor(QCursor(Qt::ForbiddenCursor));
		} else {
			m_parent->setHasBeenMoved(m_pressed);
			setCursor(Qt::BlankCursor);
		}
		QGraphicsPixmapItem::mouseMoveEvent(event);
	}
}

void TerminalPointItemPrivate::mousePressEvent(QGraphicsSceneMouseEvent *event) {
	if(m_editable && isVisible()) {
		setPixmap(m_parent->m_pixmapHash[ConnectorRectangle::Selected]);
		m_pressed = true;
		setCursor(Qt::BlankCursor);
	}
	QGraphicsPixmapItem::mousePressEvent(event);
}

void TerminalPointItemPrivate::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
	if(m_editable && isVisible()) {
		if(isOutsideConnector()) {
			unsetCursor();
			m_parent->reset();
			return;
		} else {
			unsetCursor();
			setPixmap(m_parent->m_pixmapHash[ConnectorRectangle::Hover]);
			m_pressed = false;
		}
	}
	QGraphicsPixmapItem::mouseReleaseEvent(event);
}
