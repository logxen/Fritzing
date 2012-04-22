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

$Revision: 5721 $:
$Author: cohen@irascible.com $:
$Date: 2012-01-03 07:53:58 -0800 (Tue, 03 Jan 2012) $

********************************************************************/

#ifndef VIRTUALWIRE_H
#define VIRTUALWIRE_H

#include "clipablewire.h"

class VirtualWire : public ClipableWire
{
	Q_OBJECT

public:
	VirtualWire( ModelPart * modelPart, ViewIdentifierClass::ViewIdentifier,  const ViewGeometry & , long id, QMenu* itemMenu  ); 
	~VirtualWire();
	
	void setHidden(bool hidden);
	void setInactive(bool inactivate);
	void tempRemoveAllConnections();
	void setColorWasNamed(bool);
	bool colorWasNamed();
	
protected:
	void paint (QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget );	
	void mousePressEvent(QGraphicsSceneMouseEvent *event);
	void connectionChange(ConnectorItem * onMe, ConnectorItem * onIt, bool connect);
 	class FSvgRenderer * setUpConnectors(class ModelPart *, ViewIdentifierClass::ViewIdentifier);
	void hideConnectors();	
	void inactivateConnectors();

protected:
	bool m_colorWasNamed;

};

#endif
