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

#include <qmath.h>

#include "pcbsketchwidget.h"
#include "../debugdialog.h"
#include "../items/tracewire.h"
#include "../items/virtualwire.h"
#include "../items/resizableboard.h"
#include "../items/pad.h"
#include "../waitpushundostack.h"
#include "../connectors/connectoritem.h"
#include "../items/moduleidnames.h"
#include "../items/partlabel.h"
#include "../help/sketchmainhelp.h"
#include "../fsvgrenderer.h"
#include "../autoroute/autorouteprogressdialog.h"
#include "../items/groundplane.h"
#include "../items/jumperitem.h"
#include "../utils/autoclosemessagebox.h"
#include "../utils/graphicsutils.h"
#include "../utils/textutils.h"
#include "../processeventblocker.h"
#include "../autoroute/cmrouter/cmrouter.h"
#include "../autoroute/cmrouter/tileutils.h"
#include "../autoroute/cmrouter/panelizer.h"
#include "../autoroute/autoroutersettingsdialog.h"
#include "../svg/groundplanegenerator.h"
#include "../items/logoitem.h"
#include "../dialogs/groundfillseeddialog.h"
#include "../version/version.h"

#include <limits>
#include <QApplication>
#include <QScrollBar>
#include <QDialog>
#include <QRadioButton>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QDialogButtonBox>
#include <QSettings>
#include <QPushButton>
#include <QMessageBox>

static const int MAX_INT = std::numeric_limits<int>::max();
static const double BlurBy = 3.5;
static const double StrokeWidthIncrement = 50;

static QString PCBTraceColor1 = "trace1";
static QString PCBTraceColor = "trace";

QSizeF PCBSketchWidget::m_jumperItemSize;

struct DistanceThing {
	int distance;
	bool fromConnector0;
};

QHash <ConnectorItem *, DistanceThing *> distances;

bool bySize(QList<ConnectorItem *> * l1, QList<ConnectorItem *> * l2) {
	return l1->count() >= l2->count();
}

bool distanceLessThan(ConnectorItem * end0, ConnectorItem * end1) {
	if (end0->connectorType() == Connector::Male && end1->connectorType() == Connector::Female) {
		return true;
	}
	if (end1->connectorType() == Connector::Male && end0->connectorType() == Connector::Female) {
		return false;
	}

	DistanceThing * dt0 = distances.value(end0, NULL);
	DistanceThing * dt1 = distances.value(end1, NULL);
	if (dt0 && dt1) {
		return dt0->distance <= dt1->distance;
	}

	if (dt0) {
		return true;
	}

	if (dt1) {
		return false;
	}

	return true;
}

//////////////////////////////////////////////////////

const char * PCBSketchWidget::FakeTraceProperty = "FakeTrace";

PCBSketchWidget::PCBSketchWidget(ViewIdentifierClass::ViewIdentifier viewIdentifier, QWidget *parent)
    : SketchWidget(viewIdentifier, parent)
{
	m_resizingBoard = NULL;
	m_resizingJumperItem = NULL;
	m_viewName = QObject::tr("PCB View");
	m_shortName = QObject::tr("pcb");
	initBackgroundColor();

	m_routingStatus.zero();
	m_cleanType = noClean;
}

void PCBSketchWidget::setWireVisible(Wire * wire)
{
	bool visible = wire->getRatsnest() || (wire->isTraceType(this->getTraceFlag()));
	wire->setVisible(visible);
	wire->setEverVisible(visible);
}

void PCBSketchWidget::addViewLayers() {
	addPcbViewLayers();

	// disable these for now
	//viewLayer = m_viewLayers.value(ViewLayer::Keepout);
	//viewLayer->action()->setEnabled(false);

	setBoardLayers(1, false);
}



ViewLayer::ViewLayerID PCBSketchWidget::multiLayerGetViewLayerID(ModelPart * modelPart, ViewIdentifierClass::ViewIdentifier viewIdentifier, ViewLayer::ViewLayerSpec viewLayerSpec, QDomElement & layers, QString & layerName) {
	Q_UNUSED(modelPart);
	Q_UNUSED(viewIdentifier);

	if (viewLayerSpec == ViewLayer::GroundPlane_Bottom) return ViewLayer::GroundPlane0;
	else if (viewLayerSpec == ViewLayer::GroundPlane_Top) return ViewLayer::GroundPlane1;

	// priviledge Copper if it's available
	ViewLayer::ViewLayerID wantLayer = modelPart->flippedSMD() && viewLayerSpec == ViewLayer::ThroughHoleThroughTop_TwoLayers ? ViewLayer::Copper1 : ViewLayer::Copper0;
	QDomElement layer = layers.firstChildElement("layer");
	while (!layer.isNull()) {
		QString lName = layer.attribute("layerId");
		if (ViewLayer::viewLayerIDFromXmlString(lName) == wantLayer) {
			return wantLayer;
		}

		layer = layer.nextSiblingElement("layer");
	}

	return ViewLayer::viewLayerIDFromXmlString(layerName);
}

bool PCBSketchWidget::canDeleteItem(QGraphicsItem * item, int count)
{
	VirtualWire * wire = dynamic_cast<VirtualWire *>(item);
	if (wire != NULL && count > 1) return false;

	return SketchWidget::canDeleteItem(item, count);
}

bool PCBSketchWidget::canCopyItem(QGraphicsItem * item, int count)
{
	VirtualWire * wire = dynamic_cast<VirtualWire *>(item);
	if (wire != NULL) {
		if (wire->getRatsnest()) return false;
	}

	return SketchWidget::canCopyItem(item, count);
}

bool PCBSketchWidget::canChainWire(Wire * wire) {
	bool result = SketchWidget::canChainWire(wire);
	if (!result) return result;

	if (wire->getRatsnest()) {
		ConnectorItem * c0 = wire->connector0()->firstConnectedToIsh();
		if (c0 == NULL) return false;

		ConnectorItem * c1 = wire->connector1()->firstConnectedToIsh();
		if (c1 == NULL) return false;

		return !c0->wiredTo(c1, (ViewGeometry::NormalFlag | ViewGeometry::PCBTraceFlag | ViewGeometry::RatsnestFlag | ViewGeometry::SchematicTraceFlag) ^ getTraceFlag()); 
	}

	return result;
}


void PCBSketchWidget::createTrace(Wire * wire) {
	QString commandString = tr("Create Trace from Ratsnest");
	SketchWidget::createTrace(wire, commandString, getTraceFlag());
	ensureTraceLayerVisible();
}

void PCBSketchWidget::excludeFromAutoroute(bool exclude)
{
	foreach (QGraphicsItem * item, scene()->selectedItems()) {
		TraceWire * wire = dynamic_cast<TraceWire *>(item);
		
		if (wire) {
			if (!wire->isTraceType(getTraceFlag())) continue;

			QList<Wire *> wires;
			QList<ConnectorItem *> ends;
			wire->collectChained(wires, ends);
			foreach (Wire * w, wires) {
				w->setAutoroutable(!exclude);
			}
			continue;
		}

		JumperItem * jumperItem = dynamic_cast<JumperItem *>(item);
		if (jumperItem) {
			jumperItem->setAutoroutable(!exclude);
			continue;
		}

		Via * via = dynamic_cast<Via *>(item);
		if (via) {
			via->setAutoroutable(!exclude);
			continue;
		}
	}
}

void PCBSketchWidget::selectAllExcludedTraces() 
{
	selectAllXTraces(false, QObject::tr("Select all 'Don't autoroute' traces"));
}

void PCBSketchWidget::selectAllIncludedTraces() 
{
	selectAllXTraces(true, QObject::tr("Select all autorouteable traces"));
}

void PCBSketchWidget::selectAllXTraces(bool autoroutable, const QString & cmdText) 
{
	QList<Wire *> wires;
	foreach (QGraphicsItem * item, scene()->items()) {
		TraceWire * wire = dynamic_cast<TraceWire *>(item);
		if (wire == NULL) continue;

		if (!wire->isTraceType(getTraceFlag())) continue;

		if (wire->getAutoroutable() == autoroutable) {
			wires.append(wire);
		}
	}

	QUndoCommand * parentCommand = new QUndoCommand(cmdText);

	stackSelectionState(false, parentCommand);
	SelectItemCommand * selectItemCommand = new SelectItemCommand(this, SelectItemCommand::NormalSelect, parentCommand);
	foreach (Wire * wire, wires) {
		selectItemCommand->addRedo(wire->id());
	}

	scene()->clearSelection();
	m_undoStack->push(parentCommand);
}



const QString & PCBSketchWidget::hoverEnterPartConnectorMessage(QGraphicsSceneHoverEvent * event, ConnectorItem * item)
{
	Q_UNUSED(event);
	Q_UNUSED(item);

	static QString message = tr("Click this connector to drag out a new trace.");

	return message;
}

void PCBSketchWidget::addDefaultParts() {

	long newID = ItemBase::getNextID();
	ViewGeometry viewGeometry;
	viewGeometry.setLoc(QPointF(0, 0));

	// have to put this off until later, because positioning the item doesn't work correctly until the view is visible
	m_addedDefaultPart = addItem(paletteModel()->retrieveModelPart(ModuleIDNames::TwoSidedRectangleModuleIDName), defaultViewLayerSpec(), BaseCommand::CrossView, viewGeometry, newID, -1, NULL, NULL);
	m_addDefaultParts = true;

	changeBoardLayers(2, true);
}

QPoint PCBSketchWidget::calcFixedToCenterItemOffset(const QRect & viewPortRect, const QSizeF & helpSize) {
	QPoint p((int) ((viewPortRect.width() - helpSize.width()) / 2.0),
			 30);
	return p;
}

void PCBSketchWidget::showEvent(QShowEvent * event) {
	SketchWidget::showEvent(event);
	dealWithDefaultParts();
}

void PCBSketchWidget::dealWithDefaultParts() {
	if (!m_addDefaultParts) return;
	if  (m_addedDefaultPart == NULL) return;

	m_addDefaultParts = false;

	if (m_fixedToCenterItem == NULL) return;

	// place the default rectangular board in relation to the first time help area

	QSizeF helpSize = m_fixedToCenterItem->size();
	QSizeF vpSize = this->viewport()->size();
	QSizeF partSize(600, 200);

	//if (vpSize.height() < helpSize.height() + 50 + partSize.height()) {
		//vpSize.setWidth(vpSize.width() - verticalScrollBar()->width());
	//}

	QPointF p;
	p.setX((int) ((vpSize.width() - partSize.width()) / 2.0));
	p.setY((int) helpSize.height());

	// TODO: make these constants less arbitrary (get the size and location of the icon which the board is replacing)
	p += QPointF(0, 50);

	// place it
	QPointF q = mapToScene(p.toPoint());
	m_addedDefaultPart->setPos(q);
	ResizableBoard * rb = qobject_cast<ResizableBoard *>(m_addedDefaultPart);
	if (rb) rb->resizePixels(partSize.width(), partSize.height(), m_viewLayers);
	QTimer::singleShot(10, this, SLOT(vScrollToZero()));

	setLayerActive(ViewLayer::Copper1, false);
	setLayerActive(ViewLayer::Copper0, true);
}


