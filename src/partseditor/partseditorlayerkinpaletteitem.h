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

$Revision: 5309 $:
$Author: cohen@irascible.com $:
$Date: 2011-07-30 12:17:22 -0700 (Sat, 30 Jul 2011) $

********************************************************************/

#ifndef PARTSEDITORLAYERKINPALETTEITEM_H_
#define PARTSEDITORLAYERKINPALETTEITEM_H_

#include "partseditorconnectoritem.h"
#include "../items/layerkinpaletteitem.h"

class PartsEditorLayerKinPaletteItem : public LayerKinPaletteItem {
public:
	PartsEditorLayerKinPaletteItem(
		PaletteItemBase * chief, ModelPart * modelPart, ViewIdentifierClass::ViewIdentifier viewIdentifier,const ViewGeometry & viewGeometry,
                long id, QMenu* itemMenu)
                : LayerKinPaletteItem(chief, modelPart, viewIdentifier, viewGeometry, id, itemMenu)
	{
	}
protected:
	ConnectorItem* newConnectorItem(Connector *connector) {
		return new PartsEditorConnectorItem(connector,this);
	}

	void hoverEnterEvent(QGraphicsSceneHoverEvent *event) {
		setCursor(QCursor(Qt::OpenHandCursor));
		QGraphicsSvgItem::hoverEnterEvent(event);
	}

	void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) {
		QGraphicsSvgItem::hoverLeaveEvent(event);
	}
};

#endif /* PARTSEDITORLAYERKINPALETTEITEM_H_ */
