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

$Revision: 5952 $:
$Author: cohen@irascible.com $:
$Date: 2012-04-07 15:47:10 -0700 (Sat, 07 Apr 2012) $

********************************************************************/


#include "itembase.h"
#include "partfactory.h"
#include "../debugdialog.h"
#include "../model/modelpart.h"
#include "../connectors/connectoritem.h"
#include "../connectors/connectorshared.h"
#include "../sketch/infographicsview.h"
#include "../connectors/connector.h"
#include "../connectors/bus.h"
#include "partlabel.h"
#include "../layerattributes.h"
#include "../fsvgrenderer.h"
#include "../svg/svgfilesplitter.h"
#include "../svg/svgflattener.h"
#include "../utils/folderutils.h"
#include "../utils/textutils.h"
#include "../utils/graphicsutils.h"
#include "../utils/cursormaster.h"
#include "../utils/familypropertycombobox.h"
#include "../referencemodel/referencemodel.h"

#include <QScrollBar>
#include <QTimer>
#include <QVector>
#include <QSet>
#include <QSettings>
#include <QComboBox>
#include <QBitmap>
#include <qmath.h>

/////////////////////////////////

static QRegExp NumberMatcher;
static QHash<QString, double> NumberMatcherValues;

static const double InactiveOpacity = 0.4;

bool numberValueLessThan(QString v1, QString v2)
{
	return NumberMatcherValues.value(v1, 0) <= NumberMatcherValues.value(v2, 0);
}

static QSvgRenderer MoveLockRenderer;

/////////////////////////////////

class NameTriple {

public:
	NameTriple(const QString & _xmlName, const QString & _viewName, const QString & _naturalName) {
		m_xmlName = _xmlName;
		m_viewName = _viewName;
		m_naturalName = _naturalName;
	}

	QString & xmlName() {
		return m_xmlName;
	}

	QString & viewName() {
		return m_viewName;
	}

	QString & naturalName() {
		return m_naturalName;
	}

protected:
	QString m_xmlName;
	QString m_naturalName;
	QString m_viewName;
};

/////////////////////////////////

const QString ItemBase::ITEMBASE_FONT_PREFIX = "<font size='2'>";
const QString ItemBase::ITEMBASE_FONT_SUFFIX = "</font>";

QHash<QString, QString> ItemBase::TranslatedPropertyNames;

QPointer<ReferenceModel> ItemBase::referenceModel = NULL;

QString ItemBase::partInstanceDefaultTitle;
QList<ItemBase *> ItemBase::emptyList;
QString ItemBase::SvgFilesDir = "svg";

const QColor ItemBase::hoverColor(0,0,0);
const double ItemBase::hoverOpacity = .20;
const QColor ItemBase::connectorHoverColor(0,0,255);
const double ItemBase::connectorHoverOpacity = .40;

const QColor StandardConnectedColor(0, 255, 0);
const QColor StandardUnconnectedColor(255, 0, 0);

QPen ItemBase::normalPen(QColor(255,0,0));
QPen ItemBase::hoverPen(QColor(0, 0, 255));
QPen ItemBase::connectedPen(StandardConnectedColor);
QPen ItemBase::unconnectedPen(StandardUnconnectedColor);
QPen ItemBase::chosenPen(QColor(255,0,0));
QPen ItemBase::equalPotentialPen(QColor(255,255,0));

QBrush ItemBase::normalBrush(QColor(255,0,0));
QBrush ItemBase::hoverBrush(QColor(0,0,255));
QBrush ItemBase::connectedBrush(StandardConnectedColor);
QBrush ItemBase::unconnectedBrush(StandardUnconnectedColor);
QBrush ItemBase::chosenBrush(QColor(255,0,0));
QBrush ItemBase::equalPotentialBrush(QColor(255,255,0));

const double ItemBase::normalConnectorOpacity = 0.4;

static QHash<QString, QStringList> CachedValues;

///////////////////////////////////////////////////

ItemBase::ItemBase( ModelPart* modelPart, ViewIdentifierClass::ViewIdentifier viewIdentifier, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu )
	: QGraphicsSvgItem()
{
	//DebugDialog::debug(QString("itembase %1 %2").arg(id).arg((long) static_cast<QGraphicsItem *>(this), 0, 16));
	m_hasRubberBandLeg = m_moveLock = m_hoverEnterSpaceBarWasPressed = m_spaceBarWasPressed = false;

	m_moveLockItem = NULL;

	m_everVisible = true;

	m_rightClickedConnector = NULL;

	m_partLabel = NULL;
	m_itemMenu = itemMenu;
	m_hoverCount = m_connectorHoverCount = m_connectorHoverCount2 = 0;
	m_viewIdentifier = viewIdentifier;
	m_modelPart = modelPart;
	if (m_modelPart) {
		m_modelPart->addViewItem(this);
	}
	m_id = id;
	m_inactive = m_hidden = false;
	m_sticky = false;
	m_canFlipHorizontal = m_canFlipVertical = false;

	setCursor(*CursorMaster::MoveCursor);

   	m_viewGeometry.set(viewGeometry);
	setAcceptHoverEvents ( true );
	m_zUninitialized = true;
}

ItemBase::~ItemBase() {
	//DebugDialog::debug(QString("deleting itembase %1 %2 %3").arg((long) this, 0, 16).arg(m_id).arg((long) m_modelPart, 0, 16));
	if (m_partLabel) {
		delete m_partLabel;
		m_partLabel = NULL;
	}

	foreach (ConnectorItem * connectorItem, cachedConnectorItems()) {
		foreach (ConnectorItem * toConnectorItem, connectorItem->connectedToItems()) {
			toConnectorItem->tempRemove(connectorItem, true);
		}
	}

	foreach (ItemBase * itemBase, m_stickyList) {
		itemBase->addSticky(this, false);
	}

	if (m_modelPart != NULL) {
		m_modelPart->removeViewItem(this);
	}
}

void ItemBase::setTooltip() {
	if(m_modelPart) {
		QString title = instanceTitle();
		if(!title.isNull() && !title.isEmpty()) {
			setInstanceTitleTooltip(title);
		} else {
			setDefaultTooltip();
		}
	} else {
		setDefaultTooltip();
	}
}

void ItemBase::removeTooltip() {
	this->setToolTip(___emptyString___);
}

bool ItemBase::zLessThan(ItemBase * & p1, ItemBase * & p2)
{
	return p1->z() < p2->z();
}

qint64 ItemBase::getNextID() {
	return ModelPart::nextIndex() * ModelPart::indexMultiplier;								// make sure we leave room for layerkin inbetween
}

qint64 ItemBase::getNextID(qint64 index) {

	qint64 temp = index * ModelPart::indexMultiplier;						// make sure we leave room for layerkin inbetween
	ModelPart::updateIndex(index);
	return temp;
}

QSizeF ItemBase::size() {
	return m_size;
}

qint64 ItemBase::id() const {
 	return m_id;
}

void ItemBase::resetID() {
	m_id = m_modelPart->modelIndex() * ModelPart::indexMultiplier;
}

double ItemBase::z() {
	return getViewGeometry().z();
}

ModelPart * ItemBase::modelPart() {
	return m_modelPart;
}

void ItemBase::setModelPart(ModelPart * modelPart) {
	m_modelPart = modelPart;
}

ModelPartShared * ItemBase::modelPartShared() {
	if (m_modelPart == NULL) return NULL;

	return m_modelPart->modelPartShared();
}

