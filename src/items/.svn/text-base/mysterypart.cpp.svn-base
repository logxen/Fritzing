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

#include "mysterypart.h"
#include "../utils/graphicsutils.h"
#include "../fsvgrenderer.h"
#include "../sketch/infographicsview.h"
#include "../commands.h"
#include "../utils/textutils.h"
#include "../layerattributes.h"
#include "partlabel.h"
#include "../connectors/connectoritem.h"


#include <QDomNodeList>
#include <QDomDocument>
#include <QDomElement>
#include <QLineEdit>

static QStringList Spacings;
static QRegExp Digits("(\\d)+");
static QRegExp DigitsMil("(\\d)+mil");

static const int MinSipPins = 1;
static const int MaxSipPins = 64;
static const int MinDipPins = 4;
static const int MaxDipPins = 64;


// TODO
//	save into parts bin

MysteryPart::MysteryPart( ModelPart * modelPart, ViewIdentifierClass::ViewIdentifier viewIdentifier, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu, bool doLabel)
	: PaletteItem(modelPart, viewIdentifier, viewGeometry, id, itemMenu, doLabel)
{
	m_changingSpacing = false;
	m_chipLabel = modelPart->prop("chip label").toString();
	if (m_chipLabel.isEmpty()) {
		m_chipLabel = modelPart->properties().value("chip label", "?");
		modelPart->setProp("chip label", m_chipLabel);
	}
	m_spacing = modelPart->prop("spacing").toString();
	if (m_spacing.isEmpty()) {
		m_spacing = modelPart->properties().value("spacing", "300mil");
		modelPart->setProp("spacing", m_spacing);
	}
}

MysteryPart::~MysteryPart() {
}

void MysteryPart::setProp(const QString & prop, const QString & value) {
	if (prop.compare("chip label", Qt::CaseInsensitive) == 0) {
		setChipLabel(value, false);
		return;
	}

	if (prop.compare("spacing", Qt::CaseInsensitive) == 0) {
		setSpacing(value, false);
		return;
	}

	PaletteItem::setProp(prop, value);
}

void MysteryPart::setSpacing(QString spacing, bool force) {
	if (!force && m_spacing.compare(spacing) == 0) return;
	if (!isDIP()) return;

	switch (this->m_viewIdentifier) {
		case ViewIdentifierClass::BreadboardView:
		case ViewIdentifierClass::PCBView:
			{
				InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
				if (infoGraphicsView == NULL) break;

				// hack the dom element and call setUpImage
				FSvgRenderer::removeFromHash(moduleID(), "");
				QDomElement element = LayerAttributes::getSvgElementLayers(modelPart()->domDocument(), m_viewIdentifier);
				if (element.isNull()) break;

				QString filename = element.attribute("image");
				if (filename.isEmpty()) break;

				if (spacing.indexOf(Digits) < 0) break;

				QString newSpacing = Digits.cap(0);		
				filename.replace(DigitsMil, newSpacing + "mil");
				element.setAttribute("image", filename);

				m_changingSpacing = true;
				resetImage(infoGraphicsView);
				m_changingSpacing = false;

				if (m_viewIdentifier == ViewIdentifierClass::BreadboardView) {
					if (modelPart()->properties().value("chip label", "").compare(m_chipLabel) != 0) {
						setChipLabel(m_chipLabel, true);
					}
				}

				updateConnections();
			}
			break;
		default:
			break;
	}

	m_spacing = spacing;
	modelPart()->setProp("spacing", spacing);

    if (m_partLabel) m_partLabel->displayTextsIf();

}

