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

#ifndef VIEWLAYER_H
#define VIEWLAYER_H

#include <QString>
#include <QAction>
#include <QHash>
#include <QMultiHash>

#include "utils/misc.h"

class ViewLayer : public QObject
{
	Q_OBJECT

public:
	enum ViewLayerID {
		Icon,
		BreadboardBreadboard,
		Breadboard,
		BreadboardWire,
		BreadboardLabel,
		BreadboardRatsnest,
		BreadboardNote,
		BreadboardRuler,
		SchematicFrame,
		Schematic,
		SchematicWire,
		SchematicTrace,
		SchematicLabel,
		SchematicNote,
		SchematicRuler,
		Board,
		GroundPlane0,
		Silkscreen0,
		Silkscreen0Label,
		Copper0,
		Copper0Trace,
		GroundPlane1,
		Copper1,
		Copper1Trace,
		Silkscreen1,
		Silkscreen1Label,
		PartImage,
		PcbRatsnest,
		PcbNote,
		PcbRuler,
		UnknownLayer,
		ViewLayerCount
	};

	enum ViewLayerSpec {
		ThroughHoleThroughTop_OneLayer,
		ThroughHoleThroughTop_TwoLayers,
		ThroughHoleThroughBottom_TwoLayers,
		SMDOnTop_TwoLayers,
		SMDOnBottom_OneLayer,
		SMDOnBottom_TwoLayers,
		WireOnTop_TwoLayers,
		WireOnBottom_OneLayer,
		WireOnBottom_TwoLayers,
		GroundPlane_Top,
		GroundPlane_Bottom,
		Top,
		Bottom,
		TopAndBottom,
		UnknownSpec
	};

public:
	static const QString HolesColor;
	static const QString Copper0Color;
	static const QString Copper1Color;
	static const QString Silkscreen1Color;
	static const QString Silkscreen0Color;

protected:
	static double zIncrement;
	static QHash<ViewLayerID, StringPair *> names;
	static QMultiHash<ViewLayerID, ViewLayerID> alternatives;
	static QMultiHash<ViewLayerID, ViewLayerID> unconnectables;
	static QHash<QString, ViewLayerID> xmlHash;

public:
	ViewLayer(ViewLayerID, bool visible, double initialZ);
	~ViewLayer();

	void setAction(QAction *);
	QAction* action();
	QString & displayName();
	bool visible();
	void setVisible(bool);
	double nextZ();
	ViewLayerID viewLayerID();
	double incrementZ(double);
	ViewLayer * parentLayer();
	void setParentLayer(ViewLayer *);
	const QList<ViewLayer *> & childLayers();
	bool alreadyInLayer(double z);
	void resetNextZ(double z);
	void setActive(bool);
	bool isActive();
	bool includeChildLayers();
	void setIncludeChildLayers(bool);

public:
	static ViewLayerID viewLayerIDFromXmlString(const QString &);
	static const QString & viewLayerNameFromID(ViewLayerID);
	static const QString & viewLayerXmlNameFromID(ViewLayerID);
	static void initNames();
	static double getZIncrement();
	static void cleanup();
	static QList<ViewLayerID> findAlternativeLayers(ViewLayerID);
	static bool canConnect(ViewLayerID, ViewLayerID);
	static ViewLayer::ViewLayerSpec specFromID(ViewLayer::ViewLayerID);
	static const QList<ViewLayer::ViewLayerID> & copperLayers(ViewLayer::ViewLayerSpec);
	static const QList<ViewLayer::ViewLayerID> & maskLayers(ViewLayer::ViewLayerSpec);
	static const QList<ViewLayer::ViewLayerID> & silkLayers(ViewLayer::ViewLayerSpec);
	static const QList<ViewLayer::ViewLayerID> & outlineLayers();
	static const QList<ViewLayer::ViewLayerID> & drillLayers();
	static bool isCopperLayer(ViewLayer::ViewLayerID);
	static bool isNonCopperLayer(ViewLayer::ViewLayerID viewLayerID);  // for pcb view layers only


protected:
	bool m_visible;
	ViewLayerID m_viewLayerID;
	QAction* m_action;
	double m_nextZ;
	double m_initialZ;
	QList<ViewLayer *> m_childLayers;
	ViewLayer * m_parentLayer;
	bool m_active;
	bool m_includeChildLayers;

};

typedef QHash<ViewLayer::ViewLayerID, ViewLayer *> LayerHash;
typedef QList<ViewLayer::ViewLayerID> LayerList;

Q_DECLARE_METATYPE( ViewLayer* );

#endif
