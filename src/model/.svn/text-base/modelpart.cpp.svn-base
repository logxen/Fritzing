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

#include "modelpart.h"
#include "../debugdialog.h"
#include "../connectors/connectorshared.h"
#include "../connectors/busshared.h"
#include "../connectors/bus.h"
#include "../version/version.h"
#include "../utils/folderutils.h"
#include "../items/itembase.h"
#include "../items/virtualwire.h"

#include <QDomElement>
#include <QBitArray>

long ModelPart::m_nextIndex = 0;
const int ModelPart::indexMultiplier = 10;
QStringList ModelPart::m_possibleFolders;

typedef QHash<QString, ModelPartList*> InstanceTitleIncrementHash;
static QHash<QObject *, InstanceTitleIncrementHash *> AllInstanceTitleIncrements; 
InstanceTitleIncrementHash NullInstanceTitleIncrements;

static const QRegExp InstanceTitleRegExp("^(.*[^\\d])(\\d+)$");


////////////////////////////////////////

ModelPart::ModelPart(ItemType type)
	: QObject()
{
	commonInit(type);
	m_modelPartShared = NULL;
	m_index = m_nextIndex++;
	m_originalModelPartShared = false;  // TODO: make this a QSharedPointer
}

ModelPart::ModelPart(QDomDocument * domDocument, const QString & path, ItemType type)
	: QObject()
{
	commonInit(type);
	m_modelPartShared = new ModelPartShared(domDocument, path);
	m_originalModelPartShared = true;		// TODO: make this a QSharedPointer
}

void ModelPart::commonInit(ItemType type) {
	m_type = type;
	m_locationFlags = 0;
	m_indexSynched = false;
}

ModelPart::~ModelPart() {
	//DebugDialog::debug(QString("deleting modelpart %1 %2").arg((long) this, 0, 16).arg(m_index));

	clearOldInstanceTitle(m_instanceTitle);

	InstanceTitleIncrementHash * itih = AllInstanceTitleIncrements.value(this);
	if (itih) {
		AllInstanceTitleIncrements.remove(this);
		foreach (ModelPartList * list, itih->values()) {
			delete list;
		}
		delete itih;
	}

	if (m_originalModelPartShared) {		// TODO: make this a QSharedPointer
		if (m_modelPartShared) {
			delete m_modelPartShared;
		}
	}

	foreach (Connector * connector, m_connectorHash.values()) {
		delete connector;
	}
	m_connectorHash.clear();
	foreach (Connector * connector, m_deletedConnectors) {
		delete connector;
	}
	m_deletedConnectors.clear();

	clearBuses();
}

const QString & ModelPart::moduleID() {
	if (m_modelPartShared != NULL) return m_modelPartShared->moduleID();

	return ___emptyString___;
}

const QString & ModelPart::label() {
	if (m_modelPartShared != NULL) return m_modelPartShared->label();

	return ___emptyString___;
}

const QString & ModelPart::author() {
	if (m_modelPartShared != NULL) return m_modelPartShared->author();

	return ___emptyString___;
}

const QString & ModelPart::uri() {
	if (m_modelPartShared != NULL) return m_modelPartShared->uri();

	return ___emptyString___;
}

const QDate & ModelPart::date() {
	if (m_modelPartShared != NULL) return m_modelPartShared->date();

	static QDate tempDate;
	tempDate = QDate::currentDate();
	return tempDate;
}

void ModelPart::setItemType(ItemType t) {
	m_type = t;
}

void ModelPart::copy(ModelPart * modelPart) {
	if (modelPart == NULL) return;

	m_type = modelPart->itemType();
	m_modelPartShared = modelPart->modelPartShared();
	m_locationFlags = modelPart->m_locationFlags;
}

void ModelPart::copyNew(ModelPart * modelPart) {
	copy(modelPart);
}

void ModelPart::copyStuff(ModelPart * modelPart) {
	modelPartShared()->copy(modelPart->modelPartShared());
}

ModelPartShared * ModelPart::modelPartShared() {
	if(!m_modelPartShared) {
		m_modelPartShared = new ModelPartShared();
		m_originalModelPartShared = true;		// TODO: make this a QSharedPointer
	}
	return m_modelPartShared;
}

ModelPartSharedRoot * ModelPart::modelPartSharedRoot() {
	return qobject_cast<ModelPartSharedRoot *>(m_modelPartShared);
}