void ItemBase::initNames() {
	if (NumberMatcher.isEmpty()) {
		NumberMatcher.setPattern(QString("(([0-9]+(\\.[0-9]*)?)|\\.[0-9]+)([\\s]*([") + TextUtils::PowerPrefixesString + "]))?");
	}

	if (TranslatedPropertyNames.count() == 0) {
		TranslatedPropertyNames.insert("family", tr("family"));
		TranslatedPropertyNames.insert("type", tr("type"));
		TranslatedPropertyNames.insert("model", tr("model"));
		TranslatedPropertyNames.insert("size", tr("size"));
		TranslatedPropertyNames.insert("color", tr("color"));
		TranslatedPropertyNames.insert("resistance", tr("resistance"));
		TranslatedPropertyNames.insert("capacitance", tr("capacitance"));
		TranslatedPropertyNames.insert("inductance", tr("inductance"));
		TranslatedPropertyNames.insert("voltage", tr("voltage"));
		TranslatedPropertyNames.insert("current", tr("current"));
		TranslatedPropertyNames.insert("power", tr("power"));
		TranslatedPropertyNames.insert("pin spacing", tr("pin spacing"));
		TranslatedPropertyNames.insert("rated power", tr("rated power"));
		TranslatedPropertyNames.insert("rated voltage", tr("rated voltage"));
		TranslatedPropertyNames.insert("rated current", tr("rated current"));
		TranslatedPropertyNames.insert("version", tr("version"));
		TranslatedPropertyNames.insert("package", tr("package"));
		TranslatedPropertyNames.insert("shape", tr("shape"));
		TranslatedPropertyNames.insert("form", tr("form"));
		TranslatedPropertyNames.insert("part number", tr("part number"));
		TranslatedPropertyNames.insert("maximum resistance", tr("maximum resistance"));
		TranslatedPropertyNames.insert("pins", tr("pins"));
		TranslatedPropertyNames.insert("spacing", tr("spacing"));
		TranslatedPropertyNames.insert("frequency", tr("frequency"));
		TranslatedPropertyNames.insert("processor", tr("processor"));
		TranslatedPropertyNames.insert("variant", tr("variant"));
		TranslatedPropertyNames.insert("layers", tr("layers"));
		TranslatedPropertyNames.insert("tolerance", tr("tolerance"));
		TranslatedPropertyNames.insert("descr", tr("descr"));
		TranslatedPropertyNames.insert("filename", tr("filename"));
		TranslatedPropertyNames.insert("title", tr("title"));
		TranslatedPropertyNames.insert("date", tr("date"));
		TranslatedPropertyNames.insert("rev", tr("rev"));
		TranslatedPropertyNames.insert("sheet", tr("sheet"));
		TranslatedPropertyNames.insert("project", tr("project"));

		// TODO: translate more known property names from fzp files and resource xml files

	}

	partInstanceDefaultTitle = tr("Part");

	QSettings settings;
	QString colorName = settings.value("ConnectedColor").toString();
	if (!colorName.isEmpty()) {
		QColor color;
		color.setNamedColor(colorName);
		setConnectedColor(color);
	}

	colorName = settings.value("UnconnectedColor").toString();
	if (!colorName.isEmpty()) {
		QColor color;
		color.setNamedColor(colorName);
		setUnconnectedColor(color);
	}
}

void ItemBase::saveInstance(QXmlStreamWriter & streamWriter) {
	streamWriter.writeStartElement(ViewIdentifierClass::viewIdentifierXmlName(m_viewIdentifier));
	streamWriter.writeAttribute("layer", ViewLayer::viewLayerXmlNameFromID(m_viewLayerID));
	if (m_moveLock) {
		streamWriter.writeAttribute("locked", "true");
	}
	this->saveGeometry();
	writeGeometry(streamWriter);
	if (m_partLabel) {
		m_partLabel->saveInstance(streamWriter);
	}

	bool saveConnectorItems = false;
	foreach (ConnectorItem * connectorItem, cachedConnectorItems()) {
		if (connectorItem->connectionsCount() > 0 || connectorItem->hasRubberBandLeg() || connectorItem->isGroundFillSeed()) {
			saveConnectorItems = true;
			break;
		}
	}

	if (saveConnectorItems) {
		streamWriter.writeStartElement("connectors");
		foreach (ConnectorItem * connectorItem, cachedConnectorItems()) {
			connectorItem->saveInstance(streamWriter);
		}
		streamWriter.writeEndElement();
	}


	streamWriter.writeEndElement();
}

void ItemBase::writeGeometry(QXmlStreamWriter & streamWriter) {
	streamWriter.writeStartElement("geometry");
	streamWriter.writeAttribute("z", QString::number(z()));
	this->saveInstanceLocation(streamWriter);
	// do not write attributes here
	streamWriter.writeEndElement();
}

ViewGeometry & ItemBase::getViewGeometry() {
	return m_viewGeometry;
}

ViewIdentifierClass::ViewIdentifier ItemBase::viewIdentifier() {
	return m_viewIdentifier;
}

QString & ItemBase::viewIdentifierName() {
	return ViewIdentifierClass::viewIdentifierName(m_viewIdentifier);
}

ViewLayer::ViewLayerID ItemBase::viewLayerID() const {
	return m_viewLayerID;
}

void ItemBase::setViewLayerID(const QString & layerName, const LayerHash & viewLayers) {
	//DebugDialog::debug(QString("using z %1").arg(layerName));
	setViewLayerID(ViewLayer::viewLayerIDFromXmlString(layerName), viewLayers);
}

void ItemBase::setViewLayerID(ViewLayer::ViewLayerID viewLayerID, const LayerHash & viewLayers) {
	m_viewLayerID = viewLayerID;
	if (m_zUninitialized) {
   		ViewLayer * viewLayer = viewLayers.value(m_viewLayerID);
   		if (viewLayer != NULL) {
			m_zUninitialized = false;
			if (!viewLayer->alreadyInLayer(m_viewGeometry.z())) {
   				m_viewGeometry.setZ(viewLayer->nextZ());
			}
  		}
  	}

    //DebugDialog::debug(QString("using z: %1 z:%2 lid:%3").arg(title()).arg(m_viewGeometry.z()).arg(m_viewLayerID) );
}

void ItemBase::removeLayerKin() {
}

void ItemBase::hoverEnterConnectorItem(QGraphicsSceneHoverEvent * , ConnectorItem * ) {
	//DebugDialog::debug(QString("hover enter c %1").arg(instanceTitle()));
	hoverEnterConnectorItem();
}

void ItemBase::hoverEnterConnectorItem() {
	//DebugDialog::debug(QString("hover enter c %1").arg(instanceTitle()));
	m_connectorHoverCount++;
	hoverUpdate();
}

void ItemBase::hoverLeaveConnectorItem(QGraphicsSceneHoverEvent * , ConnectorItem * ) {
	hoverLeaveConnectorItem();
}

void ItemBase::hoverMoveConnectorItem(QGraphicsSceneHoverEvent * , ConnectorItem * ) {
}

void ItemBase::hoverLeaveConnectorItem() {
	//DebugDialog::debug(QString("hover leave c %1").arg(instanceTitle()));
	m_connectorHoverCount--;
	hoverUpdate();
}

void ItemBase::clearConnectorHover()
{
	m_connectorHoverCount2 = 0;
	hoverUpdate();
}

void ItemBase::connectorHover(ConnectorItem *, ItemBase *, bool hovering) {
	//DebugDialog::debug(QString("hover c %1 %2").arg(hovering).arg(instanceTitle()));

	if (hovering) {
		m_connectorHoverCount2++;
	}
	else {
		m_connectorHoverCount2--;
	}
	// DebugDialog::debug(QString("m_connectorHoverCount2 %1 %2").arg(instanceTitle()).arg(m_connectorHoverCount2));
	hoverUpdate();
}

void ItemBase::hoverUpdate() {
	this->update();
}

void ItemBase::mousePressConnectorEvent(ConnectorItem *, QGraphicsSceneMouseEvent *) {
}