void MysteryPart::setChipLabel(QString chipLabel, bool force) {

	if (!force && m_chipLabel.compare(chipLabel) == 0) return;

	m_chipLabel = chipLabel;

	QString svg;
	switch (this->m_viewIdentifier) {
		case ViewIdentifierClass::BreadboardView:
			svg = makeSvg(chipLabel, true);
			break;
		case ViewIdentifierClass::SchematicView:
			svg = makeSvg(chipLabel, false);
			svg = retrieveSchematicSvg(svg);
			break;
		default:
			break;
	}

	loadExtraRenderer(svg, false);

	modelPart()->setProp("chip label", chipLabel);

    if (m_partLabel) m_partLabel->displayTextsIf();

}

QString MysteryPart::retrieveSvg(ViewLayer::ViewLayerID viewLayerID, QHash<QString, QString> & svgHash, bool blackOnly, double dpi) 
{
	QString svg = PaletteItem::retrieveSvg(viewLayerID, svgHash, blackOnly, dpi);
	switch (viewLayerID) {
		case ViewLayer::Breadboard:
		case ViewLayer::Icon:
			return TextUtils::replaceTextElement(svg, "label", m_chipLabel);

		case ViewLayer::Schematic:
			svg = retrieveSchematicSvg(svg);
			return TextUtils::removeSVGHeader(svg);

		default:
			break;
	}

	return svg; 
}

QString MysteryPart::retrieveSchematicSvg(QString & svg) {

	bool hasLocal = false;
	QStringList labels = getPinLabels(hasLocal);
		
	if (hasLocal) {
		svg = makeSchematicSvg(labels, false);
	}

	return TextUtils::replaceTextElement(svg, "label", m_chipLabel);
}


QString MysteryPart::makeSvg(const QString & chipLabel, bool replace) {
	QString path = filename();
	QFile file(filename());
	QString svg;
	if (file.open(QFile::ReadOnly)) {
		svg = file.readAll();
		file.close();
		if (!replace) return svg;

		return TextUtils::replaceTextElement(svg, "label", chipLabel);
	}

	return "";
}

QStringList MysteryPart::collectValues(const QString & family, const QString & prop, QString & value) {
	if (prop.compare("spacing", Qt::CaseInsensitive) == 0) {
		QStringList values;
		if (isDIP()) {
			foreach (QString f, spacings()) {
				values.append(f);
			}
		}
		else {
			values.append(m_spacing);
		}

		value = m_spacing;
		return values;
	}

	if (prop.compare("pins", Qt::CaseInsensitive) == 0) {
		QStringList values;
		value = modelPart()->properties().value("pins");

		if (moduleID().contains("dip")) {
			for (int i = MinDipPins; i <= MaxDipPins; i += 2) {
				values << QString::number(i);
			}
		}
		else {
			for (int i = MinSipPins; i <= MaxSipPins; i++) {
				values << QString::number(i);
			}
		}
		
		return values;
	}


	return PaletteItem::collectValues(family, prop, value);
}

bool MysteryPart::collectExtraInfo(QWidget * parent, const QString & family, const QString & prop, const QString & value, bool swappingEnabled, QString & returnProp, QString & returnValue, QWidget * & returnWidget)
{
	if (prop.compare("chip label", Qt::CaseInsensitive) == 0) {
		returnProp = tr("label");
		returnValue = m_chipLabel;

		QLineEdit * e1 = new QLineEdit(parent);
		e1->setEnabled(swappingEnabled);
		e1->setText(m_chipLabel);
		connect(e1, SIGNAL(editingFinished()), this, SLOT(chipLabelEntry()));
		e1->setObjectName("infoViewLineEdit");


		returnWidget = e1;

		return true;
	}

	return PaletteItem::collectExtraInfo(parent, family, prop, value, swappingEnabled, returnProp, returnValue, returnWidget);
}

QString MysteryPart::getProperty(const QString & key) {
	if (key.compare("chip label", Qt::CaseInsensitive) == 0) {
		return m_chipLabel;
	}

	if (key.compare("spacing", Qt::CaseInsensitive) == 0) {
		return m_spacing;
	}

	return PaletteItem::getProperty(key);
}

