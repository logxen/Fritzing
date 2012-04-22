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

$Revision: 5675 $:
$Author: cohen@irascible.com $:
$Date: 2011-12-13 19:57:40 -0800 (Tue, 13 Dec 2011) $

********************************************************************/

#ifndef PINHEADER_H
#define PINHEADER_H

#include <QRectF>
#include <QPainterPath>
#include <QPixmap>
#include <QVariant>

#include "paletteitem.h"

class PinHeader : public PaletteItem 
{
	Q_OBJECT

public:
	// after calling this constructor if you want to render the loaded svg (either from model or from file), MUST call <renderImage>
	PinHeader(ModelPart *, ViewIdentifierClass::ViewIdentifier, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu, bool doLabel);
	~PinHeader();

	QString getProperty(const QString & key);
	const QString & form();
	PluralType isPlural();
	void addedToScene(bool temporary);

protected:
	QStringList collectValues(const QString & family, const QString & prop, QString & value);

public:
	static QString FemaleFormString;
	static QString FemaleRoundedFormString;
	static QString MaleFormString;
	static QString ShroudedFormString;
	static QString MaleSingleRowSMDFormString;
	static QString FemaleSingleRowSMDFormString;
	static QString MaleDoubleRowSMDFormString;
	static QString FemaleDoubleRowSMDFormString;

public:
	static void initNames();
	static QString genFZP(const QString & moduleid);
	static QString genModuleID(QMap<QString, QString> & currPropsMap);
	static QString makePcbSvg(const QString & expectedFileName);
	static QString makePcbShroudedSvg(int pins);
	static QString makePcbSMDSvg(const QString & expectedFileName);
	static QString makeSchematicSvg(const QString & expectedFileName);
	static QString makeBreadboardSvg(const QString & expectedFileName);
	static QString makeBreadboardShroudedSvg(int pins);
	static QString findForm(const QString & filename);

protected:
	static const QStringList & forms();

protected:
	static void initSpacings();

protected:
	QString m_form;
};

#endif
