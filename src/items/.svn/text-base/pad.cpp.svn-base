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

// TODO:
//		flip for one-sided board

#include "pad.h"

#include "../utils/graphicsutils.h"
#include "../utils/folderutils.h"
#include "../utils/textutils.h"
#include "../fsvgrenderer.h"
#include "../sketch/infographicsview.h"
#include "../svg/svgfilesplitter.h"
#include "moduleidnames.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QRegExp>
#include <QPushButton>
#include <QImageReader>
#include <QMessageBox>
#include <QImage>
#include <QLineEdit>
#include <QDateTimeEdit>
#include <QSpinBox>
#include <QHash>
#include <QTextCursor>
#include <QTextBlock>
#include <QTextLayout>
#include <QTextLine>

static QString PadTemplate = "";
static double OriginalWidth = 28;
static double OriginalHeight = 32;

Pad::Pad( ModelPart * modelPart, ViewIdentifierClass::ViewIdentifier viewIdentifier, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu, bool doLabel)
	: ResizableBoard(modelPart, viewIdentifier, viewGeometry, id, itemMenu, doLabel)
{
	m_decimalsAfter = 2;
}

Pad::~Pad() {

}

QString Pad::makeLayerSvg(ViewLayer::ViewLayerID viewLayerID, double mmW, double mmH, double milsW, double milsH) 
{
	Q_UNUSED(milsW);
	Q_UNUSED(milsH);

	switch (viewLayerID) {
		case ViewLayer::Copper0:
		case ViewLayer::Copper1:
			break;
		default:
			return "";
	}

	double wpx = mmW > 0 ? GraphicsUtils::mm2pixels(mmW) : OriginalWidth;
	double hpx = mmH > 0 ? GraphicsUtils::mm2pixels(mmH) : OriginalHeight;

	QString connectAt = m_modelPart->prop("connect").toString();
	QRectF terminal;
	double minW = qMin(1.0, wpx / 3);
	double minH = qMin(1.0, hpx / 3);
	if (connectAt.compare("center", Qt::CaseInsensitive) == 0) {
		terminal.setRect(2, 2, wpx, hpx);
	}
	else if (connectAt.compare("north", Qt::CaseInsensitive) == 0) {
		terminal.setRect(2, 2, wpx, minH);
	}
	else if (connectAt.compare("south", Qt::CaseInsensitive) == 0) {
		terminal.setRect(2, 2 + hpx - minH, wpx, minH);
	}
	else if (connectAt.compare("east", Qt::CaseInsensitive) == 0) {
		terminal.setRect(2 + wpx - minW, 2, minW, hpx);
	}
	else if (connectAt.compare("west", Qt::CaseInsensitive) == 0) {
		terminal.setRect(2, 2, minW, hpx);
	}

	QString svg = QString("<svg version='1.1' xmlns='http://www.w3.org/2000/svg'  x='0px' y='0px' width='%1px' height='%2px' viewBox='0 0 %1 %2'>\n"
							"<g id='%5'>\n"
							"<rect  id='connector0pad' x='2' y='2' fill='#FFBF00' stroke='none' stroke-width='0' width='%3' height='%4'/>\n"
							"<rect  id='connector0terminal' x='%6' y='%7' fill='none' stroke='none' stroke-width='0' width='%8' height='%9'/>\n"
							"</g>\n"
							"</svg>"
							)
					.arg(wpx + 4)
					.arg(hpx + 4)
					.arg(wpx)
					.arg(hpx)
					.arg(ViewLayer::viewLayerXmlNameFromID(viewLayerID))
					.arg(terminal.left())
					.arg(terminal.top())
					.arg(terminal.width())
					.arg(terminal.height())
					;

	//DebugDialog::debug("pad svg: " + svg);
	return svg;
}

QString Pad::makeNextLayerSvg(ViewLayer::ViewLayerID viewLayerID, double mmW, double mmH, double milsW, double milsH) 
{
	Q_UNUSED(mmW);
	Q_UNUSED(mmH);
	Q_UNUSED(milsW);
	Q_UNUSED(milsH);
	Q_UNUSED(viewLayerID);

	return "";
}

QString Pad::makeFirstLayerSvg(double mmW, double mmH, double milsW, double milsH) {
	return makeLayerSvg(ViewLayer::Copper1, mmW, mmH, milsW, milsH);
}

ViewIdentifierClass::ViewIdentifier Pad::theViewIdentifier() {
	return ViewIdentifierClass::PCBView;
}

double Pad::minWidth() {
	return 0.2;
}

double Pad::minHeight() {
	return 0.2;
}

QString Pad::retrieveSvg(ViewLayer::ViewLayerID viewLayerID, QHash<QString, QString> & svgHash, bool blackOnly, double dpi)
{
	return ResizableBoard::retrieveSvg(viewLayerID, svgHash, blackOnly, dpi);
}