void ItemBase::mouseDoubleClickConnectorEvent(ConnectorItem *) {
}

void ItemBase::mouseMoveConnectorEvent(ConnectorItem *, QGraphicsSceneMouseEvent *) {
}

void ItemBase::mouseReleaseConnectorEvent(ConnectorItem *, QGraphicsSceneMouseEvent *) {
}

bool ItemBase::filterMousePressConnectorEvent(ConnectorItem *, QGraphicsSceneMouseEvent *) {
	return false;
}

bool ItemBase::acceptsMousePressConnectorEvent(ConnectorItem *, QGraphicsSceneMouseEvent *) {
	return true;
}

bool ItemBase::acceptsMouseReleaseConnectorEvent(ConnectorItem *, QGraphicsSceneMouseEvent *) {
	return false;
}

bool ItemBase::acceptsMouseDoubleClickConnectorEvent(ConnectorItem *, QGraphicsSceneMouseEvent *) {
	return false;
}

bool ItemBase::acceptsMouseMoveConnectorEvent(ConnectorItem *, QGraphicsSceneMouseEvent *) {
	return false;
}

void ItemBase::connectionChange(ConnectorItem * onMe, ConnectorItem * onIt, bool connect) {
	Q_UNUSED(onMe);
	Q_UNUSED(onIt);
	Q_UNUSED(connect);
}

void ItemBase::connectedMoved(ConnectorItem * /* from */, ConnectorItem * /* to */) {
}

ItemBase * ItemBase::extractTopLevelItemBase(QGraphicsItem * item) {
	ItemBase * itemBase = dynamic_cast<ItemBase *>(item);
	if (itemBase == NULL) return NULL;

	if (itemBase->topLevel()) return itemBase;

	return NULL;
}

bool ItemBase::topLevel() {
	return (this == this->layerKinChief());
}

void ItemBase::setHidden(bool hide) {

	m_hidden = hide;
	setAcceptedMouseButtons(m_hidden || m_inactive ? Qt::NoButton : ALLMOUSEBUTTONS);
	setAcceptHoverEvents(!(m_hidden || m_inactive));
	update();
	foreach (QGraphicsItem * item, childItems()) {
		NonConnectorItem * nonconnectorItem = dynamic_cast<NonConnectorItem *>(item);
		if (nonconnectorItem == NULL) continue;

		nonconnectorItem->setHidden(hide);
	}
}

void ItemBase::setInactive(bool inactivate) {

	m_inactive = inactivate;
	setAcceptedMouseButtons(m_hidden || m_inactive ? Qt::NoButton : ALLMOUSEBUTTONS);
	setAcceptHoverEvents(!(m_hidden || m_inactive));
	update();
	foreach (QGraphicsItem * item, childItems()) {
		NonConnectorItem * nonconnectorItem = dynamic_cast<NonConnectorItem *>(item);
		if (nonconnectorItem == NULL) continue;

		nonconnectorItem->setInactive(inactivate);
	}
}

bool ItemBase::hidden() {
	return m_hidden;
}

bool ItemBase::inactive() {
	return m_inactive;
}

void ItemBase::collectConnectors(ConnectorPairHash & connectorHash, SkipCheckFunction skipCheckFunction) {
	// Is this modelpart check obsolete?
	ModelPart * modelPart = this->modelPart();
	if (modelPart == NULL) return;

	// collect all the connectorItem pairs

	foreach (ConnectorItem * fromConnectorItem, cachedConnectorItems()) {
		foreach (ConnectorItem * toConnectorItem, fromConnectorItem->connectedToItems()) {
			if (skipCheckFunction && skipCheckFunction(toConnectorItem)) continue;

			connectorHash.insert(fromConnectorItem, toConnectorItem);
		}

		ConnectorItem * crossConnectorItem = fromConnectorItem->getCrossLayerConnectorItem();
		if (crossConnectorItem == NULL) continue;

		foreach (ConnectorItem * toConnectorItem, crossConnectorItem->connectedToItems()) {
			if (skipCheckFunction && skipCheckFunction(toConnectorItem)) continue;

			connectorHash.insert(crossConnectorItem, toConnectorItem);
		}
	}
}

ConnectorItem * ItemBase::findConnectorItemWithSharedID(const QString & connectorID, ViewLayer::ViewLayerSpec viewLayerSpec)  {
	foreach (ConnectorItem * connectorItem, cachedConnectorItems()) {
		if (connectorID.compare(connectorItem->connectorSharedID()) == 0) {
			return connectorItem->chooseFromSpec(viewLayerSpec);
		}
	}

	return NULL;
}

void ItemBase::hoverEnterEvent ( QGraphicsSceneHoverEvent * event ) {
	//DebugDialog::debug(QString("hover enter %1").arg(instanceTitle()));
	InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView != NULL && infoGraphicsView->spaceBarIsPressed()) {
		m_hoverEnterSpaceBarWasPressed = true;
		event->ignore();
		return;
	}

	m_hoverEnterSpaceBarWasPressed = false;
	m_hoverCount++;
	hoverUpdate();
	if (infoGraphicsView != NULL) {
		infoGraphicsView->hoverEnterItem(event, this);
	}
}

void ItemBase::hoverLeaveEvent ( QGraphicsSceneHoverEvent * event ) {
	//DebugDialog::debug(QString("hover leave %1").arg(instanceTitle()));
	if (m_hoverEnterSpaceBarWasPressed) {
		event->ignore();
		return;
	}

	m_hoverCount--;
	hoverUpdate();


	InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView != NULL) {
		infoGraphicsView->hoverLeaveItem(event, this);
	}
}

void ItemBase::updateConnections() {
}

void ItemBase::updateConnections(ConnectorItem * connectorItem) {
	connectorItem->attachedMoved();
}

const QString & ItemBase::title() const {
	if (m_modelPart == NULL) return ___emptyString___;

	return m_modelPart->title();
}

bool ItemBase::getRatsnest() {
	return m_viewGeometry.getRatsnest();
}

const QHash<QString, QPointer<Bus> > & ItemBase::buses() {
	if (m_modelPart != NULL) return m_modelPart->buses();

	return Bus::___emptyBusList___;
}

void ItemBase::busConnectorItems(class Bus * bus, QList<class ConnectorItem *> & items) {
	if (bus == NULL) return;

	foreach (Connector * connector, bus->connectors()) {
		QList< QPointer<ConnectorItem> > allConnectorItems = connector->viewItems();
		foreach (ConnectorItem * connectorItem, allConnectorItems) {
			if (connectorItem != NULL) {
				//connectorItem->debugInfo(QString("on the bus %1").arg((long) connector, 0, 16));
				if (connectorItem->attachedTo() == this) {
					items.append(connectorItem);
				}
			}
			//else {
				//DebugDialog::debug(QString("off the bus %1").arg((long) connector, 0, 16));
			//}
		}
	}

}

int ItemBase::itemType() const
{
	if (m_modelPart == NULL) return ModelPart::Unknown;

	return m_modelPart->itemType();
}

bool ItemBase::inHover() {
	return (!m_inactive && (m_connectorHoverCount > 0 || m_hoverCount > 0 || m_connectorHoverCount2 > 0));
}

void ItemBase::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
	if (inHover()) {
		//DebugDialog::debug(QString("chc:%1 hc:%2 chc2:%3").arg(m_connectorHoverCount).arg(m_hoverCount).arg(m_connectorHoverCount2));
		layerKinChief()->paintHover(painter, option, widget);
	}
	//else {
		//DebugDialog::debug("no hover");
	//}

	if (m_inactive) {
		painter->save();
		painter->setOpacity(InactiveOpacity);
	}

	paintBody(painter, option, widget);

	if (option->state & QStyle::State_Selected) {	
		layerKinChief()->paintSelected(painter, option, widget);
    }	

	if (m_inactive) {
		painter->restore();
	}
}

