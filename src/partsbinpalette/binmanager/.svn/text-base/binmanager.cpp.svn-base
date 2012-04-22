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


#include <QVBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QtDebug>
#include <QInputDialog>

#include "binmanager.h"
#include "stacktabwidget.h"
#include "stacktabbar.h"

#include "../../model/modelpart.h"
#include "../../mainwindow.h"
#include "../../model/palettemodel.h"
#include "../../waitpushundostack.h"
#include "../../debugdialog.h"
#include "../../partseditor/partseditormainwindow.h"
#include "../../utils/folderutils.h"
#include "../../utils/fileprogressdialog.h"
#include "../../referencemodel/referencemodel.h"
#include "../partsbinpalettewidget.h"

///////////////////////////////////////////////////////////

QString BinLocation::toString(BinLocation::Location location) {
	switch (location) {
	case BinLocation::User:
			return "user";
	case BinLocation::More:
			return "more";
	case BinLocation::App:
			return "app";
	case BinLocation::Outside:
	default:
		return "outside";
	}
}

BinLocation::Location BinLocation::fromString(const QString & locationString) {
	if (locationString.compare("user", Qt::CaseInsensitive) == 0) return BinLocation::User;
	if (locationString.compare("more", Qt::CaseInsensitive) == 0) return BinLocation::More;
	if (locationString.compare("app", Qt::CaseInsensitive) == 0) return BinLocation::App;
	return BinLocation::Outside;
}

BinLocation::Location BinLocation::findLocation(const QString & filename) 
{

	if (filename.startsWith(FolderUtils::getUserDataStorePath("bins"))) {
		return BinLocation::User;
	}
	else if (filename.startsWith(FolderUtils::getApplicationSubFolderPath("bins") + "/more")) {
		return BinLocation::More;
	}
	else if (filename.startsWith(FolderUtils::getApplicationSubFolderPath("bins"))) {
		return BinLocation::App;
	}

	return BinLocation::Outside;
}
///////////////////////////////////////////////////////////

QString BinManager::Title;
QString BinManager::MyPartsBinLocation;
QString BinManager::MyPartsBinTemplateLocation;
QString BinManager::SearchBinLocation;
QString BinManager::SearchBinTemplateLocation;
QString BinManager::ContribPartsBinLocation;
QString BinManager::ContribPartsBinTemplateLocation;
QString BinManager::TempPartsBinLocation;
QString BinManager::TempPartsBinTemplateLocation;
QString BinManager::CorePartsBinLocation;

QString BinManager::StandardBinStyle = "background-color: gray;";
QString BinManager::CurrentBinStyle = "background-color: black;";

QHash<QString, QString> BinManager::StandardBinIcons;

BinManager::BinManager(class ReferenceModel *refModel, class HtmlInfoView *infoView, WaitPushUndoStack *undoStack, MainWindow* parent)
	: QFrame(parent)
{
	BinManager::Title = tr("Parts");

	m_refModel = refModel;
	m_paletteModel = NULL;
	m_infoView = infoView;
	m_undoStack = undoStack;
	m_defaultSaveFolder = FolderUtils::getUserDataStorePath("bins");
	m_mainWindow = parent;
	m_currentBin = NULL;

	connect(this, SIGNAL(savePartAsBundled(const QString &)), m_mainWindow, SLOT(saveBundledPart(const QString &)));

	m_unsavedBinsCount = 0;

	QVBoxLayout *lo = new QVBoxLayout(this);

	m_stackTabWidget = new StackTabWidget(this);   
	m_stackTabWidget->setTabPosition(QTabWidget::West);
	lo->addWidget(m_stackTabWidget);

	lo->setMargin(0);
	lo->setSpacing(0);
	setMaximumHeight(500);

	createCombinedMenu();
	createContextMenus();

	DebugDialog::debug("init bin manager");
	QList<BinLocation *> actualLocations;
	findAllBins(actualLocations);
	restoreStateAndGeometry(actualLocations);
	foreach (BinLocation * location, actualLocations) {
		PartsBinPaletteWidget* bin = newBin();
        bin->load(location->path, m_mainWindow->fileProgressDialog(), true);
		m_stackTabWidget->addTab(bin, bin->icon(), bin->title());
		m_stackTabWidget->stackTabBar()->setTabToolTip(m_stackTabWidget->count() - 1, bin->title());
		registerBin(bin);
		delete location;
	}
	DebugDialog::debug("open core bin");
	openCoreBinIn();
		
	DebugDialog::debug("after core bin");

	connectTabWidget();
}

BinManager::~BinManager() {

}

void BinManager::addBin(PartsBinPaletteWidget* bin) {
	m_stackTabWidget->addTab(bin, bin->icon(), bin->title());
	registerBin(bin);
	setAsCurrentBin(bin);
}

