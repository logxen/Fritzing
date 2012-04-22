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

$Revision: 5944 $:
$Author: cohen@irascible.com $:
$Date: 2012-04-06 07:06:09 -0700 (Fri, 06 Apr 2012) $

********************************************************************/

#ifndef HOLE_H
#define HOLE_H

#include <QRectF>
#include <QPointF>
#include <QPainterPath>
#include <QPixmap>
#include <QVariant>
#include <QComboBox>
#include <QRadioButton>
#include <QDoubleValidator>

#include "paletteitem.h"

typedef QPointF (*RangeCalc)(const QString &);

struct HoleSettings
{
	QString holeDiameter;
	QString ringThickness;
	QPointer<QDoubleValidator> diameterValidator;
	QPointer<QDoubleValidator> thicknessValidator;
	QPointer<QLineEdit> diameterEdit;
	QPointer<QLineEdit> thicknessEdit;
	QPointer<QRadioButton> inRadioButton;
	QPointer<QRadioButton> mmRadioButton;
	QPointer<QComboBox> sizesComboBox;
	RangeCalc ringThicknessRange;
	RangeCalc holeDiameterRange;

	QString currentUnits();
};

class Hole : public PaletteItem 
{
	Q_OBJECT

public:
	// after calling this constructor if you want to render the loaded svg (either from model or from file), MUST call <renderImage>
	Hole(ModelPart *, ViewIdentifierClass::ViewIdentifier, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu, bool doLabel);
	~Hole();

	QString getProperty(const QString & key);
	void setProp(const QString & prop, const QString & value);
	void setHoleSize(QString holeSize, bool force);

	bool collectExtraInfo(QWidget * parent, const QString & family, const QString & prop, const QString & value, bool swappingEnabled, QString & returnProp, QString & returnValue, QWidget * & returnWidget);
	bool hasCustomSVG();
	PluralType isPlural();
	QString retrieveSvg(ViewLayer::ViewLayerID viewLayerID, QHash<QString, QString> & svgHash, bool blackOnly, double dpi); 
	bool canEditPart();
	void addedToScene(bool temporary);	
	bool hasPartNumberProperty();
	QString holeSize();
	bool rotationAllowed();
	bool rotation45Allowed();
	bool canFindConnectorsUnder();

public:
	static QWidget * createHoleSettings(QWidget * parent, HoleSettings &, bool swappingEnabled, const QString & currentHoleSize);
	static void updateValidators(HoleSettings &);
	static void updateEditTexts(HoleSettings &);
	static void updateSizes(HoleSettings &);
	static QString changeUnits(const QString &, HoleSettings &);
	static QPointF ringThicknessRange(const QString & holeDiameter);
	static QPointF holeDiameterRange(const QString & ringThickness);
	static bool changeDiameter(HoleSettings & holeSettings, QObject * sender);
	static bool changeThickness(HoleSettings & holeSettings, QObject * sender);
	static bool setHoleSize(QString & holeSize, bool force, HoleSettings & holeSettings);
	static QString holeSize(HoleSettings &);
	static void initHoleSettings(HoleSettings & holeSettings);

protected slots:
	void changeDiameter();
	void changeThickness();
	void changeUnits(bool);
	void changeHoleSize(const QString &);

protected:
	QString makeSvg(const QString & holeDiameter, const QString & ringThickness, ViewLayer::ViewLayerID);
	virtual QString makeID();
	ItemBase * setBothSvg(const QString & holeDiameter, const QString & ringThickness); 
	void setBothNonConnectors(ItemBase * itemBase, SvgIdLayer * svgIdLayer);
	void setUpHoleSizes();
	virtual void setBoth(const QString & holeDiameter, const QString &  thickness);
	QString currentUnits();
	static QStringList getSizes(QString & holeSize);
	QRectF getRect(const QString & newSize);

protected:
	QPointer<class FSvgRenderer> m_otherLayerRenderer;
	HoleSettings m_holeSettings;

public:
	static QHash<QString, QString> HoleSizes;

public:
	static const QString AutorouteViaHoleSize;
	static const QString AutorouteViaRingThickness;
	static QString DefaultAutorouteViaHoleSize;
	static QString DefaultAutorouteViaRingThickness;
	static const double OffsetPixels;
};

#endif
