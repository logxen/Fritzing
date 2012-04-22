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

$Revision$:
$Author$:
$Date$

********************************************************************/

#ifndef PARTSEDITORSKETCHWIDGET_H
#define PARTSEDITORSKETCHWIDGET_H
//
#include "../sketch/sketchwidget.h"
#include "partseditorpaletteitem.h"
//
class PartsEditorSketchWidget : public SketchWidget
{

Q_OBJECT

public:
    PartsEditorSketchWidget(ViewIdentifierClass::ViewIdentifier, QWidget *parent=0, int size=300, int minSize=100);
	void mousePressConnectorEvent(ConnectorItem *, QGraphicsSceneMouseEvent *);
	void loadSvgFile(StringPair *path, ModelPart * modelPart, ViewIdentifierClass::ViewIdentifier viewIdentifier, QString layer);

signals:
	void connectorsFound(ViewIdentifierClass::ViewIdentifier viewId, QStringList connNames);

protected:
	void clearScene();
	ItemBase * addItemAux(ModelPart * modelPart, const ViewGeometry & viewGeometry, long id, PaletteItem* paletteItem, bool doConnectors);
};
#endif