void BinManager::registerBin(PartsBinPaletteWidget* bin) {

	if(bin->fileName() != ___emptyString___) {
		m_openedBins[bin->fileName()] = bin;
	}

	if (bin->fileName().compare(CorePartsBinLocation) == 0) {
		bin->setAllowsChanges(false);
	}
	else if (bin->fileName().compare(SearchBinLocation) == 0) {
		bin->setAllowsChanges(false);
	}
	else if (bin->fileName().compare(ContribPartsBinLocation) == 0) {
		bin->setAllowsChanges(false);
	}
	else if (bin->fileName().compare(TempPartsBinLocation) == 0) {
		bin->setAllowsChanges(false);
	}
	else if (bin->fileName().contains(FolderUtils::getApplicationSubFolderPath("bins"))) {
		bin->setAllowsChanges(false);
	}
}

void BinManager::connectTabWidget() {
	connect(
		m_stackTabWidget, SIGNAL(currentChanged(int)),
		this, SLOT(currentChanged(int))
	);
	connect(
		m_stackTabWidget, SIGNAL(tabCloseRequested(int)),
		this, SLOT(tabCloseRequested(int))
	);
}

void BinManager::insertBin(PartsBinPaletteWidget* bin, int index) {
	registerBin(bin);
	m_stackTabWidget->insertTab(index, bin, bin->icon(), bin->title());
	m_stackTabWidget->setCurrentIndex(index);
}

void BinManager::loadFromModel(PaletteModel *model) {
	Q_UNUSED(model);
	/*PartsBinPaletteWidget* bin = newBin();
	m_paletteModel=model;
	bin->loadFromModel(model);
	addBin(bin);*/
}

void BinManager::setPaletteModel(PaletteModel *model) {
	m_paletteModel = model;
}

bool BinManager::beforeClosing() {
	bool retval = true;

	for(int j=0; j < m_stackTabWidget->count(); j++) {
		PartsBinPaletteWidget *bin = qobject_cast<PartsBinPaletteWidget*>(m_stackTabWidget->widget(j));
		if (bin && !bin->fastLoaded()) {
			setAsCurrentTab(bin);
			retval = retval && bin->beforeClosing();
			if(!retval) break;
		}
	}


	if(retval) {
		saveStateAndGeometry();
	}

	return retval;
}

void BinManager::setAsCurrentTab(PartsBinPaletteWidget* bin) {
	m_stackTabWidget->setCurrentWidget(bin);
}


bool BinManager::hasAlienParts() {
	return false;

}

void BinManager::setInfoViewOnHover(bool infoViewOnHover) {
	Q_UNUSED(infoViewOnHover);
}

void BinManager::addNewPart(ModelPart *modelPart) {
	PartsBinPaletteWidget* myPartsBin = getOrOpenMyPartsBin();
	myPartsBin->addPart(modelPart);
	setDirtyTab(myPartsBin);
}

PartsBinPaletteWidget* BinManager::getOrOpenMyPartsBin() {
    return getOrOpenBin(MyPartsBinLocation, MyPartsBinTemplateLocation);
}

PartsBinPaletteWidget* BinManager::getOrOpenSearchBin() {
    PartsBinPaletteWidget * bin = getOrOpenBin(SearchBinLocation, SearchBinTemplateLocation);
	if (bin) {
		bin->setSaveQuietly(true);
	}
	return bin;
}

PartsBinPaletteWidget* BinManager::getOrOpenBin(const QString & binLocation, const QString & binTemplateLocation) {

    PartsBinPaletteWidget* partsBin = findBin(binLocation);

    if(!partsBin) {
        QString fileToOpen = QFileInfo(binLocation).exists() ? binLocation : createIfBinNotExists(binLocation, binTemplateLocation);
        partsBin = openBinIn(fileToOpen, false);
	}
	if (partsBin != NULL && partsBin->fastLoaded()) {
		partsBin->load(partsBin->fileName(), partsBin, false);
	}

    return partsBin;
}

PartsBinPaletteWidget* BinManager::findBin(const QString & binLocation) {
	for (int i = 0; i < m_stackTabWidget->count(); i++) {
		PartsBinPaletteWidget* bin = (PartsBinPaletteWidget *) m_stackTabWidget->widget(i);
        if(bin->fileName() == binLocation) {
            return bin;
		}
	}

	return NULL;
}

QString BinManager::createIfMyPartsNotExists() {
    return createIfBinNotExists(MyPartsBinLocation, MyPartsBinTemplateLocation);
}

QString BinManager::createIfSearchNotExists() {
    return createIfBinNotExists(SearchBinLocation, SearchBinTemplateLocation);
}

QString BinManager::createIfBinNotExists(const QString & dest, const QString & source)
{
    QString binPath = dest;
    QFile file(source);
	file.copy(binPath);
	return binPath;
}

void BinManager::addPart(ModelPart *modelPart, int position) {
	PartsBinPaletteWidget *bin = m_currentBin? m_currentBin: getOrOpenMyPartsBin();
	addPartAux(bin,modelPart,position);
}

void BinManager::addToMyPart(ModelPart *modelPart) {
	PartsBinPaletteWidget *bin = getOrOpenMyPartsBin();
	if (bin) {
		addPartAux(bin,modelPart);
		setAsCurrentTab(bin);
	}
}

void BinManager::addToContrib(ModelPart *modelPart) {
	PartsBinPaletteWidget *bin = getOrOpenBin(ContribPartsBinLocation, ContribPartsBinTemplateLocation);
	if (bin) {
		addPartAux(bin,modelPart);
		setAsCurrentTab(bin);
	}
}