void ModelPart::setModelPartShared(ModelPartShared * modelPartShared) {
	m_modelPartShared = modelPartShared;
}

void ModelPart::addViewItem(ItemBase * item) {
	m_viewItems.append(item);
}

void ModelPart::removeViewItem(ItemBase * item) {
	m_viewItems.removeOne(item);
}

ItemBase * ModelPart::viewItem(QGraphicsScene * scene) {
	foreach (ItemBase * itemBase, m_viewItems) {
		if (itemBase->scene() == scene) return itemBase;
	}

	return NULL;
}

void ModelPart::saveInstances(const QString & fileName, QXmlStreamWriter & streamWriter, bool startDocument) {
	if (startDocument) {
		streamWriter.writeStartDocument();
    	streamWriter.writeStartElement("module");
		streamWriter.writeAttribute("fritzingVersion", Version::versionString());
		ModelPartSharedRoot * root = modelPartSharedRoot();
		if (root) {
			if (!root->icon().isEmpty()) {
				streamWriter.writeAttribute("icon", root->icon());
			}
			if (!root->searchTerm().isEmpty()) {
				streamWriter.writeAttribute("search", root->searchTerm());
			}
		}
		QString title = this->title();
		if(!title.isNull() && !title.isEmpty()) {
			streamWriter.writeTextElement("title",title);
		}
		
		emit startSaveInstances(fileName, this, streamWriter);

		streamWriter.writeStartElement("instances");
	}

	if (m_viewItems.size() > 0) {
		saveInstance(streamWriter);
	}

	QList<QObject *> children = this->children();
	if(m_orderedChildren.count() > 0) {
		children = m_orderedChildren;
	}

	QList<QObject *>::const_iterator i;
	for (i = children.constBegin(); i != children.constEnd(); ++i) {
		ModelPart* mp = qobject_cast<ModelPart *>(*i);
		if (mp == NULL) continue;

		mp->saveInstances(fileName, streamWriter, false);
	}


	if (startDocument) {
		streamWriter.writeEndElement();	  //  instances
		streamWriter.writeEndElement();   //  module
		streamWriter.writeEndDocument();
	}
}

void ModelPart::saveInstance(QXmlStreamWriter & streamWriter) 
{
	if (qobject_cast<VirtualWire *>(m_viewItems.at(0)) != NULL) return;				// don't save virtual wires

	streamWriter.writeStartElement("instance");
	if (m_modelPartShared != NULL) {
		const QString & moduleIdRef = m_modelPartShared->moduleID();
		streamWriter.writeAttribute("moduleIdRef", moduleIdRef);
		streamWriter.writeAttribute("modelIndex", QString::number(m_index));
		streamWriter.writeAttribute("path", m_modelPartShared->path());
		if (m_modelPartShared->flippedSMD()) {
			streamWriter.writeAttribute("flippedSMD", "true");
		}
	}

	bool writeLocal = false;
	foreach (Connector * connector, this->connectors()) {
		if (!connector->connectorLocalName().isEmpty()) {
			writeLocal = true;
			break;
		}
	}

	if (writeLocal) {
		streamWriter.writeStartElement("localConnectors");
		foreach (Connector * connector, this->connectors()) {
			if (!connector->connectorLocalName().isEmpty()) {
				streamWriter.writeStartElement("localConnector");
				streamWriter.writeAttribute("id", connector->connectorSharedID());
				streamWriter.writeAttribute("name", connector->connectorLocalName());
				streamWriter.writeEndElement();
			}
		}
		streamWriter.writeEndElement();
	}

	foreach (QByteArray byteArray, dynamicPropertyNames()) {
		streamWriter.writeStartElement("property");
		streamWriter.writeAttribute("name",  byteArray.data());
		streamWriter.writeAttribute("value", property(byteArray.data()).toString());
		streamWriter.writeEndElement();
	}

	QString title = instanceTitle();
	if(!title.isNull() && !title.isEmpty()) {
		writeTag(streamWriter,"title",title);
	}

	QString text = instanceText();
	if(!text.isNull() && !text.isEmpty()) {
		streamWriter.writeStartElement("text");
		streamWriter.writeCharacters(text);
		streamWriter.writeEndElement();
	}

	// tell the views to write themselves out
	streamWriter.writeStartElement("views");
	foreach (ItemBase * itemBase, m_viewItems) {
		itemBase->saveInstance(streamWriter);
	}
	streamWriter.writeEndElement();		// views
	streamWriter.writeEndElement();		//instance
}

