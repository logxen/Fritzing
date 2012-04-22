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

#include "paletteitembase.h"
#include "../sketch/infographicsview.h"
#include "../debugdialog.h"
#include "../fsvgrenderer.h"
#include "../svg/svgfilesplitter.h"
#include "../layerattributes.h"
#include "layerkinpaletteitem.h"
#include "../connectors/connectoritem.h"
#include "../connectors/svgidlayer.h"
#include "wire.h"
#include "partlabel.h"
#include "../utils/focusoutcombobox.h"
#include "../utils/textutils.h"
#include "../utils/graphicsutils.h"
#include "../utils/cursormaster.h"

#include <QBrush>
#include <QPen>
#include <QColor>
#include <QDir>
#include <QMessageBox>
#include <QLineEdit>
#include <QApplication>
#include <QEvent>
#include <qmath.h>

static QPointF RotationCenter;
static QPointF RotationAxis;
static QTransform OriginalTransform;


PaletteItemBase::PaletteItemBase(ModelPart * modelPart, ViewIdentifierClass::ViewIdentifier viewIdentifier, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu ) :
	ItemBase(modelPart, viewIdentifier, viewGeometry, id, itemMenu)
{
	m_inRotation = m_syncSelected = false;
	m_offset.setX(0);
	m_offset.setY(0);
 	m_blockItemSelectedChange = false;
	this->setPos(viewGeometry.loc());
    setFlag(QGraphicsItem::ItemIsSelectable, true);
	setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
	setAcceptHoverEvents(true);

	if (hasPartNumberProperty()) {
		QString savedValue = modelPart->prop(ModelPartShared::PartNumberPropertyName).toString();
		if (savedValue.isEmpty()) {
			savedValue = modelPart->properties().value(ModelPartShared::PartNumberPropertyName, "");
			if (!savedValue.isEmpty()) {
				modelPart->setProp(ModelPartShared::PartNumberPropertyName, savedValue);
			}
		}
	}
}

QRectF PaletteItemBase::boundingRectWithoutLegs() const
{
	return QRectF(0, 0, m_size.width(), m_size.height());
}

QRectF PaletteItemBase::boundingRect() const
{
	if (!hasRubberBandLeg()) {
		return QRectF(0, 0, m_size.width(), m_size.height());
	}

	//debugInfo(QString("bounding rect %1 %2 %3 %4").arg(r.left()).arg(r.top()).arg(r.width()).arg(r.height()));
	return shape().controlPointRect();
}

QPainterPath PaletteItemBase::hoverShape() const
{
	// TODO: figure out real shape of svg
    QPainterPath path;
    path.addRect(0, 0, m_size.width(), m_size.height());

	if (!hasRubberBandLeg()) return path;

	foreach (ConnectorItem * connectorItem, cachedConnectorItemsConst()) {
		if (connectorItem->hasRubberBandLeg()) {
			path.addPath(connectorItem->mapToParent(connectorItem->hoverShape()));
		}
	}

	path.setFillRule(Qt::WindingFill);
	return path;
}

QPainterPath PaletteItemBase::shape() const
{
	// TODO: figure out real shape of svg
    QPainterPath path;
    path.addRect(0, 0, m_size.width(), m_size.height());

	if (!hasRubberBandLeg()) return path;

	foreach (ConnectorItem * connectorItem, cachedConnectorItemsConst()) {
		if (connectorItem->hasRubberBandLeg()) {
			path.addPath(connectorItem->mapToParent(connectorItem->shape()));
		}
	}

	path.setFillRule(Qt::WindingFill);
	return path;
}

void PaletteItemBase::saveGeometry() {
	m_viewGeometry.setLoc(this->pos());
	m_viewGeometry.setSelected(this->isSelected());
	m_viewGeometry.setZ(this->zValue());
}

bool PaletteItemBase::itemMoved() {
	return (this->pos() != m_viewGeometry.loc());
}