void BinManager::addToTemp(ModelPart *modelPart) {
	PartsBinPaletteWidget *bin = getOrOpenBin(TempPartsBinLocation, TempPartsBinTemplateLocation);
	if (bin) {
		addPartAux(bin,modelPart);
		setAsCurrentTab(bin);
		bin->setDirty(false);
	}
}

void BinManager::hideTemp() {
	for (int i = 0; i < m_stackTabWidget->count(); i++) {
		PartsBinPaletteWidget* bin = (PartsBinPaletteWidget *) m_stackTabWidget->widget(i);
        if (bin->fileName() == TempPartsBinLocation) {
            m_stackTabWidget->removeTab(i);
			break;
		}
	}
}

void BinManager::addPartAux(PartsBinPaletteWidget *bin, ModelPart *modelPart, int position) {
	if(bin) {
		if (bin->fastLoaded()) {
			bin->load(bin->fileName(), bin, false);
		}
		bin->addPart(modelPart, position);
		setDirtyTab(bin);
	}
}

void BinManager::load(const QString& filename) {
	openBin(filename);
}


void BinManager::setDirtyTab(PartsBinPaletteWidget* w, bool dirty) {
	/*
	if (!w->windowTitle().contains(FritzingWindow::QtFunkyPlaceholder)) {
		// trying to deal with the warning in QWidget::setWindowModified
		// but setting the title here doesn't work
		QString t = w->windowTitle();
		if (t.isEmpty()) t = " ";
		w->setWindowTitle(t);
	}
	*/
	w->setWindowModified(dirty);
	if(m_stackTabWidget != NULL) {
		int tabIdx = m_stackTabWidget->indexOf(w);
		m_stackTabWidget->setTabText(tabIdx, w->title()+(dirty? " *": ""));
	} else {
		qWarning() << tr("BinManager::setDirtyTab: Couldn't set the bin '%1' as dirty").arg(w->title());
	}
}

void BinManager::updateTitle(PartsBinPaletteWidget* w, const QString& newTitle) {
	if(m_stackTabWidget != NULL) {
		m_stackTabWidget->setTabText(m_stackTabWidget->indexOf(w), newTitle+" *");
		setDirtyTab(w);
	}
	else {
		qWarning() << tr("BinManager::updateTitle: Couldn't set the bin '%1' as dirty").arg(w->title());
	}
}

PartsBinPaletteWidget* BinManager::newBinIn() {
	PartsBinPaletteWidget* bin = newBin();
	bin->setPaletteModel(new PaletteModel(true, false, false),true);
	bin->setTitle(tr("New bin (%1)").arg(++m_unsavedBinsCount));
	insertBin(bin, m_stackTabWidget->count());
	renameBin();
	return bin;
}

PartsBinPaletteWidget* BinManager::openBinIn(QString fileName, bool fastLoad) {
	if(fileName.isNull() || fileName.isEmpty()) {
		fileName = QFileDialog::getOpenFileName(
				this,
				tr("Select a Fritzing Parts Bin file to open"),
				m_defaultSaveFolder,
				tr("Fritzing Bin Files (*%1 *%2);;Fritzing Bin (*%1);;Fritzing Shareable Bin (*%2)")
				.arg(FritzingBinExtension).arg(FritzingBundledBinExtension)
		);
		if (fileName.isNull()) return false;
	}
	PartsBinPaletteWidget* bin = NULL;
	bool createNewOne = false;
	if(m_openedBins.contains(fileName)) {
		bin = m_openedBins[fileName];
		if(m_stackTabWidget) {
			m_stackTabWidget->setCurrentWidget(bin);
		} else {
			m_openedBins.remove(fileName);
			createNewOne = true;
		}
	} else {
		createNewOne = true;
	}

	if(createNewOne) {
		bin = newBin();
		if(bin->open(fileName, bin, fastLoad)) {
			m_openedBins[fileName] = bin;
			insertBin(bin, m_stackTabWidget->count());

			// to force the user to take a decision of what to do with the imported parts
			if(fileName.endsWith(FritzingBundledBinExtension)) {
				setDirtyTab(bin);
			} else {
				bin->saveAsLastBin();
			}
		}
	}
	if (!fastLoad) {
		setAsCurrentBin(bin);
	}
	return bin;
}

PartsBinPaletteWidget* BinManager::openCoreBinIn() {
	PartsBinPaletteWidget* bin = findBin(CorePartsBinLocation);
	if (bin != NULL) {
		setAsCurrentTab(bin);
	}
	else {
		bin = newBin();
		bin->setAllowsChanges(false);
		bin->load(BinManager::CorePartsBinLocation, bin, false);
		insertBin(bin, 0);
	}
	setAsCurrentBin(bin);
	return bin;
}

