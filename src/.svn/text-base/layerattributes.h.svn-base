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



#ifndef LAYERATTRIBUTES_H
#define LAYERATTRIBUTES_H

#include <QString>
#include <QDomElement>

#include "viewidentifierclass.h"
#include "viewlayer.h"

class LayerAttributes {
	
public:
	LayerAttributes();
	
	const QString & filename();
	void setFilename(const QString &);
	const QString & layerName();
	bool sticky();
	bool multiLayer();
	bool getSvgElementID(QDomDocument * , ViewIdentifierClass::ViewIdentifier, ViewLayer::ViewLayerID );
	bool canFlipHorizontal();
	bool canFlipVertical();
	const QByteArray & loaded();
	void clearLoaded();
	void setLoaded(const QByteArray &);

public:
	static QDomElement getSvgElementLayers(QDomDocument * doc, ViewIdentifierClass::ViewIdentifier viewIdentifier );

protected:
	static QDomElement getSvgElementLayer(QDomDocument *, ViewIdentifierClass::ViewIdentifier, ViewLayer::ViewLayerID, int & layerCount );

protected:
	QString m_filename;
	QString m_layerName;
	bool m_multiLayer;
	bool m_sticky;
	bool m_canFlipHorizontal;
	bool m_canFlipVertical;
	QByteArray m_loaded;
};

#endif
