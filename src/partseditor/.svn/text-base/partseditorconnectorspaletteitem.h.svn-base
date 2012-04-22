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


#ifndef PARTSEDITORCONNECTORSPALETTEITEM_H_
#define PARTSEDITORCONNECTORSPALETTEITEM_H_

#include "partseditorpaletteitem.h"

class PartsEditorView;

class PartsEditorConnectorsPaletteItem : public PartsEditorPaletteItem {
	Q_OBJECT
	public:
		PartsEditorConnectorsPaletteItem(PartsEditorView *owner, ModelPart *modelPart, ViewIdentifierClass::ViewIdentifier viewIdentifier);
		PartsEditorConnectorsPaletteItem(PartsEditorView *owner, ModelPart *modelPart, ViewIdentifierClass::ViewIdentifier viewIdentifier, SvgAndPartFilePath *path);

	public slots:
		void highlightConnectors(const QString &connId);

	protected:
		void highlightConnsAux(ItemBase* item, const QString &connId);
		ConnectorItem* newConnectorItem(Connector *connector);
		LayerKinPaletteItem * newLayerKinPaletteItem(
			PaletteItemBase * chief, ModelPart * modelPart, ViewIdentifierClass::ViewIdentifier viewIdentifier,
			const ViewGeometry & viewGeometry, long id,ViewLayer::ViewLayerID, ViewLayer::ViewLayerSpec, QMenu* itemMenu, const LayerHash & viewLayers
		);
		bool isShowingTerminalPoints();

		void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
		void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);

		bool m_showingTerminalPoints;
};

#endif /* PARTSEDITORCONNECTORSPALETTEITEM_H_ */
