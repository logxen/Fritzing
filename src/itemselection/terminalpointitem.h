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


#ifndef TERMINALPOINTITEM_H_
#define TERMINALPOINTITEM_H_

#include <QGraphicsRectItem>
#include "connectorrectangle.h"

class PartsEditorConnectorsConnectorItem;
class TerminalPointItem;

class TerminalPointItemPrivate: public QGraphicsPixmapItem {
public:
	TerminalPointItemPrivate(TerminalPointItem *parent, bool editable);
	bool isOutsideConnector();
	bool hasBeenMoved();
	void setHasBeenMoved(bool moved);
	bool isPressed();
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget=0);

protected:

	void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
	void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);
	void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
	void mousePressEvent(QGraphicsSceneMouseEvent *event);
	void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

	TerminalPointItem *m_parent;
	bool m_pressed;
	bool m_editable;
};

class TerminalPointItem : public QGraphicsRectItem {
friend class TerminalPointItemPrivate;
public:
	TerminalPointItem(PartsEditorConnectorsConnectorItem *parent, bool visible);
	TerminalPointItem(PartsEditorConnectorsConnectorItem *parent, bool visible, const QPointF &point);

	void updatePoint();
	bool hasBeenMoved();
	void setHasBeenMoved(bool moved);
	bool isInTheCenter();

	void setMovable(bool movable);
	QPointF mappedPoint();
	void setPoint(const QPointF &point);

	void reset();
	void doSetVisible(bool visible);

	bool isOutsideConnector();
	bool loadedFromFile();
	const PartsEditorConnectorsConnectorItem* parentConnectorItem();

protected:
	void init(PartsEditorConnectorsConnectorItem *parent, bool visible, const QPointF &point, bool centered);
	void initPixmapHash();
	void setCrossPos();
	double currentScale();

	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget=0);

	QPointF transformedCrossCenter();

	QPointF m_point;
	bool m_loadedFromFile;
	TerminalPointItemPrivate *m_cross;
	PartsEditorConnectorsConnectorItem *m_parent;
	bool m_isInTheCenter;
	bool m_hasBeenMoved;

	static QHash<ConnectorRectangle::State, QPixmap> m_pixmapHash;
};

#endif /* TERMINALPOINTITEM_H_ */
