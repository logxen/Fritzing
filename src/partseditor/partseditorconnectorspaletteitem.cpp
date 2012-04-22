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

$Revision: 4206 $:
$Author: cohen@irascible.com $:
$Date: 2010-06-02 11:15:36 -0700 (Wed, 02 Jun 2010) $

********************************************************************/


#include "partseditorconnectorspaletteitem.h"
#include "partseditorconnectorsconnectoritem.h"
#include "partseditorview.h"
#include "partseditorconnectorslayerkinpaletteitem.h"


PartsEditorConnectorsPaletteItem::PartsEditorConnectorsPaletteItem(PartsEditorView *owner, ModelPart * modelPart, ViewIdentifierClass::ViewIdentifier viewIdentifier)
	: PartsEditorPaletteItem(owner, modelPart, viewIdentifier)
{
	m_showingTerminalPoints = owner->showingTerminalPoints();
	setAcceptHoverEvents(true);
}

PartsEditorConnectorsPaletteItem::PartsEditorConnectorsPaletteItem(PartsEditorView *owner, ModelPart * modelPart, ViewIdentifierClass::ViewIdentifier viewIdentifier, SvgAndPartFilePath *path)
	: PartsEditorPaletteItem(owner, modelPart, viewIdentifier, path)
{
	m_showingTerminalPoints = owner->showingTerminalPoints();
}

void PartsEditorConnectorsPaletteItem::highlightConnectors(const QString &connId) {
	highlightConnsAux(this,connId);
	foreach(ItemBase* item, m_layerKin) {
		highlightConnsAux(item,connId);
	}
}

void PartsEditorConnectorsPaletteItem::highlightConnsAux(ItemBase* item, const QString &connId) {
	foreach(QGraphicsItem * child, item->childItems()) {
		PartsEditorConnectorsConnectorItem * connectorItem
			= dynamic_cast<PartsEditorConnectorsConnectorItem *>(child);
		if (connectorItem == NULL) continue;

		connectorItem->highlight(connId);
	}
}

bool PartsEditorConnectorsPaletteItem::isShowingTerminalPoints() {
	return dynamic_cast<PartsEditorView*>(m_owner)->showingTerminalPoints();
}

ConnectorItem* PartsEditorConnectorsPaletteItem::newConnectorItem(Connector *connector) {
	return new PartsEditorConnectorsConnectorItem(connector,this,m_showingTerminalPoints);
}

LayerKinPaletteItem * PartsEditorConnectorsPaletteItem::newLayerKinPaletteItem(PaletteItemBase * chief, ModelPart * modelPart, 
																			   ViewIdentifierClass::ViewIdentifier viewIdentifier,
																			   const ViewGeometry & viewGeometry, long id,
																			   ViewLayer::ViewLayerID viewLayerID, 
																			   ViewLayer::ViewLayerSpec viewLayerSpec, 
																			   QMenu* itemMenu, const LayerHash & viewLayers)
{
	LayerKinPaletteItem *lk = new
                PartsEditorConnectorsLayerKinPaletteItem(chief, modelPart, viewIdentifier, viewGeometry, id, itemMenu, m_showingTerminalPoints);
	lk->init(viewLayerID, viewLayerSpec, viewLayers);
	return lk;
}

void PartsEditorConnectorsPaletteItem::hoverEnterEvent(QGraphicsSceneHoverEvent *event) {
	Q_UNUSED(event);
	setCursor(QCursor(Qt::OpenHandCursor));
}

void PartsEditorConnectorsPaletteItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event) {
	Q_UNUSED(event);
	unsetCursor();
}