void PaletteItemBase::moveItem(ViewGeometry & viewGeometry) {
	this->setPos(viewGeometry.loc());
	updateConnections();
}

void PaletteItemBase::saveInstanceLocation(QXmlStreamWriter & streamWriter)
{
	saveLocAndTransform(streamWriter);
}

void PaletteItemBase::syncKinSelection(bool selected, PaletteItemBase * originator) {
	Q_UNUSED(originator);
	m_syncSelected = selected;
}

void PaletteItemBase::syncKinMoved(QPointF offset, QPointF newPos) {
	Q_UNUSED(offset);
	Q_UNUSED(newPos);
}


QPointF PaletteItemBase::syncMoved() {
	return m_syncMoved;
}

void PaletteItemBase::blockItemSelectedChange(bool selected) {
	m_blockItemSelectedChange = true;
	m_blockItemSelectedValue = selected;
}

bool PaletteItemBase::syncSelected() {
	return m_syncSelected;
}

void PaletteItemBase::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	if (m_hidden) return;

	ItemBase::paint(painter, option, widget);
}

void PaletteItemBase::paintHover(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	if (m_hidden) return;

	if (!hasRubberBandLeg()) {
		ItemBase::paintHover(painter, option, widget);
		return;
	}

	QPainterPath path;
	path.addRect(0, 0, m_size.width(), m_size.height());
	ItemBase::paintHover(painter, option, widget, path);
}

void PaletteItemBase::paintSelected(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	if (m_hidden) return;

	if (!hasRubberBandLeg()) {
		ItemBase::paintSelected(painter, option, widget);
		return;
	}

	QRectF r(0, 0, m_size.width(), m_size.height());
	GraphicsUtils::qt_graphicsItem_highlightSelected(painter, option, r, QPainterPath());
}

void PaletteItemBase::mousePressConnectorEvent(ConnectorItem * connectorItem, QGraphicsSceneMouseEvent * event) {
	InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView != NULL) {
		infoGraphicsView->mousePressConnectorEvent(connectorItem, event);
	}
}

bool PaletteItemBase::acceptsMousePressConnectorEvent(ConnectorItem *, QGraphicsSceneMouseEvent * event) {
	Q_UNUSED(event);
	
	//if (m_viewIdentifier != ViewIdentifierClass::PCBView) {
		return true;
	//}
}


void PaletteItemBase::mousePressEvent(PaletteItemBase * originalItem, QGraphicsSceneMouseEvent *event)
{
	Q_UNUSED(originalItem);

	m_inRotation = false;

	QPointF corner;
	if (freeRotationAllowed(event->modifiers()) && inRotationLocation(event->scenePos(), event->modifiers(), corner)) {
		this->saveGeometry();
		m_inRotation = true;
		RotationCenter = mapToScene(this->boundingRectWithoutLegs().center());
		RotationAxis = event->scenePos(); 
		OriginalTransform = this->transform();
		/*
		DebugDialog::debug(QString("%11:in rotation:%1,%2 a:%3,%4 t:%5,%6,%7,%8 %9,%10")
			.arg(RotationCenter.x()).arg(RotationCenter.y())
			.arg(RotationAxis.x()).arg(RotationAxis.y())
			.arg(OriginalTransform.m11()).arg(OriginalTransform.m12()).arg(OriginalTransform.m21()).arg(OriginalTransform.m22())
			.arg(OriginalTransform.dx()).arg(OriginalTransform.dy())
			.arg((long) this, 0, 16)
			);
			*/
		this->debugInfo("in rotation");
		InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
		if (infoGraphicsView) infoGraphicsView->setAnyInRotation();
		return;
	}

	ItemBase::mousePressEvent(event);
	if (canFindConnectorsUnder()) {
		foreach (ConnectorItem * connectorItem, cachedConnectorItems()) {
			connectorItem->setOverConnectorItem(NULL);
		}
	}
}

