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

$Revision: 5950 $:
$Author: cohen@irascible.com $:
$Date: 2012-04-07 10:26:22 -0700 (Sat, 07 Apr 2012) $

********************************************************************/

#include "logoitem.h"
#include "../utils/graphicsutils.h"
#include "../utils/folderutils.h"
#include "../utils/textutils.h"
#include "../fsvgrenderer.h"
#include "../sketch/infographicsview.h"
#include "../svg/svgfilesplitter.h"
#include "moduleidnames.h"
#include "../svg/groundplanegenerator.h"
#include "../utils/cursormaster.h"

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
#include <QApplication>

static QStringList ImageNames;
static QStringList NewImageNames;
static QStringList Copper0ImageNames;
static QStringList NewCopper0ImageNames;
static QStringList Copper1ImageNames;
static QStringList NewCopper1ImageNames;

LogoItem::LogoItem( ModelPart * modelPart, ViewIdentifierClass::ViewIdentifier viewIdentifier, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu, bool doLabel)
	: ResizableBoard(modelPart, viewIdentifier, viewGeometry, id, itemMenu, doLabel)
{
	if (ImageNames.count() == 0) {
		ImageNames << "Made with Fritzing" << "Fritzing icon" << "OHANDA logo" << "OSHW logo";
	}

	m_inLogoEntry = QTime::currentTime().addSecs(-10);
	m_fileNameComboBox = NULL;
	m_aspectRatioCheck = NULL;
	m_keepAspectRatio = true;
	m_hasLogo = (modelPart->moduleID() == ModuleIDNames::LogoTextModuleIDName);
	m_logo = modelPart->prop("logo").toString();
	if (m_hasLogo && m_logo.isEmpty()) {
		m_logo = modelPart->properties().value("logo", "logo");
		modelPart->setProp("logo", m_logo);
	}
}

LogoItem::~LogoItem() {
}


void LogoItem::addedToScene(bool temporary)
{
	if (this->scene()) {
		setInitialSize();
		m_aspectRatio.setWidth(this->boundingRect().width());
		m_aspectRatio.setHeight(this->boundingRect().height());
		m_originalFilename = filename();
		QString shape = prop("shape");
		if (!shape.isEmpty()) {					
			m_aspectRatio = modelPart()->prop("aspectratio").toSizeF();
			if (loadExtraRenderer(shape.toUtf8(), false)) {
			}
			else {
				DebugDialog::debug("bad shape in " + m_originalFilename + " " + shape);
			}
		}
		else {
			QFile f(m_originalFilename);
			if (f.open(QFile::ReadOnly)) {
				QString svg = f.readAll();
				f.close();
				modelPart()->setProp("shape", svg);
				modelPart()->setProp("lastfilename", m_originalFilename);
				initImage();
			}
		}
	}

    return ResizableBoard::addedToScene(temporary);
}


QString LogoItem::retrieveSvg(ViewLayer::ViewLayerID viewLayerID, QHash<QString, QString> & svgHash, bool blackOnly, double dpi)
{
	if (viewLayerID == layer() ) {
		QString svg = prop("shape");
		if (!svg.isEmpty()) {
			QString xmlName = ViewLayer::viewLayerXmlNameFromID(viewLayerID);
			SvgFileSplitter splitter;
			bool result = splitter.splitString(svg, xmlName);
			if (!result) {
				return "";
			}

			double scaleX = 1;
			double scaleY = 1;
			if (m_hasLogo) {
				QDomDocument doc = splitter.domDocument();
				QDomElement root = doc.documentElement();
				QDomElement g = root.firstChildElement("g");
				QDomElement text = g.firstChildElement("text");

				// TODO: this is really a hack and resizing should change a scale factor rather than the <text> coordinates
				// but it's not clear how to deal with existing sketches
	
				QString viewBox = root.attribute("viewBox");
				double w = TextUtils::convertToInches(root.attribute("width"));
				double h = TextUtils::convertToInches(root.attribute("height"));
				QStringList coords = viewBox.split(" ", QString::SkipEmptyParts);
				double sx = w / coords.at(2).toDouble();
				double sy = h / coords.at(3).toDouble();
				if (qAbs(sx - sy) > .001) {
					// change vertical dpi to match horizontal dpi
					// y coordinate is really intended in relation to font size so leave it be
					scaleY = sy / sx;
					root.setAttribute("viewBox", QString("0 0 %1 %2").arg(coords.at(2)).arg(h / sx));
				}
			}

			result = splitter.normalize(dpi, xmlName, blackOnly);
			if (!result) {
				return "";
			}

			QString string = splitter.elementString(xmlName);

			if (scaleY == 1) return string;

			QTransform t;
			t.scale(scaleX, scaleY);
			return TextUtils::svgTransform(string, t, false, "");
		}
	}

	return PaletteItemBase::retrieveSvg(viewLayerID, svgHash, blackOnly, dpi);
}