QString MysteryPart::chipLabel() {
	return m_chipLabel;
}

void MysteryPart::addedToScene(bool temporary)
{
	if (this->scene()) {
		setChipLabel(m_chipLabel, true);
		setSpacing(m_spacing, true);
	}

    PaletteItem::addedToScene(temporary);
}

const QString & MysteryPart::title() {
	return m_chipLabel;
}

bool MysteryPart::hasCustomSVG() {
	switch (m_viewIdentifier) {
		case ViewIdentifierClass::BreadboardView:
		case ViewIdentifierClass::SchematicView:
		case ViewIdentifierClass::IconView:
		case ViewIdentifierClass::PCBView:
			return true;
		default:
			return ItemBase::hasCustomSVG();
	}
}

void MysteryPart::chipLabelEntry() {
	QLineEdit * edit = qobject_cast<QLineEdit *>(sender());
	if (edit == NULL) return;

	if (edit->text().compare(this->chipLabel()) == 0) return;

	InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView != NULL) {
		infoGraphicsView->setProp(this, "chip label", tr("chip label"), this->chipLabel(), edit->text(), true);
	}
}

ConnectorItem* MysteryPart::newConnectorItem(Connector *connector) {
	if (m_changingSpacing) {
		return connector->connectorItemByViewLayerID(viewIdentifier(), viewLayerID());
	}

	return PaletteItem::newConnectorItem(connector);
}

ConnectorItem* MysteryPart::newConnectorItem(ItemBase * layerKin, Connector *connector) {
	if (m_changingSpacing) {
		return connector->connectorItemByViewLayerID(viewIdentifier(), layerKin->viewLayerID());
	}

	return PaletteItem::newConnectorItem(layerKin, connector);
}

const QString & MysteryPart::spacing() {
	return m_spacing;
}

bool MysteryPart::onlySpacingChanges(QMap<QString, QString> & propsMap) {
	if (propsMap.value("spacing", "").compare(m_spacing) == 0) return false;

	if (modelPart()->properties().value("pins", "").compare(propsMap.value("pins", "")) != 0) return false;

	if (otherPropsChange(propsMap)) return false;

	return true;
}

bool MysteryPart::isDIP() {
	QString layout = modelPart()->properties().value("layout", "");
	return (layout.indexOf("double", 0, Qt::CaseInsensitive) >= 0);
}

bool MysteryPart::otherPropsChange(const QMap<QString, QString> & propsMap) {
	QString layout = modelPart()->properties().value("layout", "");
	return (layout.compare(propsMap.value("layout", "")) != 0);
}

const QStringList & MysteryPart::spacings() {
	if (Spacings.count() == 0) {
		Spacings << "100mil" << "200mil" << "300mil" << "400mil" << "500mil" << "600mil" << "700mil" << "800mil" << "900mil";
	}
	return Spacings;
}

ItemBase::PluralType MysteryPart::isPlural() {
	return Plural;
}

QString MysteryPart::genSipFZP(const QString & moduleid)
{
	return PaletteItem::genFZP(moduleid, "mystery_part_sipFzpTemplate", MinSipPins, MaxSipPins, 1, false);
}

QString MysteryPart::genDipFZP(const QString & moduleid)
{
	return PaletteItem::genFZP(moduleid, "mystery_part_dipFzpTemplate", MinDipPins, MaxDipPins, 2, false);
}

QString MysteryPart::genModuleID(QMap<QString, QString> & currPropsMap)
{
	QString value = currPropsMap.value("layout");
	QString pins = currPropsMap.value("pins");
	if (value.contains("single", Qt::CaseInsensitive)) {
		return QString("mystery_part_%1").arg(pins);
	}
	else {
		int p = pins.toInt();
		if (p < 4) p = 4;
		if (p % 2 == 1) p--;
		return QString("mystery_part_%1_dip_300mil").arg(p);
	}
}

