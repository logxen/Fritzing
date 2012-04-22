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


#ifndef PALETTEITEM_H
#define PALETTEITEM_H

#include <QRectF>
#include <QPainterPath>
#include <QPixmap>
#include <QVariant>

#include "paletteitembase.h"
#include "../viewlayer.h"

class PaletteItem : public PaletteItemBase 
{
	Q_OBJECT

public:
	// after calling this constructor if you want to render the loaded svg (either from model or from file), MUST call <renderImage>
	PaletteItem(ModelPart *, ViewIdentifierClass::ViewIdentifier, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu, bool doLabel);
	~PaletteItem();

	void removeLayerKin();
	void addLayerKin(class LayerKinPaletteItem * lkpi);
	const QList<class ItemBase *> & layerKin();
 	virtual void loadLayerKin(const LayerHash & viewLayers, ViewLayer::ViewLayerSpec);
	void rotateItem(double degrees);
	void flipItem(Qt::Orientations orientation);
	void moveItem(ViewGeometry & viewGeometry);
	void transformItem2(const QMatrix &);
	void setItemPos(QPointF & pos);

	bool renderImage(ModelPart * modelPart, ViewIdentifierClass::ViewIdentifier viewIdentifier, const LayerHash & viewLayers, ViewLayer::ViewLayerID, bool doConnectors, QString & error);

	void setTransforms();
	void syncKinMoved(QPointF offset, QPointF loc);

	void setInstanceTitle(const QString&);

	bool swap(ModelPart* newModelPart, const LayerHash &layerHash, bool reinit, class SwapCommand *);
	QString family();
	void setHidden(bool hidden);
	void setInactive(bool inactivate);
	bool collectFemaleConnectees(QSet<ItemBase *> & items);
	void collectWireConnectees(QSet<class Wire *> & wires);
	void clearModelPart();
	void mousePressEvent(PaletteItemBase * originalItem, QGraphicsSceneMouseEvent *);
	ItemBase * lowerConnectorLayerVisible(ItemBase *);
	void resetID();
	void slamZ(double z);
	void resetImage(class InfoGraphicsView *);
	void resetKinImage(ItemBase * layerKin, InfoGraphicsView * infoGraphicsView);
	bool collectExtraInfo(QWidget * parent, const QString & family, const QString & prop, const QString & value, bool swappingEnabled, QString & returnProp, QString & returnValue, QWidget * & returnWidget);
	virtual bool changePinLabels(bool singleRow, bool sip);
	QStringList getPinLabels(bool & hasLocal);
	bool loadExtraRenderer(const QString & svg, bool fastload);
	void renamePins(const QStringList & labels, bool singleRow);
	void resetConnectors();
	void resetConnectors(ItemBase * otherLayer, FSvgRenderer * otherLayerRenderer);
	void resetConnector(ItemBase * itemBase, SvgIdLayer * svgIdLayer);


public:
	static QString genFZP(const QString & moduleid, const QString & templateName, int minPins, int maxPins, int steps, bool smd);

signals:
	void pinLabelSwap(ItemBase *, const QString & moduleID);

protected slots:
	void openPinLabelDialog();

protected:
	void syncKinSelection(bool selected, PaletteItemBase * originator);
 	QVariant itemChange(GraphicsItemChange change, const QVariant &value);
	void updateConnections();
	void mousePressEvent(QGraphicsSceneMouseEvent *event);
	void figureHover();
	bool isSingleRow(QList<ConnectorItem *> & connectorItems);
	QList<ConnectorItem *> sortConnectorItems();

protected:
 	QList<class ItemBase *> m_layerKin;
	QPointer<class FSvgRenderer> m_extraRenderer;

};

#endif
