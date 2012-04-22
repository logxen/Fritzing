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

#include "daos.h"
#include "../model/modelpart.h"

PartProperty::PartProperty() {}

PartProperty::PartProperty(qlonglong id, const QString &name, const QString &value, Part *part) {
	m_id = id;
	m_name = name;
	m_value = value;
	m_part = part;
}
PartProperty::PartProperty(const QString &name, const QString &value, Part *part) {
	m_name = name;
	m_value = value;
	m_part = part;
}

qlonglong PartProperty::id() const {
	return m_id;
}
void PartProperty::setId(qlonglong id) {
	m_id = id;
}

const QString &PartProperty::name() const {
	return m_name;
}

void PartProperty::setName(const QString &name) {
	m_name = name.toLower().trimmed();
	//Q_ASSERT(m_name != ___emptyString___);
}

const QString &PartProperty::value() const {
	return m_value;
}

void PartProperty::setValue(const QString &value) {
	m_value = value.toLower().trimmed();
	//Q_ASSERT(m_value != ___emptyString___);
}

Part *PartProperty::part() {
	return m_part;
}

void PartProperty::setPart(Part *part) {
	m_part = part;
}

PartProperty* PartProperty::from(const QString &name, const QString &value, Part *part) {
	return new PartProperty(name, value, part);
}



Part::Part() {
}
	
Part::Part(qlonglong id, const QString &moduleID, const QString &family, const PartPropertyList &properties, bool isCore) {
		setId(id);
		setModuleID(moduleID);
		setFamily(family);
		setProperties(properties);
		setCore(isCore);
}

Part::Part(const QString &moduleID, const QString &family, const PartPropertyList &properties, bool isCore) {
	m_id = -1;
	setModuleID(moduleID);
	setFamily(family);
	setProperties(properties);
	setCore(isCore);
}

Part::Part(const QString &family, const PartPropertyList &properties, bool isCore) {
	m_id = -1;
	m_moduleID = ___emptyString___;
	setFamily(family);
	setProperties(properties);
	setCore(isCore);
}

Part::Part(const QString &family, const QString &propname, const QString &propvalue, bool isCore) {
	m_id = -1;
	m_moduleID = ___emptyString___;
	setFamily(family);

	m_properties << new PartProperty(propname, propvalue, this);
	setCore(isCore);
}

Part::Part(const QString &family, bool isCore) {
	m_id = -1;
	m_moduleID = ___emptyString___;
	setFamily(family);
	setCore(isCore);
}

Part::Part(const QString &family) {
	m_id = -1;
	m_moduleID = ___emptyString___;
	setFamily(family);
}

Part::~Part() {
	foreach(PartProperty *pp, m_properties) {
		delete pp;
	}
}

qlonglong Part::id() const {
	return m_id;
}

void Part::setId(qlonglong id) {
	m_id = id;
}

const QString &Part::moduleID() const {
	return m_moduleID;
}

void Part::setModuleID(const QString &moduleID) {
	m_moduleID = moduleID;
	if (m_moduleID.isEmpty()) {
		throw "Part::setModuleID is empty";
	}
	//Q_ASSERT(!m_moduleID.isNull());
}

const QString & Part::family() const {
	return m_family;
}

void Part::setFamily(const QString &family) {
	m_family = family.toLower().trimmed();
	//Q_ASSERT(m_family != ___emptyString___);
}

PartPropertyList Part::properties() const {
	return m_properties;
}

void Part::setProperties(const PartPropertyList &properties) {
	//Q_ASSERT(m_properties.size() == 0);
	m_properties = properties;
	//Q_ASSERT(m_properties.size() > 0);
}

QString Part::isCore() const {
	return m_isCore;
}

void Part::setCore(bool isCore) {
	setCore((int)isCore);
}

void Part::setCore(int isCore) {
	setCore(QString("%1").arg(isCore));
}

void Part::setCore(QString isCore) {
	m_isCore = isCore;
}

void Part::addProperty(PartProperty *property) {
	m_properties << property;
}

Part *Part::from(const QString &family, const QMultiHash<QString,QString> &properties) {
	Part *part = new Part(family);
	if(properties.size() > 0) {
		PartPropertyList partprops;
		foreach(QString name, properties.uniqueKeys()) {
			foreach(QString value, properties.values(name)) {
				partprops << PartProperty::from(name, value, part);
			}
		}
		part->setProperties(partprops);
	}
	return part;
}

Part *Part::from(const QString &family, const QMultiHash<QString,QString> &properties, bool isCore) {
	Part *part = from(family, properties);
	part->setCore(isCore);
	return part;
}

Part *Part::from(const QString &moduleID, const QString &family, const QMultiHash<QString, QString> &properties, bool isCore) {
	Part *part = from(family, properties, isCore);
	part->setModuleID(moduleID);
	return part;
}

Part *Part::from(ModelPart *modelPart) {
	QString moduleID = modelPart->moduleID();
	if (moduleID.isEmpty()) {
		throw "Part::from ModuleID is empty";
	}
	//Q_ASSERT(!moduleID.isNull());

	QHash<QString,QString> props = modelPart->properties();
	if (!props.contains("family")) {
		throw QString("Part props missing family: %1").arg(modelPart->path());
	}

	QString family = props["family"];
	props.remove("family");
	return from(moduleID, family,props, modelPart->isCore());
}