void PaletteItemBase::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	if (!m_inRotation) {
		ItemBase::mouseMoveEvent(event);
		return;
	}

	// TODO: doesn't account for scaling
	// see http://www.gamedev.net/topic/441695-transform-matrix-decomposition/
	double originalAngle = atan2(OriginalTransform.m12(), OriginalTransform.m11()) * 180 / M_PI;
	double a1 = atan2(RotationAxis.y() - RotationCenter.y(), RotationAxis.x() - RotationCenter.x());
	double a2 = atan2(event->scenePos().y() - RotationCenter.y(), event->scenePos().x() - RotationCenter.x());

	double deltaAngle = (a2 - a1) * 180 / M_PI;
	//DebugDialog::debug(QString("original:%1 delta:%2").arg(originalAngle).arg(deltaAngle));
	switch (m_viewIdentifier) {
		case ViewIdentifierClass::BreadboardView:
		case ViewIdentifierClass::PCBView:
			{
				double nearest = qRound((originalAngle + deltaAngle) / 45) * 45;
				if (qAbs(originalAngle + deltaAngle - nearest) < 6) {
					deltaAngle = nearest - originalAngle;
					//DebugDialog::debug(QString("\tdelta angle %1").arg(deltaAngle));
				}
			}
			break;
		case ViewIdentifierClass::SchematicView:
			{
				double nearest = qRound((originalAngle + deltaAngle) / 90) * 90;
				deltaAngle = nearest - originalAngle;
			}
			break;
		default:
			return;
	}

	ItemBase * chief = layerKinChief();
	// restore viewGeometry to original angle
	chief->getViewGeometry().setTransform(OriginalTransform);
	foreach (ItemBase * itemBase, chief->layerKin()) {
		itemBase->getViewGeometry().setTransform(OriginalTransform);
	}

	//DebugDialog::debug(QString("rotating item %1 da:%2 oa:%3 %4").arg(QTime::currentTime().toString("HH:mm:ss.zzz")).arg(deltaAngle).arg(originalAngle).arg((long) this, 0, 16));
	chief->rotateItem(deltaAngle);
}

void PaletteItemBase::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	if (!m_inRotation) {
		ItemBase::mouseReleaseEvent(event);
		return;
	}

	m_inRotation = false;
	InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView) {
		// TODO: doesn't account for scaling
		// see: http://www.gamedev.net/topic/441695-transform-matrix-decomposition/
		double originalAngle = atan2(OriginalTransform.m12(), OriginalTransform.m11()) * 180 / M_PI;
		double currentAngle = atan2(transform().m12(), transform().m11()) * 180 / M_PI;
		rotateItem(originalAngle - currentAngle);		// put it back; undo command will redo it
		saveGeometry();
		infoGraphicsView->triggerRotate(this->layerKinChief(), currentAngle - originalAngle);
	}
}

bool PaletteItemBase::canFindConnectorsUnder() {
	return true;
}

void PaletteItemBase::findConnectorsUnder() {
	if (!canFindConnectorsUnder()) return;

	foreach (ConnectorItem * connectorItem, cachedConnectorItems()) {
		switch (connectorItem->connector()->connectorType()) {
			case Connector::Female:
			case Connector::Pad:
				continue;
			default:
				break;
		}

		connectorItem->findConnectorUnder(true, false, ConnectorItem::emptyConnectorItemList, false, NULL);
	}
}

bool PaletteItemBase::collectFemaleConnectees(QSet<ItemBase *> & items) {
	bool hasMale = false;
	foreach (ConnectorItem * item, cachedConnectorItems()) {
		if (item->connectorType() == Connector::Male) {
			hasMale = true;
			continue;
		}

		if (item->connectorType() != Connector::Female) continue;

		foreach (ConnectorItem * toConnectorItem, item->connectedToItems()) {
			if (toConnectorItem->attachedToItemType() == ModelPart::Wire) continue;
			if (!toConnectorItem->attachedTo()->isVisible()) continue;

			items.insert(toConnectorItem->attachedTo());
		}
	}

	return hasMale;
}

