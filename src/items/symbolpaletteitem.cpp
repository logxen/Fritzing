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

$Revision: 5857 $:
$Author: cohen@irascible.com $:
$Date: 2012-02-12 09:16:07 -0800 (Sun, 12 Feb 2012) $

********************************************************************/

#include "symbolpaletteitem.h"
#include "../debugdialog.h"
#include "../connectors/connectoritem.h"
#include "../connectors/bus.h"
#include "moduleidnames.h"
#include "../fsvgrenderer.h"
#include "../utils/textutils.h"
#include "../utils/focusoutcombobox.h"
#include "../sketch/infographicsview.h"
#include "partlabel.h"

#include <QLineEdit>
#include <QMultiHash>

#define VOLTAGE_HASH_CONVERSION 1000000
#define FROMVOLTAGE(v) ((long) (v * VOLTAGE_HASH_CONVERSION))

static QMultiHash<long, QPointer<ConnectorItem> > localVoltages;			// Qt doesn't do Hash keys with double
static QList< QPointer<ConnectorItem> > localGrounds;
static QList<double> Voltages;
double SymbolPaletteItem::DefaultVoltage = 5;

SymbolPaletteItem::SymbolPaletteItem( ModelPart * modelPart, ViewIdentifierClass::ViewIdentifier viewIdentifier, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu, bool doLabel)
	: PaletteItem(modelPart, viewIdentifier, viewGeometry, id, itemMenu, doLabel)
{
	m_connector0 = m_connector1 = NULL;
	m_voltage = 0;

	m_voltageReference = (modelPart->properties().value("type").compare("voltage reference") == 0);

	if (Voltages.count() == 0) {
		Voltages.append(0.0);
		Voltages.append(3.3);
		Voltages.append(5.0);
		Voltages.append(12.0);
	}

	bool ok;
	double temp = modelPart->prop("voltage").toDouble(&ok);
	if (ok) {
		m_voltage = temp;
	}
	else {
		temp = modelPart->properties().value("voltage").toDouble(&ok);
		if (ok) {
			m_voltage = SymbolPaletteItem::DefaultVoltage;
		}
		modelPart->setProp("voltage", m_voltage);
	}
	if (!Voltages.contains(m_voltage)) {
		Voltages.append(m_voltage);
	}
}

SymbolPaletteItem::~SymbolPaletteItem() {
	if (m_connector0) localGrounds.removeOne(m_connector0);
	if (m_connector1) localGrounds.removeOne(m_connector1);
	localGrounds.removeOne(NULL);
	foreach (long key, localVoltages.uniqueKeys()) {
		if (m_connector0) {
			localVoltages.remove(key, m_connector0);
		}
		if (m_connector1) {
			localVoltages.remove(key, m_connector1);
		}
		localVoltages.remove(key, NULL);
	}
}

void SymbolPaletteItem::removeMeFromBus(double v) {
	foreach (ConnectorItem * connectorItem, cachedConnectorItems()) {
		double nv = useVoltage(connectorItem);
		if (nv == v) {
			//connectorItem->debugInfo(QString("remove %1").arg(useVoltage(connectorItem)));

			bool gotOne = localGrounds.removeOne(connectorItem);
			int count = localVoltages.remove(FROMVOLTAGE(v), connectorItem);
			localVoltages.remove(FROMVOLTAGE(v), NULL);
			if (count == 0 && !gotOne) {
				DebugDialog::debug(QString("removeMeFromBus failed %1 %2 %3 %4")
					.arg(this->id())
					.arg(connectorItem->connectorSharedID())
					.arg(v).arg(nv));
			}
		}
	}
	localGrounds.removeOne(NULL);

}

ConnectorItem* SymbolPaletteItem::newConnectorItem(Connector *connector) 
{
	ConnectorItem * connectorItem = PaletteItemBase::newConnectorItem(connector);
	//if (m_viewIdentifier != ViewIdentifierClass::SchematicView) return connectorItem;

	if (connector->connectorSharedID().compare("connector0") == 0) {
		m_connector0 = connectorItem;
	}
	else if (connector->connectorSharedID().compare("connector1") == 0) {
		m_connector1 = connectorItem;
	}
	else {
		return connectorItem;
	}

	if (connectorItem->isGrounded()) {
		localGrounds.append(connectorItem);
		//connectorItem->debugInfo("new ground insert");
	}
	else {
		localVoltages.insert(FROMVOLTAGE(useVoltage(connectorItem)), connectorItem);
		//connectorItem->debugInfo(QString("new voltage insert %1").arg(useVoltage(connectorItem)));
	}
	return connectorItem;
}

void SymbolPaletteItem::busConnectorItems(Bus * bus, QList<class ConnectorItem *> & items) {
	if (bus == NULL) return;

	PaletteItem::busConnectorItems(bus, items);

	//if (m_viewIdentifier != ViewIdentifierClass::SchematicView) return;

	//foreach (ConnectorItem * bc, items) {
		//bc->debugInfo(QString("bc %1").arg(bus->id()));
	//}

	QList< QPointer<ConnectorItem> > mitems;
	if (bus->id().compare("groundbus", Qt::CaseInsensitive) == 0) {
		mitems.append(localGrounds);
	}
	else {
		mitems.append(localVoltages.values(FROMVOLTAGE(m_voltage)));
	}
	foreach (ConnectorItem * connectorItem, mitems) {
		if (connectorItem == NULL) continue;
		if (connectorItem->scene() == this->scene()) {
			items.append(connectorItem);
			//connectorItem->debugInfo(QString("symbol bus %1").arg(bus->id()));
		}
	}
}

