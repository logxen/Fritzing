/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2010 Fachhochschule Potsdam - http://fh-potsdam.de

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


#include "partseditorconnectoritem.h"
#include "../debugdialog.h"

PartsEditorConnectorItem::PartsEditorConnectorItem(Connector *conn, ItemBase *attachedTo)
	: ConnectorItem(conn, attachedTo)
{

}

PartsEditorConnectorItem::~PartsEditorConnectorItem() {
	/*if (m_connector) {
		delete m_connector;
		m_connector = NULL;
	}*/
}

void PartsEditorConnectorItem::removeFromModel() {
	m_connector->removeViewItem(this);
}

void PartsEditorConnectorItem::hoverEnterEvent(QGraphicsSceneHoverEvent *event) {
	setCursor(QCursor(Qt::ArrowCursor));
	QGraphicsRectItem::hoverEnterEvent(event);
}

void PartsEditorConnectorItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event) {
	QGraphicsRectItem::hoverLeaveEvent(event);
}

void PartsEditorConnectorItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
	QGraphicsRectItem::mousePressEvent(event);
}