void ModelPart::writeTag(QXmlStreamWriter & streamWriter, QString tagName, QString tagValue) {
	if(!tagValue.isEmpty()) {
		streamWriter.writeTextElement(tagName,tagValue);
	}
}

void ModelPart::writeNestedTag(QXmlStreamWriter & streamWriter, QString tagName, const QStringList &values, QString childTag) {
	if(values.count() > 0) {
		streamWriter.writeStartElement(tagName);
		for(int i=0; i<values.count(); i++) {
			writeTag(streamWriter, childTag, values[i]);
		}
		streamWriter.writeEndElement();
	}
}

void ModelPart::writeNestedTag(QXmlStreamWriter & streamWriter, QString tagName, const QHash<QString,QString> &values, QString childTag, QString attrName) {
	streamWriter.writeStartElement(tagName);
	for(int i=0; i<values.keys().count(); i++) {
		streamWriter.writeStartElement(childTag);
		QString key = values.keys()[i];
		streamWriter.writeAttribute(attrName,key);
		streamWriter.writeCharacters(values[key]);
		streamWriter.writeEndElement();
	}
	streamWriter.writeEndElement();
}

void ModelPart::saveAsPart(QXmlStreamWriter & streamWriter, bool startDocument) {
	if (startDocument) {
		streamWriter.writeStartDocument();
    	streamWriter.writeStartElement("module");
		streamWriter.writeAttribute("fritzingVersion", Version::versionString());
		streamWriter.writeAttribute("moduleId", m_modelPartShared->moduleID());
    	writeTag(streamWriter,"version",m_modelPartShared->version());
    	writeTag(streamWriter,"author",m_modelPartShared->author());
    	writeTag(streamWriter,"title",title());
    	writeTag(streamWriter,"label",m_modelPartShared->label());
    	writeTag(streamWriter,"date",m_modelPartShared->dateAsStr());

    	writeNestedTag(streamWriter,"tags",m_modelPartShared->tags(),"tag");
    	writeNestedTag(streamWriter,"properties",m_modelPartShared->properties(),"property","name");

    	writeTag(streamWriter,"taxonomy",m_modelPartShared->taxonomy());
    	writeTag(streamWriter,"description",m_modelPartShared->description());
    	writeTag(streamWriter,"url",m_modelPartShared->url());
	}

	if (m_viewItems.size() > 0) {
		if (startDocument) {
			streamWriter.writeStartElement("views");
		}
		for (int i = 0; i < m_viewItems.size(); i++) {
			ItemBase * item = m_viewItems[i];
			item->writeXml(streamWriter);
		}

		if(startDocument) {
			streamWriter.writeEndElement();
		}

		streamWriter.writeStartElement("connectors");
		const QList< QPointer<ConnectorShared> > connectors = m_modelPartShared->connectorsShared();
		for (int i = 0; i < connectors.count(); i++) {
			Connector * connector = new Connector(connectors[i], this);
			connector->saveAsPart(streamWriter);
			delete connector;
		}
		streamWriter.writeEndElement();
	}

	QList<QObject *>::const_iterator i;
    for (i = children().constBegin(); i != children().constEnd(); ++i) {
		ModelPart * mp = qobject_cast<ModelPart *>(*i);
		if (mp == NULL) continue;

		mp->saveAsPart(streamWriter, false);
	}

	if (startDocument) {
		streamWriter.writeEndElement();
		streamWriter.writeEndElement();
		streamWriter.writeEndDocument();
	}
}

void ModelPart::initConnectors(bool force) {
	if(m_modelPartShared == NULL) return;

	if(force) {
		foreach (Connector * connector, m_connectorHash.values()) {
			// due to craziness in the parts editor
			m_deletedConnectors.append(connector);
			//delete connector;
		}
		m_connectorHash.clear();		
		clearBuses();
	}
	if(m_connectorHash.count() > 0) return;		// already done

	m_modelPartShared->initConnectors();
	foreach (ConnectorShared * connectorShared, m_modelPartShared->connectorsShared()) {
		Connector * connector = new Connector(connectorShared, this);
		m_connectorHash.insert(connectorShared->id(), connector);
	}
	initBuses();
}