void ItemBase::paintBody(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	Q_UNUSED(option);
	Q_UNUSED(widget);

	// Qt's SVG renderer's defaultSize is not correct when the svg has a fractional pixel size
	renderer()->render(painter, boundingRectWithoutLegs());
}

void ItemBase::paintHover(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	paintHover(painter, option, widget, hoverShape());
}

void ItemBase::paintSelected(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	Q_UNUSED(widget);
	GraphicsUtils::qt_graphicsItem_highlightSelected(painter, option, boundingRect(), hoverShape());
}

void ItemBase::paintHover(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget, const QPainterPath & shape)
{
	Q_UNUSED(widget);
	Q_UNUSED(option);
	painter->save();
	if (m_connectorHoverCount > 0 || m_connectorHoverCount2 > 0) {
		painter->setOpacity(connectorHoverOpacity);
		painter->fillPath(shape, QBrush(connectorHoverColor));
	}
	else {
		painter->setOpacity(hoverOpacity);
		painter->fillPath(shape, QBrush(hoverColor));
	}
	painter->restore();
}

void ItemBase::mousePressEvent(QGraphicsSceneMouseEvent *event) {
	InfoGraphicsView *infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView != NULL && infoGraphicsView->spaceBarIsPressed()) {
		event->ignore();
		return;
	}

	//scene()->setItemIndexMethod(QGraphicsScene::NoIndex);
	//setCacheMode(QGraphicsItem::DeviceCoordinateCache);
	QGraphicsSvgItem::mousePressEvent(event);
}

void ItemBase::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	m_rightClickedConnector = NULL;
	// calling parent class so that multiple selection will work
	// haven't yet discovered any nasty side-effect
	QGraphicsSvgItem::mouseReleaseEvent(event);

	//scene()->setItemIndexMethod(QGraphicsScene::BspTreeIndex);
	// setCacheMode(QGraphicsItem::NoCache);

}

void ItemBase::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	Q_UNUSED(event);
	return;
}

void ItemBase::setItemPos(QPointF & loc) {
	setPos(loc);
}

bool ItemBase::stickyEnabled() {
	return true;
}

bool ItemBase::sticky() {
	return m_sticky;
}

void ItemBase::setSticky(bool s)
{
	m_sticky = s;
}

void ItemBase::addSticky(ItemBase * sticky, bool stickem) {
	sticky = sticky->layerKinChief();
	//this->debugInfo(QString("add sticky %1:").arg(stickem));
	//sticky->debugInfo(QString("  to"));
	if (stickem) {
		if (!m_sticky) {
			foreach (ItemBase * oldstickingTo, m_stickyList.values()) {
				if (oldstickingTo == sticky) continue;

				oldstickingTo->addSticky(this, false);
			}
			m_stickyList.clear();
		}
		m_stickyList.insert(sticky->id(), sticky);
	}
	else {
		m_stickyList.remove(sticky->id());
	}
}


ItemBase * ItemBase::stickingTo() {
	if (m_sticky) return NULL;

	if (m_stickyList.count() < 1) return NULL;

	if (m_stickyList.count() > 1) {
		DebugDialog::debug(QString("error: sticky list > 1 %1").arg(title()));
	}

	return *m_stickyList.begin();
}

QList< QPointer<ItemBase> > ItemBase::stickyList() {
	return m_stickyList.values();
}

bool ItemBase::alreadySticking(ItemBase * itemBase) {
	return m_stickyList.value(itemBase->layerKinChief()->id(), NULL) != NULL;
}

ConnectorItem* ItemBase::newConnectorItem(Connector *connector) 
{
	return newConnectorItem(this, connector);
}

ConnectorItem* ItemBase::newConnectorItem(ItemBase * layerKin, Connector *connector) 
{
	return new ConnectorItem(connector, layerKin);
}

ConnectorItem * ItemBase::anyConnectorItem() {
	foreach (ConnectorItem * connectorItem, cachedConnectorItems()) {
		return connectorItem;
	}

	return NULL;
}


const QString & ItemBase::instanceTitle() const {
	if (m_modelPart) {
		return m_modelPart->instanceTitle();
	}
	return ___emptyString___;
}

void ItemBase::setInstanceTitle(const QString &title) {
	setInstanceTitleAux(title);
	if (m_partLabel) {
		m_partLabel->setPlainText(title);
	}
}

void ItemBase::updatePartLabelInstanceTitle() {
	if (m_partLabel) {
		m_partLabel->setPlainText(instanceTitle());
	}
}

void ItemBase::setInstanceTitleAux(const QString &title)
{
	if (m_modelPart) {
		m_modelPart->setInstanceTitle(title);
	}
	setInstanceTitleTooltip(title);

//	InfoGraphicsView *infographics = InfoGraphicsView::getInfoGraphicsView(this);
//	if (infographics != NULL) {
//		infographics->setItemTooltip(this, title);
//	}
}

QString ItemBase::label() {
	if(m_modelPart) {
		return m_modelPart->label();
	}
	return ___emptyString___;
}

void ItemBase::updateTooltip() {
	setInstanceTitleTooltip(instanceTitle());
}

void ItemBase::setInstanceTitleTooltip(const QString &text) {
	setToolTip("<b>"+text+"</b><br></br>" + ITEMBASE_FONT_PREFIX + title()+ ITEMBASE_FONT_SUFFIX);
}

void ItemBase::setDefaultTooltip() {
	if (m_modelPart) {
		if (m_viewIdentifier == ViewIdentifierClass::IconView) {
			QString base = ITEMBASE_FONT_PREFIX + "%1" + ITEMBASE_FONT_SUFFIX;
			if(m_modelPart->itemType() != ModelPart::Wire) {
				this->setToolTip(base.arg(m_modelPart->title()));
			} else {
				this->setToolTip(base.arg(m_modelPart->title() + " (" + m_modelPart->moduleID() + ")"));
			}
			return;
		}

		QString title = ItemBase::partInstanceDefaultTitle;
		QString inst = instanceTitle();
		if(!inst.isNull() && !inst.isEmpty()) {
			title = inst;
		} else {
			QString defaultTitle = label();
			if(!defaultTitle.isNull() && !defaultTitle.isEmpty()) {
				title = defaultTitle;
			}
		}
		ensureUniqueTitle(title, false);
		setInstanceTitleTooltip(instanceTitle());
	}
}

void ItemBase::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
	if ((acceptedMouseButtons() & Qt::RightButton) == 0) {
		event->ignore();
		return;
	}

	if (m_hidden || m_inactive) {
		event->ignore();
		return;
	}

    scene()->clearSelection();
    setSelected(true);

    if (m_itemMenu != NULL) {
		m_rightClickedConnector = NULL;
		foreach (QGraphicsItem * item, scene()->items(event->scenePos())) {
			ConnectorItem * connectorItem = dynamic_cast<ConnectorItem *>(item);
			if (connectorItem == NULL) continue;

			if (connectorItem->attachedTo() == this) {
				m_rightClickedConnector = connectorItem;
				break;
			}
		}

     	m_itemMenu->exec(event->screenPos());
	}
}

bool ItemBase::hasConnectors() {
	return cachedConnectorItems().count() > 0;
}

bool ItemBase::hasNonConnectors() {
	foreach (QGraphicsItem * childItem, childItems()) {
		if (dynamic_cast<NonConnectorItem *>(childItem) != NULL) return true;
	}

	return false;
}

bool ItemBase::canFlip(Qt::Orientations orientations) {
	bool result = true;
	if (orientations & Qt::Horizontal) {
		result = result && m_canFlipHorizontal;
	}
	if (orientations & Qt::Vertical) {
		result = result && m_canFlipVertical;
	}
	return result;
}