void PCBSketchWidget::setClipEnds(ClipableWire * vw, bool clipEnds) {
	vw->setClipEnds(clipEnds);
}

ViewLayer::ViewLayerID PCBSketchWidget::getDragWireViewLayerID(ConnectorItem * connectorItem) {
	switch (connectorItem->attachedToViewLayerID()) {
		case ViewLayer::Copper1:
		case ViewLayer::Copper1Trace:
		case ViewLayer::GroundPlane1:
			return ViewLayer::Copper1Trace;
		default:
			return ViewLayer::Copper0Trace;
	}
}

ViewLayer::ViewLayerID PCBSketchWidget::getWireViewLayerID(const ViewGeometry & viewGeometry, ViewLayer::ViewLayerSpec viewLayerSpec) {
	if (viewGeometry.getRatsnest()) {
		return ViewLayer::PcbRatsnest;
	}

	if (viewGeometry.getAnyTrace()) {
		switch (viewLayerSpec) {
			case ViewLayer::Top:
			case ViewLayer::WireOnTop_TwoLayers:
			case ViewLayer::GroundPlane_Top:
				return ViewLayer::Copper1Trace;
			default:
				return ViewLayer::Copper0Trace;
		}
	}

	switch (viewLayerSpec) {
		case ViewLayer::Top:
		case ViewLayer::WireOnTop_TwoLayers:
			return ViewLayer::Copper1Trace;
		default:
			return m_wireViewLayerID;
	}
}

void PCBSketchWidget::initWire(Wire * wire, int penWidth) {
	Q_UNUSED(penWidth);
	if (wire->getRatsnest()) return;

	wire->setColorString(traceColor(wire->connector0()), 1.0);
	wire->setPenWidth(1, this, 2);
}

bool PCBSketchWidget::autorouteTypePCB() {
	return true;
}

const QString & PCBSketchWidget::traceColor(ConnectorItem * forColor) {
	switch(forColor->attachedToViewLayerID()) {
		case ViewLayer::Copper1:
		case ViewLayer::Copper1Trace:
		case ViewLayer::GroundPlane1:
			return PCBTraceColor1;
		default:
			return PCBTraceColor;
	}	
}

const QString & PCBSketchWidget::traceColor(ViewLayer::ViewLayerSpec viewLayerSpec) {
	if (viewLayerSpec == ViewLayer::Top) {
		return PCBTraceColor1;
	}

	return PCBTraceColor;
}

PCBSketchWidget::CleanType PCBSketchWidget::cleanType() {
	return m_cleanType;
}

void PCBSketchWidget::ensureTraceLayersVisible() {
	ensureLayerVisible(ViewLayer::Copper0);
	ensureLayerVisible(ViewLayer::Copper0Trace);
	ensureLayerVisible(ViewLayer::GroundPlane0);
	if (m_boardLayers == 2) {
		ensureLayerVisible(ViewLayer::Copper1);
		ensureLayerVisible(ViewLayer::Copper1Trace);
		ensureLayerVisible(ViewLayer::GroundPlane1);
	}
}

void PCBSketchWidget::ensureTraceLayerVisible() {
	ensureLayerVisible(ViewLayer::Copper0);
	ensureLayerVisible(ViewLayer::Copper0Trace);
}

bool PCBSketchWidget::canChainMultiple() {
	return false;
}

void PCBSketchWidget::setNewPartVisible(ItemBase * itemBase) {
	if (itemBase->itemType() == ModelPart::Breadboard  || 
		itemBase->itemType() == ModelPart::Symbol || 
		itemBase->moduleID().endsWith(ModuleIDNames::SchematicFrameModuleIDName)) 
	{
		// don't need to see the breadboard in the other views
		// but it's there so connections can be more easily synched between views
		itemBase->setVisible(false);
		itemBase->setEverVisible(false);
	}
}

bool PCBSketchWidget::canDropModelPart(ModelPart * modelPart) {
	switch (modelPart->itemType()) {
		case ModelPart::Jumper:
		case ModelPart::Logo:
		case ModelPart::Ruler:
		case ModelPart::CopperFill:
			return true;
		case ModelPart::Wire:
		case ModelPart::Breadboard:
		case ModelPart::Symbol:
			// can't drag and drop these parts in this view
			return false;
		case ModelPart::Board:
		case ModelPart::ResizableBoard:
			if (!matchesLayer(modelPart)) return false;
			
			if (findBoard().count() != 0) {
				// don't allow multiple boards
				AutoCloseMessageBox::showMessage(this->window(), tr("Fritzing only allows one board part per sketch. Either delete the current board, or select it and swap it for a different one."));
				return false;
			}

			return true;
		default:
			return !modelPart->moduleID().endsWith(ModuleIDNames::SchematicFrameModuleIDName);
	}

	return true;
}

bool PCBSketchWidget::bothEndsConnected(Wire * wire, ViewGeometry::WireFlags flag, ConnectorItem * oneEnd, QList<Wire *> & wires, QList<ConnectorItem *> & partConnectorItems)
{
	QList<Wire *> visited;
	return bothEndsConnectedAux(wire, flag, oneEnd, wires, partConnectorItems, visited);
}


bool PCBSketchWidget::bothEndsConnectedAux(Wire * wire, ViewGeometry::WireFlags flag, ConnectorItem * oneEnd, QList<Wire *> & wires, QList<ConnectorItem *> & partConnectorItems, QList<Wire *> & visited)
{
	if (visited.contains(wire)) return false;
	visited.append(wire);

	bool result = false;
	ConnectorItem * otherEnd = wire->otherConnector(oneEnd);
	foreach (ConnectorItem * toConnectorItem, otherEnd->connectedToItems()) {
		if (partConnectorItems.contains(toConnectorItem)) {
			result = true;
			continue;
		}

		if (toConnectorItem->attachedToItemType() != ModelPart::Wire) continue;

		Wire * w = qobject_cast<Wire *>(toConnectorItem->attachedTo());
		ViewGeometry::WireFlags wflag = w->wireFlags() & (ViewGeometry::RatsnestFlag | getTraceFlag());
		if (wflag != flag) continue;

		result = bothEndsConnectedAux(w, flag, toConnectorItem, wires, partConnectorItems, visited) || result;   // let it recurse
	}

	if (result) {
		wires.removeOne(wire);
	}

	return result;
}

bool PCBSketchWidget::canCreateWire(Wire * dragWire, ConnectorItem * from, ConnectorItem * to)
{
	Q_UNUSED(dragWire);
	return ((from != NULL) && (to != NULL));
}

double PCBSketchWidget::getRatsnestOpacity() {
	return 0.7;
}

double PCBSketchWidget::getRatsnestWidth() {
	return 0.7;
}

ConnectorItem * PCBSketchWidget::findNearestPartConnectorItem(ConnectorItem * fromConnectorItem) {
	// find the nearest part to fromConnectorItem
	Wire * wire = qobject_cast<Wire *>(fromConnectorItem->attachedTo());
	if (wire == NULL) return NULL;

	QList<ConnectorItem *> ends;
	calcDistances(wire, ends);
	clearDistances();
	if (ends.count() < 1) return NULL;

	return ends[0];
}

void PCBSketchWidget::calcDistances(Wire * wire, QList<ConnectorItem *> & ends) {
	QList<Wire *> chained;
	wire->collectChained(chained, ends);
	if (ends.count() < 2) return;

	clearDistances();
	foreach (ConnectorItem * end, ends) {
		bool fromConnector0;
		QList<Wire *> distanceWires;
		int distance = calcDistance(wire, end, 0, distanceWires, fromConnector0);
		DistanceThing * dt = new DistanceThing;
		dt->distance = distance;
		dt->fromConnector0 = fromConnector0;
		DebugDialog::debug(QString("distance %1 %2 %3, %4 %5")
			.arg(end->attachedToID()).arg(end->attachedToTitle()).arg(end->connectorSharedID())
			.arg(distance).arg(fromConnector0 ? "connector0" : "connector1"));
		distances.insert(end, dt);
	}
	qSort(ends.begin(), ends.end(), distanceLessThan);

}

void PCBSketchWidget::clearDistances() {
	foreach (ConnectorItem * c, distances.keys()) {
		DistanceThing * dt = distances.value(c, NULL);
		if (dt) delete dt;
	}
	distances.clear();
}

int PCBSketchWidget::calcDistanceAux(ConnectorItem * from, ConnectorItem * to, int distance, QList<Wire *> & distanceWires) {
	//DebugDialog::debug(QString("calc distance aux: %1 %2, %3 %4, %5").arg(from->attachedToID()).arg(from->connectorSharedID())
		//.arg(to->attachedToTitle()).arg(to->connectorSharedID()).arg(distance));

	foreach (ConnectorItem * toConnectorItem, from->connectedToItems()) {
		if (toConnectorItem == to) {
			return distance;
		}
	}

	int result = MAX_INT;
	foreach (ConnectorItem * toConnectorItem, from->connectedToItems()) {
		if (toConnectorItem->attachedToItemType() != ModelPart::Wire) continue;

		Wire * w = qobject_cast<Wire *>(toConnectorItem->attachedTo());
		if (distanceWires.contains(w)) continue;

		bool fromConnector0;
		int temp = calcDistance(w, to, distance + 1, distanceWires, fromConnector0);
		if (temp < result) {
			result = temp;
		}
	}

	return result;
}

int PCBSketchWidget::calcDistance(Wire * wire, ConnectorItem * end, int distance, QList<Wire *> & distanceWires, bool & fromConnector0) {
	//DebugDialog::debug(QString("calc distance wire: %1 rat:%2 to %3 %4, %5").arg(wire->id()).arg(wire->getRatsnest())
		//.arg(end->attachedToTitle()).arg(end->connectorSharedID()).arg(distance));
	
	distanceWires.append(wire);
	int d0 = calcDistanceAux(wire->connector0(), end, distance, distanceWires);
	if (d0 == distance) {
		fromConnector0 = true;
		return d0;
	}

	int d1 = calcDistanceAux(wire->connector1(), end, distance, distanceWires);
	if (d0 <= d1) {
		fromConnector0 = true;
		return d0;
	}

	fromConnector0 = false;
	return d1;
}


bool PCBSketchWidget::usesJumperItem() {
	return true;
}

void PCBSketchWidget::showGroundTraces(QList<ConnectorItem *> & connectorItems, bool show) {

	foreach (ConnectorItem * connectorItem, connectorItems) {
		TraceWire * trace = dynamic_cast<TraceWire *>(connectorItem->attachedTo());
		if (trace == NULL) continue;

		if (!trace->isTraceType(getTraceFlag())) continue;

		trace->setVisible(show);
	}
}