void PaletteItemBase::collectWireConnectees(QSet<Wire *> & wires) {
	foreach (ConnectorItem * item, cachedConnectorItems()) {
		foreach (ConnectorItem * toConnectorItem, item->connectedToItems()) {
			if (toConnectorItem->attachedToItemType() == ModelPart::Wire) {
				if (toConnectorItem->attachedTo()->isVisible()) {
					wires.insert(qobject_cast<Wire *>(toConnectorItem->attachedTo()));
				}
			}
		}
	}
}

bool PaletteItemBase::setUpImage(ModelPart * modelPart, ViewIdentifierClass::ViewIdentifier viewIdentifier, const LayerHash & viewLayers, ViewLayer::ViewLayerID viewLayerID, ViewLayer::ViewLayerSpec viewLayerSpec, bool doConnectors, LayerAttributes & layerAttributes, QString & error)
{
	FSvgRenderer * renderer = ItemBase::setUpImage(modelPart, viewIdentifier, viewLayerID, viewLayerSpec, layerAttributes, error);
	if (renderer == NULL) {
		return false;
	}

	m_canFlipVertical = layerAttributes.canFlipVertical();
	m_canFlipHorizontal = layerAttributes.canFlipHorizontal();
	m_filename = layerAttributes.filename();
	//DebugDialog::debug(QString("filename %1").arg(m_filename) );
	setSticky(layerAttributes.sticky());
	QString elementID = layerAttributes.layerName();
	setViewLayerID(elementID, viewLayers);

	//DebugDialog::debug(QString("setting layer %1 view:%2 z:%3").arg(modelPart->title()).arg(viewIdentifier).arg(this->z()) );
	this->setZValue(this->z());
	this->setSharedRendererEx(renderer);

	m_svg = true;

	if (doConnectors) {
		setUpConnectors(renderer, modelPart->ignoreTerminalPoints());
	}

	if (!m_viewGeometry.transform().isIdentity()) {
		setTransform(m_viewGeometry.transform());
		update();
	}

	return true;
}

void PaletteItemBase::setSharedRendererEx(FSvgRenderer * newRenderer) {
	if (newRenderer != renderer()) {
		setSharedRenderer(newRenderer);
	}
	else {
		update();
	}
	m_size = newRenderer->defaultSizeF();
}