void ModelPart::clearBuses() {
	foreach (Bus * bus, m_busHash.values()) {
		delete bus;
	}
	m_busHash.clear();
}

void ModelPart::initBuses() {
	foreach (Connector * connector, m_connectorHash.values()) {
		BusShared * busShared = connector->connectorShared()->bus();
		if (busShared != NULL) {
			Bus * bus = m_busHash.value(busShared->id());
			if (bus == NULL) {
				bus = new Bus(busShared, this);
				m_busHash.insert(busShared->id(), bus);
			}
			connector->setBus(bus);
			bus->addConnector(connector);
		}
	}
}

const QHash<QString, QPointer<Connector> > & ModelPart::connectors() {
	return m_connectorHash;
}

long ModelPart::modelIndex() {
	return m_index;
}

void ModelPart::setModelIndex(long index) {
	m_index = index;
	updateIndex(index);
}

void ModelPart::setModelIndexFromMultiplied(long multiplied) {
	if (m_indexSynched) {
		// this is gross.  m_index should always be itemBase->id() / ModelPart::indexMultiplier
		// but sometimes multiple parts reuse the same model part, so this makes sure we don't overwrite
		// when temporarily reusing a modelpart.  Eventually always create a new model part and get rid of modelIndex
		if (m_index != multiplied / ModelPart::indexMultiplier) {
			//DebugDialog::debug("temporary model part?");
		}

		return;
	}

	m_indexSynched = true;
	setModelIndex(multiplied / ModelPart::indexMultiplier);
}

void ModelPart::updateIndex(long index)
{
	if (index >= m_nextIndex) {
		m_nextIndex = index + 1;
	}
}

long ModelPart::nextIndex() {
	return m_nextIndex++;
}

void ModelPart::setInstanceDomElement(const QDomElement & domElement) {
	//DebugDialog::debug(QString("model part instance %1").arg((long) this, 0, 16));
	m_instanceDomElement = domElement;
}

const QDomElement & ModelPart::instanceDomElement() {
	return m_instanceDomElement;
}

const QString & ModelPart::title() {
	if (!m_localTitle.isEmpty()) return m_localTitle;

	if (m_modelPartShared != NULL) return m_modelPartShared->title();

	return m_localTitle;
}

void ModelPart::setLocalTitle(const QString & localTitle) {
	m_localTitle = localTitle;
}

const QString & ModelPart::version() {
	if (m_modelPartShared != NULL) return m_modelPartShared->version();

	return ___emptyString___;
}

const QString & ModelPart::path() {
	if (m_modelPartShared != NULL) return m_modelPartShared->path();

	return ___emptyString___;
}

const QString & ModelPart::description() {
	if (m_modelPartShared != NULL) return m_modelPartShared->description();

	return ___emptyString___;
}

const QString & ModelPart::url() {
	if (m_modelPartShared != NULL) return m_modelPartShared->url();

	return ___emptyString___;
}

const QStringList & ModelPart::tags() {
	if (m_modelPartShared != NULL) return m_modelPartShared->tags();

	return ___emptyStringList___;
}

const QHash<QString,QString> & ModelPart::properties() const {
	if (m_modelPartShared != NULL) return m_modelPartShared->properties();

	return ___emptyStringHash___;
}

Connector * ModelPart::getConnector(const QString & id) {
	return m_connectorHash.value(id);
}

const QHash<QString, QPointer<Bus> > & ModelPart::buses() {
	return  m_busHash;
}

Bus * ModelPart::bus(const QString & busID) {
	return m_busHash.value(busID);
}

bool ModelPart::ignoreTerminalPoints() {
	if (m_modelPartShared != NULL) return m_modelPartShared->ignoreTerminalPoints();

	return true;
}

bool ModelPart::isCore() {
	return (m_locationFlags & CoreFlag) != 0;
}

void ModelPart::setCore(bool core) {
	setLocationFlag(core, CoreFlag);
}

bool ModelPart::isContrib() {
	return (m_locationFlags & ContribFlag) != 0;
}

void ModelPart::setContrib(bool contrib) {
	setLocationFlag(contrib, ContribFlag);
}

bool ModelPart::isAlien() {
	return (m_locationFlags & AlienFlag) != 0;;
}

void ModelPart::setAlien(bool alien) {
	setLocationFlag(alien, AlienFlag);
}

bool ModelPart::isFzz() {
	return (m_locationFlags & FzzFlag) != 0;;
}