bool LogoItem::collectExtraInfo(QWidget * parent, const QString & family, const QString & prop, const QString & value, bool swappingEnabled, QString & returnProp, QString & returnValue, QWidget * & returnWidget) 
{
	if (m_hasLogo) {
		if (prop.compare("logo", Qt::CaseInsensitive) == 0) {
			returnProp = tr("logo");
			returnValue = m_logo;

			QLineEdit * edit = new QLineEdit(parent);
			edit->setObjectName("infoViewLineEdit");

			edit->setText(m_logo);
			edit->setEnabled(swappingEnabled);
			connect(edit, SIGNAL(editingFinished()), this, SLOT(logoEntry()));

			returnWidget = edit;
			return true;
		}
	}
	else {
		if (prop.compare("filename", Qt::CaseInsensitive) == 0) {
			returnProp = tr("image file");

			QFrame * frame = new QFrame();
			frame->setObjectName("infoViewPartFrame");
			QVBoxLayout * vboxLayout = new QVBoxLayout();
			vboxLayout->setContentsMargins(0, 0, 0, 0);
			vboxLayout->setSpacing(0);
			vboxLayout->setMargin(0);

			QComboBox * comboBox = new QComboBox();
			comboBox->setObjectName("infoViewComboBox");
			comboBox->setEditable(false);
			comboBox->setEnabled(swappingEnabled);
			m_fileNameComboBox = comboBox;

			setFileNameItems();

			connect(comboBox, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(fileNameEntry(const QString &)));

			QPushButton * button = new QPushButton (tr("load image file"));
			button->setObjectName("infoViewButton");
			connect(button, SIGNAL(pressed()), this, SLOT(prepLoadImage()));
			button->setEnabled(swappingEnabled);

			vboxLayout->addWidget(comboBox);
			vboxLayout->addWidget(button);

			frame->setLayout(vboxLayout);
			returnWidget = frame;

			returnProp = "";
			return true;
		}
	}

	if (prop.compare("shape", Qt::CaseInsensitive) == 0) {
		returnWidget = setUpDimEntry(true, returnWidget);
		returnProp = tr("shape");
		return true;
	}

	return PaletteItem::collectExtraInfo(parent, family, prop, value, swappingEnabled, returnProp, returnValue, returnWidget);

}

void LogoItem::prepLoadImage() {
	QList<QByteArray> supportedImageFormats = QImageReader::supportedImageFormats();
	QString imagesStr = tr("Images");
	imagesStr += " (";
	foreach (QByteArray ba, supportedImageFormats) {
		imagesStr += "*." + QString(ba) + " ";
	}
	if (!imagesStr.contains("svg")) {
		imagesStr += "*.svg";
	}
	imagesStr += ")";
	QString fileName = FolderUtils::getOpenFileName(
		NULL,
		tr("Select an image file to load"),
		"",
		imagesStr
	);

	if (fileName.isEmpty()) return;

	prepLoadImageAux(fileName, true);
}

void LogoItem::prepLoadImageAux(const QString & fileName, bool addName)
{
	InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView != NULL) {
		infoGraphicsView->loadLogoImage(this->id(), prop("shape"), m_aspectRatio, prop("lastfilename"), fileName, addName);
	}
}

