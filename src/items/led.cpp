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

$Revision: 5948 $:
$Author: cohen@irascible.com $:
$Date: 2012-04-07 02:02:13 -0700 (Sat, 07 Apr 2012) $

********************************************************************/

#include "led.h"
#include "../utils/graphicsutils.h"
#include "../utils/textutils.h"
#include "../utils/focusoutcombobox.h"
#include "../fsvgrenderer.h"
#include "../sketch/infographicsview.h"
#include "../svg/svgfilesplitter.h"
#include "../commands.h"
#include "../layerattributes.h"
#include "moduleidnames.h"
#include "partlabel.h"

static QHash<QString, QString> BreadboardSvg;
static QHash<QString, QString> IconSvg;

LED::LED( ModelPart * modelPart, ViewIdentifierClass::ViewIdentifier viewIdentifier, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu, bool doLabel)
	: Capacitor(modelPart, viewIdentifier, viewGeometry, id, itemMenu, doLabel)
{
}

LED::~LED() {
}

QString LED::retrieveSvg(ViewLayer::ViewLayerID viewLayerID, QHash<QString, QString> & svgHash, bool blackOnly, double dpi) 
{
	switch (viewLayerID) {
		case ViewLayer::Breadboard:
		case ViewLayer::Icon:
			break;
		default:
			return Capacitor::retrieveSvg(viewLayerID, svgHash, blackOnly, dpi);
	}

	QString svg = getColorSVG(prop("color"), viewLayerID);
	if (svg.isEmpty()) return "";

	QString xmlName = ViewLayer::viewLayerXmlNameFromID(viewLayerID);
	SvgFileSplitter splitter;
	bool result = splitter.splitString(svg, xmlName);
	if (!result) {
		return "";
	}
	result = splitter.normalize(dpi, xmlName, blackOnly);
	if (!result) {
		return "";
	}
	return splitter.elementString(xmlName);
}

void LED::addedToScene(bool temporary)
{
	if (this->scene()) {
		setColor(prop("color"));
	}

    return Capacitor::addedToScene(temporary);
}

bool LED::hasCustomSVG() {
	switch (m_viewIdentifier) {
		case ViewIdentifierClass::BreadboardView:
		case ViewIdentifierClass::IconView:
			return true;
		default:
			return ItemBase::hasCustomSVG();
	}
}

bool LED::canEditPart() {
	return true;
}

ItemBase::PluralType LED::isPlural() {
	return Plural;
}

void LED::setProp(const QString & prop, const QString & value) 
{
	Capacitor::setProp(prop, value);

	if (prop.compare("color") == 0) {
		setColor(value);
	}
}

void LED::slamColor(QDomElement & element, const QString & colorString)
{
	QString id = element.attribute("id");
	if (id.startsWith("color_")) {
		element.setAttribute("fill", colorString);
	}

	QDomElement child = element.firstChildElement();
	while (!child.isNull()) {
		slamColor(child, colorString);
		child = child.nextSiblingElement();
	}
}

void LED::setColor(const QString & color) 
{
	switch (m_viewLayerID) {
		case ViewLayer::Breadboard:
		case ViewLayer::Icon:
			break;
		default:
			return;
	}

	loadExtraRenderer(getColorSVG(color, m_viewLayerID).toUtf8(),true);

	QString title = modelPart()->modelPartShared()->title();  // bypass any local title by going to modelPartShared
	if (title.startsWith("red", Qt::CaseInsensitive)) {
		title.remove(0, 3);
		QStringList strings = color.split(" ");
		modelPart()->setLocalTitle(strings.at(0) + title);
	}
	else if (title.endsWith("blue", Qt::CaseInsensitive)) {
		title.chop(4);
		QStringList strings = color.split(" ");
		modelPart()->setLocalTitle(title + strings.at(0));
	}
}

QString LED::getColorSVG(const QString & color, ViewLayer::ViewLayerID viewLayerID) 
{
	QString errorStr;
	int errorLine;
	int errorColumn;
	QDomDocument domDocument;
	if (!domDocument.setContent(viewLayerID == ViewLayer::Breadboard ? BreadboardSvg.value(m_filename) : IconSvg.value(m_filename), &errorStr, &errorLine, &errorColumn)) {
		return "";
	}

	QString colorString;
	foreach (PropertyDef * propertyDef, m_propertyDefs.keys()) {
		if (propertyDef->name.compare("color") == 0) {
			colorString = propertyDef->adjuncts.value(color, "");
			break;
		}
	}

	if (colorString.isEmpty()) return "";

	QDomElement root = domDocument.documentElement();
	slamColor(root, colorString);
	return domDocument.toString();
}

bool LED::setUpImage(ModelPart * modelPart, ViewIdentifierClass::ViewIdentifier viewIdentifier, const LayerHash & viewLayers, ViewLayer::ViewLayerID viewLayerID, ViewLayer::ViewLayerSpec viewLayerSpec, bool doConnectors, LayerAttributes & layerAttributes, QString & error)
{
	bool result = Capacitor::setUpImage(modelPart, viewIdentifier, viewLayers, viewLayerID, viewLayerSpec, doConnectors, layerAttributes, error);
	if (viewLayerID == ViewLayer::Breadboard && BreadboardSvg.value(m_filename).isEmpty() && result) {
		BreadboardSvg.insert(m_filename, QString(layerAttributes.loaded()));
	}
	else if (viewLayerID == ViewLayer::Icon && IconSvg.value(m_filename).isEmpty() && result) {
		IconSvg.insert(m_filename, QString(layerAttributes.loaded()));
	}
	return result;
}

const QString & LED::title() {
	m_title = prop("color") + " LED";
	return m_title;
}
