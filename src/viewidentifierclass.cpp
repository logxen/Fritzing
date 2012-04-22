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


#include "viewidentifierclass.h"

/////////////////////////////////

class NameTriple {

public:
	NameTriple(const QString & _xmlName, const QString & _viewName, const QString & _naturalName) {
		m_xmlName = _xmlName;
		m_viewName = _viewName;
		m_naturalName = _naturalName;
	}

	QString & xmlName() {
		return m_xmlName;
	}

	QString & viewName() {
		return m_viewName;
	}

	QString & naturalName() {
		return m_naturalName;
	}

protected:
	QString m_xmlName;
	QString m_naturalName;
	QString m_viewName;
};


/////////////////////////////////

QHash <ViewIdentifierClass::ViewIdentifier, NameTriple * > ViewIdentifierClass::Names;
static LayerList ii;
static LayerList bb;
static LayerList ss;
static LayerList pp;
static LayerList ee;

QString & ViewIdentifierClass::viewIdentifierName(ViewIdentifierClass::ViewIdentifier viewIdentifier) {
	if (viewIdentifier < 0 || viewIdentifier >= ViewIdentifierClass::ViewCount) {
		throw "ViewIdentifierClass::viewIdentifierName bad identifier";
	}

	return Names[viewIdentifier]->viewName();
}

QString & ViewIdentifierClass::viewIdentifierXmlName(ViewIdentifierClass::ViewIdentifier viewIdentifier) {
	if (viewIdentifier < 0 || viewIdentifier >= ViewIdentifierClass::ViewCount) {
		throw "ViewIdentifierClass::viewIdentifierXmlName bad identifier";
	}

	return Names[viewIdentifier]->xmlName();
}

QString & ViewIdentifierClass::viewIdentifierNaturalName(ViewIdentifierClass::ViewIdentifier viewIdentifier) {
	if (viewIdentifier < 0 || viewIdentifier >= ViewIdentifierClass::ViewCount) {
		throw "ViewIdentifierClass::viewIdentifierNaturalName bad identifier";
	}

	return Names[viewIdentifier]->naturalName();
}

void ViewIdentifierClass::initNames() {
	if (Names.count() == 0) {
		Names.insert(ViewIdentifierClass::IconView, new NameTriple("iconView", QObject::tr("icon view"), "icon"));
		Names.insert(ViewIdentifierClass::BreadboardView, new NameTriple("breadboardView", QObject::tr("breadboard view"), "breadboard"));
		Names.insert(ViewIdentifierClass::SchematicView, new NameTriple("schematicView", QObject::tr("schematic view"), "schematic"));
		Names.insert(ViewIdentifierClass::PCBView, new NameTriple("pcbView", QObject::tr("pcb view"), "pcb"));
	}

	if (bb.count() == 0) {
		ii << ViewLayer::Icon;
		bb << ViewLayer::BreadboardBreadboard << ViewLayer::Breadboard 
			<< ViewLayer::BreadboardWire << ViewLayer::BreadboardLabel 
			<< ViewLayer::BreadboardRatsnest 
			<< ViewLayer::BreadboardNote << ViewLayer::BreadboardRuler;
		ss << ViewLayer::SchematicFrame << ViewLayer::Schematic 
			<< ViewLayer::SchematicWire 
			<< ViewLayer::SchematicTrace << ViewLayer::SchematicLabel 
			<< ViewLayer::SchematicNote <<  ViewLayer::SchematicRuler;
		pp << ViewLayer::Board << ViewLayer::GroundPlane0 
			<< ViewLayer::Silkscreen0 << ViewLayer::Silkscreen0Label
			<< ViewLayer::Copper0 
			<< ViewLayer::Copper0Trace << ViewLayer::GroundPlane1 
			<< ViewLayer::Copper1 << ViewLayer::Copper1Trace 
			<< ViewLayer::PcbRatsnest 
			<< ViewLayer::Silkscreen1 << ViewLayer::Silkscreen1Label 
			<< ViewLayer::PartImage 
			<< ViewLayer::PcbNote << ViewLayer::PcbRuler;
	}
}

ViewIdentifierClass::ViewIdentifier ViewIdentifierClass::idFromXmlName(const QString & name) {
	foreach (ViewIdentifier id, Names.keys()) {
		NameTriple * nameTriple = Names.value(id);
		if (name.compare(nameTriple->xmlName()) == 0) return id;
	}

	return UnknownView;
}

void ViewIdentifierClass::cleanup() {
	foreach (NameTriple * nameTriple, Names) {
		delete nameTriple;
	}
	Names.clear();
}

const LayerList & ViewIdentifierClass::layersForView(ViewIdentifierClass::ViewIdentifier viewIdentifier) {
	switch(viewIdentifier) {
		case IconView:
			return ii;
		case BreadboardView:
			return bb;
		case SchematicView:
			return ss;
		case PCBView:
			return pp;
		default:
			return ee;
	}
}

bool ViewIdentifierClass::viewHasLayer(ViewIdentifierClass::ViewIdentifier viewIdentifier, ViewLayer::ViewLayerID viewLayerID) {
	return layersForView(viewIdentifier).contains(viewLayerID);
}
