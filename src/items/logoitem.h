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

$Revision: 5949 $:
$Author: cohen@irascible.com $:
$Date: 2012-04-07 10:16:07 -0700 (Sat, 07 Apr 2012) $

********************************************************************/

#ifndef LOGOITEM_H
#define LOGOITEM_H

#include <QRectF>
#include <QPainterPath>
#include <QPixmap>
#include <QVariant>
#include <QCheckBox>
#include <QComboBox>
#include <QTime>

#include "resizableboard.h"

class LogoItem : public ResizableBoard 
{
	Q_OBJECT

public:
	// after calling this constructor if you want to render the loaded svg (either from model or from file), MUST call <renderImage>
	LogoItem(ModelPart *, ViewIdentifierClass::ViewIdentifier, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu, bool doLabel);
	~LogoItem();

	QString retrieveSvg(ViewLayer::ViewLayerID, QHash<QString, QString> & svgHash, bool blackOnly, double dpi);
	bool collectExtraInfo(QWidget * parent, const QString & family, const QString & prop, const QString & value, bool swappingEnabled, QString & returnProp, QString & returnValue, QWidget * & returnWidget);
	void resizeMM(double w, double h, const LayerHash & viewLayers);
	QString getProperty(const QString & key);
	void setLogo(QString logo, bool force);
	const QString & logo();
	bool canEditPart();
	void setProp(const QString & prop, const QString & value);
	bool hasPartLabel();
	void loadImage(const QString & fileName, bool addName);
	virtual void reloadImage(const QString & svg, const QSizeF & aspectRatio, const QString & fileName, bool addName);
	bool stickyEnabled();
	PluralType isPlural();
	void addedToScene(bool temporary);
	void setHeight(double h);
	void paintHover(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

protected slots:
	void prepLoadImage();
	void logoEntry();
	void widthEntry();
	void heightEntry();
	void keepAspectRatio(bool checkState);
	void fileNameEntry(const QString & filename);

protected:
	virtual QString hackSvg(const QString & svg, const QString & logo);
	void initImage();
	void unableToLoad(const QString & fileName);
	void prepLoadImageAux(const QString & fileName, bool addName);
	void setFileNameItems();
	virtual ViewLayer::ViewLayerID layer();
	virtual QString colorString();
	virtual QString layerName();
	virtual QStringList & getImageNames();
	virtual QStringList & getNewImageNames();
	bool rerender(const QString & svg);
	double minWidth();
	double minHeight();
	bool freeRotationAllowed(Qt::KeyboardModifiers modifiers);
	ResizableBoard::Corner findCorner(QPointF p, Qt::KeyboardModifiers);

protected:
	QString m_logo;
	bool m_hasLogo;
	QString m_originalFilename;
	QComboBox * m_fileNameComboBox;
	QTime m_inLogoEntry;
};

class CopperLogoItem : public LogoItem
{
Q_OBJECT
	
public:
	// after calling this constructor if you want to render the loaded svg (either from model or from file), MUST call <renderImage>
	CopperLogoItem(ModelPart *, ViewIdentifierClass::ViewIdentifier, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu, bool doLabel);
	~CopperLogoItem();

	void reloadImage(const QString & svg, const QSizeF & aspectRatio, const QString & fileName, bool addName);

protected:
	ViewLayer::ViewLayerID layer();
	QString colorString();
	QStringList & getImageNames();
	QStringList & getNewImageNames();
	QString hackSvg(const QString & svg, const QString & logo);
	QString flipSvg(const QString & svg);
	bool isCopper0();
};

#endif