void LogoItem::reloadImage(const QString & svg, const QSizeF & aspectRatio, const QString & fileName, bool addName) 
{
	bool result = loadExtraRenderer(svg.toUtf8(), false);
	if (result) {
		if (aspectRatio == QSizeF(0, 0)) {
			QRectF r = m_extraRenderer->viewBoxF();
			m_aspectRatio.setWidth(r.width());
			m_aspectRatio.setHeight(r.height());
		}
		else {
			m_aspectRatio = aspectRatio;
		}
		modelPart()->setProp("aspectratio", m_aspectRatio);
		modelPart()->setProp("shape", svg);
		modelPart()->setProp("logo", "");
		modelPart()->setProp("lastfilename", fileName);
		if (addName) {
			if (!getNewImageNames().contains(fileName, Qt::CaseInsensitive)) {
				getNewImageNames().append(fileName);
				bool wasBlocked = m_fileNameComboBox->blockSignals(true);
				while (m_fileNameComboBox->count() > 0) {
					m_fileNameComboBox->removeItem(0);
				}
				setFileNameItems();
				m_fileNameComboBox->blockSignals(wasBlocked);
			}
		}
		m_logo = "";

		LayerHash layerHash;
		resizeMM(GraphicsUtils::pixels2mm(m_aspectRatio.width(), FSvgRenderer::printerScale()),
				 GraphicsUtils::pixels2mm(m_aspectRatio.height(), FSvgRenderer::printerScale()),
				 layerHash);
	}
	else {
		// restore previous (not sure whether this is necessary)
		loadExtraRenderer(prop("shape").toUtf8(), false);
		unableToLoad(fileName);
	}
}

void LogoItem::loadImage(const QString & fileName, bool addName)
{
	QString svg;
	if (fileName.endsWith(".svg")) {
		QFile f(fileName);
		if (f.open(QFile::ReadOnly)) {
			svg = f.readAll();
		}
		if (svg.isEmpty()) {
			unableToLoad(fileName);
			return;
		}

		TextUtils::cleanSodipodi(svg);
		TextUtils::fixPixelDimensionsIn(svg);
		TextUtils::fixViewboxOrigin(svg);
		TextUtils::tspanRemove(svg);

		QString errorStr;
		int errorLine;
		int errorColumn;

		QDomDocument domDocument;

		if (!domDocument.setContent(svg, true, &errorStr, &errorLine, &errorColumn)) {
			unableToLoad(fileName);
			return;
		}

		QDomElement root = domDocument.documentElement();
		if (root.isNull()) {
			unableToLoad(fileName);
			return;
		}

		if (root.tagName() != "svg") {
			unableToLoad(fileName);
			return;
		}

		QStringList exceptions;
		exceptions << "none" << "";
		QString toColor(colorString());
		SvgFileSplitter::changeColors(root, toColor, exceptions);

		bool isIllustrator = TextUtils::isIllustratorDoc(domDocument);

		QString viewBox = root.attribute("viewBox");
		if (viewBox.isEmpty()) {
			bool ok1, ok2;
			double w = TextUtils::convertToInches(root.attribute("width"), &ok1, isIllustrator) * FSvgRenderer::printerScale();
			double h = TextUtils::convertToInches(root.attribute("height"), &ok2, isIllustrator) * FSvgRenderer::printerScale();
			if (!ok1 || !ok2) {
				unableToLoad(fileName);
				return;
			}

			root.setAttribute("viewBox", QString("0 0 %1 %2").arg(w).arg(h));
		}

		QList<QDomNode> rootChildren;
		QDomNode rootChild = root.firstChild();
		while (!rootChild.isNull()) {
			rootChildren.append(rootChild);
			rootChild = rootChild.nextSibling();
		}

		QDomElement topG = domDocument.createElement("g");
		topG.setAttribute("id", layerName());
		root.appendChild(topG);
		foreach (QDomNode node, rootChildren) {
			topG.appendChild(node);
		}

		svg = TextUtils::removeXMLEntities(domDocument.toString());
	}
	else {
		QImage image(fileName);
		if (image.isNull()) {
			unableToLoad(fileName);
			return;
		}

		if (image.format() != QImage::Format_RGB32 && image.format() != QImage::Format_ARGB32) {
			image = image.convertToFormat(QImage::Format_Mono);
		}

		GroundPlaneGenerator gpg;
		gpg.setLayerName(layerName());
		gpg.setMinRunSize(1, 1);
		double res = image.dotsPerMeterX() / GraphicsUtils::InchesPerMeter;
		gpg.scanImage(image, image.width(), image.height(), 1, res, colorString(), false, false, QSizeF(0, 0), 0, QPointF(0, 0));
		QStringList newSvgs = gpg.newSVGs();
		if (newSvgs.count() < 1) {
			QMessageBox::information(
				NULL,
				tr("Unable to display"),
				tr("Unable to display image from %1").arg(fileName)
			);
			return;
		}

		QDomDocument doc;
		foreach (QString newSvg, newSvgs) {
			TextUtils::mergeSvg(doc, newSvg, layerName());
		}
		svg = TextUtils::mergeSvgFinish(doc);
	}

	reloadImage(svg, QSizeF(0, 0), fileName, addName);
}

