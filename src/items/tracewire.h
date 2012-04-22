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

#ifndef TRACEWIRE_H
#define TRACEWIRE_H

#include "clipablewire.h"

class TraceWire : public ClipableWire
{
Q_OBJECT
public:
	TraceWire( ModelPart * modelPart, ViewIdentifierClass::ViewIdentifier,  const ViewGeometry & , long id, QMenu* itemMenu  ); 
	~TraceWire();

	bool collectExtraInfo(QWidget * parent, const QString & family, const QString & prop, const QString & value, bool swappingEnabled, QString & returnProp, QString & returnValue, QWidget * & returnWidget);
	bool canSwitchLayers();
	void setSchematic(bool schematic);

public:
	static TraceWire * getTrace(ConnectorItem *);
	static class QComboBox * createWidthComboBox(double currentMils, QWidget * parent);
	static int widthEntry(const QString & text, QObject * sender);

public:
	enum WireDirection {
		NoDirection,
		Vertical,
		Horizontal,
		Diagonal
	};

	void setWireDirection(TraceWire::WireDirection);
	TraceWire::WireDirection wireDirection();

public:
	static const int MinTraceWidthMils;
	static const int MaxTraceWidthMils;


protected:
	void setColorFromElement(QDomElement & element);

protected slots:
	void widthEntry(const QString & text);

protected:
	WireDirection m_wireDirection;


};

#endif
