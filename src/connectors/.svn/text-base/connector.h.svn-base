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

#ifndef CONNECTOR_H
#define CONNECTOR_H

#include <QHash>
#include <QString>
#include <QList>
#include <QXmlStreamWriter>
#include <QGraphicsScene>
#include <QSvgRenderer>
#include <QPointer>

#include "../viewidentifierclass.h"
#include "../viewlayer.h"

class Connector : public QObject
{
Q_OBJECT
public:
	enum ConnectorType {
		Male,
		Female,
		Wire,
		Pad,
		Unknown
	};

public:
	Connector(class ConnectorShared *, class ModelPart * modelPart);
	~Connector();

	Connector::ConnectorType connectorType();
	void addViewItem(class ConnectorItem *);
	void removeViewItem(class ConnectorItem *);
	class ConnectorShared * connectorShared();
	void connectTo(Connector *);
	void disconnectFrom(Connector *);
	void saveAsPart(QXmlStreamWriter & writer);
	const QList<Connector *> & toConnectors();
	ConnectorItem * connectorItemByViewLayerID(ViewIdentifierClass::ViewIdentifier viewIdentifier, ViewLayer::ViewLayerID);
	bool connectionIsAllowed(Connector* that);
	const QString & connectorSharedID();
	const QString & connectorSharedName();	
	const QString & connectorSharedDescription();
	class ErcData * connectorSharedErcData();
	const QString & busID();
	class Bus * bus();
	void setBus(class Bus *);
	long modelIndex();
	ModelPart * modelPart();
	int connectorItemCount();
	void unprocess(ViewIdentifierClass::ViewIdentifier viewIdentifier, ViewLayer::ViewLayerID viewLayerID);
	struct SvgIdLayer * fullPinInfo(ViewIdentifierClass::ViewIdentifier viewId, ViewLayer::ViewLayerID viewLayerID);
	QList< QPointer<class ConnectorItem> > viewItems();
	const QString & legID(ViewIdentifierClass::ViewIdentifier, ViewLayer::ViewLayerID);
	void setConnectorLocalName(const QString &);
	const QString & connectorLocalName();

public:
	static void initNames();
	static const QString & connectorNameFromType(ConnectorType);
	static ConnectorType connectorTypeFromName(const QString & name);

protected:
	void writeLayerAttr(QXmlStreamWriter &writer, ViewLayer::ViewLayerID);
	void writeSvgIdAttr(QXmlStreamWriter &writer, ViewIdentifierClass::ViewIdentifier view, QString connId);
	void writeTerminalIdAttr(QXmlStreamWriter &writer, ViewIdentifierClass::ViewIdentifier view, QString terminalId);

protected:
	QPointer<class ConnectorShared> m_connectorShared;
	QHash< int, QPointer<class ConnectorItem> > m_connectorItems;
	QList<Connector *> m_toConnectors;
	QPointer<class ModelPart> m_modelPart;
	QPointer<class Bus> m_bus;
	QString m_connectorLocalName;

protected:
	static QHash<ConnectorType, QString> Names;
};

#endif