void PaletteItemBase::setUpConnectors(FSvgRenderer * renderer, bool ignoreTerminalPoints) {
	clearConnectorItemCache();

	if (m_viewIdentifier == ViewIdentifierClass::PCBView && ViewLayer::isNonCopperLayer(m_viewLayerID)) {
		//DebugDialog::debug(QString("skip connectors: %1 vid:%2 vlid:%3")
		//				   .arg(this->title())
		//				   .arg(m_viewIdentifier) 
		//				   .arg(m_viewLayerID)
		//	);
		// don't waste time
		return;
	}

	foreach (Connector * connector, m_modelPart->connectors().values()) {
		if (connector == NULL) continue;

		//DebugDialog::debug(QString("id:%1 vid:%2 vlid:%3")
		//				   .arg(connector->connectorSharedID())
		//				   .arg(m_viewIdentifier) 
		//				   .arg(m_viewLayerID)
		//	);


		SvgIdLayer * svgIdLayer = connector->fullPinInfo(m_viewIdentifier, m_viewLayerID);
		if (svgIdLayer == NULL) {
			DebugDialog::debug(QString("svgidlayer fail %1 vid:%2 vlid:%3").arg(connector->connectorSharedID(), m_viewIdentifier, m_viewLayerID));
			continue;
		}

		bool result = renderer->setUpConnector(svgIdLayer, ignoreTerminalPoints);
		if (!result) {
			DebugDialog::debug(QString("setup connector fail %1 vid:%2 vlid:%3").arg(connector->connectorSharedID(), m_viewIdentifier, m_viewLayerID));
			continue;
		}


		ConnectorItem * connectorItem = newConnectorItem(connector);

		connectorItem->setHybrid(svgIdLayer->m_hybrid);
		connectorItem->setRect(svgIdLayer->m_rect);
		connectorItem->setTerminalPoint(svgIdLayer->m_point);
		connectorItem->setRadius(svgIdLayer->m_radius, svgIdLayer->m_strokeWidth);
		if (!svgIdLayer->m_legId.isEmpty()) {
			m_hasRubberBandLeg = true;
			connectorItem->setRubberBandLeg(QColor(svgIdLayer->m_legColor), svgIdLayer->m_legStrokeWidth, svgIdLayer->m_legLine);
		}

		//DebugDialog::debug(QString("terminal point %1 %2").arg(terminalPoint.x()).arg(terminalPoint.y()) );

	}

	foreach (SvgIdLayer * svgIdLayer, renderer->setUpNonConnectors()) {
		if (svgIdLayer == NULL) continue;

		NonConnectorItem * nonConnectorItem = new NonConnectorItem(this);

		//DebugDialog::debug(	QString("in layer %1 with z %2")
			//.arg(ViewLayer::viewLayerNameFromID(m_viewLayerID))
			//.arg(this->zValue()) );

		nonConnectorItem->setRect(svgIdLayer->m_rect);
		nonConnectorItem->setRadius(svgIdLayer->m_radius, svgIdLayer->m_strokeWidth);
		//DebugDialog::debug(QString("terminal point %1 %2").arg(terminalPoint.x()).arg(terminalPoint.y()) );

		delete svgIdLayer;
	}
}

void PaletteItemBase::connectedMoved(ConnectorItem * from, ConnectorItem * to) {
	if (from->connectorType() != Connector::Female) return;

	// female connectors really only operate in breadboard view
	if (m_viewIdentifier != ViewIdentifierClass::BreadboardView) return;

	QTransform t = this->transform();
	if (t.isRotating()) {
		// map a square and see if it's a diamond
		QRectF test(0, 0, 10, 10);
		QRectF result = t.mapRect(test);
		if (result.width() != test.width() || result.height() != test.height()) {
			return;
		}
	}

	QPointF fromTerminalPoint = from->sceneAdjustedTerminalPoint(to);
	QPointF toTerminalPoint = to->sceneAdjustedTerminalPoint(from);

	if (fromTerminalPoint == toTerminalPoint) return;

	this->setPos(this->pos() + fromTerminalPoint - toTerminalPoint);
	updateConnections();
}

void PaletteItemBase::hoverEnterEvent ( QGraphicsSceneHoverEvent * event ) {
	if (lowerConnectorLayerVisible(this)) {
		DebugDialog::debug("PaletteItemBase::hoverEnterEvent isn't obsolete");
		event->ignore();
		return;
	}

	ItemBase::hoverEnterEvent(event);
	if (event->isAccepted()) {
		if (hasRubberBandLeg()) {
			//DebugDialog::debug("---pab set override cursor");
			CursorMaster::instance()->addCursor(this, cursor());

			bool connected = false;
			foreach (ConnectorItem * connectorItem, cachedConnectorItems()) {
				if (connectorItem->connectionsCount() > 0) {
					connected = true;
					break;
				}
			}
		}

		checkFreeRotation(event->modifiers(), event->scenePos());
	}

}

void PaletteItemBase::hoverMoveEvent ( QGraphicsSceneHoverEvent * event ) {
	checkFreeRotation(event->modifiers(), event->scenePos());
}

void PaletteItemBase::hoverLeaveEvent ( QGraphicsSceneHoverEvent * event ) {
	if (hasRubberBandLeg()) {
		//DebugDialog::debug("------pab restore override cursor");
	}

	CursorMaster::instance()->removeCursor(this);

	ItemBase::hoverLeaveEvent(event);
}