void PCBSketchWidget::getLabelFont(QFont & font, QColor & color, ViewLayer::ViewLayerSpec viewLayerSpec) {
	font.setFamily("OCRA");			// ocra10
    font.setPointSize(getLabelFontSizeSmall());
	font.setBold(false);
	font.setItalic(false);
	color.setAlpha(255);

	switch (viewLayerSpec) {
		case ViewLayer::WireOnTop_TwoLayers:
		case ViewLayer::WireOnBottom_OneLayer:
		case ViewLayer::WireOnBottom_TwoLayers:
                case ViewLayer::Top:
                case ViewLayer::Bottom:
                case ViewLayer::TopAndBottom:
                case ViewLayer::UnknownSpec:
			DebugDialog::debug("bad viewLayerSpec in getLabelFont");
			break;

		case ViewLayer::ThroughHoleThroughTop_OneLayer:
		case ViewLayer::ThroughHoleThroughTop_TwoLayers:
		case ViewLayer::GroundPlane_Top:
			color.setNamedColor(ViewLayer::Silkscreen1Color);
			break;
		case ViewLayer::ThroughHoleThroughBottom_TwoLayers:
		case ViewLayer::GroundPlane_Bottom:
			color.setNamedColor(ViewLayer::Silkscreen0Color);
			break;
		case ViewLayer::SMDOnTop_TwoLayers:
			color.setNamedColor(ViewLayer::Silkscreen1Color);
			break;
		case ViewLayer::SMDOnBottom_TwoLayers:
		case ViewLayer::SMDOnBottom_OneLayer:
			color.setNamedColor(ViewLayer::Silkscreen0Color);
			break;
	}
}

double PCBSketchWidget::getLabelFontSizeTiny() {
        return 3;
}

double PCBSketchWidget::getLabelFontSizeSmall() {
	return 5;
}

double PCBSketchWidget::getLabelFontSizeMedium() {
	return 7;
}

double PCBSketchWidget::getLabelFontSizeLarge() {
	return 12;
}

void PCBSketchWidget::resizeBoard(double mmW, double mmH, bool doEmit)
{
	Q_UNUSED(doEmit);

	PaletteItem * item = getSelectedPart();
	if (item == NULL) return;

	bool handle = false;
	switch (item->itemType()) {
		case ModelPart::ResizableBoard:
		case ModelPart::Logo:
			handle = true;
			break;
		case ModelPart::Part:
			handle = item->moduleID().endsWith(ModuleIDNames::PadModuleIDName);
			break;
		default:
			break;
	}

	if (!handle) return SketchWidget::resizeBoard(mmW, mmH, doEmit);


	double origw = item->modelPart()->prop("width").toDouble();
	double origh = item->modelPart()->prop("height").toDouble();

	if (mmH == 0 || mmW == 0) {
		dynamic_cast<ResizableBoard *>(item)->setInitialSize();
		double w = item->modelPart()->prop("width").toDouble();
		double h = item->modelPart()->prop("height").toDouble();
		if (origw == w && origh == h) {
			// no change
			return;
		}

		viewItemInfo(item);
		mmW = w;
		mmH = h;
	}

	QUndoCommand * parentCommand = new QUndoCommand(tr("Resize board to %1 %2").arg(mmW).arg(mmH));
	rememberSticky(item, parentCommand);
	new ResizeBoardCommand(this, item->id(), origw, origh, mmW, mmH, parentCommand);
	new CheckStickyCommand(this, BaseCommand::SingleView, item->id(), true, CheckStickyCommand::RedoOnly, parentCommand);
	m_undoStack->waitPush(parentCommand, PropChangeDelay);
}

void PCBSketchWidget::showLabelFirstTime(long itemID, bool show, bool doEmit) {
	// called when new item is added, to decide whether to show part label
	SketchWidget::showLabelFirstTime(itemID, show, doEmit);
	ItemBase * itemBase = findItem(itemID);
	if (itemBase == NULL) return;

	switch (itemBase->itemType()) {
		case ModelPart::Part:
		case ModelPart::Jumper:
			{
				if (itemBase->hasPartLabel()) {
					ViewLayer * viewLayer = m_viewLayers.value(getLabelViewLayerID(itemBase->viewLayerSpec()));
					itemBase->showPartLabel(itemBase->isVisible(), viewLayer);
					itemBase->partLabelSetHidden(!viewLayer->visible());
				}
			}
			break;
		default:
			break;
	}

}

QList<ItemBase *> PCBSketchWidget::findBoard() {
	QSet<ItemBase *> boards;
    foreach (QGraphicsItem * childItem, items()) {
        ItemBase * board = dynamic_cast<ItemBase *>(childItem);
        if (board == NULL) continue;

        //for now take the first board you find
        if (board->itemType() == ModelPart::ResizableBoard || board->itemType() == ModelPart::Board) {
           boards.insert(board->layerKinChief());
        }
    }

	return boards.toList();
}

void PCBSketchWidget::forwardRoutingStatus(const RoutingStatus & routingStatus) 
{
	m_routingStatus = routingStatus;
	SketchWidget::forwardRoutingStatus(routingStatus);
}


double PCBSketchWidget::defaultGridSizeInches() {
	return 0.1;
}

ViewLayer::ViewLayerID PCBSketchWidget::getLabelViewLayerID(ViewLayer::ViewLayerSpec viewLayerSpec) {
	switch (viewLayerSpec) {
		case ViewLayer::WireOnTop_TwoLayers:
		case ViewLayer::WireOnBottom_OneLayer:
		case ViewLayer::WireOnBottom_TwoLayers:
			DebugDialog::debug("bad viewLayerSpec in getLabelViewLayerID");
			return ViewLayer::Silkscreen1Label;

		case ViewLayer::ThroughHoleThroughTop_OneLayer:
		case ViewLayer::ThroughHoleThroughTop_TwoLayers:
			return ViewLayer::Silkscreen1Label;
		case ViewLayer::ThroughHoleThroughBottom_TwoLayers:
			return ViewLayer::Silkscreen0Label;
		case ViewLayer::SMDOnTop_TwoLayers:
			return ViewLayer::Silkscreen1Label;
		case ViewLayer::SMDOnBottom_OneLayer:
		case ViewLayer::SMDOnBottom_TwoLayers:
			return ViewLayer::Silkscreen0Label;
		default:
			return ViewLayer::Silkscreen1Label;
	}

}

ViewLayer::ViewLayerSpec PCBSketchWidget::wireViewLayerSpec(ConnectorItem * connectorItem) {
	switch (connectorItem->attachedToViewLayerID()) {
		case ViewLayer::Copper1:
		case ViewLayer::Copper1Trace:
		case ViewLayer::GroundPlane1:
			return ViewLayer::WireOnTop_TwoLayers;
		default:
			return (m_boardLayers == 1) ?  ViewLayer::WireOnBottom_OneLayer : ViewLayer::WireOnBottom_TwoLayers;
	}
}

void PCBSketchWidget::setBoardLayers(int layers, bool redraw) {
	SketchWidget::setBoardLayers(layers, redraw);

	QList <ViewLayer::ViewLayerID> viewLayerIDs;
	viewLayerIDs << ViewLayer::Copper1 << ViewLayer::Copper1Trace;
	foreach (ViewLayer::ViewLayerID viewLayerID, viewLayerIDs) {
		ViewLayer * layer = m_viewLayers.value(viewLayerID, NULL);
		if (layer) {
			layer->action()->setEnabled(layers == 2);
			layer->setVisible(layers == 2);
			if (redraw) {
				setLayerVisible(layer, layers == 2, true);
				if (layers == 2) {
					layer->action()->setChecked(true);
				}
			}
		}
	}
}

long PCBSketchWidget::setUpSwap(ItemBase * itemBase, long newModelIndex, const QString & newModuleID, ViewLayer::ViewLayerSpec viewLayerSpec, 
								bool master, bool noFinalChangeWiresCommand, QList<Wire *> & wiresToDelete, QUndoCommand * parentCommand)
{
	Q_UNUSED(noFinalChangeWiresCommand);

	QList<ItemBase *> smds;
	QList<ItemBase *> pads;
	QList<Wire *> already;

	int newLayers = isBoardLayerChange(itemBase, newModuleID, master);
	bool wasSMD = itemBase->modelPart()->flippedSMD();

	long newID = SketchWidget::setUpSwap(itemBase, newModelIndex, newModuleID, viewLayerSpec, master, newLayers != m_boardLayers, wiresToDelete, parentCommand);
	if (newLayers == m_boardLayers) {
		if (!wasSMD) {
			ModelPart * mp = paletteModel()->retrieveModelPart(newModuleID);
			if (mp->flippedSMD()) {
				smds.append(itemBase);
				clearSmdTraces(smds, already, parentCommand);
			}
		}
		return newID;
	}

	if (itemBase->viewIdentifier() != m_viewIdentifier) {
		itemBase = findItem(itemBase->id());
		if (itemBase == NULL) {
			if (master) {
				new CleanUpWiresCommand(this, CleanUpWiresCommand::RedoOnly, parentCommand);
			}
			return newID;
		}
	}

	ChangeBoardLayersCommand * changeBoardCommand = new ChangeBoardLayersCommand(this, m_boardLayers, newLayers, parentCommand);

	ModelPart * mp = paletteModel()->retrieveModelPart(newModuleID);
	if (mp->itemType() == ModelPart::ResizableBoard) {
		// preserve the size if swapping rectangular board
		ResizableBoard * rb = qobject_cast<ResizableBoard *>(itemBase);
		if (rb) {
			QPointF p;
			QSizeF sz;
			rb->getParams(p, sz);
			new ResizeBoardCommand(this, newID, sz.width(), sz.height(), sz.width(), sz.height(), parentCommand);
		}
		new CheckStickyCommand(this, BaseCommand::SingleView, newID, false, CheckStickyCommand::RemoveOnly, parentCommand);
	}


	// disconnect and flip smds
	foreach (QGraphicsItem * item, scene()->items()) {
		ItemBase * smd = dynamic_cast<ItemBase *>(item);
		if (smd == NULL) continue;
        if (smd->moduleID().compare(ModuleIDNames::PadModuleIDName) == 0) {
            pads << smd;
            continue;
        }

		if (!smd->modelPart()->flippedSMD()) continue;

		smd = smd->layerKinChief();
		if (smds.contains(smd)) continue;

		smds.append(smd);
	}

	clearSmdTraces(smds, already, changeBoardCommand);

	if (newLayers == 1) {
		// remove traces on the top layer
		foreach (QGraphicsItem * item, scene()->items()) {
			TraceWire * tw = dynamic_cast<TraceWire *>(item);
			if (tw == NULL) continue;

			if (!tw->isTraceType(getTraceFlag())) continue;
			if (tw->viewLayerID() != ViewLayer::Copper1Trace) continue;
			if (already.contains(tw)) continue;
				
			QList<ConnectorItem *> ends;
			removeWire(tw, ends, already, changeBoardCommand);
		}
	}

	foreach (ItemBase * smd, smds) {
		emit subSwapSignal(this, smd, (newLayers == 1) ? ViewLayer::ThroughHoleThroughTop_OneLayer : ViewLayer::ThroughHoleThroughTop_TwoLayers, changeBoardCommand);
	}

	foreach (ItemBase * itemBase, pads) {
        Pad * pad = qobject_cast<Pad *>(itemBase);
        if (pad == NULL) continue;

        double w = pad->modelPart()->prop("width").toDouble();
        double h = pad->modelPart()->prop("height").toDouble();

        new ResizeBoardCommand(this, pad->id(), w, h, w, h, parentCommand);
	}

	return newID;
}

