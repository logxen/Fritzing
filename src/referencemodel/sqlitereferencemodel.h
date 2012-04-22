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

$Revision: 5204 $:
$Author: cohen@irascible.com $:
$Date: 2011-07-08 04:16:29 -0700 (Fri, 08 Jul 2011) $

********************************************************************/



#ifndef SQLITEREFERENCEMODEL_H_
#define SQLITEREFERENCEMODEL_H_

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QApplication>

#include "referencemodel.h"
#include "daos.h"

class SqliteReferenceModel : public ReferenceModel {
	Q_OBJECT
	public:
		SqliteReferenceModel();
		~SqliteReferenceModel();

		void loadAll(bool fastLoad);
		ModelPart *loadPart(const QString & path, bool update, bool fastLoad);

		ModelPart *retrieveModelPart(const QString &moduleID);
		ModelPart *retrieveModelPart(const QString &family, const QMultiHash<QString /*name*/, QString /*value*/> &properties);
		ModelPart *retrieveModelPart(const Part *examplePart);
		QString retrieveModuleId(const Part *examplePart, const QString &propertyName, bool closestMatch);

		bool addPart(ModelPart * newModel, bool update);
		bool updatePart(ModelPart * newModel);
		bool addPart(Part* part);
		ModelPart * addPart(QString newPartPath, bool addToReference, bool updateIdAlreadyExists);

		bool swapEnabled();
		bool containsModelPart(const QString & moduleID);

		QString partTitle(const QString moduleID);

	public slots:
		void recordProperty(const QString &name, const QString &value);
		QString retrieveModuleIdWith(const QString &family, const QString &propertyName, bool closestMatch);
		QString retrieveModuleId(const QString &family, const QMultiHash<QString /*name*/, QString /*value*/> &properties, const QString &propertyName, bool closestMatch);
		QStringList values(const QString &family, const QString &propName, bool distinct=true);
		bool lastWasExactMatch();

	protected:
		void initParts(bool fastLoad);

	protected:
		bool addPartAux(ModelPart * newModel);

		QString closestMatchId(const Part *examplePart, const QString &propertyName, const QString &propertyValue);
		QStringList getPossibleMatches(const Part *examplePart, const QString &propertyName, const QString &propertyValue);
		QString getClosestMatch(const Part *examplePart, QStringList possibleMatches);
		int countPropsInCommon(const Part *part1, const ModelPart *part2);

		bool createConnection();
		void deleteConnection();
		bool insertPart(Part *part);
		bool insertProperty(PartProperty *property);
		qlonglong partId(QString moduleID);
		bool removePart(qlonglong partId);
		bool removeProperties(qlonglong partId);

		volatile bool m_swappingEnabled;
		volatile bool m_lastWasExactMatch;
		bool m_init;
		QMultiHash<QString /*name*/, QString /*value*/> m_recordedProperties;
};

#endif /* SQLITEREFERENCEMODEL_H_ */
