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

$Revision: 5922 $:
$Author: cohen@irascible.com $:
$Date: 2012-03-19 12:11:41 -0700 (Mon, 19 Mar 2012) $

********************************************************************/

#ifndef MODELBASE_H
#define MODELBASE_H

#include <QObject>
#include "modelpart.h"

class ModelBase : public QObject
{
Q_OBJECT

public:
	ModelBase(bool makeRoot);
	virtual ~ModelBase();

	ModelPart * root();
	ModelPartSharedRoot * rootModelPartShared();
	virtual ModelPart* retrieveModelPart(const QString & moduleID);
	virtual ModelPart * addModelPart(ModelPart * parent, ModelPart * copyChild);
	virtual bool load(const QString & fileName, ModelBase* refModel, QList<ModelPart *> & modelParts);
	void save(const QString & fileName, bool asPart);
	void save(const QString & fileName, class QXmlStreamWriter &, bool asPart);
	virtual ModelPart * addPart(QString newPartPath, bool addToReference);
	virtual bool addPart(ModelPart * modelPart, bool update);
	virtual ModelPart * addPart(QString newPartPath, bool addToReference, bool updateIdAlreadyExists);
	bool paste(ModelBase * refModel, QByteArray & data, QList<ModelPart *> & modelParts, QHash<QString, QRectF> & boundingRects, bool preserveIndex);
	void setReportMissingModules(bool);
	bool genFZP(const QString & moduleID, ModelBase * refModel);
	const QString & fritzingVersion();

signals:
	void loadedViews(ModelBase *, QDomElement & views);
	void loadedRoot(const QString & fileName, ModelBase *, QDomElement & root);
	void loadingInstances(ModelBase *, QDomElement & instances);
	void loadingInstance(ModelBase *, QDomElement & instance);
	void loadedInstances(ModelBase *, QDomElement & instances);

protected:
	void renewModelIndexes(QDomElement & root, const QString & childName, QHash<long, long> & oldToNew);
	bool loadInstances(QDomDocument &, QDomElement & root, QList<ModelPart *> & modelParts);
	ModelPart * fixObsoleteModuleID(QDomDocument & domDocument, QDomElement & instance, QString & moduleIDRef);
	static bool isRatsnest(QDomElement & instance);
	static void checkTraces(QDomElement & instance);

protected:
	QPointer<ModelPart> m_root;
	QPointer<ModelBase> m_referenceModel;
	bool m_reportMissingModules;
	QString m_fritzingVersion;
};

#endif