bool Pad::collectExtraInfo(QWidget * parent, const QString & family, const QString & prop, const QString & value, bool swappingEnabled, QString & returnProp, QString & returnValue, QWidget * & returnWidget) 
{
	if (prop.compare("shape", Qt::CaseInsensitive) == 0) {
		returnWidget = setUpDimEntry(false, returnWidget);
		returnProp = tr("shape");
		return true;
	}

	if (prop.compare("connect to", Qt::CaseInsensitive) == 0) {
		QComboBox * comboBox = new QComboBox();
		comboBox->setObjectName("infoViewComboBox");
		comboBox->setEditable(false);
		comboBox->setEnabled(swappingEnabled);
		comboBox->addItem(tr("center"), "center");
		comboBox->addItem(tr("north"), "north");
		comboBox->addItem(tr("east"), "east");
		comboBox->addItem(tr("south"), "south");
		comboBox->addItem(tr("west"), "west");
		QString connectAt = m_modelPart->prop("connect").toString();
		for (int i = 0; i < comboBox->count(); i++) {
			if (comboBox->itemData(i).toString().compare(connectAt) == 0) {
				comboBox->setCurrentIndex(i);
				break;
			}
		}

		connect(comboBox, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(terminalPointEntry(const QString &)));

		returnWidget = comboBox;
		returnProp = tr("connect to");
		return true;
	}

	return PaletteItem::collectExtraInfo(parent, family, prop, value, swappingEnabled, returnProp, returnValue, returnWidget);
}

void Pad::setProp(const QString & prop, const QString & value) 
{	
	if (prop.compare("connect to", Qt::CaseInsensitive) == 0) {
		modelPart()->setProp("connect", value);
		resizeMMAux(m_modelPart->prop("width").toDouble(), m_modelPart->prop("height").toDouble());
		return;
	}

	ResizableBoard::setProp(prop, value);
}

bool Pad::canEditPart() {
	return false;
}

bool Pad::hasPartLabel() {
	return true;
}

bool Pad::stickyEnabled() {
	return true;
}

ItemBase::PluralType Pad::isPlural() {
	return Plural;
}

bool Pad::rotationAllowed() {
	return true;
}

bool Pad::rotation45Allowed() {
	return true;
}

bool Pad::freeRotationAllowed(Qt::KeyboardModifiers) {
	return true;
}

bool Pad::hasPartNumberProperty()
{
	return false;
}

void Pad::setInitialSize() {
	double w = m_modelPart->prop("width").toDouble();
	if (w == 0) {
		// set the size so the infoGraphicsView will display the size as you drag
		modelPart()->setProp("width", GraphicsUtils::pixels2mm(OriginalWidth, FSvgRenderer::printerScale())); 
		modelPart()->setProp("height", GraphicsUtils::pixels2mm(OriginalHeight, FSvgRenderer::printerScale())); 
		modelPart()->setProp("connect", "center"); 
	}
}

void Pad::paintHover(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	PaletteItem::paintHover(painter, option, widget);
}

void Pad::mousePressEvent(QGraphicsSceneMouseEvent * event) 
{
	PaletteItem::mousePressEvent(event);
}

void Pad::mouseReleaseEvent(QGraphicsSceneMouseEvent * event) 
{
	PaletteItem::mouseReleaseEvent(event);
}

void Pad::mouseMoveEvent(QGraphicsSceneMouseEvent * event) 
{
	PaletteItem::mouseMoveEvent(event);
}

void Pad::paintSelected(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	PaletteItem::paintSelected(painter, option, widget);
}

void Pad::hoverEnterEvent( QGraphicsSceneHoverEvent * event ) {
	PaletteItem::hoverEnterEvent(event);
}

void Pad::hoverMoveEvent( QGraphicsSceneHoverEvent * event ) {
	PaletteItem::hoverMoveEvent(event);
}

void Pad::hoverLeaveEvent( QGraphicsSceneHoverEvent * event ) {
	PaletteItem::hoverLeaveEvent(event);
}

void Pad::resizeMMAux(double mmW, double mmH) {
	ResizableBoard::resizeMMAux(mmW, mmH);
	resetConnectors(NULL, NULL);
}

void Pad::addedToScene(bool temporary)
{
	if (this->scene()) {
		setInitialSize();
		resizeMMAux(m_modelPart->prop("width").toDouble(), m_modelPart->prop("height").toDouble());
	}

    return PaletteItem::addedToScene(temporary);
}

void Pad::terminalPointEntry(const QString &) {
	QComboBox * comboBox = qobject_cast<QComboBox *>(sender());
	if (comboBox == NULL) return;

	QString value = comboBox->itemData(comboBox->currentIndex()).toString();
	QString connectAt = m_modelPart->prop("connect").toString();
	if (connectAt.compare(value) == 0) return;

	InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView != NULL) {
		infoGraphicsView->setProp(this, "connect to", tr("connect to"), connectAt, value, true);
	}
}

ResizableBoard::Corner Pad::findCorner(QPointF, Qt::KeyboardModifiers) {
	return ResizableBoard::NO_CORNER;
}