void PCBSketchWidget::clearSmdTraces(QList<ItemBase *> & smds, 	QList<Wire *> & already, QUndoCommand * parentCommand) {

	foreach (ItemBase * smd, smds) {
		foreach (ConnectorItem * ci, smd->cachedConnectorItems()) {
			//ci->debugInfo("smd from");

			foreach (ConnectorItem * toci, ci->connectedToItems()) {
				//toci->debugInfo(" smd to");
				TraceWire * tw = qobject_cast<TraceWire *>(toci->attachedTo());
				if (tw == NULL) continue;
				if (already.contains(tw)) continue;
				if (!tw->isTraceType(getTraceFlag())) continue;

				QList<ConnectorItem *> ends;
				removeWire(tw, ends, already, parentCommand);		
			}
		}
	}

	// remove these trace connections now so they're not added back in when the part is swapped
	QList<ConnectorItem *> ends;
	foreach (Wire * wire, already) {
		ends.append(wire->connector0());				
		ends.append(wire->connector1());
	}
	foreach (ConnectorItem * end, ends) {
		//end->debugInfo("end from");
		foreach (ConnectorItem * endTo, end->connectedToItems()) {
			//endTo->debugInfo("   end to");
			end->tempRemove(endTo, false);
			endTo->tempRemove(end, false);
		}
	}
}

int PCBSketchWidget::isBoardLayerChange(ItemBase * itemBase, const QString & newModuleID, bool master)
{							
	if (!master) return m_boardLayers;

	switch (itemBase->itemType()) {
		case ModelPart::Board:
		case ModelPart::ResizableBoard:
			// maybe a change
			break;
		default: 
			// no change
			return m_boardLayers;
	}

	ModelPart * modelPart = paletteModel()->retrieveModelPart(newModuleID);
	if (modelPart == NULL) {
		// shouldn't happen
		return m_boardLayers;
	}

	QString slayers = modelPart->properties().value("layers", "");
	if (slayers.isEmpty()) {
		// shouldn't happen
		return m_boardLayers;
	}

	bool ok;
	int layers = slayers.toInt(&ok);
	if (!ok) {
		// shouldn't happen
		return m_boardLayers;
	}

	return layers;
}


void PCBSketchWidget::changeBoardLayers(int layers, bool doEmit) {
	setBoardLayers(layers, true);
	SketchWidget::changeBoardLayers(layers, doEmit);
	if (layers == 1) {
		this->setLayerActive(ViewLayer::Copper0, true);
	}
	emit updateLayerMenuSignal();
}

void PCBSketchWidget::loadFromModelParts(QList<ModelPart *> & modelParts, BaseCommand::CrossViewType crossViewType, QUndoCommand * parentCommand, 
						bool offsetPaste, const QRectF * boundingRect, bool seekOutsideConnections, QList<long> & newIDs) {
	if (parentCommand == NULL) {
		bool done = false;
		foreach (ModelPart * modelPart, modelParts) {
			switch (modelPart->itemType()) {
				case ModelPart::Board:
				case ModelPart::ResizableBoard:
					{
						done = true;		// not allowed to have multiple boards, so we're almost done

						QString slayers = modelPart->properties().value("layers", "");
						if (slayers.isEmpty()) {
							// shouldn't happen
							break;
						}
						bool ok;
						int layers = slayers.toInt(&ok);
						if (!ok) {
							// shouldn't happen
							break;
						}
						if (layers != m_boardLayers) {
							changeBoardLayers(layers, true);
							break;
						}		
					}
					break;
				default: 
					break;
			}

			if (done) break;
		}
	}

	SketchWidget::loadFromModelParts(modelParts, crossViewType, parentCommand, offsetPaste, boundingRect, seekOutsideConnections, newIDs);

	if (parentCommand == NULL) {
		shiftHoles();
	}

}

bool PCBSketchWidget::isInLayers(ConnectorItem * connectorItem, ViewLayer::ViewLayerSpec viewLayerSpec) {
	return connectorItem->isInLayers(viewLayerSpec);
}

bool PCBSketchWidget::routeBothSides() {
	return m_boardLayers > 1;
}

bool PCBSketchWidget::sameElectricalLayer2(ViewLayer::ViewLayerID id1, ViewLayer::ViewLayerID id2) {
	switch (id1) {
		case ViewLayer::Copper0Trace:
			if (id1 == id2) return true;
			return (id2 == ViewLayer::Copper0);
		case ViewLayer::Copper0:
			if (id1 == id2) return true;
			return (id2 == ViewLayer::Copper0Trace);
		case ViewLayer::Copper1Trace:
			if (id1 == id2) return true;
			return (id2 == ViewLayer::Copper1);
		case ViewLayer::Copper1:
			if (id1 == id2) return true;
			return (id2 == ViewLayer::Copper1Trace);
                default:
                        break;
	}

	return false;
}

void PCBSketchWidget::changeTraceLayer() {
	QList<Wire *> visitedWires;
	QList<Wire *> changeWires;
	foreach (QGraphicsItem * item, scene()->selectedItems()) {
		TraceWire * tw = dynamic_cast<TraceWire *>(item);
		if (tw == NULL) continue;

		if (!tw->isTraceType(getTraceFlag())) continue;
		if (visitedWires.contains(tw)) continue;

		QList<Wire *> wires;
		QList<ConnectorItem *> ends;
		tw->collectChained(wires, ends);
		visitedWires.append(wires);
		if (ends.count() < 2) continue;   // should never happen, since traces have to be connected at both ends

		bool canChange = true;
		foreach(ConnectorItem * end, ends) {
			if (end->getCrossLayerConnectorItem() == NULL) {
				canChange = false;
				break;
			}
		}
		if (!canChange) continue;

		changeWires.append(tw);
	}

	if (changeWires.count() == 0) return;

	QUndoCommand * parentCommand = new QUndoCommand(tr("Change trace layer"));

	ViewLayer::ViewLayerID newViewLayerID = (changeWires.at(0)->viewLayerID() == ViewLayer::Copper0Trace) ? ViewLayer::Copper1Trace : ViewLayer::Copper0Trace;;
	foreach (Wire * wire, changeWires) {
		QList<Wire *> wires;
		QList<ConnectorItem *> ends;
		wire->collectChained(wires, ends);

		// probably safest to disconnect change the layers and reconnect, so that's why the redundant looping

		foreach (ConnectorItem * end, ends) {
			ConnectorItem * targetConnectorItem = NULL;
			foreach (ConnectorItem * toConnectorItem, end->connectedToItems()) {
				Wire * w = qobject_cast<Wire *>(toConnectorItem->attachedTo());
				if (w == NULL) continue;

				if (wires.contains(w)) {
					targetConnectorItem = toConnectorItem;
					break;
				}
			}

			extendChangeConnectionCommand(BaseCommand::SingleView,
								targetConnectorItem, end,
								ViewLayer::specFromID(end->attachedToViewLayerID()), 
								false, parentCommand);
		}

		foreach (Wire * w, wires) {
			new ChangeLayerCommand(this, w->id(), w->zValue(), m_viewLayers.value(newViewLayerID)->nextZ(), w->viewLayerID(), newViewLayerID, parentCommand);
		}

		foreach (ConnectorItem * end, ends) {
			ConnectorItem * targetConnectorItem = NULL;
			foreach (ConnectorItem * toConnectorItem, end->connectedToItems()) {
				Wire * w = qobject_cast<Wire *>(toConnectorItem->attachedTo());
				if (w == NULL) continue;

				if (wires.contains(w)) {
					targetConnectorItem = toConnectorItem;
					break;
				}
			}

			new ChangeConnectionCommand(this, BaseCommand::SingleView,
								targetConnectorItem->attachedToID(), targetConnectorItem->connectorSharedID(),
								end->attachedToID(), end->connectorSharedID(),
								ViewLayer::specFromID(end->getCrossLayerConnectorItem()->attachedToViewLayerID()), 
								true, parentCommand);
		}
	}

	m_undoStack->waitPush(parentCommand, PropChangeDelay);
}

void PCBSketchWidget::changeLayer(long id, double z, ViewLayer::ViewLayerID viewLayerID) {
	ItemBase * itemBase = findItem(id);
	if (itemBase == NULL) return;

	itemBase->setViewLayerID(viewLayerID, m_viewLayers);
	itemBase->setZValue(z);
	itemBase->saveGeometry();

	TraceWire * tw = qobject_cast<TraceWire *>(itemBase);
	if (tw != NULL) {
		ViewLayer::ViewLayerSpec viewLayerSpec = ViewLayer::specFromID(viewLayerID);
		tw->setViewLayerSpec(viewLayerSpec);
		tw->setColorString(traceColor(viewLayerSpec), 1.0);
		ViewLayer * viewLayer = m_viewLayers.value(viewLayerID);
		tw->setInactive(!viewLayer->isActive());
		tw->setHidden(!viewLayer->visible());
		tw->update();
	}
}

bool PCBSketchWidget::resizingJumperItemPress(QGraphicsItem * item) {
	JumperItem * jumperItem = dynamic_cast<JumperItem *>(item);
	if (jumperItem == NULL) return false;

	if (jumperItem->inDrag()) {
		m_resizingJumperItem = jumperItem;
		m_resizingJumperItem->saveParams();
		if (m_alignToGrid) {
			m_alignmentStartPoint = QPointF(0,0);
			QList<ItemBase *> boards = findBoard();
			ItemBase * board = boards.count() == 1 ? boards.at(0) : NULL;
			QHash<long, ItemBase *> savedItems;
			QHash<Wire *, ConnectorItem *> savedWires;
			if (board == NULL) {
				foreach (QGraphicsItem * item, scene()->items()) {
					PaletteItemBase * itemBase = dynamic_cast<PaletteItemBase *>(item);
					if (itemBase->itemType() == ModelPart::Jumper) continue;

					savedItems.insert(itemBase->layerKinChief()->id(), itemBase);
				}
			}
			findAlignmentAnchor(board, savedItems, savedWires);
			m_jumperDragOffset = jumperItem->dragOffset();
			connect(m_resizingJumperItem, SIGNAL(alignMe(JumperItem *, QPointF &)), this, SLOT(alignJumperItem(JumperItem *, QPointF &)), Qt::DirectConnection);
		}
		return true;
	}

	return false;
}

