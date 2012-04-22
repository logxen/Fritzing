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

$Revision: 5909 $:
$Author: cohen@irascible.com $:
$Date: 2012-03-10 05:48:44 -0800 (Sat, 10 Mar 2012) $

********************************************************************/

#include "breadboardsketchwidget.h"
#include "../debugdialog.h"
#include "../items/virtualwire.h"
#include "../connectors/connectoritem.h"
#include "../items/moduleidnames.h"
#include "../waitpushundostack.h"
#include "../help/sketchmainhelp.h"

#include <QScrollBar>

static const double WireHoverStrokeFactor = 4.0;

BreadboardSketchWidget::BreadboardSketchWidget(ViewIdentifierClass::ViewIdentifier viewIdentifier, QWidget *parent)
    : SketchWidget(viewIdentifier, parent)
{
	m_shortName = QObject::tr("bb");
	m_viewName = QObject::tr("Breadboard View");
	initBackgroundColor();
}

void BreadboardSketchWidget::setWireVisible(Wire * wire)
{
	bool visible = !(wire->getTrace());
	wire->setVisible(visible);
	wire->setEverVisible(visible);
	//wire->setVisible(true);					// for debugging
}

bool BreadboardSketchWidget::collectFemaleConnectees(ItemBase * itemBase, QSet<ItemBase *> & items) {
	return itemBase->collectFemaleConnectees(items);
}

bool BreadboardSketchWidget::checkUnder() {
	return true;
};

void BreadboardSketchWidget::findConnectorsUnder(ItemBase * item) {
	item->findConnectorsUnder();
}

void BreadboardSketchWidget::addViewLayers() {
	addBreadboardViewLayers();
}

bool BreadboardSketchWidget::disconnectFromFemale(ItemBase * item, QHash<long, ItemBase *> & savedItems, ConnectorPairHash & connectorHash, bool doCommand, bool rubberBandLegEnabled, QUndoCommand * parentCommand)
{
	// if item is attached to a virtual wire or a female connector in breadboard view
	// then disconnect it
	// at the moment, I think this doesn't apply to other views

	bool result = false;
	foreach (ConnectorItem * fromConnectorItem, item->cachedConnectorItems()) {
		if (rubberBandLegEnabled && fromConnectorItem->hasRubberBandLeg()) continue;

		foreach (ConnectorItem * toConnectorItem, fromConnectorItem->connectedToItems())  {
			if (rubberBandLegEnabled && toConnectorItem->hasRubberBandLeg()) continue;

			if (toConnectorItem->connectorType() == Connector::Female) {
				if (savedItems.keys().contains(toConnectorItem->attachedTo()->layerKinChief()->id())) {
					// the thing we're connected to is also moving, so don't disconnect
					continue;
				}

				result = true;
				fromConnectorItem->tempRemove(toConnectorItem, true);
				toConnectorItem->tempRemove(fromConnectorItem, true);
				if (doCommand) {
					extendChangeConnectionCommand(BaseCommand::CrossView, fromConnectorItem, toConnectorItem, ViewLayer::Bottom, false, parentCommand);
				}
				connectorHash.insert(fromConnectorItem, toConnectorItem);

			}
		}
	}

	return result;
}


BaseCommand::CrossViewType BreadboardSketchWidget::wireSplitCrossView()
{
	return BaseCommand::CrossView;
}

bool BreadboardSketchWidget::canDropModelPart(ModelPart * modelPart) {	
	switch (modelPart->itemType()) {
		case ModelPart::Board:
		case ModelPart::ResizableBoard:
			return matchesLayer(modelPart);
		case ModelPart::Logo:
		case ModelPart::Symbol:
		case ModelPart::Jumper:
		case ModelPart::CopperFill:
		case ModelPart::Hole:
		case ModelPart::Via:
			return false;
		default:
			if (modelPart->moduleID().endsWith(ModuleIDNames::SchematicFrameModuleIDName)) return false;
			if (modelPart->moduleID().endsWith(ModuleIDNames::PadModuleIDName)) return false;
			return true;
	}
}

void BreadboardSketchWidget::initWire(Wire * wire, int penWidth) {
	if (wire->getRatsnest()) {
		// handle elsewhere
		return;
	}
	wire->setPenWidth(penWidth - 2, this, (penWidth - 2) * WireHoverStrokeFactor);
	wire->setColorString("blue", 1.0);
}