bool ItemBase::canFlipHorizontal() {
	return m_canFlipHorizontal && !m_moveLock;
}

void ItemBase::setCanFlipHorizontal(bool cf) {
	m_canFlipHorizontal = cf;
}

bool ItemBase::canFlipVertical() {
	return m_canFlipVertical && !m_moveLock;
}

void ItemBase::setCanFlipVertical(bool cf) {
	m_canFlipVertical = cf;
}

bool ItemBase::rotationAllowed() {
	return !m_moveLock;
}

bool ItemBase::rotation45Allowed() {
	return !m_moveLock;
}

void ItemBase::clearModelPart() {
	m_modelPart = NULL;
}

void ItemBase::hidePartLabel() 
{
	InfoGraphicsView *infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView) infoGraphicsView->hidePartLabel(this);
}

void ItemBase::showPartLabel(bool showIt, ViewLayer* viewLayer) {
	if (m_partLabel) {
		m_partLabel->showLabel(showIt, viewLayer);
	}
}

void ItemBase::partLabelChanged(const QString & newText) {
	// sent from part label after inline edit
	InfoGraphicsView *infographics = InfoGraphicsView::getInfoGraphicsView(this);
	QString oldText = modelPart()->instanceTitle();
	setInstanceTitleAux(newText);
	if (infographics != NULL) {
		infographics->partLabelChanged(this, oldText, newText);
	}
}

bool ItemBase::isPartLabelVisible() {
	if (m_partLabel == NULL) return false;
	if (!m_partLabel->initialized()) return false;
	return m_partLabel->isVisible();
}


void ItemBase::clearPartLabel() {
	m_partLabel = NULL;
}

void ItemBase::restorePartLabel(QDomElement & labelGeometry, ViewLayer::ViewLayerID viewLayerID)
{
	if (m_partLabel) {
		m_partLabel->setPlainText(instanceTitle());
		if (!labelGeometry.isNull()) {
			m_partLabel->restoreLabel(labelGeometry, viewLayerID);
			//m_partLabel->setPlainText(instanceTitle());
		}
	}
}

void ItemBase::movePartLabel(QPointF newPos, QPointF newOffset) {
	if (m_partLabel) {
		m_partLabel->moveLabel(newPos, newOffset);
	}
}

void ItemBase::partLabelSetHidden(bool hide) {
	if (m_partLabel) {
		m_partLabel->setHidden(hide);
	}
}

void ItemBase::partLabelMoved(QPointF oldPos, QPointF oldOffset, QPointF newPos, QPointF newOffset) {
	InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView != NULL) {
		infoGraphicsView->partLabelMoved(this, oldPos, oldOffset, newPos, newOffset);
	}
}

void ItemBase::rotateFlipPartLabel(double degrees, Qt::Orientations orientation)
{
	InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView != NULL) {
		infoGraphicsView->rotateFlipPartLabel(this, degrees, orientation);
	}
}


void ItemBase::doRotateFlipPartLabel(double degrees, Qt::Orientations orientation)
{
	if (m_partLabel) {
		m_partLabel->rotateFlipLabel(degrees, orientation);
	}
}

bool ItemBase::isSwappable() {
	return true;
}

void ItemBase::ensureUniqueTitle(const QString & title, bool force) {
	if (force || instanceTitle().isEmpty() || instanceTitle().isNull()) {
		setInstanceTitle(modelPart()->getNextTitle(title));
	}
}

QVariant ItemBase::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant & value)
{
	switch (change) {
		case QGraphicsItem::ItemSelectedChange:
			if (m_partLabel) {
				m_partLabel->ownerSelected(value.toBool());
			}
			
			break;
		default:
			break;
	}

	return QGraphicsSvgItem::itemChange(change, value);
}

void ItemBase::cleanup() {
}

const QList<ItemBase *> & ItemBase::layerKin() {
	return emptyList;
}

ItemBase * ItemBase::layerKinChief() {
	return this;
}

void ItemBase::rotateItem(double degrees) {
	//DebugDialog::debug(QString("\trotating item %1 %2").arg(degrees).arg((long) this, 0, 16));
	transformItem(QTransform().rotate(degrees));
}

void ItemBase::flipItem(Qt::Orientations orientation) {
	int xScale; int yScale;
	if(orientation == Qt::Vertical) {
		xScale = 1;
		yScale = -1;
	} else if(orientation == Qt::Horizontal) {
		xScale = -1;
		yScale = 1;
	}
	else {
		return;
	}

	transformItem(QTransform().scale(xScale,yScale));
}

void ItemBase::transformItem(const QTransform & currTransf) {
	//DebugDialog::debug(QString("\ttransform item %1").arg((long) this, 0, 16));

	if (m_hasRubberBandLeg) {
		prepareGeometryChange();
	}
	QRectF rect = this->boundingRectWithoutLegs();
	double x = rect.width() / 2.0;
	double y = rect.height() / 2.0;
	QTransform transf = QTransform().translate(-x, -y) * currTransf * QTransform().translate(x, y);
	getViewGeometry().setTransform(getViewGeometry().transform()*transf);
	this->setTransform(getViewGeometry().transform());
	if (!m_hasRubberBandLeg) {
		updateConnections();
	}
	//QTransform t = this->transform();
	//DebugDialog::debug(QString("matrix m11:%1 m12:%2 m21:%3 m22:%4").arg(t.m11()).arg(t.m12()).arg(t.m21()).arg(t.m22()));
	update();
}

void ItemBase::transformItem2(const QMatrix & matrix) {
	QTransform transform(matrix);
	transformItem(transform);
}

void ItemBase::collectWireConnectees(QSet<class Wire *> & wires) {
	Q_UNUSED(wires);
}

bool ItemBase::collectFemaleConnectees(QSet<ItemBase *> & items) {
	Q_UNUSED(items);
	return false;			// means no male connectors
}

void ItemBase::prepareGeometryChange() {
	// made public so it can be invoked from outside ItemBase class

	//debugInfo("itembase prepare geometry change");
	QGraphicsSvgItem::prepareGeometryChange();
}

void ItemBase::saveLocAndTransform(QXmlStreamWriter & streamWriter)
{
	streamWriter.writeAttribute("x", QString::number(m_viewGeometry.loc().x()));
	streamWriter.writeAttribute("y", QString::number(m_viewGeometry.loc().y()));
	GraphicsUtils::saveTransform(streamWriter, m_viewGeometry.transform());
}

