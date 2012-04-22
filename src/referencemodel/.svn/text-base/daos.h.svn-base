/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2010 Fachhochschule Potsdam - http://fh-potsdam.de

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




#ifndef DAOS_H_
#define DAOS_H_

#include <QList>
#include "../utils/misc.h"

class Part;

class PartProperty {
public:
	PartProperty();
	PartProperty(qlonglong id, const QString &name, const QString &value, Part *part = NULL);
	PartProperty(const QString &name, const QString &value, Part *part = NULL);

	qlonglong id() const;
	void setId(qlonglong id);

	const QString &name() const;
	void setName(const QString &name);

	const QString &value() const;
	void setValue(const QString &value);

	Part *part();
	void setPart(Part *part);

	static PartProperty* from(const QString &name, const QString &value, Part *part=NULL);

private:
	qlonglong m_id;
	QString m_name;
	QString m_value;
	Part * m_part;
};

typedef QList<PartProperty*> PartPropertyList;

class Part {
public:
	Part();
	Part(qlonglong id, const QString &moduleID, const QString &family, const PartPropertyList &properties, bool isCore);
	Part(const QString &moduleID, const QString &family, const PartPropertyList &properties, bool isCore);
	Part(const QString &family, const PartPropertyList &properties, bool isCore);
	Part(const QString &family, const QString &propname, const QString &propvalue, bool isCore);
	Part(const QString &family, bool isCore);
	Part(const QString &family);
	~Part();

	qlonglong id() const;
	void setId(qlonglong id);

	const QString &moduleID() const;
	void setModuleID(const QString &moduleID);

	const QString & family() const;
	void setFamily(const QString &family);
	PartPropertyList properties() const;
	void setProperties(const PartPropertyList &properties);
	

	QString isCore() const;
	void setCore(bool isCore);
	void setCore(int isCore);
	void setCore(QString isCore);

	void addProperty(PartProperty *property);

	static Part *from(const QString &family, const QMultiHash<QString,QString> &properties);

	static Part *from(const QString &family, const QMultiHash<QString,QString> &properties, bool isCore);
	static Part *from(const QString &moduleID, const QString &family, const QMultiHash<QString, QString> &properties, bool isCore);
	static Part *from(class ModelPart *modelPart);

private:
	qlonglong m_id;
	QString m_moduleID;
	QString m_family;
	PartPropertyList m_properties;
	QString m_isCore;
};

#endif /* DAOS_H_ */
