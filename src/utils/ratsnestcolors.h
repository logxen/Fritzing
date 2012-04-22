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

$Revision: 5762 $:
$Author: cohen@irascible.com $:
$Date: 2012-01-19 14:04:44 -0800 (Thu, 19 Jan 2012) $

********************************************************************/

#ifndef RATSNESTCOLORS_H
#define RATSNESTCOLORS_H

#include <QColor>
#include <QDomElement>
#include <QHash>
#include <QStringList>
#include "../viewidentifierclass.h"

class RatsnestColors {

public:
	RatsnestColors(const QDomElement &);
	~RatsnestColors();

	static void initNames();
	static void cleanup();
	static const QColor & netColor(ViewIdentifierClass::ViewIdentifier m_viewIdentifier);
	static bool findConnectorColor(const QStringList & names, QColor & color);
	static bool isConnectorColor(ViewIdentifierClass::ViewIdentifier m_viewIdentifier, const QColor &);
	static void reset(ViewIdentifierClass::ViewIdentifier m_viewIdentifier);
	static QColor backgroundColor(ViewIdentifierClass::ViewIdentifier);
	static const QString & shadowColor(ViewIdentifierClass::ViewIdentifier, const QString& name);
	static QString wireColor(ViewIdentifierClass::ViewIdentifier, QString& name);

protected:
	const QColor & getNextColor();

protected:
	ViewIdentifierClass::ViewIdentifier m_viewIdentifier;
	QColor m_backgroundColor;
	int m_index;
	QHash<QString, class RatsnestColor *> m_ratsnestColorHash;
	QList<class RatsnestColor *> m_ratsnestColorList;

	static QHash<ViewIdentifierClass::ViewIdentifier, RatsnestColors *> m_viewList;
	static QHash<QString, class RatsnestColor *> m_allNames;
};


#endif