void PCBSketchWidget::alignJumperItem(JumperItem * jumperItem, QPointF & loc) {
	Q_UNUSED(jumperItem);
	if (!m_alignToGrid) return;

	QPointF newPos = loc - m_jumperDragOffset - m_alignmentStartPoint;
	double ny = GraphicsUtils::getNearestOrdinate(newPos.y(), gridSizeInches() * FSvgRenderer::printerScale());
	double nx = GraphicsUtils::getNearestOrdinate(newPos.x(), gridSizeInches() * FSvgRenderer::printerScale());
	loc.setX(loc.x() + nx - newPos.x());
	loc.setY(loc.y() + ny - newPos.y());
}

bool PCBSketchWidget::resizingJumperItemRelease() {
	if (m_resizingJumperItem == NULL) return false;

	if (m_alignToGrid) {
		disconnect(m_resizingJumperItem, SIGNAL(alignMe(JumperItem *, QPointF &)), this, SLOT(alignJumperItem(JumperItem *, QPointF &)));
	}
	resizeJumperItem();
	return true;
}

void PCBSketchWidget::resizeJumperItem() {
	QPointF oldC0, oldC1;
	QPointF oldPos;
	m_resizingJumperItem->getParams(oldPos, oldC0, oldC1);
	QPointF newC0, newC1;
	QPointF newPos;
	m_resizingJumperItem->saveParams();
	m_resizingJumperItem->getParams(newPos, newC0, newC1);
	QUndoCommand * cmd = new ResizeJumperItemCommand(this, m_resizingJumperItem->id(), oldPos, oldC0, oldC1, newPos, newC0, newC1, NULL);
	cmd->setText("Resize Jumper");
	m_undoStack->waitPush(cmd, 10);
	m_resizingJumperItem = NULL;
}

bool PCBSketchWidget::resizingBoardPress(QGraphicsItem * item) {
	// board's child items (at the moment) are the resize grips
	ResizableBoard * rb = dynamic_cast<ResizableBoard *>(item);
	if (rb == NULL) return false;
	if (!rb->inResize()) return false;

	m_resizingBoard = rb;
	m_resizingBoard->saveParams();
	return true;
}

bool PCBSketchWidget::resizingBoardRelease() {

	if (m_resizingBoard == NULL) return false;

	resizeBoard();
	return true;
}

void PCBSketchWidget::resizeBoard() {
	QSizeF oldSize;
	QPointF oldPos;
	m_resizingBoard->getParams(oldPos, oldSize);
	QSizeF newSize;
	QPointF newPos;
	m_resizingBoard->saveParams();
	m_resizingBoard->getParams(newPos, newSize);
	QUndoCommand * parentCommand = new QUndoCommand(tr("Resize board to %1 %2").arg(newSize.width()).arg(newSize.height()));
	rememberSticky(m_resizingBoard, parentCommand);
	new ResizeBoardCommand(this, m_resizingBoard->id(), oldSize.width(), oldSize.height(), newSize.width(), newSize.height(), parentCommand);
	if (oldPos != newPos) {
		m_resizingBoard->saveGeometry();
		ViewGeometry vg1 = m_resizingBoard->getViewGeometry();
		ViewGeometry vg2 = vg1;
		vg1.setLoc(oldPos);
		vg2.setLoc(newPos);
		new MoveItemCommand(this, m_resizingBoard->id(), vg1, vg2, false, parentCommand);
	}
	new CheckStickyCommand(this, BaseCommand::SingleView, m_resizingBoard->id(), true, CheckStickyCommand::RedoOnly, parentCommand);
	m_undoStack->waitPush(parentCommand, 10);
	m_resizingBoard = NULL;
}

bool PCBSketchWidget::canDragWire(Wire * wire) {
	if (wire == NULL) return false;

	if (wire->getRatsnest()) return false;

	return true;
}

void PCBSketchWidget::wireSplitSlot(Wire* wire, QPointF newPos, QPointF oldPos, QLineF oldLine) {
	if (!wire->getRatsnest()) {
		SketchWidget::wireSplitSlot(wire, newPos, oldPos, oldLine);
	}

	createTrace(wire);
}


ItemBase * PCBSketchWidget::addCopperLogoItem(ViewLayer::ViewLayerSpec viewLayerSpec) 
{
	long newID = ItemBase::getNextID();
	ViewGeometry viewGeometry;
	viewGeometry.setLoc(QPointF(0, 0));
	QString moduleID = (viewLayerSpec == ViewLayer::Bottom) ? ModuleIDNames::Copper0LogoTextModuleIDName : ModuleIDNames::Copper1LogoTextModuleIDName;
	return addItem(paletteModel()->retrieveModelPart(moduleID), viewLayerSpec, BaseCommand::SingleView, viewGeometry, newID, -1, NULL, NULL);
}

void PCBSketchWidget::updateNet(Wire * wire) {
	if (wire == NULL) return;

	QList<ConnectorItem *> connectorItems;
	connectorItems.append(wire->connector0());
	ConnectorItem::collectEqualPotential(connectorItems, true, ViewGeometry::NoFlag);

	QList<ConnectorItem *> partConnectorItems;
	ConnectorItem::collectParts(connectorItems, partConnectorItems, includeSymbols(), ViewLayer::TopAndBottom);
	if (partConnectorItems.count() < 1) return;

	partConnectorItems.at(0)->displayRatsnest(partConnectorItems, this->getTraceFlag());
}

bool PCBSketchWidget::hasAnyNets() {
	return m_routingStatus.m_netCount > 0;
}

QSizeF PCBSketchWidget::jumperItemSize() {
	return m_jumperItemSize;
}

double PCBSketchWidget::getKeepout() {
	return 0.015 * FSvgRenderer::printerScale();  // mils converted to pixels
}

bool PCBSketchWidget::acceptsTrace(const ViewGeometry & viewGeometry) {
	return !viewGeometry.getSchematicTrace();
}

ItemBase * PCBSketchWidget::placePartDroppedInOtherView(ModelPart * modelPart, ViewLayer::ViewLayerSpec viewLayerSpec, const ViewGeometry & viewGeometry, long id, SketchWidget * dropOrigin) 
{
	ItemBase * newItem = SketchWidget::placePartDroppedInOtherView(modelPart, viewLayerSpec, viewGeometry, id, dropOrigin);
	if (newItem == NULL) return newItem;
	if (!autorouteTypePCB()) return newItem;

	QList<ItemBase *> boards = findBoard();
	if (boards.count() == 0) {
		return newItem;
	}
	if (boards.count() > 1) {
		return newItem;
	}

	ItemBase * board = boards.at(0);
	dealWithDefaultParts();

	// This is a 2d bin-packing problem. We can use our tile datastructure for this.  
	// Use a simple best-fit approach for now.  No idea how optimal a solution it is.

	CMRouter router(this);
	int keepout = 10;
	router.setKeepout(keepout);
	Plane * plane = router.initPlane(false);
	QList<Tile *> alreadyTiled;	
	router.initBoard(board, plane, alreadyTiled);

	QRectF boardRect = board->sceneBoundingRect();
	foreach (QGraphicsItem * item, scene()->items()) {
		ItemBase * itemBase = dynamic_cast<ItemBase *>(item);
		if (itemBase == NULL) continue;
		if (itemBase->layerKinChief() == board) continue;
		if (itemBase->layerKinChief() == newItem) continue;

		QRectF r = itemBase->sceneBoundingRect();
		r.adjust(-keepout, -keepout, keepout, keepout);
		QRectF s = r.intersected(boardRect);
		if (s.width() <= 0) continue;
		if (s.height() <= 0) continue;	

		router.insertTile(plane, s, alreadyTiled, NULL, Tile::OBSTACLE, CMRouter::IgnoreAllOverlaps);
	}

	BestPlace bestPlace;
	bestPlace.maxRect = router.boardRect();
	bestPlace.rotate90 = false;
	bestPlace.bestTile = NULL;
	bestPlace.width = realToTile(newItem->boundingRect().width());
	bestPlace.height = realToTile(newItem->boundingRect().height());
	bestPlace.plane = plane;
	TiSrArea(NULL, plane, &bestPlace.maxRect, Panelizer::placeBestFit, &bestPlace);
	if (bestPlace.bestTile != NULL) {
		QRectF r;
		tileToQRect(bestPlace.bestTile, r);
		ItemBase * chief = newItem->layerKinChief();
		chief->setPos(r.topLeft());
		DebugDialog::debug(QString("placing part with rotation:%1").arg(bestPlace.rotate90), r);
		if (bestPlace.rotate90) {
			chief->rotateItem(90);
		}
		alignOneToGrid(newItem);
	}
	router.drcClean();

	return newItem;
}


void PCBSketchWidget::autorouterSettings() {	
	AutorouterSettingsDialog dialog;
	dialog.exec();
}

void PCBSketchWidget::getViaSize(double & ringThickness, double & holeSize) {
	QString ringThicknessStr, holeSizeStr;
	getDefaultViaSize(ringThicknessStr, holeSizeStr);
	double rt = TextUtils::convertToInches(ringThicknessStr);
	double hs = TextUtils::convertToInches(holeSizeStr);
	ringThickness = rt * FSvgRenderer::printerScale();
	holeSize = hs * FSvgRenderer::printerScale();
}

void PCBSketchWidget::getDefaultViaSize(QString & ringThickness, QString & holeSize) {
	// these settings are initialized in hole.cpp
	QSettings settings;
	ringThickness = settings.value(Hole::AutorouteViaRingThickness, Hole::DefaultAutorouteViaRingThickness).toString();
	holeSize = settings.value(Hole::AutorouteViaHoleSize, Hole::DefaultAutorouteViaHoleSize).toString();
}