void ModelPart::setFzz(bool fzz) {
	setLocationFlag(fzz, FzzFlag);
}

void ModelPart::setLocationFlag(bool setting, LocationFlag flag) {
	if (setting) {
		m_locationFlags |= flag;
	}
	else {
		m_locationFlags &= ~flag;
	}
}

bool ModelPart::hasViewIdentifier(ViewIdentifierClass::ViewIdentifier viewIdentifier) {
	QDomDocument * doc = domDocument();
	if (doc == NULL) return false;

	QDomElement viewsElems = doc->documentElement().firstChildElement("views");
	if(viewsElems.isNull()) return false;

	return !viewsElems.firstChildElement(ViewIdentifierClass::viewIdentifierXmlName(viewIdentifier)).isNull();
}


QList<ModelPart*> ModelPart::getAllParts() {
	QList<ModelPart*> retval;
	QList<QObject *>::const_iterator i;
	for (i = children().constBegin(); i != children().constEnd(); ++i) {
		ModelPart* mp = qobject_cast<ModelPart *>(*i);
		if (mp == NULL) continue;
		retval << mp;
	}

	return retval;
}

QList<ModelPart*> ModelPart::getAllNonCoreParts() {
	QList<ModelPart*> retval;
	QList<QObject *>::const_iterator i;
	for (i = children().constBegin(); i != children().constEnd(); ++i) {
		ModelPart* mp = qobject_cast<ModelPart *>(*i);
		if (mp == NULL) continue;

		if(!mp->isCore()) {
			retval << mp;
		}
	}

	return retval;
}

bool ModelPart::hasViewID(long id) {
	foreach (ItemBase * item, m_viewItems) {
		if (item->id() == id) return true;
	}

	return false;
}

const QString & ModelPart::instanceTitle() const {
	return m_instanceTitle;
}

const QString & ModelPart::instanceText() {
	return m_instanceText;
}

void ModelPart::setInstanceText(QString text) {
	m_instanceText = text;
}

void ModelPart::clearOldInstanceTitle(const QString & title) 
{
	InstanceTitleIncrementHash * itih = NULL;
	if (parent() == NULL) {
		itih = &NullInstanceTitleIncrements;
	}
	else {
		itih = AllInstanceTitleIncrements.value(parent(), NULL);
	}

	if (itih == NULL) return;

	//DebugDialog::debug(QString("clearing title:%1 ix:%2").arg(title).arg(modelPart->modelIndex()));
	QString prefix = title;
	int ix = InstanceTitleRegExp.indexIn(title);
	if (ix >= 0) {
		prefix = InstanceTitleRegExp.cap(1);
	}
	ModelPartList * modelParts = itih->value(prefix, NULL);
	if (modelParts) {
		modelParts->removeOne(this);
		//DebugDialog::debug(QString("\tc:%1").arg(modelParts->count()));
	}
}

ModelPartList * ModelPart::ensureInstanceTitleIncrements(const QString & prefix) 
{
	InstanceTitleIncrementHash * itih = NULL;
	if (parent() == NULL) {
		itih = &NullInstanceTitleIncrements;
	}
	else {
		itih = AllInstanceTitleIncrements.value(parent(), NULL);
		if (itih == NULL) {
			itih = new InstanceTitleIncrementHash;
			AllInstanceTitleIncrements.insert(parent(), itih);
		}
	}

	ModelPartList * modelParts = itih->value(prefix, NULL);
	if (modelParts == NULL) {
		modelParts =  new ModelPartList;
		itih->insert(prefix, modelParts);
	}
	return modelParts;
}

void ModelPart::setInstanceTitle(QString title) {
	if (title.compare(m_instanceTitle) == 0) return;

	clearOldInstanceTitle(m_instanceTitle);

	m_instanceTitle = title;

	QString prefix = title;
	int ix = InstanceTitleRegExp.indexIn(title);
	if (ix >= 0) {
		prefix = InstanceTitleRegExp.cap(1);
		ModelPartList * modelParts = ensureInstanceTitleIncrements(prefix);
		modelParts->append(this);
	}
	//DebugDialog::debug(QString("adding title:%1 ix:%2 c:%3").arg(title).arg(modelIndex()).arg(modelParts->count()));
}

