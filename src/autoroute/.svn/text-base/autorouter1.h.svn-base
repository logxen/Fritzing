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

#ifndef AUTOROUTER1_H
#define AUTOROUTER1_H

#include <QAction>
#include <QHash>
#include <QVector>
#include <QList>
#include <QPointF>
#include <QGraphicsItem>
#include <QLine>
#include <QProgressDialog>
#include <QUndoCommand>

#include "autorouter.h"
#include "../viewgeometry.h"
#include "../viewlayer.h"

class Autorouter1 : public Autorouter
{
	Q_OBJECT

public:
	Autorouter1(class PCBSketchWidget *);

	void start();
	
protected:
	bool drawTrace(class ConnectorItem * from, class ConnectorItem * to, const QPolygonF & boundingPoly, QList<class Wire *> & wires);
	bool drawTrace(QPointF fromPos, QPointF toPos, class ConnectorItem * from, class ConnectorItem * to, QList<class Wire *> & wires, const QPolygonF & boundingPoly, int level, QPointF endPos, bool recurse, bool & shortcut);
	bool tryLeftAndRight(QPointF fromPos, QPointF toPos, class ConnectorItem * from, class ConnectorItem * to, QPointF right, QPointF left, QList<class Wire *> & wires, const QPolygonF & boundingPoly, int level, QPointF endPos, bool & shortcut);
	bool tryOne(QPointF fromPos, QPointF toPos, class ConnectorItem * from, class ConnectorItem * to, QPointF midPos, QList<class Wire *> & wires, const QPolygonF & boundingPoly, int level, QPointF endPos, bool & shortcut);
	bool tryWithWires(QPointF fromPos, QPointF toPos, class ConnectorItem * from, class ConnectorItem * to, QList<class Wire *> & wires, class ConnectorItem * end, QList<class Wire *> & chainedWires, const QPolygonF & boundingPoly, int level, QPointF endPos, bool & shortcut);
	bool tryWithWire(QPointF fromPos, QPointF toPos, class ConnectorItem * from, class ConnectorItem * to, QList<class Wire *> & wires, QPointF midpoint, QList<class Wire *> & chainedWires, const QPolygonF & boundingPoly, int level, QPointF endPos, bool & shortcut);
	bool prePoly(QGraphicsItem * nearestObstacle, QPointF fromPos, QPointF toPos, QPointF & leftPoint, QPointF & rightPoint, bool adjust);
	void cleanUp();
	class JumperItem * drawJumperItem(struct JumperItemStruct *);
	void restoreOriginalState(QUndoCommand * parentCommand);
	void addToUndo(Wire * wire, QUndoCommand * parentCommand);
	void addToUndo(QUndoCommand * parentCommand, QList<struct JumperItemStruct *> &);
	void reduceWires(QList<Wire *> & wires, ConnectorItem * from, ConnectorItem * to, const QPolygonF & boundingPoly);
	Wire * reduceWiresAux(QList<Wire *> & wires, ConnectorItem * from, ConnectorItem * to, QPointF fromPos, QPointF toPos, const QPolygonF & boundingPoly);
	bool clean90(ConnectorItem * from, ConnectorItem * to, QList<Wire *> & oldWires);
	bool clean90(QPointF fromPos, QPointF toPos, QList<Wire *> & newWires, int level);
	QPointF calcPrimePoint(ConnectorItem *);
	void findNearestIntersection(QLineF & l1, QPointF & fromPos, const QPolygonF & boundingPoly, bool & inBounds, QPointF & nearestBoundsIntersection, qreal & nearestBoundsIntersectionDistance); 
	bool hitsObstacle(class ItemBase * traceWire, ItemBase * ignore); 
	bool drawThree(QPointF fromPos, QPointF toPos, QPointF d1, QPointF d2, QList<Wire *> & newWires, int level, bool recurse);
	bool drawTwo(QPointF fromPos, QPointF toPos, QPointF d1, QList<Wire *> & newWires, int level, bool recurse);
	void clearLastDrawTraces();
	void reduceColinearWires(QList<Wire *> &);
	bool sameY(const QPointF & fromPos0, const QPointF & fromPos1, const QPointF & toPos0, const QPointF & toPos1);
	bool sameX(const QPointF & fromPos0, const QPointF & fromPos1, const QPointF & toPos0, const QPointF & toPos1);
	bool findSpaceFor(ConnectorItem * & from, class JumperItem *, struct JumperItemStruct *, QPointF & candidate); 
	void dijkstraNets(QHash<ConnectorItem *, int> & indexer, QVector<int> & netCounters, QList<struct Edge *> & edges);
	void dijkstra(QList<class ConnectorItem *> & vertices, QHash<class ConnectorItem *, int> & indexer, QVector< QVector<double> > & adjacency, ViewGeometry::WireFlags skipFlags);
	void addSubedge(Wire * wire, QList<ConnectorItem *> & toConnectorItems, QList<struct Subedge *> & subedges);
	bool traceSubedge(Subedge* subedge, QList<Wire *> & wires, ItemBase * partForBounds, const QPolygonF & boundingPoly, QGraphicsLineItem *);
	ItemBase * getPartForBounds(struct Edge *);
	void fixupJumperItems(QList<struct JumperItemStruct *> &);
	void runEdges(QList<Edge *> & edges, QGraphicsLineItem * lineItem, 	
				  QList<struct JumperItemStruct *> & jumperItemStructs,
				  QVector<int> & netCounters, struct RoutingStatus &);
	void clearEdges(QList<Edge *> & edges);
	void doCancel(QUndoCommand * parentCommand);
	bool alreadyJumper(QList<struct JumperItemStruct *> & jumperItemStructs, ConnectorItem * from, ConnectorItem * to);
	bool hasCollisions(JumperItem *, ViewLayer::ViewLayerID, QGraphicsItem *, ConnectorItem * from); 
	void updateProgress(int num, int denom);

protected:
	static void calcDistance(QGraphicsItem * & nearestObstacle, double & nearestObstacleDistance, QPointF fromPos, QGraphicsItem * item);
	static double calcDistance(QPointF fromPos, QGraphicsItem *);
	static double distanceToLine(QPointF fromPos, QPointF p1, QPointF p2);
	static void clearTraces(PCBSketchWidget * sketchWidget, bool deleteAll, QUndoCommand * parentCommand);
	static void addUndoConnections(PCBSketchWidget * sketchWidget, bool connect, QList<Wire *> & wires, QUndoCommand * parentCommand);

protected:
	QList< QLine * > m_lastDrawTraces;
	QList<class ConnectorItem *> * m_drawingNet;
	int m_autobail;
	QGraphicsItem * m_nearestObstacle;
	QList<Wire *> m_cleanWires;
	ViewLayer::ViewLayerSpec m_viewLayerSpec;
};

#endif
