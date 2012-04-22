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

$Revision$:
$Author$:
$Date$

********************************************************************/


#ifndef PARTSEDITORCONNECTORSCONNECTORITEM_H_
#define PARTSEDITORCONNECTORSCONNECTORITEM_H_

#include "partseditorconnectoritem.h"

class PartsEditorConnectorsConnectorItem : public PartsEditorConnectorItem {
	Q_OBJECT

public:
	PartsEditorConnectorsConnectorItem(Connector * conn, ItemBase* attachedTo, bool showingTerminalPoint);
	PartsEditorConnectorsConnectorItem(Connector * conn, ItemBase* attachedTo, bool showingTerminalPoint, const QRectF &bounds);
	~PartsEditorConnectorsConnectorItem();

	void highlight(const QString &connId);
	void setConnector(Connector *connector);
	void setMismatching(bool isMismatching);

	void setShowTerminalPoint(bool show);
	bool isShowingTerminalPoint();
	void setTerminalPoint(QPointF);
	void updateTerminalPoint();
	TerminalPointItem *terminalPointItem();

	double minWidth();
	double minHeight();

	QRectF mappedRect();

	bool hasBeenMoved() const;
	bool hasBeenResized() const;
	QPointF initialPos() const;

protected slots:
	void isResizableSlot(bool & resizable);
	void resizeSlot(double x1, double y1, double x2, double y2);
	void createTerminalPoint();

protected:
	void init(bool resizable);
	void mousePressEvent(QGraphicsSceneMouseEvent *event);
	void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

	void resizeRect(double x, double y, double width, double height);
	void setResizable(bool resizable);

	void showErrorIcon(bool showIt);
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
	QVariant itemChange(GraphicsItemChange change, const QVariant &value);

	void drawDottedRect(QPainter *painter, const QColor &color1, const QColor &color2, const QRectF &rect);
	QPen drawDottedLine(
		Qt::Orientations orientation, QPainter *painter, const QPen &pen1, const QPen &pen2,
		double pos1, double pos2, double fixedAxis, const QPen &lastUsedPen = QPen()
	);
	QPen drawDottedLineAux(
		Qt::Orientations orientation, QPainter *painter, const QPen &firstPen, const QPen &secondPen,
		double pos, double fixedAxis, double dotSize, int dotCount
	);

	void informChange();
	void doPrepareGeometryChange();

	void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
	void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);

	void paintErrorIcon(QPainter *painter);
	TerminalPointItem* newTerminalPointItem();

	bool m_showErrorIcon;
	bool m_showingTerminalPoint; // important only if m_showsTerminalPoints == true
	TerminalPointItem *m_terminalPointItem;

	QRectF m_resizedRect;
	bool m_geometryHasChanged;
	bool m_geometryHasChangedAlLeastOnce;
	bool m_inFileDefined;

	class ConnectorRectangle *m_handlers;
	bool m_resizable;
	bool m_resized;
	QPointF m_initialPos;

	static double MinWidth;
	static double MinHeight;
};

#endif /* PARTSEDITORCONNECTORSCONNECTORITEM_H_ */
