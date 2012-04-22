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

$Revision: 5945 $:
$Author: cohen@irascible.com $:
$Date: 2012-04-06 15:49:26 -0700 (Fri, 06 Apr 2012) $

********************************************************************/


#ifndef SCHEMATICSKETCHWIDGET_H
#define SCHEMATICSKETCHWIDGET_H

#include "pcbsketchwidget.h"

class SchematicSketchWidget : public PCBSketchWidget
{
	Q_OBJECT

public:
    SchematicSketchWidget(ViewIdentifierClass::ViewIdentifier, QWidget *parent=0);

	void addViewLayers();
	ViewLayer::ViewLayerID getWireViewLayerID(const ViewGeometry & viewGeometry, ViewLayer::ViewLayerSpec);
	ViewLayer::ViewLayerID getDragWireViewLayerID(ConnectorItem *);
	void initWire(Wire *, int penWidth);
	bool autorouteTypePCB();
	double getKeepout();
	void tidyWires();
	void ensureTraceLayersVisible();
	void ensureTraceLayerVisible();
	bool usesJumperItem();
	void setClipEnds(ClipableWire * vw, bool);
	void getBendpointWidths(class Wire *, double w, double & w1, double & w2, bool & negativeOffsetRect);
	void getLabelFont(QFont &, QColor &, ViewLayer::ViewLayerSpec);
	void setNewPartVisible(ItemBase *);
	bool canDropModelPart(ModelPart * modelPart); 
	bool includeSymbols();
	bool hasBigDots();
	void changeConnection(long fromID,
						  const QString & fromConnectorID,
						  long toID, const QString & toConnectorID,
						  ViewLayer::ViewLayerSpec,
						  bool connect, bool doEmit, 
						  bool updateConnections);
	double defaultGridSizeInches();
	const QString & traceColor(ConnectorItem * forColor);
	const QString & traceColor(ViewLayer::ViewLayerSpec);
	long setUpSwap(ItemBase *, long newModelIndex, const QString & newModuleID,  ViewLayer::ViewLayerSpec, bool doEmit, bool noFinalChangeWiresCommand, QList<Wire *> & wiresToDelete, QUndoCommand * parentCommand);
	bool isInLayers(ConnectorItem *, ViewLayer::ViewLayerSpec);
	bool routeBothSides();
	void addDefaultParts();
	bool sameElectricalLayer2(ViewLayer::ViewLayerID, ViewLayer::ViewLayerID);
	bool acceptsTrace(const ViewGeometry &);
	ViewGeometry::WireFlag getTraceFlag();
	double getTraceWidth();
	double getAutorouterTraceWidth();
	QString generateCopperFillUnit(ItemBase * itemBase, QPointF whereToStart);
	double getWireStrokeWidth(Wire *, double wireWidth);
	Wire * createTempWireForDragging(Wire * fromWire, ModelPart * wireModel, ConnectorItem * connectorItem, ViewGeometry & viewGeometry, ViewLayer::ViewLayerSpec);
	void rotatePartLabels(double degrees, QTransform &, QPointF center, QUndoCommand * parentCommand);
	void loadFromModelParts(QList<ModelPart *> & modelParts, BaseCommand::CrossViewType, QUndoCommand * parentCommand, 
							bool offsetPaste, const QRectF * boundingRect, bool seekOutsideConnections, QList<long> & newIDs);

public slots:
	void setVoltage(double voltage, bool doEmit);

protected slots:
	void updateBigDots();

protected:
	double getRatsnestOpacity();
	double getRatsnestWidth();
	ViewLayer::ViewLayerID getLabelViewLayerID(ViewLayer::ViewLayerSpec);
	QPoint calcFixedToCenterItemOffset(const QRect & viewPortRect, const QSizeF & helpSize);
	void extraRenderSvgStep(ItemBase *, QPointF offset, double dpi, double printerScale, QString & outputSvg);
	QString makeCircleSVG(QPointF p, double r, QPointF offset, double dpi, double printerScale);
	ViewLayer::ViewLayerSpec createWireViewLayerSpec(ConnectorItem * from, ConnectorItem * to);


protected:
	QTimer m_updateDotsTimer;

};

#endif