QString MysteryPart::makeSchematicSvg(const QString & expectedFileName) 
{
	bool sip = expectedFileName.contains("sip", Qt::CaseInsensitive);

	QStringList pieces = expectedFileName.split("_");
	if (sip) {
		if (pieces.count() != 5) return "";
	}
	else {
		if (pieces.count() != 4) return "";
	}

	int pins = pieces.at(2).toInt();
	QStringList labels;
	for (int i = 0; i < pins; i++) {
		labels << QString::number(i + 1);
	}

	return makeSchematicSvg(labels, sip);
}

QString MysteryPart::makeSchematicSvg(const QStringList & labels, bool sip) 
{	
	int increment = GraphicsUtils::StandardSchematicSeparationMils;   // 7.5mm;
	int border = 30;
	double totalHeight = (labels.count() * increment) + increment + border;
	int textOffset = 50;
	int repeatTextOffset = 50;
	int fontSize = 255;
	int labelFontSize = 130;
	int defaultLabelWidth = 30;
	QString labelText = "?";
	double textMax = defaultLabelWidth;
	QFont font("Droid Sans", labelFontSize * 72 / GraphicsUtils::StandardFritzingDPI, QFont::Normal);
	QFontMetricsF fm(font);
	for (int i = 0; i < labels.count(); i++) {
		double w = fm.width(labels.at(i));
		if (w > textMax) textMax = w;
	}
	textMax = textMax * GraphicsUtils::StandardFritzingDPI / 72;

	int totalWidth = (5 * increment) + border;
	int innerWidth = 4 * increment;
	if (textMax > defaultLabelWidth) {
		totalWidth += (textMax - defaultLabelWidth);
		innerWidth += (textMax - defaultLabelWidth);
	}

	QString header("<?xml version='1.0' encoding='UTF-8' standalone='no'?>\n"
					"<svg xmlns:svg='http://www.w3.org/2000/svg' \n"
					"xmlns='http://www.w3.org/2000/svg' \n"
					"version='1.2' baseProfile='tiny' \n"
					"width='%7in' height='%1in' viewBox='0 0 %8 %2'>\n"
					"<g id='schematic'>\n"
					"<rect x='315' y='15' fill='none' width='%9' height='%3' stroke='#000000' stroke-linejoin='round' stroke-linecap='round' stroke-width='30' />\n"
					"<text id='label' x='%11' y='%4' fill='#000000' stroke='none' font-family='DroidSans' text-anchor='middle' font-size='%5' >%6</text>\n");

	if (!sip) {
		header +=	"<circle fill='#000000' cx='%10' cy='200' r='150' stroke-width='0' />\n"
					"<text x='%10' fill='#FFFFFF' y='305' font-family='DroidSans' text-anchor='middle' font-weight='bold' stroke-width='0' font-size='275' >?</text>\n";
	}
	else {
		labelText = "IC";
		fontSize = 235;
		textOffset = 0;
	}

	QString svg = header
		.arg(totalHeight / GraphicsUtils::StandardFritzingDPI)
		.arg(totalHeight)
		.arg(totalHeight - border)
		.arg((totalHeight / 2) + textOffset)
		.arg(fontSize)
		.arg(labelText)
		.arg(totalWidth / 1000.0)
		.arg(totalWidth)
		.arg(innerWidth)
		.arg(totalWidth - 200)
		.arg(increment + textMax + ((totalWidth - increment - textMax) / 2.0))
		;


	QString repeat("<line fill='none' stroke='#000000' stroke-linejoin='round' stroke-linecap='round' stroke-width='30' x1='15' y1='%1' x2='300' y2='%1'  />\n"
					"<rect x='0' y='%2' fill='none' width='300' height='30' id='connector%3pin' stroke-width='0' />\n"
					"<rect x='0' y='%2' fill='none' width='30' height='30' id='connector%3terminal' stroke-width='0' />\n"
					"<text id='label%3' x='390' y='%4' font-family='DroidSans' stroke='none' fill='#000000' text-anchor='start' font-size='%6' >%5</text>\n");
  
	for (int i = 0; i < labels.count(); i++) {
		svg += repeat
			.arg(increment + (border / 2) + (i * increment))
			.arg(increment + (i * increment))
			.arg(i)
			.arg(increment + repeatTextOffset + (i * increment))
			.arg(labels.at(i))
			.arg(labelFontSize);
	}

	svg += "</g>\n";
	svg += "</svg>\n";
	return svg;
}