void PaletteItemBase::cursorKeyEvent(Qt::KeyboardModifiers modifiers)
{
	InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	QPoint p = infoGraphicsView->mapFromGlobal(QCursor::pos());
	checkFreeRotation(modifiers, infoGraphicsView->mapToScene(p));
	if (hasRubberBandLeg()) {
		QCursor cursor;
		if (modifiers & altOrMetaModifier()) {
			cursor = *CursorMaster::RubberbandCursor;
		}
		else {
			cursor = *CursorMaster::MoveCursor;
		}
		CursorMaster::instance()->addCursor(this, cursor);
	}
}

void PaletteItemBase::contextMenuEvent(QGraphicsSceneContextMenuEvent *event) {
	if (lowerConnectorLayerVisible(this)) {
		// TODO: I think this is obsolete
		DebugDialog::debug("PaletteItemBase::contextMenuEvent isn't obsolete");
		event->ignore();
		return;
	}

	ItemBase::contextMenuEvent(event);
}


LayerKinPaletteItem *PaletteItemBase::newLayerKinPaletteItem(PaletteItemBase * chief, ModelPart * modelPart, 
															 ViewIdentifierClass::ViewIdentifier viewIdentifier,
															 const ViewGeometry & viewGeometry, long id,
															 ViewLayer::ViewLayerID viewLayerID, 
															 ViewLayer::ViewLayerSpec viewLayerSpec, 
															 QMenu* itemMenu, const LayerHash & viewLayers)
{
	LayerKinPaletteItem *lk = new
                LayerKinPaletteItem(chief, modelPart, viewIdentifier, viewGeometry, id, itemMenu);
	lk->init(viewLayerID, viewLayerSpec, viewLayers);
	return lk;
}

QString PaletteItemBase::retrieveSvg(ViewLayer::ViewLayerID viewLayerID, QHash<QString, QString> & svgHash, bool blackOnly, double dpi) 
{
	QString xmlName = ViewLayer::viewLayerXmlNameFromID(viewLayerID);
	QString path = filename();

	QDomDocument flipDoc;
	if (!getFlipDoc(modelPart(), path, viewLayerID, m_viewLayerSpec, flipDoc)) {
		fixCopper1(modelPart(), path, viewLayerID, m_viewLayerSpec, flipDoc);
	}
	
	//DebugDialog::debug(QString("path: %1").arg(path));

	QString svg = svgHash.value(path + xmlName, "");
	if (!svg.isEmpty()) return svg;

	SvgFileSplitter splitter;

	bool result;
	if (flipDoc.isNull()) {
		result = splitter.split(path, xmlName);
	}
	else {
		QString f = flipDoc.toString(); 
		result = splitter.splitString(f, xmlName);
	}

	if (!result) {
		return "";
	}

	if (hasRubberBandLeg()) {
		foreach (ConnectorItem * connectorItem, cachedConnectorItems()) {
			if (!connectorItem->hasRubberBandLeg()) continue;

			splitter.gReplace(connectorItem->legID(m_viewIdentifier, m_viewLayerID));
		}
	}

	result = splitter.normalize(dpi, xmlName, blackOnly);
	if (!result) {
		return "";
	}
	svg = splitter.elementString(xmlName);
	svgHash.insert(path + xmlName, svg);
	return svg;
}

bool PaletteItemBase::canEditPart() {
	if (itemType() == ModelPart::Part) return true;

	return ItemBase::canEditPart();
}

bool PaletteItemBase::collectExtraInfo(QWidget * parent, const QString & family, const QString & prop, const QString & value, bool swappingEnabled, QString & returnProp, QString & returnValue, QWidget * & returnWidget)
{
	if (prop.compare(ModelPartShared::PartNumberPropertyName, Qt::CaseInsensitive) == 0) {
		returnProp = TranslatedPropertyNames.value(prop);

		QLineEdit * lineEdit = new QLineEdit();
		lineEdit->setEnabled(swappingEnabled);
		QString current = m_modelPart->prop(ModelPartShared::PartNumberPropertyName).toString();
		lineEdit->setText(current);
		connect(lineEdit, SIGNAL(editingFinished()), this, SLOT(partPropertyEntry()));	
		lineEdit->setObjectName("infoViewLineEdit");		
		returnWidget = lineEdit;	
		returnValue = current;
		return true;
	}

	return ItemBase::collectExtraInfo(parent, family, prop, value, swappingEnabled, returnProp, returnValue, returnWidget);
}

