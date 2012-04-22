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

#ifndef AUTOROUTER_H
#define AUTOROUTER_H

#include <QAction>
#include <QHash>
#include <QVector>
#include <QList>
#include <QPointF>
#include <QGraphicsItem>
#include <QLine>
#include <QProgressDialog>
#include <QUndoCommand>

#include "../viewgeometry.h"
#include "../viewlayer.h"
#include "../connectors/connectoritem.h"

class Autorouter : public QObject
{
	Q_OBJECT

public:
	Autorouter(class PCBSketchWidget *);
	virtual ~Autorouter(void);

	virtual void start()=0;
	
protected:
	virtual void cleanUpNets();
	virtual void updateRoutingStatus();
	virtual class TraceWire * drawOneTrace(QPointF fromPos, QPointF toPos, double width, ViewLayer::ViewLayerSpec);

public slots:
	virtual void cancel();
	virtual void cancelTrace();
	virtual void stopTracing();

signals:
	void setMaximumProgress(int);
	void setProgressValue(int);
	void wantTopVisible();
	void wantBottomVisible();	
	void wantBothVisible();
	void setProgressMessage(const QString &);

protected:
	class PCBSketchWidget * m_sketchWidget;
	QList< QList<class ConnectorItem *>* > m_allPartConnectorItems;
	bool m_cancelled;
	bool m_cancelTrace;
	bool m_stopTracing;
	bool m_bothSidesNow;
	int m_maximumProgressPart;
	int m_currentProgressPart;
};

#endif
