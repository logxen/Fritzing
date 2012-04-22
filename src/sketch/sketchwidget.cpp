/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2011 Fachhochschule Potsdam - http://fh-potsdam.de

Fritzing is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Fritzing is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty ofro
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Fritzing.  If not, see <http://www.gnu.org/licenses/>.

********************************************************************

$Revision: 5979 $:
$Author: cohen@irascible.com $:
$Date: 2012-04-18 21:45:53 -0700 (Wed, 18 Apr 2012) $

********************************************************************/


#include <QtGui>
#include <QGraphicsScene>
#include <QPoint>
#include <QPair>
#include <QMatrix>
#include <QtAlgorithms>
#include <QPen>
#include <QColor>
#include <QRubberBand>
#include <QLine>
#include <QHash>
#include <QMultiHash>
#include <QBrush>
#include <QGraphicsItem>
#include <QMainWindow>
#include <QApplication>
#include <QDomElement>
#include <QSettings>
#include <limits>

#include "../items/partfactory.h"
#include "../items/paletteitem.h"
#include "../items/logoitem.h"
#include "../items/pad.h"
#include "../items/ruler.h"
#include "../items/symbolpaletteitem.h"
#include "../items/wire.h"
#include "../commands.h"
#include "../model/modelpart.h"
#include "../debugdialog.h"
#include "../items/layerkinpaletteitem.h"
#include "sketchwidget.h"
#include "../connectors/connectoritem.h"
#include "../items/jumperitem.h"
#include "../items/stripboard.h"
#include "../items/virtualwire.h"
#include "../items/tracewire.h"
#include "../itemdrag.h"
#include "../layerattributes.h"
#include "../waitpushundostack.h"
#include "../fgraphicsscene.h"
#include "../version/version.h"
#include "../items/partlabel.h"
#include "../items/note.h"
#include "../svg/svgfilesplitter.h"
#include "../svg/svgflattener.h"
#include "../help/sketchmainhelp.h"
#include "../infoview/htmlinfoview.h"
#include "../items/resizableboard.h"
#include "../utils/graphicsutils.h"
#include "../utils/textutils.h"
#include "../utils/bezier.h"
#include "../utils/cursormaster.h"
#include "../fsvgrenderer.h"
#include "../items/resistor.h"
#include "../items/mysterypart.h"
#include "../items/pinheader.h"
#include "../items/dip.h"
#include "../items/groundplane.h"
#include "../items/moduleidnames.h"
#include "../items/hole.h"
#include "../items/capacitor.h"
#include "../utils/graphutils.h"
#include "../utils/ratsnestcolors.h"

/////////////////////////////////////////////////////////////////////

SizeItem::SizeItem()
{
}

SizeItem::~SizeItem()
{
}

/////////////////////////////////////////////////////////////////////

enum ConnectionStatus {
	IN_,
	OUT_,
	FREE_,
	UNDETERMINED_
};

static const double CloseEnough = 0.5;  // in pixels, for swapping into the breadboard

const int SketchWidget::MoveAutoScrollThreshold = 5;
const int SketchWidget::DragAutoScrollThreshold = 10;
static const int AutoRepeatDelay = 750;
const int SketchWidget::PropChangeDelay = 100;

/////////////////////////////////////////////////////////////////////

bool zLessThan(QGraphicsItem * & p1, QGraphicsItem * & p2)
{
	return p1->zValue() < p2->zValue();
}

/////////////////////////////////////////////////////////////////////

SketchWidget::SketchWidget(ViewIdentifierClass::ViewIdentifier viewIdentifier, QWidget *parent, int size, int minSize)
    : InfoGraphicsView(parent)
{
	m_rubberBandLegWasEnabled = m_curvyWires = false;
	m_middleMouseIsPressed = false;
	m_arrowTimer.setParent(this);
	m_arrowTimer.setInterval(AutoRepeatDelay);
	m_arrowTimer.setSingleShot(true);
	connect(&m_arrowTimer, SIGNAL(timeout()), this, SLOT(arrowTimerTimeout()));
	m_addDefaultParts = false;
	m_addedDefaultPart = NULL;
	m_movingItem = NULL;
	m_movingSVGRenderer = NULL;
	m_clearSceneRect = false;
	m_draggingBendpoint = false;
	m_zoom = 100;
	m_showGrid = m_alignToGrid = false;
	m_movingByMouse = m_movingByArrow = false;
	m_statusConnectState = StatusConnectNotTried;
	m_dragBendpointWire = NULL;
	m_lastHoverEnterItem = NULL;
	m_lastHoverEnterConnectorItem = NULL;
	m_fixedToCenterItem = NULL;
	m_spaceBarWasPressed = m_spaceBarIsPressed = false;
	m_current = false;
	m_ignoreSelectionChangeEvents = 0;
	m_droppingItem = NULL;
	m_chainDrag = false;
	m_bendpointWire = m_connectorDragWire = NULL;
	m_tempDragWireCommand = m_holdingSelectItemCommand = NULL;
	m_viewIdentifier = viewIdentifier;
	//setAlignment(Qt::AlignLeft | Qt::AlignTop);
	setDragMode(QGraphicsView::RubberBandDrag);
    setFrameStyle(QFrame::Sunken | QFrame::StyledPanel);
    setAcceptDrops(true);
	setRenderHint(QPainter::Antialiasing, true);

	//setCacheMode(QGraphicsView::CacheBackground);
	//setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	//setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

	setTransformationAnchor(QGraphicsView::AnchorViewCenter);
	//setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
	//setTransformationAnchor(QGraphicsView::NoAnchor);
    FGraphicsScene* scene = new FGraphicsScene(this);
    this->setScene(scene);

    //this->scene()->setSceneRect(0,0, rect().width(), rect().height());

    // Setting the scene rect here seems to mean it never resizes when the user drags an object
    // outside the sceneRect bounds.  So catch some signal and do the resize manually?
    // this->scene()->setSceneRect(0, 0, 500, 500);

    // if the sceneRect isn't set, the view seems to grow and scroll gracefully as new items are added
    // however, it doesn't shrink if items are removed.

    // a bit of a hack so that, when there is no scenerect set,
    // the first item dropped into the scene doesn't leap to the top left corner
    // as the scene resizes to fit the new item
   	m_sizeItem = new SizeItem();
    m_sizeItem->setLine(0, 0, rect().width(), rect().height());
	//DebugDialog::debug(QString("initial rect %1 %2").arg(rect().width()).arg(rect().height()));
    this->scene()->addItem(m_sizeItem);
    m_sizeItem->setVisible(false);
	
	connect(this->scene(), SIGNAL(selectionChanged()), this, SLOT(selectionChangedSlot()));

	connect(QApplication::clipboard(),SIGNAL(changed(QClipboard::Mode)),this,SLOT(restartPasteCount()));
    restartPasteCount(); // the first time

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    resize(size, size);
    setMinimumSize(minSize, minSize);

    setLastPaletteItemSelected(NULL);

    m_infoViewOnHover = true;

	setMouseTracking(true);

}

SketchWidget::~SketchWidget() {
	foreach (ViewLayer * viewLayer, m_viewLayers.values()) {
		if (viewLayer == NULL) continue;

		delete viewLayer;
	}
	m_viewLayers.clear();
}

void SketchWidget::restartPasteCount() {
	m_pasteCount = 0;
}

WaitPushUndoStack* SketchWidget::undoStack() {
	return m_undoStack;
}

void SketchWidget::setUndoStack(WaitPushUndoStack * undoStack) {
	m_undoStack = undoStack;
}

void SketchWidget::loadFromModelParts(QList<ModelPart *> & modelParts, BaseCommand::CrossViewType crossViewType, QUndoCommand * parentCommand, bool offsetPaste, const QRectF * boundingRect, bool seekOutsideConnections, QList<long> & newIDs) {
	clearHoldingSelectItem();

	if (parentCommand) {
		SelectItemCommand * selectItemCommand = stackSelectionState(false, parentCommand);
		selectItemCommand->setSelectItemType(SelectItemCommand::DeselectAll);
		selectItemCommand->setCrossViewType(crossViewType);
		new CleanUpWiresCommand(this, CleanUpWiresCommand::UndoOnly, parentCommand);
	}

	QHash<long, ItemBase *> newItems;
	setIgnoreSelectionChangeEvents(true);

	QString viewName = ViewIdentifierClass::viewIdentifierXmlName(m_viewIdentifier);
	QMultiMap<double, ItemBase *> zmap;

	QPointF sceneCenter = mapToScene(viewport()->rect().center());

	QPointF sceneCorner;
	if (boundingRect) {
		sceneCorner.setX(sceneCenter.x() - (boundingRect->width() / 2));
		sceneCorner.setY(sceneCenter.y() - (boundingRect->height() / 2));
	}

	// make parts
	foreach (ModelPart * mp, modelParts) {
		QDomElement instance = mp->instanceDomElement();
		if (instance.isNull()) continue;

		QDomElement views = instance.firstChildElement("views");
		if (views.isNull()) continue;

		QDomElement view = views.firstChildElement(viewName);
		if (view.isNull()) continue;

		bool locked = view.attribute("locked", "").compare("true") == 0;

		QDomElement geometry = view.firstChildElement("geometry");
		if (geometry.isNull()) continue;

		ViewGeometry viewGeometry(geometry);

		QDomElement labelGeometry = view.firstChildElement("titleGeometry");
		
		ViewLayer::ViewLayerSpec viewLayerSpec = getViewLayerSpec(mp, instance, view, viewGeometry);

		// use a function of the model index to ensure the same parts have the same ID across views
		long newID = ItemBase::getNextID(mp->modelIndex());
		if (parentCommand == NULL) {
			ItemBase * itemBase = addItemAux(mp, viewLayerSpec, viewGeometry, newID, NULL, true, m_viewIdentifier, false);
			if (itemBase != NULL) {
				if (locked) {
					itemBase->setMoveLock(true);
				}
				zmap.insert(viewGeometry.z() - qFloor(viewGeometry.z()), itemBase);   
				bool gotOne = false;
				if (!gotOne) {
					PaletteItem * paletteItem = qobject_cast<PaletteItem *>(itemBase);
					if (paletteItem != NULL) {
						// wires don't have transforms
						paletteItem->setTransforms();
						gotOne = true;
					}
				}
				if (!gotOne) {
					Wire * wire = qobject_cast<Wire *>(itemBase);
					if (wire != NULL) {
						QDomElement extras = view.firstChildElement("wireExtras");
						wire->setExtras(extras, this);
						gotOne = true;
					}
				}
				if (!gotOne) {
					Note * note = qobject_cast<Note *>(itemBase);
					if (note != NULL) {
						note->setText(mp->instanceText(), true);
						gotOne = true;
					}
				}

				// use the modelIndex from mp, not from the newly created item, because we're mapping from the modelIndex in the xml file
				newItems.insert(mp->modelIndex(), itemBase);
				if (itemBase->itemType() != ModelPart::Wire) {
					itemBase->restorePartLabel(labelGeometry, getLabelViewLayerID(itemBase->viewLayerSpec()));
				}
			}
		}
		else {
			// offset pasted items so we can differentiate them from the originals
			if (offsetPaste) {
				if (m_pasteOffset.x() != 0 || m_pasteOffset.y() != 0) {
					viewGeometry.offset((20 * m_pasteCount) + m_pasteOffset.x(), (20 * m_pasteCount) + m_pasteOffset.y());
				}
				else if (boundingRect && !boundingRect->isNull()) {
					double dx = viewGeometry.loc().x() - boundingRect->left() + sceneCorner.x() + (20 * m_pasteCount);
					double dy = viewGeometry.loc().y() - boundingRect->top() + sceneCorner.y() + (20 * m_pasteCount);
					viewGeometry.setLoc(QPointF(dx, dy));
				}
			}
			newAddItemCommand(crossViewType, mp->moduleID(), viewLayerSpec, viewGeometry, newID, false, mp->modelIndex(), parentCommand);
			
			// TODO: all this part specific stuff should be in the PartFactory
			
			if (mp->itemType() == ModelPart::ResizableBoard) {
				bool ok;
				double w = mp->prop("width").toDouble(&ok);
				if (ok) {
					double h = mp->prop("height").toDouble(&ok);
					if (ok) {
						new ResizeBoardCommand(this, newID, w, h, w, h, parentCommand);
					}
				}
			}
			else if (mp->itemType() == ModelPart::Note) {
				ChangeNoteTextCommand * changeNoteTextCommand = new ChangeNoteTextCommand(this, newID, mp->instanceText(), mp->instanceText(), viewGeometry.rect().size(), viewGeometry.rect().size(), parentCommand);
				changeNoteTextCommand->setFirstTime(false);
			}
			else if (mp->itemType() == ModelPart::Ruler) {
				QString w = mp->prop("width").toString();
				QString w2 = w;
				w.chop(2);
				int units = w2.endsWith("cm") ? 0 : 1;
				new ResizeBoardCommand(this, newID, w.toDouble(), units, w.toDouble(), units, parentCommand);
				mp->setProp("width", "");		// ResizeBoardCommand won't execute if the width property is already set
			}

			if (locked) {
				new MoveLockCommand(this, newID, true, true, parentCommand);
			}

			if (!labelGeometry.isNull()) {
				QDomElement clone = labelGeometry.cloneNode(true).toElement();
				bool ok;
				double x = clone.attribute("x").toDouble(&ok);
				if (ok) {
					if (m_pasteOffset.x() == 0 && m_pasteOffset.y() == 0) {
						int dx = (boundingRect) ? boundingRect->left() : 0;
						x = x - dx + sceneCorner.x() + (20 * m_pasteCount);
					}
					else {
						x += (20 * m_pasteCount) + m_pasteOffset.x();
					}
					clone.setAttribute("x", QString::number(x));
				}
				double y = clone.attribute("y").toDouble(&ok);
				if (ok) {
					if (m_pasteOffset.x() == 0 && m_pasteOffset.y() == 0) {
						int dy = boundingRect ? boundingRect->top() : 0;
						y = y - dy + sceneCorner.y() + (20 * m_pasteCount);
					}
					else {
						y += (20 * m_pasteCount) + m_pasteOffset.y();
					}
					clone.setAttribute("y", QString::number(y));
				}
				new RestoreLabelCommand(this, newID, clone, parentCommand);
			}

			newIDs << newID;
			if (mp->moduleID() == ModuleIDNames::WireModuleIDName) {
				addWireExtras(newID, view, parentCommand);
			}
		}
	}

	foreach (long id, newIDs) {
		new CheckStickyCommand(this, crossViewType, id, false, CheckStickyCommand::RemoveOnly, parentCommand);
	}

	if (zmap.count() > 0) {
		double z = 0.5;
		foreach (ItemBase * itemBase, zmap.values()) {
			itemBase->slamZ(z);
			z += ViewLayer::getZIncrement();
		}
		foreach (ViewLayer * viewLayer, m_viewLayers) {
			if (viewLayer != NULL) viewLayer->resetNextZ(z);
		}
	}

	QStringList alreadyConnected;

	QHash<QString, QDomElement> legs;

	// now restore connections
	foreach (ModelPart * mp, modelParts) {
		QDomElement instance = mp->instanceDomElement();
		if (instance.isNull()) continue;

		QDomElement views = instance.firstChildElement("views");
		if (views.isNull()) continue;

		QDomElement view = views.firstChildElement(viewName);
		if (view.isNull()) continue;

		QDomElement connectors = view.firstChildElement("connectors");
		if (connectors.isNull()) continue;

		QDomElement connector = connectors.firstChildElement("connector");
		while (!connector.isNull()) {
			QString fromConnectorID = connector.attribute("connectorId");
			ViewLayer::ViewLayerID connectorViewLayerID = ViewLayer::viewLayerIDFromXmlString(connector.attribute("layer"));
			bool gfs = connector.attribute("groundFillSeed").compare("true") == 0;
			if (gfs) {
				ItemBase * fromBase = newItems.value(mp->modelIndex(), NULL);
				if (fromBase) {
					ConnectorItem * fromConnectorItem = fromBase->findConnectorItemWithSharedID(fromConnectorID, ViewLayer::specFromID(connectorViewLayerID));
					if (fromConnectorItem) {
						fromConnectorItem->setGroundFillSeed(true);
					}
				}
			}
			QDomElement connects = connector.firstChildElement("connects");
			if (!connects.isNull()) {
				QDomElement connect = connects.firstChildElement("connect");
				while (!connect.isNull()) {
					handleConnect(connect, mp, fromConnectorID, connectorViewLayerID, alreadyConnected, newItems, parentCommand, seekOutsideConnections);
					connect = connect.nextSiblingElement("connect");
				}
			}

			QDomElement leg = connector.firstChildElement("leg");
			if (!leg.isNull() && !leg.firstChildElement("point").isNull()) {
				if (parentCommand) {
					legs.insert(QString::number(ItemBase::getNextID(mp->modelIndex())) + "." + fromConnectorID, leg);
				}
				else {
					ItemBase * fromBase = newItems.value(mp->modelIndex(), NULL);
					if (fromBase) {
						legs.insert(QString::number(fromBase->id()) + "." + fromConnectorID, leg);
					}
				}
			}

			connector = connector.nextSiblingElement("connector");
		}
	}

	// must do legs after all connections are set up
	foreach (QString key, legs.keys()) {
		int ix = key.indexOf(".");
		if (ix <= 0) continue;

		QDomElement leg = legs.value(key);
		long id = key.left(ix).toInt();
		QString fromConnectorID = key.remove(0, ix + 1);

		QPolygonF poly = TextUtils::polygonFromElement(leg);
		if (poly.count() < 2) continue;

		if (parentCommand) {
			ChangeLegCommand * clc = new ChangeLegCommand(this, id, fromConnectorID, poly, poly, true, true, "copy", parentCommand);
			clc->setSimple();
		}
		else {
			changeLeg(id, fromConnectorID, poly, true, "load");
		}

		QDomElement bElement = leg.firstChildElement("bezier");
		int bIndex = 0;
		while (!bElement.isNull()) {
			Bezier bezier = Bezier::fromElement(bElement);
			if (!bezier.isEmpty()) {
				if (parentCommand) {
					new ChangeLegCurveCommand(this, id, fromConnectorID, bIndex, &bezier, &bezier, parentCommand);
				}
				else {
					changeLegCurve(id, fromConnectorID, bIndex, &bezier);
				}
			}
			bElement = bElement.nextSiblingElement("bezier");
			bIndex++;
		}
	}

	if (parentCommand == NULL) {
		foreach (ItemBase * item, newItems) {
			item->doneLoading();
			if (item->sticky()) {
				stickyScoop(item, false, NULL);
			}
		}

		m_pasteCount = 0;
		this->scene()->clearSelection();
		cleanUpWires(false, NULL);

	}
	else {
		if (offsetPaste) {
			// m_pasteCount used for offsetting paste items, not a count of how many items are pasted
			m_pasteCount++;
		}

		new CleanUpWiresCommand(this, CleanUpWiresCommand::RedoOnly, parentCommand);
	}

	setIgnoreSelectionChangeEvents(false);
	m_pasteOffset = QPointF(0,0);
}

void SketchWidget::handleConnect(QDomElement & connect, ModelPart * mp, const QString & fromConnectorID, ViewLayer::ViewLayerID fromViewLayerID, 
									QStringList & alreadyConnected, QHash<long, ItemBase *> & newItems, QUndoCommand * parentCommand, 
									bool seekOutsideConnections)
{
	bool ok;
	QHash<long, ItemBase *> otherNewItems;
	long modelIndex = connect.attribute("modelIndex").toLong(&ok);
	QString toConnectorID = connect.attribute("connectorId");
	ViewLayer::ViewLayerID toViewLayerID = ViewLayer::viewLayerIDFromXmlString(connect.attribute("layer"));
	QString already = ((mp->modelIndex() <= modelIndex) ? QString("%1.%2.%3.%4.%5.%6") : QString("%4.%5.%6.%1.%2.%3"))
						.arg(mp->modelIndex()).arg(fromConnectorID).arg(fromViewLayerID)
						.arg(modelIndex).arg(toConnectorID).arg(toViewLayerID);
	if (alreadyConnected.contains(already)) return;

	alreadyConnected.append(already);

	if (parentCommand == NULL) {
		ItemBase * fromBase = newItems.value(mp->modelIndex(), NULL);
		ItemBase * toBase = newItems.value(modelIndex, NULL);
		if (toBase == NULL) {
			toBase = otherNewItems.value(modelIndex, NULL);
		}
		if (fromBase == NULL || toBase == NULL) {
			if (!seekOutsideConnections) return;

			if (fromBase == NULL) {
				fromBase = findItem(mp->modelIndex() * ModelPart::indexMultiplier);
				if (fromBase == NULL) return;
			}

			if (toBase == NULL) {
				toBase = findItem(modelIndex * ModelPart::indexMultiplier);
				if (toBase == NULL) return;
			}
		}

		ConnectorItem * fromConnectorItem = fromBase->findConnectorItemWithSharedID(fromConnectorID, ViewLayer::specFromID(fromViewLayerID));
		ConnectorItem * toConnectorItem = toBase->findConnectorItemWithSharedID(toConnectorID, ViewLayer::specFromID(toViewLayerID));
		if (fromConnectorItem == NULL || toConnectorItem == NULL) {
			return;
		}

		fromConnectorItem->connectTo(toConnectorItem);
		toConnectorItem->connectTo(fromConnectorItem);
		fromConnectorItem->connector()->connectTo(toConnectorItem->connector());
		if (fromConnectorItem->attachedToItemType() == ModelPart::Wire && toConnectorItem->attachedToItemType() == ModelPart::Wire) {
			fromConnectorItem->setHidden(false);
			toConnectorItem->setHidden(false);
		}
		ratsnestConnect(fromConnectorItem, true);
		ratsnestConnect(toConnectorItem, true);
		return;
	}

	// single view because handle connect is called from loadModelPart which gets called for each view
	new ChangeConnectionCommand(this, BaseCommand::SingleView,
								ItemBase::getNextID(mp->modelIndex()), fromConnectorID,
								ItemBase::getNextID(modelIndex), toConnectorID,
								ViewLayer::specFromID(fromViewLayerID),
								true, parentCommand);
}

void SketchWidget::addWireExtras(long newID, QDomElement & view, QUndoCommand * parentCommand)
{
	QDomElement extras = view.firstChildElement("wireExtras");
	if (extras.isNull()) return;

	QDomElement copy(extras);

	new WireExtrasCommand(this, newID, copy, copy, parentCommand);
}

void SketchWidget::setWireExtras(long newID, const QDomElement & extras)
{
	Wire * wire = qobject_cast<Wire *>(findItem(newID));
	if (wire == NULL) return;

	wire->setExtras(extras, this);
}

ItemBase * SketchWidget::addItem(const QString & moduleID, ViewLayer::ViewLayerSpec viewLayerSpec, BaseCommand::CrossViewType crossViewType, const ViewGeometry & viewGeometry, long id, long modelIndex,  AddDeleteItemCommand * originatingCommand) {
	if (m_paletteModel == NULL) return NULL;

	ItemBase * itemBase = NULL;
	ModelPart * modelPart = m_paletteModel->retrieveModelPart(moduleID);

	if (modelPart != NULL) {
		QApplication::setOverrideCursor(Qt::WaitCursor);
		statusMessage(tr("loading part"));
		itemBase = addItem(modelPart, viewLayerSpec, crossViewType, viewGeometry, id, modelIndex, originatingCommand, NULL);
		statusMessage(tr("done loading"), 2000);
		QApplication::restoreOverrideCursor();
	}

	return itemBase;
}


ItemBase * SketchWidget::addItem(ModelPart * modelPart, ViewLayer::ViewLayerSpec viewLayerSpec, BaseCommand::CrossViewType crossViewType, const ViewGeometry & viewGeometry, long id, long modelIndex, AddDeleteItemCommand * originatingCommand, PaletteItem* partsEditorPaletteItem)
{
	
	ItemBase * newItem = NULL;
	//if (checkAlreadyExists) {
	//	newItem = findItem(id);
	//	if (newItem != NULL) {
	//		newItem->debugInfo("already exists");
	//	}
	//}
	if (newItem == NULL) {
		ModelPart * mp = NULL;
		if (modelIndex >= 0) {
			// used only with Paste, so far--this assures that parts created across views will share the same ModelPart
			mp = m_sketchModel->findModelPart(modelPart->moduleID(), id);
		}
		if (mp == NULL) {
			modelPart = m_sketchModel->addModelPart(m_sketchModel->root(), modelPart);
		}
		else {
			modelPart = mp;
		}
		if (modelPart == NULL) return NULL;
	
		newItem = addItemAux(modelPart, viewLayerSpec, viewGeometry, id, partsEditorPaletteItem, true, m_viewIdentifier, false);
	}

	if (crossViewType == BaseCommand::CrossView) {
		//DebugDialog::debug(QString("emit item added"));
		emit itemAddedSignal(modelPart, viewLayerSpec, viewGeometry, id, originatingCommand ? originatingCommand->dropOrigin() : NULL);
		//DebugDialog::debug(QString("after emit item added"));
	}

	return newItem;
}

ItemBase * SketchWidget::addItemAuxTemp(ModelPart * modelPart, ViewLayer::ViewLayerSpec viewLayerSpec, const ViewGeometry & viewGeometry, long id, PaletteItem* partsEditorPaletteItem, bool doConnectors, ViewIdentifierClass::ViewIdentifier viewIdentifier, bool temporary)
{
	modelPart = m_sketchModel->addModelPart(m_sketchModel->root(), modelPart);
	if (modelPart == NULL) return NULL;   // this is very fucked up

	return addItemAux(modelPart, viewLayerSpec, viewGeometry, id, partsEditorPaletteItem, doConnectors, viewIdentifier, temporary);
}

ItemBase * SketchWidget::addItemAux(ModelPart * modelPart, ViewLayer::ViewLayerSpec viewLayerSpec, const ViewGeometry & viewGeometry, long id, PaletteItem* partsEditorPaletteItem, bool doConnectors, ViewIdentifierClass::ViewIdentifier viewIdentifier, bool temporary)
{
	Q_UNUSED(partsEditorPaletteItem);
	if (viewIdentifier == ViewIdentifierClass::UnknownView) {
		viewIdentifier = m_viewIdentifier;
	}

	if (doConnectors) {
		modelPart->initConnectors();    // is a no-op if connectors already in place
	}

	ItemBase * newItem = PartFactory::createPart(modelPart, viewLayerSpec, viewIdentifier, viewGeometry, id, m_itemMenu, m_wireMenu, true);
	Wire * wire = qobject_cast<Wire *>(newItem);
	if (wire) {

		QString descr;
		bool ratsnest = viewGeometry.getRatsnest();
		if (ratsnest) {
			setClipEnds((ClipableWire *) wire, true);
			descr = "ratsnest";
		}
		else if (viewGeometry.getAnyTrace() ) {
			setClipEnds((ClipableWire *) wire, true);
			descr = "trace";
		}
		else {
			wire->setNormal(true);
			descr = "wire";
		}

		wire->setUp(getWireViewLayerID(viewGeometry, wire->viewLayerSpec()), m_viewLayers, this);
		setWireVisible(wire);
		wire->updateConnectors();

		addToScene(wire, wire->viewLayerID());
		wire->addedToScene(temporary);
		wire->debugInfo("add " + descr);

		return wire;
	}

	if (modelPart->itemType() == ModelPart::Note) {
		newItem->setViewLayerID(getNoteViewLayerID(), m_viewLayers);
		newItem->setZValue(newItem->z());
		newItem->setVisible(true);
		addToScene(newItem, getNoteViewLayerID());
		return newItem;
	}

	bool ok;
	addPartItem(modelPart, viewLayerSpec, (PaletteItem *) newItem, doConnectors, ok, viewIdentifier, temporary);
	newItem->debugInfo("add part");
	setNewPartVisible(newItem);
	newItem->updateConnectors();
	return newItem;
}


void SketchWidget::setNewPartVisible(ItemBase * itemBase) {
	Q_UNUSED(itemBase);
	// defaults to visible, so do nothing
}

void SketchWidget::checkSticky(long id, bool doEmit, bool checkCurrent, CheckStickyCommand * checkStickyCommand)
{
	ItemBase * itemBase = findItem(id);
	if (itemBase == NULL) return;

	if (itemBase->sticky()) {
		stickyScoop(itemBase, checkCurrent, checkStickyCommand);
	}
	else {
		ItemBase * stickyOne = overSticky(itemBase);
		ItemBase * wasStickyOne = itemBase->stickingTo();
		if (stickyOne != wasStickyOne) {
			if (wasStickyOne != NULL) {
				wasStickyOne->addSticky(itemBase, false);
				itemBase->addSticky(wasStickyOne, false);
				if (checkStickyCommand) {
					checkStickyCommand->stick(this, wasStickyOne->id(), itemBase->id(), false);
				}
			}
			if (stickyOne != NULL) {
				stickyOne->addSticky(itemBase, true);
				itemBase->addSticky(stickyOne, true);
				if (checkStickyCommand) {
					checkStickyCommand->stick(this, stickyOne->id(), itemBase->id(), true);
				}
			}
		}
	}

	if (doEmit) {
		checkStickySignal(id, false, false, checkStickyCommand);
	}
}

PaletteItem* SketchWidget::addPartItem(ModelPart * modelPart, ViewLayer::ViewLayerSpec viewLayerSpec, PaletteItem * paletteItem, bool doConnectors, bool & ok, ViewIdentifierClass::ViewIdentifier viewIdentifier, bool temporary) {

	ok = false;
	ViewLayer::ViewLayerID viewLayerID = getViewLayerID(modelPart, viewIdentifier, viewLayerSpec);

	// render it, only if the layer is defined in the fzp file
	// if the view is not defined in the part file, without this condition
	// fritzing crashes
	if(viewLayerID != ViewLayer::UnknownLayer) {
		QString error;
		if (paletteItem->renderImage(modelPart, viewIdentifier, m_viewLayers, viewLayerID, doConnectors, error)) {
			//DebugDialog::debug(QString("addPartItem %1").arg(viewIdentifier));
			addToScene(paletteItem, paletteItem->viewLayerID());
			paletteItem->loadLayerKin(m_viewLayers, viewLayerSpec);
			foreach (ItemBase * lkpi, paletteItem->layerKin()) {
				this->scene()->addItem(lkpi);
				lkpi->setHidden(!layerIsVisible(lkpi->viewLayerID()));
				lkpi->setInactive(!layerIsActive(lkpi->viewLayerID()));
			}
			//DebugDialog::debug(QString("after layerkin %1").arg(viewIdentifier));
			ok = true;
		}
		else {
			// nobody falls through to here now?

			QMessageBox::information(NULL, QObject::tr("Fritzing"),
				QObject::tr("Error reading file %1: %2.").arg(modelPart->path()).arg(error) );


			DebugDialog::debug(QString("addPartItem renderImage failed %1").arg(modelPart->moduleID()) );

			//paletteItem->modelPart()->removeViewItem(paletteItem);
			//delete paletteItem;
			//return NULL;
			scene()->addItem(paletteItem);
			//paletteItem->setVisible(false);
		}
		paletteItem->addedToScene(temporary);
		return paletteItem;

	} else {
		return paletteItem;
	}
}

void SketchWidget::addToScene(ItemBase * item, ViewLayer::ViewLayerID viewLayerID) {
	scene()->addItem(item);
 	item->setSelected(true);
 	item->setHidden(!layerIsVisible(viewLayerID));
 	item->setInactive(!layerIsActive(viewLayerID));
}

ItemBase * SketchWidget::findItem(long id) {
	// TODO:  this needs to be optimized: could make a hash table

	long baseid = id / ModelPart::indexMultiplier;  

	foreach (QGraphicsItem * item, this->scene()->items()) {
		ItemBase* base = dynamic_cast<ItemBase *>(item);
		if (base == NULL) continue;

		if (base->id() == id) {
			return base;
		}

		if (base->id() / ModelPart::indexMultiplier == baseid) {
			// found chief or layerkin
			ItemBase * chief = base->layerKinChief();
			if (chief->id() == id) return chief;

			foreach (ItemBase * lk, chief->layerKin()) {
				if (lk->id() == id) return lk;
			}

			if (chief->layerKin().count() == 0) {
				// going from layerkin in one view to non-layered part in another view
				return chief;
			}

			return NULL;
		}
	}

	return NULL;
}

void SketchWidget::deleteItem(long id, bool deleteModelPart, bool doEmit, bool later) {
	ItemBase * pitem = findItem(id);
	DebugDialog::debug(QString("delete item (1) %1 %2 %3 %4").arg(id).arg(doEmit).arg(m_viewIdentifier).arg((long) pitem, 0, 16) );
	if (pitem != NULL) {
		deleteItem(pitem, deleteModelPart, doEmit, later);
	}
	else {
		if (doEmit) {
			emit itemDeletedSignal(id);
		}
	}
}

void SketchWidget::deleteItem(ItemBase * itemBase, bool deleteModelPart, bool doEmit, bool later)
{
	long id = itemBase->id();
	DebugDialog::debug(QString("delete item (2) %1 %2 %3 %4").arg(id).arg(itemBase->title()).arg(m_viewIdentifier).arg((long) itemBase, 0, 16) );

	// this is a hack to try to workaround a Qt 4.7 crash in QGraphicsSceneFindItemBspTreeVisitor::visit 
	// when using a custom boundingRect, after deleting an item, it still appears on the visit list.
	//
	// the problem arises because the legItems are used to calculate the boundingRect() of the item.
	// But in the destructor, the childItems are deleted first, then the BSP tree is updated
	// at that point, the boundingRect() will return a different value than what's in the BSP tree,
	// which is the old value of the boundingRect before the legs were deleted.

	if (itemBase->hasRubberBandLeg()) {
		DebugDialog::debug("kill rubberBand");
		itemBase->killRubberBandLeg();
	}

	if (m_infoView != NULL) {
		m_infoView->unregisterCurrentItemIf(itemBase->id());
	}
	if (itemBase == this->m_lastPaletteItemSelected) {
		setLastPaletteItemSelected(NULL);
	}
	// m_lastSelected.removeOne(itemBase); hack for 4.5.something

	if (deleteModelPart) {
		ModelPart * modelPart = itemBase->modelPart();
		if (modelPart != NULL) {
			m_sketchModel->removeModelPart(modelPart);
			delete modelPart;
		}
	}

	itemBase->removeLayerKin();
	this->scene()->removeItem(itemBase);

	if (later) {
		itemBase->deleteLater();
	}
	else {
		delete itemBase;
	}

	if (doEmit) {
		emit itemDeletedSignal(id);
	}

}

void SketchWidget::deleteSelected(Wire * wire) {
	QSet<ItemBase *> itemBases;
	if (wire) {
		itemBases << wire;
	}
	else {
		foreach (QGraphicsItem * item, scene()->selectedItems()) {
			ItemBase * itemBase = dynamic_cast<ItemBase *>(item);
			if (itemBase == NULL) continue;
			if (itemBase->moveLock()) continue;

			itemBase = itemBase->layerKinChief();
			if (itemBase->moveLock()) continue;

			itemBases.insert(itemBase);
		}
	}

	// assumes ratsnest is not mixed with other itembases
	bool rats = true;
	foreach (ItemBase * itemBase, itemBases) {
		Wire * wire = qobject_cast<Wire *>(itemBase);
		if (wire == NULL) {
			rats = false;
			break;
		}
		if (!wire->getRatsnest()) {
			rats = false;
			break;
		}
	}

	if (!rats) {
		cutDeleteAux("Delete");			// wire is selected in this case, so don't bother sending it along
		return;
	}

	QUndoCommand * parentCommand = new QUndoCommand(tr("Delete ratsnest"));
	deleteRatsnest(wire, parentCommand);
	m_undoStack->waitPush(parentCommand, PropChangeDelay);
}

void SketchWidget::cutDeleteAux(QString undoStackMessage) {

	//DebugDialog::debug("before delete");

    // get sitems first, before calling stackSelectionState
    // because selectedItems will return an empty list
	const QList<QGraphicsItem *> sitems = scene()->selectedItems();

	QSet<ItemBase *> deletedItems;

	foreach (QGraphicsItem * sitem, sitems) {
		if (!canDeleteItem(sitem, sitems.count())) continue;

		// canDeleteItem insures dynamic_cast<ItemBase *>(sitem)->layerKinChief() won't break
		deletedItems.insert(dynamic_cast<ItemBase *>(sitem)->layerKinChief());
	}

	if (deletedItems.count() <= 0) {
		return;
	}

	QString string;
	if (deletedItems.count() == 1) {
		ItemBase * firstItem = *(deletedItems.begin());
		string = tr("%1 %2").arg(undoStackMessage).arg(firstItem->title());
	}
	else {
		string = tr("%1 %2 items").arg(undoStackMessage).arg(QString::number(deletedItems.count()));
	}

	QUndoCommand * parentCommand = new QUndoCommand(string);
	parentCommand->setText(string);

	deleteAux(deletedItems, parentCommand, true);
}

void SketchWidget::deleteAux(QSet<ItemBase *> & deletedItems, QUndoCommand * parentCommand, bool doPush) 
{
    stackSelectionState(false, parentCommand);

	new CleanUpWiresCommand(this, CleanUpWiresCommand::UndoOnly, parentCommand);

	// some day we won't have to go through all this crap because all items will exist in all 3 views.
	QHash<ItemBase *, SketchWidget *> otherDeletedItems;
	QList<long> deletedIDs;
	foreach(ItemBase * itemBase, deletedItems) {
		deletedIDs.append(itemBase->id());
	}
	emit deleteTracesSignal(deletedItems, otherDeletedItems, deletedIDs, true, parentCommand);

	deleteTracesSlot(deletedItems, otherDeletedItems, deletedIDs, false, parentCommand);
	foreach (ItemBase * itemBase, deletedItems) {
		otherDeletedItems.insert(itemBase, this);
	}
	deleteMiddle(otherDeletedItems, parentCommand);
	//emit deleteMoreTracesSignal(deletedItems, otherDeletedItems, parentCommand);

	new CleanUpWiresCommand(this, CleanUpWiresCommand::RedoOnly, parentCommand);

	// actual delete commands must come last for undo to work properly
	foreach (ItemBase * itemBase, otherDeletedItems.keys()) {
		SketchWidget * sketchWidget = otherDeletedItems.value(itemBase);
		sketchWidget->makeDeleteItemCommand(itemBase, BaseCommand::CrossView, parentCommand);
	}
	if (doPush) {
   		m_undoStack->waitPush(parentCommand, PropChangeDelay);
	}
}

bool isVirtualWireConnector(ConnectorItem * toConnectorItem) {
	return (qobject_cast<VirtualWire *>(toConnectorItem->attachedTo()) != NULL);
}

void SketchWidget::deleteMiddle(QHash<ItemBase *, SketchWidget *> & deletedItems, QUndoCommand * parentCommand) {
	foreach (ItemBase * itemBase, deletedItems.keys()) {
		foreach (ConnectorItem * fromConnectorItem, itemBase->cachedConnectorItems()) {
			foreach (ConnectorItem * toConnectorItem, fromConnectorItem->connectedToItems()) {
				deletedItems.value(itemBase)->extendChangeConnectionCommand(BaseCommand::CrossView, fromConnectorItem, toConnectorItem,
											  ViewLayer::specFromID(fromConnectorItem->attachedToViewLayerID()),
											  false, parentCommand);
				fromConnectorItem->tempRemove(toConnectorItem, false);
				toConnectorItem->tempRemove(fromConnectorItem, false);
			}

			fromConnectorItem = fromConnectorItem->getCrossLayerConnectorItem();
			if (fromConnectorItem) {
				foreach (ConnectorItem * toConnectorItem, fromConnectorItem->connectedToItems()) {
					deletedItems.value(itemBase)->extendChangeConnectionCommand(BaseCommand::CrossView, fromConnectorItem, toConnectorItem,
												  ViewLayer::specFromID(fromConnectorItem->attachedToViewLayerID()),
												  false, parentCommand);
					fromConnectorItem->tempRemove(toConnectorItem, false);
					toConnectorItem->tempRemove(fromConnectorItem, false);
				}
			}
		}
	}
}

void SketchWidget::deleteTracesSlot(QSet<ItemBase *> & deletedItems, QHash<ItemBase *, SketchWidget *> & otherDeletedItems, QList<long> & deletedIDs, bool isForeign, QUndoCommand * parentCommand) {
	Q_UNUSED(parentCommand);
	foreach (ItemBase * itemBase, deletedItems) {
		if (itemBase->itemType() == ModelPart::Wire) continue;

		if (isForeign) {
			itemBase = findItem(itemBase->id());
			if (itemBase == NULL) continue;

			// only foreign items need move/transform; the current view carries its own viewgeometry
			itemBase->saveGeometry();
		}

		bool isJumper = (itemBase->itemType() == ModelPart::Jumper);

		foreach (ConnectorItem * fromConnectorItem, itemBase->cachedConnectorItems()) {
			QList<ConnectorItem *> connectorItems;
			foreach (ConnectorItem * ci, fromConnectorItem->connectedToItems()) connectorItems << ci;
			ConnectorItem * crossConnectorItem = fromConnectorItem->getCrossLayerConnectorItem();
			if (crossConnectorItem) {
				foreach (ConnectorItem * ci, crossConnectorItem->connectedToItems()) connectorItems << ci;
			}
		
			foreach (ConnectorItem * toConnectorItem, connectorItems) {
				Wire * wire = qobject_cast<Wire *>(toConnectorItem->attachedTo());
				if (wire == NULL) continue;

				if (isJumper || (wire->isTraceType(getTraceFlag()))) {
					QList<Wire *> wires;
					QList<ConnectorItem *> ends;
					wire->collectChained(wires, ends);
					foreach (Wire * w, wires) {
						if (!deletedIDs.contains(w->id())) {
							otherDeletedItems.insert(w, this);
							deletedIDs.append(w->id());
						}
					}
				}
			}
		}
	}
}

ChangeConnectionCommand * SketchWidget::extendChangeConnectionCommand(BaseCommand::CrossViewType crossView,
												 long fromID, const QString & fromConnectorID,
												 long toID, const QString & toConnectorID,
												 ViewLayer::ViewLayerSpec viewLayerSpec,
												 bool connect, QUndoCommand * parentCommand)
{
	ItemBase * fromItem = findItem(fromID);
	if (fromItem == NULL) {
		return NULL;  // for now
	}

	ItemBase * toItem = findItem(toID);
	if (toItem == NULL) {
		return NULL;		// for now
	}

	ConnectorItem * fromConnectorItem = findConnectorItem(fromItem, fromConnectorID, viewLayerSpec);
	if (fromConnectorItem == NULL) return NULL; // for now

	ConnectorItem * toConnectorItem = findConnectorItem(toItem, toConnectorID, viewLayerSpec);
	if (toConnectorItem == NULL) return NULL; // for now

	return extendChangeConnectionCommand(crossView, fromConnectorItem, toConnectorItem, viewLayerSpec, connect, parentCommand);
}

ChangeConnectionCommand * SketchWidget::extendChangeConnectionCommand(BaseCommand::CrossViewType crossView,
												 ConnectorItem * fromConnectorItem, ConnectorItem * toConnectorItem,
												 ViewLayer::ViewLayerSpec viewLayerSpec,
												 bool connect, QUndoCommand * parentCommand)
{
	// cases:
	//		delete
	//		paste
	//		drop (wire)
	//		drop (part)
	//		move (part)
	//		move (wire)
	//		drag wire end
	//		drag out new wire

	ItemBase * fromItem = fromConnectorItem->attachedTo();
	if (fromItem == NULL) {
		return  NULL;  // for now
	}

	ItemBase * toItem = toConnectorItem->attachedTo();
	if (toItem == NULL) {
		return NULL;		// for now
	}

	return new ChangeConnectionCommand(this, crossView,
								fromItem->id(), fromConnectorItem->connectorSharedID(),
								toItem->id(), toConnectorItem->connectorSharedID(),
								viewLayerSpec, connect, parentCommand);
}


long SketchWidget::createWire(ConnectorItem * from, ConnectorItem * to, 
							  ViewGeometry::WireFlags wireFlags, bool dontUpdate,
							  BaseCommand::CrossViewType crossViewType, QUndoCommand * parentCommand)
{
	if (from == NULL || to == NULL) {
		return -1;
	}

	long newID = ItemBase::getNextID();
	ViewGeometry viewGeometry;
	QPointF fromPos = from->sceneAdjustedTerminalPoint(NULL);
	viewGeometry.setLoc(fromPos);
	QPointF toPos = to->sceneAdjustedTerminalPoint(NULL);
	QLineF line(0, 0, toPos.x() - fromPos.x(), toPos.y() - fromPos.y());
	viewGeometry.setLine(line);
	viewGeometry.setWireFlags(wireFlags);

	DebugDialog::debug(QString("creating wire %11: %1, flags: %6, from %7 %8, to %9 %10, frompos: %2 %3, topos: %4 %5")
		.arg(newID)
		.arg(fromPos.x()).arg(fromPos.y())
		.arg(toPos.x()).arg(toPos.y())
		.arg(wireFlags)
		.arg(from->attachedToTitle()).arg(from->connectorSharedID())
		.arg(to->attachedToTitle()).arg(to->connectorSharedID())
		.arg(m_viewIdentifier)
		);

	ViewLayer::ViewLayerSpec viewLayerSpec = createWireViewLayerSpec(from, to);

	new AddItemCommand(this, crossViewType, ModuleIDNames::WireModuleIDName, viewLayerSpec, viewGeometry, newID, false, -1, parentCommand);
	new CheckStickyCommand(this, crossViewType, newID, false, CheckStickyCommand::RemoveOnly, parentCommand);
	ChangeConnectionCommand * ccc = new ChangeConnectionCommand(this, crossViewType, from->attachedToID(), from->connectorSharedID(),
						newID, "connector0", 
						viewLayerSpec,							// ViewLayer::specFromID(from->attachedToViewLayerID())
						true, parentCommand);
	ccc->setUpdateConnections(!dontUpdate);
	ccc = new ChangeConnectionCommand(this, crossViewType, to->attachedToID(), to->connectorSharedID(),
						newID, "connector1", 
						viewLayerSpec,							// ViewLayer::specFromID(to->attachedToViewLayerID())
						true, parentCommand);
	ccc->setUpdateConnections(!dontUpdate);

	return newID;
}

ViewLayer::ViewLayerSpec SketchWidget::createWireViewLayerSpec(ConnectorItem * from, ConnectorItem * to) {
	Q_UNUSED(to);
	return from->attachedToViewLayerSpec();
}

void SketchWidget::moveItem(long id, ViewGeometry & viewGeometry, bool updateRatsnest) {
	ItemBase * pitem = findItem(id);
	if (pitem != NULL) {
		if (updateRatsnest) {
			ratsnestConnect(pitem, true);
		}
		pitem->moveItem(viewGeometry);
	}
}

void SketchWidget::simpleMoveItem(long id, QPointF p) {
	ItemBase * pitem = findItem(id);
	if (pitem != NULL) {
		pitem->setItemPos(p);
	}
}

void SketchWidget::moveItem(long id, const QPointF & p, bool updateRatsnest) {
	ItemBase * pitem = findItem(id);
	if (pitem != NULL) {
		pitem->setPos(p);
		if (updateRatsnest) {
			ratsnestConnect(pitem, true);
		}
	}
}

void SketchWidget::updateWire(long id, const QString & connectorID, bool updateRatsnest) {
	ItemBase * pitem = findItem(id);
	if (pitem == NULL) return;

	Wire * wire = qobject_cast<Wire *>(pitem);
	if (wire == NULL) return;

	ConnectorItem * connectorItem = findConnectorItem(wire, connectorID, ViewLayer::specFromID(wire->viewLayerID()));
	if (connectorItem == NULL) return;

	if (updateRatsnest) {
		ratsnestConnect(connectorItem, true);
	}

	wire->simpleConnectedMoved(connectorItem);
}

void SketchWidget::rotateItem(long id, double degrees) {
	//DebugDialog::debug(QString("rotating %1 %2").arg(id).arg(degrees) );

	if (!isVisible()) return;

	ItemBase * pitem = findItem(id);
	if (pitem != NULL) {
		pitem->rotateItem(degrees);
	}

}
void SketchWidget::transformItem(long id, const QMatrix & matrix) {
	ItemBase * pitem = findItem(id);
	if (pitem != NULL) {
		pitem->transformItem2(matrix);
	}
}

void SketchWidget::flipItem(long id, Qt::Orientations orientation) {
	DebugDialog::debug(QString("fliping %1 %2").arg(id).arg(orientation) );

	if (!isVisible()) return;

	ItemBase * pitem = findItem(id);
	if (pitem != NULL) {
		pitem->flipItem(orientation);
		ratsnestConnect(pitem, true);
	}
}

void SketchWidget::changeWire(long fromID, QLineF line, QPointF pos, bool updateConnections, bool updateRatsnest)
{
	/*
	DebugDialog::debug(QString("change wire %1; %2,%3,%4,%5; %6,%7; %8")
			.arg(fromID)
			.arg(line.x1())
			.arg(line.y1())
			.arg(line.x2())
			.arg(line.y2())
			.arg(pos.x())
			.arg(pos.y())
			.arg(updateConnections) );
	*/

	ItemBase * fromItem = findItem(fromID);
	if (fromItem == NULL) return;

	Wire* wire = dynamic_cast<Wire *>(fromItem);
	if (wire == NULL) return;

	wire->setLineAnd(line, pos, true);
	if (updateConnections) {
		wire->updateConnections(wire->connector0());
		wire->updateConnections(wire->connector1());
	}

	if (updateRatsnest) {
		ratsnestConnect(wire->connector0(), true);
		ratsnestConnect(wire->connector1(), true);
	}
}

void SketchWidget::rotateLeg(long fromID, const QString & fromConnectorID, const QPolygonF & leg, bool active)
{
	ItemBase * fromItem = findItem(fromID);
	if (fromItem == NULL) {
		DebugDialog::debug("rotate leg exit 1");
		return;
	}

	ConnectorItem * fromConnectorItem = findConnectorItem(fromItem, fromConnectorID, ViewLayer::specFromID(fromItem->viewLayerID()));
	if (fromConnectorItem == NULL) {
		DebugDialog::debug("charotatenge leg exit 2");
		return;
	}

	fromConnectorItem->rotateLeg(leg, active);
}


void SketchWidget::changeLeg(long fromID, const QString & fromConnectorID, const QPolygonF & leg, bool relative, const QString & why)
{
	changeLegAux(fromID, fromConnectorID, leg, false, relative, true, why);
}

void SketchWidget::recalcLeg(long fromID, const QString & fromConnectorID, const QPolygonF & leg, bool relative, bool active, const QString & why)
{
	changeLegAux(fromID, fromConnectorID, leg, true, relative, active, why);
}

void SketchWidget::changeLegAux(long fromID, const QString & fromConnectorID, const QPolygonF & leg, bool reset, bool relative, bool active, const QString & why)
{
	ItemBase * fromItem = findItem(fromID);
	if (fromItem == NULL) {
		DebugDialog::debug("change leg exit 1");
		return;
	}

	ConnectorItem * fromConnectorItem = findConnectorItem(fromItem, fromConnectorID, ViewLayer::specFromID(fromItem->viewLayerID()));
	if (fromConnectorItem == NULL) {
		DebugDialog::debug("change leg exit 2");
		return;
	}

	if (reset) {
		fromConnectorItem->resetLeg(leg, relative, active, why);
	}
	else {
		fromConnectorItem->setLeg(leg, relative, why);
	}

	fromItem->updateConnections(fromConnectorItem);
}

void SketchWidget::selectItem(long id, bool state, bool updateInfoView, bool doEmit) {
	this->clearHoldingSelectItem();
	ItemBase * item = findItem(id);
	if (item != NULL) {
		item->setSelected(state);
		if(updateInfoView) {
			// show something in the info view, even if it's not selected
			InfoGraphicsView::viewItemInfo(item);
		}
		if (doEmit) {
			emit itemSelectedSignal(id, state);
		}
	}

	PaletteItem *pitem = dynamic_cast<PaletteItem*>(item);
	if(pitem) {
		setLastPaletteItemSelected(pitem);
	}
}

void SketchWidget::selectDeselectAllCommand(bool state) {
	this->clearHoldingSelectItem();

	SelectItemCommand * cmd = stackSelectionState(false, NULL);
	cmd->setText(state ? tr("Select All") : tr("Deselect"));
	cmd->setSelectItemType( state ? SelectItemCommand::SelectAll : SelectItemCommand::DeselectAll );

	m_undoStack->push(cmd);

}

void SketchWidget::selectAllItems(bool state, bool doEmit) {
	foreach (QGraphicsItem * item, this->scene()->items()) {
		item->setSelected(state);
	}

	if (doEmit) {
		emit selectAllItemsSignal(state, false);
	}
}

void SketchWidget::cut() {
	copy();
	cutDeleteAux("Cut");
}

void SketchWidget::copy() {
	QList<ItemBase *> bases;

	// sort them in z-order so the copies also appear in the same order
	sortSelectedByZ(bases);
	copyAux(bases, true);
}

void SketchWidget::copyAux(QList<ItemBase *> & bases, bool saveBoundingRects)
{
	for (int i = bases.count() - 1; i >= 0; i--) {
		Wire * wire = qobject_cast<Wire *>(bases.at(i));
		if (wire == NULL) continue;
		if (!wire->getTrace()) continue;

		QList<ConnectorItem *> ends;
		QList<Wire *> wires;
		wire->collectChained(wires, ends);
		if (ends.count() < 2) {
			// trace is dangling
			bases.removeAt(i);
			continue;
		}

		foreach (ConnectorItem * end, ends) {
			if (bases.contains(end->attachedTo())) continue;
			if (bases.contains(end->attachedTo()->layerKinChief())) continue;

			// trace is dangling
			bases.removeAt(i);
			break;
		}
	}

	if (bases.count() == 0) {
		return;
	}

    QByteArray itemData;
	QList<long> modelIndexes;
	copyHeart(bases, saveBoundingRects, itemData, modelIndexes);

	// only preserve connections for copied items that connect to each other
	QByteArray newItemData = removeOutsideConnections(itemData, modelIndexes);

    QMimeData *mimeData = new QMimeData;
    mimeData->setData("application/x-dnditemsdata", newItemData);
	mimeData->setData("text/plain", newItemData);

	QClipboard *clipboard = QApplication::clipboard();
	if (clipboard == NULL) {
		// shouldn't happen
		delete mimeData;
		return;
	}

	clipboard->setMimeData(mimeData, QClipboard::Clipboard);
}

void SketchWidget::pasteHeart(QByteArray & itemData, bool seekOutsideConnections) {
	QList<ModelPart *> modelParts;
	QHash<QString, QRectF> boundingRects;
	if (m_sketchModel->paste(m_paletteModel, itemData, modelParts, boundingRects, true)) {
		QRectF r;
		QRectF boundingRect = boundingRects.value(this->viewName(), r);
		QList<long> newIDs;
		this->loadFromModelParts(modelParts, BaseCommand::SingleView, NULL, true, &r, seekOutsideConnections, newIDs);
	}
}

void SketchWidget::copyHeart(QList<ItemBase *> & bases, bool saveBoundingRects, QByteArray & itemData, QList<long> & modelIndexes) {
	QXmlStreamWriter streamWriter(&itemData);

	streamWriter.writeStartElement("module");
	streamWriter.writeAttribute("fritzingVersion", Version::versionString());

	if (saveBoundingRects) {
		QRectF itemsBoundingRect;
		foreach (ItemBase * itemBase, bases) {
			if (itemBase->getRatsnest()) continue;

			itemsBoundingRect |= itemBase->sceneBoundingRect();
		}

		QHash<QString, QRectF> boundingRects;
		boundingRects.insert(m_viewName, itemsBoundingRect);
		emit copyBoundingRectsSignal(boundingRects);

		streamWriter.writeStartElement("boundingRects");
		foreach (QString key, boundingRects.keys()) {
			streamWriter.writeStartElement("boundingRect");
			streamWriter.writeAttribute("name", key);
			QRectF r = boundingRects.value(key);
			streamWriter.writeAttribute("rect", QString("%1 %2 %3 %4")
						.arg(r.left())
						.arg(r.top())
						.arg(r.width())
						.arg(r.height()));
			streamWriter.writeEndElement();
		}
		streamWriter.writeEndElement();
	}

	streamWriter.writeStartElement("instances");
	foreach (ItemBase * base, bases) {
		if (base->getRatsnest()) continue;

		base->modelPart()->saveInstances("", streamWriter, false);
		modelIndexes.append(base->modelPart()->modelIndex());
	}
	streamWriter.writeEndElement();
	streamWriter.writeEndElement();
}

QByteArray SketchWidget::removeOutsideConnections(const QByteArray & itemData, QList<long> & modelIndexes) {
	// now have to remove each connection that points to a part outside of the set of parts being copied

	QDomDocument domDocument;
	QString errorStr;
	int errorLine;
	int errorColumn;
	bool result = domDocument.setContent(itemData, &errorStr, &errorLine, &errorColumn);
	if (!result) return ___emptyByteArray___;

	QDomElement root = domDocument.documentElement();
   	if (root.isNull()) {
   		return ___emptyByteArray___;
	}

	QDomElement instances = root.firstChildElement("instances");
	if (instances.isNull()) return ___emptyByteArray___;

	QDomElement instance = instances.firstChildElement("instance");
	while (!instance.isNull()) {
		QDomElement views = instance.firstChildElement("views");
		if (!views.isNull()) {
			QDomElement view = views.firstChildElement();
			while (!view.isNull()) {
				QDomElement connectors = view.firstChildElement("connectors");
				if (!connectors.isNull()) {
					QDomElement connector = connectors.firstChildElement("connector");
					while (!connector.isNull()) {
						QDomElement connects = connector.firstChildElement("connects");
						if (!connects.isNull()) {
							QDomElement connect = connects.firstChildElement("connect");
							QList<QDomElement> toDelete;
							while (!connect.isNull()) {
								long modelIndex = connect.attribute("modelIndex").toLong();
								if (!modelIndexes.contains(modelIndex)) {
									toDelete.append(connect);
								}

								connect = connect.nextSiblingElement("connect");
							}

							foreach (QDomElement connect, toDelete) {
								QDomNode removed = connects.removeChild(connect);
								if (removed.isNull()) {
									DebugDialog::debug("removed is null");
								}
							}
						}
						connector = connector.nextSiblingElement("connector");
					}
				}

				view = view.nextSiblingElement();
			}
		}

		instance = instance.nextSiblingElement("instance");
	}

	return domDocument.toByteArray();
}


void SketchWidget::dragEnterEvent(QDragEnterEvent *event)
{
	if (dragEnterEventAux(event)) {
		setupAutoscroll(false);
		event->acceptProposedAction();
	}
	else if (event->mimeData()->hasFormat("application/x-dndsketchdata")) {
		if (event->source() != this) {
			m_movingItem = NULL;
			SketchWidget * other = dynamic_cast<SketchWidget *>(event->source());
			if (other == NULL) {
				throw "drag enter event from unknown source";
			}

			// TODO: this checkunder will probably never work
			m_checkUnder = other->m_checkUnder;

			m_movingItem = new QGraphicsSvgItem();
			m_movingItem->setSharedRenderer(other->m_movingSVGRenderer);
			this->scene()->addItem(m_movingItem);
			m_movingItem->setPos(mapToScene(event->pos()) - other->m_movingSVGOffset);
		}
		event->acceptProposedAction();
	}
	else {
		// subclass seems to call acceptProposedAction so don't invoke it
		// QGraphicsView::dragEnterEvent(event);
		event->ignore();
	}
}

bool SketchWidget::dragEnterEventAux(QDragEnterEvent *event) {
	if (!event->mimeData()->hasFormat("application/x-dnditemdata")) return false;

	scene()->setSceneRect(scene()->sceneRect());	// prevents inadvertent scrolling when dragging in items from the parts bin
	m_clearSceneRect = true;

	m_droppingWire = false;
    QByteArray itemData = event->mimeData()->data("application/x-dnditemdata");
    QDataStream dataStream(&itemData, QIODevice::ReadOnly);

    QString moduleID;
    QPointF offset;
    dataStream >> moduleID >> offset;

	ModelPart * modelPart = m_paletteModel->retrieveModelPart(moduleID);
	if (modelPart ==  NULL) return false;

	if (!canDropModelPart(modelPart)) return false;

	m_droppingWire = (modelPart->itemType() == ModelPart::Wire);
	m_droppingOffset = offset;

	if (ItemDrag::cache().contains(this)) {
		m_droppingItem->setVisible(true);
	} 
	else {
		ViewGeometry viewGeometry;
		QPointF p = QPointF(this->mapToScene(event->pos())) - offset;
		viewGeometry.setLoc(p);

		long fromID = ItemBase::getNextID();

		bool doConnectors = true;

		/* 
		// seems to be fast enough now that we don't need this case statement
		// don't need connectors for breadboard
		// TODO: how to specify which parts don't need connectors during drag and drop from palette?
		switch (modelPart->itemType()) {
			case ModelPart::Breadboard:
			case ModelPart::Board:
			case ModelPart::ResizableBoard:
			case ModelPart::Logo:
			case ModelPart::Ruler:
			case ModelPart::Symbol:
			case ModelPart::Jumper:
			case ModelPart::CopperFill:
			case ModelPart::Via:
			case ModelPart::Unknown:
				doConnectors = true;
				break;
			default:
				doConnectors = true;
				break;
		}
		*/

		// create temporary item for dragging
		m_droppingItem = addItemAuxTemp(modelPart, defaultViewLayerSpec(), viewGeometry, fromID, NULL, doConnectors, m_viewIdentifier, true);

		QHash<long, ItemBase *> savedItems;
		QHash<Wire *, ConnectorItem *> savedWires;
		findAlignmentAnchor(m_droppingItem, savedItems, savedWires);

		ItemDrag::cache().insert(this, m_droppingItem);
		//m_droppingItem->setCacheMode(QGraphicsItem::ItemCoordinateCache);
		connect(ItemDrag::singleton(), SIGNAL(dragIsDoneSignal(ItemDrag *)), this, SLOT(dragIsDoneSlot(ItemDrag *)));
	}
	//ItemDrag::_setPixmapVisible(false);

	m_checkUnder.clear();
	if (checkUnder()) {
		m_checkUnder.append(m_droppingItem);
	}


// make sure relevant layer is visible
	ViewLayer::ViewLayerID viewLayerID;
	if (m_droppingWire) {
		viewLayerID = getWireViewLayerID(m_droppingItem->getViewGeometry(), m_droppingItem->viewLayerSpec());
	}
	else if(modelPart->moduleID().compare(ModuleIDNames::RulerModuleIDName)) {
		viewLayerID = getRulerViewLayerID();
	}
	else if(modelPart->moduleID().compare(ModuleIDNames::NoteModuleIDName)) {
		viewLayerID = getNoteViewLayerID();
	}
	else {
		viewLayerID = getPartViewLayerID();
	}

	ensureLayerVisible(viewLayerID);  // TODO: if any layer in the dragged part is visible, then don't bother calling ensureLayerVisible
	return true;
}

bool SketchWidget::canDropModelPart(ModelPart * modelPart) {
	Q_UNUSED(modelPart);
	return true;
}

void SketchWidget::dragLeaveEvent(QDragLeaveEvent * event) {
	Q_UNUSED(event);
	turnOffAutoscroll();

	if (m_droppingItem != NULL) {
		if (m_clearSceneRect) {
			m_clearSceneRect = false;
			scene()->setSceneRect(QRectF());
		}
		m_droppingItem->setVisible(false);
		//ItemDrag::_setPixmapVisible(true);
	}
	else {
		// dragging sketch items
		if (m_movingItem) {
			delete m_movingItem;
			m_movingItem = NULL;
		}
	}

	//QGraphicsView::dragLeaveEvent(event);		// we override QGraphicsView::dragEnterEvent so don't call the subclass dragLeaveEvent here
}

void SketchWidget::dragMoveEvent(QDragMoveEvent *event)
{
    if (event->mimeData()->hasFormat("application/x-dnditemdata")) {
    	dragMoveHighlightConnector(event->pos());
        event->acceptProposedAction();
        return;
    }

	if (event->mimeData()->hasFormat("application/x-dndsketchdata")) {
		if (event->source() == this) {
			m_globalPos = this->mapToGlobal(event->pos());
			if ((QApplication::keyboardModifiers() & Qt::ShiftModifier) != 0) {
				QPointF p = GraphicsUtils::calcConstraint(m_mousePressGlobalPos, m_globalPos);
				m_globalPos.setX(p.x());
				m_globalPos.setY(p.y());
			}

			moveItems(m_globalPos, true, m_rubberBandLegWasEnabled);
			m_moveEventCount++;
		}
		else {
			SketchWidget * other = dynamic_cast<SketchWidget *>(event->source());
			if (other == NULL) {
				throw "drag move event from unknown source";
			}
			m_movingItem->setPos(mapToScene(event->pos()) - other->m_movingSVGOffset);
		}
		event->acceptProposedAction();
		return;
	}

	//QGraphicsView::dragMoveEvent(event);   // we override QGraphicsView::dragEnterEvent so don't call the subclass dragMoveEvent here
}

void SketchWidget::dragMoveHighlightConnector(QPoint eventPos) {
	if (m_droppingItem == NULL) return;

	m_globalPos = this->mapToGlobal(eventPos);
	checkAutoscroll(m_globalPos);

	QPointF loc = this->mapToScene(eventPos) - m_droppingOffset;
	if (m_alignToGrid && (m_alignmentItem != NULL)) {
		QPointF l =  m_alignmentItem->getViewGeometry().loc();
		alignLoc(loc, m_alignmentStartPoint, loc, l);

		//QPointF q = m_alignmentItem->pos();
		//if (q != l) {
		//	DebugDialog::debug(QString("m alignment %1 %2, %3 %4").arg(q.x()).arg(q.y()).arg(l.x()).arg(l.y()));
		//}
	}

	m_droppingItem->setItemPos(loc);
	if (m_checkUnder.contains(m_droppingItem)) {
		m_droppingItem->findConnectorsUnder();
	}

}

void SketchWidget::dropEvent(QDropEvent *event)
{
	m_alignmentItem = NULL;

	turnOffAutoscroll();
	clearHoldingSelectItem();

    if (event->mimeData()->hasFormat("application/x-dnditemdata")) {
		dropItemEvent(event);
    }
	else if (event->mimeData()->hasFormat("application/x-dndsketchdata")) {
		if (m_movingItem) {
			delete m_movingItem;
			m_movingItem = NULL;
		}

		ConnectorItem::clearEqualPotentialDisplay();
		if (event->source() == this) {
			checkMoved();
			m_savedItems.clear();
			m_savedWires.clear();
		}
		else {
			SketchWidget * other = dynamic_cast<SketchWidget *>(event->source());
			if (other == NULL) {
				throw "drag and drop from unknown source";
			}

			ItemBase * ref = other->m_moveReferenceItem;
			QPointF originalPos = ref->getViewGeometry().loc();
			other->copyDrop();
			QPointF startLocal = other->mapFromGlobal(QPoint(other->m_mousePressGlobalPos.x(), other->m_mousePressGlobalPos.y()));
			QPointF sceneLocal = other->mapToScene(startLocal.x(), startLocal.y());
			QPointF offset = sceneLocal - originalPos;
			m_pasteOffset = this->mapToScene(event->pos()) - offset - originalPos;

			DebugDialog::debug(QString("other %1 %2, event %3 %4")
				.arg(startLocal.x()).arg(startLocal.y())
				.arg(event->pos().x()).arg(event->pos().y())
			);
			m_pasteCount = 0;
			emit dropPasteSignal(this);
		}
        event->acceptProposedAction();
	}
	else {
		QGraphicsView::dropEvent(event);
	}

	DebugDialog::debug("after drop event");

}

void SketchWidget::dropItemEvent(QDropEvent *event) {
	if (m_droppingItem == NULL) return;

	if (m_clearSceneRect) {
		m_clearSceneRect = false;
		scene()->setSceneRect(QRectF());
	}

	ModelPart * modelPart = m_droppingItem->modelPart();
	if (modelPart == NULL) return;
	if (modelPart->modelPartShared() == NULL) return;

	QUndoCommand* parentCommand = new QUndoCommand(tr("Add %1").arg(m_droppingItem->title()));
	stackSelectionState(false, parentCommand);
	CleanUpWiresCommand * cuw = new CleanUpWiresCommand(this, CleanUpWiresCommand::Noop, parentCommand);

	m_droppingItem->saveGeometry();
	ViewGeometry viewGeometry = m_droppingItem->getViewGeometry();

	long fromID = m_droppingItem->id();

	BaseCommand::CrossViewType crossViewType = BaseCommand::CrossView;
	switch (modelPart->itemType()) {
		case ModelPart::Ruler:
		case ModelPart::Logo:
		case ModelPart::Note:
			// rulers and logos are local to a particular view
			crossViewType = BaseCommand::SingleView;
			break;
		default:
			break;				
	}
	AddItemCommand * addItemCommand = newAddItemCommand(crossViewType, modelPart->moduleID(), defaultViewLayerSpec(), viewGeometry, fromID, true, -1, parentCommand);
	addItemCommand->setDropOrigin(this);

	new SetDropOffsetCommand(this, fromID, m_droppingOffset, parentCommand);
	
	new CheckStickyCommand(this, crossViewType, fromID, false, CheckStickyCommand::RemoveOnly, parentCommand);

	SelectItemCommand * selectItemCommand = new SelectItemCommand(this, SelectItemCommand::NormalSelect, parentCommand);
	selectItemCommand->addRedo(fromID);

	new ShowLabelFirstTimeCommand(this, crossViewType, fromID, true, true, parentCommand);

	if (modelPart->itemType() == ModelPart::Wire && !m_lastColorSelected.isEmpty()) {
		new WireColorChangeCommand(this, fromID, m_lastColorSelected, m_lastColorSelected, 1.0, 1.0, parentCommand);
	}

	bool gotConnector = false;

	// jrc: 24 aug 2010: don't see why restoring color on dropped item is necessary
	//QList<ConnectorItem *> connectorItems;
	foreach (ConnectorItem * connectorItem, m_droppingItem->cachedConnectorItems()) {
		//connectorItem->setMarked(false);
		//connectorItems.append(connectorItem);
		ConnectorItem * to = connectorItem->overConnectorItem();
		if (to != NULL) {
			to->connectorHover(to->attachedTo(), false);
			connectorItem->setOverConnectorItem(NULL);   // clean up
			extendChangeConnectionCommand(BaseCommand::CrossView, connectorItem, to, ViewLayer::specFromID(connectorItem->attachedToViewLayerID()), true, parentCommand);
			gotConnector = true;
		}
		//connectorItem->clearConnectorHover();
	}
	//foreach (ConnectorItem * connectorItem, connectorItems) {
		//if (!connectorItem->marked()) {
			//connectorItem->restoreColor(false, 0, true);
		//}
	//}
	//m_droppingItem->clearConnectorHover();

	clearTemporaries();

	killDroppingItem();

	if (gotConnector) {
		new CleanUpWiresCommand(this, CleanUpWiresCommand::RedoOnly, parentCommand);
		cuw->setDirection(CleanUpWiresCommand::UndoOnly);
	}
    m_undoStack->waitPush(parentCommand, 10);


    event->acceptProposedAction();

	emit dropSignal(event->pos(), modelPart);
	emit dropTempSignal(modelPart, ItemDrag::originator());
	emit warnSMDSignal(modelPart->moduleID());
}

SelectItemCommand* SketchWidget::stackSelectionState(bool pushIt, QUndoCommand * parentCommand) {

	// if pushIt assumes m_undoStack->beginMacro has previously been called

	//DebugDialog::debug(QString("stacking"));

	// DebugDialog::debug(QString("stack selection state %1 %2").arg(pushIt).arg((long) parentCommand));
	SelectItemCommand* selectItemCommand = new SelectItemCommand(this, SelectItemCommand::NormalSelect, parentCommand);
	const QList<QGraphicsItem *> sitems = scene()->selectedItems();
 	for (int i = 0; i < sitems.size(); ++i) {
 		ItemBase * base = ItemBase::extractTopLevelItemBase(sitems.at(i));
 		if (base == NULL) continue;

 		 selectItemCommand->addUndo(base->id());
		 //DebugDialog::debug(QString("\tstacking %1").arg(base->id()));
    }

	selectItemCommand->setText(tr("Selection"));

    if (pushIt) {
     	m_undoStack->push(selectItemCommand);
    }

    return selectItemCommand;
}

bool SketchWidget::moveByArrow(int dx, int dy, QKeyEvent * event) {
	bool rubberBandLegEnabled = false;
	DebugDialog::debug(QString("move by arrow %1").arg(event->isAutoRepeat()));
	if (!event->isAutoRepeat()) {
		m_dragBendpointWire = NULL;
		clearHoldingSelectItem();
		m_savedItems.clear();
		m_savedWires.clear();
		m_moveEventCount = 0;
		m_arrowTotalX = m_arrowTotalY = 0;

		QPoint cp = QCursor::pos();
		QPoint wp = this->mapFromGlobal(cp);
		QPointF sp = this->mapToScene(wp);
		Wire * wire = dynamic_cast<Wire *>(scene()->itemAt(sp));
		bool draggingWire = false;
		if (wire != NULL) {
			if (canChainWire(wire) && wire->hasConnections()) {
				if (canDragWire(wire) && ((event->modifiers() & altOrMetaModifier()) != 0)) {
					prepDragWire(wire);
					draggingWire = true;
				}
			}
		}

		if (!draggingWire) {
			rubberBandLegEnabled = (event->modifiers() & altOrMetaModifier()) != 0;
			prepMove(NULL, rubberBandLegEnabled);
		}
		if (m_savedItems.count() == 0) return false;

		m_mousePressScenePos = this->mapToScene(this->rect().center());
		m_movingByArrow = true;
	}
	else {
		//DebugDialog::debug("autorepeat");
	}

	if (event->modifiers() & Qt::ShiftModifier) {
		dx *= 10;
		dy *= 10;
	}

	if (m_alignToGrid) {
		dx *= gridSizeInches() * FSvgRenderer::printerScale();
		dy *= gridSizeInches() * FSvgRenderer::printerScale();
	}

	m_arrowTotalX += dx;
	m_arrowTotalY += dy;

	QPoint globalPos = mapFromScene(m_mousePressScenePos + QPoint(m_arrowTotalX, m_arrowTotalY));
	globalPos = mapToGlobal(globalPos);
	moveItems(globalPos, false, rubberBandLegEnabled);
	m_moveEventCount++;
	return true;
}


void SketchWidget::mousePressEvent(QMouseEvent *event) 
{
	m_draggingBendpoint = false;
	if (m_movingByArrow) return;

	m_movingByMouse = true;

	QMouseEvent * hackEvent = NULL;
	if (event->button() == Qt::MidButton && !spaceBarIsPressed()) {
		m_middleMouseIsPressed = true;
		setDragMode(QGraphicsView::ScrollHandDrag);
		setCursor(Qt::OpenHandCursor);
		// make the event look like a left button press to fool the underlying drag mode implementation
		event = hackEvent = new QMouseEvent(event->type(), event->pos(), event->globalPos(), Qt::LeftButton, event->buttons() | Qt::LeftButton, event->modifiers());
	}

	m_dragBendpointWire = NULL;
	m_spaceBarWasPressed = spaceBarIsPressed();
	if (m_spaceBarWasPressed) {
		InfoGraphicsView::mousePressEvent(event);
		if (hackEvent) delete hackEvent;
		return;
	}

	//setRenderHint(QPainter::Antialiasing, false);

	clearHoldingSelectItem();
	m_savedItems.clear();
	m_savedWires.clear();
	m_moveEventCount = 0;
	m_holdingSelectItemCommand = stackSelectionState(false, NULL);
	m_mousePressScenePos = mapToScene(event->pos());
	m_mousePressGlobalPos = event->globalPos();

	QList<QGraphicsItem *> items = this->items(event->pos());
	QGraphicsItem* wasItem = NULL;
	foreach (QGraphicsItem * gitem, items) {
		if (gitem->acceptedMouseButtons() != Qt::NoButton) {
			wasItem = gitem;
			break;
		}
	}

	m_anyInRotation = false;
	// mouse event gets passed through to individual QGraphicsItems
	QGraphicsView::mousePressEvent(event);
	if (m_anyInRotation) {
		m_anyInRotation = false;
		return;
	}

	items = this->items(event->pos());
	QGraphicsItem* item = NULL;
	foreach (QGraphicsItem * gitem, items) {
		if (gitem->acceptedMouseButtons() != Qt::NoButton) {
			item = gitem;
			break;
		}
	}

	if (item != wasItem) {
		// if the item was deleted during mousePressEvent
		// for example, by shift-clicking a connectorItem
		return;
	}

	if (item == NULL) {
		if (items.length() == 1) {
			// if we unambiguously click on a partlabel whose owner is unselected, go ahead and activate it
			PartLabel * partLabel =  dynamic_cast<PartLabel *>(items[0]);
			if (partLabel != NULL) {
				partLabel->owner()->setSelected(true);
				return;
			}
		}

		clickBackground(event);
		if (m_infoView != NULL) {
			m_infoView->viewItemInfo(this, NULL, true);
		}
		return;
	}

	PartLabel * partLabel =  dynamic_cast<PartLabel *>(item);
	if (partLabel != NULL) {
		InfoGraphicsView::viewItemInfo(partLabel->owner());
		setLastPaletteItemSelectedIf(partLabel->owner());
		return;
	}

	// Note's child items (at the moment) are the resize grip and the text editor
	Note * note = dynamic_cast<Note *>(item->parentItem());
	if (note != NULL)  {
		return;
	}

	Stripbit * stripbit = dynamic_cast<Stripbit *>(item);
	if (stripbit) return;

	ItemBase * itemBase = dynamic_cast<ItemBase *>(item);
	if (itemBase) {
		InfoGraphicsView::viewItemInfo(itemBase);
		setLastPaletteItemSelectedIf(itemBase);
	}

	if (resizingBoardPress(item)) {
		return;
	}

	ConnectorItem * connectorItem = dynamic_cast<ConnectorItem *>(wasItem);
	if (connectorItem != NULL && connectorItem->isDraggingLeg()) {
		return;
	}

	Wire * wire = dynamic_cast<Wire *>(item);
	if ((event->button() == Qt::LeftButton) && (wire != NULL)) {
		if (canChainWire(wire) && wire->hasConnections()) {
			if (canDragWire(wire) && ((event->modifiers() & altOrMetaModifier()) != 0)) {
				prepDragWire(wire);
				return;
			}
			else {
				m_dragCurve = curvyWiresIndicated(event->modifiers()) && wire->canHaveCurve();
				m_dragBendpointWire = wire;
				m_dragBendpointPos = event->pos();
				return;	
			}
		}
	}

	if (resizingJumperItemPress(item)) {
		return;
	}

	prepMove(itemBase ? itemBase : dynamic_cast<ItemBase *>(item->parentItem()), (event->modifiers() & altOrMetaModifier()) != 0);

	if (m_alignToGrid && (itemBase == NULL) && (event->modifiers() == Qt::NoModifier)) {
		Wire * wire = dynamic_cast<Wire *>(item->parentItem());
		if (wire != NULL && wire->draggingEnd()) {
			ConnectorItem * connectorItem = dynamic_cast<ConnectorItem *>(item);
			if (connectorItem != NULL) {
				m_draggingBendpoint = (connectorItem->connectionsCount() > 0);
				this->m_alignmentStartPoint = mapToScene(event->pos()) - connectorItem->sceneAdjustedTerminalPoint(NULL);
			}
		}
	}

	m_moveReferenceItem = m_savedItems.count() > 0 ? m_savedItems.values().at(0) : NULL;

	setupAutoscroll(true);


}

void SketchWidget::prepMove(ItemBase * originatingItem, bool rubberBandLegEnabled) {
	m_rubberBandLegWasEnabled = rubberBandLegEnabled;
	m_checkUnder.clear();
	//DebugDialog::debug("prep move check under = false");
	QSet<Wire *> wires;
	QList<ItemBase *> items;
	foreach (QGraphicsItem * gitem,  this->scene()->selectedItems()) {
		ItemBase *itemBase = dynamic_cast<ItemBase *>(gitem);
		if (itemBase == NULL) continue;
		if (itemBase->moveLock()) continue;

		items.append(itemBase);
	}


	//DebugDialog::debug(QString("prep move items %1").arg(items.count()));

	int originalItemsCount = items.count();

	for (int i = 0; i < items.count(); i++) {
		ItemBase * itemBase = items[i];
		if (itemBase->itemType() == ModelPart::Wire) {
			if (itemBase->isVisible()) {
				wires.insert(qobject_cast<Wire *>(itemBase));
			}
			continue;
		}

		ItemBase * chief = itemBase->layerKinChief();
		if (chief->moveLock()) continue;

		m_savedItems.insert(chief->id(), chief);
		if (chief->sticky()) {
			foreach(ItemBase * sitemBase, chief->stickyList()) {
				if (sitemBase->isVisible()) {
					if (sitemBase->itemType() == ModelPart::Wire) {
						wires.insert(qobject_cast<Wire *>(sitemBase));
					}
					else {
						m_savedItems.insert(sitemBase->layerKinChief()->id(), sitemBase);
						if (!items.contains(sitemBase)) {
							items.append(sitemBase);
						}
					}
				}
			}
		}

		QSet<ItemBase *> set;
		if (collectFemaleConnectees(chief, set)) {  // for historical reasons returns true if chief has male connectors
			if (i < originalItemsCount) {

				m_checkUnder.append(chief);
			}
		}
		foreach (ItemBase * sitemBase, set) {
			if (!items.contains(sitemBase)) {
				items.append(sitemBase);
			}
		}
		chief->collectWireConnectees(wires);
	}

	for (int i = m_checkUnder.count() - 1; i >= 0; i--) {
		ItemBase * itemBase = m_checkUnder.at(i);
		foreach (ConnectorItem * connectorItem, itemBase->cachedConnectorItems()) {
			bool gotOne = false;
			foreach (ConnectorItem * toConnectorItem, connectorItem->connectedToItems()) {
				if (m_savedItems.value(toConnectorItem->attachedToID(), NULL) != NULL) {
					m_checkUnder.removeAt(i);
					gotOne = true;
					break;
				}
			}
			if (gotOne) break;
		}
	}

	if (wires.count() > 0) {
		QList<ItemBase *> freeWires;
		categorizeDragWires(wires, freeWires);
		m_checkUnder.append(freeWires);
	}

	categorizeDragLegs(rubberBandLegEnabled);

	foreach (ItemBase * itemBase, m_savedItems.values()) {
		itemBase->saveGeometry();
	}

	foreach (Wire * w, m_savedWires.keys()) {
		w->saveGeometry();
	}

	findAlignmentAnchor(originatingItem, m_savedItems, m_savedWires);


}

void SketchWidget::alignLoc(QPointF & loc, const QPointF startPoint, const QPointF newLoc, const QPointF originalLoc) 
{
	// in the standard case, startpoint is the center of the connectorItem, newLoc is the current mouse position, 
	// originalLoc is the original mouse position.  newpos is therefore the new position of the center of the connectorItem
	// and ny and ny make up the nearest grid point.  nx, ny - newloc give just the offset from the grid, which is then
	// applied to loc, which is the location of the item being dragged

	QPointF newPos = startPoint + newLoc - originalLoc;
	double ny = GraphicsUtils::getNearestOrdinate(newPos.y(), gridSizeInches() * FSvgRenderer::printerScale());
	double nx = GraphicsUtils::getNearestOrdinate(newPos.x(), gridSizeInches() * FSvgRenderer::printerScale());
	loc.setX(loc.x() + nx - newPos.x());
	loc.setY(loc.y() + ny - newPos.y());
}


void SketchWidget::findAlignmentAnchor(ItemBase * originatingItem, 	QHash<long, ItemBase *> & savedItems, QHash<Wire *, ConnectorItem *> & savedWires) 
{
	m_alignmentItem = NULL;
	if (!m_alignToGrid) return;

	if (originatingItem) {
		foreach (ConnectorItem * connectorItem, originatingItem->cachedConnectorItems()) {
				m_alignmentStartPoint = connectorItem->sceneAdjustedTerminalPoint(NULL);
				m_alignmentItem = originatingItem;
				return;
		}
		if (canAlignToTopLeft(originatingItem)) {
			m_alignmentStartPoint = originatingItem->pos();
			m_alignmentItem = originatingItem;
			return;
		}
	}

	foreach (ItemBase * itemBase, savedItems) {
		foreach (ConnectorItem * connectorItem, itemBase->cachedConnectorItems()) {
				m_alignmentStartPoint = connectorItem->sceneAdjustedTerminalPoint(NULL);
				m_alignmentItem = itemBase;
				return;
		}
	}

	foreach (Wire * w, savedWires.keys()) {
		m_alignmentItem = w;
		m_alignmentStartPoint = w->connector0()->sceneAdjustedTerminalPoint(NULL);
		return;
	}

	foreach (ItemBase * itemBase, savedItems) {
		if (canAlignToTopLeft(itemBase)) {
			m_alignmentStartPoint = itemBase->pos();
			m_alignmentItem = itemBase;
			return;
		}
	}
}

struct ConnectionThing {
	Wire * wire;
	ConnectionStatus status[2];
};


void SketchWidget::categorizeDragLegs(bool rubberBandLegEnabled) 
{
	m_stretchingLegs.clear();
	if (!rubberBandLegEnabled) return;

	QSet<ItemBase *> passives;
	foreach (ItemBase * itemBase, m_savedItems.values()) {
		if (itemBase->itemType() == ModelPart::Wire) continue;
		if (!itemBase->hasRubberBandLeg()) continue;

		// 1. we are dragging a part with rubberBand legs which are attached to some part not being dragged along (i.e. a breadboard)
		//		so we stretch those attached legs
		// 2. a part has rubberBand legs attached to multiple parts, and we are only dragging some of the parts

		foreach (ConnectorItem * connectorItem, itemBase->cachedConnectorItems()) {
			if (!connectorItem->hasRubberBandLeg()) continue;
			if (connectorItem->connectionsCount() == 0) continue;

			bool treatAsNormal = true;
			foreach (ConnectorItem * toConnectorItem, connectorItem->connectedToItems()) {
				if (toConnectorItem->attachedToItemType() == ModelPart::Wire) continue;
				if (m_savedItems.value(toConnectorItem->attachedTo()->layerKinChief()->id(), NULL)) continue;
					
				treatAsNormal = false;
				break;
			}
			if (treatAsNormal) continue;

			if (itemBase->isSelected()) {
				// itemBase is being dragged, but the connector doesn't come along
				connectorItem->prepareToStretch(true);
				m_stretchingLegs.insert(itemBase, connectorItem);
				continue;
			}

			// itemBase has connectors stuck into multiple parts, not all of which are being dragged
			// but we're in a loop of savedItems, so we have to treat it later
			passives.insert(itemBase);		
		}
	}

	foreach (ItemBase * itemBase, passives) {
		// we're not actually dragging the itemBase
		// one of its connectors is coming along for the ride
		m_savedItems.remove(itemBase->id());
		foreach (ConnectorItem * connectorItem, itemBase->cachedConnectorItems()) {
			if (!connectorItem->hasRubberBandLeg()) continue;
			if (connectorItem->connectionsCount() == 0) continue;

			foreach (ConnectorItem * toConnectorItem, connectorItem->connectedToItems()) {
				if (toConnectorItem->attachedToItemType() == ModelPart::Wire) continue;
				ItemBase * chief = toConnectorItem->attachedTo()->layerKinChief();
				if (m_savedItems.value(chief->id(), NULL) == NULL) {
					// connected to another part
					// treat it as passive as well
				}

				// the connector is passively dragged along with the part it is connected to
				// but the part it is attached to stays put
				connectorItem->prepareToStretch(false);
				m_stretchingLegs.insert(chief, connectorItem);
				break;
			}
		}
	}
}

void SketchWidget::categorizeDragWires(QSet<Wire *> & wires, QList<ItemBase *> & freeWires) 
{
	foreach (Wire * w, wires) {
		QList<Wire *> chainedWires;
		QList<ConnectorItem *> ends;
		w->collectChained(chainedWires, ends);
		foreach (Wire * ww, chainedWires) {
			wires.insert(ww);
		}
	}

	QList<ConnectionThing *> connectionThings;
	QHash<ItemBase *, ConnectionThing *> ctHash;
	foreach (Wire * w, wires) {
		ConnectionThing * ct = new ConnectionThing;
		ct->wire = w;
		ct->status[0] = ct->status[1] = UNDETERMINED_;
		connectionThings.append(ct);
		ctHash.insert(w, ct);
	}

	// handle free first
	foreach (Wire * w, wires) {
		if (w->getTrace()) continue;

		QList<ConnectorItem *> pairs;
		pairs << w->connector0() << w->connector1();
		for (int i = 0; i < pairs.count(); i++) {
			ConnectorItem * ci = pairs.at(i);
			if (ci->connectionsCount() == 0) {
				ConnectionThing * ct = ctHash.value(ci->attachedTo());
				if (ct->status[i] != UNDETERMINED_) continue;

				// if one end is FREE, treat all connected wires as free (except possibly if the other end connector is attached to something)
				QList<Wire *> chainedWires;
				QList<ConnectorItem *> ends;
				ct->wire->collectChained(chainedWires, ends);
				ends.clear();
				foreach (Wire * ww, chainedWires) {
					ends.append(ww->connector0());
					ends.append(ww->connector1());
				}
				foreach (ConnectorItem * end, ends) {
					if (end->connectionsCount() == 0) {
						ctHash.value(end->attachedTo())->status[i] = FREE_;
					}
					else {
						bool onlyWires = true;
						foreach (ConnectorItem * to, end->connectedToItems()) {
							if (to->attachedToItemType() != ModelPart::Wire) {
								onlyWires = false;
								break;
							}
						}
						if (onlyWires) {
							ctHash.value(end->attachedTo())->status[i] = FREE_;
						}
					}
				}
			}
		}
	}

	int noChangeCount = 0;
	QList<ItemBase *> outWires;
	while (connectionThings.count() > 0) {
		ConnectionThing * ct = connectionThings.takeFirst();
		bool changed = false;

		QList<ConnectorItem *> from;
		from.append(ct->wire->connector0());
		from.append(ct->wire->connector1());
		for (int i = 0; i < 2; i++) {
			if (ct->status[i] != UNDETERMINED_) continue;

			foreach (ConnectorItem * toConnectorItem, from.at(i)->connectedToItems()) {
				if (m_savedItems.keys().contains(toConnectorItem->attachedTo()->layerKinChief()->id())) {
					changed = true;
					ct->status[i] = IN_;
					break;
				}

				bool notWire = toConnectorItem->attachedToItemType() != ModelPart::Wire;

				if (notWire || outWires.contains(toConnectorItem->attachedTo())) {
					changed = true;
					ct->status[i] = OUT_;
					break;
				}
			}
			if (ct->status[i] != UNDETERMINED_) continue;

			ItemBase * stickingTo = ct->wire->stickingTo();
			if (stickingTo != NULL) {
				QPointF p = from.at(i)->sceneAdjustedTerminalPoint(NULL);
				if (stickingTo->contains(stickingTo->mapFromScene(p))) {
					if (m_savedItems.keys().contains(stickingTo->layerKinChief()->id())) {
						ct->status[i] = IN_;
						changed = true;
					}
				}
			}
			if (ct->status[i] != UNDETERMINED_) continue;

			// it's not connected and not stuck

			if (ct->wire->getTrace() && from.at(i)->connectionsCount() == 0) {
				// this is a bug.  traces should be connected at both ends. Pretend that the unconnected end is connected to something
				DebugDialog::debug(QString("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n"
											"Trace %1 connector %2 is unconnected\n"
											"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!")
								.arg(ct->wire->id()).arg(i));
				QPointF p = from.at(i)->sceneAdjustedTerminalPoint(NULL);
				foreach (QGraphicsItem * item,  scene()->items(p)) {
					ItemBase * itemBase = dynamic_cast<ItemBase *>(item);
					if (itemBase == NULL) continue;

					ct->status[i] = m_savedItems.keys().contains(itemBase->layerKinChief()->id()) ? IN_ : OUT_;
					changed = true;
					break;
				}
			}
			if (ct->status[i] != UNDETERMINED_) continue;

			if (from.at(i)->connectionsCount() == 0) {
				DebugDialog::debug("FREE_ wire should not be found at this point");
			}
		}

		if (ct->status[0] != UNDETERMINED_ && ct->status[1] != UNDETERMINED_) {
			if (ct->status[0] == IN_) {
				if (ct->status[1] == IN_ || ct->status[1] == FREE_) {
					m_savedItems.insert(ct->wire->id(), ct->wire);
					if (ct->status[1] == FREE_) freeWires.append(ct->wire);
				}
				else {
					// attach the connector that stays IN
					m_savedWires.insert(ct->wire, ct->wire->connector0());
				}
			}
			else if (ct->status[0] == OUT_) {
				if (ct->status[1] == IN_) {
					// attach the connector that stays in
					m_savedWires.insert(ct->wire, ct->wire->connector1());
				}
				else {
					// don't drag this; both ends are connected OUT
					outWires.append(ct->wire);
				}
			}
			else /* ct->status[0] == FREE_ */ {  
				if (ct->status[1] == IN_) {
					// attach wire that stays IN
					m_savedItems.insert(ct->wire->id(), ct->wire);
					freeWires.append(ct->wire);
				}
				else if (ct->status[1] == FREE_) {
					// both sides are free, so if the wire is selected, drag it
					if (ct->wire->isSelected()) {
						m_savedItems.insert(ct->wire->id(), ct->wire);
						freeWires.append(ct->wire);
					}
					else {
						outWires.append(ct->wire);
					}
				}
				else {
					// don't drag this; both ends are connected OUT
					outWires.append(ct->wire);
				}
			}
			delete ct;
			noChangeCount = 0;
		}
		else {
			connectionThings.append(ct);
			if (changed) {
				noChangeCount = 0;
			}
			else {
				if (++noChangeCount > connectionThings.count()) {
					QList<ConnectionThing *> cts;
					foreach (ConnectionThing * ct, connectionThings) {
						// if one end is OUT and the other end is unaccounted for at this pass, then both ends are OUT
						if ((ct->status[0] == FREE_ || ct->status[0] == OUT_) || 
							(ct->status[1] == FREE_ || ct->status[1] == OUT_)) 
						{
							noChangeCount = 0;
							outWires.append(ct->wire);
							delete ct;
						}
						else {
							cts.append(ct);
						}
					}
					if (noChangeCount == 0) {
						// get ready for another pass, we got rid of some 
						connectionThings.clear();
						foreach (ConnectionThing * ct, cts) {
							connectionThings.append(ct);
						}
					}
					else {
						// we've elimated all OUT items so mark everybody IN
						foreach (ConnectionThing * ct, connectionThings) {
							m_savedItems.insert(ct->wire->id(), ct->wire);
							delete ct;
						}						
						connectionThings.clear();
					}
				}
			}
		}
	}
}

void SketchWidget::clickBackground(QMouseEvent * event) 
{
	// in here if you clicked on the sketch itself,

	if (m_fixedToCenterItem && m_fixedToCenterItem->getVisible()) {
		QRectF r(m_fixedToCenterItemOffset, m_fixedToCenterItem->size());
		if (r.contains(event->pos())) {
			QMouseEvent newEvent(event->type(), event->pos() - m_fixedToCenterItemOffset,
				event->globalPos(), event->button(), event->buttons(), event->modifiers());
			if (m_fixedToCenterItem->forwardMousePressEvent(&newEvent)) {
				// update background
				setBackground(background());
				this->update();
				emit firstTimeHelpHidden();
			}
		}
	}
}

void SketchWidget::prepDragWire(Wire * wire) 
{
	bool drag = true;
	foreach (ConnectorItem * toConnectorItem, wire->connector0()->connectedToItems()) {
		if (toConnectorItem->attachedToItemType() == ModelPart::Wire) {
			m_savedWires.insert(qobject_cast<Wire *>(toConnectorItem->attachedTo()), toConnectorItem);
		}
		else {
			drag = false;
			break;
		}
	}
	if (drag) {
		foreach (ConnectorItem * toConnectorItem, wire->connector1()->connectedToItems()) {
			if (toConnectorItem->attachedToItemType() == ModelPart::Wire) {
				m_savedWires.insert(qobject_cast<Wire *>(toConnectorItem->attachedTo()), toConnectorItem);
			}
			else {
				drag = false;
				break;
			}
		}
	}
	if (!drag) {
		m_savedWires.clear();
		return;
	}

	m_savedItems.clear();
	m_savedItems.insert(wire->id(), wire);
	wire->saveGeometry();
	foreach (Wire * w, m_savedWires.keys()) {
		w->saveGeometry();
	}
	setupAutoscroll(true);
}

void SketchWidget::prepDragBendpoint(Wire * wire, QPoint eventPos, bool dragCurve) 
{
	m_bendpointWire = wire;
	wire->saveGeometry();
	ViewGeometry vg = m_bendpointVG = wire->getViewGeometry();
	QPointF newPos = mapToScene(eventPos); 

	if (dragCurve) {
		setupAutoscroll(true);
		wire->initDragCurve(newPos);
		wire->grabMouse();
		return;
	}

	QPointF oldPos = wire->pos();
	QLineF oldLine = wire->line();
	Bezier left, right;
	bool curved = wire->initNewBendpoint(newPos, left, right);
	//DebugDialog::debug(QString("oldpos"), oldPos);
	//DebugDialog::debug(QString("oldline p1"), oldLine.p1());
	//DebugDialog::debug(QString("oldline p2"), oldLine.p2());
	QLineF newLine(oldLine.p1(), newPos - oldPos);
	wire->setLine(newLine);
	if (curved) wire->changeCurve(&left);
	vg.setLoc(newPos);
	QLineF newLine2(QPointF(0,0), oldLine.p2() + oldPos - newPos);
	vg.setLine(newLine2);
	ConnectorItem * oldConnector1 = wire->connector1();
	m_connectorDragWire = this->createTempWireForDragging(wire, wire->modelPart(), oldConnector1, vg, wire->viewLayerSpec());
	if (curved) {
		right.translateToZero();
		m_connectorDragWire->changeCurve(&right);
	}
	ConnectorItem * newConnector1 = m_connectorDragWire->connector1();
	foreach (ConnectorItem * toConnectorItem, oldConnector1->connectedToItems()) {
		oldConnector1->tempRemove(toConnectorItem, false);
		toConnectorItem->tempRemove(oldConnector1, false);
		newConnector1->tempConnectTo(toConnectorItem, false);
		toConnectorItem->tempConnectTo(newConnector1, false);
	}
	oldConnector1->tempConnectTo(m_connectorDragWire->connector0(), false);
	m_connectorDragWire->connector0()->tempConnectTo(oldConnector1, false);
	m_connectorDragConnector = oldConnector1;

	setupAutoscroll(true);

	m_connectorDragWire->initDragEnd(m_connectorDragWire->connector0(), newPos);
	m_connectorDragWire->grabMouse();
}

bool SketchWidget::collectFemaleConnectees(ItemBase * itemBase, QSet<ItemBase *> & items) {
	Q_UNUSED(itemBase);
	Q_UNUSED(items);
	return false;
}

bool SketchWidget::checkUnder() {
	return false;
};

bool SketchWidget::draggingWireEnd() {
	if (m_connectorDragWire != NULL) return true;

	QGraphicsItem * mouseGrabberItem = scene()->mouseGrabberItem();
	Wire * wire = dynamic_cast<Wire *>(mouseGrabberItem);
	if (wire == NULL) {
		ConnectorItem * connectorItem = dynamic_cast<ConnectorItem *>(mouseGrabberItem);
		if (connectorItem == NULL) return false;
		if (connectorItem->attachedToItemType() != ModelPart::Wire) return false;

		wire = qobject_cast<Wire *>(connectorItem->attachedTo());
	}

	return wire->draggingEnd();
}

void SketchWidget::mouseMoveEvent(QMouseEvent *event) {
	// if its just dragging a wire end do default
	// otherwise handle all move action here

	if (m_movingByArrow) return;

	QPointF sp = mapToScene(event->pos());
	emit cursorLocationSignal(sp.x() / FSvgRenderer::printerScale(), sp.y() / FSvgRenderer::printerScale());

	if (m_dragBendpointWire != NULL) {
		Wire * tempWire = m_dragBendpointWire;
		m_dragBendpointWire = NULL;
		prepDragBendpoint(tempWire, m_dragBendpointPos, m_dragCurve);
		m_draggingBendpoint = true;
		this->m_alignmentStartPoint = mapToScene(m_dragBendpointPos);		// not sure this will be correct...
		return;
	}

	if (m_spaceBarWasPressed) {
		InfoGraphicsView::mouseMoveEvent(event);
		return;
	}

	if (m_savedItems.count() > 0) {
		if ((event->buttons() & Qt::LeftButton) && !draggingWireEnd()) {
			m_globalPos = event->globalPos();
			if ((m_globalPos - m_mousePressGlobalPos).manhattanLength() >= QApplication::startDragDistance()) {
				QMimeData *mimeData = new QMimeData;
				mimeData->setData("application/x-dndsketchdata", NULL);

				QDrag * drag = new QDrag(this);
				drag->setMimeData(mimeData);
				//QBitmap bitmap = *CursorMaster::MoveCursor->bitmap();
				//drag->setDragCursor(bitmap, Qt::MoveAction);

				QPointF offset;
				QString svg = makeMoveSVG(FSvgRenderer::printerScale(),  GraphicsUtils::StandardFritzingDPI, offset);
				m_movingSVGRenderer = new QSvgRenderer(new QXmlStreamReader(svg));
				m_movingSVGOffset = m_mousePressScenePos - offset;

				m_moveEventCount = 0;					// reset m_moveEventCount to make sure that equal potential highlights are cleared
				m_movingByMouse = false;

				drag->exec();

                delete m_movingSVGRenderer;
				m_movingSVGRenderer = NULL;
				return;
			}
		}
	}

	if (event->buttons() == Qt::NoButton) {
		if (m_fixedToCenterItem && m_fixedToCenterItem->getVisible()) {
			QSize size((int) m_fixedToCenterItem->size().width(), (int) m_fixedToCenterItem->size().height());
			QRect r(m_fixedToCenterItemOffset, size);
			bool within = r.contains(event->pos()) && (itemAt(event->pos()) == NULL);
			if (m_fixedToCenterItem->setMouseWithin(within)) {
				// seems to be the only way to force a redraw of the background here
				setBackground(background());
			}
		}
	}

	m_moveEventCount++;
	if (m_alignToGrid && m_draggingBendpoint) {
		QPointF sp = mapToScene(event->pos());
		
		alignLoc(sp, sp, QPointF(0,0), QPointF(0,0));  
		QPointF p = mapFromScene(sp);
		QPoint pp(qRound(p.x()), qRound(p.y()));
		QPointF q = mapToGlobal(pp);
		QMouseEvent alignedEvent(event->type(), pp, QPoint(q.x(), q.y()), event->button(), event->buttons(), event->modifiers());
		
		DebugDialog::debug(QString("sketch move event %1,%2").arg(sp.x()).arg(sp.y()));
		QGraphicsView::mouseMoveEvent(&alignedEvent);
		return;
	}

	if (draggingWireEnd()) {
		checkAutoscroll(event->globalPos());
	}

		
	QGraphicsView::mouseMoveEvent(event);
}

QString SketchWidget::makeMoveSVG(double printerScale, double dpi, QPointF & offset) 
{

	QRectF itemsBoundingRect;
	foreach (ItemBase * itemBase, m_savedItems.values()) {
		itemsBoundingRect |= itemBase->sceneBoundingRect();
	}

	double width = itemsBoundingRect.width();
	double height = itemsBoundingRect.height();
	offset = itemsBoundingRect.topLeft();

	QString outputSVG = TextUtils::makeSVGHeader(printerScale, dpi, width, height);

	foreach (ItemBase * itemBase, m_savedItems.values()) {
		Wire * wire = qobject_cast<Wire *>(itemBase);
		if (wire != NULL) {
			outputSVG.append(makeWireSVG(wire, offset, dpi, printerScale, true));
		}
		else {
			outputSVG.append(TextUtils::makeRectSVG(itemBase->sceneBoundingRect(), offset, dpi, printerScale));
		}
	}
	//outputSVG.append(makeRectSVG(itemsBoundingRect, offset, dpi, printerScale));

	outputSVG += "</svg>";

	return outputSVG;



	/*
	// this is too slow:
	LayerList viewLayerIDs;
	foreach (ViewLayer * viewLayer, viewLayers()) {
		if (viewLayer == NULL) continue;

		viewLayerIDs << viewLayer->viewLayerID();
	}

	QSizeF imageSize;
	return renderToSVG(printerScale, viewLayerIDs, true, imageSize, NULL, dpi, false, itemBases, itemsBoundingRect);

	*/
}



void SketchWidget::moveItems(QPoint globalPos, bool checkAutoScrollFlag, bool rubberBandLegEnabled)
{
	if (checkAutoScrollFlag) {
		bool result = checkAutoscroll(globalPos);
		if (!result) return;
	}

	QPoint q = mapFromGlobal(globalPos);
	QPointF scenePos = mapToScene(q);

	if (m_alignToGrid && (m_alignmentItem != NULL)) {
		QPointF currentParentPos = m_alignmentItem->mapToParent(m_alignmentItem->mapFromScene(scenePos));
		QPointF buttonDownParentPos = m_alignmentItem->mapToParent(m_alignmentItem->mapFromScene(m_mousePressScenePos));
		alignLoc(scenePos, m_alignmentStartPoint, currentParentPos, buttonDownParentPos);
	}

/*
	DebugDialog::debug(QString("scroll 1 sx:%1 sy:%2 sbx:%3 sby:%4 qx:%5 qy:%6")
		.arg(scenePos.x()).arg(scenePos.y())
		.arg(m_mousePressScenePos.x()).arg(m_mousePressScenePos.y())
		.arg(q.x()).arg(q.y())
		);
*/

	if (m_moveEventCount == 0) {
		// first time
		m_moveDisconnectedFromFemale.clear();
		foreach (ItemBase * item, m_savedItems) {
			if (item->itemType() == ModelPart::Wire) continue;

			//DebugDialog::debug(QString("disconnecting from female %1").arg(item->instanceTitle()));
			disconnectFromFemale(item, m_savedItems, m_moveDisconnectedFromFemale, false, rubberBandLegEnabled, NULL);
		}
	}

	foreach (ItemBase * itemBase, m_savedItems) {
		QPointF currentParentPos = itemBase->mapToParent(itemBase->mapFromScene(scenePos));
		QPointF buttonDownParentPos = itemBase->mapToParent(itemBase->mapFromScene(m_mousePressScenePos));
		itemBase->setPos(itemBase->getViewGeometry().loc() + currentParentPos - buttonDownParentPos);
		foreach (ConnectorItem * connectorItem, m_stretchingLegs.values(itemBase)) {
			connectorItem->stretchBy(currentParentPos - buttonDownParentPos);
		}

		if (m_checkUnder.contains(itemBase)) {
			findConnectorsUnder(itemBase);
		}

/*
		DebugDialog::debug(QString("scroll 2 lx:%1 ly:%2 cpx:%3 cpy:%4 qx:%5 qy:%6 px:%7 py:%8")
		.arg(item->getViewGeometry().loc().x()).arg(item->getViewGeometry().loc().y())
		.arg(currentParentPos.x()).arg(currentParentPos.y())
		.arg(buttonDownParentPos.x()).arg(buttonDownParentPos.y())
		.arg(item->pos().x()).arg(item->pos().y())
		);
*/

	}

	foreach (Wire * wire, m_savedWires.keys()) {
		wire->simpleConnectedMoved(m_savedWires.value(wire));
	}

	//DebugDialog::debug(QString("done move items %1").arg(QTime::currentTime().msec()) );

}


void SketchWidget::findConnectorsUnder(ItemBase * item) {
	Q_UNUSED(item);
}

void SketchWidget::mouseReleaseEvent(QMouseEvent *event) {
	//setRenderHint(QPainter::Antialiasing, true);

	m_draggingBendpoint = false;
	if (m_movingByArrow) return;

	m_alignmentItem = NULL;
	m_movingByMouse = false;

	m_dragBendpointWire = NULL;

	ConnectorItem::clearEqualPotentialDisplay();

	if (m_spaceBarWasPressed) {

		QMouseEvent * hackEvent = NULL;
		if (m_middleMouseIsPressed) {
		// make the event look like a left button press to fool the underlying drag mode implementation
			event = hackEvent = new QMouseEvent(event->type(), event->pos(), event->globalPos(), Qt::LeftButton, event->buttons() | Qt::LeftButton, event->modifiers());
		}

		InfoGraphicsView::mouseReleaseEvent(event);
		m_spaceBarWasPressed = false;
		if (m_middleMouseIsPressed) {
			m_middleMouseIsPressed = false;
			setDragMode(QGraphicsView::RubberBandDrag);
			setCursor(Qt::ArrowCursor);		
		}

		//DebugDialog::debug("turning off spacebar was");
		if (hackEvent) delete hackEvent;
		return;
	}

	if (resizingBoardRelease()) {
		InfoGraphicsView::mouseReleaseEvent(event);
		return;
	}

	if (resizingJumperItemRelease()) {
		InfoGraphicsView::mouseReleaseEvent(event);
		return;
	}

	turnOffAutoscroll();
	QGraphicsView::mouseReleaseEvent(event);

	if (m_connectorDragWire != NULL) {
		if (scene()->mouseGrabberItem() == m_connectorDragWire) {
			// probably already ungrabbed by the wire, but just in case
			m_connectorDragWire->ungrabMouse();
		}

		// remove again (may not have been removed earlier)
		if (m_connectorDragWire->scene() != NULL) {
			this->scene()->removeItem(m_connectorDragWire);
			//m_infoView->unregisterCurrentItem();
			updateInfoView();

		}

		if (m_bendpointWire) {
			// click on wire but no drag:  restore original state of wire
			foreach (ConnectorItem * toConnectorItem, m_connectorDragWire->connector1()->connectedToItems()) {
				m_connectorDragWire->connector1()->tempRemove(toConnectorItem, false);
				toConnectorItem->tempRemove(m_connectorDragWire->connector1(), false);
				m_bendpointWire->connector1()->tempConnectTo(toConnectorItem, false);
				toConnectorItem->tempConnectTo(m_bendpointWire->connector1(), false);
			}
			m_bendpointWire->connector1()->tempRemove(m_connectorDragWire->connector0(), false);
			m_connectorDragWire->connector0()->tempRemove(m_bendpointWire->connector1(), false);
			m_bendpointWire->setLine(m_bendpointVG.line());			
		}

		DebugDialog::debug("deleting connector drag wire");
		delete m_connectorDragWire;

		m_bendpointWire = m_connectorDragWire = NULL;
		m_savedItems.clear();
		m_savedWires.clear();
		m_connectorDragConnector = NULL;
		return;
	}

	if (m_moveEventCount == 0) {
		if (this->m_holdingSelectItemCommand != NULL) {
			if (m_holdingSelectItemCommand->updated()) {
				SelectItemCommand* tempCommand = m_holdingSelectItemCommand;
				m_holdingSelectItemCommand = NULL;
				DebugDialog::debug(QString("scene changed push select %1").arg(scene()->selectedItems().count()));
				m_undoStack->push(tempCommand);
			}
			else {
				clearHoldingSelectItem();
			}
		}
	}
	m_savedItems.clear();
	m_savedWires.clear();
}

bool SketchWidget::checkMoved()
{
	if (m_moveEventCount == 0) {
		return false;
	}

	int moveCount = m_savedItems.count();
	if (moveCount <= 0) {
		return false;
	}

	ItemBase * saveBase = NULL;
	foreach (ItemBase * item, m_savedItems) {
		saveBase = item;
		break;
	}

	clearHoldingSelectItem();

	QString moveString;
	QString viewName = ViewIdentifierClass::viewIdentifierName(m_viewIdentifier);

	if (moveCount == 1) {
		moveString = tr("Move %2 (%1)").arg(viewName).arg(saveBase->title());
	}
	else {
		moveString = tr("Move %2 items (%1)").arg(viewName).arg(QString::number(moveCount));
	}

	QUndoCommand * parentCommand = new QUndoCommand(moveString);

	bool hasBoard = false;

	foreach (ItemBase * item, m_savedItems) {
		rememberSticky(item, parentCommand);
	}

	CleanUpWiresCommand * cuw = new CleanUpWiresCommand(this, CleanUpWiresCommand::UndoOnly, parentCommand);
	moveLegBendpoints(true, parentCommand);

	bool gotConnection = true;

	MoveItemsCommand * moveItemsCommand = new MoveItemsCommand(this, true, parentCommand);

	foreach (ItemBase * item, m_savedItems) {
		if (item == NULL) continue;

		ViewGeometry viewGeometry(item->getViewGeometry());
		item->saveGeometry();

		moveItemsCommand->addItem(item->id(), viewGeometry.loc(), item->getViewGeometry().loc());

		if (item->itemType() == ModelPart::Breadboard) {
			hasBoard = true;
			continue;
		}

		// TODO: boardtypes and breadboard types are always sticky
		if (item->itemType() == ModelPart::Board || item->itemType() == ModelPart::ResizableBoard) {
			hasBoard = true;
		}
	}

	foreach (ItemBase * item, m_savedItems) {
		new CheckStickyCommand(this, BaseCommand::SingleView, item->id(), false, CheckStickyCommand::RedoOnly, parentCommand);
	}

	foreach (ItemBase * item, m_savedWires.keys()) {
		rememberSticky(item, parentCommand);
	}

	foreach (Wire * wire, m_savedWires.keys()) {
		if (wire == NULL) continue;

		moveItemsCommand->addWire(wire->id(), m_savedWires.value(wire)->connectorSharedID());
	}

	foreach (ItemBase * item, m_savedWires.keys()) {
		new CheckStickyCommand(this, BaseCommand::SingleView, item->id(), false, CheckStickyCommand::RedoOnly, parentCommand);
	}

	foreach (ConnectorItem * fromConnectorItem, m_moveDisconnectedFromFemale.uniqueKeys()) {
		foreach (ConnectorItem * toConnectorItem, m_moveDisconnectedFromFemale.values(fromConnectorItem)) {
			extendChangeConnectionCommand(BaseCommand::CrossView, fromConnectorItem, toConnectorItem, ViewLayer::specFromID(fromConnectorItem->attachedToViewLayerID()), false, parentCommand);
			gotConnection = true;
		}
	}

	QList<ConnectorItem *> restoreConnectorItems;
	foreach (ItemBase * item, m_savedItems) {
		foreach (ConnectorItem * fromConnectorItem, item->cachedConnectorItems()) {
			if (item->itemType() == ModelPart::Wire) {
				if (fromConnectorItem->connectionsCount() > 0) {
					continue;
				}
			}

			ConnectorItem * toConnectorItem = fromConnectorItem->overConnectorItem();
			if (toConnectorItem != NULL) {
				toConnectorItem->connectorHover(item, false);
				fromConnectorItem->setOverConnectorItem(NULL);   // clean up
				gotConnection = true;
				extendChangeConnectionCommand(BaseCommand::CrossView, fromConnectorItem, toConnectorItem, 
					ViewLayer::specFromID(toConnectorItem->attachedToViewLayerID()),
					true, parentCommand);
			}
			restoreConnectorItems.append(fromConnectorItem);
			fromConnectorItem->clearConnectorHover();
		}

		item->clearConnectorHover();
	}

	foreach (ConnectorItem * connectorItem, restoreConnectorItems) {
		if (!connectorItem->marked()) {
			connectorItem->restoreColor(false, 0, true);
		}
	}

	// must restore legs after connections are restored (redo direction)
	moveLegBendpoints(false, parentCommand);

	clearTemporaries();

	if (gotConnection) {
		new CleanUpWiresCommand(this, CleanUpWiresCommand::RedoOnly, parentCommand);
		cuw->setDirection(CleanUpWiresCommand::UndoOnly);
	}
	m_undoStack->push(parentCommand);

	return true;
}

void SketchWidget::setPaletteModel(PaletteModel * paletteModel) {
	m_paletteModel = paletteModel;
}

void SketchWidget::setRefModel(ReferenceModel *refModel) {
	m_refModel = refModel;
}

void SketchWidget::setSketchModel(SketchModel * sketchModel) {
	m_sketchModel = sketchModel;
}

void SketchWidget::itemAddedSlot(ModelPart * modelPart, ViewLayer::ViewLayerSpec viewLayerSpec, const ViewGeometry & viewGeometry, long id, SketchWidget * dropOrigin) {
	if (dropOrigin != NULL && dropOrigin != this) {
		placePartDroppedInOtherView(modelPart, viewLayerSpec, viewGeometry, id, dropOrigin);
	}
	else {
		addItemAux(modelPart, viewLayerSpec, viewGeometry, id, NULL, true, m_viewIdentifier, false);
	}
}

ItemBase * SketchWidget::placePartDroppedInOtherView(ModelPart * modelPart, ViewLayer::ViewLayerSpec viewLayerSpec, const ViewGeometry & viewGeometry, long id, SketchWidget * dropOrigin) 
{
	// offset the part 
	QPointF from = dropOrigin->mapToScene(QPoint(0, 0));
	QPointF to = this->mapToScene(QPoint(0, 0));
	QPointF dp = viewGeometry.loc() - from;
	ViewGeometry vg(viewGeometry);
	vg.setLoc(to + dp);
	ItemBase * itemBase = addItemAux(modelPart, viewLayerSpec, vg, id, NULL, true, m_viewIdentifier, false);
	if (m_alignToGrid && (itemBase != NULL)) {
		alignOneToGrid(itemBase);
	}

	return itemBase;
}

void SketchWidget::itemDeletedSlot(long id) {
	ItemBase * pitem = findItem(id);
	if (pitem != NULL) {
		deleteItem(pitem, false, false, false);
	}
}

void SketchWidget::selectionChangedSlot() {
	if (m_ignoreSelectionChangeEvents > 0) {
		return;
	}

	emit selectionChangedSignal();

	if (m_holdingSelectItemCommand != NULL) {
		//DebugDialog::debug("update holding command");

		int selCount = 0;
		ItemBase* saveBase = NULL;
		QString selString;
		m_holdingSelectItemCommand->clearRedo();
		const QList<QGraphicsItem *> sitems = scene()->selectedItems();
		foreach (QGraphicsItem * item, scene()->selectedItems()) {
	 		ItemBase * base = dynamic_cast<ItemBase *>(item);
	 		if (base == NULL) continue;

			saveBase = base;
	 		m_holdingSelectItemCommand->addRedo(base->layerKinChief()->id());
	 		selCount++;
	    }
		if (selCount == 1) {
			selString = tr("Select %1").arg(saveBase->title());
		}
		else {
			selString = tr("Select %1 items").arg(QString::number(selCount));
		}
		m_holdingSelectItemCommand->setText(selString);
		m_holdingSelectItemCommand->setUpdated(true);
	}
}

void SketchWidget::clearHoldingSelectItem() {
	// DebugDialog::debug("clear holding");
	if (m_holdingSelectItemCommand != NULL) {
		delete m_holdingSelectItemCommand;
		m_holdingSelectItemCommand = NULL;
	}
}

void SketchWidget::clearSelection() {
	this->scene()->clearSelection();
	emit clearSelectionSignal();
}

void SketchWidget::clearSelectionSlot() {
	this->scene()->clearSelection();
}

void SketchWidget::itemSelectedSlot(long id, bool state) {
	ItemBase * item = findItem(id);
	//DebugDialog::debug(QString("got item selected signal %1 %2 %3 %4").arg(id).arg(state).arg(item != NULL).arg(m_viewIdentifier));
	if (item != NULL) {
		item->setSelected(state);
	}

	PaletteItem *pitem = dynamic_cast<PaletteItem*>(item);
	if(pitem) {
		setLastPaletteItemSelected(pitem);
	}
}


void SketchWidget::prepLegSelection(ItemBase * itemBase) 
{
	this->clearHoldingSelectItem();
	this->m_moveEventCount = 0;  // clear this so an extra MoveItemCommand isn't posted

	if (itemBase->isSelected()) return;

	m_holdingSelectItemCommand = stackSelectionState(false, NULL);
	itemBase->setSelected(true);
}

void SketchWidget::prepLegBendpointMove(ConnectorItem * from, int index, QPointF oldPos, QPointF newPos, ConnectorItem * to, bool changeConnections)
{
	this->m_moveEventCount = 0;  // clear this so an extra MoveItemCommand isn't posted

	QUndoCommand * parentCommand = new QUndoCommand();

	if (m_holdingSelectItemCommand) {
		SelectItemCommand * selectItemCommand = new SelectItemCommand(this, SelectItemCommand::NormalSelect, parentCommand);
		selectItemCommand->copyUndo(m_holdingSelectItemCommand);
		selectItemCommand->copyRedo(m_holdingSelectItemCommand);
		clearHoldingSelectItem();
	}

	long fromID = from->attachedToID();

	QString fromConnectorID = from->connectorSharedID();

	long toID = -1;
	QString toConnectorID;
	if (changeConnections && (to != NULL)) {
		toID = to->attachedToID();
		toConnectorID = to->connectorSharedID();
	}

	if (changeConnections) {
		new CleanUpWiresCommand(this, CleanUpWiresCommand::UndoOnly, parentCommand);
	}

	MoveLegBendpointCommand * mlbc = new MoveLegBendpointCommand(this, fromID, fromConnectorID, index, oldPos, newPos, parentCommand);
	mlbc->setUndoOnly();

	if (changeConnections) {
		QList< QPointer<ConnectorItem> > former = from->connectedToItems();

		QString prefix;
		QString suffix;
		if (to == NULL) {
			if (former.count() > 0) {
				prefix = tr("Disconnect");
				suffix = tr("from %1").arg(former.at(0)->attachedToInstanceTitle());
			}
			else {
				prefix = tr("Move leg of");
			}
		}
		else {
			prefix = tr("Connect");
			suffix = tr("to %1").arg(to->attachedToInstanceTitle());
		}

		parentCommand->setText(QObject::tr("%1 %2,%3 %4")
				.arg(prefix)
				.arg(from->attachedTo()->instanceTitle())
				.arg(from->connectorSharedName())
				.arg(suffix) 
				);


		if (former.count() > 0) {
			QList<ConnectorItem *> connectorItems;
			connectorItems.append(from);
			ConnectorItem::collectEqualPotential(connectorItems, true, ViewGeometry::RatsnestFlag | ViewGeometry::PCBTraceFlag | ViewGeometry::SchematicTraceFlag);

			foreach (ConnectorItem * formerConnectorItem, former) {
				ChangeConnectionCommand * ccc = extendChangeConnectionCommand(BaseCommand::CrossView, from, formerConnectorItem, 
												ViewLayer::specFromID(from->attachedToViewLayerID()),
												false, parentCommand);
				ccc->setUpdateConnections(false);
				from->tempRemove(formerConnectorItem, false);
				formerConnectorItem->tempRemove(from, false);
			}

		}
		if (to != NULL) {
			ChangeConnectionCommand * ccc = extendChangeConnectionCommand(BaseCommand::CrossView, from, to, ViewLayer::specFromID(from->attachedToViewLayerID()), true, parentCommand);
			ccc->setUpdateConnections(false);
		}
	}
	else {
		parentCommand->setText(QObject::tr("Change leg of %1,%2")
				.arg(from->attachedTo()->instanceTitle())
				.arg(from->connectorSharedName())
			);
	}

	// change leg after connections have been restored
	mlbc = new MoveLegBendpointCommand(this, fromID, fromConnectorID, index, oldPos, newPos, parentCommand);
	mlbc->setRedoOnly();


	if (changeConnections) {
		new CleanUpWiresCommand(this, CleanUpWiresCommand::RedoOnly, parentCommand);
	}

	m_undoStack->push(parentCommand);
}

void SketchWidget::prepLegCurveChange(ConnectorItem * from, int index, const class Bezier * oldB, const class Bezier * newB, bool triggerFirstTime) 
{
	this->m_moveEventCount = 0;  // clear this so an extra MoveItemCommand isn't posted

	QUndoCommand * parentCommand = new QUndoCommand(tr("Change leg curvature for %1.").arg(from->attachedToInstanceTitle()));

	if (m_holdingSelectItemCommand) {
		SelectItemCommand * selectItemCommand = new SelectItemCommand(this, SelectItemCommand::NormalSelect, parentCommand);
		selectItemCommand->copyUndo(m_holdingSelectItemCommand);
		selectItemCommand->copyRedo(m_holdingSelectItemCommand);
		clearHoldingSelectItem();
	}

	long fromID = from->attachedToID();

	QString fromConnectorID = from->connectorSharedID();

	ChangeLegCurveCommand * clcc = new ChangeLegCurveCommand(this, fromID, fromConnectorID, index, oldB, newB, parentCommand);
	if (!triggerFirstTime) {
		clcc->setFirstTime();
	}

	m_undoStack->push(parentCommand);
}

void SketchWidget::prepLegBendpointChange(ConnectorItem * from, int oldCount, int newCount, int index, QPointF p, 
					const class Bezier * bezier0, const class Bezier * bezier1, const class Bezier * bezier2, bool triggerFirstTime)
{
	this->m_moveEventCount = 0;  // clear this so an extra MoveItemCommand isn't posted

	QUndoCommand * parentCommand = new QUndoCommand(tr("Change leg bendpoint for %1.").arg(from->attachedToInstanceTitle()));

	if (m_holdingSelectItemCommand) {
		SelectItemCommand * selectItemCommand = new SelectItemCommand(this, SelectItemCommand::NormalSelect, parentCommand);
		selectItemCommand->copyUndo(m_holdingSelectItemCommand);
		selectItemCommand->copyRedo(m_holdingSelectItemCommand);
		clearHoldingSelectItem();
	}

	long fromID = from->attachedToID();

	QString fromConnectorID = from->connectorSharedID();

	ChangeLegBendpointCommand * clbc = new ChangeLegBendpointCommand(this, fromID, fromConnectorID, oldCount, newCount, index, p, bezier0, bezier1, bezier2, parentCommand);
	if (!triggerFirstTime) {
		clbc->setFirstTime();
	}

	m_undoStack->push(parentCommand);
}

void SketchWidget::wireChangedSlot(Wire* wire, const QLineF & oldLine, const QLineF & newLine, QPointF oldPos, QPointF newPos, ConnectorItem * from, ConnectorItem * to) {
	this->clearHoldingSelectItem();
	this->m_moveEventCount = 0;  // clear this so an extra MoveItemCommand isn't posted

	// TODO: make sure all these pointers to pointers to pointers aren't null...

	if (wire == this->m_connectorDragWire) {
		dragWireChanged(wire, from, to);
		return;
	}

	clearDragWireTempCommand();
	if ((to != NULL) && from->connectedToItems().contains(to)) {
		// there's no change: the wire was dragged back to its original connection
		from->attachedTo()->updateConnections(to);
		return;
	}

	QUndoCommand * parentCommand = new QUndoCommand();

	long fromID = wire->id();

	QString fromConnectorID;
	if (from != NULL) {
		fromConnectorID = from->connectorSharedID();
	}

	long toID = -1;
	QString toConnectorID;
	if (to != NULL) {
		toID = to->attachedToID();
		toConnectorID = to->connectorSharedID();
	}

	new CleanUpWiresCommand(this, CleanUpWiresCommand::UndoOnly, parentCommand);

	rememberSticky(wire, parentCommand);


	bool chained = false;
	foreach (ConnectorItem * toConnectorItem, from->connectedToItems()) {
		Wire * toWire = qobject_cast<Wire *>(toConnectorItem->attachedTo());
		if (toWire) {
			chained = true;
			break;
		}
	}

	if (wire->getTrace() && !chained) {
		changeTrace(wire, from, to, parentCommand);
		return;
	}

	new ChangeWireCommand(this, fromID, oldLine, newLine, oldPos, newPos, true, true, parentCommand);
	new CheckStickyCommand(this, BaseCommand::SingleView, fromID, false, CheckStickyCommand::RedoOnly, parentCommand);

	foreach (ConnectorItem * toConnectorItem, from->connectedToItems()) {
		Wire * toWire = qobject_cast<Wire *>(toConnectorItem->attachedTo());
		if (toWire == NULL) continue;

		rememberSticky(toWire, parentCommand);

		ViewGeometry vg = toWire->getViewGeometry();
		QLineF nl = toWire->line();
		QPointF np = toWire->pos();
		new ChangeWireCommand(this, toWire->id(), vg.line(), nl, vg.loc(), np, true, true, parentCommand);
		new CheckStickyCommand(this, BaseCommand::SingleView, toWire->id(), false, CheckStickyCommand::RedoOnly, parentCommand);
	}

	QList< QPointer<ConnectorItem> > former = from->connectedToItems();

	QString prefix;
	QString suffix;
	if (to == NULL) {
		if (former.count() > 0  && !chained) {
			prefix = tr("Disconnect");
			// the suffix is a little tricky to determine
			// it might be multiple disconnects, or might be disconnecting a virtual wire, in which case, the
			// title needs to come from the virtual wire's other connection's attachedTo()

			// suffix = tr("from %1").arg(former->attachedToTitle());
		}
		else {
			prefix = tr("Change");
		}
	}
	else {
		prefix = tr("Connect");
		suffix = tr("to %1").arg(to->attachedToInstanceTitle());
	}

	parentCommand->setText(QObject::tr("%1 %2 %3").arg(prefix).arg(wire->title()).arg(suffix) );

	if (!chained) {
		if (former.count() > 0) {
			foreach (ConnectorItem * formerConnectorItem, former) {
				extendChangeConnectionCommand(BaseCommand::CrossView, from, formerConnectorItem, 
					ViewLayer::specFromID(wire->viewLayerID()),
					false, parentCommand);
				from->tempRemove(formerConnectorItem, false);
				formerConnectorItem->tempRemove(from, false);
			}

		}
		if (to != NULL) {
			extendChangeConnectionCommand(BaseCommand::CrossView, from, to, ViewLayer::specFromID(wire->viewLayerID()), true, parentCommand);
		}
	}

	clearTemporaries();

	new CleanUpWiresCommand(this, CleanUpWiresCommand::RedoOnly, parentCommand);
	m_undoStack->waitPush(parentCommand, PropChangeDelay);
}

void SketchWidget::dragWireChanged(Wire* wire, ConnectorItem * fromOnWire, ConnectorItem * to)
{
	if (m_bendpointWire != NULL && wire->getRatsnest()) {
		dragRatsnestChanged();
		return;
	}

	prereleaseTempWireForDragging(m_connectorDragWire);
	BaseCommand::CrossViewType crossViewType = BaseCommand::CrossView;
	if (m_bendpointWire) {
	}
	else {
		m_connectorDragConnector->tempRemove(m_connectorDragWire->connector1(), false);
		m_connectorDragWire->connector1()->tempRemove(m_connectorDragConnector, false);

		// if to and from are the same connector, you can't draw a wire to yourself 
		// or to == NULL and it's pcb or schematic view, bail out
		if ((m_connectorDragConnector == to) || !canCreateWire(wire, fromOnWire, to)) {
			clearDragWireTempCommand();
			this->scene()->removeItem(m_connectorDragWire);
			return;
		}
	}

	QUndoCommand * parentCommand = new QUndoCommand();
	parentCommand->setText(tr("Create and connect wire"));

	SelectItemCommand * selectItemCommand = new SelectItemCommand(this, SelectItemCommand::NormalSelect, parentCommand);
	if (m_tempDragWireCommand != NULL) {
		selectItemCommand->copyUndo(m_tempDragWireCommand);
		clearDragWireTempCommand();
	}

	new CleanUpWiresCommand(this, CleanUpWiresCommand::UndoOnly, parentCommand);

	m_connectorDragWire->saveGeometry();
	bool doEmit = false;
	long fromID = wire->id();

	DebugDialog::debug(QString("m_connectorDragConnector:%1 %4 from:%2 to:%3")
						.arg(m_connectorDragConnector->connectorSharedID())
						.arg(fromOnWire->connectorSharedID())
						.arg((to == NULL) ? "null" : to->connectorSharedID())
						.arg(m_connectorDragConnector->attachedTo()->title()) );


	// create a new wire with the same id as the temporary wire
	ViewGeometry vg = m_connectorDragWire->getViewGeometry();
	vg.setWireFlags(getTraceFlag());
	new AddItemCommand(this, crossViewType, m_connectorDragWire->moduleID(), m_connectorDragWire->viewLayerSpec(), vg, fromID, true, -1, parentCommand);
	new CheckStickyCommand(this, crossViewType, fromID, false, CheckStickyCommand::RemoveOnly, parentCommand);
	selectItemCommand->addRedo(fromID);

	if (m_bendpointWire == NULL) {
		ConnectorItem * anchor = wire->otherConnector(fromOnWire);
		if (anchor != NULL) {
			extendChangeConnectionCommand(BaseCommand::CrossView, anchor, m_connectorDragConnector, ViewLayer::specFromID(wire->viewLayerID()), true, parentCommand);
			doEmit = true;
		}
		if (to != NULL) {
			extendChangeConnectionCommand(BaseCommand::CrossView, fromOnWire, to, ViewLayer::specFromID(wire->viewLayerID()), true, parentCommand);
			doEmit = true;
		}

		setUpColor(m_connectorDragConnector, to, wire, parentCommand);
	}
	else {
		new WireColorChangeCommand(this, wire->id(), m_bendpointWire->colorString(), m_bendpointWire->colorString(), m_bendpointWire->opacity(), m_bendpointWire->opacity(), parentCommand);
		new WireWidthChangeCommand(this, wire->id(), m_bendpointWire->width(), m_bendpointWire->width(), parentCommand);
	}

	if (m_bendpointWire) {
		ChangeWireCurveCommand * cwcc = new ChangeWireCurveCommand(this, m_bendpointWire->id(), m_bendpointWire->undoCurve(), m_bendpointWire->curve(), parentCommand);
		cwcc->setUndoOnly();

		// puts the wire in position at redo time
		ChangeWireCommand * cwc = new ChangeWireCommand(this, m_bendpointWire->id(), m_bendpointVG.line(), m_bendpointWire->line(), m_bendpointVG.loc(), m_bendpointWire->pos(), true, false, parentCommand);		
		cwc->setRedoOnly();
		foreach (ConnectorItem * toConnectorItem, wire->connector1()->connectedToItems()) {
			toConnectorItem->tempRemove(wire->connector1(), false);
			wire->connector1()->tempRemove(toConnectorItem, false);
			new ChangeConnectionCommand(this, BaseCommand::CrossView,
										m_bendpointWire->id(), m_connectorDragConnector->connectorSharedID(),
										toConnectorItem->attachedToID(), toConnectorItem->connectorSharedID(),
										ViewLayer::specFromID(toConnectorItem->attachedToViewLayerID()),
										false, parentCommand);
			new ChangeConnectionCommand(this, BaseCommand::CrossView,
										wire->id(), wire->connector1()->connectorSharedID(),
										toConnectorItem->attachedToID(), toConnectorItem->connectorSharedID(),
										ViewLayer::specFromID(toConnectorItem->attachedToViewLayerID()),
										true, parentCommand);
		}


		m_connectorDragConnector->tempRemove(wire->connector0(), false);
		wire->connector0()->tempRemove(m_connectorDragConnector, false);
		new ChangeConnectionCommand(this, BaseCommand::CrossView,
										m_connectorDragConnector->attachedToID(), m_connectorDragConnector->connectorSharedID(),
										wire->id(), wire->connector0()->connectorSharedID(),
										ViewLayer::specFromID(wire->viewLayerID()),
										true, parentCommand);

		new ChangeWireCurveCommand(this, wire->id(), NULL, wire->curve(), parentCommand);
		cwcc = new ChangeWireCurveCommand(this, m_bendpointWire->id(), m_bendpointWire->undoCurve(), m_bendpointWire->curve(), parentCommand);
		cwcc->setRedoOnly();

		// puts the wire in position at undo time
		cwc = new ChangeWireCommand(this, m_bendpointWire->id(), m_bendpointVG.line(), m_bendpointWire->line(), m_bendpointVG.loc(), m_bendpointWire->pos(), true, false, parentCommand);		
		cwc->setUndoOnly();

		SelectItemCommand * selectItemCommand = new SelectItemCommand(this, SelectItemCommand::NormalSelect, parentCommand);
		selectItemCommand->addRedo(m_bendpointWire->id());

		m_bendpointWire = NULL;			// signal that we're done

	}

	clearTemporaries();

	// remove the temporary wire
	this->scene()->removeItem(m_connectorDragWire);

	new CleanUpWiresCommand(this, CleanUpWiresCommand::RedoOnly, parentCommand);
	m_undoStack->push(parentCommand);

}

void SketchWidget::dragRatsnestChanged()
{
	// m_bendpointWire is the original wire
	// m_connectorDragWire is temporary
	// wire == m_connectorDragWire
	// m_connectorDragConnector is from original wire

	QList<Wire *> wires;
	QList<ConnectorItem *> ends;
	m_bendpointWire->collectChained(wires, ends);
	if (ends.count() != 2) {
		// ratsnest wires should always and only have two ends: we're screwed
		return;
	}

	ViewLayer::ViewLayerSpec viewLayerSpec = createWireViewLayerSpec(ends[0], ends[1]);
	if (viewLayerSpec == ViewLayer::UnknownSpec) {
		// for now this should not be possible
		QMessageBox::critical(NULL, tr("Fritzing"), tr("This seems like an attempt to create a trace across layers. This circumstance should not arise: please contact the developers."));
		return;
	}

	BaseCommand::CrossViewType crossViewType = BaseCommand::CrossView;

	QUndoCommand * parentCommand = new QUndoCommand(); 
	parentCommand->setText(tr("Create and connect %1").arg(m_viewIdentifier == ViewIdentifierClass::BreadboardView ? tr("wire") : tr("trace")));

	new CleanUpWiresCommand(this, CleanUpWiresCommand::UndoOnly, parentCommand);

	//SelectItemCommand * selectItemCommand = new SelectItemCommand(this, SelectItemCommand::NormalSelect, parentCommand);

	m_connectorDragWire->saveGeometry();
	m_bendpointWire->saveGeometry();

	double traceWidth = getTraceWidth();
	QString tColor = traceColor(viewLayerSpec);

	long newID1 = ItemBase::getNextID();
	ViewGeometry vg1 = m_connectorDragWire->getViewGeometry();
	vg1.setWireFlags(getTraceFlag());
	new AddItemCommand(this, crossViewType, m_connectorDragWire->moduleID(), viewLayerSpec, vg1, newID1, true, -1, parentCommand);
	new CheckStickyCommand(this, crossViewType, newID1, false, CheckStickyCommand::RemoveOnly, parentCommand);
	new WireColorChangeCommand(this, newID1, tColor, tColor, 1.0, 1.0, parentCommand);
	new WireWidthChangeCommand(this, newID1, traceWidth, traceWidth, parentCommand);

	long newID2 = ItemBase::getNextID();
	ViewGeometry vg2 = m_bendpointWire->getViewGeometry();
	vg2.setWireFlags(getTraceFlag());
	new AddItemCommand(this, crossViewType, m_bendpointWire->moduleID(), viewLayerSpec, vg2, newID2, true, -1, parentCommand);
	new CheckStickyCommand(this, crossViewType, newID2, false, CheckStickyCommand::RemoveOnly, parentCommand);
	new WireColorChangeCommand(this, newID2, tColor, tColor, 1.0, 1.0, parentCommand);
	new WireWidthChangeCommand(this, newID2, traceWidth, traceWidth, parentCommand);

	new ChangeConnectionCommand(this, BaseCommand::CrossView,
									newID2, m_connectorDragConnector->connectorSharedID(),
									newID1, m_connectorDragWire->connector0()->connectorSharedID(),
									viewLayerSpec,					// ViewLayer::specFromID(wire->viewLayerID())
									true, parentCommand);

	foreach (ConnectorItem * toConnectorItem, m_bendpointWire->connector0()->connectedToItems()) {
		new ChangeConnectionCommand(this, BaseCommand::CrossView,
									newID2, m_bendpointWire->connector0()->connectorSharedID(),
									toConnectorItem->attachedToID(), toConnectorItem->connectorSharedID(),
									viewLayerSpec,					// ViewLayer::specFromID(toConnectorItem->attachedToViewLayerID())
									true, parentCommand);
	}
	foreach (ConnectorItem * toConnectorItem, m_connectorDragWire->connector1()->connectedToItems()) {
		new ChangeConnectionCommand(this, BaseCommand::CrossView,
									newID1, m_connectorDragWire->connector1()->connectorSharedID(),
									toConnectorItem->attachedToID(), toConnectorItem->connectorSharedID(),
									viewLayerSpec,					// ViewLayer::specFromID(toConnectorItem->attachedToViewLayerID())
									true, parentCommand);
		m_connectorDragWire->connector1()->tempRemove(toConnectorItem, false);
		toConnectorItem->tempRemove(m_connectorDragWire->connector1(), false);
		m_bendpointWire->connector1()->tempConnectTo(toConnectorItem, false);
		toConnectorItem->tempConnectTo(m_bendpointWire->connector1(), false);
	}

	m_bendpointWire->setPos(m_bendpointVG.loc());
	m_bendpointWire->setLine(m_bendpointVG.line());
	m_connectorDragConnector->tempRemove(m_connectorDragWire->connector0(), false);
	m_connectorDragWire->connector0()->tempRemove(m_connectorDragConnector, false);
	m_bendpointWire = NULL;			// signal that we're done

	// remove the temporary wire
	this->scene()->removeItem(m_connectorDragWire);

	new CleanUpWiresCommand(this, CleanUpWiresCommand::RedoOnly, parentCommand);
	m_undoStack->push(parentCommand);
}

void SketchWidget::setUpColor(ConnectorItem * fromConnectorItem, ConnectorItem * toConnectorItem, Wire * wire, QUndoCommand * parentCommand) {
	Q_UNUSED(fromConnectorItem);
	Q_UNUSED(toConnectorItem);

	if (!this->m_lastColorSelected.isEmpty()) {
		new WireColorChangeCommand(this, wire->id(), m_lastColorSelected, m_lastColorSelected, wire->opacity(), wire->opacity(), parentCommand);
	}
}

void SketchWidget::addViewLayer(ViewLayer * viewLayer) {
	ViewLayer * oldViewLayer = m_viewLayers.value(viewLayer->viewLayerID(), NULL);
	if (oldViewLayer) {
		delete oldViewLayer;
	}

	m_viewLayers.insert(viewLayer->viewLayerID(), viewLayer);
	QAction* action = new QAction(QObject::tr("%1 Layer").arg(viewLayer->displayName()), this);
	action->setData(QVariant::fromValue<ViewLayer *>(viewLayer));
	action->setCheckable(true);
	action->setChecked(viewLayer->visible());
	action->setEnabled(true);
    connect(action, SIGNAL(triggered()), this, SLOT(toggleLayerVisibility()));
    viewLayer->setAction(action);
}

void SketchWidget::setAllLayersVisible(bool visible) {
	LayerList keys = m_viewLayers.keys();

	for (int i = 0; i < keys.count(); i++) {
		ViewLayer * viewLayer = m_viewLayers.value(keys[i]);
		if (viewLayer != NULL && viewLayer->action()->isEnabled()) {
			setLayerVisible(viewLayer, visible, true);
		}
	}
}

ItemCount SketchWidget::calcItemCount() {
	ItemCount itemCount;

	// TODO: replace scene()->items()
	QList<QGraphicsItem *> items = scene()->items();
	QList<QGraphicsItem *> selItems = scene()->selectedItems();

	itemCount.visLabelCount = itemCount.hasLabelCount = 0;
	itemCount.selCount = 0;
	itemCount.selHFlipable = itemCount.selVFlipable = itemCount.selRotatable  = itemCount.sel45Rotatable = 0;
	itemCount.itemsCount = 0;
	itemCount.obsoleteCount = 0;
	itemCount.moveLockCount = 0;
	itemCount.wireCount = 0;

	for (int i = 0; i < selItems.count(); i++) {
		ItemBase * itemBase = ItemBase::extractTopLevelItemBase(selItems[i]);
		if (itemBase != NULL) {
			itemCount.selCount++;

			if (itemBase->moveLock()) {
				itemCount.moveLockCount++;
			}

			if (itemBase->hasPartLabel()) {
				itemCount.hasLabelCount++;
				if (itemBase->isPartLabelVisible()) {
					itemCount.visLabelCount++;
				}
			}

			if (itemBase->canFlipHorizontal()) {
				itemCount.selHFlipable++;
			}

			if (itemBase->canFlipVertical()) {
				itemCount.selVFlipable++;
			}

			if (itemBase->isObsolete()) {
				itemCount.obsoleteCount++;
			}

			if (itemBase->itemType() == ModelPart::Wire) {
				itemCount.wireCount++;
			}

			bool rotatable = itemBase->rotationAllowed();
			if (rotatable) {
				itemCount.selRotatable++;
			}

			rotatable = itemBase->rotation45Allowed();
			if (rotatable) {
				itemCount.sel45Rotatable++;
			}
		}
	}

	if (itemCount.selCount != itemCount.selRotatable) {
		// if you can't rotate them all, then you can't rotate any
		itemCount.selRotatable = 0;
	}
	if (itemCount.selCount != itemCount.sel45Rotatable) {
		// if you can't rotate them all, then you can't rotate any
		itemCount.sel45Rotatable = 0;
	}
	if (itemCount.selCount != itemCount.selVFlipable) {
		itemCount.selVFlipable = 0;
	}
	if (itemCount.selCount != itemCount.selHFlipable) {
		itemCount.selHFlipable = 0;
	}
	if (itemCount.selCount > 0) {
		for (int i = 0; i < items.count(); i++) {
			if (ItemBase::extractTopLevelItemBase(items[i]) != NULL) {
				itemCount.itemsCount++;
			}
		}
	}

	return itemCount;
}

bool SketchWidget::layerIsVisible(ViewLayer::ViewLayerID viewLayerID) {
	ViewLayer * viewLayer = m_viewLayers.value(viewLayerID);
	if (viewLayer == NULL) return false;

	return viewLayer->visible();
}

bool SketchWidget::layerIsActive(ViewLayer::ViewLayerID viewLayerID) {
	ViewLayer * viewLayer = m_viewLayers.value(viewLayerID);
	if (viewLayer == NULL) return false;

	return viewLayer->isActive();
}

void SketchWidget::setLayerVisible(ViewLayer::ViewLayerID viewLayerID, bool vis, bool doChildLayers) {
	ViewLayer * viewLayer = m_viewLayers.value(viewLayerID);
	if (viewLayer) {
		setLayerVisible(viewLayer, vis, doChildLayers);
	}
}

void SketchWidget::toggleLayerVisibility() {
	QAction * action = qobject_cast<QAction *>(sender());
	if (action == NULL) return;

	ViewLayer * viewLayer = action->data().value<ViewLayer *>();
	if (viewLayer == NULL) return;

	setLayerVisible(viewLayer, !viewLayer->visible(), viewLayer->includeChildLayers());
}

void SketchWidget::setLayerVisible(ViewLayer * viewLayer, bool visible, bool doChildLayers) {

	LayerList viewLayerIDs;
	viewLayerIDs.append(viewLayer->viewLayerID());

	viewLayer->setVisible(visible);
	if (doChildLayers) {
		foreach (ViewLayer * childLayer, viewLayer->childLayers()) {
			childLayer->setVisible(visible);
			viewLayerIDs.append(childLayer->viewLayerID());
		}
	}

	// TODO: replace scene()->items()
	foreach (QGraphicsItem * item, scene()->items()) {
		// want all items, not just topLevel
		ItemBase * itemBase = dynamic_cast<ItemBase *>(item);
		if (itemBase) {
			if (viewLayerIDs.contains(itemBase->viewLayerID())) {
				itemBase->setHidden(!visible);
				//DebugDialog::debug(QString("setting visible %1").arg(viewLayer->visible()));
			}
			continue;
		}

		PartLabel * partLabel = dynamic_cast<PartLabel *>(item);
		if (partLabel && (viewLayerIDs.contains(partLabel->viewLayerID()))) {
			partLabel->setHidden(!visible);
		}
	}
}

void SketchWidget::setLayerActive(ViewLayer::ViewLayerID viewLayerID, bool active) {
	ViewLayer * viewLayer = m_viewLayers.value(viewLayerID);
	if (viewLayer) {
		setLayerActive(viewLayer, active);
	}
}


void SketchWidget::setLayerActive(ViewLayer * viewLayer, bool active) {

	LayerList viewLayerIDs;
	viewLayerIDs.append(viewLayer->viewLayerID());

	viewLayer->setActive(active);
	foreach (ViewLayer * childLayer, viewLayer->childLayers()) {
		childLayer->setActive(active);
		viewLayerIDs.append(childLayer->viewLayerID());
	}

	// TODO: replace scene()->items()
	foreach (QGraphicsItem * item, scene()->items()) {
		// want all items, not just topLevel
		ItemBase * itemBase = dynamic_cast<ItemBase *>(item);
		if (itemBase && (viewLayerIDs.contains(itemBase->viewLayerID()))) {
			itemBase->setInactive(!active);
			//DebugDialog::debug(QString("setting visible %1").arg(viewLayer->visible()));
		}

		PartLabel * partLabel = dynamic_cast<PartLabel *>(item);
		if (partLabel && (viewLayerIDs.contains(partLabel->viewLayerID()))) {
			partLabel->setInactive(!active);
		}
	}
}

void SketchWidget::sendToBack() {
	QList<ItemBase *> bases;
	if (!startZChange(bases)) return;

	QString text = QObject::tr("Bring forward");
	continueZChangeMax(bases, bases.size() - 1, -1, greaterThan, -1, text);
}

void SketchWidget::sendBackward() {

	QList<ItemBase *> bases;
	if (!startZChange(bases)) return;

	QString text = QObject::tr("Send backward");
	continueZChange(bases, 0, bases.size(), lessThan, 1, text);
}

void SketchWidget::bringForward() {
	QList<ItemBase *> bases;
	if (!startZChange(bases)) return;

	QString text = QObject::tr("Bring forward");
	continueZChange(bases, bases.size() - 1, -1, greaterThan, -1, text);

}

void SketchWidget::bringToFront() {
	QList<ItemBase *> bases;
	if (!startZChange(bases)) return;

	QString text = QObject::tr("Bring to front");
	continueZChangeMax(bases, 0, bases.size(), lessThan, 1, text);
}

double SketchWidget::fitInWindow() {

	QRectF itemsRect;
	foreach(QGraphicsItem * item, scene()->items()) {
		ItemBase * itemBase = dynamic_cast<ItemBase *>(item);
		if (itemBase == NULL) continue;
		if (!itemBase->isEverVisible()) continue;

		itemsRect |= itemBase->sceneBoundingRect();
	}

	QRectF viewRect = rect();

	//fitInView(itemsRect.x(), itemsRect.y(), itemsRect.width(), itemsRect.height(), Qt::KeepAspectRatio);

	double wRelation = (viewRect.width() - this->verticalScrollBar()->width() - 5)  / itemsRect.width();
	double hRelation = (viewRect.height() - this->horizontalScrollBar()->height() - 5) / itemsRect.height();

	DebugDialog::debug(QString("scen rect: w%1 h%2").arg(itemsRect.width()).arg(itemsRect.height()));
	DebugDialog::debug(QString("view rect: w%1 h%2").arg(viewRect.width()).arg(viewRect.height()));
	DebugDialog::debug(QString("relations (v/s): w%1 h%2").arg(wRelation).arg(hRelation));

	if(wRelation < hRelation) {
		m_scaleValue = (wRelation * 100);
	} else {
		m_scaleValue = (hRelation * 100);
	}

	this->centerOn(itemsRect.center());
	this->absoluteZoom(m_scaleValue);

	return m_scaleValue;
}

bool SketchWidget::startZChange(QList<ItemBase *> & bases) {
	int selCount = bases.count();
	if (selCount == 0) {
		selCount = scene()->selectedItems().count();
		if (selCount <= 0) return false;
	}

	const QList<QGraphicsItem *> items = scene()->items();
	if (items.count() <= selCount) return false;

	sortAnyByZ(items, bases);

	return true;
}

void SketchWidget::continueZChange(QList<ItemBase *> & bases, int start, int end, bool (*test)(int current, int start), int inc, const QString & text) {

	bool moved = false;
	int last = bases.size();
	for (int i = start; test(i, end); i += inc) {
		ItemBase * base = bases[i];

		if (!base->getViewGeometry().selected()) continue;

		int j = i - inc;
		if (j >= 0 && j < last && bases[j]->viewLayerID() == base->viewLayerID()) {
			bases.move(i, j);
			moved = true;
		}
	}

	if (!moved) {
		return;
	}

	continueZChangeAux(bases, text);
}

void SketchWidget::continueZChangeMax(QList<ItemBase *> & bases, int start, int end, bool (*test)(int current, int start), int inc, const QString & text) {

	QHash<ItemBase *, ItemBase *> marked;
	bool moved = false;
	int last = bases.size();
	for (int i = start; test(i, end); i += inc) {
		ItemBase * base = bases[i];
		if (!base->getViewGeometry().selected()) continue;
		if (marked[base] != NULL) continue;

		marked.insert(base, base);

		int dest = -1;
		for (int j = i + inc; j >= 0 && j < last && bases[j]->viewLayerID() == base->viewLayerID(); j += inc) {
			dest = j;
		}

		if (dest >= 0) {
			moved = true;
			bases.move(i, dest);
			DebugDialog::debug(QString("moving %1 to %2").arg(i).arg(dest));
			i -= inc;	// because we just modified the list and would miss the next item
		}
	}

	if (!moved) {
		return;
	}

	continueZChangeAux(bases, text);
}


void SketchWidget::continueZChangeAux(QList<ItemBase *> & bases, const QString & text) {

	ChangeZCommand * changeZCommand = new ChangeZCommand(this, NULL);

	ViewLayer::ViewLayerID lastViewLayerID = ViewLayer::UnknownLayer;
	double z = 0;
	for (int i = 0; i < bases.size(); i++) {
		double oldZ = bases[i]->getViewGeometry().z();
		if (bases[i]->viewLayerID() != lastViewLayerID) {
			lastViewLayerID = bases[i]->viewLayerID();
			z = qFloor(oldZ);
		}
		else {
			z += ViewLayer::getZIncrement();
		}


		if (oldZ == z) continue;

		// optimize this by only adding z's that must change
		// rather than changing all of them
		changeZCommand->addTriplet(bases[i]->id(), oldZ, z);
	}

	changeZCommand->setText(text);
	m_undoStack->push(changeZCommand);
}

void SketchWidget::sortSelectedByZ(QList<ItemBase *> & bases) {

	const QList<QGraphicsItem *> items = scene()->selectedItems();
	if (items.size() <= 0) return;

	QList<QGraphicsItem *> tlBases;
	for (int i = 0; i < items.count(); i++) {
		ItemBase * itemBase =  ItemBase::extractTopLevelItemBase(items[i]);
		if (itemBase == NULL) continue;
		if (itemBase->getRatsnest()) continue;
		if (tlBases.contains(itemBase)) continue;

		if (itemBase != NULL) {
			tlBases.append(itemBase);
		}
	}

	sortAnyByZ(tlBases, bases);
}


void SketchWidget::sortAnyByZ(const QList<QGraphicsItem *> & items, QList<ItemBase *> & bases) {
	for (int i = 0; i < items.size(); i++) {
		ItemBase * base = dynamic_cast<ItemBase *>(items[i]);
		if (base != NULL) {
			bases.append(base);
			base->saveGeometry();
		}
	}

    // order by z
    qSort(bases.begin(), bases.end(), ItemBase::zLessThan);
}

bool SketchWidget::lessThan(int a, int b) {
	return a < b;
}

bool SketchWidget::greaterThan(int a, int b) {
	return a > b;
}

void SketchWidget::changeZ(QHash<long, RealPair * > triplets, double (*pairAccessor)(RealPair *) ) {

	// TODO: replace scene->items
	const QList<QGraphicsItem *> items = scene()->items();
	for (int i = 0; i < items.size(); i++) {
		// want all items, not just topLevel
		ItemBase * itemBase = dynamic_cast<ItemBase *>(items[i]);
		if (itemBase == NULL) continue;

		RealPair * pair = triplets[itemBase->id()];
		if (pair == NULL) continue;

		double newZ = pairAccessor(pair);
		DebugDialog::debug(QString("change z %1 %2").arg(itemBase->id()).arg(newZ));
		items[i]->setZValue(newZ);

	}
}

ViewLayer::ViewLayerID SketchWidget::getDragWireViewLayerID(ConnectorItem *) {
	return m_wireViewLayerID;
}

ViewLayer::ViewLayerID SketchWidget::getWireViewLayerID(const ViewGeometry & viewGeometry, ViewLayer::ViewLayerSpec) {
	if (viewGeometry.getRatsnest()) {
		return ViewLayer::BreadboardRatsnest;
	}

	return m_wireViewLayerID;
}

ViewLayer::ViewLayerID SketchWidget::getRulerViewLayerID() {
	return m_rulerViewLayerID;
}

ViewLayer::ViewLayerID SketchWidget::getPartViewLayerID() {
	return m_partViewLayerID;
}

ViewLayer::ViewLayerID SketchWidget::getConnectorViewLayerID() {
	return m_connectorViewLayerID;
}

ViewLayer::ViewLayerID SketchWidget::getLabelViewLayerID(ViewLayer::ViewLayerSpec) {
	return ViewLayer::UnknownLayer;
}

ViewLayer::ViewLayerID SketchWidget::getNoteViewLayerID() {
	return m_noteViewLayerID;
}


void SketchWidget::mousePressConnectorEvent(ConnectorItem * connectorItem, QGraphicsSceneMouseEvent * event) {

	ModelPart * wireModel = m_paletteModel->retrieveModelPart(ModuleIDNames::WireModuleIDName);
	if (wireModel == NULL) return;

	m_tempDragWireCommand = m_holdingSelectItemCommand;
	m_holdingSelectItemCommand = NULL;
	clearHoldingSelectItem();
	

	// make sure wire layer is visible
	ViewLayer::ViewLayerID viewLayerID = getDragWireViewLayerID(connectorItem);
	ViewLayer * viewLayer = m_viewLayers.value(viewLayerID);
	if (viewLayer != NULL && !viewLayer->visible()) {
		setLayerVisible(viewLayer, true, true);
	}


	ViewGeometry viewGeometry;
   	QPointF p = QPointF(connectorItem->mapToScene(event->pos()));
   	viewGeometry.setLoc(p);
	viewGeometry.setLine(QLineF(0,0,0,0));

	m_connectorDragConnector = connectorItem;
	m_connectorDragWire = createTempWireForDragging(NULL, wireModel, connectorItem, viewGeometry, ViewLayer::UnknownSpec);
	if (m_connectorDragWire == NULL) {
		clearDragWireTempCommand();
		return;
	}

	m_connectorDragWire->debugInfo("creating connector drag wire");

	setupAutoscroll(true);

	// give connector item the mouse, so wire doesn't get mouse moved events
	m_connectorDragWire->setVisible(true);
	m_connectorDragWire->grabMouse();
	m_connectorDragWire->initDragEnd(m_connectorDragWire->connector0(), event->scenePos());
	m_connectorDragConnector->tempConnectTo(m_connectorDragWire->connector1(), false);
	m_connectorDragWire->connector1()->tempConnectTo(m_connectorDragConnector, false);
	if (!m_lastColorSelected.isEmpty()) {
		m_connectorDragWire->setColorString(m_lastColorSelected, m_connectorDragWire->opacity());
	}
}

void SketchWidget::rotateX(double degrees, bool rubberBandLegEnabled) 
{
	clearHoldingSelectItem();
	m_savedItems.clear();
	m_savedWires.clear();
	prepMove(NULL, rubberBandLegEnabled);

	QRectF itemsBoundingRect;
	// want the bounding rect of the original selected items, not all the items that are secondarily being rotated
	foreach (QGraphicsItem * item, scene()->selectedItems()) {
		ItemBase * itemBase = dynamic_cast<ItemBase *>(item);
		if (itemBase == NULL) continue;

		itemsBoundingRect |= (item->transform() * QTransform().translate(item->x(), item->y()))
                            .mapRect(itemBase->boundingRectWithoutLegs() /* | item->childrenBoundingRect() */);
	}

	QPointF center = itemsBoundingRect.center();

	QTransform rotation;
	rotation.rotate(degrees);

	QString string = tr("Rotate %2 (%1)")
			.arg(ViewIdentifierClass::viewIdentifierName(m_viewIdentifier))
			.arg((m_savedItems.count() == 1) ? m_savedItems.values().at(0)->title() : QString::number(m_savedItems.count() + m_savedWires.count()) + " items" );
	QUndoCommand * parentCommand = new QUndoCommand(string);

	//foreach (long id, m_savedItems.keys()) {
		//m_savedItems.value(id)->debugInfo(QString("save item %1").arg(id));
	//}

	new CleanUpWiresCommand(this, CleanUpWiresCommand::UndoOnly, parentCommand);

	// change legs after connections have been updated (undo direction)
	moveLegBendpoints(true, parentCommand);

	rotatePartLabels(degrees, rotation, center, parentCommand);

	foreach (ItemBase * itemBase, m_savedItems.values()) {
		switch (itemBase->itemType()) {
			case ModelPart::Via:
			case ModelPart::Hole:
				{
					QPointF p = itemBase->sceneBoundingRect().center();
					QPointF d = p - center;
					QPointF dt = rotation.map(d) + center;
					ViewGeometry vg1 = itemBase->getViewGeometry();
					ViewGeometry vg2(vg1);
					vg2.setLoc(vg1.loc() + dt - p);
					new MoveItemCommand(this, itemBase->id(), vg1, vg2, true, parentCommand);
				}
				break;

			case ModelPart::Wire:
				{
					Wire * wire = qobject_cast<Wire *>(itemBase);
					QPointF p0 = wire->connector0()->sceneAdjustedTerminalPoint(NULL);
					QPointF d0 = p0 - center;
					QPointF d0t = rotation.map(d0);

					QPointF p1 = wire->connector1()->sceneAdjustedTerminalPoint(NULL);
					QPointF d1 = p1 - center;
					QPointF d1t = rotation.map(d1);

					ViewGeometry vg1 = itemBase->getViewGeometry();
					new ChangeWireCommand(this, wire->id(), vg1.line(), QLineF(QPointF(0,0), d1t - d0t), vg1.loc(), d0t + center, true, true, parentCommand);
				}
				break;

			default:
				{
					ViewGeometry vg1 = itemBase->getViewGeometry();
					ViewGeometry vg2(vg1);
					itemBase->calcRotation(rotation, center, vg2);
					ConnectorPairHash connectorHash;
					disconnectFromFemale(itemBase, m_savedItems, connectorHash, true, rubberBandLegEnabled, parentCommand);
					new MoveItemCommand(this, itemBase->id(), vg1, vg1, true, parentCommand);
					new RotateItemCommand(this, itemBase->id(), degrees, parentCommand);
					new MoveItemCommand(this, itemBase->id(), vg2, vg2, true, parentCommand);
				}
				break;
		}
	}

	// change legs after connections have been updated (redo direction)
	QList<ConnectorItem *> connectorItems;
	foreach (ItemBase * itemBase, m_stretchingLegs.uniqueKeys()) {
		foreach (ConnectorItem * connectorItem, m_stretchingLegs.values(itemBase)) {
			connectorItems.append(connectorItem);
			QPolygonF oldLeg, newLeg;
			bool active;
			connectorItem->stretchDone(oldLeg, newLeg, active);
			new RotateLegCommand(this, connectorItem->attachedToID(), connectorItem->connectorSharedID(), oldLeg, active, parentCommand);
		}
	}

	foreach (Wire * wire, m_savedWires.keys()) {
		ViewGeometry vg1 = wire->getViewGeometry();
		ViewGeometry vg2(vg1);

		ConnectorItem * rotater = m_savedWires.value(wire);
		QPointF p0 = rotater->sceneAdjustedTerminalPoint(NULL);
		QPointF d0 = p0 - center;
		QPointF d0t = rotation.map(d0);

		QPointF p1 = wire->otherConnector(rotater)->sceneAdjustedTerminalPoint(NULL);
		if (rotater == wire->connector0()) {
			new ChangeWireCommand(this, wire->id(), vg1.line(), QLineF(QPointF(0,0), p1 - (d0t + center)), vg1.loc(), d0t + center, true, true, parentCommand);
		}
		else {
			new ChangeWireCommand(this, wire->id(), vg1.line(), QLineF(QPointF(0,0), d0t + center - p1), vg1.loc(), vg1.loc(), true, true, parentCommand);
		}
	}

	new CleanUpWiresCommand(this, CleanUpWiresCommand::RedoOnly, parentCommand);

	m_undoStack->push(parentCommand);
}

void SketchWidget::rotatePartLabels(double degrees, QTransform &, QPointF center, QUndoCommand * parentCommand)
{
	Q_UNUSED(center);
	Q_UNUSED(degrees);
	Q_UNUSED(parentCommand);
}

void SketchWidget::flipX(Qt::Orientations orientation, bool rubberBandLegEnabled) 
{
	if (!this->isVisible()) return;

	clearHoldingSelectItem();
	m_savedItems.clear();
	m_savedWires.clear();
	prepMove(NULL, rubberBandLegEnabled);

	QList <QGraphicsItem *> items = scene()->selectedItems();
	QList <ItemBase *> targets;

	for (int i = 0; i < items.size(); i++) {
		// can't flip layerkin (layerkin flipped indirectly)
		ItemBase *itemBase = ItemBase::extractTopLevelItemBase(items[i]);
		if (itemBase == NULL) continue;

		switch (itemBase->itemType()) {
			case ModelPart::Wire:
			case ModelPart::Note:
			case ModelPart::CopperFill:
			case ModelPart::Unknown:
			case ModelPart::Via:
			case ModelPart::Hole:
			case ModelPart::Board:
			case ModelPart::ResizableBoard:
			case ModelPart::Breadboard:
				continue;

			default:
				if (!itemBase->canFlip(orientation)) {
					continue;
				}
				break;
		}

		targets.append(itemBase);
	}

	if (targets.count() <= 0) {
		return;
	}

	QString string = tr("Flip %2 (%1)")
			.arg(ViewIdentifierClass::viewIdentifierName(m_viewIdentifier))
			.arg((targets.count() == 1) ? targets[0]->title() : QString::number(targets.count()) + " items" );

	QUndoCommand * parentCommand = new QUndoCommand(string);

	new CleanUpWiresCommand(this, CleanUpWiresCommand::UndoOnly, parentCommand);

	// change legs after connections have been updated (undo direction)
	moveLegBendpoints(true, parentCommand);

	QHash<long, ItemBase *> emptyList;			// emptylist is only used for a move command
	ConnectorPairHash connectorHash;
	foreach (ItemBase * item, targets) {
		disconnectFromFemale(item, emptyList, connectorHash, true, rubberBandLegEnabled, parentCommand);

		if (item->sticky()) {
			//TODO: apply transformation to stuck items
		}
		// TODO: if item has female connectors, then apply transform to connected items

		new FlipItemCommand(this, item->id(), orientation, parentCommand);
	}

	// change legs after connections have been updated (redo direction)
	foreach (ItemBase * itemBase, m_stretchingLegs.uniqueKeys()) {
		foreach (ConnectorItem * connectorItem, m_stretchingLegs.values(itemBase)) {
			QPolygonF oldLeg, newLeg;
			bool active;
			connectorItem->stretchDone(oldLeg, newLeg, active);
			new RotateLegCommand(this, connectorItem->attachedToID(), connectorItem->connectorSharedID(), oldLeg, active, parentCommand);
		}
	}

	clearTemporaries();

	new CleanUpWiresCommand(this, CleanUpWiresCommand::RedoOnly, parentCommand);
	m_undoStack->push(parentCommand);

}

ConnectorItem * SketchWidget::findConnectorItem(ConnectorItem * foreignConnectorItem) {
	ItemBase * itemBase = findItem(foreignConnectorItem->attachedTo()->layerKinChief()->id());

	if (itemBase == NULL) {
		return NULL;
	}

	ConnectorItem * result = findConnectorItem(itemBase, foreignConnectorItem->connectorSharedID(), ViewLayer::Bottom);
	if (result) return result;

	return findConnectorItem(itemBase, foreignConnectorItem->connectorSharedID(), ViewLayer::Top);
}

ConnectorItem * SketchWidget::findConnectorItem(ItemBase * itemBase, const QString & connectorID, ViewLayer::ViewLayerSpec viewLayerSpec) {

	ConnectorItem * connectorItem = itemBase->findConnectorItemWithSharedID(connectorID, viewLayerSpec);
	if (connectorItem != NULL) return connectorItem;

	DebugDialog::debug("used to seek layer kin");
	/*
	if (seekLayerKin) {
		PaletteItem * pitem = qobject_cast<PaletteItem *>(itemBase);
		if (pitem == NULL) return NULL;

		foreach (ItemBase * lkpi, pitem->layerKin()) {
			connectorItem = lkpi->findConnectorItemWithSharedID(connectorID);
			if (connectorItem != NULL) return connectorItem;
		}

	}
	*/

	return NULL;
}

PaletteItem * SketchWidget::getSelectedPart(){
	QList <QGraphicsItem *> items= scene()->selectedItems();
	PaletteItem *item = NULL;

	// dynamic cast returns null in cases where non-PaletteItems (i.e. wires and layerKin palette items) are selected
	for(int i=0; i < items.count(); i++){
		PaletteItem *temp = dynamic_cast<PaletteItem *>(items[i]);
		if (temp == NULL) continue;

		if (item != NULL) return NULL;  // there are multiple items selected
		item = temp;
	}

	return item;
}

void SketchWidget::setBackground(QColor color) {
	/*QBrush brush(color);
	brush.setTexture(QPixmap("/home/merun/workspace/fritzing_trunk/phoenix/resources/images/schematic_grid_tile.png"));
	scene()->setBackgroundBrush(brush);*/
	scene()->setBackgroundBrush(QBrush(color));
}

const QColor& SketchWidget::background() {
	return scene()->backgroundBrush().color();
}

void SketchWidget::setItemMenu(QMenu* itemMenu){
	m_itemMenu = itemMenu;
}

void SketchWidget::setWireMenu(QMenu* wireMenu){
	m_wireMenu = wireMenu;
}

void SketchWidget::wireConnectedSlot(long fromID, QString fromConnectorID, long toID, QString toConnectorID) {
	ItemBase * fromItem = findItem(fromID);
	if (fromItem == NULL) return;

	Wire* wire = qobject_cast<Wire *>(fromItem);
	if (wire == NULL) return;

	ConnectorItem * fromConnectorItem = findConnectorItem(fromItem, fromConnectorID, ViewLayer::specFromID(wire->viewLayerID()));
	if (fromConnectorItem == NULL) {
		// shouldn't be here
		return;
	}

	ItemBase * toItem = findItem(toID);
	if (toItem == NULL) {
		// this was a disconnect
		return;
	}

	ConnectorItem * toConnectorItem = findConnectorItem(toItem, toConnectorID, ViewLayer::specFromID(wire->viewLayerID()));
	if (toConnectorItem == NULL) {
		// shouldn't really be here
		return;
	}

	QPointF p1(0,0), p2, pos;

	ConnectorItem * other = wire->otherConnector(fromConnectorItem);
	if (fromConnectorItem == wire->connector0()) {
		pos = toConnectorItem->sceneAdjustedTerminalPoint(fromConnectorItem);
		ConnectorItem * toConnector1 = other->firstConnectedToIsh();
		if (toConnector1 == NULL) {
			p2 = other->mapToScene(other->pos()) - pos;
		}
		else {
			p2 = toConnector1->sceneAdjustedTerminalPoint(other);
		}
	}
	else {
		pos = wire->pos();
		ConnectorItem * toConnector0 = other->firstConnectedToIsh();
		if (toConnector0 == NULL) {
			pos = wire->pos();
		}
		else {
			pos = toConnector0->sceneAdjustedTerminalPoint(other);
		}
		p2 = toConnectorItem->sceneAdjustedTerminalPoint(fromConnectorItem) - pos;
	}
	wire->setLineAnd(QLineF(p1, p2), pos, true);

	// here's the connect (model has been previously updated)
	fromConnectorItem->connectTo(toConnectorItem);
	toConnectorItem->connectTo(fromConnectorItem);

	this->update();

}

void SketchWidget::wireDisconnectedSlot(long fromID, QString fromConnectorID) {
	DebugDialog::debug(QString("got wire disconnected"));
	ItemBase * fromItem = findItem(fromID);
	if (fromItem == NULL) return;

	Wire* wire = qobject_cast<Wire *>(fromItem);
	if (wire == NULL) return;

	ConnectorItem * fromConnectorItem = findConnectorItem(fromItem, fromConnectorID, ViewLayer::specFromID(wire->viewLayerID()));
	if (fromConnectorItem == NULL) {
		// shouldn't be here
		return;
	}

	ConnectorItem * toConnectorItem = fromConnectorItem->firstConnectedToIsh();
	if (toConnectorItem != NULL) {
		fromConnectorItem->removeConnection(toConnectorItem, true);
		toConnectorItem->removeConnection(fromConnectorItem, true);
	}
}

void SketchWidget::changeConnection(long fromID, const QString & fromConnectorID,
									long toID, const QString & toConnectorID,
									ViewLayer::ViewLayerSpec viewLayerSpec,
									bool connect, bool doEmit, bool updateConnections)
{
	changeConnectionAux(fromID, fromConnectorID, toID, toConnectorID, viewLayerSpec, connect, updateConnections);

	if (doEmit) {
		//TODO:  findPartOrWire not necessary for harmonize? 
		//fromID = findPartOrWire(fromID);
		//toID = findPartOrWire(toID);
		emit changeConnectionSignal(fromID, fromConnectorID, toID, toConnectorID, viewLayerSpec, connect, updateConnections);
	}
}

void SketchWidget::changeConnectionAux(long fromID, const QString & fromConnectorID,
									long toID, const QString & toConnectorID,
									ViewLayer::ViewLayerSpec viewLayerSpec,
									bool connect, bool updateConnections)
{

	DebugDialog::debug(QString("changeConnection: from %1 %2; to %3 %4 con:%5 v:%6")
				.arg(fromID).arg(fromConnectorID)
				.arg(toID).arg(toConnectorID)
				.arg(connect).arg(m_viewIdentifier) );

	ItemBase * fromItem = findItem(fromID);
	if (fromItem == NULL) {
		DebugDialog::debug(QString("change connection exit 1 %1").arg(fromID));
		return;
	}

	ConnectorItem * fromConnectorItem = findConnectorItem(fromItem, fromConnectorID, viewLayerSpec);
	if (fromConnectorItem == NULL) {
		// shouldn't be here
		DebugDialog::debug(QString("change connection exit 2 %1 %2").arg(fromID).arg(fromConnectorID));
		return;
	}

	ItemBase * toItem = findItem(toID);
	if (toItem == NULL) {
		DebugDialog::debug(QString("change connection exit 3 %1").arg(toID));
		return;
	}

	ConnectorItem * toConnectorItem = findConnectorItem(toItem, toConnectorID, viewLayerSpec);
	if (toConnectorItem == NULL) {
		// shouldn't be here
		DebugDialog::debug(QString("change connection exit 4 %1 %2").arg(toID).arg(toConnectorID));
		return;
	}

	fromConnectorItem->debugInfo("   from");
	toConnectorItem->debugInfo("   to");

	ratsnestConnect(fromConnectorItem, toConnectorItem, connect);

	if (connect) {
		fromConnectorItem->connector()->connectTo(toConnectorItem->connector());
		fromConnectorItem->connectTo(toConnectorItem);
		toConnectorItem->connectTo(fromConnectorItem);
	}
	else {
		fromConnectorItem->connector()->disconnectFrom(toConnectorItem->connector());
		fromConnectorItem->removeConnection(toConnectorItem, true);
		toConnectorItem->removeConnection(fromConnectorItem, true);
	}

	if (updateConnections) {
		fromConnectorItem->attachedTo()->updateConnections(fromConnectorItem);
		toConnectorItem->attachedTo()->updateConnections(toConnectorItem);
	}
}

void SketchWidget::changeConnectionSlot(long fromID, QString fromConnectorID,
												 long toID, QString toConnectorID,
												 ViewLayer::ViewLayerSpec viewLayerSpec, 
												 bool connect, bool updateConnections)
{
	changeConnection(fromID, fromConnectorID,
					 toID, toConnectorID, viewLayerSpec,
					 connect, false, updateConnections);
}

void SketchWidget::navigatorScrollChange(double x, double y) {
	QScrollBar * h = this->horizontalScrollBar();
	QScrollBar * v = this->verticalScrollBar();
	int xmin = h->minimum();
   	int xmax = h->maximum();
   	int ymin = v->minimum();
   	int ymax = v->maximum();

   	h->setValue((int) ((xmax - xmin) * x) + xmin);
   	v->setValue((int) ((ymax - ymin) * y) + ymin);
}

void SketchWidget::keyReleaseEvent(QKeyEvent * event) {
	//DebugDialog::debug(QString("key release event %1").arg(event->isAutoRepeat()));
	if (m_movingByArrow) {
		m_autoScrollTimer.stop();
		m_arrowTimer.start();
		//DebugDialog::debug("key release event");
	}
	else {
		QGraphicsView::keyReleaseEvent(event);
	}
}

void SketchWidget::arrowTimerTimeout() {
	m_movingByArrow = false;
	if (checkMoved()) {
		m_savedItems.clear();
		m_savedWires.clear();
	}
}

void SketchWidget::keyPressEvent ( QKeyEvent * event ) {
	//DebugDialog::debug("key press event");
	if ((m_inFocus.length() == 0) && !m_movingByMouse) {
		int dx = 0, dy = 0;
		switch (event->key()) {
			case Qt::Key_Up:
				dy = -1;
				break;
			case Qt::Key_Down:
				dy = 1;
				break;
			case Qt::Key_Left:
				dx = -1;
				break;
			case Qt::Key_Right:
				dx = 1;
				break;
			default:
				break;
		}
		if (dx != 0 || dy != 0) {
			m_arrowTimer.stop();
			DebugDialog::debug("arrow press event");
			ConnectorItem::clearEqualPotentialDisplay();
			moveByArrow(dx, dy, event);
			m_arrowTimer.start();
			return;
		}
	}

	QGraphicsView::keyPressEvent(event);
}

void SketchWidget::makeDeleteItemCommand(ItemBase * itemBase, BaseCommand::CrossViewType crossView, QUndoCommand * parentCommand) {

	if (crossView == BaseCommand::CrossView) {
		emit makeDeleteItemCommandPrepSignal(itemBase, true, parentCommand);
	}
	makeDeleteItemCommandPrepSlot(itemBase, false, parentCommand);

	if (crossView == BaseCommand::CrossView) {
		emit makeDeleteItemCommandFinalSignal(itemBase, true, parentCommand);
	}
	makeDeleteItemCommandFinalSlot(itemBase, false, parentCommand);

}

void SketchWidget::makeDeleteItemCommandPrepSlot(ItemBase * itemBase, bool foreign, QUndoCommand * parentCommand) 
{
	if (foreign) {
		itemBase = findItem(itemBase->id());
		if (itemBase == NULL) return;
	}

	if (itemBase->isPartLabelVisible()) {
		ShowLabelCommand * slc = new ShowLabelCommand(this, parentCommand);
		slc->add(itemBase->id(), true, true);
	}

	Note * note = qobject_cast<Note *>(itemBase);
	if (note != NULL) {
		new ChangeNoteTextCommand(this, note->id(), note->text(), note->text(), QSizeF(), QSizeF(), parentCommand);
	}
	else {
		new ChangeLabelTextCommand(this, itemBase->id(), itemBase->instanceTitle(), itemBase->instanceTitle(), parentCommand);
	}

	if (!foreign) {
		prepDeleteProps(itemBase, itemBase->id(), "", parentCommand);
	}

	rememberSticky(itemBase, parentCommand);

	Wire * wire = qobject_cast<Wire *>(itemBase);
	if (wire) {
		const Bezier * bezier = wire->curve();
		if (bezier && !bezier->isEmpty()) {
			ChangeWireCurveCommand * cwcc = new ChangeWireCurveCommand(this, itemBase->id(), bezier, NULL, parentCommand);
			cwcc->setUndoOnly();
		}
	}

	if (itemBase->hasRubberBandLeg()) {
		foreach (ConnectorItem * connectorItem, itemBase->cachedConnectorItems()) {
			if (!connectorItem->hasRubberBandLeg()) continue;

			// backwards order: curves then polys, since these will be trigged by undo
			QVector<Bezier *> beziers = connectorItem->beziers();
			for (int i = 0; i < beziers.count() - 1; i++) {
				Bezier * bezier = beziers.at(i);
				if (bezier == NULL) continue;
				if (bezier->isEmpty()) continue;

				ChangeLegCurveCommand * clcc = new ChangeLegCurveCommand(this, itemBase->id(), connectorItem->connectorSharedID(), i, bezier, bezier, parentCommand);
				clcc->setUndoOnly();
			}

			QPolygonF poly = connectorItem->leg();
			ChangeLegCommand * clc = new ChangeLegCommand(this, itemBase->id(), connectorItem->connectorSharedID(), poly, poly, true, true, "delete", parentCommand);
			clc->setUndoOnly();	

			// TODO: beziers here
		}
	}
}

void SketchWidget::makeDeleteItemCommandFinalSlot(ItemBase * itemBase, bool foreign, QUndoCommand * parentCommand) 
{
	if (foreign) {
		itemBase = findItem(itemBase->id());
		if (itemBase == NULL) return;
	}

	ModelPart * mp = itemBase->modelPart();
	// single view because this is called for each view
	new DeleteItemCommand(this, BaseCommand::SingleView, mp->moduleID(), itemBase->viewLayerSpec(), itemBase->getViewGeometry(), itemBase->id(), mp->modelIndex(), parentCommand);
}

void SketchWidget::prepDeleteProps(ItemBase * itemBase, long id, const QString & newModuleID, QUndoCommand * parentCommand) 
{
	// TODO: does this need to be generalized to the whole set of modelpart props?
	// TODO: Ruler?

	// TODO: this all belongs in PartFactory

	// NOTE:  prepDeleteProps is called after a swap and assumes that the new part is closely related to the old part
	// meaning that the properties of itemBase (which is the old part) apply to the new part (which has not yet been created)
	// this works most of the time, but does not, for example, when a ResizableBoard is swapped for a custom board shape

	ModelPart * mp = (newModuleID.isEmpty()) ? itemBase->modelPart() : paletteModel()->retrieveModelPart(newModuleID);

	switch (mp->itemType()) {
		case ModelPart::Wire:
			{
				Wire * wire = qobject_cast<Wire *>(itemBase);
				new WireWidthChangeCommand(this, id, wire->width(), wire->width(), parentCommand);
				new WireColorChangeCommand(this, id, wire->colorString(), wire->colorString(), wire->opacity(), wire->opacity(), parentCommand);
			}
			return;
		
		case ModelPart::Board:
			new ChangeBoardLayersCommand(this, m_boardLayers, m_boardLayers, parentCommand);
			prepDeleteOtherProps(itemBase, id, newModuleID, parentCommand);
			return;
		
		case ModelPart::ResizableBoard:
			{
				new ChangeBoardLayersCommand(this, m_boardLayers, m_boardLayers, parentCommand);
				ResizableBoard * brd = qobject_cast<ResizableBoard *>(itemBase);
				if (brd) {
					brd->saveParams();
					QPointF p;
					QSizeF sz;
					brd->getParams(p, sz);
					new ResizeBoardCommand(this, id, sz.width(), sz.height(), sz.width(), sz.height(), parentCommand);
				}
				prepDeleteOtherProps(itemBase, id, newModuleID, parentCommand);
			}
			return;

		case ModelPart::Logo:
			{
				LogoItem * logo = qobject_cast<LogoItem *>(itemBase);
				logo->saveParams();
				QPointF p;
				QSizeF sz;
				logo->getParams(p, sz);
				new ResizeBoardCommand(this, id, sz.width(), sz.height(), sz.width(), sz.height(), parentCommand);
				QString logoProp = logo->prop("logo");
				QString shapeProp = logo->prop("shape");
				if (!logoProp.isEmpty()) {
					new SetPropCommand(this, id, "logo", logoProp, logoProp, true, parentCommand);
				}
				else if (!shapeProp.isEmpty()) {
					new LoadLogoImageCommand(this, id, shapeProp, logo->modelPart()->prop("aspectratio").toSizeF(), logo->prop("lastfilename"), "", false, parentCommand);
				}
				prepDeleteOtherProps(itemBase, id, newModuleID, parentCommand);
			}
			return;

		case ModelPart::Jumper:
			{
				JumperItem * jumper = qobject_cast<JumperItem *>(itemBase);
				jumper->saveParams();
				QPointF p;
				QPointF c0, c1;
				jumper->getParams(p, c0, c1);
				new ResizeJumperItemCommand(this, id, p, c0, c1, p, c0, c1, parentCommand);
				prepDeleteOtherProps(itemBase, id, newModuleID, parentCommand);
			}
			return;

		case ModelPart::CopperFill:
			{
				GroundPlane * groundPlane = dynamic_cast<GroundPlane *>(itemBase);
				new SetPropCommand(this, id, "svg", groundPlane->svg(), groundPlane->svg(), true, parentCommand);
				prepDeleteOtherProps(itemBase, id, newModuleID, parentCommand);
			}
			return;

		default:
			break;
	}

	Resistor * resistor =  qobject_cast<Resistor *>(itemBase);
	if (resistor != NULL) {
		new SetResistanceCommand(this, id, resistor->resistance(), resistor->resistance(), resistor->pinSpacing(), resistor->pinSpacing(), parentCommand);
		prepDeleteOtherProps(itemBase, id, newModuleID, parentCommand);
		return;
	}

	MysteryPart * mysteryPart = qobject_cast<MysteryPart *>(itemBase);
	if (mysteryPart != NULL) {
		new SetPropCommand(this, id, "chip label", mysteryPart->chipLabel(), mysteryPart->chipLabel(), true, parentCommand);
		new SetPropCommand(this, id, "spacing", mysteryPart->spacing(), mysteryPart->spacing(), true, parentCommand);
		prepDeleteOtherProps(itemBase, id, newModuleID, parentCommand);
		return;
	}

	PinHeader * pinHeader = qobject_cast<PinHeader *>(itemBase);
	if (pinHeader != NULL) {
		// deal with old-style pin headers (pre 0.6.4)
		new SetPropCommand(this, id, "form", pinHeader->form(), PinHeader::findForm(newModuleID), true, parentCommand);
		prepDeleteOtherProps(itemBase, id, newModuleID, parentCommand);
		return;
	}


	Hole * hole = qobject_cast<Hole *>(itemBase);
	if (hole != NULL) {
		new SetPropCommand(this, id, "hole size", hole->holeSize(), hole->holeSize(), true, parentCommand);
		prepDeleteOtherProps(itemBase, id, newModuleID, parentCommand);
		return;
	}

	prepDeleteOtherProps(itemBase, id, newModuleID, parentCommand);
}

void SketchWidget::prepDeleteOtherProps(ItemBase * itemBase, long id, const QString & newModuleID, QUndoCommand * parentCommand) 
{
	Capacitor * capacitor = qobject_cast<Capacitor *>(itemBase); 
	if (capacitor) {
		QHash<QString, QString> properties;
		capacitor->getProperties(properties);
		foreach(QString prop, properties.keys()) {
			new SetPropCommand(this, id, prop, properties.value(prop), properties.value(prop), true, parentCommand);
		}
	}

	if (itemBase->moduleID().endsWith(ModuleIDNames::StripboardModuleIDName)) {
		QString buses = itemBase->prop("buses");
		if (!buses.isEmpty()) {
			new SetPropCommand(this, id, "buses", buses, buses, true, parentCommand);
		}
	}

	QString value = itemBase->modelPart()->prop(ModelPartShared::PartNumberPropertyName).toString();
	if (!value.isEmpty()) {
		QString newValue = value;
		if (!newModuleID.isEmpty()) {
			newValue = "";
			ModelPart * newModelPart = m_refModel->retrieveModelPart(newModuleID);
			if (newModelPart != NULL) {
				newValue = newModelPart->properties().value(ModelPartShared::PartNumberPropertyName, "");
			}
		}
		new SetPropCommand(this, id, ModelPartShared::PartNumberPropertyName, value, newValue, true, parentCommand);
	}
}

void SketchWidget::rememberSticky(long id, QUndoCommand * parentCommand) {
	ItemBase * itemBase = findItem(id);
	if (itemBase == NULL) return;

	rememberSticky(itemBase, parentCommand);
}

void SketchWidget::rememberSticky(ItemBase * itemBase, QUndoCommand * parentCommand) {

	QList< QPointer<ItemBase> > stickyList = itemBase->stickyList();
	if (stickyList.count() <= 0) return;

	CheckStickyCommand * checkStickyCommand = new CheckStickyCommand(this, BaseCommand::SingleView, itemBase->id(), false, CheckStickyCommand::UndoOnly, parentCommand);
	if (itemBase->sticky()) {
		foreach (ItemBase * sticking, stickyList) {
			checkStickyCommand->stick(this, itemBase->id(), sticking->id(), true);
		}
	}
	else if (itemBase->stickingTo() != NULL) {
		checkStickyCommand->stick(this, itemBase->stickingTo()->id(), itemBase->id(), true);
	}
}

ViewIdentifierClass::ViewIdentifier SketchWidget::viewIdentifier() {
	return m_viewIdentifier;
}

void SketchWidget::setViewLayerIDs(ViewLayer::ViewLayerID part, ViewLayer::ViewLayerID wire, ViewLayer::ViewLayerID connector, ViewLayer::ViewLayerID ruler, ViewLayer::ViewLayerID note) {
	m_partViewLayerID = part;
	m_wireViewLayerID = wire;
	m_connectorViewLayerID = connector;
	m_rulerViewLayerID = ruler;
	m_noteViewLayerID = note;
}

void SketchWidget::dragIsDoneSlot(ItemDrag * itemDrag) {
	disconnect(itemDrag, SIGNAL(dragIsDoneSignal(ItemDrag *)), this, SLOT(dragIsDoneSlot(ItemDrag *)));
	killDroppingItem();					// drag is done, but nothing dropped here: remove the temporary item
}


void SketchWidget::clearTemporaries() {
	for (int i = 0; i < m_temporaries.count(); i++) {
		QGraphicsItem * item = m_temporaries[i];
		scene()->removeItem(item);
		delete item;
	}

	m_temporaries.clear();
}

void SketchWidget::killDroppingItem() {
	m_alignmentItem = NULL;
	if (m_droppingItem != NULL) {
		m_droppingItem->removeLayerKin();
		this->scene()->removeItem(m_droppingItem);
		delete m_droppingItem;
		m_droppingItem = NULL;
	}
}

ViewLayer::ViewLayerID SketchWidget::getViewLayerID(ModelPart * modelPart, ViewIdentifierClass::ViewIdentifier viewIdentifier, ViewLayer::ViewLayerSpec viewLayerSpec) {

	QDomElement layers = LayerAttributes::getSvgElementLayers(modelPart->domDocument(), viewIdentifier);
	if (layers.isNull()) return ViewLayer::UnknownLayer;

	QDomElement layer = layers.firstChildElement("layer");
	if (layer.isNull()) return ViewLayer::UnknownLayer;

	QString layerName = layer.attribute("layerId");
	int layerCount = 0;
	while (!layer.isNull()) {
		if (++layerCount > 1) break;

		layer = layer.nextSiblingElement("layer");
	}

	if (layerCount == 1) {
		return ViewLayer::viewLayerIDFromXmlString(layerName);
	}

	return multiLayerGetViewLayerID(modelPart, viewIdentifier, viewLayerSpec, layers, layerName);
}

ViewLayer::ViewLayerID SketchWidget::multiLayerGetViewLayerID(ModelPart * modelPart, ViewIdentifierClass::ViewIdentifier viewIdentifier, ViewLayer::ViewLayerSpec, QDomElement & layers, QString & layerName) {
	Q_UNUSED(modelPart);
	Q_UNUSED(layers);
	Q_UNUSED(viewIdentifier);

	return ViewLayer::viewLayerIDFromXmlString(layerName);
}

ItemBase * SketchWidget::overSticky(ItemBase * itemBase) {
	if (!itemBase->stickyEnabled()) return NULL;

	foreach (QGraphicsItem * item, scene()->collidingItems(itemBase)) {
		ItemBase * s = dynamic_cast<ItemBase *>(item);
		if (s == NULL) continue;
		if (s == itemBase) continue;
		if (!s->sticky()) continue;

		return s->layerKinChief();
	}

	return NULL;
}


void SketchWidget::stickem(long stickTargetID, long stickSourceID, bool stick) {
	ItemBase * stickTarget = findItem(stickTargetID);
	if (stickTarget == NULL) return;

	ItemBase * stickSource = findItem(stickSourceID);
	if (stickSource == NULL) return;

	stickTarget->addSticky(stickSource, stick);
	stickSource->addSticky(stickTarget, stick);
}

void SketchWidget::setChainDrag(bool chainDrag) {
	m_chainDrag = chainDrag;
}

void SketchWidget::stickyScoop(ItemBase * stickyOne, bool checkCurrent, CheckStickyCommand * checkStickyCommand) {
	// TODO: use the shape rather than the rect
	// need to find the best layerkin to use in that case
	//foreach (QGraphicsItem * item, scene()->collidingItems(stickyOne)) {

	QList<ItemBase *> added;
	QList<ItemBase *> already;
	QPolygonF poly = stickyOne->mapToScene(stickyOne->boundingRect());
	foreach (QGraphicsItem * item, scene()->items(poly)) {
		ItemBase * itemBase = dynamic_cast<ItemBase *>(item);
		if (itemBase == NULL) continue;

		itemBase = itemBase->layerKinChief();

		if (!itemBase->stickyEnabled()) continue;
		if (added.contains(itemBase)) continue;
		if (itemBase->sticky()) continue;
		if (stickyOne->alreadySticking(itemBase)) {
			already.append(itemBase);
			continue;
		}

		stickyOne->addSticky(itemBase, true);
		itemBase->addSticky(stickyOne, true);
		if (checkStickyCommand) {
			checkStickyCommand->stick(this, stickyOne->id(), itemBase->id(), true);
		}

		added.append(itemBase);
	}

	if (checkCurrent) {
		foreach (ItemBase * itemBase, stickyOne->stickyList()) {
			if (added.contains(itemBase)) continue;
			if (already.contains(itemBase)) continue;

			stickyOne->addSticky(itemBase, false);
			itemBase->addSticky(stickyOne, false);
			if (checkStickyCommand) {
				checkStickyCommand->stick(this, stickyOne->id(), itemBase->id(), false);
			}
		}
	}
}

void SketchWidget::wireSplitSlot(Wire* wire, QPointF newPos, QPointF oldPos, const QLineF & oldLine) {
	if (!canChainWire(wire)) return;

	this->clearHoldingSelectItem();
	this->m_moveEventCount = 0;  // clear this so an extra MoveItemCommand isn't posted

	QUndoCommand * parentCommand = new QUndoCommand();
	parentCommand->setText(QObject::tr("Split Wire") );

	new CleanUpWiresCommand(this, CleanUpWiresCommand::UndoOnly, parentCommand);

	long fromID = wire->id();

	QLineF newLine(oldLine.p1(), newPos - oldPos);

	long newID = ItemBase::getNextID();
	ViewGeometry vg(wire->getViewGeometry());
	vg.setLoc(newPos);
	QLineF newLine2(QPointF(0,0), oldLine.p2() + oldPos - newPos);
	vg.setLine(newLine2);

	BaseCommand::CrossViewType crossView = wireSplitCrossView();

	new AddItemCommand(this, crossView, ModuleIDNames::WireModuleIDName, wire->viewLayerSpec(), vg, newID, true, -1, parentCommand);
	new CheckStickyCommand(this, crossView, newID, false, CheckStickyCommand::RemoveOnly, parentCommand);
	new WireColorChangeCommand(this, newID, wire->colorString(), wire->colorString(), wire->opacity(), wire->opacity(), parentCommand);
	new WireWidthChangeCommand(this, newID, wire->width(), wire->width(), parentCommand);

	// disconnect from original wire and reconnect to new wire
	ConnectorItem * connector1 = wire->connector1();
	foreach (ConnectorItem * toConnectorItem, connector1->connectedToItems()) {
		new ChangeConnectionCommand(this, crossView, toConnectorItem->attachedToID(), toConnectorItem->connectorSharedID(),
			wire->id(), connector1->connectorSharedID(),
			ViewLayer::specFromID(wire->viewLayerID()),
			false, parentCommand);
		new ChangeConnectionCommand(this, crossView, toConnectorItem->attachedToID(), toConnectorItem->connectorSharedID(),
			newID, connector1->connectorSharedID(),
			ViewLayer::specFromID(wire->viewLayerID()),
			true, parentCommand);
	}

	new ChangeWireCommand(this, fromID, oldLine, newLine, oldPos, oldPos, true, false, parentCommand);

	// connect the two wires
	new ChangeConnectionCommand(this, crossView, wire->id(), connector1->connectorSharedID(),
								newID, "connector0", 
								ViewLayer::specFromID(wire->viewLayerID()),
								true, parentCommand);

	SelectItemCommand * selectItemCommand = new SelectItemCommand(this, SelectItemCommand::NormalSelect, parentCommand);
	selectItemCommand->addRedo(newID);

	new CleanUpWiresCommand(this, CleanUpWiresCommand::RedoOnly, parentCommand);

	m_undoStack->push(parentCommand);
}

void SketchWidget::wireJoinSlot(Wire* wire, ConnectorItem * clickedConnectorItem) {
	// if (!canChainWire(wire)) return;  // can't join a wire in some views (for now)

	this->clearHoldingSelectItem();
	this->m_moveEventCount = 0;  // clear this so an extra MoveItemCommand isn't posted

	QUndoCommand * parentCommand = new QUndoCommand();
	parentCommand->setText(QObject::tr("Join Wire") );

	new CleanUpWiresCommand(this, CleanUpWiresCommand::UndoOnly, parentCommand);

	// assumes there is one and only one item connected
	ConnectorItem * toConnectorItem = clickedConnectorItem->connectedToItems()[0];
	if (toConnectorItem == NULL) return;

	Wire * toWire = qobject_cast<Wire *>(toConnectorItem->attachedTo());
	if (toWire == NULL) return;

	if (wire->id() > toWire->id()) {
		// delete the wire with the higher id
		// so we can keep the three views in sync
		// i.e. the original wire has the lowest id in the chain
		Wire * wtemp = toWire;
		toWire = wire;
		wire = wtemp;
		ConnectorItem * ctemp = toConnectorItem;
		toConnectorItem = clickedConnectorItem;
		clickedConnectorItem = ctemp;
	}

	ConnectorItem * otherConnector = toWire->otherConnector(toConnectorItem);

	BaseCommand::CrossViewType crossView = BaseCommand::CrossView; // wireSplitCrossView();

	ChangeWireCurveCommand * cwcc = new ChangeWireCurveCommand(this, wire->id(), wire->curve(), NULL, parentCommand);
	cwcc->setUndoOnly();
	cwcc = new ChangeWireCurveCommand(this, toWire->id(), toWire->curve(), NULL, parentCommand);
	cwcc->setUndoOnly();


	// disconnect the wires
	new ChangeConnectionCommand(this, crossView, wire->id(), clickedConnectorItem->connectorSharedID(),
								toWire->id(), toConnectorItem->connectorSharedID(), 
								ViewLayer::specFromID(wire->viewLayerID()),
								false, parentCommand);

	// disconnect everyone from the other end of the wire being deleted, and reconnect to the remaining wire
	foreach (ConnectorItem * otherToConnectorItem, otherConnector->connectedToItems()) {
		new ChangeConnectionCommand(this, crossView, otherToConnectorItem->attachedToID(), otherToConnectorItem->connectorSharedID(),
			toWire->id(), otherConnector->connectorSharedID(),
			ViewLayer::specFromID(toWire->viewLayerID()),
			false, parentCommand);
		new ChangeConnectionCommand(this, crossView, otherToConnectorItem->attachedToID(), otherToConnectorItem->connectorSharedID(),
			wire->id(), clickedConnectorItem->connectorSharedID(),
			ViewLayer::specFromID(wire->viewLayerID()),
			true, parentCommand);
	}

	toWire->saveGeometry();
	makeDeleteItemCommand(toWire, crossView, parentCommand);

	Bezier b0, b1;
	QLineF newLine;
	QPointF newPos;
	if (otherConnector == toWire->connector1()) {
		newPos = wire->pos();
		newLine = QLineF(QPointF(0,0), toWire->pos() - wire->pos() + toWire->line().p2());
		b0.copy(wire->curve());
		b1.copy(toWire->curve());
		b0.set_endpoints(wire->line().p1(), wire->line().p2());
		b1.set_endpoints(toWire->line().p1(), toWire->line().p2());
		b1.translate(toWire->pos() - wire->pos());

	}
	else {
		newPos = toWire->pos();
		newLine = QLineF(QPointF(0,0), wire->pos() - toWire->pos() + wire->line().p2());
		b0.copy(toWire->curve());
		b1.copy(wire->curve());
		b0.set_endpoints(toWire->line().p1(), toWire->line().p2());
		b1.set_endpoints(wire->line().p1(), wire->line().p2());
		b1.translate(wire->pos() - toWire->pos());
	}
	new ChangeWireCommand(this, wire->id(), wire->line(), newLine, wire->pos(), newPos, true, false, parentCommand);
	Bezier joinBezier = b0.join(&b1);
	if (!joinBezier.isEmpty()) {
		ChangeWireCurveCommand * cwcc = new ChangeWireCurveCommand(this, wire->id(), wire->curve(), &joinBezier, parentCommand);
		cwcc->setRedoOnly();
	}

	new CleanUpWiresCommand(this, CleanUpWiresCommand::RedoOnly, parentCommand);

	m_undoStack->push(parentCommand);
}

void SketchWidget::hoverEnterItem(QGraphicsSceneHoverEvent * event, ItemBase * item) {
	if(m_infoViewOnHover || currentlyInfoviewed(item)) {
		InfoGraphicsView::hoverEnterItem(event, item);
	}

	Wire * wire = dynamic_cast<Wire *>(item);
	if (wire != NULL) {
		if (canChainWire(wire)) {	
			bool segment = wire->connector0()->chained() && wire->connector1()->chained();
			bool disconnected = wire->connector0()->connectionsCount() == 0 &&  wire->connector1()->connectionsCount() == 0;
			statusMessage(QString("%1 to add a bendpoint %2")
				.arg(disconnected ? tr("Double-click") : tr("Drag or double-click"))
				.arg(segment ? tr("or alt-drag to move the segment") : tr("")));
			m_lastHoverEnterItem = item;
		}
	}
}

void SketchWidget::statusMessage(QString message, int timeout) 
{
	// TODO: eventually do the connecting from the window not from the sketch
	QMainWindow * mainWindow = qobject_cast<QMainWindow *>(window());
	if (mainWindow == NULL) return;

	if (m_statusConnectState == StatusConnectNotTried) {
		bool result = connect(this, SIGNAL(statusMessageSignal(QString, int)),
							  mainWindow, SLOT(statusMessage(QString, int)));
		m_statusConnectState = (result) ? StatusConnectSucceeded : StatusConnectFailed;
	}

	if (m_statusConnectState == StatusConnectFailed) {
		QStatusBar * sb = mainWindow->statusBar();
		if (sb != NULL) {
			sb->showMessage(message, timeout);
		}
	}
	else {
		emit statusMessageSignal(message, timeout);
	}
}

void SketchWidget::hoverLeaveItem(QGraphicsSceneHoverEvent * event, ItemBase * item){
	m_lastHoverEnterItem = NULL;

	if(m_infoViewOnHover) {
		InfoGraphicsView::hoverLeaveItem(event, item);
	}

	if (canChainWire(qobject_cast<Wire *>(item))) {
		statusMessage(QString());
	}
}

void SketchWidget::hoverEnterConnectorItem(QGraphicsSceneHoverEvent * event, ConnectorItem * item) {
	if(m_infoViewOnHover || currentlyInfoviewed(item->attachedTo())) {
		InfoGraphicsView::hoverEnterConnectorItem(event, item);
	}

	if (item->attachedToItemType() == ModelPart::Wire) {
		if (!this->m_chainDrag) return;
		if (!item->chained()) return;

		m_lastHoverEnterConnectorItem = item;
		QString msg = hoverEnterWireConnectorMessage(event, item);
		statusMessage(msg);
	}
	else {
		QString msg = hoverEnterPartConnectorMessage(event, item);
		statusMessage(msg);
	}

}
const QString & SketchWidget::hoverEnterWireConnectorMessage(QGraphicsSceneHoverEvent * event, ConnectorItem * item)
{
	Q_UNUSED(event);
	Q_UNUSED(item);

	static QString message = tr("Double-click to delete this bend point");
	return message;
}


const QString & SketchWidget::hoverEnterPartConnectorMessage(QGraphicsSceneHoverEvent * event, ConnectorItem * item)
{
	Q_UNUSED(event);
	Q_UNUSED(item);

	return ___emptyString___;
}

void SketchWidget::hoverLeaveConnectorItem(QGraphicsSceneHoverEvent * event, ConnectorItem * item) {
	m_lastHoverEnterConnectorItem = NULL;

	ItemBase* attachedTo = item->attachedTo();
	if(attachedTo) {
		if(m_infoViewOnHover || currentlyInfoviewed(attachedTo)) {
			InfoGraphicsView::hoverLeaveConnectorItem(event, item);
			if(attachedTo->collidesWithItem(item)) {
				hoverEnterItem(event,attachedTo);
			}
		}
		attachedTo->hoverLeaveConnectorItem(event, item);
	}

	if (attachedTo->itemType() == ModelPart::Wire) {
		if (!this->m_chainDrag) return;
		if (!item->chained()) return;

		statusMessage(QString());
	}
	else {
		statusMessage(QString());
	}
}

bool SketchWidget::currentlyInfoviewed(ItemBase *item) {
	if(m_infoView) {
		ItemBase * currInfoView = m_infoView->currentItem();
		return !currInfoView || item == currInfoView;//->cu selItems.size()==1 && selItems[0] == item;
	}
	return false;
}

void SketchWidget::cleanUpWires(bool doEmit, CleanUpWiresCommand * command) {
	RoutingStatus routingStatus;
	updateRoutingStatus(command, routingStatus, false);

	if (doEmit) {
		emit cleanUpWiresSignal(command);
	}
}

void SketchWidget::cleanUpWiresSlot(CleanUpWiresCommand * command) {
	RoutingStatus routingStatus;
	updateRoutingStatus(command, routingStatus, false);
}

void SketchWidget::noteChanged(ItemBase * item, const QString &oldText, const QString & newText, QSizeF oldSize, QSizeF newSize) {
	ChangeNoteTextCommand * command = new ChangeNoteTextCommand(this, item->id(), oldText, newText, oldSize, newSize, NULL);
	command->setText(tr("Change note to '%2'").arg(newText));
	m_undoStack->waitPush(command, PropChangeDelay);
}

void SketchWidget::partLabelChanged(ItemBase * pitem,const QString & oldText, const QString &newText) {
	// partLabelChanged triggered from inline editing the label

	if (!m_current) {
		// all three views get the partLabelChanged call, but only need to act on this once
		return;
	}

	//if (currentlyInfoviewed(pitem))  {
		// TODO: just change the affected item in the info view
		//InfoGraphicsView::viewItemInfo(pitem);
	//}

	partLabelChangedAux(pitem, oldText, newText);
}

void SketchWidget::partLabelChangedAux(ItemBase * pitem,const QString & oldText, const QString &newText)
{
	if (pitem == NULL) return;

	ChangeLabelTextCommand * command = new ChangeLabelTextCommand(this, pitem->id(), oldText, newText, NULL);
	command->setText(tr("Change %1 label to '%2'").arg(pitem->title()).arg(newText));
	m_undoStack->waitPush(command, PropChangeDelay);
}

void SketchWidget::setInfoViewOnHover(bool infoViewOnHover) {
	m_infoViewOnHover = infoViewOnHover;
}

void SketchWidget::updateInfoView() {
	QTimer::singleShot(50, this,  SLOT(updateInfoViewSlot()));
}

void SketchWidget::updateInfoViewSlot() {
	foreach (QGraphicsItem * item, scene()->selectedItems())
	{
		ItemBase * itemBase = dynamic_cast<ItemBase *>(item);
		if (itemBase == NULL) continue;

		ItemBase * chief = itemBase->layerKinChief();
		InfoGraphicsView::viewItemInfo(chief);
		return;
	}

	InfoGraphicsView::viewItemInfo(m_lastPaletteItemSelected);
}

long SketchWidget::setUpSwap(ItemBase * itemBase, long newModelIndex, const QString & newModuleID, ViewLayer::ViewLayerSpec viewLayerSpec, 
								bool master, bool noFinalChangeWiresCommand, QList<Wire *> & wiresToDelete, QUndoCommand * parentCommand)
{
	long newID = ItemBase::getNextID(newModelIndex);
	if (itemBase->viewIdentifier() != m_viewIdentifier) {
		itemBase = findItem(itemBase->id());
		if (itemBase == NULL) return newID;
	}

	ViewGeometry vg = itemBase->getViewGeometry();
	QTransform oldTransform = vg.transform();
	bool needsTransform = false;
	if (!oldTransform.isIdentity()) {
		// restore identity transform
		vg.setTransform(QTransform());
		needsTransform = true;
	}

	new MoveItemCommand(this, itemBase->id(), vg, vg, false, parentCommand);

	// command created for each view
	newAddItemCommand(BaseCommand::SingleView, newModuleID, viewLayerSpec, vg, newID, true, newModelIndex, parentCommand);

	if (needsTransform) {
		QMatrix m;
		m.setMatrix(oldTransform.m11(), oldTransform.m12(), oldTransform.m21(), oldTransform.m22(), 0, 0);
		new TransformItemCommand(this, newID, m, m, parentCommand);
	}

	setUpSwapReconnect(itemBase, newID, newModuleID, master, wiresToDelete, parentCommand);
	new CheckStickyCommand(this, BaseCommand::SingleView, newID, false, CheckStickyCommand::RemoveOnly, parentCommand);
	if (itemBase->isPartLabelVisible()) {
		ShowLabelCommand * slc = new ShowLabelCommand(this, parentCommand);
		slc->add(newID, true, true);
	}

	if (master) {		
		SelectItemCommand * selectItemCommand = new SelectItemCommand(this, SelectItemCommand::NormalSelect, parentCommand);
		selectItemCommand->addRedo(newID);
		selectItemCommand->addUndo(itemBase->id());

		new ChangeLabelTextCommand(this, itemBase->id(), itemBase->instanceTitle(), itemBase->instanceTitle(), parentCommand);
		new ChangeLabelTextCommand(this, newID, itemBase->instanceTitle(), itemBase->instanceTitle(), parentCommand);

		foreach (Wire * wire, wiresToDelete) {
			QList<Wire *> chained;
			QList<ConnectorItem *> ends;
			wire->collectChained(chained, ends);
			foreach (Wire * w, chained) {
				if (!wiresToDelete.contains(w)) wiresToDelete.append(w);
			}
		}

		makeWiresChangeConnectionCommands(wiresToDelete, parentCommand);
		foreach (Wire * wire, wiresToDelete) {
			makeDeleteItemCommand(wire, BaseCommand::CrossView, parentCommand);
		}

		makeDeleteItemCommand(itemBase, BaseCommand::CrossView, parentCommand);
		selectItemCommand = new SelectItemCommand(this, SelectItemCommand::NormalSelect, parentCommand);
		selectItemCommand->addRedo(newID);  // to make sure new item is selected so it appears in the info view

		prepDeleteProps(itemBase, newID, newModuleID, parentCommand);
		if (!noFinalChangeWiresCommand) {
			new CleanUpWiresCommand(this, CleanUpWiresCommand::RedoOnly, parentCommand);
		}
	}

	return newID;
}

void SketchWidget::setUpSwapReconnect(ItemBase* itemBase, long newID, const QString & newModuleID, bool master, QList<Wire *> & wiresToDelete, QUndoCommand * parentCommand)
{
	ModelPart * newModelPart = m_refModel->retrieveModelPart(newModuleID);
	if (newModelPart == NULL) return;

	QList<ConnectorItem *> fromConnectorItems(itemBase->cachedConnectorItems());

	newModelPart->initConnectors();			//  make sure the connectors are set up
	QList< QPointer<Connector> > newConnectors = newModelPart->connectors().values();

	QList<ConnectorItem *> notFound;
	QList<ConnectorItem *> other;
	QHash<ConnectorItem *, Connector *> found;
	QHash<ConnectorItem *, ConnectorItem *> m2f;

	foreach (ConnectorItem * fromConnectorItem, fromConnectorItems) {
		QList<Connector *> candidates;
		Connector * newConnector = NULL;

		if (fromConnectorItem->connectorType() == Connector::Male) {
			foreach (ConnectorItem * toConnectorItem, fromConnectorItem->connectedToItems()) {
				if (toConnectorItem->connectorType() == Connector::Female) {
					m2f.insert(fromConnectorItem, toConnectorItem);
					break;
				}
			}
		}

		// matching by connectorid can lead to weird results because these all just usually count up from zero
		// so only match by name and description (the latter is a bit of a hail mary)

		QString fromName = fromConnectorItem->connectorSharedName();
		QString fromDescription = fromConnectorItem->connectorSharedDescription();
		foreach (Connector * connector, newConnectors) {
			QString toName = connector->connectorSharedName();
			QString toDescription = connector->connectorSharedDescription();
			if (fromName.compare(toName, Qt::CaseInsensitive) == 0) {
				candidates.append(connector);
			}
			else if (fromDescription.compare(toDescription, Qt::CaseInsensitive) == 0) {
				candidates.append(connector);
			}
			else if (fromDescription.compare(toName, Qt::CaseInsensitive) == 0) {
				candidates.append(connector);
			}
			else if (fromName.compare(toDescription, Qt::CaseInsensitive) == 0) {
				candidates.append(connector);
			}
		}

		if (candidates.count() > 0) {
			newConnector = candidates[0];
			if (candidates.count() > 1) {
				foreach (Connector * connector, candidates) {
					// this gets an exact match, if there is one
					if (fromConnectorItem->connectorSharedID().compare(connector->connectorSharedID(), Qt::CaseInsensitive) == 0) {					
						newConnector = connector;
						break;
					}
				}
			}

			newConnectors.removeOne(newConnector);
			found.insert(fromConnectorItem, newConnector);
			fromConnectorItem = fromConnectorItem->getCrossLayerConnectorItem();
			if (fromConnectorItem) {
				other.append(fromConnectorItem);
				found.insert(fromConnectorItem, newConnector);
			}
		}
		else {
			notFound.append(fromConnectorItem);
		}
	}

	QHash<ConnectorItem *, Connector *> byWire;
	QHash<QString, QPolygonF> legs;
	QHash<QString, ConnectorItem *> formerLegs;
	if (master && m2f.count() > 0 && (m_viewIdentifier == ViewIdentifierClass::BreadboardView)) {
		checkFit(newModelPart, itemBase, newID, found, notFound, m2f, byWire, legs, formerLegs, parentCommand);
	}

	fromConnectorItems.append(other);
	foreach (ConnectorItem * fromConnectorItem, fromConnectorItems) {
		Connector * newConnector = found.value(fromConnectorItem, NULL);
		foreach (ConnectorItem * toConnectorItem, fromConnectorItem->connectedToItems()) {
			// delete connection to part being swapped out
			Wire * wire = qobject_cast<Wire *>(toConnectorItem->attachedTo());
			if (wire != NULL) {
				if (wire->getRatsnest()) continue;

				if (newConnector == NULL && wire->getTrace() && wire->isTraceType(getTraceFlag())) {
					wiresToDelete.append(wire);
					continue;
				}
			}

			// command created for each view
			extendChangeConnectionCommand(BaseCommand::SingleView,
										fromConnectorItem, toConnectorItem,
										ViewLayer::specFromID(toConnectorItem->attachedToViewLayerID()),
										false, parentCommand);

			bool cleanup = false;
			if (newConnector) {
				cleanup = (byWire.value(fromConnectorItem, NULL) == newConnector) || swappedGender(fromConnectorItem, newConnector);
			}
			if ((newConnector == NULL) || cleanup) {
					// clean up after disconnect
			}
			else {
				// reconnect directly without cleaning up; command created for each view
				new ChangeConnectionCommand(this, BaseCommand::SingleView,
											newID, newConnector->connectorSharedID(),
											toConnectorItem->attachedToID(), toConnectorItem->connectorSharedID(),
											ViewLayer::specFromID(toConnectorItem->attachedToViewLayerID()),
											true, parentCommand);

			}

			if (cleanup && master) {
				long wireID = ItemBase::getNextID();
				ViewGeometry vg;
				new AddItemCommand(this, BaseCommand::CrossView, ModuleIDNames::WireModuleIDName, itemBase->viewLayerSpec(), vg, wireID, false, -1, parentCommand);
				new CheckStickyCommand(this, BaseCommand::CrossView, wireID, false, CheckStickyCommand::RemoveOnly, parentCommand);
				new ChangeConnectionCommand(this, BaseCommand::CrossView, newID, newConnector->connectorSharedID(),
											wireID, "connector0", 
											ViewLayer::specFromID(toConnectorItem->attachedToViewLayerID()),
											true, parentCommand);
				new ChangeConnectionCommand(this, BaseCommand::CrossView, toConnectorItem->attachedToID(), toConnectorItem->connectorSharedID(),
											wireID, "connector1", 
											ViewLayer::specFromID(toConnectorItem->attachedToViewLayerID()),
											true, parentCommand);
			}
		}
	}

	// changeConnection calls PaletteItemBase::connectedMoved which repositions the new part
	// so slam in the desired position
        QPointF p = itemBase->getViewGeometry().loc();
        new SimpleMoveItemCommand(this, newID, p, p, parentCommand);

	foreach (QString connectorID, legs.keys()) {
		// must be invoked after all the connections have been dealt with
		QPolygonF poly = legs.value(connectorID);

		ConnectorItem * connectorItem = formerLegs.value(connectorID, NULL);
		if (connectorItem && connectorItem->hasRubberBandLeg()) {
			poly = connectorItem->leg();
		}

		ChangeLegCommand * clc = new ChangeLegCommand(this, newID, connectorID, poly, poly, true, true, "swap", parentCommand);
		clc->setRedoOnly();

		if (connectorItem && connectorItem->hasRubberBandLeg()) {
			QVector<Bezier *> beziers = connectorItem->beziers();
			for (int i = 0; i < beziers.count() - 1; i++) {
				Bezier * bezier = beziers.at(i);
				if (bezier == NULL) continue;
				if (bezier->isEmpty()) continue;

				ChangeLegCurveCommand * clcc = new ChangeLegCurveCommand(this, newID, connectorID, i, bezier, bezier, parentCommand);
				clcc->setRedoOnly();
			}
		}
	}
}

void SketchWidget::checkFit(ModelPart * newModelPart, ItemBase * itemBase, long newID,
							QHash<ConnectorItem *, Connector *> & found, QList<ConnectorItem *> & notFound,
							QHash<ConnectorItem *, ConnectorItem *> & m2f, QHash<ConnectorItem *, Connector *> & byWire, 
							QHash<QString, QPolygonF> & legs, QHash<QString, ConnectorItem *> & formerLegs, QUndoCommand * parentCommand)
{
	if (found.count() == 0) return;

	ItemBase * tempItemBase = addItemAuxTemp(newModelPart, itemBase->viewLayerSpec(), itemBase->getViewGeometry(), newID, NULL, true, m_viewIdentifier, true);
	if (tempItemBase == NULL) return;			// we're really screwed 

	checkFitAux(tempItemBase, itemBase, newID, found, notFound, m2f, byWire, legs, formerLegs, parentCommand);
	delete tempItemBase;
}

void SketchWidget::checkFitAux(ItemBase * tempItemBase, ItemBase * itemBase, long newID,
							QHash<ConnectorItem *, Connector *> & found, QList<ConnectorItem *> & notFound,
							QHash<ConnectorItem *, ConnectorItem *> & m2f, QHash<ConnectorItem *, Connector *> & byWire, 
							QHash<QString, QPolygonF> & legs, QHash<QString, ConnectorItem *> & formerLegs, QUndoCommand * parentCommand)
{
	QPointF foundAnchor(std::numeric_limits<int>::max(), std::numeric_limits<int>::max());
	QPointF newAnchor(std::numeric_limits<int>::max(), std::numeric_limits<int>::max());
	QHash<ConnectorItem *, QPointF> foundPoints;
	QHash<ConnectorItem *, QPointF> newPoints;
	QHash<ConnectorItem *, ConnectorItem *> foundNews;
	QHash<ConnectorItem *, ConnectorItem *> newFounds;
	QList<ConnectorItem *> removeFromFound;

	foreach (ConnectorItem * foundConnectorItem, found.keys()) {
		if (!m2f.value(foundConnectorItem, false)) {
			// we only care about replacing the female connectors here

			continue;
		}

		Connector * connector = found.value(foundConnectorItem);
		ConnectorItem * newConnectorItem = NULL;
		foreach (ConnectorItem * nci, tempItemBase->cachedConnectorItems()) {
			if (nci->connector()->connectorShared() == connector->connectorShared()) {
				newConnectorItem = nci;
				break;
			}
		}
		if (newConnectorItem == NULL) {
			removeFromFound.append(foundConnectorItem);
		}
		else {
			foundNews.insert(foundConnectorItem, newConnectorItem);
			newFounds.insert(newConnectorItem, foundConnectorItem);

			QPointF lastNew = newConnectorItem->sceneAdjustedTerminalPoint(NULL);
			if (lastNew.x() < newAnchor.x()) newAnchor.setX(lastNew.x());
			if (lastNew.y() < newAnchor.y()) newAnchor.setY(lastNew.y());
			newPoints.insert(newConnectorItem, lastNew);
			QPointF lastFound = foundConnectorItem->sceneAdjustedTerminalPoint(NULL);
			if (lastFound.x() < foundAnchor.x()) foundAnchor.setX(lastFound.x());
			if (lastFound.y() < foundAnchor.y()) foundAnchor.setY(lastFound.y());
			foundPoints.insert(foundConnectorItem, lastFound);
		}
	}

	foreach (ConnectorItem * connectorItem, removeFromFound) {
		found.remove(connectorItem);
		notFound.append(connectorItem);
	}

	if (found.count() == 0) {
		return;
	}


	bool allCorrespond = true;
	foreach (ConnectorItem * foundConnectorItem, foundNews.keys()) {
		QPointF fp = foundPoints.value(foundConnectorItem) - foundAnchor;
		ConnectorItem * newConnectorItem = foundNews.value(foundConnectorItem);
		QPointF np = newPoints.value(newConnectorItem) - newAnchor;
		if (!newConnectorItem->hasRubberBandLeg() && (qAbs(fp.x() - np.x()) >= CloseEnough || qAbs(fp.y() - np.y()) >= CloseEnough)) {
			// pins can be off by a little
			// but if even one connector is out of place, hook everything up by wires
			allCorrespond = false;
			break;
		}
	}

	if (allCorrespond) {
		if (tempItemBase->cachedConnectorItems().count() == found.count()) {
			if (tempItemBase->hasRubberBandLeg()) {
				foreach (ConnectorItem * connectorItem, tempItemBase->cachedConnectorItems()) {
					legs.insert(connectorItem->connectorSharedID(), connectorItem->leg());
					formerLegs.insert(connectorItem->connectorSharedID(), newFounds.value(connectorItem, NULL));
				}
			}

			// it's a clean swap: all connectors line up
			return;
		}
	}

	// there's a mismatch in terms of connector count between the swaps, 
	// so make sure all target connections are open or to be swapped out
	// establish the location of the new item's connectors

	QHash<ConnectorItem *, ConnectorItem *> newConnections;

	if (allCorrespond) {
		QList<ConnectorItem *> alreadyFits = foundNews.values();
		foreach (ConnectorItem * nci, tempItemBase->cachedConnectorItems()) {
			if (alreadyFits.contains(nci)) continue;
			if (nci->connectorType() != Connector::Male) continue;

			// TODO: this doesn't handle all scenarios.  For example, if another part with a female connector is
			// on top of the breadboard, and would be in the way

			QPointF p = nci->sceneAdjustedTerminalPoint(NULL) - newAnchor + foundAnchor;			// eventual position of this new connector
			ConnectorItem * connectorUnder = NULL;
			foreach (QGraphicsItem * item, scene()->items(p)) {
				ConnectorItem * cu = dynamic_cast<ConnectorItem *>(item);
				if (cu == NULL || cu == nci || cu->attachedTo() == itemBase || cu->connectorType() != Connector::Female) {
					continue;
				}

				connectorUnder = cu;
				break;
			}

			if (connectorUnder == NULL) {
				// safe?
				continue;
			}

			foreach (ConnectorItem * uct, connectorUnder->connectedToItems()) {
				if (uct->attachedTo() == itemBase) {
					// we're safe, itemBase is swapping out
					continue;
				}

				// some other part is in the way
				allCorrespond = false;
				break;
			}

			if (!allCorrespond) break;

			newConnections.insert(nci, connectorUnder);			// add a new direct connection

		}
	}

	if (allCorrespond) {
		// the extra new connectors will also fit
		foreach (ConnectorItem * nci, newConnections.keys()) {
			ConnectorItem * toConnectorItem = newConnections.value(nci);
			new ChangeConnectionCommand(this, BaseCommand::CrossView,
									newID, nci->connectorSharedID(),
									toConnectorItem->attachedToID(), toConnectorItem->connectorSharedID(),
									ViewLayer::specFromID(toConnectorItem->attachedToViewLayerID()),
									true, parentCommand);

		}
		if (tempItemBase->hasRubberBandLeg()) {
			foreach (ConnectorItem * connectorItem, newConnections) {
				legs.insert(connectorItem->connectorSharedID(), connectorItem->leg());
			}
			foreach (ConnectorItem * newConnectorItem, newFounds.keys()) {
				legs.insert(newConnectorItem->connectorSharedID(), newConnectorItem->leg());
				formerLegs.insert(newConnectorItem->connectorSharedID(), newFounds.value(newConnectorItem, NULL));
			}
		}

		return;
	}

	// have to replace each found value with a wire
	foreach (ConnectorItem * fci, found.keys()) {
		byWire.insert(fci, found.value(fci));
	}
}

void SketchWidget::changeWireColor(const QString newColor)
{
	m_lastColorSelected = newColor;
	QList <Wire *> wires;
	foreach (QGraphicsItem * item, scene()->selectedItems()) {
		Wire * wire = dynamic_cast<Wire *>(item);
		if (wire == NULL) continue;

		wires.append(wire);
	}

	if (wires.count() <= 0) return;

	QString commandString;
	if (wires.count() == 1) {
		commandString = tr("Change %1 color from %2 to %3")
			.arg(wires[0]->instanceTitle())
			.arg(wires[0]->colorString())
			.arg(newColor);
	}
	else {
		commandString = tr("Change color of %1 wires to %2")
			.arg(wires.count())
			.arg(newColor);
	}

	QUndoCommand* parentCommand = new QUndoCommand(commandString);
	foreach (Wire * wire, wires) {
		QList<Wire *> subWires;
		wire->collectWires(subWires);

		foreach (Wire * subWire, subWires) {
			new WireColorChangeCommand(
					this,
					subWire->id(),
					subWire->colorString(),
					newColor,
					subWire->opacity(),
					subWire->opacity(),
					parentCommand);
		}
	}

	m_undoStack->waitPush(parentCommand, PropChangeDelay);
}

void SketchWidget::changeWireWidthMils(const QString newWidthStr)
{
	bool ok = false;
	double newWidth = newWidthStr.toDouble(&ok);
	if (!ok) return;

	double trueWidth = FSvgRenderer::printerScale() * newWidth / 1000.0;

	QList <Wire *> wires;
	foreach (QGraphicsItem * item, scene()->selectedItems()) {
		Wire * wire = dynamic_cast<Wire *>(item);
		if (wire == NULL) continue;
		if (!wire->getTrace()) continue;

		wires.append(wire);
	}

	if (wires.count() <= 0) return;

	QString commandString;
	if (wires.count() == 1) {
		commandString = tr("Change %1 width from %2 to %3")
			.arg(wires[0]->instanceTitle())
			.arg((int) wires[0]->mils())
			.arg(newWidth);
	}
	else {
		commandString = tr("Change width of %1 wires to %2")
			.arg(wires.count())
			.arg(newWidth);
	}

	QUndoCommand* parentCommand = new QUndoCommand(commandString);
	foreach (Wire * wire, wires) {
		QList<Wire *> subWires;
		wire->collectWires(subWires);

		foreach (Wire * subWire, subWires) {
			new WireWidthChangeCommand(
					this,
					subWire->id(),
					subWire->width(),
					trueWidth,
					parentCommand);
		}
	}

	m_undoStack->waitPush(parentCommand, PropChangeDelay);
}

void SketchWidget::changeWireColor(long wireId, const QString& color, double opacity) {
	ItemBase *item = findItem(wireId);
	Wire* wire = qobject_cast<Wire*>(item);
	if (wire) {
		wire->setColorString(color, opacity);
		updateInfoView();
	}
}

void SketchWidget::changeWireWidth(long wireId, double width) {
	ItemBase *item = findItem(wireId);
	Wire* wire = qobject_cast<Wire*>(item);
	if (wire) {
		wire->setWireWidth(width, this, getWireStrokeWidth(wire, width));
		updateInfoView();
	}
}

PaletteModel * SketchWidget::paletteModel() {
	return m_paletteModel;
}

bool SketchWidget::swappingEnabled(ItemBase * itemBase) {
	if (itemBase == NULL) {
		return m_refModel->swapEnabled();
	}

	return (m_refModel->swapEnabled() && itemBase->isSwappable());
}

void SketchWidget::resizeEvent(QResizeEvent * event) {
	InfoGraphicsView::resizeEvent(event);

	QPoint s(event->size().width(), event->size().height());
	QPointF p = this->mapToScene(s);

	QPointF z = this->mapToScene(QPoint(0,0));

//	DebugDialog::debug(QString("resize event %1 %2, %3 %4, %5 %6")		/* , %3 %4 %5 %6, %7 %8 %9 %10 */
//		.arg(event->size().width()).arg(event->size().height())
//		.arg(p.x()).arg(p.y())
//		.arg(z.x()).arg(z.y())
//		.arg(sr.left()).arg(sr.top()).arg(sr.width()).arg(sr.height())
//		.arg(sr.left()).arg(sr.top()).arg(ir.width()).arg(ir.height())
//			
//	);

	if (m_sizeItem != NULL) {
		m_sizeItem->setLine(z.x(), z.y(), p.x(), p.y()); 
	}

	emit resizeSignal();
}

void SketchWidget::addBreadboardViewLayers() {
	setViewLayerIDs(ViewLayer::Breadboard, ViewLayer::BreadboardWire, ViewLayer::Breadboard, ViewLayer::BreadboardRuler, ViewLayer::BreadboardNote);
	addViewLayersAux(ViewIdentifierClass::layersForView(ViewIdentifierClass::BreadboardView));
}

void SketchWidget::addSchematicViewLayers() {
	setViewLayerIDs(ViewLayer::Schematic, ViewLayer::SchematicTrace, ViewLayer::Schematic, ViewLayer::SchematicRuler, ViewLayer::SchematicNote);
	addViewLayersAux(ViewIdentifierClass::layersForView(ViewIdentifierClass::SchematicView));
}

void SketchWidget::addPcbViewLayers() {
	setViewLayerIDs(ViewLayer::Silkscreen1, ViewLayer::Copper0Trace, ViewLayer::Copper0, ViewLayer::PcbRuler, ViewLayer::PcbNote);
	addViewLayersAux(ViewIdentifierClass::layersForView(ViewIdentifierClass::PCBView));

	ViewLayer * silkscreen1 = m_viewLayers.value(ViewLayer::Silkscreen1);
	ViewLayer * silkscreen1Label = m_viewLayers.value(ViewLayer::Silkscreen1Label);
	if (silkscreen1 && silkscreen1Label) {
		silkscreen1Label->setParentLayer(silkscreen1);
	}
	ViewLayer * silkscreen0 = m_viewLayers.value(ViewLayer::Silkscreen0);
	ViewLayer * silkscreen0Label = m_viewLayers.value(ViewLayer::Silkscreen0Label);
	if (silkscreen0 && silkscreen0Label) {
		silkscreen0Label->setParentLayer(silkscreen0);
	}

	ViewLayer * copper0 = m_viewLayers.value(ViewLayer::Copper0);
	ViewLayer * copper0Trace = m_viewLayers.value(ViewLayer::Copper0Trace);
	ViewLayer * copper1 = m_viewLayers.value(ViewLayer::Copper1);
	ViewLayer * copper1Trace = m_viewLayers.value(ViewLayer::Copper1Trace);
	if (copper0 && copper0Trace) {
		copper0Trace->setParentLayer(copper0);
	}
	ViewLayer * groundPlane0 = m_viewLayers.value(ViewLayer::GroundPlane0);
	if (copper0 && groundPlane0) {
		groundPlane0->setParentLayer(copper0);
	}
	if (copper1 && copper1Trace) {
		copper1Trace->setParentLayer(copper1);
	}
	ViewLayer * groundPlane1 = m_viewLayers.value(ViewLayer::GroundPlane1);
	if (copper1 && groundPlane1) {
		groundPlane1->setParentLayer(copper1);
	}
}

void SketchWidget::addViewLayers() {
}

void SketchWidget::addViewLayersAux(const LayerList &layers, float startZ) {
	m_z = startZ;
	foreach(ViewLayer::ViewLayerID vlId, layers) {
		addViewLayer(new ViewLayer(vlId, true, m_z));
		m_z += 1;
	}
}

void SketchWidget::setIgnoreSelectionChangeEvents(bool ignore) {
	if (ignore) {
		m_ignoreSelectionChangeEvents++;
	}
	else {
		m_ignoreSelectionChangeEvents--;
	}
}

void SketchWidget::hideConnectors(bool hide) {
	foreach (QGraphicsItem * item, scene()->items()) {
		ItemBase * itemBase = dynamic_cast<ItemBase *>(item);
		if (itemBase == NULL) continue;
		if (!itemBase->isVisible()) continue;

		foreach (ConnectorItem * connectorItem, itemBase->cachedConnectorItems()) {
			connectorItem->setVisible(!hide);
		}
	}
}

void SketchWidget::saveLayerVisibility()
{
	m_viewLayerVisibility.clear();
	foreach (ViewLayer::ViewLayerID viewLayerID, m_viewLayers.keys()) {
		ViewLayer * viewLayer = m_viewLayers.value(viewLayerID);
		if (viewLayer == NULL) continue;

		m_viewLayerVisibility.insert(viewLayerID, viewLayer->visible());
	}
}

void SketchWidget::restoreLayerVisibility()
{
	foreach (ViewLayer::ViewLayerID viewLayerID, m_viewLayerVisibility.keys()) {
		setLayerVisible(m_viewLayers.value(viewLayerID),  m_viewLayerVisibility.value(viewLayerID), false);
	}
}

void SketchWidget::changeWireFlags(long wireId, ViewGeometry::WireFlags wireFlags)
{
	ItemBase *item = findItem(wireId);
	if(Wire* wire = qobject_cast<Wire*>(item)) {
		wire->setWireFlags(wireFlags);
	}
}

bool SketchWidget::disconnectFromFemale(ItemBase * item, QHash<long, ItemBase *> & savedItems, ConnectorPairHash & connectorHash, bool doCommand, bool rubberBandLegEnabled, QUndoCommand * parentCommand)
{
	// schematic and pcb view connections are always via wires so this is a no-op.  breadboard view has its own version.

	Q_UNUSED(item);
	Q_UNUSED(savedItems);
	Q_UNUSED(parentCommand);
	Q_UNUSED(connectorHash);
	Q_UNUSED(doCommand);
	Q_UNUSED(rubberBandLegEnabled);
	return false;
}

void SketchWidget::spaceBarIsPressedSlot(bool isPressed) {
	if (m_middleMouseIsPressed) return;

	m_spaceBarIsPressed = isPressed;
	if (isPressed) {
		setDragMode(QGraphicsView::ScrollHandDrag);
		//setInteractive(false);
		setCursor(Qt::OpenHandCursor);
	}
	else {
		setDragMode(QGraphicsView::RubberBandDrag);
		//setInteractive(true);
		setCursor(Qt::ArrowCursor);
	}
}

void SketchWidget::updateRoutingStatus(CleanUpWiresCommand* command, RoutingStatus & routingStatus, bool manual)
{
	//DebugDialog::debug("update ratsnest status");


	routingStatus.zero();
	updateRoutingStatus(routingStatus, manual);

	if (routingStatus != m_routingStatus) {
		if (command) {
			// changing state after the command has already been executed
			command->addRoutingStatus(this, m_routingStatus, routingStatus);
		}

		emit routingStatusSignal(this, routingStatus);

		m_routingStatus = routingStatus;
	}
}

void SketchWidget::updateRoutingStatus(RoutingStatus & routingStatus, bool manual) 
{
	//DebugDialog::debug(QString("update routing status %1 %2 %3")
	//	.arg(m_viewIdentifier) 
	//	.arg(m_ratsnestUpdateConnect.count())
	//	.arg(m_ratsnestUpdateDisconnect.count())
	//	);

	// TODO: think about ways to optimize this...

	QList< QPointer<VirtualWire> > ratsToDelete;

	QList< QList<ConnectorItem *> > ratnestsToUpdate;
	QList<ConnectorItem *> visited;
	foreach (QGraphicsItem * item, scene()->items()) {
		ConnectorItem * connectorItem = dynamic_cast<ConnectorItem *>(item);
		if (connectorItem == NULL) continue;
		if (visited.contains(connectorItem)) continue;

		//connectorItem->debugInfo("testing urs");

		VirtualWire * vw = qobject_cast<VirtualWire *>(connectorItem->attachedTo());
		if (vw != NULL) {
			if (vw->connector0()->connectionsCount() == 0 || vw->connector1()->connectionsCount() == 0) {
				ratsToDelete.append(vw);
			}
			visited << vw->connector0() << vw->connector1();
			continue;  
		}


		QList<ConnectorItem *> connectorItems;
		connectorItems.append(connectorItem);
		ConnectorItem::collectEqualPotential(connectorItems, true, ViewGeometry::RatsnestFlag);
		visited.append(connectorItems);

		//if (this->viewIdentifier() == ViewIdentifierClass::PCBView) {
		//	DebugDialog::debug("________________________");
		//	foreach (ConnectorItem * ci, connectorItems) ci->debugInfo("cep");
		//}

		bool doRatsnest = manual || checkUpdateRatsnest(connectorItems);
		if (!doRatsnest && connectorItems.count() <= 1) continue;

		QList<ConnectorItem *> partConnectorItems;
		ConnectorItem::collectParts(connectorItems, partConnectorItems, includeSymbols(), ViewLayer::TopAndBottom);
		if (partConnectorItems.count() < 1) continue;
		if (!doRatsnest && partConnectorItems.count() <= 1) continue;

		//DebugDialog::debug("________________________");
		for (int i = partConnectorItems.count() - 1; i >= 0; i--) {
			ConnectorItem * ci = partConnectorItems[i];
			
			//if (this->viewIdentifier() == ViewIdentifierClass::PCBView) {
				//ci->debugInfo("pc");
			//}

			if (!ci->attachedTo()->isEverVisible()) {
				partConnectorItems.removeAt(i);
				// ci->debugInfo("ever visible check");
			}
		}

		if (partConnectorItems.count() < 1) continue;

		if (doRatsnest) {
			ratnestsToUpdate.append(partConnectorItems);
		}

		if (partConnectorItems.count() <= 1) continue;

		GraphUtils::scoreOneNet(partConnectorItems, this->getTraceFlag(), routingStatus);
	}

	routingStatus.m_jumperItemCount /= 4;			// since we counted each connector twice on two layers (4 connectors per jumper item)

	// can't do this in the above loop since VirtualWires and ConnectorItems are added and deleted
	foreach (QList<ConnectorItem *> partConnectorItems, ratnestsToUpdate) {
		//partConnectorItems.at(0)->debugInfo("display ratsnest");
		partConnectorItems.at(0)->displayRatsnest(partConnectorItems, this->getTraceFlag());
	}

	foreach(QPointer<VirtualWire> vw, ratsToDelete) {
		if (vw != NULL) {
			vw->debugInfo("removing rat 2");
			vw->scene()->removeItem(vw);
			delete vw;
		}
	}


	m_ratsnestUpdateConnect.clear();
	m_ratsnestUpdateDisconnect.clear();

        /*
        // uncomment for live drc
        CMRouter cmRouter(this);
        QString message;
        bool result = cmRouter.drc(message);
        */
}


void SketchWidget::ensureLayerVisible(ViewLayer::ViewLayerID viewLayerID)
{
	ViewLayer * viewLayer = m_viewLayers.value(viewLayerID, NULL);
	if (viewLayer == NULL) return;

	if (!viewLayer->visible()) {
		setLayerVisible(viewLayer, true, true);
	}
}

void SketchWidget::clearDragWireTempCommand()
{
	if (m_tempDragWireCommand) {
		delete m_tempDragWireCommand;
		m_tempDragWireCommand = NULL;
	}
}

void SketchWidget::autoScrollTimeout()
{
	//DebugDialog::debug(QString("scrolling dx:%1 dy:%2").arg(m_autoScrollX).arg(m_autoScrollY) );

	if (m_autoScrollX == 0 && m_autoScrollY == 0 ) return;


	if (m_autoScrollX != 0) {
		QScrollBar * h = horizontalScrollBar();
		h->setValue(m_autoScrollX + h->value());
	}
	if (m_autoScrollY != 0) {
		QScrollBar * v = verticalScrollBar();
		v->setValue(m_autoScrollY + v->value());
	}

	//DebugDialog::debug(QString("autoscrolling %1 %2").arg(m_autoScrollX).arg(m_autoScrollX));
}

void SketchWidget::dragAutoScrollTimeout()
{
	autoScrollTimeout();
	dragMoveHighlightConnector(mapFromGlobal(m_globalPos));
}


void SketchWidget::moveAutoScrollTimeout()
{
	autoScrollTimeout();
	moveItems(m_globalPos, true, m_rubberBandLegWasEnabled);
}

const QString &SketchWidget::selectedModuleID() {
	if(m_lastPaletteItemSelected) {
		return m_lastPaletteItemSelected->moduleID();
	}
	return ___emptyString___;
}

BaseCommand::CrossViewType SketchWidget::wireSplitCrossView()
{
	return BaseCommand::CrossView;
}

bool SketchWidget::canDeleteItem(QGraphicsItem * item, int count)
{
	Q_UNUSED(count);
	ItemBase * itemBase = dynamic_cast<ItemBase *>(item);
	if (itemBase == NULL) return false;

	if (itemBase->moveLock()) return false;

	ItemBase * chief = itemBase->layerKinChief();
	if (chief == NULL) return false;

	if (chief->moveLock()) return false;

	return true;
}

bool SketchWidget::canCopyItem(QGraphicsItem * item, int count)
{
	Q_UNUSED(count);
	ItemBase * itemBase = dynamic_cast<ItemBase *>(item);
	if (itemBase == NULL) return false;

	ItemBase * chief = itemBase->layerKinChief();
	if (chief == NULL) return false;

	return true;
}

bool SketchWidget::canChainMultiple() {
	return false;
}

bool SketchWidget::canChainWire(Wire * wire) {
	if (!this->m_chainDrag) return false;
	if (wire == NULL) return false;

	return true;
}

bool SketchWidget::canDragWire(Wire * wire) {
	if (wire == NULL) return false;

	return true;
}

bool SketchWidget::canCreateWire(Wire * dragWire, ConnectorItem * from, ConnectorItem * to)
{
	Q_UNUSED(dragWire);
	Q_UNUSED(from);
	Q_UNUSED(to);
	return true;
}

/*
bool SketchWidget::modifyNewWireConnections(Wire * dragWire, ConnectorItem * fromOnWire, ConnectorItem * from, ConnectorItem * to, QUndoCommand * parentCommand)
{
	Q_UNUSED(dragWire);
	Q_UNUSED(fromOnWire);
	Q_UNUSED(from);
	Q_UNUSED(to);
	Q_UNUSED(parentCommand);
	return false;
}
*/

void SketchWidget::setupAutoscroll(bool moving) {
	m_autoScrollX = m_autoScrollY = 0;
	m_autoScrollThreshold = (moving) ? MoveAutoScrollThreshold : DragAutoScrollThreshold;
	m_autoScrollCount = 0;
	connect(&m_autoScrollTimer, SIGNAL(timeout()), this,
		moving ? SLOT(moveAutoScrollTimeout()) : SLOT(dragAutoScrollTimeout()));
	//DebugDialog::debug("set up autoscroll");
}

void SketchWidget::turnOffAutoscroll() {
	m_autoScrollTimer.stop();
	disconnect(&m_autoScrollTimer, SIGNAL(timeout()), this, SLOT(moveAutoScrollTimeout()));
	disconnect(&m_autoScrollTimer, SIGNAL(timeout()), this, SLOT(dragAutoScrollTimeout()));
	//DebugDialog::debug("turn off autoscroll");

}

bool SketchWidget::checkAutoscroll(QPoint globalPos)
{
	QRect r = rect();
	QPoint q = mapFromGlobal(globalPos);

	if (verticalScrollBar()->isVisible()) {
		r.setWidth(width() - verticalScrollBar()->width());
	}

	if (horizontalScrollBar()->isVisible()) {
		r.setHeight(height() - horizontalScrollBar()->height());
	}

	if (!r.contains(q)) {
		m_autoScrollX = m_autoScrollY = 0;
		if (m_autoScrollCount < m_autoScrollThreshold) {
			m_autoScrollCount = 0;
		}
		return false;
	}

	//DebugDialog::debug(QString("check autoscroll %1, %2 %3").arg(QTime::currentTime().msec()).arg(q.x()).arg(q.y()) );

	r.adjust(16,16,-16,-16);						// these should be set someplace
	bool autoScroll = !r.contains(q);
	if (autoScroll) {
		if (++m_autoScrollCount < m_autoScrollThreshold) {
			m_autoScrollX = m_autoScrollY = 0;
			//DebugDialog::debug("in autoscrollThreshold");
			return true;
		}

		if (m_clearSceneRect) {
			scene()->setSceneRect(QRectF());
			m_clearSceneRect = true;
		}

		int dx = 0, dy = 0;
		if (q.x() > r.right()) {
			dx = q.x() - r.right();
		}
		else if (q.x() < r.left()) {
			dx = q.x() - r.left();
		}
		if (q.y() > r.bottom()) {
			dy = q.y() - r.bottom();
		}
		else if (q.y() < r.top()) {
			dy = q.y() - r.top();
		}

		int div = 3;
		if (dx != 0) {
			m_autoScrollX = (dx + ((dx > 0) ? div : -div)) / (div + 1);		// ((m_autoScrollX > 0) ? 1 : -1)
		}
		if (dy != 0) {
			m_autoScrollY = (dy + ((dy > 0) ? div : -div)) / (div + 1);		// ((m_autoScrollY > 0) ? 1 : -1)
		}

		if (!m_autoScrollTimer.isActive()) {
			//DebugDialog::debug("starting autoscroll timer");
			m_autoScrollTimer.start(10);
		}

	}
	else {
		m_autoScrollX = m_autoScrollY = 0;
		if (m_autoScrollCount < m_autoScrollThreshold) {
			m_autoScrollCount = 0;
		}
	}

	//DebugDialog::debug(QString("autoscroll %1 %2").arg(m_autoScrollX).arg(m_autoScrollY) );
	return true;
}

void SketchWidget::setWireVisible(Wire * wire) {
	Q_UNUSED(wire);
}

void SketchWidget::forwardRoutingStatus(const RoutingStatus & routingStatus) {

	emit routingStatusSignal(this, routingStatus);
}

bool SketchWidget::matchesLayer(ModelPart * modelPart) {
	QDomDocument * domDocument = modelPart->domDocument();
	if (domDocument->isNull()) return false;

	QDomElement views = domDocument->documentElement().firstChildElement("views");
	if(views.isNull()) return false;

	QDomElement view = views.firstChildElement(ViewIdentifierClass::viewIdentifierXmlName(m_viewIdentifier));
	if (view.isNull()) return false;

	QDomElement layers = view.firstChildElement("layers");
	if (layers.isNull()) return false;

	QDomElement layer = layers.firstChildElement("layer");
	while (!layer.isNull()) {
		QString layerName = layer.attribute("layerId");
		ViewLayer::ViewLayerID viewLayerID = ViewLayer::viewLayerIDFromXmlString(layerName);
		foreach (ViewLayer* viewLayer, m_viewLayers) {
			if (viewLayer == NULL) continue;

			if (viewLayer->viewLayerID() == viewLayerID) return true;
		}

		layer = layer.nextSiblingElement("layer");
	}

	return false;
}

const QString & SketchWidget::viewName() {
	return m_viewName;
}

void SketchWidget::setNoteText(long itemID, const QString & newText) {
	ItemBase * itemBase = findItem(itemID);
	if (itemBase == NULL) return;

	Note * note = qobject_cast<Note *>(itemBase);
	if (note == NULL) return;

	note->setText(newText, false);
}

void SketchWidget::incInstanceTitle(long itemID) {
	ItemBase * itemBase = findItem(itemID);
	if (itemBase) {
		itemBase->ensureUniqueTitle(itemBase->instanceTitle(), true);
	}

	emit updatePartLabelInstanceTitleSignal(itemID);
}

void SketchWidget::updatePartLabelInstanceTitleSlot(long itemID) {
	ItemBase * itemBase = findItem(itemID);
	if (itemBase) {
		itemBase->updatePartLabelInstanceTitle();
	}
}

void SketchWidget::setInstanceTitle(long itemID, const QString & newText, bool isUndoable, bool doEmit) {
	// isUndoable is true when setInstanceTitle is called from the infoview 
	ItemBase * itemBase = findItem(itemID);
	if (itemBase == NULL) return;

	QString oldText = itemBase->instanceTitle();
	if (!isUndoable) {
		itemBase->setInstanceTitle(newText);
		if (doEmit && currentlyInfoviewed(itemBase))  {
			// TODO: just change the affected item in the info view
			InfoGraphicsView::viewItemInfo(itemBase);
		}

		if (doEmit) {
			emit setInstanceTitleSignal(itemID, newText, isUndoable, false);
		}
	}
	else {
		if (oldText.compare(newText) == 0) return;

		partLabelChangedAux(itemBase, oldText, newText);
	}
}

void SketchWidget::showPartLabel(long itemID, bool showIt) {

	ItemBase * itemBase = findItem(itemID);
	if (itemBase != NULL) {
		itemBase->showPartLabel(showIt, m_viewLayers.value(getLabelViewLayerID(itemBase->viewLayerSpec())));
	}
}

void SketchWidget::hidePartLabel(ItemBase * item) {
	QList <ItemBase *> itemBases;
	itemBases.append(item);
	showPartLabelsAux(false, itemBases);
}


void SketchWidget::collectParts(QList<ItemBase *> & partList) {
	foreach (QGraphicsItem * item, scene()->items()) {
		PaletteItem * pitem = dynamic_cast<PaletteItem *>(item);
		if (pitem == NULL) continue;
		if (pitem->itemType() == ModelPart::Symbol) continue;

		partList.append(pitem);
	}
}

void SketchWidget::movePartLabel(long itemID, QPointF newPos, QPointF newOffset)
{
	ItemBase * item = findItem(itemID);
	if (item == NULL) return;

	item->movePartLabel(newPos, newOffset);
}

void SketchWidget::setCurrent(bool current) {
	m_current = current;
}

void SketchWidget::partLabelMoved(ItemBase * itemBase, QPointF oldPos, QPointF oldOffset, QPointF newPos, QPointF newOffset)
{
	MoveLabelCommand * command = new MoveLabelCommand(this, itemBase->id(), oldPos, oldOffset, newPos, newOffset, NULL);
	command->setText(tr("Move label '%1'").arg(itemBase->title()));
	m_undoStack->push(command);
}


void SketchWidget::rotateFlipPartLabel(ItemBase * itemBase, double degrees, Qt::Orientations flipDirection) {
	RotateFlipLabelCommand * command = new RotateFlipLabelCommand(this, itemBase->id(), degrees, flipDirection, NULL);
	command->setText(tr("%1 label '%2'").arg((degrees != 0) ? tr("Rotate") : tr("Flip")).arg(itemBase->title()));
	m_undoStack->push(command);
}


void SketchWidget::rotateFlipPartLabel(long itemID, double degrees, Qt::Orientations flipDirection) {
	ItemBase * itemBase = findItem(itemID);
	if (itemBase == NULL) return;

	itemBase->doRotateFlipPartLabel(degrees, flipDirection);
}


void SketchWidget::showPartLabels(bool show)
{
	QList<ItemBase *> itemBases;
	foreach (QGraphicsItem * item, scene()->selectedItems()) {
		ItemBase * itemBase = ItemBase::extractTopLevelItemBase(item);
		if (itemBase == NULL) continue;

		if (itemBase->hasPartLabel()) {
			itemBases.append(itemBase);
		}
	}

	if (itemBases.count() <= 0) return;

	showPartLabelsAux(show, itemBases);
}

void SketchWidget::showPartLabelsAux(bool show, QList<ItemBase *> & itemBases)
{
	ShowLabelCommand * showLabelCommand = new ShowLabelCommand(this, NULL);
	QString text;
	if (show) {
		text = tr("show part label(s)", "", itemBases.count());
	}
	else {
		text = tr("hide part label(s)", "", itemBases.count());
	}
	showLabelCommand->setText(text);

	foreach (ItemBase * itemBase, itemBases) {
		showLabelCommand->add(itemBase->id(), itemBase->isPartLabelVisible(), show);
	}

	m_undoStack->push(showLabelCommand);
}

void SketchWidget::noteSizeChanged(ItemBase * itemBase, const QSizeF & oldSize, const QSizeF & newSize)
{
	ResizeNoteCommand * command = new ResizeNoteCommand(this, itemBase->id(), oldSize, newSize, NULL);
	command->setText(tr("Resize Note"));
	m_undoStack->push(command);
	clearHoldingSelectItem();
}

void SketchWidget::resizeNote(long itemID, const QSizeF & size)
{
	Note * note = qobject_cast<Note *>(findItem(itemID));
	if (note == NULL) return;

	note->setSize(size);
}

QString SketchWidget::renderToSVG(double printerScale, const LayerList & layers, 
								  bool blackOnly, QSizeF & imageSize, ItemBase * board, double dpi, 
								  bool selectedItems, bool flatten, bool fillHoles, bool & empty)
{
	QRectF offsetRect;
	if (board) {
		offsetRect = board->sceneBoundingRect();
	}
	return renderToSVG(printerScale, layers, blackOnly, imageSize, offsetRect, dpi, selectedItems, flatten, fillHoles, empty);
}


QString SketchWidget::renderToSVG(double printerScale, const LayerList & layers, 
								  bool blackOnly, QSizeF & imageSize, QRectF & offsetRect, double dpi, 
								  bool selectedItems, bool flatten, bool fillHoles, bool & empty)
{

	QList<QGraphicsItem *> itemsAndLabels;
	QRectF itemsBoundingRect;
	foreach (QGraphicsItem * item, (selectedItems) ? scene()->selectedItems() : scene()->items()) {
		ItemBase * itemBase = dynamic_cast<ItemBase *>(item);
		if (itemBase == NULL) continue;
		if (itemBase->hidden()) continue;

		if (itemBase == itemBase->layerKinChief() && itemBase->isPartLabelVisible()) {
			if (layers.contains(itemBase->partLabelViewLayerID())) {
				itemsAndLabels.append(itemBase->partLabel());
				itemsBoundingRect |= itemBase->partLabelSceneBoundingRect();
			}
		}

		if (!itemBase->isVisible()) continue;
		if (!layers.contains(itemBase->viewLayerID())) continue;

		itemsAndLabels.append(itemBase);
		itemsBoundingRect |= item->sceneBoundingRect();
	}

	return renderToSVG(printerScale, blackOnly, imageSize, offsetRect, dpi, flatten, fillHoles, itemsAndLabels, itemsBoundingRect, empty);
}

QString translateSVG(QString & svg, QPointF loc, double dpi, double printerScale) {
	loc.setX(loc.x() * dpi / printerScale);
	loc.setY(loc.y() * dpi / printerScale);
	if (loc.x() != 0 || loc.y() != 0) {
		svg = QString("<g transform='translate(%1,%2)' >%3</g>")
			.arg(loc.x())
			.arg(loc.y())
			.arg(svg);
	}
	return svg;
}

QString SketchWidget::renderToSVG(double printerScale, bool blackOnly, QSizeF & imageSize, QRectF & offsetRect, double dpi, bool flatten,
								  bool fillHoles, QList<QGraphicsItem *> & itemsAndLabels, QRectF itemsBoundingRect,
								  bool & empty)
{
	Q_UNUSED(fillHoles);
	empty = true;

	double width = itemsBoundingRect.width();
	double height = itemsBoundingRect.height();
	QPointF offset = itemsBoundingRect.topLeft();

	if (!offsetRect.isEmpty()) {
		offset = offsetRect.topLeft();
		width = offsetRect.width();
		height = offsetRect.height();
	}

	imageSize.setWidth(width);
	imageSize.setHeight(height);

	QString outputSVG = TextUtils::makeSVGHeader(printerScale, dpi, width, height);

	QHash<QString, QString> svgHash;

	// put them in z order
	qSort(itemsAndLabels.begin(), itemsAndLabels.end(), zLessThan);

	QList<ItemBase *> gotLabel;
	foreach (QGraphicsItem * item, itemsAndLabels) {
		ItemBase * itemBase = dynamic_cast<ItemBase *>(item);
		if (itemBase == NULL) {
			PartLabel * partLabel = dynamic_cast<PartLabel *>(item);
			if (partLabel == NULL) continue;

			QString labelSvg = partLabel->owner()->makePartLabelSvg(blackOnly, dpi, printerScale);
			if (labelSvg.isEmpty()) continue;

			empty = false;
			outputSVG.append(translateSVG(labelSvg, partLabel->owner()->partLabelScenePos() - offset, dpi, printerScale));
			continue;
		}
			
		if (itemBase->itemType() != ModelPart::Wire) {
			QString itemSvg = itemBase->retrieveSvg(itemBase->viewLayerID(), svgHash, blackOnly, dpi);
			if (itemSvg.isEmpty()) continue;

			if (flatten) {
				QDomDocument domDocument;
				QString errorStr;
				int errorLine;
				int errorColumn;
				bool result = domDocument.setContent(itemSvg, &errorStr, &errorLine, &errorColumn);
				if (!result) continue;

				QDomElement root = domDocument.documentElement();
				SvgFlattener flattener;
				flattener.flattenChildren(root);
				SvgFileSplitter::fixStyleAttributeRecurse(root);
				itemSvg = domDocument.toString();
			}
			if (false && fillHoles) {
				QDomDocument domDocument;
				QString errorStr;
				int errorLine;
				int errorColumn;
				bool result = domDocument.setContent(itemSvg, &errorStr, &errorLine, &errorColumn);
				if (!result) continue;

				QDomNodeList circleList = domDocument.elementsByTagName("circle");
				for(uint i = 0; i < circleList.length(); i++) {
					QDomElement circle = circleList.item(i).toElement();
					QString fill = circle.attribute("fill");
					if (fill.isEmpty() || fill.compare("none") == 0) {
						circle.setAttribute("fill", circle.attribute("stroke"));
					}
				}
				itemSvg = domDocument.toString();
			}

			foreach (ConnectorItem * ci, itemBase->cachedConnectorItems()) {
				if (!ci->hasRubberBandLeg()) continue;

				outputSVG.append(ci->makeLegSvg(offset, dpi, printerScale, blackOnly));
			}

            QTransform t = itemBase->transform();
            itemSvg = TextUtils::svgTransform(itemSvg, t, false, QString());
			outputSVG.append(translateSVG(itemSvg, itemBase->scenePos() - offset, dpi, printerScale));
			empty = false;

			/*
			// TODO:  deal with rotations and flips
			QString shifted = splitter->shift(loc.x(), loc.y(), xmlName);
			outputSVG.append(shifted);
			empty = false;
			splitter->shift(-loc.x(), -loc.y(), xmlName);
			*/
		}
		else {
			Wire * wire = qobject_cast<Wire *>(itemBase);
			if (wire == NULL) continue;

			//if (wire->getTrace()) {
			//	DebugDialog::debug(QString("trace %1 %2,%3 %4,%5")
			//		.arg(wire->id()) 
			//		.arg(wire->line().p1().x())
			//		.arg(wire->line().p1().y())
			//		.arg(wire->line().p2().x())
			//		.arg(wire->line().p2().y())
			//		);
			//}

			outputSVG.append(makeWireSVG(wire, offset, dpi, printerScale, blackOnly));
			empty = false;
		}
		extraRenderSvgStep(itemBase, offset, dpi, printerScale, outputSVG);
	}

	outputSVG += "</svg>";

	return outputSVG;

}

void SketchWidget::extraRenderSvgStep(ItemBase * itemBase, QPointF offset, double dpi, double printerScale, QString & outputSvg)
{
	Q_UNUSED(itemBase);
	Q_UNUSED(offset);
	Q_UNUSED(dpi);
	Q_UNUSED(printerScale);
	Q_UNUSED(outputSvg);
}

QString SketchWidget::makeWireSVG(Wire * wire, QPointF offset, double dpi, double printerScale, bool blackOnly) 
{
	QString shadow;
	if (wire->hasShadow()) {
		shadow = makeWireSVGAux(wire, wire->shadowWidth(), wire->shadowHexString(), offset, dpi, printerScale, blackOnly);
	}

	return shadow + makeWireSVGAux(wire, wire->width(), wire->hexString(), offset, dpi, printerScale, blackOnly);
}

QString SketchWidget::makeWireSVGAux(Wire * wire, double width, const QString & color, QPointF offset, double dpi, double printerScale, bool blackOnly) 
{
	if (wire->isCurved()) {
		QPolygonF poly = wire->sceneCurve(offset);
		return TextUtils::makeCubicBezierSVG(poly, width, color, dpi, printerScale, blackOnly);
	}
	else {
		QLineF line = wire->getPaintLine();
		QPointF p1 = wire->scenePos() + line.p1() - offset;
		QPointF p2 = wire->scenePos() + line.p2() - offset;
		return TextUtils::makeLineSVG(p1, p2, width, color, dpi, printerScale, blackOnly);
	}
}

void SketchWidget::addFixedToCenterItem2(SketchMainHelp * item) {
	m_fixedToCenterItem = item;
}

void SketchWidget::drawBackground( QPainter * painter, const QRectF & rect )
{
	InfoGraphicsView::drawBackground(painter, rect);

	// from:  http://www.qtcentre.org/threads/27364-Trying-to-draw-a-grid-on-a-QGraphicsScene-View
	
	InfoGraphicsView::drawForeground(painter, rect);

	if (m_showGrid) {
		QColor gridColor(0, 0, 0, 55);
		double gridSize = m_gridSizeInches * FSvgRenderer::printerScale();

		painter->save();
		double left = int(rect.left() * 10000) - (int(rect.left() * 10000) % int(gridSize * 10000));
		left /= 10000;
		double top = int(rect.top() * 10000) - (int(rect.top() * 10000) % int(gridSize * 10000));
		top /= 10000;
 
		QVarLengthArray<QLineF, 100> linesX;
		for (double x = left; x < rect.right(); x += gridSize) {
			linesX.append(QLineF(x, rect.top(), x, rect.bottom()));
		}

		QVarLengthArray<QLineF, 100> linesY;
		for (double  y = top; y < rect.bottom(); y += gridSize) {
			linesY.append(QLineF(rect.left(), y, rect.right(), y));
		}

		QPen pen;
		pen.setColor(gridColor);
		pen.setWidth(0);
		pen.setCosmetic(true);
		//pen.setStyle(Qt::DotLine);
		QVector<double> dashes;
		dashes << 1 << 1;
		pen.setDashPattern(dashes);
		painter->setPen(pen);
		painter->drawLines(linesX.data(), linesX.size());
		painter->drawLines(linesY.data(), linesY.size());
		painter->restore();
	}

	// always draw the widget in the same place in the window
	// no matter how the view is zoomed or scrolled

	if (m_fixedToCenterItem != NULL) {
		if (m_fixedToCenterItem->getVisible()) {
			QWidget * widget = m_fixedToCenterItem->widget();
			if (widget != NULL) {
				QSizeF helpSize = m_fixedToCenterItem->size();

				/*
				// add in scrollbar widths so image doesn't jump when scroll bars appear or disappear?
				if (verticalScrollBar()->isVisible()) {
					vp.setWidth(vp.width() + verticalScrollBar()->width());
				}
				if (horizontalScrollBar()->isVisible()) {
					vp.setHeight(vp.height() + horizontalScrollBar()->height());
				}
				*/
			
				m_fixedToCenterItemOffset = calcFixedToCenterItemOffset(painter->viewport(), helpSize);

				painter->save();
				painter->setWindow(painter->viewport());
				painter->setTransform(QTransform());
				painter->drawPixmap(m_fixedToCenterItemOffset, m_fixedToCenterItem->getPixmap());
				//painter->fillRect(m_fixedToCenterItemOffset.x(), m_fixedToCenterItemOffset.y(), helpsize.width(), helpsize.height(), QBrush(QColor(Qt::blue)));
				painter->restore();
			}
		}
	}
}

QPoint SketchWidget::calcFixedToCenterItemOffset(const QRect & viewPortRect, const QSizeF & helpSize) {
	QPoint p((int) ((viewPortRect.width() - helpSize.width()) / 2.0),
			 (int) ((viewPortRect.height() - helpSize.height()) / 2.0));
	return p;
}

void SketchWidget::pushCommand(QUndoCommand * command) {
	if (m_undoStack) {
		m_undoStack->push(command);
	}
}

bool SketchWidget::spaceBarIsPressed() {
	return m_spaceBarIsPressed || m_middleMouseIsPressed;
}

ViewLayer::ViewLayerID SketchWidget::defaultConnectorLayer(ViewIdentifierClass::ViewIdentifier viewId) {
	switch(viewId) {
		case ViewIdentifierClass::IconView: return ViewLayer::Icon;
		case ViewIdentifierClass::BreadboardView: return ViewLayer::Breadboard;
		case ViewIdentifierClass::SchematicView: return ViewLayer::Schematic;
		case ViewIdentifierClass::PCBView: return ViewLayer::Copper0;
		default: return ViewLayer::UnknownLayer;
	}
}

bool SketchWidget::swappedGender(ConnectorItem * connectorItem, Connector * newConnector) 
{
	return (connectorItem->connectorType() != newConnector->connectorType());
}

void SketchWidget::setLastPaletteItemSelected(PaletteItem * paletteItem)
{
	m_lastPaletteItemSelected = paletteItem;
	//DebugDialog::debug(QString("m_lastPaletteItemSelected:%1 %2").arg(paletteItem == NULL ? "NULL" : paletteItem->instanceTitle()).arg(m_viewIdentifier));
}

void SketchWidget::setLastPaletteItemSelectedIf(ItemBase * itemBase)
{
	PaletteItem * paletteItem = qobject_cast<PaletteItem *>(itemBase);
	if (paletteItem == NULL) return;

	setLastPaletteItemSelected(paletteItem);
}

void SketchWidget::setSpacing(const QString & spacing) {
	PaletteItem * item = getSelectedPart();
	if (item == NULL) return;

	MysteryPart * mysteryPart = qobject_cast<MysteryPart *>(item);
	if (mysteryPart == NULL) return;

	SetPropCommand * cmd = new SetPropCommand(this, item->id(), "spacing", mysteryPart->spacing(), spacing, true, NULL);
	cmd->setText(tr("Change pin spacing from %1 to %2").arg(mysteryPart->spacing()).arg(spacing));
	m_undoStack->waitPush(cmd, PropChangeDelay);
}

void SketchWidget::setForm(const QString & form) {
	PaletteItem * item = getSelectedPart();
	if (item == NULL) return;

	PinHeader * pinHeader = qobject_cast<PinHeader *>(item);
	if (pinHeader == NULL) return;

	SetPropCommand * cmd = new SetPropCommand(this, item->id(), "form", pinHeader->form(), form, true, NULL);
	cmd->setText(tr("Change form from %1 to %2").arg(pinHeader->form()).arg(form));
	m_undoStack->waitPush(cmd, PropChangeDelay);
}

void SketchWidget::setResistance(QString resistance, QString pinSpacing)
{
	PaletteItem * item = getSelectedPart();
	if (item == NULL) return;

	ModelPart * modelPart = item->modelPart();

	if (!modelPart->moduleID().endsWith(ModuleIDNames::ResistorModuleIDName)) return;

	Resistor * resistor = qobject_cast<Resistor *>(item);
	if (resistor == NULL) return;

	if (resistance.isEmpty()) {
		resistance = resistor->resistance();
	}

	if (pinSpacing.isEmpty()) {
		pinSpacing = resistor->pinSpacing();
	}

	SetResistanceCommand * cmd = new SetResistanceCommand(this, item->id(), resistor->resistance(), resistance, resistor->pinSpacing(), pinSpacing, NULL);
	cmd->setText(tr("Change Resistance from %1 to %2").arg(resistor->resistance()).arg(resistance));
	m_undoStack->waitPush(cmd, PropChangeDelay);
}

void SketchWidget::setResistance(long itemID, QString resistance, QString pinSpacing, bool doEmit) {
	ItemBase * item = findItem(itemID);
	if (item == NULL) return;

	Resistor * ritem = qobject_cast<Resistor *>(item);
	if (ritem == NULL) return;

	ritem->setResistance(resistance, pinSpacing, false);
	viewItemInfo(item);

	if (doEmit) {
		emit setResistanceSignal(itemID, resistance, pinSpacing, false);
	}
}

void SketchWidget::setProp(ItemBase * item, const QString & prop, const QString & trProp, const QString & oldValue, const QString & newValue, bool redraw)
{
	if (oldValue.isEmpty() && newValue.isEmpty()) return;

	SetPropCommand * cmd = new SetPropCommand(this, item->id(), prop, oldValue, newValue, redraw, NULL);
	cmd->setText(tr("Change %1 from %2 to %3").arg(trProp).arg(oldValue).arg(newValue));
	// unhook triggered action from originating widget event
    m_undoStack->waitPush(cmd, PropChangeDelay);
}

void SketchWidget::setHoleSize(ItemBase * item, const QString & prop, const QString & trProp, const QString & oldValue, const QString & newValue, QRectF & oldRect, QRectF & newRect, bool redraw)
{
	if (oldValue.isEmpty() && newValue.isEmpty()) return;

	QUndoCommand * parentCommand = new QUndoCommand(tr("Change %1 from %2 to %3").arg(trProp).arg(oldValue).arg(newValue));	
	new SetPropCommand(this, item->id(), prop, oldValue, newValue, redraw, parentCommand);
	item->saveGeometry();
	ViewGeometry vg(item->getViewGeometry());
	QPointF p(vg.loc().x() + (oldRect.width() / 2) - (newRect.width() / 2), 
				vg.loc().y() + (oldRect.height() / 2) - (newRect.height() / 2));
	vg.setLoc(p);
	new MoveItemCommand(this, item->id(), item->getViewGeometry(), vg, false, parentCommand);
        //DebugDialog::debug("set hole", oldRect);
        //DebugDialog::debug("        ", newRect);
        //DebugDialog::debug("        ", item->getViewGeometry().loc());
        //DebugDialog::debug("        ", p);
    m_undoStack->waitPush(parentCommand, PropChangeDelay);
}

void SketchWidget::setProp(long itemID, const QString & prop, const QString & value, bool redraw, bool doEmit) {
	ItemBase * item = findItem(itemID);
	if (item == NULL) return;

	item->setProp(prop, value);
	if (redraw) {
		viewItemInfo(item);
	}

	if (doEmit) {
		emit setPropSignal(itemID, prop, value, false, false);
	}
}

// called from ResizeBoardCommand
void SketchWidget::resizeBoard(long itemID, double mmW, double mmH) {
	ItemBase * item = findItem(itemID);
	if (item == NULL) return;

	switch (item->itemType()) {
		case ModelPart::ResizableBoard:
			qobject_cast<ResizableBoard *>(item)->resizeMM(mmW, mmH, m_viewLayers);
			return;

		case ModelPart::Logo:
			qobject_cast<LogoItem *>(item)->resizeMM(mmW, mmH, m_viewLayers);
			return;

		case ModelPart::Ruler:
			qobject_cast<Ruler *>(item)->resizeMM(mmW, mmH, m_viewLayers);
			return;
	}

	Pad * pad = qobject_cast<Pad *>(item);
	if (pad) {
		pad->resizeMM(mmW, mmH, m_viewLayers);
	}
}

void SketchWidget::resizeBoard(double mmW, double mmH, bool doEmit)
{
	Q_UNUSED(doEmit);
	Q_UNUSED(mmH);

	PaletteItem * item = getSelectedPart();
	if (item == NULL) {
		return InfoGraphicsView::resizeBoard(mmW, mmH, doEmit);
	}

	switch (item->itemType()) {
		case ModelPart::Ruler:
			break;
		default:
			return;
	}

	QString orig = item->prop("width");
	QString temp = orig;
	temp.chop(2);
	double origw = temp.toDouble();
	double origh = orig.endsWith("cm") ? 0 : 1;
	QUndoCommand * parentCommand = new QUndoCommand(tr("Resize ruler to %1%2").arg(mmW).arg((mmH == 0) ? "cm" : "in"));
	new ResizeBoardCommand(this, item->id(), origw, origh, mmW, mmH, parentCommand);
	m_undoStack->waitPush(parentCommand, PropChangeDelay);
}

void SketchWidget::addBendpoint(ItemBase * lastHoverEnterItem, ConnectorItem * lastHoverEnterConnectorItem, QPointF lastLocation) {
	if (lastHoverEnterConnectorItem) {
		Wire * wire = qobject_cast<Wire *>(lastHoverEnterConnectorItem->attachedTo());
		if (wire != NULL) {
			wireJoinSlot(wire, lastHoverEnterConnectorItem);
		}
	}
	else if (lastHoverEnterItem) {
		Wire * wire = qobject_cast<Wire *>(lastHoverEnterItem);
		if (wire != NULL) {
			wireSplitSlot(wire, lastLocation, wire->pos(), wire->line());
		}
	}
}

void SketchWidget::flattenCurve(ItemBase * lastHoverEnterItem, ConnectorItem * lastHoverEnterConnectorItem, QPointF lastLocation) {
	Q_UNUSED(lastLocation);
	Wire * wire = NULL;
	if (lastHoverEnterConnectorItem) {
		wire = qobject_cast<Wire *>(lastHoverEnterConnectorItem->attachedTo());
	}

	if (wire == NULL && lastHoverEnterItem) {
		wire = qobject_cast<Wire *>(lastHoverEnterItem);
	}

	if (wire != NULL) {
		wireChangedCurveSlot(wire, wire->curve(), NULL, true);
	}

}

ConnectorItem * SketchWidget::lastHoverEnterConnectorItem() {
	return m_lastHoverEnterConnectorItem;
}

ItemBase * SketchWidget::lastHoverEnterItem() {
	return m_lastHoverEnterItem;
}

LayerHash & SketchWidget::viewLayers() {
	return m_viewLayers;
}

void SketchWidget::setClipEnds(ClipableWire * vw, bool) {
	vw->setClipEnds(false);
}

void SketchWidget::createTrace(Wire * wire) {
	QString commandString = tr("Create wire from Ratsnest");
	createTrace(wire, commandString, getTraceFlag());
}

void SketchWidget::createTrace(Wire * fromWire, const QString & commandString, ViewGeometry::WireFlag flag)
{
	QList<Wire *> done;
	QUndoCommand * parentCommand = new QUndoCommand(commandString);

	new CleanUpWiresCommand(this, CleanUpWiresCommand::UndoOnly, parentCommand);

	bool gotOne = false;
	if (fromWire == NULL) {
		foreach (QGraphicsItem * item, scene()->selectedItems()) {
			Wire * wire = dynamic_cast<Wire *>(item);
			if (wire == NULL) continue;
			if (done.contains(wire)) continue;

			gotOne = createOneTrace(wire, flag, false, done, parentCommand);
		}
	}
	else {
		gotOne = createOneTrace(fromWire, flag, false, done, parentCommand);
	}

	if (!gotOne) {
		delete parentCommand;
		return;
	}

	new CleanUpWiresCommand(this, CleanUpWiresCommand::RedoOnly, parentCommand);
	m_undoStack->push(parentCommand);
}


bool SketchWidget::createOneTrace(Wire * wire, ViewGeometry::WireFlag flag, bool allowAny, QList<Wire *> & done, QUndoCommand * parentCommand) 
{
	QList<ConnectorItem *> ends;
	Wire * trace = NULL;
	if (wire->getRatsnest()) {
		trace = wire->findTraced(getTraceFlag(), ends);
	}
	else if (wire->isTraceType(getTraceFlag())) {
		trace = wire;
	}
	else if (!allowAny) {
		// not eligible
		return false;
	}
	else {
		trace = wire->findTraced(getTraceFlag(), ends);
	}

	if (trace && trace->hasFlag(flag)) {
		return false;
	}

	if (trace != NULL) {
		removeWire(trace, ends, done, parentCommand);
	}

	QString colorString = traceColor(createWireViewLayerSpec(ends[0], ends[1]));
	long newID = createWire(ends[0], ends[1], flag, false, BaseCommand::CrossView, parentCommand);
	// TODO: is this opacity constant stored someplace
	new WireColorChangeCommand(this, newID, colorString, colorString, 1.0, 1.0, parentCommand);
	new WireWidthChangeCommand(this, newID, getTraceWidth(), getTraceWidth(), parentCommand);
	return true;
}

void SketchWidget::updateNet(Wire *) {
}

void SketchWidget::selectAllWires(ViewGeometry::WireFlag flag) 
{
	QList<Wire *> wires;
	foreach (QGraphicsItem * item, scene()->items()) {
		Wire * wire = dynamic_cast<Wire *>(item);
		if (wire == NULL) continue;

		if (wire->hasFlag(flag)) {
			if (wire->parentItem() != NULL) {
				// skip module wires
				continue;
			}

			wires.append(wire);
		}
	}

	if (wires.count() <= 0) {
		// TODO: tell user?
	}

	QString wireName;
	if (flag == getTraceFlag()) {
		wireName = QObject::tr("Trace wires");
	}
	else if (flag == ViewGeometry::RatsnestFlag) {
		wireName = QObject::tr("Ratsnest wires");
	}
	QUndoCommand * parentCommand = new QUndoCommand(QObject::tr("Select all %1").arg(wireName));

	stackSelectionState(false, parentCommand);
	SelectItemCommand * selectItemCommand = new SelectItemCommand(this, SelectItemCommand::NormalSelect, parentCommand);
	foreach (Wire * wire, wires) {
		selectItemCommand->addRedo(wire->id());
	}

	scene()->clearSelection();
	m_undoStack->push(parentCommand);
}

void SketchWidget::tidyWires() {
}   

void SketchWidget::updateConnectors() {
	// update issue with 4.5.0?
	foreach (QGraphicsItem* item, scene()->items()) {
		ConnectorItem * connectorItem = dynamic_cast<ConnectorItem *>(item);
		if (connectorItem == NULL) continue;

		connectorItem->setMarked(false);
	}

	foreach (QGraphicsItem* item, scene()->items()) {
		ConnectorItem * connectorItem = dynamic_cast<ConnectorItem *>(item);
		if (connectorItem == NULL) continue;
		if (connectorItem->marked()) {
			continue;
		}

		connectorItem->restoreColor(true, 0, true);
	}
}

const QString & SketchWidget::getShortName() {
	return m_shortName;
}

void SketchWidget::getBendpointWidths(Wire * wire, double width, double & bendpointWidth, double & bendpoint2Width, bool & negativeOffsetRect) {
	Q_UNUSED(wire);
	Q_UNUSED(width);
	bendpoint2Width = bendpointWidth = -1;
	negativeOffsetRect = true;
}

QColor SketchWidget::standardBackground() {
	return RatsnestColors::backgroundColor(m_viewIdentifier);
}

void SketchWidget::initBackgroundColor() {
	setBackground(standardBackground());

	QSettings settings;
	QString colorName = settings.value(QString("%1BackgroundColor").arg(getShortName())).toString();
	if (!colorName.isEmpty()) {
		QColor color;
		color.setNamedColor(colorName);
		setBackground(color);
	}

	m_curvyWires = false;
	QString curvy = settings.value(QString("%1CurvyWires").arg(getShortName())).toString();
	if (!curvy.isEmpty()) {
		m_curvyWires = (curvy.compare("1") == 0);
	}
}

bool SketchWidget::includeSymbols() {
	return false;
}

void SketchWidget::disconnectAll() {

	QSet<ItemBase *> itemBases;
	foreach (QGraphicsItem * item, scene()->selectedItems()) {
		ItemBase * itemBase = dynamic_cast<ItemBase *>(item);
		if (itemBase == NULL) continue;

		itemBases.insert(itemBase);
	}

	QList<ConnectorItem *> connectorItems;
	foreach (ItemBase * itemBase, itemBases) {
		ConnectorItem * fromConnectorItem = itemBase->rightClickedConnector();
		if (fromConnectorItem == NULL) continue;

		if (fromConnectorItem->connectedToWires()) {
			connectorItems.append(fromConnectorItem);
		}
	}

	if (connectorItems.count() <= 0) return;

	QString string;
	if (itemBases.count() == 1) {
		ItemBase * firstItem = *(itemBases.begin());
		string = tr("Disconnect all wires from %1").arg(firstItem->title());
	}
	else {
		string = tr("Disconnect all wires from %1 items").arg(QString::number(itemBases.count()));
	}

	QUndoCommand * parentCommand = new QUndoCommand(string);

	stackSelectionState(false, parentCommand);
	new CleanUpWiresCommand(this, CleanUpWiresCommand::UndoOnly, parentCommand);


	QHash<ItemBase *, SketchWidget *> itemsToDelete;
	disconnectAllSlot(connectorItems, itemsToDelete, parentCommand);
	emit disconnectAllSignal(connectorItems, itemsToDelete, parentCommand);

	new CleanUpWiresCommand(this, CleanUpWiresCommand::RedoOnly, parentCommand);
	foreach (ItemBase * item, itemsToDelete.keys()) {
		itemsToDelete.value(item)->makeDeleteItemCommand(item, BaseCommand::CrossView, parentCommand);
	}
	m_undoStack->push(parentCommand);
}

void SketchWidget::disconnectAllSlot(QList<ConnectorItem *> connectorItems, QHash<ItemBase *, SketchWidget *> & itemsToDelete, QUndoCommand * parentCommand)
{
	// (jc 2011 Aug 16): this code is not hooked up and my last recollection is that it wasn't working

	QList<ConnectorItem *> myConnectorItems;
	foreach (ConnectorItem * ci, connectorItems) {
		ItemBase * itemBase = findItem(ci->attachedToID());
		if (itemBase == NULL) continue;

		ConnectorItem * fromConnectorItem = findConnectorItem(itemBase, ci->connectorSharedID(), ViewLayer::Top);
		if (fromConnectorItem != NULL) {
			myConnectorItems.append(fromConnectorItem);
		}
		fromConnectorItem = findConnectorItem(itemBase, ci->connectorSharedID(), ViewLayer::Bottom);
		if (fromConnectorItem != NULL) {
			myConnectorItems.append(fromConnectorItem);
		}
	}

	QHash<ItemBase *, SketchWidget *> deletedItems;
	foreach (ConnectorItem * fromConnectorItem, myConnectorItems) {
		foreach (ConnectorItem * toConnectorItem, fromConnectorItem->connectedToItems()) {
			if (toConnectorItem->attachedToItemType() == ModelPart::Wire) {
				Wire * wire = qobject_cast<Wire *>(toConnectorItem->attachedTo());
				if (!wire->getRatsnest()) {
					QList<Wire *> chained;
					QList<ConnectorItem *> ends;
					wire->collectChained(chained, ends);
					foreach (Wire * w, chained) {
						itemsToDelete.insert(w, this);
						deletedItems.insert(w, this);
					}
				}
			}
			else if (toConnectorItem->connectorType() == Connector::Female) {
				if (ignoreFemale()) {
					//fromConnectorItem->tempRemove(toConnectorItem, false);
					//toConnectorItem->tempRemove(fromConnectorItem, false);
					//extendChangeConnectionCommand(fromConnectorItem, toConnectorItem, false, true, parentCommand);
				}
				else {
					ItemBase * detachee = fromConnectorItem->attachedTo();
					QPointF newPos = calcNewLoc(detachee, toConnectorItem->attachedTo());
					// delete connections
					// add wires and connections for undisconnected connectors

					detachee->saveGeometry();
					ViewGeometry vg = detachee->getViewGeometry();
					vg.setLoc(newPos);
					new MoveItemCommand(this, detachee->id(), detachee->getViewGeometry(), vg, false, parentCommand);
					QHash<long, ItemBase *> emptyList;
					ConnectorPairHash connectorHash;
					disconnectFromFemale(detachee, emptyList, connectorHash, true, false, parentCommand);
					foreach (ConnectorItem * fConnectorItem, connectorHash.uniqueKeys()) {
						if (myConnectorItems.contains(fConnectorItem)) {
							// don't need to reconnect
							continue;
						}

						foreach (ConnectorItem * tConnectorItem, connectorHash.values(fConnectorItem)) {
							createWire(fConnectorItem, tConnectorItem, ViewGeometry::NoFlag, false, BaseCommand::CrossView, parentCommand);
						}
					}
				}
			}
		}
	}

	deleteMiddle(deletedItems, parentCommand);
}

bool SketchWidget::canDisconnectAll() {
	return true;
}

bool SketchWidget::ignoreFemale() {
	return true;
}

QPointF SketchWidget::calcNewLoc(ItemBase * moveBase, ItemBase * detachFrom)
{
	QRectF dr = detachFrom->boundingRect();
	dr.moveTopLeft(detachFrom->pos());

	QPointF pos = moveBase->pos();
	QRectF r = moveBase->boundingRect();
	pos.setX(pos.x() + (r.width() / 2.0));
	pos.setY(pos.y() + (r.height() / 2.0));
	double d[4];
	d[0] = qAbs(pos.y() - dr.top());
	d[1] = qAbs(pos.y() - dr.bottom());
	d[2] = qAbs(pos.x() - dr.left());
	d[3] = qAbs(pos.x() - dr.right());
	int ix = 0;
	for (int i = 1; i < 4; i++) {
		if (d[i] < d[ix]) {
			ix = i;
		}
	}
	QPointF newPos = moveBase->pos();
	switch (ix) {
		case 0:
			newPos.setY(dr.top() - r.height());
			break;
		case 1:
			newPos.setY(dr.bottom());
			break;
		case 2:
			newPos.setX(dr.left() - r.width());
			break;
		case 3:
			newPos.setX(dr.right());
			break;
	}
	return newPos;
}

long SketchWidget::findPartOrWire(long itemID) 
{
	ItemBase * item = findItem(itemID);
	if (item == NULL) return itemID;

	if (item->itemType() != ModelPart::Wire) return itemID;

	QList<Wire *> chained;
	QList<ConnectorItem *> ends;
	qobject_cast<Wire *>(item)->collectChained(chained, ends);
	if (chained.length() <= 1) return itemID;

	foreach (Wire * w, chained) {
		if (w->id() < itemID) {
			itemID = w->id();
		}
	}

	return itemID;
}

void SketchWidget::resizeJumperItem(long itemID, QPointF pos, QPointF c0, QPointF c1) {
	ItemBase * item = findItem(itemID);
	if (item == NULL) return;

	if (item->itemType() != ModelPart::Jumper) return;

	qobject_cast<JumperItem *>(item)->resize(pos, c0, c1);
}


int SketchWidget::selectAllItemType(ModelPart::ItemType itemType) 
{
	QSet<ItemBase *> itemBases;
	foreach (QGraphicsItem * item, scene()->items()) {
		ItemBase * itemBase = dynamic_cast<ItemBase *>(item);
		if (itemBase == NULL) continue;
		if (itemBase->itemType() != itemType) continue;

		itemBases.insert(itemBase->layerKinChief());
	}

	return selectAllItems(itemBases, QObject::tr("Select all jumpers"));

}

int SketchWidget::selectAllObsolete() 
{
	QSet<ItemBase *> itemBases;
	foreach (QGraphicsItem * item, scene()->items()) {
		ItemBase * itemBase = dynamic_cast<ItemBase *>(item);
		if (itemBase == NULL) continue;
		if (!itemBase->isObsolete()) continue;

		itemBases.insert(itemBase->layerKinChief());
	}

	return selectAllItems(itemBases, QObject::tr("Select outdated parts"));
}

int SketchWidget::selectAllMoveLock() 
{
	QSet<ItemBase *> itemBases;
	foreach (QGraphicsItem * item, scene()->items()) {
		ItemBase * itemBase = dynamic_cast<ItemBase *>(item);
		if (itemBase == NULL) continue;
		if (!itemBase->moveLock()) continue;

		itemBases.insert(itemBase->layerKinChief());
	}

	return selectAllItems(itemBases, QObject::tr("Select locked parts"));
}


int SketchWidget::selectAllItems(QSet<ItemBase *> & itemBases, const QString & msg) 
{
	if (itemBases.count() <= 0) {
		// TODO: tell user?
		return 0;
	}

	QUndoCommand * parentCommand = new QUndoCommand(msg);

	stackSelectionState(false, parentCommand);
	SelectItemCommand * selectItemCommand = new SelectItemCommand(this, SelectItemCommand::NormalSelect, parentCommand);
	foreach (ItemBase * itemBase, itemBases) {
		selectItemCommand->addRedo(itemBase->id());
	}

	scene()->clearSelection();
	m_undoStack->push(parentCommand);

	return itemBases.count();
}

AddItemCommand * SketchWidget::newAddItemCommand(BaseCommand::CrossViewType crossViewType, QString moduleID, ViewLayer::ViewLayerSpec viewLayerSpec, ViewGeometry & viewGeometry, qint64 id, bool updateInfoView, long modelIndex, QUndoCommand *parent)
{
	return new AddItemCommand(this, crossViewType, moduleID, viewLayerSpec, viewGeometry, id, updateInfoView, modelIndex, parent);
}

bool SketchWidget::partLabelsVisible() {
	foreach (QGraphicsItem * item, scene()->items()) {
		ItemBase * itemBase = dynamic_cast<ItemBase *>(item);
		if (itemBase == NULL) continue;

		if (itemBase->isPartLabelVisible()) return true;

	}

	return false;
}

void SketchWidget::showLabelFirstTime(long itemID, bool show, bool doEmit) {
	if (doEmit) {
		emit showLabelFirstTimeSignal(itemID, show, false);
	}
}

void SketchWidget::restorePartLabel(long itemID, QDomElement & element) {
	ItemBase * itemBase = findItem(itemID);
	if (itemBase == NULL) return;

	itemBase->restorePartLabel(element, getLabelViewLayerID(itemBase->viewLayerSpec()));
}

void SketchWidget::loadLogoImage(long itemID, const QString & oldSvg, const QSizeF oldAspectRatio, const QString & oldFilename, const QString & newFilename, bool addName) {
	QUndoCommand * cmd = new LoadLogoImageCommand(this, itemID, oldSvg, oldAspectRatio, oldFilename, newFilename, addName, NULL);
	cmd->setText(tr("Change image from %1 to %2").arg(oldFilename).arg(newFilename));
	m_undoStack->waitPush(cmd, PropChangeDelay);
}

void SketchWidget::loadLogoImage(long itemID, const QString & oldSvg, const QSizeF oldAspectRatio, const QString & oldFilename) {
	ItemBase * itemBase = findItem(itemID);
	if (itemBase == NULL) return;

	LogoItem * logoItem = qobject_cast<LogoItem *>(itemBase);
	if (logoItem == NULL) return;

	logoItem->reloadImage(oldSvg, oldAspectRatio, oldFilename, false);
}

void SketchWidget::loadLogoImage(long itemID, const QString & newFilename, bool addName) {
	ItemBase * itemBase = findItem(itemID);
	if (itemBase == NULL) return;

	LogoItem * logoItem = qobject_cast<LogoItem *>(itemBase);
	if (logoItem == NULL) return;

	logoItem->loadImage(newFilename, addName);
}

void SketchWidget::paintEvent ( QPaintEvent * event ) {
    //DebugDialog::debug("sketch widget paint event");
    if (scene()) {
        ((FGraphicsScene *) scene())->setDisplayHandles(true);
    }
    QGraphicsView::paintEvent(event);
}

void SketchWidget::setNoteFocus(QGraphicsItem * item, bool inFocus) {
	if (inFocus) {
		m_inFocus.append(item);
	}
	else {
		m_inFocus.removeOne(item);
	}
}

double SketchWidget::defaultGridSizeInches() {
	return 0.0;			// should never get here
}

double SketchWidget::gridSizeInches() {
	return m_gridSizeInches;		
}

void SketchWidget::alignToGrid(bool align) {
	m_alignToGrid = align;
	QSettings settings;
	settings.setValue(QString("%1AlignToGrid").arg(viewName()), align);
}

void SketchWidget::showGrid(bool show) {
	m_showGrid = show;
	QSettings settings;
	settings.setValue(QString("%1ShowGrid").arg(viewName()), show);
	update();
}

bool SketchWidget::alignedToGrid() {
	return m_alignToGrid;
}

bool SketchWidget::showingGrid() {
	return m_showGrid;
}

bool SketchWidget::canAlignToTopLeft(ItemBase *) 
{
	return false;
}

void SketchWidget::saveZoom(double zoom) {
	m_zoom = zoom;
}

double SketchWidget::retrieveZoom() {
	return m_zoom;
}

void SketchWidget::initGrid() {
	m_showGrid = m_alignToGrid = false;
	m_gridSizeInches = defaultGridSizeInches();
	QSettings settings;
	QString szString = settings.value(QString("%1GridSize").arg(viewName()), "").toString();
	if (!szString.isEmpty()) {
		bool ok;
		double temp = TextUtils::convertToInches(szString, &ok, false);
		if (ok) {
			m_gridSizeInches = temp;
		}
	}
	m_alignToGrid = settings.value(QString("%1AlignToGrid").arg(viewName()), false).toBool();
	m_showGrid = settings.value(QString("%1ShowGrid").arg(viewName()), false).toBool();
}


void SketchWidget::copyDrop() {
	QList<ItemBase *> itemBases = m_savedItems.values();
    qSort(itemBases.begin(), itemBases.end(), ItemBase::zLessThan);
	foreach (ItemBase * itemBase, itemBases) {
		QPointF loc = itemBase->getViewGeometry().loc();
        itemBase->setItemPos(loc);
	}
	copyAux(itemBases, false);

	m_savedItems.clear();
	m_savedWires.clear();
}

ViewLayer::ViewLayerSpec SketchWidget::defaultViewLayerSpec() {
	return (m_boardLayers == 1) ? ViewLayer::ThroughHoleThroughTop_OneLayer : ViewLayer::ThroughHoleThroughTop_TwoLayers;
}

ViewLayer::ViewLayerSpec SketchWidget::wireViewLayerSpec(ConnectorItem *) {
	return (m_boardLayers == 1) ? ViewLayer::WireOnBottom_OneLayer : ViewLayer::WireOnBottom_TwoLayers;
}

void SketchWidget::changeBoardLayers(int layers, bool doEmit) {
	m_boardLayers = layers;

	foreach (QGraphicsItem * item, scene()->items()) {
		ItemBase * itemBase = dynamic_cast<ItemBase *>(item);
		if (itemBase == NULL) continue;
		if (itemBase->modelPart()->flippedSMD()) continue;
		if (itemBase->itemType() != ModelPart::Part) continue;

		if (itemBase->viewLayerSpec() == ViewLayer::ThroughHoleThroughTop_OneLayer && layers == 2) {
			itemBase->setViewLayerSpec(ViewLayer::ThroughHoleThroughTop_TwoLayers);
			continue;
		}
	
		if (itemBase->viewLayerSpec() == ViewLayer::ThroughHoleThroughTop_TwoLayers && layers == 1) {
			itemBase->setViewLayerSpec(ViewLayer::ThroughHoleThroughTop_OneLayer);
			continue;
		}
	}

	if (doEmit) {
		emit changeBoardLayersSignal(layers, false);
	}
}

void SketchWidget::collectAllNets(QHash<ConnectorItem *, int> & indexer, QList< QList<class ConnectorItem *>* > & allPartConnectorItems, bool includeSingletons, bool bothSides) 
{
	// get the set of all connectors in the sketch
	QList<ConnectorItem *> allConnectors;
	foreach (QGraphicsItem * item, scene()->items()) {
		ConnectorItem * connectorItem = dynamic_cast<ConnectorItem *>(item);
		if (connectorItem == NULL) continue;
		if (!bothSides && connectorItem->attachedToViewLayerID() == ViewLayer::Copper1) continue;

		allConnectors.append(connectorItem);
	}

	// find all the nets and make a list of nodes (i.e. part ConnectorItems) for each net
	while (allConnectors.count() > 0) {
		
		ConnectorItem * connectorItem = allConnectors.takeFirst();
		QList<ConnectorItem *> connectorItems;
		connectorItems.append(connectorItem);
		ConnectorItem::collectEqualPotential(connectorItems, bothSides, ViewGeometry::NoFlag);
		if (connectorItems.count() <= 0) {
			continue;
		}

		foreach (ConnectorItem * ci, connectorItems) {
			//if (connectorItems.count(ci) > 1) {
				//DebugDialog::debug("collect equal potential bug");
			//}
			//DebugDialog::debug(QString("from in equal potential %1 %2").arg(ci->connectorSharedName()).arg(ci->attachedToInstanceTitle()));
			allConnectors.removeOne(ci);
		}

		if (!includeSingletons && (connectorItems.count() <= 1)) {
			continue;
		}

		QList<ConnectorItem *> * partConnectorItems = new QList<ConnectorItem *>;
		ConnectorItem::collectParts(connectorItems, *partConnectorItems, includeSymbols(), ViewLayer::TopAndBottom);

		for (int i = partConnectorItems->count() - 1; i >= 0; i--) {
			if (!partConnectorItems->at(i)->attachedTo()->isEverVisible()) {
				partConnectorItems->removeAt(i);
			}
		}

		if ((partConnectorItems->count() <= 0) || (!includeSingletons && (partConnectorItems->count() <= 1))) {
			delete partConnectorItems;
			continue;
		}

		foreach (ConnectorItem * ci, *partConnectorItems) {
			//if (partConnectorItems->count(ci) > 1) {
				//DebugDialog::debug("collect Parts bug");
			//}
			if (!connectorItems.contains(ci)) {
				// crossed layer: toss it
				//DebugDialog::debug(QString("not in equal potential '%1' '%2' %3")
				//	.arg(ci->connectorSharedName())
				//	.arg(ci->attachedToInstanceTitle())
				//	.arg(ci->attachedToViewLayerID()));
				continue;
			}
			//if (indexer.keys().contains(ci)) {
				//DebugDialog::debug(QString("connector item already indexed %1 %2").arg(ci->connectorSharedName()).arg(ci->attachedToInstanceTitle()));
			//}
			//int c = indexer.count();
			//DebugDialog::debug(QString("insert indexer %1 '%2' '%3' %4")
				//.arg(c)
				//.arg(ci->connectorSharedName())
				//.arg(ci->attachedToInstanceTitle())
				//.arg(ci->attachedToViewLayerID()));
			indexer.insert(ci, indexer.count());
		}

		//DebugDialog::debug("________________");
		allPartConnectorItems.append(partConnectorItems);
	}
}

ViewLayer::ViewLayerSpec SketchWidget::getViewLayerSpec(ModelPart * modelPart, QDomElement & instance, QDomElement & view, ViewGeometry & viewGeometry) 
{
	Q_UNUSED(modelPart);
	Q_UNUSED(instance);

	ViewLayer::ViewLayerSpec viewLayerSpec = defaultViewLayerSpec();

	if (modelPart->moduleID().compare(ModuleIDNames::GroundPlaneModuleIDName) == 0) {
		QString layer = view.attribute("layer");
		if (layer.isEmpty()) return viewLayerSpec;

		ViewLayer::ViewLayerID viewLayerID = ViewLayer::viewLayerIDFromXmlString(layer);
		if (viewLayerID == ViewLayer::GroundPlane1) {
			return ViewLayer::GroundPlane_Top;
		}

		return ViewLayer::GroundPlane_Bottom;
	}

	if (viewGeometry.getAnyTrace()) {
		QString layer = view.attribute("layer");
		if (layer.isEmpty()) return viewLayerSpec;

		ViewLayer::ViewLayerID viewLayerID = ViewLayer::viewLayerIDFromXmlString(layer);
		if (viewLayerID == ViewLayer::Copper1Trace) {
			return ViewLayer::WireOnTop_TwoLayers;
		}
	}

	return viewLayerSpec;

}

bool SketchWidget::routeBothSides() {
	return false;
}


void SketchWidget::copyBoundingRectsSlot(QHash<QString, QRectF> & boundingRects)
{
	QRectF itemsBoundingRect;
	QList<QGraphicsItem *> tlBases;
	foreach (QGraphicsItem * item, scene()->selectedItems()) {
		ItemBase * itemBase =  ItemBase::extractTopLevelItemBase(item);
		if (itemBase == NULL) continue;
		if (itemBase->getRatsnest()) continue;

		itemsBoundingRect |= itemBase->sceneBoundingRect();	
	}

	boundingRects.insert(m_viewName, itemsBoundingRect);
}

void SketchWidget::changeLayer(long id, double z, ViewLayer::ViewLayerID viewLayerID) {
	Q_UNUSED(id);
	Q_UNUSED(z);
	Q_UNUSED(viewLayerID);
}

bool SketchWidget::resizingJumperItemRelease() {
	return false;
}

bool SketchWidget::resizingJumperItemPress(QGraphicsItem *) {
	return false;
}

bool SketchWidget::resizingBoardRelease() {
	return false;
}

bool SketchWidget::resizingBoardPress(QGraphicsItem *) {
	return false;
}

bool SketchWidget::hasAnyNets() {
	return false;
}

void SketchWidget::ratsnestConnect(ItemBase * itemBase, bool connect) {
	foreach (ConnectorItem * connectorItem, itemBase->cachedConnectorItems()) {
		ratsnestConnect(connectorItem, connect);
	}
}

void SketchWidget::ratsnestConnect(long id, const QString & connectorID, bool connect, bool doEmit) {
	if (doEmit) {
		emit ratsnestConnectSignal(id, connectorID, connect, false);
	}

	ItemBase * itemBase = findItem(id);
	if (itemBase == NULL) return;

	ConnectorItem * connectorItem = findConnectorItem(itemBase, connectorID, itemBase->viewLayerSpec());
	if (connectorItem == NULL) return;

	ratsnestConnect(connectorItem, connect);
}

void SketchWidget::ratsnestConnect(ConnectorItem * c1, ConnectorItem * c2, bool connect) {
	QList<ConnectorItem *> connectorItems;
	connectorItems.append(c1);
	connectorItems.append(c2);
	ConnectorItem::collectEqualPotential(connectorItems, true, ViewGeometry::RatsnestFlag);
	foreach (ConnectorItem * connectorItem, connectorItems) {
		ratsnestConnect(connectorItem, connect);
	}
}

void SketchWidget::ratsnestConnect(ConnectorItem * connectorItem, bool connect) {
	if (connect) {
		m_ratsnestUpdateConnect << connectorItem;
	}
	else {
		m_ratsnestUpdateDisconnect << connectorItem;
	}

	//connectorItem->debugInfo(QString("rat connect %1").arg(connect));
}


void SketchWidget::deleteRatsnest(Wire * ratsnest, QUndoCommand * parentCommand)
{
	// deleting a ratsnest really means deleting underlying connections

	// assume ratsnest has only one connection at each end
	ConnectorItem * source = ratsnest->connector0()->firstConnectedToIsh();
	ConnectorItem * sink = ratsnest->connector1()->firstConnectedToIsh();

	QList<ConnectorItem *> connectorItems;
	connectorItems.append(source);
	connectorItems.append(sink);
	ConnectorItem::collectEqualPotential(connectorItems, true, ViewGeometry::RatsnestFlag);

	QList<SketchWidget *> foreignSketchWidgets;
	emit collectRatsnestSignal(foreignSketchWidgets);

	// there are multiple possibilities for each pair of connectors:

	// they are directly connected because they're each inserted into female connectors on the same bus
	// they are directly connected with a wire
	// they are "directly" connected through some combination of female connectors and wires (i.e. one part is connected to a wire which is inserted into a female connector)
	// they are indirectly connected via other parts

	// what if there are multiple direct connections--treat it as a single connection and delete them all
	
	QList<ConnectorEdge *> cutSet;
	GraphUtils::minCut(connectorItems, foreignSketchWidgets, source, sink, cutSet);
	emit removeRatsnestSignal(cutSet, parentCommand);
	foreach (ConnectorEdge * ce, cutSet) {
		delete ce;
	}
}

void SketchWidget::removeRatsnestSlot(QList<ConnectorEdge *> & cutSet, QUndoCommand * parentCommand)
{
	QHash<ConnectorItem *, ConnectorItem *> detachItems;			// key is part to be detached, value is part to detach from
	QSet<ItemBase *> deletedItems;
	QList<long> deletedIDs;

	foreach (ConnectorEdge * ce, cutSet) {

		if (ce->c0->attachedToViewIdentifier() != viewIdentifier()) continue;
		if (ce->c1->attachedToViewIdentifier() != viewIdentifier()) continue;

		if (ce->wire) {
			QList<ConnectorItem *> ends;
			QList<Wire *> wires;
			ce->wire->collectChained(wires, ends);
			foreach (Wire * w, wires) {
				if (!deletedIDs.contains(w->id())) {
					deletedItems.insert(w);
					deletedIDs.append(w->id());
				}
			}
		}
		else {
			// we have to detach the source or sink from a female connector
			if (ce->c0->connectorType() == Connector::Female) {
				detachItems.insert(ce->c1, ce->c0);
			}
			else {
				detachItems.insert(ce->c0, ce->c1);
			}
		}
	}

	foreach (ConnectorItem * detacheeConnector, detachItems.keys()) {
		ItemBase * detachee = detacheeConnector->attachedTo();
		ConnectorItem * detachFromConnector = detachItems.value(detacheeConnector);
		ItemBase * detachFrom = detachFromConnector->attachedTo();
		QPointF newPos = calcNewLoc(detachee, detachFrom);

		// delete connections
		// add wires and connections for undisconnected connectors

		detachee->saveGeometry();
		ViewGeometry vg = detachee->getViewGeometry();
		vg.setLoc(newPos);
		new MoveItemCommand(this, detachee->id(), detachee->getViewGeometry(), vg, false, parentCommand);
		QHash<long, ItemBase *> emptyList;
		ConnectorPairHash connectorHash;
		disconnectFromFemale(detachee, emptyList, connectorHash, true, false, parentCommand);
		foreach (ConnectorItem * fromConnectorItem, connectorHash.uniqueKeys()) {
			if (detachItems.keys().contains(fromConnectorItem)) {
				// don't need to reconnect
				continue;
			}
			if (detachItems.values().contains(fromConnectorItem)) {
				// don't need to reconnect
				continue;
			}

			foreach (ConnectorItem * toConnectorItem, connectorHash.values(fromConnectorItem)) {
				createWire(fromConnectorItem, toConnectorItem, ViewGeometry::NoFlag, false, BaseCommand::CrossView, parentCommand);
			}
		}
	}


	deleteAux(deletedItems, parentCommand, false);
}

void SketchWidget::addDefaultParts() {
}

void SketchWidget::vScrollToZero() {
	verticalScrollBar()->setValue(verticalScrollBar()->minimum());
}

float SketchWidget::getTopZ() {
	return m_z;
}

QGraphicsItem * SketchWidget::addWatermark(const QString &filename)
{
	QGraphicsSvgItem * item = new QGraphicsSvgItem(filename);
	if (item == NULL) return NULL;

	this->scene()->addItem(item);
	return item;
}

bool SketchWidget::acceptsTrace(const ViewGeometry &) {
	return false;
}

void SketchWidget::alignOneToGrid(ItemBase * itemBase) {
	if (m_alignToGrid) {
		QHash<long, ItemBase *> savedItems;
		QHash<Wire *, ConnectorItem *> savedWires;
		findAlignmentAnchor(itemBase, savedItems, savedWires);
		if (m_alignmentItem) {
			m_alignmentItem = NULL;
			QPointF loc = itemBase->pos();
			alignLoc(loc, m_alignmentStartPoint, QPointF(0,0), QPointF(0, 0));
			itemBase->setPos(loc);
		}
	}
}

void SketchWidget::changeTrace(Wire * wire, ConnectorItem * from, ConnectorItem * to, QUndoCommand * parentCommand) {
	Q_UNUSED(wire);
	Q_UNUSED(from);
	Q_UNUSED(to);
	Q_UNUSED(parentCommand);
}

ViewGeometry::WireFlag SketchWidget::getTraceFlag() {
	return ViewGeometry::NormalFlag;
}

void SketchWidget::changeBus(ItemBase * itemBase, bool connect, const QString & oldBus, const QString & newBus, QList<ConnectorItem *> & connectorItems, const QString & message)
{
	QUndoCommand * parentCommand = new QUndoCommand(message);
	CleanUpWiresCommand * cuwc = new CleanUpWiresCommand(this, CleanUpWiresCommand::UndoOnly, parentCommand);
	foreach(ConnectorItem * connectorItem, connectorItems) {
		cuwc->addRatsnestConnect(connectorItem->attachedToID(), connectorItem->connectorSharedID(), connect);
	}
	
	new SetPropCommand(this, itemBase->id(), "buses", oldBus, newBus, true, parentCommand);
	cuwc = new CleanUpWiresCommand(this, CleanUpWiresCommand::RedoOnly, parentCommand);
	foreach(ConnectorItem * connectorItem, connectorItems) {
		cuwc->addRatsnestConnect(connectorItem->attachedToID(), connectorItem->connectorSharedID(), connect);
	}

	m_undoStack->waitPush(parentCommand, PropChangeDelay);

}

const QString & SketchWidget::filenameIf()
{
	static QString filename;
	emit filenameIfSignal(filename);
	return filename;
}

void SketchWidget::setItemDropOffset(long id, QPointF offset)
{
	ItemBase * itemBase = findItem(id);
	if (itemBase == NULL) return;

	itemBase->setDropOffset(offset);
}

Wire * SketchWidget::createTempWireForDragging(Wire * fromWire, ModelPart * wireModel, ConnectorItem * connectorItem, ViewGeometry & viewGeometry, ViewLayer::ViewLayerSpec spec) 
{
	Q_UNUSED(fromWire);
	if (spec == ViewLayer::UnknownSpec) {
		spec = wireViewLayerSpec(connectorItem);
	}
	return qobject_cast<Wire *>(addItemAuxTemp(wireModel, spec, viewGeometry, ItemBase::getNextID(), NULL, true, m_viewIdentifier, true));
}

void SketchWidget::prereleaseTempWireForDragging(Wire*)
{
}

void SketchWidget::wireChangedCurveSlot(Wire* wire, const Bezier * oldB, const Bezier * newB, bool triggerFirstTime) {
	this->clearHoldingSelectItem();
	this->m_moveEventCount = 0;  // clear this so an extra MoveItemCommand isn't posted

	ChangeWireCurveCommand * cwcc = new ChangeWireCurveCommand(this, wire->id(), oldB, newB, NULL);
	cwcc->setText("Change wire curvature");
	if (!triggerFirstTime) {
		cwcc->setFirstTime();
	}
	m_undoStack->push(cwcc);
}

void SketchWidget::changeWireCurve(long id, const Bezier * bezier) {
	Wire * wire = qobject_cast<Wire *>(findItem(id));
	if (wire == NULL) return;

	wire->changeCurve(bezier);
}


void SketchWidget::changeLegCurve(long id, const QString & connectorID, int index, const Bezier * bezier) {
	ItemBase * itemBase = findItem(id);
	if (itemBase == NULL) return;

	ConnectorItem * connectorItem = findConnectorItem(itemBase, connectorID, ViewLayer::specFromID(itemBase->viewLayerID()));
	if (connectorItem == NULL) return;

	connectorItem->changeLegCurve(index, bezier);
}

void SketchWidget::addLegBendpoint(long id, const QString & connectorID, int index, QPointF p, const class Bezier * bezierLeft, const class Bezier * bezierRight)
{
	ItemBase * itemBase = findItem(id);
	if (itemBase == NULL) return;

	ConnectorItem * connectorItem = findConnectorItem(itemBase, connectorID, ViewLayer::specFromID(itemBase->viewLayerID()));
	if (connectorItem == NULL) return;

	connectorItem->addLegBendpoint(index, p, bezierLeft, bezierRight);
}

void SketchWidget::removeLegBendpoint(long id, const QString & connectorID, int index, const class Bezier * bezierCombined)
{
	ItemBase * itemBase = findItem(id);
	if (itemBase == NULL) return;

	ConnectorItem * connectorItem = findConnectorItem(itemBase, connectorID, ViewLayer::specFromID(itemBase->viewLayerID()));
	if (connectorItem == NULL) return;

	connectorItem->removeLegBendpoint(index, bezierCombined);
}

void SketchWidget::moveLegBendpoint(long id, const QString & connectorID, int index, QPointF p)
{
	ItemBase * itemBase = findItem(id);
	if (itemBase == NULL) return;

	ConnectorItem * connectorItem = findConnectorItem(itemBase, connectorID, ViewLayer::specFromID(itemBase->viewLayerID()));
	if (connectorItem == NULL) return;

	connectorItem->moveLegBendpoint(index, p);
}

void SketchWidget::moveLegBendpoints(bool undoOnly, QUndoCommand * parentCommand) 
{
	foreach (ItemBase * itemBase, m_stretchingLegs.uniqueKeys()) {
		foreach (ConnectorItem * connectorItem, m_stretchingLegs.values(itemBase)) {
			moveLegBendpointsAux(connectorItem, undoOnly, parentCommand);
		}
	}
}

void SketchWidget::moveLegBendpointsAux(ConnectorItem * connectorItem, bool undoOnly, QUndoCommand * parentCommand) 
{
	int index0, index1;
	QPointF oldPos0, newPos0, oldPos1, newPos1;
	connectorItem->moveDone(index0, oldPos0, newPos0, index1, oldPos1, newPos1);
	MoveLegBendpointCommand * mlbc = new MoveLegBendpointCommand(this, connectorItem->attachedToID(), connectorItem->connectorSharedID(), index0, oldPos0, newPos0, parentCommand);
	if (undoOnly) mlbc->setUndoOnly();
	else mlbc->setRedoOnly();
	if (index0 != index1) {
		mlbc = new MoveLegBendpointCommand(this, connectorItem->attachedToID(), connectorItem->connectorSharedID(), index1, oldPos1, newPos1, parentCommand);
		if (undoOnly) mlbc->setUndoOnly();
		else mlbc->setRedoOnly();
	}
}


bool SketchWidget::curvyWires()
{
	return m_curvyWires;
}

void SketchWidget::setCurvyWires(bool curvyWires)
{
	m_curvyWires = curvyWires;
}

bool SketchWidget::curvyWiresIndicated(Qt::KeyboardModifiers modifiers)
{
	if (m_curvyWires) {
		return ((modifiers & Qt::ControlModifier) == 0);
	}

	return ((modifiers & Qt::ControlModifier) != 0);
}

void SketchWidget::setMoveLock(long id, bool lock)
{
	ItemBase * itemBase = findItem(id);
	if (itemBase) itemBase->setMoveLock(lock);
}

void SketchWidget::triggerRotate(ItemBase * itemBase, double degrees)
{
	RotateItemCommand * ric = new RotateItemCommand(this, itemBase->id(), degrees, NULL);
	ric->setText(tr("Rotate %1").arg(itemBase->instanceTitle()));
	m_undoStack->push(ric);
}

void SketchWidget::makeWiresChangeConnectionCommands(const QList<Wire *> & wires, QUndoCommand * parentCommand)
{
	QStringList alreadyList;
	foreach (Wire * wire, wires) {
		QList<ConnectorItem *> wireConnectorItems;
		wireConnectorItems << wire->connector0() << wire->connector1();
		foreach (ConnectorItem * fromConnectorItem, wireConnectorItems) {
			foreach(ConnectorItem * toConnectorItem, fromConnectorItem->connectedToItems()) {
				QString already = ((fromConnectorItem->attachedToID() <= toConnectorItem->attachedToID()) ? QString("%1.%2.%3.%4") : QString("%3.%4.%1.%2"))
					.arg(fromConnectorItem->attachedToID()).arg(fromConnectorItem->connectorSharedID())
					.arg(toConnectorItem->attachedToID()).arg(toConnectorItem->connectorSharedID());
				if (alreadyList.contains(already)) continue;

				alreadyList.append(already);

				extendChangeConnectionCommand(BaseCommand::CrossView, fromConnectorItem, toConnectorItem,
											ViewLayer::specFromID(toConnectorItem->attachedToViewLayerID()),
											false, parentCommand);
			}
		}
	}
}

void SketchWidget::changePinLabelsSlot(ItemBase * itemBase, bool singleRow)
{
	itemBase = this->findItem(itemBase->id());
	if (itemBase == NULL) return;
	if (itemBase->viewIdentifier() != ViewIdentifierClass::SchematicView) return;

	PaletteItem * paletteItem = qobject_cast<PaletteItem *>(itemBase->layerKinChief());
	if (paletteItem == NULL) return;

	if (qobject_cast<Dip *>(itemBase)) {
		paletteItem->changePinLabels(singleRow, true);
		paletteItem->resetConnectors();
	}
	else if (qobject_cast<MysteryPart *>(itemBase)) {
		paletteItem->changePinLabels(singleRow, false);
	}
	else {
		bool hasLocal = false;
		QStringList labels = paletteItem->getPinLabels(hasLocal);
		if (labels.count() == 0) return;

		// part was formerly a mystery part or generic ic ...
		QHash<QString, QString> properties = itemBase->modelPart()->properties();
		bool hasLayout = false;
		bool sip = false;
		foreach (QString key, properties.keys()) {
			QString value = properties.value(key);
			if (key.compare("layout", Qt::CaseInsensitive) == 0) {
				// was a mystery part
				hasLayout = true;
				break;
			}

			if (key.compare("package") == 0) {
				// was a generic ic
				QString svg;
				sip = value.contains("sip", Qt::CaseInsensitive);		
			}
		}

		QString svg;
		if (hasLayout) {
			svg = MysteryPart::makeSchematicSvg(labels, false);
		}
		else if (sip) {
			svg = MysteryPart::makeSchematicSvg(labels, true);
		}
		else {
			svg = Dip::makeSchematicSvg(labels);
		}
		paletteItem->loadExtraRenderer(svg, false);
		if (!hasLayout && !sip) {
			paletteItem->resetConnectors();
		}
	}

}

void SketchWidget::changePinLabels(ItemBase * itemBase, bool singleRow)
{
	emit changePinLabelsSignal(itemBase, singleRow);
	changePinLabelsSlot(itemBase, singleRow);
}

void SketchWidget::renamePins(ItemBase * itemBase, const QStringList & oldLabels, const QStringList & newLabels, bool singleRow)
{
	QUndoCommand * command = new RenamePinsCommand(this, itemBase->id(), oldLabels, newLabels, singleRow, NULL);
	command->setText(tr("change pin labels"));
	m_undoStack->waitPush(command, 10);
}

void SketchWidget::renamePins(long id, const QStringList & labels, bool singleRow)
{
	ItemBase * itemBase = findItem(id);
	if (itemBase == NULL) return;

	PaletteItem * paletteItem = qobject_cast<PaletteItem *>(itemBase->layerKinChief());
	if (paletteItem == NULL) return;

	paletteItem->renamePins(labels, singleRow);
}

bool SketchWidget::checkUpdateRatsnest(QList<ConnectorItem *> & connectorItems) {
	foreach (ConnectorItem * ci, m_ratsnestUpdateConnect) {
		if (ci == NULL) continue;
		if (connectorItems.contains(ci)) return true;
	}
	foreach (ConnectorItem * ci, m_ratsnestUpdateDisconnect) {
		if (ci == NULL) continue;
		if (connectorItems.contains(ci)) return true;
	}

	return false;
}

void SketchWidget::getRatsnestColor(QColor & color) 
{
	//RatsnestColors::reset(m_viewIdentifier);
	color = RatsnestColors::netColor(m_viewIdentifier);
}

VirtualWire * SketchWidget::makeOneRatsnestWire(ConnectorItem * source, ConnectorItem * dest, bool routed, QColor color) {
	if (source->attachedTo() == dest->attachedTo()) {
		if (source == dest) return NULL;

		if (source->bus() == dest->bus() && dest->bus() != NULL) {
			return NULL;				// don't draw a wire within the same part on the same bus
		}
	}
	
	long newID = ItemBase::getNextID();

	ViewGeometry viewGeometry;
	makeRatsnestViewGeometry(viewGeometry, source, dest);
	viewGeometry.setRouted(routed);

	if (viewIdentifier() == ViewIdentifierClass::PCBView) {
		source->debugInfo("making rat src");
		dest->debugInfo("making rat dst");
	}

	// ratsnest only added to one view
	ItemBase * newItemBase = addItem(m_paletteModel->retrieveModelPart(ModuleIDNames::WireModuleIDName), source->attachedTo()->viewLayerSpec(), BaseCommand::SingleView, viewGeometry, newID, -1, NULL, NULL);		
	VirtualWire * wire = qobject_cast<VirtualWire *>(newItemBase);
	ConnectorItem * connector0 = wire->connector0();
	source->tempConnectTo(connector0, false);
	connector0->tempConnectTo(source, false);

	ConnectorItem * connector1 = wire->connector1();
	dest->tempConnectTo(connector1, false);
	connector1->tempConnectTo(dest, false);

	if (!source->attachedTo()->isVisible() || !dest->attachedTo()->isVisible()) {
		wire->setVisible(false);
	}

	wire->setColor(color, getRatsnestOpacity());
	wire->setWireWidth(getRatsnestWidth(), this, 1.0);

	return wire;
}

double SketchWidget::getRatsnestOpacity() {
	return 1.0;
}

double SketchWidget::getRatsnestWidth() {
	return 1.0;
}

void SketchWidget::makeRatsnestViewGeometry(ViewGeometry & viewGeometry, ConnectorItem * source, ConnectorItem * dest) 
{
	QPointF fromPos = source->sceneAdjustedTerminalPoint(NULL);
	viewGeometry.setLoc(fromPos);
	QPointF toPos = dest->sceneAdjustedTerminalPoint(NULL);
	QLineF line(0, 0, toPos.x() - fromPos.x(), toPos.y() - fromPos.y());
	viewGeometry.setLine(line);
	viewGeometry.setWireFlags(ViewGeometry::RatsnestFlag);
}

const QString & SketchWidget::traceColor(ViewLayer::ViewLayerSpec) {
	return ___emptyString___;
}

double SketchWidget::getTraceWidth() {
	return 1;
}

void SketchWidget::setAnyInRotation()
{
	m_anyInRotation = true;
}

void SketchWidget::removeWire(Wire * w, QList<ConnectorItem *> & ends, QList<Wire *> & done, QUndoCommand * parentCommand) 
{
	QList<Wire *> chained;
	w->collectChained(chained, ends);
	makeWiresChangeConnectionCommands(chained, parentCommand);
	foreach (Wire * c, chained) {
		makeDeleteItemCommand(c, BaseCommand::CrossView, parentCommand);
		done.append(c);
	}
}

void SketchWidget::collectRatsnestSlot(QList<SketchWidget *> & foreignSketchWidgets)
{
	foreignSketchWidgets << this;
}

void SketchWidget::setGroundFillSeed(long id, const QString & connectorID, bool seed)
{
	ItemBase * itemBase = findItem(id);
	if (itemBase == NULL) return;

	ConnectorItem * connectorItem = findConnectorItem(itemBase, connectorID, ViewLayer::specFromID(itemBase->viewLayerID()));
	if (connectorItem == NULL) return;

	connectorItem->setGroundFillSeed(seed);
}