double SymbolPaletteItem::voltage() {
	return m_voltage;
}

void SymbolPaletteItem::setProp(const QString & prop, const QString & value) {
	if (prop.compare("voltage", Qt::CaseInsensitive) == 0) {
		setVoltage(value.toDouble());
		return;
	}

	PaletteItem::setProp(prop, value);
}

void SymbolPaletteItem::setVoltage(double v) {
	removeMeFromBus(m_voltage);

	m_voltage = v;
	m_modelPart->setProp("voltage", v);
	if (!Voltages.contains(v)) {
		Voltages.append(v);
	}

	//if (m_viewIdentifier != ViewIdentifierClass::SchematicView) return;

	foreach (ConnectorItem * connectorItem, cachedConnectorItems()) {
		if (connectorItem->isGrounded()) {
			localGrounds.append(connectorItem);
			//connectorItem->debugInfo("ground insert");

		}
		else {
			localVoltages.insert(FROMVOLTAGE(v), connectorItem);
			//connectorItem->debugInfo(QString("voltage insert %1").arg(useVoltage(connectorItem)));
		}
	}

	if (!m_voltageReference) return;

	QString svg = makeSvg();
	loadExtraRenderer(svg, false);

    if (m_partLabel) m_partLabel->displayTextsIf();
}

QString SymbolPaletteItem::makeSvg() {
	QString path = filename();
	QFile file(filename());
	QString svg;
	if (file.open(QFile::ReadOnly)) {
		svg = file.readAll();
		file.close();
		return replaceTextElement(svg);
	}

	return "";
}

QString SymbolPaletteItem::replaceTextElement(QString svg) {
	double v = ((int) (m_voltage * 1000)) / 1000.0;
	return TextUtils::replaceTextElement(svg, "label", QString::number(v) + "V");
}

QString SymbolPaletteItem::getProperty(const QString & key) {
	if (key.compare("voltage", Qt::CaseInsensitive) == 0) {
		return QString::number(m_voltage);
	}

	return PaletteItem::getProperty(key);
}

double SymbolPaletteItem::useVoltage(ConnectorItem * connectorItem) {
	return (connectorItem->connectorSharedName().compare("GND", Qt::CaseInsensitive) == 0) ? 0 : m_voltage;
}

ConnectorItem * SymbolPaletteItem::connector0() {
	return m_connector0;
}

ConnectorItem * SymbolPaletteItem::connector1() {
	return m_connector1;
}

void SymbolPaletteItem::addedToScene(bool temporary)
{
	if (this->scene()) {
		setVoltage(m_voltage);
	}

    return PaletteItem::addedToScene(temporary);
}

QString SymbolPaletteItem::retrieveSvg(ViewLayer::ViewLayerID viewLayerID, QHash<QString, QString> & svgHash, bool blackOnly, double dpi) 
{
	QString svg = PaletteItem::retrieveSvg(viewLayerID, svgHash, blackOnly, dpi);
	if (m_voltageReference) {
		switch (viewLayerID) {
			case ViewLayer::Schematic:
				return replaceTextElement(svg);
			default:
				break;
		}
	}

	return svg; 
}

bool SymbolPaletteItem::collectExtraInfo(QWidget * parent, const QString & family, const QString & prop, const QString & value, bool swappingEnabled, QString & returnProp, QString & returnValue, QWidget * & returnWidget)
{
	if ((prop.compare("voltage", Qt::CaseInsensitive) == 0) && 
		(moduleID().compare(ModuleIDNames::GroundModuleIDName) != 0)) 
	{

		FocusOutComboBox * edit = new FocusOutComboBox(parent);
		edit->setEnabled(swappingEnabled);
		int ix = 0;
		foreach (double v, Voltages) {
			edit->addItem(QString::number(v));
			if (v == m_voltage) {
				edit->setCurrentIndex(ix);
			}
			ix++;
		}

		QDoubleValidator * validator = new QDoubleValidator(edit);
		validator->setRange(-9999.99, 9999.99, 2);
		validator->setNotation(QDoubleValidator::StandardNotation);
		edit->setValidator(validator);

		edit->setObjectName("infoViewComboBox");


		connect(edit, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(voltageEntry(const QString &)));
		returnWidget = edit;	

		returnValue = m_voltage;
		returnProp = tr("voltage");
		return true;
	}

	return PaletteItem::collectExtraInfo(parent, family, prop, value, swappingEnabled, returnProp, returnValue, returnWidget);
}

void SymbolPaletteItem::voltageEntry(const QString & text) {
	InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView != NULL) {
		infoGraphicsView->setVoltage(text.toDouble(), true);
	}
}

ItemBase::PluralType SymbolPaletteItem::isPlural() {
	return Singular;
}

bool SymbolPaletteItem::hasPartNumberProperty()
{
	return false;
}