QString MysteryPart::makeBreadboardSvg(const QString & expectedFileName) 
{
	if (expectedFileName.contains("_sip_")) return makeBreadboardSipSvg(expectedFileName);
	if (expectedFileName.contains("_dip_")) return makeBreadboardDipSvg(expectedFileName);

	return "";
}

QString MysteryPart::makeBreadboardDipSvg(const QString & expectedFileName) 
{
	QStringList pieces = expectedFileName.split("_");
	if (pieces.count() != 6) return "";

	int pins = pieces.at(2).toInt();
	double spacing = TextUtils::convertToInches(pieces.at(4)) * 100;


	int increment = 10;

	QString repeatT("<rect id='connector%1terminal' x='[1.87]' y='1' fill='#8C8C8C' stroke='none' stroke-width='0' width='2.3' height='0'/>\n"
					"<rect id='connector%1pin' x='[1.87]' y='0' fill='#8C8C8C' stroke='none' stroke-width='0' width='2.3' height='3.5'/>\n");

	QString repeatB("<rect id='connector%1terminal' x='{1.87}' y='[11.0]' fill='#8C8C8C' stroke='none' stroke-width='0' width='2.3' height='0'/>\n"
					"<rect id='connector%1pin' x='{1.87}' y='[7.75]' fill='#8C8C8C' stroke='none' stroke-width='0' width='2.3' height='4.25'/>\n");

	QString header("<?xml version='1.0' encoding='utf-8'?>\n"
					"<svg version='1.2' baseProfile='tiny' xmlns='http://www.w3.org/2000/svg' \n"
					"width='.percent.1in' height='%1in' viewBox='0 0 {16.0022} [12.0]'>\n"
					"<g id='breadboard'>\n"
					".percent.2\n"
					"<rect width='{16.0022}' x='0' y='2.5' height='[6.5]' fill='#000000' id='upper' stroke-width='0' />\n"
					"<rect width='{16.0022}' x='0' y='[6.5]' fill='#404040' height='3.096' id='lower' stroke-width='0' />\n"
					"<text id='label' x='2.5894' y='{{6.0}}' fill='#e6e6e6' stroke='none' font-family='DroidSans' text-anchor='start' font-size='7.3' >?</text>\n"
					"<circle fill='#8C8C8C' cx='11.0022' cy='{{7.5}}' r='3' stroke-width='0' />\n"
					"<text x='11.0022' y='{{9.2}}' font-family='DroidSans' text-anchor='middle' font-weight='bold' stroke-width='0' font-size='5.5' >?</text>\n"
					".percent.3\n"
					"</g>\n"
					"</svg>\n");


	header = TextUtils::incrementTemplateString(header, 1, spacing - increment, TextUtils::incMultiplyPinFunction, TextUtils::noCopyPinFunction, NULL);
	header = header.arg(TextUtils::getViewBoxCoord(header, 3) / 100.0);
	if (spacing == 10) {
		header.replace("{{6.0}}", "8.0");
		header.replace("{{7.5}}", "5.5");
		header.replace("{{9.2}}", "7.2");
	}
	else {
		header.replace("{{6.0}}", QString::number(6.0 + ((spacing - increment) / 2)));
		header.replace("{{7.5}}", "7.5");
		header.replace("{{9.2}}", "9.2");
	}
	header.replace(".percent.", "%");
	header.replace("{", "[");
	header.replace("}", "]");

	QString svg = TextUtils::incrementTemplateString(header, 1, increment * ((pins - 4) / 2), TextUtils::incMultiplyPinFunction, TextUtils::noCopyPinFunction, NULL);

	repeatB = TextUtils::incrementTemplateString(repeatB, 1, spacing - increment, TextUtils::incMultiplyPinFunction, TextUtils::noCopyPinFunction, NULL);
	repeatB.replace("{", "[");
	repeatB.replace("}", "]");

	int userData[2];
	userData[0] = pins;
	userData[1] = 1;
	QString repeatTs = TextUtils::incrementTemplateString(repeatT, pins / 2, increment, TextUtils::standardMultiplyPinFunction, TextUtils::negIncCopyPinFunction, userData);
	QString repeatBs = TextUtils::incrementTemplateString(repeatB, pins / 2, increment, TextUtils::standardMultiplyPinFunction, TextUtils::standardCopyPinFunction, NULL);

	return svg.arg(TextUtils::getViewBoxCoord(svg, 2) / 100).arg(repeatTs).arg(repeatBs);
}

