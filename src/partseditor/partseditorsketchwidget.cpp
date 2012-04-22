/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2009 Fachhochschule Potsdam - http://fh-potsdam.de

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

$Revision: 2412 $:
$Author: merunga $:
$Date: 2009-02-18 03:34:49 -0800 (Wed, 18 Feb 2009) $

********************************************************************/

#include "partseditorsketchwidget.h"
#include "partseditorpaletteitem.h"
#include "../layerkinpaletteitem.h"
#include "../debugdialog.h"

QT_BEGIN_NAMESPACE

PartsEditorSketchWidget::PartsEditorSketchWidget(ItemBase::ViewIdentifier viewIdentifier, QWidget *parent, int size, int minSize)
	: SketchWidget(viewIdentifier, parent, size, minSize)
{

}

void PartsEditorSketchWidget::mousePressConnectorEvent(ConnectorItem *, QGraphicsSceneMouseEvent *){
	DebugDialog::debug("got connector mouse press.  not yet implemented...");
	return;
}

void PartsEditorSketchWidget::loadSvgFile(StringPair *path, ModelPart * modelPart, ItemBase::ViewIdentifier viewIdentifier, QString layer) {
	PartsEditorPaletteItem * item = new PartsEditorPaletteItem(NULL,modelPart, viewIdentifier, path, layer, false);
	if(item->connectors().size() > 0) {
		emit connectorsFound(this->m_viewIdentifier,item->connectors());
	}
	this->addItem(modelPart, BaseCommand::CrossView, item->getViewGeometry(),item->id(), -1, item);
}

ItemBase * PartsEditorSketchWidget::addItemAux(ModelPart * modelPart, const ViewGeometry & /*viewGeometry*/, long /*id*/, PaletteItem * paletteItem, bool doConnectors)
{
	if(paletteItem == NULL) {
		//paletteItem = new PartsEditorPaletteItem(NULL,modelPart, m_viewIdentifier, false);
	}
	modelPart->initConnectors();    // is a no-op if connectors already in place
	return addPartItem(modelPart, paletteItem, doConnectors);
}

void PartsEditorSketchWidget::clearScene() {
	QGraphicsScene * scene = this->scene();
	//QList<QGraphicsItem*> items;
	for(int i=0; i < scene->items().size(); i++){
		ItemBase * itemBase = ItemBase::extractTopLevelItemBase(scene->items()[i]);
		if (itemBase == NULL) {
			//items << scene->items()[i];
			continue;
		}

		this->deleteItem(itemBase, true, true);
	}

	/*for(int i=0; i < items.size(); i++) {
		scene->removeItem(items[i]);
	}*/
}