void LogoItem::resizeMM(double mmW, double mmH, const LayerHash & viewLayers) {
	Q_UNUSED(viewLayers);

	if (mmW == 0 || mmH == 0) {
		return;
	}

	DebugDialog::debug(QString("resize mm %1 %2").arg(mmW).arg(mmH));

	QRectF r = this->boundingRect();
	if (qAbs(GraphicsUtils::pixels2mm(r.width(), FSvgRenderer::printerScale()) - mmW) < .001 &&
		qAbs(GraphicsUtils::pixels2mm(r.height(), FSvgRenderer::printerScale()) - mmH) < .001) 
	{
		return;
	}

	double inW = GraphicsUtils::mm2mils(mmW) / 1000;
	double inH = GraphicsUtils::mm2mils(mmH) / 1000;

	// TODO: deal with aspect ratio

	QString svg = prop("shape");
	if (svg.isEmpty()) return;

	QString errorStr;
	int errorLine;
	int errorColumn;

	QDomDocument domDocument;
	if (!domDocument.setContent(svg, &errorStr, &errorLine, &errorColumn)) {
		return;
	}

	QDomElement root = domDocument.documentElement();
	if (root.isNull()) {
		return;
	}

	if (root.tagName() != "svg") {
		return;
	}

	root.setAttribute("width", QString::number(inW) + "in");
	root.setAttribute("height", QString::number(inH) + "in");

	svg = TextUtils::removeXMLEntities(domDocument.toString());			

	bool result = loadExtraRenderer(svg.toUtf8(), false);
	if (result) {
		modelPart()->setProp("shape", svg);
		modelPart()->setProp("width", mmW);
		modelPart()->setProp("height", mmH);
	}

	setWidthAndHeight(qRound(mmW * 10) / 10.0, qRound(mmH * 10) / 10.0);
}

void LogoItem::setProp(const QString & prop, const QString & value) {
	if (prop.compare("logo", Qt::CaseInsensitive) == 0) {
		setLogo(value, false);
		return;
	}

	ResizableBoard::setProp(prop, value);
}