QString ModelPart::getNextTitle(const QString & title) {
	QString prefix = title;
	int ix = InstanceTitleRegExp.indexIn(title);
	if (ix >= 0) {
		prefix = InstanceTitleRegExp.cap(1);
	}
	else {
		bool allDigits = true;
		foreach (QChar c, title) {
			if (!c.isDigit()) {
				allDigits = false;
				break;
			}
		}
		if (allDigits) {
			return title;
		}
	}

	// TODO: if this were a sorted list, 
	ModelPartList * modelParts = ensureInstanceTitleIncrements(prefix);
	int highestSoFar = 0;
	bool gotNull = false;
	foreach (ModelPart * modelPart, *modelParts) {
		if (modelPart == NULL) {
			gotNull = true;
			continue;
		}

		QString title = modelPart->instanceTitle();
		title.remove(0, prefix.length());
		int count = title.toInt();			// returns zero on failure
		if (count > highestSoFar) {
			highestSoFar = count;
		}
	}

	if (gotNull) {
		modelParts->removeAll(NULL);
	}

	//DebugDialog::debug(QString("returning increment %1, %2").arg(prefix).arg(highestSoFar + 1));
	return QString("%1%2").arg(prefix).arg(highestSoFar + 1);
}

void ModelPart::setOrderedChildren(QList<QObject*> children) {
	m_orderedChildren = children;
}

void ModelPart::setProp(const char * name, const QVariant & value) {
	setProperty(name, value);
}

QVariant ModelPart::prop(const char * name) const {
	return property(name);
}

void ModelPart::setProp(const QString & name, const QVariant & value) {
	QByteArray b = name.toLatin1();
	setProp(b.data(), value);
}

QVariant ModelPart::prop(const QString & name) const {
	QByteArray b = name.toLatin1();
	return prop(b.data());
}

const QStringList & ModelPart::possibleFolders() {
	if (m_possibleFolders.count() == 0) {
		m_possibleFolders << "core" << "obsolete" << "contrib" << "user";
	}

	return m_possibleFolders;
}

const QString & ModelPart::replacedby() {
	if (m_modelPartShared != NULL) return m_modelPartShared->replacedby();

	return ___emptyString___;
}

bool ModelPart::isObsolete() {
	if (m_modelPartShared != NULL) return !m_modelPartShared->replacedby().isEmpty();

	return false;
}

void ModelPart::setFlippedSMD(bool f) {
	if (m_modelPartShared != NULL) m_modelPartShared->setFlippedSMD(f);
}

bool ModelPart::flippedSMD() {
	if (m_modelPartShared != NULL) {
		return m_modelPartShared->flippedSMD();
	}

	return false;
}

bool ModelPart::needsCopper1() {
	if (m_modelPartShared != NULL) {
		return m_modelPartShared->needsCopper1();
	}

	return false;
}

QDomDocument* ModelPart::domDocument() {
	if (m_modelPartShared == NULL) return NULL;

	return m_modelPartShared->domDocument();
}

bool ModelPart::hasViewFor(ViewIdentifierClass::ViewIdentifier viewIdentifier) {
	if (m_modelPartShared == NULL) return false;

	return m_modelPartShared->hasViewFor(viewIdentifier);
}

bool ModelPart::hasViewFor(ViewIdentifierClass::ViewIdentifier viewIdentifier, ViewLayer::ViewLayerID viewLayerID) {
	if (m_modelPartShared == NULL) return false;

	return m_modelPartShared->hasViewFor(viewIdentifier, viewLayerID);
}

QString ModelPart::hasBaseNameFor(ViewIdentifierClass::ViewIdentifier viewIdentifier) {
	if (m_modelPartShared == NULL) return ___emptyString___;

	return m_modelPartShared->hasBaseNameFor(viewIdentifier);
}

const QStringList & ModelPart::displayKeys() {
	if (m_modelPartShared == NULL) return ___emptyStringList___;

	return m_modelPartShared->displayKeys();
}

ModelPart::ItemType ModelPart::itemType() const 
{ 
	return m_type; 
};

void ModelPart::setConnectorLocalName(const QString & id, const QString & name)
{
	if (id.isEmpty()) return;
	Connector * connector = m_connectorHash.value(id, NULL);
	if (connector) {
		connector->setConnectorLocalName(name);
	}
}

QString ModelPart::family(){
	if (m_modelPartShared) return m_modelPartShared->family();

	return "";
}

bool ModelPart::hasViewItems() {
	return (m_viewItems.count() > 0);
}
