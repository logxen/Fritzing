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

$Revision: 5309 $:
$Author: cohen@irascible.com $:
$Date: 2011-07-30 12:17:22 -0700 (Sat, 30 Jul 2011) $

********************************************************************/

#include "layerattributes.h"
#include "debugdialog.h"

LayerAttributes::LayerAttributes()
{
	m_sticky = false;
	m_multiLayer = false;
	m_canFlipHorizontal = m_canFlipVertical = false;
}

const QString & LayerAttributes::filename() {
	return m_filename;
}

void LayerAttributes::setFilename(const QString & filename) {
	m_filename = filename;
}

const QString & LayerAttributes::layerName() {
	return m_layerName;
}

bool LayerAttributes::multiLayer() {
	return m_multiLayer;
}

bool LayerAttributes::getSvgElementID(QDomDocument * doc, ViewIdentifierClass::ViewIdentifier viewIdentifier, ViewLayer::ViewLayerID viewLayerID) {
	int layerCount;
	QDomElement layer = getSvgElementLayer(doc, viewIdentifier, viewLayerID, layerCount);
	m_multiLayer = (layerCount > 1);

	if (layer.isNull()) return false;
	if (layerCount == 0) return false;

	m_filename = layer.parentNode().toElement().attribute("image");
	if (m_filename.isNull()) return false;
	if (m_filename.isEmpty()) return false;

	m_canFlipVertical = layer.parentNode().parentNode().toElement().attribute("flipvertical").compare("true") == 0;
	m_canFlipHorizontal = layer.parentNode().parentNode().toElement().attribute("fliphorizontal").compare("true") == 0;

	m_layerName = layer.attribute("layerId");
	if (m_layerName.isNull()) return false;
	if (m_layerName.isEmpty()) return false;

	QString stickyVal = layer.attribute("sticky");
	m_sticky = stickyVal.compare("true", Qt::CaseInsensitive) == 0;

	return true;
}

QDomElement LayerAttributes::getSvgElementLayers(QDomDocument * doc, ViewIdentifierClass::ViewIdentifier viewIdentifier )
{
   	if (doc == NULL) return ___emptyElement___;

   	QDomElement root = doc->documentElement();
   	if (root.isNull()) return ___emptyElement___;

	QDomElement views = root.firstChildElement("views");
	if (views.isNull()) return ___emptyElement___;

	QString name = ViewIdentifierClass::viewIdentifierXmlName(viewIdentifier);
	if (name.isEmpty() || name.isNull()) return ___emptyElement___;

	QDomElement view = views.firstChildElement(name);
	if (view.isNull()) return ___emptyElement___;

	QDomElement layers = view.firstChildElement("layers");
	if (layers.isNull()) return ___emptyElement___;

	return layers;
}


QDomElement LayerAttributes::getSvgElementLayer(QDomDocument * doc, ViewIdentifierClass::ViewIdentifier viewIdentifier, ViewLayer::ViewLayerID viewLayerID, int & layerCount )
{
	QString layerName = ViewLayer::viewLayerXmlNameFromID(viewLayerID);
	if (layerName.isNull() || layerName.isEmpty()) return ___emptyElement___;

	layerCount = 0;

	QDomElement layers = getSvgElementLayers(doc, viewIdentifier);
	if (layers.isNull()) return ___emptyElement___;

	QDomElement retval = ___emptyElement___;
	QDomElement layer = layers.firstChildElement("layer");
	bool gotOne = false;
	while (!layer.isNull()) {
		layerCount++;
		if (gotOne && layerCount > 1) break;

		if (!gotOne && layer.attribute("layerId").compare(layerName, Qt::CaseInsensitive) == 0) {
			gotOne = true;
			retval = layer;
		}
		//DebugDialog::debug(QString("layer id %1, want: %2").arg(layer.attribute("layerId")).arg(layerName) );
		layer = layer.nextSiblingElement("layer");
	}

	return retval;

}


bool LayerAttributes::sticky() {
	return m_sticky;
}

bool LayerAttributes::canFlipHorizontal() {
	return m_canFlipHorizontal;
}

bool LayerAttributes::canFlipVertical() {
	return m_canFlipVertical;
}

const QByteArray & LayerAttributes::loaded() {
	return m_loaded;
}

void LayerAttributes::clearLoaded() {
	m_loaded.clear();
}

void LayerAttributes::setLoaded(const QByteArray & loaded) {
	m_loaded = loaded;
}