FSvgRenderer * ItemBase::setUpImage(ModelPart * modelPart, ViewIdentifierClass::ViewIdentifier viewIdentifier, ViewLayer::ViewLayerID viewLayerID, ViewLayer::ViewLayerSpec viewLayerSpec, LayerAttributes & layerAttributes, QString & error)
{
#ifndef QT_NO_DEBUG
	QTime t;
	t.start();
#endif

    ModelPartShared * modelPartShared = modelPart->modelPartShared();

	if (modelPartShared == NULL) {
		error = tr("model part problem");
		return NULL;
	}
	if (modelPartShared->domDocument() == NULL) {
		error = tr("part xml missing");
		return NULL;
	}

	bool result = layerAttributes.getSvgElementID(modelPartShared->domDocument(), viewIdentifier, viewLayerID);
	if (!result) {
		error = tr("missing xml for view %1 layer %2").arg(viewIdentifier).arg(viewLayerID);
		return NULL;
	}

	//DebugDialog::debug(QString("setting z %1 %2")
		//.arg(this->z())
		//.arg(ViewLayer::viewLayerNameFromID(viewLayerID))  );


	//DebugDialog::debug(QString("set up image elapsed (1) %1").arg(t.elapsed()) );
	FSvgRenderer * renderer = FSvgRenderer::getByModuleID(modelPartShared->moduleID(), viewLayerID);
	if (renderer == NULL) {
		QString filename = getSvgFilename(modelPart, layerAttributes.filename());
		if (filename.isEmpty()) {
			filename = PartFactory::getSvgFilename(modelPart, layerAttributes.filename());
		}

//#ifndef QT_NO_DEBUG
		//DebugDialog::debug(QString("set up image elapsed (2) %1").arg(t.elapsed()) );
//#endif

		if (filename.isEmpty()) {
			//QString deleteme = modelPartShared->domDocument()->toString();
			error = tr("file %1 not found").arg(layerAttributes.filename());
		}
		else {
			renderer = FSvgRenderer::getByFilename(filename, viewLayerID);
			if (renderer == NULL) {
				QStringList connectorIDs, terminalIDs, legIDs;
				QString setColor;
				QString colorElementID;
				switch (viewIdentifier) {
					case ViewIdentifierClass::PCBView:
						colorElementID = ViewLayer::viewLayerXmlNameFromID(viewLayerID);
						switch (viewLayerID) {
							case ViewLayer::Copper0:
								modelPartShared->connectorIDs(viewIdentifier, viewLayerID, connectorIDs, terminalIDs, legIDs);
								setColor = ViewLayer::Copper0Color;
								break;
							case ViewLayer::Copper1:
								modelPartShared->connectorIDs(viewIdentifier, viewLayerID, connectorIDs, terminalIDs, legIDs);
								setColor = ViewLayer::Copper1Color;
								break;
							case ViewLayer::Silkscreen1:
								setColor = ViewLayer::Silkscreen1Color;
								break;
							case ViewLayer::Silkscreen0:
								setColor = ViewLayer::Silkscreen0Color;
								break;
							default:
								break;
						}
						break;
					case ViewIdentifierClass::BreadboardView:
						modelPartShared->connectorIDs(viewIdentifier, viewLayerID, connectorIDs, terminalIDs, legIDs);
						break;
                    default:
                        break;
				}

				bool gotOne = false;
				renderer = new FSvgRenderer();
				QDomDocument flipDoc;
				if (!getFlipDoc(modelPart, filename, viewLayerID, viewLayerSpec, flipDoc)) {
					fixCopper1(modelPart, filename, viewLayerID, viewLayerSpec, flipDoc);
				}
				if (layerAttributes.multiLayer()) {
					// need to treat create "virtual" svg file for each layer
					SvgFileSplitter svgFileSplitter;
					bool result;
					if (flipDoc.isNull()) {
						result = svgFileSplitter.split(filename, layerAttributes.layerName());
					}
					else {
						QString f = flipDoc.toString(); 
						result = svgFileSplitter.splitString(f, layerAttributes.layerName());
					}
					if (result) {
						QByteArray bytes = renderer->loadSvg(svgFileSplitter.byteArray(), filename, connectorIDs, terminalIDs, legIDs, setColor, colorElementID, viewIdentifier == ViewIdentifierClass::PCBView);
						if (!bytes.isEmpty()) {
							gotOne = true;
						}
					}
				}
				else {
//#ifndef QT_NO_DEBUG
//					DebugDialog::debug(QString("set up image elapsed (2.3) %1").arg(t.elapsed()) );
//#endif
					// only one layer, just load it directly
					if (flipDoc.isNull()) {
						layerAttributes.setLoaded(renderer->loadSvg(filename, connectorIDs, terminalIDs, legIDs, setColor, colorElementID, viewIdentifier == ViewIdentifierClass::PCBView));
					}
					else {
						layerAttributes.setLoaded(renderer->loadSvg(flipDoc.toByteArray(), filename, connectorIDs, terminalIDs, legIDs, setColor, colorElementID, viewIdentifier == ViewIdentifierClass::PCBView));
					}
					if (!layerAttributes.loaded().isEmpty()) {
						gotOne = true;
					}
//#ifndef QT_NO_DEBUG
//					DebugDialog::debug(QString("set up image elapsed (2.4) %1").arg(t.elapsed()) );
//#endif
				}
				if (!gotOne) {
					delete renderer;
					error = tr("unable to create renderer for svg %1").arg(filename);
					renderer = NULL;
				}
			}
			//DebugDialog::debug(QString("set up image elapsed (3) %1").arg(t.elapsed()) );

			if (renderer) {
				FSvgRenderer::set(modelPartShared->moduleID(), viewLayerID, renderer);
			}
    	}
	}

	if (renderer) {
		layerAttributes.setFilename(renderer->filename());
	}

	return renderer;
}

QString ItemBase::getSvgFilename(ModelPart * modelPart, const QString & baseName) 
{
	QStringList tempPaths;
	QString postfix = +"/"+ ItemBase::SvgFilesDir +"/%1/"+ baseName;
	if(modelPart->path() != ___emptyString___) {
		QDir dir(modelPart->path());			// is a path to a filename
		dir.cdUp();									// lop off the filename
		dir.cdUp();									// parts root
		tempPaths << dir.absolutePath() + "/" + ItemBase::SvgFilesDir +"/%1/" + baseName;
		tempPaths << FolderUtils::getApplicationSubFolderPath("parts")+postfix;    // some svgs may still be in the fritzing parts folder, though the other svgs are in the user folder
	} 
	else { // for fake models
		tempPaths << FolderUtils::getApplicationSubFolderPath("parts")+postfix;
		tempPaths << FolderUtils::getUserDataStorePath("parts")+postfix;
	}
	tempPaths << ":resources/parts/svg/%1/" + baseName;

		//DebugDialog::debug(QString("got tempPath %1").arg(tempPath));

	QString filename;
	foreach (QString tempPath, tempPaths) {
		foreach (QString possibleFolder, ModelPart::possibleFolders()) {
			filename = tempPath.arg(possibleFolder);
			if (QFileInfo(filename).exists()) {
				if (possibleFolder == "obsolete") {
					DebugDialog::debug(QString("module %1:%2 obsolete svg %3").arg(modelPart->title()).arg(modelPart->moduleID()).arg(filename));
				}
				return filename;
			} 
		}
	}

	return PartFactory::getSvgFilename(modelPart, baseName);
}

void ItemBase::updateConnectionsAux() {
	//DebugDialog::debug("update connections");
	foreach (ConnectorItem * connectorItem, cachedConnectorItems()) {
		updateConnections(connectorItem);
	}
}

ItemBase * ItemBase::lowerConnectorLayerVisible(ItemBase * itemBase) {
	Q_UNUSED(itemBase);
	return NULL;
}

void ItemBase::figureHover() {
}

QString ItemBase::retrieveSvg(ViewLayer::ViewLayerID viewLayerID, QHash<QString, QString> & svgHash, bool blackOnly, double dpi)
{
	Q_UNUSED(viewLayerID);
	Q_UNUSED(svgHash);
	Q_UNUSED(blackOnly);
	Q_UNUSED(dpi);
	return ___emptyString___;
}

bool ItemBase::hasConnections()
{
	foreach (ConnectorItem * connectorItem, cachedConnectorItems()) {
		if (connectorItem->connectionsCount() > 0) return true;
	}

	return false;
}

void ItemBase::getConnectedColor(ConnectorItem *, QBrush * &brush, QPen * &pen, double & opacity, double & negativePenWidth, bool & negativeOffsetRect) {
	brush = &connectedBrush;
	pen = &connectedPen;
	opacity = 0.2;
	negativePenWidth = 0;
	negativeOffsetRect = true;
}

void ItemBase::getNormalColor(ConnectorItem *, QBrush * &brush, QPen * &pen, double & opacity, double & negativePenWidth, bool & negativeOffsetRect) {
	brush = &normalBrush;
	pen = &normalPen;
	opacity = normalConnectorOpacity;
	negativePenWidth = 0;
	negativeOffsetRect = true;
}