PartsBinPaletteWidget* BinManager::newBin() {
	PartsBinPaletteWidget* bin = new PartsBinPaletteWidget(m_refModel,m_infoView,m_undoStack,this);
	connect(
		bin, SIGNAL(fileNameUpdated(PartsBinPaletteWidget*, const QString&, const QString&)),
		this, SLOT(updateFileName(PartsBinPaletteWidget*, const QString&, const QString&))
	);
	connect(
		bin, SIGNAL(focused(PartsBinPaletteWidget*)),
		this, SLOT(setAsCurrentBin(PartsBinPaletteWidget*))
	);
	connect(bin, SIGNAL(saved(bool)), m_mainWindow, SLOT(binSaved(bool)));
	connect(m_mainWindow, SIGNAL(alienPartsDismissed()), bin, SLOT(removeAlienParts()));

	return bin;
}

void BinManager::currentChanged(int index) {
	PartsBinPaletteWidget *bin = getBin(index);
	if (bin) setAsCurrentBin(bin);
}

void BinManager::setAsCurrentBin(PartsBinPaletteWidget* bin) {
	if (bin == NULL) {
		qWarning() << tr("Cannot set a NULL bin as the current one");
		return;
	}

	if (bin->fastLoaded()) {
		bin->load(bin->fileName(), bin, false);
	}

	if (m_currentBin == bin) return;

	if (bin->fileName().compare(SearchBinLocation) == 0) {
		bin->focusSearch();
	}

	QString style = m_mainWindow->styleSheet();
	StackTabBar *currTabBar = NULL;
	if(m_currentBin && m_stackTabWidget) {
		currTabBar = m_stackTabWidget->stackTabBar();
		currTabBar->setProperty("current","false");
		currTabBar->setStyleSheet("");
		currTabBar->setStyleSheet(style);
	}
	if(m_stackTabWidget) {
		m_currentBin = bin;
		currTabBar = m_stackTabWidget->stackTabBar();
		currTabBar->setProperty("current","true");
		currTabBar->setStyleSheet("");
		currTabBar->setStyleSheet(style);
	}
}

void BinManager::closeBinIn(int index) {
	if (m_stackTabWidget->count() == 1) return;

	int realIndex = index == -1? m_stackTabWidget->currentIndex(): index;
	PartsBinPaletteWidget *w = getBin(realIndex);
	if(w && w->beforeClosing()) {
		m_stackTabWidget->removeTab(realIndex);
		m_openedBins.remove(w->fileName());
	}
}

PartsBinPaletteWidget* BinManager::getBin(int index) {
	return qobject_cast<PartsBinPaletteWidget*>(m_stackTabWidget->widget(index));
}

PartsBinPaletteWidget* BinManager::currentBin() {
	return qobject_cast<PartsBinPaletteWidget*>(m_stackTabWidget->currentWidget());
}

void BinManager::updateFileName(PartsBinPaletteWidget* bin, const QString &newFileName, const QString &oldFilename) {
	m_openedBins.remove(oldFilename);
	m_openedBins[newFileName] = bin;
}

void BinManager::saveStateAndGeometry() {
	QSettings settings;
	settings.remove("bins2"); // clean up previous state
	settings.beginGroup("bins2");

	for(int j = m_stackTabWidget->count() - 1; j >= 0; j--) {
		PartsBinPaletteWidget *bin = qobject_cast<PartsBinPaletteWidget*>(m_stackTabWidget->widget(j));
		if (bin) {
			settings.beginGroup(QString::number(j));
			settings.setValue("location", BinLocation::toString(bin->location()));
			settings.setValue("title", bin->title());
			settings.setValue("path", bin->fileName());
			settings.endGroup();
		}
	}

	settings.endGroup();
}

void BinManager::restoreStateAndGeometry(QList<BinLocation *> & actualLocations) {
	QList<BinLocation *> theoreticalLocations;

	QSettings settings;
	settings.beginGroup("bins2");
	int size = settings.childGroups().size();
	if (size == 0) { 
		// first time
		readTheoreticalLocations(theoreticalLocations);
	} 
	else {
		for (int i = 0; i < size; ++i) {
			settings.beginGroup(QString::number(i));
			BinLocation  * location = new BinLocation;
			location->location = BinLocation::fromString(settings.value("location").toString());
			location->path = settings.value("path").toString();
			location->title = settings.value("title").toString();
			theoreticalLocations.append(location);
			settings.endGroup();
		}
	}

	foreach (BinLocation * location, actualLocations) {
		location->marked = false;
	}
	foreach (BinLocation * tLocation, theoreticalLocations) {
		foreach (BinLocation * aLocation, actualLocations) {
			if (aLocation->title.compare(tLocation->title) == 0 && aLocation->location == tLocation->location) {
				aLocation->marked = true;
				break;
			}
		}
	}

	QList<BinLocation *> tempLocations(actualLocations);
	actualLocations.clear();

	foreach (BinLocation * tLocation, theoreticalLocations) {
                tLocation->marked = false;
                bool gotOne = false;

		for (int ix = 0; ix < tempLocations.count(); ix++) {
			BinLocation  * aLocation = tempLocations[ix];
			if (aLocation->title.compare(tLocation->title) == 0 && aLocation->location == tLocation->location) {
				gotOne = true;
				actualLocations.append(aLocation);
				tempLocations.removeAt(ix);
				break;
			}
		}
		if (gotOne) continue;

		if (tLocation->title == "___*___") {
			for (int ix = 0; ix < tempLocations.count(); ix++) {
				BinLocation * aLocation = tempLocations[ix];
				if (!aLocation->marked && aLocation->location == tLocation->location) {
					gotOne = true;
					actualLocations.append(aLocation);
					tempLocations.removeAt(ix);
					break;
				}
			}
		}
		if (gotOne) continue;

		if (!tLocation->path.isEmpty()) {
			QFileInfo info(tLocation->path);
			if (info.exists()) {
				actualLocations.append(tLocation);
                                tLocation->marked = true;
			}		
		}
	}

        foreach (BinLocation * tLocation, theoreticalLocations) {
            if (!tLocation->marked) {
                delete tLocation;
            }
	}

	// catch the leftovers
	actualLocations.append(tempLocations);

}