void PCBSketchWidget::changeTrace(Wire * wire, ConnectorItem * from, ConnectorItem * to, QUndoCommand * parentCommand) 
{
	// first figure out whether this is a delete or a reconnect
	QList<ConnectorItem *> ends;
	QList<Wire *> wires;
	wire->collectChained(wires, ends);

	ConnectorItem * anchor = wire->otherConnector(from);

	ConnectorItem * fromDest = NULL;
	bool reconnect = (to != NULL);
	if (reconnect) {
		// check whether wire end was dropped on a legitimate target
		if (ends.contains(to)) {
			reconnect = false;
		}
		else {
			foreach (Wire * w, wires) {
				if (w->connector0() == to) {
					reconnect = false;
					break;
				}
				if (w->connector1() == to) {
					reconnect = false;
					break;
				}
			}
		}
	}

	if (reconnect) {
		foreach (ConnectorItem * toConnectorItem, from->connectedToItems()) {
			if (toConnectorItem->attachedToItemType() == ModelPart::Wire) continue;

			// the part we were originally connected to
			fromDest = toConnectorItem;
			break;
		}
	}

	if (fromDest) {
		ViewGeometry vg = wire->getViewGeometry();
		ViewGeometry newvg;
		QPointF wp = wire->pos();
		QPointF ap = anchor->sceneAdjustedTerminalPoint(NULL);
		if (wp == ap) {
			makeRatsnestViewGeometry(newvg, anchor, to);
		}
		else {
			makeRatsnestViewGeometry(newvg, to, anchor);
		}

		new ChangeWireCommand(this, wire->id(), vg.line(), newvg.line(), vg.loc(), newvg.loc(), true, true, parentCommand);

		extendChangeConnectionCommand(BaseCommand::CrossView, from, fromDest, 
					ViewLayer::specFromID(wire->viewLayerID()),
					false, parentCommand);


		ConnectorItem * toDest = to;
		if (toDest->attachedToItemType() == ModelPart::Wire) {
			toDest = this->findNearestPartConnectorItem(toDest);
		}

		extendChangeConnectionCommand(BaseCommand::CrossView, from, to, 
				ViewLayer::specFromID(wire->viewLayerID()),
				true, parentCommand);

		parentCommand->setText(QObject::tr("change trace %1").arg(wire->title()) );
		new CleanUpWiresCommand(this, CleanUpWiresCommand::RedoOnly, parentCommand);
		m_undoStack->waitPush(parentCommand, PropChangeDelay);
		return;
	}


	QList<Wire *> toDelete;
	ConnectorItem * target = anchor;
	toDelete.append(wire);
	while (true) {
		int count = 0;
		Wire * candidate = NULL;
		ConnectorItem * candidateTarget = NULL;
		foreach (ConnectorItem * toTarget, target->connectedToItems()) {
			Wire * w = qobject_cast<Wire *>(toTarget->attachedTo());
			if (w == NULL) {
				// this is a part, so delete all wires
				break;
			}

			candidate = w;
			candidateTarget = w->otherConnector(toTarget);
			++count;

			// it may be that the other connector is the one with multiple connections
			foreach (ConnectorItem * toTargetTarget, toTarget->connectedToItems()) {
				Wire * wx = qobject_cast<Wire *>(toTargetTarget->attachedTo());
				if (wx == NULL) continue;
				if (toTargetTarget->attachedTo() == target->attachedTo()) continue;

				++count;
				break;
			}

			if (count > 1) {
				break;
			}
		}

		if (candidate == NULL) {
			// connected to a part
			break;
		}

		if (count > 1) {
			// junction for multiple wires; stop here
			break;
		}

		toDelete.append(candidate);
		target = candidateTarget;	
	}

	makeWiresChangeConnectionCommands(toDelete, parentCommand);
	foreach (Wire * w, toDelete) {
		makeDeleteItemCommand(w, BaseCommand::CrossView, parentCommand);
	}

	parentCommand->setText(QObject::tr("delete trace %1").arg(wire->title()) );

	new CleanUpWiresCommand(this, CleanUpWiresCommand::RedoOnly, parentCommand);
	m_undoStack->waitPush(parentCommand, PropChangeDelay);
}



void PCBSketchWidget::deleteItem(ItemBase * itemBase, bool deleteModelPart, bool doEmit, bool later)
{
	bool boardDeleted = (itemBase->itemType() == ModelPart::Board || itemBase->itemType() == ModelPart::ResizableBoard);
	SketchWidget::deleteItem(itemBase, deleteModelPart, doEmit, later);
	if (boardDeleted) {
		if (findBoard().count() == 0) {
			// no board found, so set to single-layer by default
			changeBoardLayers(1, true);
			emit boardDeletedSignal();
		}
	}
}

double PCBSketchWidget::getTraceWidth() {
	return Wire::STANDARD_TRACE_WIDTH;
}

double PCBSketchWidget::getAutorouterTraceWidth() {
	QSettings settings;
	QString def = QString::number(GraphicsUtils::pixels2mils(getTraceWidth(), FSvgRenderer::printerScale()));
	int traceWidthMils = settings.value(AutorouterSettingsDialog::AutorouteTraceWidth, def).toInt();
	return FSvgRenderer::printerScale() * traceWidthMils / 1000.0;
}

void PCBSketchWidget::getBendpointWidths(Wire * wire, double width, double & bendpointWidth, double & bendpoint2Width, bool & negativeOffsetRect) 
{
	Q_UNUSED(wire);
	bendpointWidth = bendpoint2Width = (width / -2);
	negativeOffsetRect = false;
}

double PCBSketchWidget::getSmallerTraceWidth(double minDim) {
	int mils = qMax((int) GraphicsUtils::pixels2mils(minDim, FSvgRenderer::printerScale()) - 1, TraceWire::MinTraceWidthMils);
	return GraphicsUtils::mils2pixels(mils, FSvgRenderer::printerScale());
}

bool PCBSketchWidget::groundFill(bool fillGroundTraces, QUndoCommand * parentCommand)
{
	m_groundFillSeeds = NULL;
	QList<ItemBase *> boards = findBoard();
    // barf an error if there's no board
    if (boards.count() == 0) {
        QMessageBox::critical(NULL, tr("Fritzing"),
                   tr("Your sketch does not have a board yet!  Please add a PCB in order to use copper fill."));
        return false;
    }
    if (boards.count() > 1) {
        QMessageBox::critical(NULL, tr("Fritzing"),
                   tr("Copper Fill: multiple boards are not supported."));
        return false;
    }

	ItemBase * board = boards.at(0);

	QList<ConnectorItem *> seeds;
	if (fillGroundTraces) {
		bool gotTrueSeeds = collectGroundFillSeeds(seeds, false);

		if (!gotTrueSeeds && (seeds.count() != 1)) {
			QString message =  tr("Please designate one or more ground fill seeds before doing a ground fill.\n\n");							
			setGroundFillSeeds(message);
			return false;
		}

		ConnectorItem::collectEqualPotential(seeds, true, ViewGeometry::NoFlag);
		m_groundFillSeeds = &seeds;
	}

	LayerList viewLayerIDs;
	viewLayerIDs << ViewLayer::Board;
	QSizeF boardImageSize;
	bool empty;
	QString boardSvg = renderToSVG(FSvgRenderer::printerScale(), viewLayerIDs, true, boardImageSize, board, GraphicsUtils::StandardFritzingDPI, false, false, false, empty);
	if (boardSvg.isEmpty()) {
        QMessageBox::critical(NULL, tr("Fritzing"), tr("Fritzing error: unable to render board svg (1)."));
		return false;
	}

	viewLayerIDs.clear();
	viewLayerIDs << ViewLayer::Copper0 << ViewLayer::Copper0Trace;
	QSizeF copperImageSize;

	// hide ground traces so the ground plane will intersect them
	if (fillGroundTraces) showGroundTraces(seeds, false);
	QString svg = renderToSVG(FSvgRenderer::printerScale(), viewLayerIDs, true, copperImageSize, board, GraphicsUtils::StandardFritzingDPI, false, false, true, empty);
	if (fillGroundTraces) showGroundTraces(seeds, true);
	if (svg.isEmpty()) {
        QMessageBox::critical(NULL, tr("Fritzing"), tr("Fritzing error: unable to render copper svg (1)."));
		return false;
	}

	QString svg2;
	if (boardLayers() > 1) {
		viewLayerIDs.clear();
		viewLayerIDs << ViewLayer::Copper1 << ViewLayer::Copper1Trace;
		if (fillGroundTraces) showGroundTraces(seeds, false);
		svg2 = renderToSVG(FSvgRenderer::printerScale(), viewLayerIDs, true, copperImageSize, board, GraphicsUtils::StandardFritzingDPI, false, false, true, empty);
		if (fillGroundTraces) showGroundTraces(seeds, true);
		if (svg2.isEmpty()) {
			QMessageBox::critical(NULL, tr("Fritzing"), tr("Fritzing error: unable to render copper svg (1)."));
			return false;
		}
	}

	QStringList exceptions;
	exceptions << "none" << "" << background().name();    // the color of holes in the board

	GroundPlaneGenerator gpg;
	gpg.setBlurBy(BlurBy);
	gpg.setLayerName("groundplane");
	gpg.setStrokeWidthIncrement(StrokeWidthIncrement);
	gpg.setMinRunSize(10, 10);
	if (fillGroundTraces) {
		connect(&gpg, SIGNAL(postImageSignal(GroundPlaneGenerator *, QImage *, QGraphicsItem *)), 
				this, SLOT(postImageSlot(GroundPlaneGenerator *, QImage *, QGraphicsItem *)));
	}

	bool result = gpg.generateGroundPlane(boardSvg, boardImageSize, svg, copperImageSize, exceptions, board, GraphicsUtils::StandardFritzingDPI / 2.0  /* 2 MIL */,
											ViewLayer::Copper0Color);
	if (result == false) {
        QMessageBox::critical(NULL, tr("Fritzing"), tr("Fritzing error: unable to write copper fill (1)."));
		return false;
	}

	GroundPlaneGenerator gpg2;
	if (boardLayers() > 1) {
		gpg2.setBlurBy(BlurBy);
		gpg2.setLayerName("groundplane1");
		gpg2.setStrokeWidthIncrement(StrokeWidthIncrement);
		gpg2.setMinRunSize(10, 10);
		if (fillGroundTraces) {
			connect(&gpg2, SIGNAL(postImageSignal(GroundPlaneGenerator *, QImage *, QGraphicsItem *)), 
					this, SLOT(postImageSlot(GroundPlaneGenerator *, QImage *, QGraphicsItem *)));
		}
		bool result = gpg2.generateGroundPlane(boardSvg, boardImageSize, svg2, copperImageSize, exceptions, board, GraphicsUtils::StandardFritzingDPI / 2.0  /* 2 MIL */,
												ViewLayer::Copper1Color);
		if (result == false) {
			QMessageBox::critical(NULL, tr("Fritzing"), tr("Fritzing error: unable to write copper fill (2)."));
			return false;
		}
	}


	QString fillType = (fillGroundTraces) ? GroundPlane::fillTypeGround : GroundPlane::fillTypePlain;
	QRectF bsbr = board->sceneBoundingRect();

	int ix = 0;
	foreach (QString svg, gpg.newSVGs()) {
		ViewGeometry vg;
		vg.setLoc(bsbr.topLeft() + gpg.newOffsets()[ix++]);
		long newID = ItemBase::getNextID();
		new AddItemCommand(this, BaseCommand::CrossView, ModuleIDNames::GroundPlaneModuleIDName, ViewLayer::GroundPlane_Bottom, vg, newID, false, -1, parentCommand);
		new SetPropCommand(this, newID, "svg", svg, svg, true, parentCommand);
		new SetPropCommand(this, newID, "fillType", fillType, fillType, false, parentCommand);
	}

	ix = 0;
	foreach (QString svg, gpg2.newSVGs()) {
		ViewGeometry vg;
		vg.setLoc(bsbr.topLeft() + gpg2.newOffsets()[ix++]);
		long newID = ItemBase::getNextID();
		new AddItemCommand(this, BaseCommand::CrossView, ModuleIDNames::GroundPlaneModuleIDName, ViewLayer::GroundPlane_Top, vg, newID, false, -1, parentCommand);
		new SetPropCommand(this, newID, "svg", svg, svg, true, parentCommand);
		new SetPropCommand(this, newID, "fillType", fillType, fillType, false, parentCommand);
	}

	return true;

}