void ItemBase::getUnconnectedColor(ConnectorItem *, QBrush * &brush, QPen * &pen, double & opacity, double & negativePenWidth, bool & negativeOffsetRect) {
	brush = &unconnectedBrush;
	pen = &unconnectedPen;
        opacity = 0.3;
	negativePenWidth = 0;
	negativeOffsetRect = true;
}

void ItemBase::getChosenColor(ConnectorItem *, QBrush * &brush, QPen * &pen, double & opacity, double & negativePenWidth, bool & negativeOffsetRect) {
	brush = &chosenBrush;
	pen = &chosenPen;
	opacity = normalConnectorOpacity;
	negativePenWidth = 0;
	negativeOffsetRect = true;
}

void ItemBase::getHoverColor(ConnectorItem *, QBrush * &brush, QPen * &pen, double & opacity, double & negativePenWidth, bool & negativeOffsetRect) {
	brush = &hoverBrush;
	pen = &hoverPen;
	opacity = normalConnectorOpacity;
	negativePenWidth = 0;
	negativeOffsetRect = true;
}

void ItemBase::getEqualPotentialColor(ConnectorItem *, QBrush * &brush, QPen * &pen, double & opacity, double & negativePenWidth, bool & negativeOffsetRect) {
	brush = &equalPotentialBrush;
	pen = &equalPotentialPen;
	opacity = 1.0;
	negativePenWidth = 0;
	negativeOffsetRect = true;
}

void ItemBase::slamZ(double newZ) {
	double z = qFloor(m_viewGeometry.z()) + newZ;
	m_viewGeometry.setZ(z);
	setZValue(z);
}

bool ItemBase::isEverVisible() {
	return m_everVisible;
}

void ItemBase::setEverVisible(bool v) {
	m_everVisible = v;
}

bool ItemBase::connectionIsAllowed(ConnectorItem * other) {
	return ViewLayer::canConnect(this->viewLayerID(), other->attachedToViewLayerID());
}

QString ItemBase::getProperty(const QString & key) {
	if (m_modelPart == NULL) return "";

	QString result = m_modelPart->prop(key).toString();
	if (!result.isEmpty()) return result;

	return m_modelPart->properties().value(key, "");
}

ConnectorItem * ItemBase::rightClickedConnector() {
	return m_rightClickedConnector;
}

QColor ItemBase::connectedColor() {
	return connectedPen.color();
}

QColor ItemBase::unconnectedColor() {
	return unconnectedPen.color();
}

QColor ItemBase::standardConnectedColor() {
	return StandardConnectedColor;
}

QColor ItemBase::standardUnconnectedColor() {
	return StandardUnconnectedColor;
}

void ItemBase::setConnectedColor(QColor & c) {
	connectedPen.setColor(c);
	connectedBrush.setColor(c);
}

void ItemBase::setUnconnectedColor(QColor & c) {
	unconnectedPen.setColor(c);
	unconnectedBrush.setColor(c);
}

QString ItemBase::translatePropertyName(const QString & key) {
	return TranslatedPropertyNames.value(key.toLower(), key);
}

bool ItemBase::canEditPart() {
	return false;
}

bool ItemBase::hasCustomSVG() {
	return false;
}

void ItemBase::setProp(const QString & prop, const QString & value) {
	if (!m_modelPart) return;

	m_modelPart->setProp(prop, value);
}

QString ItemBase::prop(const QString & p)
{
	if (m_modelPart == NULL) return "";

	return m_modelPart->prop(p).toString();
}

bool ItemBase::isObsolete() {
	if (modelPart() == NULL) return false;

	return modelPart()->isObsolete();
}

bool ItemBase::collectExtraInfo(QWidget * parent, const QString & family, const QString & prop, const QString & value, bool swappingEnabled, QString & returnProp, QString & returnValue, QWidget * & returnWidget)
{
	returnWidget = NULL;
	returnProp = ItemBase::translatePropertyName(prop);
	returnValue = value;	

	if (prop.compare("family", Qt::CaseInsensitive) == 0) {
		return true;
	}
	if (prop.compare("id", Qt::CaseInsensitive) == 0) {
		return true;
	}

	QString tempValue = value;
	QStringList values = collectValues(family, prop, tempValue);
	if (values.count() > 1) {
		FamilyPropertyComboBox * comboBox = new FamilyPropertyComboBox(family, prop, parent);
		comboBox->setObjectName("infoViewComboBox");

		comboBox->addItems(values);
		comboBox->setCurrentIndex(comboBox->findText(tempValue));
		comboBox->setEnabled(swappingEnabled);
		connect(comboBox, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(swapEntry(const QString &)));

		returnWidget = comboBox;
		m_propsMap.insert(prop, tempValue);
		return true;
	}
		
	return true;
}

void ItemBase::swapEntry(const QString & text) {
    FamilyPropertyComboBox * comboBox = qobject_cast<FamilyPropertyComboBox *>(sender());
    if (comboBox == NULL) return;

    m_propsMap.insert(comboBox->prop(), text);

    InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
    if (infoGraphicsView != NULL) {
        infoGraphicsView->swap(comboBox->family(), comboBox->prop(), m_propsMap, this);
    }
}

void ItemBase::setReferenceModel(ReferenceModel * rm) {
	referenceModel = rm;
}

QStringList ItemBase::collectValues(const QString & family, const QString & prop, QString & value) {
	Q_UNUSED(value);

	if (referenceModel == NULL) return ___emptyStringList___;

	QStringList values = CachedValues.value(family + prop, QStringList());
	if (values.count() > 0) return values;

	values = referenceModel->values(family, prop);

	// sort values numerically
	NumberMatcherValues.clear();
	bool ok = true;
	foreach(QString opt, values) {
		int ix = NumberMatcher.indexIn(opt);
		if (ix < 0) {
			ok = false;
			break;
		}

		double n = TextUtils::convertFromPowerPrefix(NumberMatcher.cap(1) + NumberMatcher.cap(5), "");
		NumberMatcherValues.insert(opt, n);
	}
	if (ok) {
		qSort(values.begin(), values.end(), numberValueLessThan);
	}

	CachedValues.insert(family + prop, values);
	return values;
}

void ItemBase::prepareProps() {
	m_propsMap.clear();

};

void ItemBase::resetValues(const QString & family, const QString & prop) {
	CachedValues.remove(family + prop);
}

bool ItemBase::hasPartLabel() {
	return true;
}

const QString & ItemBase::filename() {
	return m_filename;
}

void ItemBase::setFilename(const QString & fn) {
	m_filename = fn;
}

ItemBase::PluralType ItemBase::isPlural() {
	return ItemBase::NotSure;
}

ViewLayer::ViewLayerSpec ItemBase::viewLayerSpec() const {
	return m_viewLayerSpec;
}

void ItemBase::setViewLayerSpec(ViewLayer::ViewLayerSpec viewLayerSpec) {
	m_viewLayerSpec = viewLayerSpec;
}

ViewLayer::ViewLayerID ItemBase::partLabelViewLayerID() {
	if (m_partLabel == NULL) return ViewLayer::UnknownLayer;
	if (!m_partLabel->initialized()) return ViewLayer::UnknownLayer;
	return m_partLabel->viewLayerID();
}

QString ItemBase::makePartLabelSvg(bool blackOnly, double dpi, double printerScale) {
	if (m_partLabel == NULL) return "";
	if (!m_partLabel->initialized()) return "";
	return m_partLabel->makeSvg(blackOnly, dpi, printerScale, true);
}

QPointF ItemBase::partLabelScenePos() {
	if (m_partLabel == NULL) return QPointF();
	if (!m_partLabel->initialized()) return QPointF();
	return m_partLabel->scenePos();
}