void BinManager::readTheoreticalLocations(QList<BinLocation *> & theoreticalLocations) 
{
	QFile file(":/resources/bins/order.xml");
	QString errorStr;
	int errorLine;
	int errorColumn;
	QDomDocument domDocument;

	if (!domDocument.setContent(&file, true, &errorStr, &errorLine, &errorColumn)) {
		DebugDialog::debug(QString("unable to parse order.xml: %1 %2 %3").arg(errorStr).arg(errorLine).arg(errorColumn));
		return;
	}

	QDomElement bin = domDocument.documentElement().firstChildElement("bin");
	while (!bin.isNull()) {
		BinLocation * location = new BinLocation;
		location->title = bin.attribute("title", "");
		location->location = BinLocation::fromString(bin.attribute("location", ""));
		theoreticalLocations.append(location);
		bin = bin.nextSiblingElement("bin");
	}
}

void BinManager::findAllBins(QList<BinLocation *> & locations) 
{
	BinLocation * location = new BinLocation;
	location->location = BinLocation::App;
	location->path = CorePartsBinLocation;
	QString icon;
	getBinTitle(location->path, location->title, icon);
	locations.append(location);

	QDir userBinsDir(FolderUtils::getUserDataStorePath("bins"));
	findBins(userBinsDir, locations, BinLocation::User);

	QDir dir(FolderUtils::getApplicationSubFolderPath("bins"));
	dir.cd("more");
	findBins(dir, locations, BinLocation::More);
}

void BinManager::findBins(QDir & dir, QList<BinLocation *> & locations, BinLocation::Location loc) {

	QStringList filters;
	filters << "*"+FritzingBinExtension;
	QFileInfoList files = dir.entryInfoList(filters);
	foreach(QFileInfo info, files) {
		BinLocation * location = new BinLocation;
		location->path = info.absoluteFilePath();
		location->location = loc;
		QString icon;
		getBinTitle(location->path, location->title, icon);
		locations.append(location);
	}
}

bool BinManager::getBinTitle(const QString & filename, QString & binName, QString & iconName) {
	QFile file(filename);
	file.open(QFile::ReadOnly);
	QXmlStreamReader xml(&file);
	xml.setNamespaceProcessing(false);

	while (!xml.atEnd()) {
        switch (xml.readNext()) {
        case QXmlStreamReader::StartElement:
			if (xml.name().toString().compare("module") == 0) {
				iconName = xml.attributes().value("icon").toString();
			}
			else if (xml.name().toString().compare("title") == 0) {
				binName = xml.readElementText();
				return true;
			}
			break;
			
		default:
			break;
		}
	}

	return false;
}

void BinManager::tabCloseRequested(int index) {
	closeBinIn(index);
}

void BinManager::addPartTo(PartsBinPaletteWidget* bin, ModelPart* mp) {
	if(mp) {
		bool alreadyIn = bin->contains(mp->moduleID());
		bin->addPart(mp);
		if(!alreadyIn) {
			setDirtyTab(bin,true);
		}
	}
}

void BinManager::newPartTo(PartsBinPaletteWidget* bin) {
	PartsEditorMainWindow *partsEditor = m_mainWindow->getPartsEditor(NULL, -1, NULL, bin);
	if (partsEditor == NULL) return;

	partsEditor->show();
	partsEditor->raise();
}

void BinManager::importPartTo(PartsBinPaletteWidget* bin) {
	QString newPartPath = QFileDialog::getOpenFileName(
		this,
		tr("Select a part to import"),
		"",
		tr("External Part (*%1)").arg(FritzingBundledPartExtension)
	);
	if(!newPartPath.isEmpty() && !newPartPath.isNull()) {
		ModelPart *mp = m_mainWindow->loadBundledPart(newPartPath,!bin->allowsChanges());
		if (bin->allowsChanges()) {
			addPartTo(bin,mp);
		}
	}
}

void BinManager::editSelectedPartFrom(PartsBinPaletteWidget* bin) {
	ModelPart *selectedMP = bin->selected();
	PartsEditorMainWindow *partsEditor = m_mainWindow->getPartsEditor(selectedMP, -1, NULL, bin);
	if (partsEditor == NULL) return;

	partsEditor->show();
	partsEditor->raise();
}

void BinManager::dockedInto(FDockWidget* dock) {
	Q_UNUSED(dock);
}

bool BinManager::isTabReorderingEvent(QDropEvent* event) {
	const QMimeData *m = event->mimeData();
	QStringList formats = m->formats();
	return formats.contains("action") && (m->data("action") == "tab-reordering");
}