void LogoItem::setLogo(QString logo, bool force) {
	if (!force && m_logo.compare(logo) == 0) return;

	switch (this->m_viewIdentifier) {
		case ViewIdentifierClass::PCBView:
			break;
		default:
			return;
	}

	QString svg;
	QFile f(m_originalFilename);
	if (f.open(QFile::ReadOnly)) {
		svg = f.readAll();
	}

	if (svg.isEmpty()) return;

	QSizeF oldSize = m_size;
	QXmlStreamReader streamReader(svg);
	QSizeF oldSvgSize = m_extraRenderer ? m_extraRenderer->viewBoxF().size() : QSizeF(0, 0);
	
	DebugDialog::debug(QString("size %1 %2, %3 %4").arg(m_size.width()).arg(m_size.height()).arg(oldSvgSize.width()).arg(oldSvgSize.height()));

	svg = hackSvg(svg, logo);
	QXmlStreamReader newStreamReader(svg);

	bool ok = rerender(svg);

	m_logo = logo;
	modelPart()->setProp("logo", logo);
	modelPart()->setProp("shape", svg);
	if (ok && !force) {
		QSizeF newSvgSize = m_extraRenderer->viewBoxF().size();
		QSizeF newSize = newSvgSize * oldSize.height() / oldSvgSize.height();
		DebugDialog::debug(QString("size %1 %2, %3 %4").arg(m_size.width()).arg(m_size.height()).arg(newSize.width()).arg(newSize.height()));
		
		// set the new text to approximately the same height as the original
		// if the text is non-proportional that will be lost
		LayerHash layerHash;
		resizeMM(GraphicsUtils::pixels2mm(newSize.width(), FSvgRenderer::printerScale()),
				 GraphicsUtils::pixels2mm(newSize.height(), FSvgRenderer::printerScale()),
				 layerHash);
		//DebugDialog::debug(QString("size %1 %2").arg(m_size.width()).arg(m_size.height()));
	}
}

bool LogoItem::rerender(const QString & svg)
{
	bool result = loadExtraRenderer(svg.toUtf8(), false);
	if (result) {
		QRectF r = m_extraRenderer->viewBoxF();
		m_aspectRatio.setWidth(r.width());
		m_aspectRatio.setHeight(r.height());
	}
	return result;
}

QString LogoItem::getProperty(const QString & key) {
	if (key.compare("logo", Qt::CaseInsensitive) == 0) {
		return m_logo;
	}

	return PaletteItem::getProperty(key);
}

const QString & LogoItem::logo() {
	return m_logo;
}

bool LogoItem::canEditPart() {
	return false;
}

bool LogoItem::hasPartLabel() {
	return false;
}

void LogoItem::logoEntry() {
	QLineEdit * edit = qobject_cast<QLineEdit *>(sender());
	if (edit == NULL) return;

	if (edit->text().compare(this->logo()) == 0) return;

	// m_inLogoEntry is a hack because a focus out event was being triggered on m_widthEntry when a user hit the return key in logoentry
	// this triggers a call to widthEntry() which causes all kinds of havoc.  (bug in version 0.7.1 and possibly earlier)
	m_inLogoEntry = QTime::currentTime().addSecs(1);

	InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView != NULL) {
		infoGraphicsView->setProp(this, "logo", tr("logo"), this->logo(), edit->text(), true);
	}
}

void LogoItem::initImage() {
	if (m_hasLogo) {
		setLogo(m_logo, true);
		return;
	}

	loadImage(m_originalFilename, false);
}

QString LogoItem::hackSvg(const QString & svg, const QString & logo) 
{
	QString errorStr;
	int errorLine;
	int errorColumn;
	QDomDocument doc;
	if (!doc.setContent(svg, &errorStr, &errorLine, &errorColumn)) return svg;

	QDomElement root = doc.documentElement();
	root.setAttribute("width", QString::number(logo.length() * 0.1) + "in");
	
	QString viewBox = root.attribute("viewBox");
	QStringList coords = viewBox.split(" ", QString::SkipEmptyParts);
	coords[2] = QString::number(logo.length() * 10);
	root.setAttribute("viewBox", coords.join(" "));

	QDomNodeList domNodeList = root.elementsByTagName("text");
	for (int i = 0; i < domNodeList.count(); i++) {
		QDomElement node = domNodeList.item(i).toElement();
		if (node.isNull()) continue;

		if (node.attribute("id").compare("label") != 0) continue;

		node.setAttribute("x", QString::number(logo.length() * 5));

		QDomNodeList childList = node.childNodes();
		for (int j = 0; j < childList.count(); j++) {
			QDomNode child = childList.item(i);
			if (child.isText()) {
				child.setNodeValue(logo);

				modelPart()->setProp("width", logo.length() * 0.1 * 25.4);
				QString h = root.attribute("height");
				bool ok;
				modelPart()->setProp("height", TextUtils::convertToInches(h, &ok, false) * 25.4);
				return doc.toString();
			}
		}
	}

	return svg;
}