QRectF ItemBase::partLabelSceneBoundingRect() {
	if (m_partLabel == NULL) return QRectF();
	if (!m_partLabel->initialized()) return QRectF();
	return m_partLabel->sceneBoundingRect();
}

bool ItemBase::getFlipDoc(ModelPart * modelPart, const QString & filename, ViewLayer::ViewLayerID viewLayerID, ViewLayer::ViewLayerSpec viewLayerSpec, QDomDocument & flipDoc)
{
	if (viewLayerSpec == ViewLayer::ThroughHoleThroughTop_OneLayer) {
		if ((viewLayerID == ViewLayer::Copper0) && modelPart->flippedSMD()) {
			SvgFlattener::flipSMDSvg(filename, "", flipDoc, ViewLayer::viewLayerXmlNameFromID(ViewLayer::Copper1), ViewLayer::viewLayerXmlNameFromID(ViewLayer::Copper0), FSvgRenderer::printerScale());
			return true;
		}
		else if ((viewLayerID == ViewLayer::Silkscreen0) && modelPart->flippedSMD()) {
			SvgFlattener::flipSMDSvg(filename, "", flipDoc, ViewLayer::viewLayerXmlNameFromID(ViewLayer::Silkscreen1), ViewLayer::viewLayerXmlNameFromID(ViewLayer::Silkscreen0), FSvgRenderer::printerScale());
			return true;
		}
	}

	return false;
}

bool ItemBase::fixCopper1(ModelPart * modelPart, const QString & filename, ViewLayer::ViewLayerID viewLayerID, ViewLayer::ViewLayerSpec viewLayerSpec, QDomDocument & doc)
{
	Q_UNUSED(viewLayerSpec);
	if (viewLayerID != ViewLayer::Copper1) return false;
	if (!modelPart->needsCopper1()) return false;

	return TextUtils::addCopper1(filename, doc, ViewLayer::viewLayerXmlNameFromID(ViewLayer::Copper0), ViewLayer::viewLayerXmlNameFromID(ViewLayer::Copper1));
}

void ItemBase::calcRotation(QTransform & rotation, QPointF center, ViewGeometry & vg2) 
{
	vg2.setLoc(GraphicsUtils::calcRotation(rotation, center, pos(), boundingRectWithoutLegs().center()));
}

void ItemBase::updateConnectors()
{
	if (!isEverVisible()) return;

	// assumes all connectors have previously been initialized unmarked;
	//int count = 0;
	foreach(ConnectorItem * connectorItem, cachedConnectorItems()) {
		if (connectorItem->marked()) continue;

		connectorItem->restoreColor(false, 0, false);
		//count++;
	}
	//DebugDialog::debug(QString("set up connectors restore:%1").arg(count));
}

const QString & ItemBase::moduleID() {
	if (m_modelPart) return m_modelPart->moduleID();

	return ___emptyString___;
}

bool ItemBase::moveLock() {
	return m_moveLock;
}

void ItemBase::setMoveLock(bool moveLock) 
{
	if (moveLock == m_moveLock) return;

	m_moveLock = moveLock;
	if (moveLock) {
		if (!MoveLockRenderer.isValid()) {
			QString fn(":resources/images/part_lock.svg");
			bool success = MoveLockRenderer.load(fn);
			DebugDialog::debug(QString("movelock load success %1").arg(success));
		}

		m_moveLockItem = new QGraphicsSvgItem();
		m_moveLockItem->setAcceptHoverEvents(false);
		m_moveLockItem->setAcceptedMouseButtons(Qt::NoButton);
		m_moveLockItem->setSharedRenderer(&MoveLockRenderer);
		m_moveLockItem->setPos(0,0);
		m_moveLockItem->setZValue(-99999);
		m_moveLockItem->setParentItem(this);
		m_moveLockItem->setVisible(true);

	}
	else {
		if (m_moveLockItem) {
			delete m_moveLockItem;
			m_moveLockItem = NULL;
		}
	}

	update();
}


void ItemBase::debugInfo(const QString & msg) const
{

#ifndef QT_NO_DEBUG
	DebugDialog::debug(QString("%1 '%2' id:%3 '%4' vid:%9 vlid:%5 spec:%6 flg:%7 gi:%8")
		.arg(msg)
		.arg(this->title())
		.arg(this->id())
		.arg(this->instanceTitle())
		.arg(this->viewLayerID())
		.arg(this->viewLayerSpec())
		.arg(this->wireFlags())
		.arg((long) dynamic_cast<const QGraphicsItem *const>(this), 0, 16)
		.arg(m_viewIdentifier)
	);

	/*
	foreach (ConnectorItem * connectorItem, cachedConnectorItems()) {
		if (connectorItem) connectorItem->debugInfo("\t");
	}
	*/
#else
	Q_UNUSED(msg);
#endif
}

void ItemBase::addedToScene(bool temporary) {
	if (this->scene() && instanceTitle().isEmpty() && !temporary) {
		setTooltip();
	}
}

bool ItemBase::hasPartNumberProperty()
{
	return true;
}

void ItemBase::collectPropsMap(QString & family, QMap<QString, QString> & propsMap) {
	QHash<QString, QString> properties;
	properties = m_modelPart->properties();
	family = properties.value("family", "");
	foreach (QString key, properties.keys()) {
		if (key.compare("family") == 0) continue;
		if (key.compare("id") == 0) continue;

		QString value = properties.value(key,"");
		QString tempValue = value;
		QStringList values = collectValues(family, key, tempValue);
		propsMap.insert(key, tempValue);
		DebugDialog::debug(QString("props map %1 %2").arg(key).arg(tempValue));
	}
}

void ItemBase::setDropOffset(QPointF) 
{
}

bool ItemBase::hasRubberBandLeg() const
{
	return m_hasRubberBandLeg;
}

bool ItemBase::sceneEvent(QEvent *event)
{
	return QGraphicsSvgItem::sceneEvent(event);
}

const QList<ConnectorItem *> & ItemBase::cachedConnectorItems() 
{
	if (m_cachedConnectorItems.isEmpty()) {
		foreach (QGraphicsItem * childItem, childItems()) {
			ConnectorItem * connectorItem = dynamic_cast<ConnectorItem *>(childItem);
			if (connectorItem != NULL) m_cachedConnectorItems.append(connectorItem);
		}
	}

	return m_cachedConnectorItems;
}

const QList<ConnectorItem *> & ItemBase::cachedConnectorItemsConst() const 
{
	return m_cachedConnectorItems;
}

void ItemBase::clearConnectorItemCache() 
{
	m_cachedConnectorItems.clear();
}

void ItemBase::killRubberBandLeg() {
	if (!hasRubberBandLeg()) return;

	prepareGeometryChange();

	foreach (ConnectorItem * connectorItem, cachedConnectorItems()) {
		connectorItem->killRubberBandLeg();
	}
}

ViewGeometry::WireFlags ItemBase::wireFlags() const {
	return m_viewGeometry.wireFlags();
}

QRectF ItemBase::boundingRectWithoutLegs() const
{
	return boundingRect();
}

QRectF ItemBase::boundingRect() const
{    
	FSvgRenderer * frenderer = dynamic_cast<FSvgRenderer *>(this->renderer());
	if (frenderer == NULL) {
		return QGraphicsSvgItem::boundingRect();
	}

	QSizeF s = frenderer->defaultSizeF();
	QRectF r(0,0, s.width(), s.height());
	return r;
}

QPainterPath ItemBase::hoverShape() const
{	
	return shape();
}

const QCursor * ItemBase::getCursor(Qt::KeyboardModifiers)
{
	return CursorMaster::MoveCursor;
}

PartLabel * ItemBase::partLabel() {
	return m_partLabel;
}

void ItemBase::doneLoading() {
}