const QString &BinManager::getSelectedModuleIDFromSketch() {
	return m_mainWindow->selectedModuleID();
}

QList<QAction*> BinManager::openedBinsActions(const QString &moduleId) {
	QMap<QString,QAction*> titlesAndActions; // QMap sorts values by key

	for (int i = 0; i < m_stackTabWidget->count(); i++) {
		PartsBinPaletteWidget* pppw = (PartsBinPaletteWidget *) m_stackTabWidget->widget(i);
		QAction *act = pppw->addPartToMeAction();
		act->setEnabled(!pppw->contains(moduleId));
		titlesAndActions[pppw->title()] = act;
	}

	return titlesAndActions.values();
}

void BinManager::openBin(const QString &filename) {
	openBinIn(filename, false);
}

MainWindow* BinManager::mainWindow() {
	return m_mainWindow;
}

void BinManager::initNames() {
    BinManager::MyPartsBinLocation = FolderUtils::getUserDataStorePath("bins")+"/my_parts.fzb";
    BinManager::MyPartsBinTemplateLocation =":/resources/bins/my_parts.fzb";
    BinManager::SearchBinLocation = FolderUtils::getUserDataStorePath("bins")+"/search.fzb";
    BinManager::SearchBinTemplateLocation =":/resources/bins/search.fzb";
	BinManager::ContribPartsBinLocation = FolderUtils::getUserDataStorePath("bins")+"/contribParts.fzb";
	BinManager::TempPartsBinLocation = FolderUtils::getUserDataStorePath("bins")+"/tempParts.fzb";
    BinManager::CorePartsBinLocation = FolderUtils::getApplicationSubFolderPath("bins")+"/core.fzb";

	StandardBinIcons.insert(BinManager::MyPartsBinLocation, "Mine.png");
	StandardBinIcons.insert(BinManager::SearchBinLocation, "Search.png");
	StandardBinIcons.insert(BinManager::ContribPartsBinLocation, "Contrib.png");
	StandardBinIcons.insert(BinManager::CorePartsBinLocation, "Core.png");
	StandardBinIcons.insert(BinManager::TempPartsBinLocation, "Temp.png");
}

void BinManager::search(const QString & searchText) {
    PartsBinPaletteWidget * searchBin = getOrOpenSearchBin();
    if (searchBin == NULL) return;

    QList<ModelPart *> modelParts = m_refModel->search(searchText, false);

    searchBin->removeParts();
    foreach (ModelPart * modelPart, modelParts) {
        //DebugDialog::debug(modelPart->title());
        this->addPartTo(searchBin, modelPart);
    }

    setDirtyTab(searchBin);
}

bool BinManager::currentViewIsIconView() {
	PartsBinPaletteWidget * bin = currentBin();
	if (bin == NULL) return true;
	
	return bin->currentViewIsIconView();
}

void BinManager::toIconView() {
	PartsBinPaletteWidget * bin = currentBin();
	if (bin == NULL) return;

	bin->toIconView();
}

void BinManager::toListView() {
	PartsBinPaletteWidget * bin = currentBin();
	if (bin == NULL) return;

	bin->toListView();
}

void BinManager::updateBinCombinedMenuCurrent() {
	PartsBinPaletteWidget * bin = currentBin();
	if (bin == NULL) return;

	updateBinCombinedMenu(bin);
}

void BinManager::updateBinCombinedMenu(PartsBinPaletteWidget * bin) {
	m_saveBinAction->setEnabled(bin->allowsChanges());
	m_renameBinAction->setEnabled(bin->canClose());
	m_closeBinAction->setEnabled(bin->canClose());
	m_deleteBinAction->setEnabled(bin->canClose());
	ModelPart *mp = bin->selected();
	bool enabled = (mp != NULL);
	m_editPartAction->setEnabled(enabled);
	m_exportPartAction->setEnabled(enabled && !mp->isCore());
	m_removePartAction->setEnabled(enabled && bin->allowsChanges());
	m_importPartAction->setEnabled(true);
}