void LogoItem::widthEntry() {
	if (QTime::currentTime() < m_inLogoEntry) return;

	QLineEdit * edit = qobject_cast<QLineEdit *>(sender());
	if (edit == NULL) return;

	double w = edit->text().toDouble();
	double oldW = m_modelPart->prop("width").toDouble();
	if (w == oldW) return;

	double h = m_modelPart->prop("height").toDouble();
	if (m_keepAspectRatio) {
		h = w * m_aspectRatio.height() / m_aspectRatio.width();
	}

	InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView != NULL) {
		infoGraphicsView->resizeBoard(w, h, true);
	}
}

void LogoItem::heightEntry() {
	if (QTime::currentTime() < m_inLogoEntry) return;

	QLineEdit * edit = qobject_cast<QLineEdit *>(sender());
	if (edit == NULL) return;

	double h = edit->text().toDouble();
	double oldH =  m_modelPart->prop("height").toDouble();
	if (h == oldH) return;

	setHeight(h);
}

void LogoItem::setHeight(double h)
{
	double w = m_modelPart->prop("width").toDouble();
	if (m_keepAspectRatio) {
		w = h * m_aspectRatio.width() / m_aspectRatio.height();
	}

	InfoGraphicsView * infoGraphicsView = InfoGraphicsView::getInfoGraphicsView(this);
	if (infoGraphicsView != NULL) {
		infoGraphicsView->resizeBoard(w, h, true);
	}
}

void LogoItem::unableToLoad(const QString & fileName) {
	QMessageBox::information(
		NULL,
		tr("Unable to load"),
		tr("Unable to load image from %1").arg(fileName)
	);
}

void LogoItem::keepAspectRatio(bool checkState) {
	m_keepAspectRatio = checkState;
}

void LogoItem::fileNameEntry(const QString & filename) {
	foreach (QString name, getImageNames()) {
		if (filename.compare(name) == 0) {
			QString f = FolderUtils::getApplicationSubFolderPath("parts") + "/svg/core/pcb/" + filename + ".svg";
			return prepLoadImageAux(f, false);
			break;
		}
	}

	prepLoadImageAux(filename, true);
}

void LogoItem::setFileNameItems() {
	if (m_fileNameComboBox == NULL) return;

	m_fileNameComboBox->addItems(getImageNames());
	m_fileNameComboBox->addItems(getNewImageNames());

	int ix = 0;
	foreach (QString name, getImageNames()) {
		if (prop("lastfilename").contains(name)) {
			m_fileNameComboBox->setCurrentIndex(ix);
			return;
		}
		ix++;
	}

	foreach (QString name, getNewImageNames()) {
		if (prop("lastfilename").contains(name)) {
			m_fileNameComboBox->setCurrentIndex(ix);
			return;
		}
		ix++;
	}
}

bool LogoItem::stickyEnabled() {
	return true;
}

ItemBase::PluralType LogoItem::isPlural() {
	return Plural;
}

ViewLayer::ViewLayerID LogoItem::layer() {
	return  ViewLayer::Silkscreen1;
}

QString LogoItem::colorString() {
	return ViewLayer::Silkscreen1Color;
}

QString LogoItem::layerName() 
{
	return ViewLayer::viewLayerXmlNameFromID(layer());
}

QStringList & LogoItem::getImageNames() {
	return ImageNames;
}

QStringList & LogoItem::getNewImageNames() {
	return NewImageNames;
}

double LogoItem::minWidth() {
	return ResizableBoard::CornerHandleSize * 2;
}

double LogoItem::minHeight() {
	return ResizableBoard::CornerHandleSize * 2;
}