void PaletteItemBase::setProp(const QString & prop, const QString & value) 
{	
	if (prop.compare(ModelPartShared::PartNumberPropertyName) == 0) {
		modelPart()->setProp(ModelPartShared::PartNumberPropertyName, value);
		if (m_partLabel) m_partLabel->displayTextsIf();
		return;
	}

	ItemBase::setProp(prop, value);
}

void PaletteItemBase::partPropertyEntry() {
	QLineEdit * lineEdit = qobject_cast<QLineEdit *>(sender());
	if (lineEdit == NULL) return;

	InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView != NULL) {
		infoGraphicsView->setProp(this, ModelPartShared::PartNumberPropertyName, "", m_modelPart->prop(ModelPartShared::PartNumberPropertyName).toString(), lineEdit->text(), true);
	}
}


const QCursor * PaletteItemBase::getCursor(Qt::KeyboardModifiers modifiers)
{
	if (hasRubberBandLeg()) {
		if ((modifiers & altOrMetaModifier())) {
			foreach (ConnectorItem * connectorItem, cachedConnectorItems()) {
				if (connectorItem->connectionsCount() > 0) {
					return CursorMaster::RubberbandCursor;
				}
			}
		}
	}

	//DebugDialog::debug("returning move cursor");
	return CursorMaster::MoveCursor;
}

bool PaletteItemBase::freeRotationAllowed(Qt::KeyboardModifiers modifiers) {
	Q_UNUSED(modifiers);
	//if ((modifiers & altOrMetaModifier()) == 0) return false;
	if (!isSelected()) return false;
	if (this->moveLock()) return false;

	return rotation45Allowed();
}

bool PaletteItemBase::inRotationLocation(QPointF scenePos, Qt::KeyboardModifiers modifiers, QPointF & returnPoint)
{
	if (!freeRotationAllowed(modifiers)) return false;
	if (m_viewIdentifier == ViewIdentifierClass::SchematicView) return false;

	QRectF r = this->boundingRectWithoutLegs();
	QPolygonF polygon;
	polygon.append(mapToScene(r.topLeft()));
	polygon.append(mapToScene(r.topRight()));
	polygon.append(mapToScene(r.bottomRight()));
	polygon.append(mapToScene(r.bottomLeft()));
	foreach (QPointF p, polygon) {
		double dsqd = GraphicsUtils::distanceSqd(p, scenePos);
		if (dsqd < 9) {
			returnPoint = p;
			return true;
		}
	}

	return false;
}

bool PaletteItemBase::inRotation() {
	return m_inRotation;
}

void PaletteItemBase::checkFreeRotation(Qt::KeyboardModifiers modifiers, QPointF scenePos)
{
	if (!freeRotationAllowed(modifiers)) return;

	QPointF returnPoint;
	bool inCorner = inRotationLocation(scenePos, modifiers, returnPoint);

	if (inCorner) {
		CursorMaster::instance()->addCursor(this, *CursorMaster::RotateCursor);
	}
	else {
		CursorMaster::instance()->addCursor(this, cursor());
	}
}

/*

void PaletteItemBase::setPos(const QPointF & pos) {
	ItemBase::setPos(pos);
	DebugDialog::debug(QString("set pos %1 %2, %3").arg(this->id()).arg(pos.x()).arg(pos.y()) );
}

void PaletteItemBase::setPos(double x, double y) {
	ItemBase::setPos(x, y);
}


*/