void BinManager::createCombinedMenu() 
{
	m_combinedMenu = new QMenu(tr("Bin"), this);

	m_newBinAction = new QAction(tr("New Bin..."), this);
	m_newBinAction->setToolTip(tr("Create a new parts bin"));
	connect(m_newBinAction, SIGNAL(triggered()),this, SLOT(newBinIn()));

	m_openBinAction = new QAction(tr("Open Bin..."), this);
	m_openBinAction->setToolTip(tr("Open a parts bin from a file"));
	connect(m_openBinAction, SIGNAL(triggered()),this, SLOT(openNewBin()));

	m_closeBinAction = new QAction(tr("Close Bin"), this);
	m_closeBinAction->setToolTip(tr("Close parts bin"));
	connect(m_closeBinAction, SIGNAL(triggered()),this, SLOT(closeBin()));

	m_deleteBinAction = new QAction(tr("Delete Bin"), this);
	m_deleteBinAction->setToolTip(tr("Delete parts bin"));
	connect(m_deleteBinAction, SIGNAL(triggered()),this, SLOT(deleteBin()));

	m_saveBinAction = new QAction(tr("Save Bin"), this);
	m_saveBinAction->setToolTip(tr("Save parts bin"));
	connect(m_saveBinAction, SIGNAL(triggered()),this, SLOT(saveBin()));

	m_saveBinAsAction = new QAction(tr("Save Bin As..."), this);
	m_saveBinAsAction->setToolTip(tr("Save parts bin as..."));
	connect(m_saveBinAsAction, SIGNAL(triggered()),this, SLOT(saveBinAs()));

	m_saveBinAsBundledAction = new QAction(tr("Export Bin..."), this);
	m_saveBinAsBundledAction->setToolTip(tr("Save parts bin in compressed format..."));
	connect(m_saveBinAsBundledAction, SIGNAL(triggered()),this, SLOT(saveBundledBin()));

	m_renameBinAction = new QAction(tr("Rename Bin..."), this);
	m_renameBinAction->setToolTip(tr("Rename parts bin..."));
	connect(m_renameBinAction, SIGNAL(triggered()),this, SLOT(renameBin()));

	m_showListViewAction = new QAction(tr("Show Bin in List View"), this);
	m_showListViewAction->setCheckable(true);
	m_showListViewAction->setToolTip(tr("Display parts as a list"));
	connect(m_showListViewAction, SIGNAL(triggered()),this, SLOT(toListView()));

	m_showIconViewAction = new QAction(tr("Show Bin in Icon View"), this);
	m_showIconViewAction->setCheckable(true);
	m_showIconViewAction->setToolTip(tr("Display parts as icons"));
	connect(m_showIconViewAction, SIGNAL(triggered()),this, SLOT(toIconView()));

	m_combinedMenu->addAction(m_newBinAction);
	m_combinedMenu->addAction(m_openBinAction);
	m_combinedMenu->addSeparator();
	m_combinedMenu->addAction(m_closeBinAction);
	m_combinedMenu->addAction(m_deleteBinAction);
	m_combinedMenu->addAction(m_saveBinAction);
	m_combinedMenu->addAction(m_saveBinAsAction);
	m_combinedMenu->addAction(m_saveBinAsBundledAction);
	m_combinedMenu->addAction(m_renameBinAction);
	m_combinedMenu->addSeparator();
	m_combinedMenu->addAction(m_showIconViewAction);
	m_combinedMenu->addAction(m_showListViewAction);

	m_newPartAction = new QAction(tr("New Part..."), this);
	m_importPartAction = new QAction(tr("Import Part..."),this);
	m_editPartAction = new QAction(tr("Edit Part..."),this);
	m_exportPartAction = new QAction(tr("Export Part..."),this);
	m_removePartAction = new QAction(tr("Remove Part"),this);

	connect(m_newPartAction, SIGNAL(triggered()),this, SLOT(newPart()));
	connect(m_importPartAction, SIGNAL(triggered()),this, SLOT(importPart()));
	connect(m_editPartAction, SIGNAL(triggered()),this, SLOT(editSelected()));
	connect(m_exportPartAction, SIGNAL(triggered()),this, SLOT(exportSelected()));
	connect(m_removePartAction, SIGNAL(triggered()),this, SLOT(removeSelected()));

	connect(m_combinedMenu, SIGNAL(aboutToShow()), this, SLOT(updateBinCombinedMenuCurrent()));

	m_combinedMenu->addSeparator();
	m_combinedMenu->addAction(m_newPartAction);
	m_combinedMenu->addAction(m_importPartAction);
	m_combinedMenu->addSeparator();
	m_combinedMenu->addAction(m_editPartAction);
	m_combinedMenu->addAction(m_exportPartAction);
	m_combinedMenu->addAction(m_removePartAction);

}

void BinManager::createContextMenus() {
	m_binContextMenu = new QMenu(this);
	m_binContextMenu->addAction(m_closeBinAction);
	m_binContextMenu->addAction(m_deleteBinAction);
	m_binContextMenu->addAction(m_saveBinAction);
	m_binContextMenu->addAction(m_saveBinAsAction);
	m_binContextMenu->addAction(m_saveBinAsBundledAction);
	m_binContextMenu->addAction(m_renameBinAction);

	m_partContextMenu = new QMenu(this);
	m_partContextMenu->addAction(m_editPartAction);
	m_partContextMenu->addAction(m_exportPartAction);
	m_partContextMenu->addAction(m_removePartAction);
}

void BinManager::openNewBin() {
	openBinIn("", false);
}

void BinManager::closeBin() {
	closeBinIn(-1);
}

void BinManager::deleteBin() {
	PartsBinPaletteWidget * bin = currentBin();
	if (bin == NULL) return;

	QMessageBox::StandardButton answer = QMessageBox::question(
		this,
		tr("Delete bin"),
		tr("Do you really want to delete bin '%1'?  This action cannot be undone.").arg(bin->title()),
		QMessageBox::Yes | QMessageBox::No,
		QMessageBox::No
	);
	// TODO: make button texts translatable
	if (answer != QMessageBox::Yes) return;

	QString filename = bin->fileName();
	closeBinIn(-1);
	QFile::remove(filename);
}