const QString & BreadboardSketchWidget::traceColor(ViewLayer::ViewLayerSpec) {
	if (!m_lastColorSelected.isEmpty()) return m_lastColorSelected;

	static QString color = "blue";
	return color;
}

double BreadboardSketchWidget::getTraceWidth() {
	// TODO: dig this constant out of wire.svg or somewhere else...
	return 2.0;
}

void BreadboardSketchWidget::getLabelFont(QFont & font, QColor & color, ViewLayer::ViewLayerSpec) {
	font.setFamily("Droid Sans");
	font.setPointSize(9);
	font.setBold(false);
	font.setItalic(false);
	color.setAlpha(255);
	color.setRgb(0);
}

void BreadboardSketchWidget::setNewPartVisible(ItemBase * itemBase) {
	switch (itemBase->itemType()) {
		case ModelPart::Symbol:
		case ModelPart::Jumper:
		case ModelPart::CopperFill:
		case ModelPart::Logo:
		case ModelPart::Hole:
		case ModelPart::Via:
			itemBase->setVisible(false);
			itemBase->setEverVisible(false);
			return;
		default:
			if (itemBase->moduleID().endsWith(ModuleIDNames::SchematicFrameModuleIDName) || 
				itemBase->moduleID().endsWith(ModuleIDNames::PadModuleIDName)) 
			{
				itemBase->setVisible(false);
				itemBase->setEverVisible(false);
				return;
			}
			break;
	}
}

bool BreadboardSketchWidget::canDisconnectAll() {
	return false;
}

bool BreadboardSketchWidget::ignoreFemale() {
	return false;
}

double BreadboardSketchWidget::defaultGridSizeInches() {
	return 0.1;
}

ViewLayer::ViewLayerID BreadboardSketchWidget::getLabelViewLayerID(ViewLayer::ViewLayerSpec) {
	return ViewLayer::BreadboardLabel;
}

void BreadboardSketchWidget::addDefaultParts() {
	long newID = ItemBase::getNextID();
	ViewGeometry viewGeometry;
	viewGeometry.setLoc(QPointF(0, 0));
	m_addedDefaultPart = addItem(paletteModel()->retrieveModelPart(ModuleIDNames::BreadboardModuleIDName), defaultViewLayerSpec(), BaseCommand::CrossView, viewGeometry, newID, -1, NULL, NULL);
	m_addDefaultParts = true;
	// have to put this off until later, because positioning the item doesn't work correctly until the view is visible
}

void BreadboardSketchWidget::showEvent(QShowEvent * event) {
	SketchWidget::showEvent(event);
	if (m_addDefaultParts && (m_addedDefaultPart != NULL)) {
		m_addDefaultParts = false;
		if (m_fixedToCenterItem != NULL) {
			QSizeF helpSize = m_fixedToCenterItem->size();
			QSizeF partSize = m_addedDefaultPart->size();
			QSizeF vpSize = this->viewport()->size();
			//if (vpSize.height() < helpSize.height() + 50 + partSize.height()) {
				//vpSize.setWidth(vpSize.width() - verticalScrollBar()->width());
			//}

			QPointF p;
			p.setX((int) ((vpSize.width() - partSize.width()) / 2.0));
			p.setY((int) helpSize.height());

			// add a board to the empty sketch, and place it below the help area.

			p += QPointF(0, 50);
			QPointF q = mapToScene(p.toPoint());
			m_addedDefaultPart->setPos(q);
			QTimer::singleShot(10, this, SLOT(vScrollToZero()));
		}
	}
}

QPoint BreadboardSketchWidget::calcFixedToCenterItemOffset(const QRect & viewPortRect, const QSizeF & helpSize) {
	QPoint p((int) ((viewPortRect.width() - helpSize.width()) / 2.0),
			 30);
	return p;
}

double BreadboardSketchWidget::getWireStrokeWidth(Wire *, double wireWidth)
{
	return wireWidth * WireHoverStrokeFactor;
}

void BreadboardSketchWidget::getBendpointWidths(Wire * wire, double width, double & bendpointWidth, double & bendpoint2Width, bool & negativeOffsetRect) {
	Q_UNUSED(wire);
	Q_UNUSED(width);
	bendpoint2Width = bendpointWidth = -1;
	negativeOffsetRect = true;
}

double BreadboardSketchWidget::getRatsnestOpacity() {
	return 0.7;
}

double BreadboardSketchWidget::getRatsnestWidth() {
	return 0.7;
}