bool LogoItem::freeRotationAllowed(Qt::KeyboardModifiers modifiers) {

	if ((modifiers & altOrMetaModifier()) == 0) return false;
	if (!isSelected()) return false;

	return true;
}

ResizableBoard::Corner LogoItem::findCorner(QPointF scenePos, Qt::KeyboardModifiers modifiers) {
	ResizableBoard::Corner corner = ResizableBoard::findCorner(scenePos, modifiers);
	if (corner == ResizableBoard::NO_CORNER) return corner;

	if (modifiers & altOrMetaModifier()) {
		// free rotate
		setCursor(*CursorMaster::RotateCursor);
		return ResizableBoard::NO_CORNER;
	}

	return corner;
}

void LogoItem::paintHover(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	PaletteItem::paintHover(painter, option, widget);
}

///////////////////////////////////////////////////////////////////////

CopperLogoItem::CopperLogoItem( ModelPart * modelPart, ViewIdentifierClass::ViewIdentifier viewIdentifier, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu, bool doLabel)
	: LogoItem(modelPart, viewIdentifier, viewGeometry, id, itemMenu, doLabel)
{
	if (Copper1ImageNames.count() == 0) {
		Copper1ImageNames << "Fritzing icon copper1";
	}

	if (Copper0ImageNames.count() == 0) {
		Copper0ImageNames << "Fritzing icon copper0";
	}

	m_hasLogo = (modelPart->moduleID().endsWith(ModuleIDNames::LogoTextModuleIDName));
	m_logo = modelPart->prop("logo").toString();
	if (m_hasLogo && m_logo.isEmpty()) {
		m_logo = modelPart->properties().value("logo", "logo");
		modelPart->setProp("logo", m_logo);
	}
}

CopperLogoItem::~CopperLogoItem() {
}

ViewLayer::ViewLayerID CopperLogoItem::layer() {
	return isCopper0() ? ViewLayer::Copper0 :  ViewLayer::Copper1;
}

QString CopperLogoItem::colorString() {
	return isCopper0() ? ViewLayer::Copper0Color :  ViewLayer::Copper1Color;
}

QStringList & CopperLogoItem::getImageNames() {
	return isCopper0() ? Copper0ImageNames :  Copper1ImageNames;
}

QStringList & CopperLogoItem::getNewImageNames() {
	return isCopper0() ? NewCopper0ImageNames :  NewCopper1ImageNames;
}

QString CopperLogoItem::hackSvg(const QString & svg, const QString & logo) {
	QString newSvg = LogoItem::hackSvg(svg, logo);
	if (!isCopper0()) return newSvg;

	return flipSvg(newSvg);
}

QString CopperLogoItem::flipSvg(const QString & svg)
{
	QString newSvg = svg;
	newSvg.replace("copper1", "copper0");
	newSvg.replace(ViewLayer::Copper1Color, ViewLayer::Copper0Color, Qt::CaseInsensitive);
	QMatrix m;
	QSvgRenderer renderer(newSvg.toUtf8());
	QRectF bounds = renderer.viewBoxF();
	m.translate(bounds.center().x(), bounds.center().y());
	QMatrix mMinus = m.inverted();
    QMatrix cm = mMinus * QMatrix().scale(-1, 1) * m;
	int gix = newSvg.indexOf("<g");
	newSvg.replace(gix, 2, "<g _flipped_='1' transform='" + TextUtils::svgMatrix(cm) + "'");
	return newSvg;
}

void CopperLogoItem::reloadImage(const QString & svg, const QSizeF & aspectRatio, const QString & fileName, bool addName) 
{
	if (isCopper0()) {
		if (!svg.contains("_flipped_")) {
			LogoItem::reloadImage(flipSvg(svg), aspectRatio, fileName, addName);
			return;
		}
	}

	LogoItem::reloadImage(svg, aspectRatio, fileName, addName);
}

bool CopperLogoItem::isCopper0() {
	return modelPart()->properties().value("layer").contains("0");
}

