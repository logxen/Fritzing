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

You should have received a copy of the GNU General Public Licensetriple
along with Fritzing.  If not, see <http://www.gnu.org/licenses/>.

********************************************************************

$Revision: 4195 $:
$Author: cohen@irascible.com $:
$Date: 2010-05-21 11:45:42 -0700 (Fri, 21 May 2010) $

********************************************************************/



#ifndef VIEWIDENTIFIERCLASS_H
#define VIEWIDENTIFIERCLASS_H

#include <QHash>
#include "viewlayer.h"

class ViewIdentifierClass
{

public:
   enum ViewIdentifier {
    	IconView,
    	BreadboardView,
    	SchematicView,
    	PCBView,
    	AllViews,
		UnknownView,
    	ViewCount
   	};

	static QString & viewIdentifierName(ViewIdentifier);
	static QString & viewIdentifierXmlName(ViewIdentifier);
	static QString & viewIdentifierNaturalName(ViewIdentifier);
	static ViewIdentifier idFromXmlName(const QString & name);
	static void initNames();
	static void cleanup();
	static const LayerList & layersForView(ViewIdentifier);
	static bool viewHasLayer(ViewIdentifier, ViewLayer::ViewLayerID);

protected:
	static QHash <ViewIdentifier, class NameTriple * > Names;
};
#endif
