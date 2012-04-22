/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2011 Fachhochschule Potsdam - http://fh-potsdam.de

Fritzing is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.a

Fritzing is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Fritzing.  If not, see <http://www.gnu.org/licenses/>.

********************************************************************

$Revision: 5913 $:
$Author: cohen@irascible.com $:
$Date: 2012-03-14 11:23:42 -0700 (Wed, 14 Mar 2012) $

********************************************************************/

#include "autorouter.h"
#include "../sketch/pcbsketchwidget.h"
#include "../debugdialog.h"
#include "../items/virtualwire.h"
#include "../items/tracewire.h"
#include "../items/jumperitem.h"
#include "../utils/graphicsutils.h"
#include "../connectors/connectoritem.h"
#include "../items/moduleidnames.h"
#include "../processeventblocker.h"

#include <qmath.h>
#include <QApplication>

Autorouter::Autorouter(PCBSketchWidget * sketchWidget)
{
	m_sketchWidget = sketchWidget;
	m_stopTracing = m_cancelTrace = m_cancelled = false;
}

Autorouter::~Autorouter(void)
{
}

void Autorouter::cleanUpNets() {
	foreach (QList<ConnectorItem *> * connectorItems, m_allPartConnectorItems) {
		delete connectorItems;
	}
	m_allPartConnectorItems.clear();
}

void Autorouter::updateRoutingStatus() {
	RoutingStatus routingStatus;
	routingStatus.zero();
	m_sketchWidget->updateRoutingStatus(routingStatus, true);
}

TraceWire * Autorouter::drawOneTrace(QPointF fromPos, QPointF toPos, double width, ViewLayer::ViewLayerSpec viewLayerSpec)
{
	long newID = ItemBase::getNextID();
	ViewGeometry viewGeometry;
	viewGeometry.setWireFlags(m_sketchWidget->getTraceFlag());
	viewGeometry.setAutoroutable(true);
	viewGeometry.setLoc(fromPos);
	QLineF line(0, 0, toPos.x() - fromPos.x(), toPos.y() - fromPos.y());
	viewGeometry.setLine(line);

	ItemBase * trace = m_sketchWidget->addItem(m_sketchWidget->paletteModel()->retrieveModelPart(ModuleIDNames::WireModuleIDName), 
		  viewLayerSpec, BaseCommand::SingleView, viewGeometry, newID, -1, NULL, NULL);
	if (trace == NULL) {
		// we're in trouble
		DebugDialog::debug("autorouter unable to draw one trace");
		return NULL;
	}

	// addItem calls trace->setSelected(true) so unselect it (TODO: this may no longer be necessar)
	trace->setSelected(false);
	TraceWire * traceWire = dynamic_cast<TraceWire *>(trace);
	if (traceWire == NULL) {
		DebugDialog::debug("autorouter unable to draw one trace as trace");
		return NULL;
	}


	m_sketchWidget->setClipEnds(traceWire, false);
	traceWire->setColorString(m_sketchWidget->traceColor(viewLayerSpec), 1.0);
	traceWire->setWireWidth(width, m_sketchWidget, m_sketchWidget->getWireStrokeWidth(traceWire, width));

	return traceWire;
}

void Autorouter::cancel() {
	m_cancelled = true;
}

void Autorouter::cancelTrace() {
	m_cancelTrace = true;
}

void Autorouter::stopTracing() {
	m_stopTracing = true;
}