void BinManager::newPart() {
	newPartTo(currentBin());
}

void BinManager::importPart() {
	importPartTo(currentBin());
}

void BinManager::editSelected() {
	editSelectedPartFrom(currentBin());
}

void BinManager::renameBin() {
	PartsBinPaletteWidget * bin = currentBin();
	if (bin == NULL) return;

	if (!currentBin()->allowsChanges()) {
		// TODO: disable menu item instead
		QMessageBox::warning(NULL, tr("Read-only bin"), tr("This bin cannot be renamed."));
		return;
	}

	bool ok;
	QString newTitle = QInputDialog::getText(
		this,
		tr("Rename bin"),
		tr("Please choose a name for the bin:"),
		QLineEdit::Normal,
		bin->title(),
		&ok
	);
	if(ok) {
		bin->setTitle(newTitle);
		m_stackTabWidget->stackTabBar()->setTabToolTip(m_stackTabWidget->currentIndex(), newTitle);
		bin->addPartToMeAction()->setText(newTitle);
		updateTitle(bin, newTitle);
	}
}

void BinManager::saveBin() {
	PartsBinPaletteWidget * bin = currentBin();
	if (bin == NULL) return;

	bool result = bin->save();
	if (result) setDirtyTab(currentBin(),false);
}

void BinManager::saveBinAs() {
	PartsBinPaletteWidget * bin = currentBin();
	if (bin == NULL) return;

	bin->saveAs();
}


void BinManager::updateViewChecks(bool iconView) {
	if (iconView) {
		m_showListViewAction->setChecked(false);
		m_showIconViewAction->setChecked(true);
	}
	else {
		m_showListViewAction->setChecked(true);
		m_showIconViewAction->setChecked(false);
	}
}

QMenu * BinManager::binContextMenu(PartsBinPaletteWidget * bin) {
	updateBinCombinedMenu(bin);
	return m_binContextMenu;
}

QMenu * BinManager::partContextMenu(PartsBinPaletteWidget * bin) {
	updateBinCombinedMenu(bin);
	return m_partContextMenu;
}

QMenu * BinManager::combinedMenu(PartsBinPaletteWidget * bin) {
	updateBinCombinedMenu(bin);
	return m_combinedMenu;
}

QMenu * BinManager::combinedMenu() {
	return m_combinedMenu;
}

bool BinManager::removeSelected() {
	PartsBinPaletteWidget * bin = currentBin();
	if (bin == NULL) return false;

	ModelPart * mp = bin->selected();
	if (mp == NULL) return false;

	QMessageBox::StandardButton answer = QMessageBox::question(
		this,
		tr("Remove from bin"),
		tr("Do you really want to remove '%1' from the bin?").arg(mp->title()),
		QMessageBox::Yes | QMessageBox::No,
		QMessageBox::Yes
	);
	// TODO: make button texts translatable
	if(answer != QMessageBox::Yes) return false;


	m_undoStack->push(new QUndoCommand("Parts bin: part removed"));
	bin->removePart(mp->moduleID());
	bin->setDirty();

	return true;
}

void BinManager::exportSelected() {
	PartsBinPaletteWidget * bin = currentBin();
	if (bin == NULL) return;

	ModelPart * mp = bin->selected();
	if (mp == NULL) return;

	emit savePartAsBundled(mp->moduleID());
}

void BinManager::saveBundledBin() {
	PartsBinPaletteWidget * bin = currentBin();
	if (bin == NULL) return;

	bin->saveBundledBin();
}

void BinManager::setTabIcon(PartsBinPaletteWidget* w, QIcon * icon) 
{
	if (m_stackTabWidget != NULL) {
		int tabIdx = m_stackTabWidget->indexOf(w);
		m_stackTabWidget->setTabIcon(tabIdx, *icon);
	}
}


void BinManager::copyFilesToContrib(ModelPart * mp, QWidget * originator) {
	PartsBinPaletteWidget * bin = qobject_cast<PartsBinPaletteWidget *>(originator);
	if (bin == NULL) return;

	if (bin->fileName().compare(TempPartsBinLocation) != 0) return;				// only copy from temp bin

	QString path = mp->path();
	if (path.isEmpty()) return;

	QFileInfo info(path);
	QFile fzp(path);
	
	QString parts = FolderUtils::getUserDataStorePath("parts");
	fzp.copy(parts + "/contrib/" + info.fileName());
	QString prefix = parts + "/svg/contrib/";

	QDir dir = info.absoluteDir();
	dir.cdUp();
	dir.cd("svg");
	dir.cd("contrib");

	QList<ViewIdentifierClass::ViewIdentifier> viewIdentifiers;
	viewIdentifiers << ViewIdentifierClass::IconView << ViewIdentifierClass::BreadboardView << ViewIdentifierClass::SchematicView << ViewIdentifierClass::PCBView;
	foreach (ViewIdentifierClass::ViewIdentifier viewIdentifier, viewIdentifiers) {
		QString fn = mp->hasBaseNameFor(viewIdentifier);
		if (!fn.isEmpty()) {
			QFile svg(dir.absoluteFilePath(fn));
			svg.copy(prefix + fn);
		}
	}
}
