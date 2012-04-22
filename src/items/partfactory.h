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

$Revision: 5721 $:
$Author: cohen@irascible.com $:
$Date: 2012-01-03 07:53:58 -0800 (Tue, 03 Jan 2012) $

********************************************************************/

#ifndef PARTFACTORY_H
#define PARTFACTORY_H

#include <QMenu>
#include <QDomDocument>
#include <QDomElement>
#include "../viewidentifierclass.h"
#include "../viewlayer.h"

class PartFactory
{
public:
	static class ItemBase * createPart(class ModelPart *, ViewLayer::ViewLayerSpec, ViewIdentifierClass::ViewIdentifier, const class ViewGeometry & viewGeometry, long id, QMenu * itemMenu, QMenu * wireMenu, bool doLabel);
	static QString getSvgFilename(class ModelPart *, const QString & filename);
	static QString getFzpFilename(const QString & moduleID);
	static void initFolder();
	static void cleanup();
	static class ModelPart * fixObsoleteModuleID(QDomDocument & domDocument, QDomElement & instance, QString & moduleIDRef, class ModelBase * referenceModel);
	static QString folderPath();



protected:
	static QString getFzpFilenameAux(const QString & moduleID, QString (*getFzp)(const QString &));
	static QString getSvgFilenameAux(const QString & expectedFileName, QString (*getSvg)(const QString &));
	static class ItemBase * createPartAux(class ModelPart *, ViewIdentifierClass::ViewIdentifier, const class ViewGeometry & viewGeometry, long id, QMenu * itemMenu, QMenu * wireMenu, bool doLabel);
};

#endif