QString PCBSketchWidget::generateCopperFillUnit(ItemBase * itemBase, QPointF whereToStart)
{
	QList<ItemBase *> boards = findBoard();
    // barf an error if there's no board
    if (boards.count() == 0) {
        QMessageBox::critical(NULL, tr("Fritzing"),
                   tr("Your sketch does not have a board yet!  Please add a PCB in order to use copper fill."));
        return "";
    }
    if (boards.count() > 1) {
        QMessageBox::critical(NULL, tr("Fritzing"),
                   tr("Copper fill: multiple boards are not supported."));
        return "";
    }

	ItemBase * board = boards.at(0);

	QRectF bsbr = board->sceneBoundingRect();
	if (!bsbr.contains(whereToStart)) {
        QMessageBox::critical(NULL, tr("Fritzing"), tr("Unable to create copper fill--probably the part wasn't dropped onto the PCB."));
		return "";
	}

	LayerList viewLayerIDs;
	viewLayerIDs << ViewLayer::Board;
	QSizeF boardImageSize;
	bool empty;
	QString boardSvg = renderToSVG(FSvgRenderer::printerScale(), viewLayerIDs, true, boardImageSize, board, GraphicsUtils::StandardFritzingDPI, false, false, false, empty);
	if (boardSvg.isEmpty()) {
        QMessageBox::critical(NULL, tr("Fritzing"), tr("Fritzing error: unable to render board svg (1)."));
		return "";
	}

	ViewLayer::ViewLayerSpec viewLayerSpec = ViewLayer::Bottom;
	QString color = ViewLayer::Copper0Color;
	QString gpLayerName = "groundplane";
	if (m_boardLayers == 2 && layerIsActive(ViewLayer::Copper1)) {
		gpLayerName += "1";
		color = ViewLayer::Copper1Color;
		viewLayerSpec = ViewLayer::Top;
	}

	viewLayerIDs = ViewLayer::copperLayers(viewLayerSpec);
	QSizeF copperImageSize;

	bool vis = itemBase->isVisible();
	itemBase->setVisible(false);
	QString svg = renderToSVG(FSvgRenderer::printerScale(), viewLayerIDs, true, copperImageSize, board, GraphicsUtils::StandardFritzingDPI, false, false, true, empty);
	itemBase->setVisible(vis);
	if (svg.isEmpty()) {
        QMessageBox::critical(NULL, tr("Fritzing"), tr("Fritzing error: unable to render copper svg (1)."));
		return "";
	}

	QStringList exceptions;
	exceptions << "none" << "" << background().name();    // the color of holes in the board

	GroundPlaneGenerator gpg;
	gpg.setBlurBy(BlurBy);
	gpg.setStrokeWidthIncrement(StrokeWidthIncrement);
	gpg.setLayerName(gpLayerName);
	gpg.setMinRunSize(10, 10);
	bool result = gpg.generateGroundPlaneUnit(boardSvg, boardImageSize, svg, copperImageSize, exceptions, board, GraphicsUtils::StandardFritzingDPI / 2.0  /* 2 MIL */, 
												color, whereToStart);

	if (result == false || gpg.newSVGs().count() < 1) {
        QMessageBox::critical(NULL, tr("Fritzing"), tr("Unable to create copper fill--possibly the part was dropped onto another part or wire rather than the actual PCB."));
		return "";
	}

	itemBase->setPos(bsbr.topLeft() + gpg.newOffsets()[0]);
	itemBase->setViewLayerID(gpLayerName, m_viewLayers);

	return gpg.newSVGs()[0];
}


bool PCBSketchWidget::connectorItemHasSpec(ConnectorItem * connectorItem, ViewLayer::ViewLayerSpec spec) {
	if (ViewLayer::specFromID(connectorItem->attachedToViewLayerID()) == spec)  return true;

	connectorItem = connectorItem->getCrossLayerConnectorItem();
	if (connectorItem == NULL) return false;

	return (ViewLayer::specFromID(connectorItem->attachedToViewLayerID()) == spec);
}

ViewLayer::ViewLayerSpec PCBSketchWidget::createWireViewLayerSpec(ConnectorItem * from, ConnectorItem * to) {
	QList<ViewLayer::ViewLayerSpec> guesses;
	guesses.append(layerIsActive(ViewLayer::Copper0) ? ViewLayer::Bottom : ViewLayer::Top);
	guesses.append(layerIsActive(ViewLayer::Copper0) ? ViewLayer::Top : ViewLayer::Bottom);
	foreach (ViewLayer::ViewLayerSpec guess, guesses) {
		if (connectorItemHasSpec(from, guess) && connectorItemHasSpec(to, guess)) {
			return guess;
		}
	}

	return ViewLayer::UnknownSpec;
}

double PCBSketchWidget::getWireStrokeWidth(Wire * wire, double wireWidth)
{
	double w, h;
	wire->originalConnectorDimensions(w, h);
	if (wireWidth < Wire::THIN_TRACE_WIDTH) {
		wire->setConnectorDimensions(qMin(w, wireWidth + 1), qMin(w, wireWidth + 1));
	}
	if (wireWidth < Wire::STANDARD_TRACE_WIDTH) {
		wire->setConnectorDimensions(qMin(w, wireWidth + 1.5), qMin(w, wireWidth + 1.5));
	}
	else {
		wire->setConnectorDimensions(w, h);
	}

	return wireWidth + 6;
}

Wire * PCBSketchWidget::createTempWireForDragging(Wire * fromWire, ModelPart * wireModel, ConnectorItem * connectorItem, ViewGeometry & viewGeometry, ViewLayer::ViewLayerSpec spec) 
{
	if (spec == ViewLayer::UnknownSpec) {
		spec = wireViewLayerSpec(connectorItem);
	}
	viewGeometry.setPCBTrace(true);
	Wire * wire =  SketchWidget::createTempWireForDragging(fromWire, wireModel, connectorItem, viewGeometry, spec);
	if (fromWire == NULL) {
		wire->setColorString(traceColor(connectorItem), 1.0);
		double traceWidth = getTraceWidth();
		double minDim = connectorItem->minDimension();
		if (minDim < traceWidth) {
			traceWidth = getSmallerTraceWidth(minDim);  
		}
		wire->setWireWidth(traceWidth, this, getWireStrokeWidth(wire, traceWidth));
		wire->setProperty(FakeTraceProperty, true);
	}
	else {
		wire->setColorString(fromWire->colorString(), fromWire->opacity());
		wire->setWireWidth(fromWire->width(), this, fromWire->hoverStrokeWidth());
	}

	return wire;
}

void PCBSketchWidget::prereleaseTempWireForDragging(Wire* wire)
{
	if (wire->property(PCBSketchWidget::FakeTraceProperty).toBool()) {
		// make it not look like a trace, or modifyNewWireConnections will create the wrong kind of wire
		wire->setWireFlags(0);
	}
}

bool PCBSketchWidget::curvyWiresIndicated(Qt::KeyboardModifiers)
{
	return false;
}

void PCBSketchWidget::rotatePartLabels(double degrees, QTransform & transform, QPointF center, QUndoCommand * parentCommand)
{
	ItemBase * board = NULL;
	foreach (ItemBase * itemBase, m_savedItems.values()) {
		if (itemBase->itemType() == ModelPart::ResizableBoard || itemBase->itemType() == ModelPart::Board) {
			board = itemBase->layerKinChief();
			break;
		}
	}

	if (board == NULL) return;

	QRectF bbr = board->sceneBoundingRect();

	foreach (QGraphicsItem * item, scene()->items()) {
		PartLabel * partLabel = dynamic_cast<PartLabel *>(item);
		if (partLabel == NULL) continue;
		if (!partLabel->isVisible()) continue;
		if (!bbr.intersects(partLabel->sceneBoundingRect())) continue;

		QPointF offset = partLabel->pos() - partLabel->owner()->pos();
		new MoveLabelCommand(this, partLabel->owner()->id(), partLabel->pos(), offset, partLabel->pos(), offset, parentCommand);
		new RotateFlipLabelCommand(this, partLabel->owner()->id(), degrees, 0, parentCommand);
		QPointF p = GraphicsUtils::calcRotation(transform, center, partLabel->pos(), partLabel->boundingRect().center());
		ViewGeometry vg;
		partLabel->owner()->calcRotation(transform, center, vg);
		new MoveLabelCommand(this, partLabel->owner()->id(), p, p - vg.loc(), p, p - vg.loc(), parentCommand);
	}
}

QString PCBSketchWidget::characterizeGroundFill() {
	QString result = GroundPlane::fillTypeNone;
	bool gotZero = false;
	bool gotOne = false;

	foreach (QGraphicsItem * item, scene()->items()) {
		GroundPlane * gp = dynamic_cast<GroundPlane *>(item);
		if (gp == NULL) continue;

		if (gp->viewLayerID() == ViewLayer::GroundPlane0) {
			gotZero = true;
		}
		else if (gp->viewLayerID() == ViewLayer::GroundPlane1) {
			gotOne = true;
		}

		if (gotZero && gotOne) break;
	}

	if (!(gotZero && gotOne)) return result;

	foreach (QGraphicsItem * item, scene()->items()) {
		GroundPlane * gp = dynamic_cast<GroundPlane *>(item);
		if (gp == NULL) continue;

		QString fillType = gp->prop("fillType");
		if (fillType.isEmpty()) {
			// old style fill with no property
			return GroundPlane::fillTypeGround;
		}

		if (fillType == GroundPlane::fillTypeGround) {
			// assumes multiple fill types are not possible
			return fillType;
		}

		if (fillType == GroundPlane::fillTypePlain) {
			// assumes multiple fill types are not possible
			return fillType;
		}

		result = fillType;
	}

	return result;
}

void PCBSketchWidget::setUpColor(ConnectorItem * fromConnectorItem, ConnectorItem * toConnectorItem, Wire * wire, QUndoCommand * parentCommand) {
		ViewGeometry::WireFlags flags =  getTraceFlag();
		QString tc = traceColor(fromConnectorItem);
		new WireColorChangeCommand(this, wire->id(), tc, tc, 1.0, 1.0, parentCommand);
		double traceWidth = getTraceWidth();
		if (autorouteTypePCB()) {
			double minDim = qMin(fromConnectorItem->minDimension(), toConnectorItem->minDimension());
			if (minDim < traceWidth) {
				traceWidth = getSmallerTraceWidth(minDim);  
			}
		}
		new WireWidthChangeCommand(this, wire->id(), traceWidth, traceWidth, parentCommand);

}