QString MysteryPart::makeBreadboardSipSvg(const QString & expectedFileName) 
{
	QStringList pieces = expectedFileName.split("_");
	if (pieces.count() != 6) return "";

	int pins = pieces.at(2).toInt();
	int increment = 10;

	QString header("<?xml version='1.0' encoding='utf-8'?>\n"
					"<svg version='1.2' baseProfile='tiny' id='svg2' xmlns='http://www.w3.org/2000/svg' \n"
					"width='%1in' height='0.27586in' viewBox='0 0 [6.0022] 27.586'>\n"
					"<g id='breadboard'>\n"
					"<rect width='[6.0022]' x='0' y='0' height='24.17675' fill='#000000' id='upper' stroke-width='0' />\n"
					"<rect width='[6.0022]' x='0' y='22' fill='#404040' height='3.096' id='lower' stroke-width='0' />\n"
					"<text id='label' x='2.5894' y='13' fill='#e6e6e6' stroke='none' font-family='DroidSans' text-anchor='start' font-size='7.3' >?</text>\n"
					"<circle fill='#8C8C8C' cx='[1.0022]' cy='5' r='3' stroke-width='0' />\n"
					"<text x='[1.0022]' y='6.7' font-family='DroidSans' text-anchor='middle' font-weight='bold' stroke-width='0' font-size='5.5' >?</text>\n"      
					"%2\n"
					"</g>\n"
					"</svg>\n");

	QString svg = TextUtils::incrementTemplateString(header, 1, increment * (pins - 1), TextUtils::incMultiplyPinFunction, TextUtils::noCopyPinFunction, NULL);

	QString repeat("<rect id='connector%1terminal' stroke='none' stroke-width='0' x='[1.87]' y='25.586' fill='#8C8C8C' width='2.3' height='2.0'/>\n"
					"<rect id='connector%1pin' stroke='none' stroke-width='0' x='[1.87]' y='23.336' fill='#8C8C8C' width='2.3' height='4.25'/>\n");

	QString repeats = TextUtils::incrementTemplateString(repeat, pins, increment, TextUtils::standardMultiplyPinFunction, TextUtils::standardCopyPinFunction, NULL);

	return svg.arg(TextUtils::getViewBoxCoord(svg, 2) / 100).arg(repeats);
}

bool MysteryPart::changePinLabels(bool singleRow, bool sip) {
	Q_UNUSED(singleRow);

	if (m_viewIdentifier != ViewIdentifierClass::SchematicView) return true;

	bool hasLocal = false;
	QStringList labels = getPinLabels(hasLocal);
	if (labels.count() == 0) return true;

	QString svg = MysteryPart::makeSchematicSvg(labels, sip);
	loadExtraRenderer(svg, false);

	return true;
}