ViewGeometry::WireFlag PCBSketchWidget::getTraceFlag() {
	return ViewGeometry::PCBTraceFlag;
}

void PCBSketchWidget::postImageSlot(GroundPlaneGenerator * gpg, QImage * image, QGraphicsItem * board) {

	if (m_groundFillSeeds == NULL) return;

	ViewLayer::ViewLayerID viewLayerID = (gpg->layerName() == "groundplane") ? ViewLayer::Copper0 : ViewLayer::Copper1;

	QRectF boardRect = board->sceneBoundingRect();

	QList<QRectF> rects;
	foreach (ConnectorItem * connectorItem, *m_groundFillSeeds) {
		if (connectorItem->attachedToViewLayerID() != viewLayerID) continue;
		if (connectorItem->attachedToItemType() == ModelPart::Wire) continue;
		if (!connectorItem->attachedTo()->isEverVisible()) continue;

		//connectorItem->debugInfo("post image b");
		QRectF r = connectorItem->sceneBoundingRect();
		//DebugDialog::debug("pb", r);
		QRectF check = r;
		check.setLeft(r.right());
		check.setRight(r.right() + r.width());
		bool checkRight = !hasNeighbor(connectorItem, viewLayerID, check);

		check = r;
		check.setLeft(r.left() - r.width());
		check.setRight(r.left());
		bool checkLeft = !hasNeighbor(connectorItem, viewLayerID, check);

		check = r;
		check.setTop(r.bottom());
		check.setBottom(r.bottom() + r.height());
		bool checkDown = !hasNeighbor(connectorItem, viewLayerID, check);

		check = r;
		check.setTop(r.top() - r.width());
		check.setBottom(r.top());
		bool checkUp = !hasNeighbor(connectorItem, viewLayerID, check);

		double x1 = (r.left() - boardRect.left()) * image->width() / boardRect.width();
		double x2 = (r.right() - boardRect.left()) * image->width() / boardRect.width();
		double y1 = (r.top() - boardRect.top()) * image->height() / boardRect.height();
		double y2 = (r.bottom() - boardRect.top()) * image->height() / boardRect.height();
		double w = x2 - x1;
		double h = y2 - y1;

		double cw = w / 4;
		double ch = h / 4;
		int cx = (x1 + x2) /2;
		int cy = (y1 + y2) /2;

		int rad = qFloor(connectorItem->calcClipRadius() * image->width() / boardRect.width());

		double borderl = x1 - w;
		double borderr = x2 + w;
		double bordert = y1 - h;
		double borderb = y2 + h;

		// check left, up, right, down for groundplane, and if it's there draw to it from the connector

		if (checkUp){
			for (int y = y1; y > bordert; y--) {
				if (image->pixel(cx, y) & 0xffffff) {
					QRectF s(cx - cw, y - 1, cw + cw, cy - y - rad);
					rects.append(s);
					break;
				}
			}
		}
		if (checkDown) {
			for (int y = y2; y < borderb; y++) {
				if (image->pixel(cx, y) & 0xffffff) {
					QRectF s(cx - cw, cy + rad, cw + cw, y - cy - rad);
					rects.append(s);
					break;
				}
			}
		}
		if (checkLeft) {
			for (int x = x1; x > borderl; x--) {
				if (image->pixel(x, cy) & 0xffffff) {
					QRectF s(x - 1, cy - ch, cx - x - rad, ch + ch);
					rects.append(s);
					break;
				}
			}
		}
		if (checkRight) {
			for (int x = x2; x < borderr; x++) {
				if (image->pixel(x, cy) & 0xffffff) {
					QRectF s(cx + rad, cy - ch, x - cx - rad, ch + ch);
					rects.append(s);
					break;
				}
			}
		}

		DebugDialog::debug(QString("x1:%1 y1:%2 x2:%3 y2:%4").arg(x1).arg(y1).arg(x2).arg(y2));
	}

	if (rects.count() == 0) return;

	QPainter painter(image);
	foreach (QRectF r, rects) {
		painter.fillRect(r, QColor(255, 255, 255));
	}
	painter.end();
#ifndef QT_NO_DEBUG
	image->save("testGroundFillCopperPost.png");
#endif
}

bool PCBSketchWidget::hasNeighbor(ConnectorItem * connectorItem, ViewLayer::ViewLayerID viewLayerID, const QRectF & r) 
{
	foreach (QGraphicsItem * item, scene()->items(r)) {
		ConnectorItem * ci = dynamic_cast<ConnectorItem *>(item);
		if (ci != NULL) {
			if (ci->attachedToViewLayerID() != viewLayerID) continue;
			if (!ci->attachedTo()->isEverVisible()) continue;
			if (ci == connectorItem) continue;

			return true;
		}

		TraceWire * traceWire = dynamic_cast<TraceWire *>(item);
		if (traceWire != NULL) {
			if (!sameElectricalLayer2(traceWire->viewLayerID(), viewLayerID)) continue;
			if (!traceWire->isTraceType(getTraceFlag())) continue;

			return true;
		}
	}

	return false;
}

void PCBSketchWidget::hideCopperLogoItems(QList<ItemBase *> & copperLogoItems)
{
	foreach (QGraphicsItem * item, this->items()) {
		CopperLogoItem * logoItem = dynamic_cast<CopperLogoItem *>(item);
		if (logoItem && logoItem->isVisible()) {
			copperLogoItems.append(logoItem);
			logoItem->setVisible(false);
		}
	}
}

void PCBSketchWidget::restoreCopperLogoItems(QList<ItemBase *> & copperLogoItems)
{
	foreach (ItemBase * logoItem, copperLogoItems) {
		logoItem->setVisible(true);
	}
}


void PCBSketchWidget::clearGroundFillSeeds() 
{
	QList<ConnectorItem *> trueSeeds;

	foreach (QGraphicsItem * item, scene()->items()) {
		ConnectorItem * connectorItem = dynamic_cast<ConnectorItem *>(item);
		if (connectorItem == NULL) continue;
		if (connectorItem->attachedToItemType() == ModelPart::CopperFill) continue;

		if (connectorItem->isGroundFillSeed()) {
			trueSeeds.append(connectorItem);
			continue;
		}
	}

	if (trueSeeds.count() == 0) return;

	GroundFillSeedCommand * command = new GroundFillSeedCommand(this, NULL);
	command->setText(tr("Clear ground fill seeds"));
	foreach (ConnectorItem * connectorItem, trueSeeds) {
		command->addItem(connectorItem->attachedToID(), connectorItem->connectorSharedID(), false);
	}

	m_undoStack->waitPush(command, PropChangeDelay);
}


void PCBSketchWidget::setGroundFillSeeds() 
{
	setGroundFillSeeds("");
}

void PCBSketchWidget::setGroundFillSeeds(const QString & intro) 
{
	QList<ConnectorItem *> seeds;
	collectGroundFillSeeds(seeds, true);
	GroundFillSeedDialog gfsd(this, seeds, intro, NULL);
	int result = gfsd.exec();
	if (result == QDialog::Accepted) {
		GroundFillSeedCommand * command = NULL;
		QList<bool> results;
		gfsd.getResults(results);
		bool checked = false;
		for (int i = 0; i < seeds.count(); i++) {
			ConnectorItem * ci = seeds.at(i);
			bool isSeed = results.at(i);
			checked |= isSeed;
			if (isSeed != ci->isGroundFillSeed()) {
				if (command == NULL) {
					command = new GroundFillSeedCommand(this, NULL);
				}
				command->addItem(ci->attachedToID(), ci->connectorSharedID(), isSeed);
			}
		}
		if (command) {
			m_undoStack->push(command);
		}

		if (gfsd.getFill()) {
			if (checked) emit groundFillSignal();
			else emit copperFillSignal();
		}
	}
}

bool PCBSketchWidget::collectGroundFillSeeds(QList<ConnectorItem *> & seeds, bool includePotential) {
	QList<ConnectorItem *> trueSeeds;
	QList<ConnectorItem *> potentialSeeds;

	foreach (QGraphicsItem * item, scene()->items()) {
		ConnectorItem * connectorItem = dynamic_cast<ConnectorItem *>(item);
		if (connectorItem == NULL) continue;
		if (connectorItem->attachedToItemType() == ModelPart::CopperFill) continue;

		if (connectorItem->isGroundFillSeed()) {
			trueSeeds.append(connectorItem);
			continue;
		}

		if (connectorItem->isGrounded()) {
			potentialSeeds.append(connectorItem);
		}
	}

	for (int ix = 0; ix < trueSeeds.count(); ix++) {
		ConnectorItem * ci = trueSeeds.at(ix);
		QList<ConnectorItem *> cis;
		cis.append(ci);
		ConnectorItem::collectEqualPotential(cis, true, ViewGeometry::NoFlag);
		foreach (ConnectorItem * eq, cis) {
			if (eq != ci) trueSeeds.removeAll(eq);
			potentialSeeds.removeAll(eq);
		}
	}

	for (int ix = 0; ix < potentialSeeds.count(); ix++) {
		ConnectorItem * ci = potentialSeeds.at(ix);
		QList<ConnectorItem *> cis;
		cis.append(ci);
		ConnectorItem::collectEqualPotential(cis, true, ViewGeometry::NoFlag);
		foreach (ConnectorItem * eq, cis) {
			if (eq != ci) potentialSeeds.removeAll(eq);
		}
	}

	seeds.append(trueSeeds);
	if (trueSeeds.count() == 0 || includePotential) {
		seeds.append(potentialSeeds);
	}
	
	return trueSeeds.count() > 0;
}

void PCBSketchWidget::shiftHoles() {
	// vias and holes before version 0.7.3 did not have offset

	VersionThing versionThingOffset;
	versionThingOffset.majorVersion = 0;
	versionThingOffset.minorVersion = 7;
	versionThingOffset.minorSubVersion = 2;
	versionThingOffset.releaseModifier = "b";
	VersionThing versionThingFz;
	Version::toVersionThing(m_sketchModel->fritzingVersion(), versionThingFz);
	bool doShift = !Version::greaterThan(versionThingOffset, versionThingFz);
	if (!doShift) return;

	foreach (QGraphicsItem * item, scene()->items()) {
		ItemBase * itemBase = dynamic_cast<ItemBase *>(item);
		if (itemBase == NULL) continue;

		switch (itemBase->itemType()) {
			case ModelPart::Via:
			case ModelPart::Hole:
				itemBase->setPos(itemBase->pos().x() - (Hole::OffsetPixels / 2), itemBase->pos().y() - (Hole::OffsetPixels / 2));
				break;

			default:
				continue;
		}		
	}
}
