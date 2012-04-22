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

$Revision: 5973 $:
$Author: cohen@irascible.com $:
$Date: 2012-04-12 10:58:05 -0700 (Thu, 12 Apr 2012) $

********************************************************************/

#include <QtGui>
#include <QSvgGenerator>
#include <QColor>
#include <QImageWriter>
#include <QInputDialog>

#include "mainwindow.h"
#include "debugdialog.h"
#include "waitpushundostack.h"
#include "partseditor/partseditormainwindow.h"
#include "help/aboutbox.h"
#include "autoroute/cmrouter/cmrouter.h"
#include "autoroute/autorouteprogressdialog.h"
#include "items/virtualwire.h"
#include "items/jumperitem.h"
#include "items/via.h"
#include "fsvgrenderer.h"
#include "items/note.h"
#include "eagle/fritzing2eagle.h"
#include "sketch/breadboardsketchwidget.h"
#include "sketch/schematicsketchwidget.h"
#include "sketch/pcbsketchwidget.h"
#include "partsbinpalette/binmanager/binmanager.h"
#include "utils/expandinglabel.h"
#include "infoview/htmlinfoview.h"
#include "utils/bendpointaction.h"
#include "fgraphicsscene.h"
#include "utils/fileprogressdialog.h"
#include "svg/svgfilesplitter.h"
#include "version/version.h"
#include "svg/groundplanegenerator.h"
#include "help/tipsandtricks.h"
#include "dialogs/setcolordialog.h"
#include "utils/folderutils.h"
#include "utils/graphicsutils.h"
#include "utils/textutils.h"
#include "connectors/ercdata.h"
#include "items/moduleidnames.h"
#include "navigator/miniviewcontainer.h"
#include "utils/zoomslider.h"
#include "layerpalette.h"
#include "program/programwindow.h"
#include "utils/autoclosemessagebox.h"
#include "processeventblocker.h"

////////////////////////////////////////////////////////

// help struct to create the example menu from a xml file
struct SketchDescriptor {
	SketchDescriptor(const QString &_id, const QString &_name, const QString &_src, QAction * _action) {
		id = _id;
		name = _name;
		src = _src;
		action = _action;
	}

	QString id;
	QString name;
	QString src;
	QAction * action;
};

bool sortSketchDescriptors(SketchDescriptor * s1, SketchDescriptor * s2){
    return s1->name.toLower() < s2->name.toLower();
}

/////////////////////////////////////////////////////////

void MainWindow::closeIfEmptySketch(MainWindow* mw) {
	int cascFactorX; int cascFactorY;
	// close empty sketch window if user opens from a file
	if (FolderUtils::isEmptyFileName(mw->m_fwFilename, untitledFileName()) && mw->undoStackIsEmpty()) {
		QTimer::singleShot(0, mw, SLOT(close()) );
		cascFactorX = 0;
		cascFactorY = 0;
	} else {
		cascFactorX = CascadeFactorX;
		cascFactorY = CascadeFactorY;
	}
	mw->move(x()+cascFactorX,y()+cascFactorY);
	mw->show();
}

void MainWindow::mainLoad() {
	QString path;
	// if it's the first time load is called use Documents folder
	if(m_firstOpen){
		path = defaultSaveFolder();
		m_firstOpen = false;
	}
	else {
		path = "";
	}

	QString fileName = FolderUtils::getOpenFileName(
			this,
			tr("Select a Fritzing File to Open"),
			path,
			tr("Fritzing Files (*%1 *%2);;Fritzing (*%1);;Fritzing Shareable (*%2)").arg(FritzingSketchExtension).arg(FritzingBundleExtension)
		);
	if (fileName.isNull()) return;

	if (alreadyOpen(fileName)) return;

	QFile file(fileName);
	if (!file.exists()) {
       QMessageBox::warning(this, tr("Fritzing"),
                             tr("Cannot find file %1.")
                             .arg(fileName));


		return;
	}



    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("Fritzing"),
                             tr("Cannot read file  1 %1:\n%2.")
                             .arg(fileName)
                             .arg(file.errorString()));
        return;
    }

    file.close();

    MainWindow* mw = newMainWindow(m_paletteModel, m_refModel, fileName, true, true);
	mw->loadWhich(fileName, true, true, "");
    mw->clearFileProgressDialog();
	closeIfEmptySketch(mw);
}

bool MainWindow::loadWhich(const QString & fileName, bool setAsLastOpened, bool addToRecent, const QString & displayName)
{
	if (!QFileInfo(fileName).exists()) {
		QMessageBox::warning(NULL, tr("Fritzing"), tr("File '%1' not found").arg(fileName));
		return false;
	}

	bool result = false;
    if (fileName.endsWith(FritzingSketchExtension)) {
		QFileInfo info(fileName);
		QMessageBox messageBox(NULL);
		messageBox.setWindowTitle(tr("the .fz file format is obsolete"));
		messageBox.setText(tr("The .fz file format has been deprecated.\n\nWould you like to convert '%1' to the .fzz format now or open it read-only?\n").arg(info.fileName()));
		messageBox.setInformativeText(tr("The conversion process will not modify '%1'.").arg(info.fileName()));
		messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
		messageBox.setDefaultButton(QMessageBox::Yes);
		messageBox.setIcon(QMessageBox::Question);
		messageBox.setWindowModality(Qt::WindowModal);
		messageBox.setButtonText(QMessageBox::Yes, tr("Convert"));
		messageBox.setButtonText(QMessageBox::No, tr("Read-only"));
		messageBox.setButtonText(QMessageBox::Cancel, tr("Cancel"));
		QMessageBox::StandardButton answer = (QMessageBox::StandardButton) messageBox.exec();

		if (answer == QMessageBox::Cancel) return false;

		QString bundledFileName;
		if (answer == QMessageBox::Yes) {
			QString fileExt;
			bundledFileName = FolderUtils::getSaveFileName(this, tr("Please specify an .fzz file name to save '%1' to").arg(info.fileName()), fileName + "z", tr("Fritzing (*%1)").arg(FritzingBundleExtension), &fileExt);
			if (bundledFileName.isEmpty()) return false;	
		}

    	mainLoad(fileName, displayName);
		result = true;

		QFile file(fileName);
		QDir dest(m_fzzFolder);
		file.copy(dest.absoluteFilePath(info.fileName()));			// copy the .fz file directly

		if (answer == QMessageBox::Yes) {
			saveAsShareable(bundledFileName, false);					// false to prevent saving a bundle inside the bundle
			setCurrentFile(bundledFileName, addToRecent, setAsLastOpened);
		}
		else {
			this->setReadOnly(true);
			setCurrentFile(fileName, false, false);
		}
    } 
	else if(fileName.endsWith(FritzingBundleExtension)) {
    	loadBundledSketch(fileName, addToRecent, setAsLastOpened);
		result = true;
    } 
	else if (
    		fileName.endsWith(FritzingBinExtension)
    		|| fileName.endsWith(FritzingBundledBinExtension)
    	) {
		m_binManager->load(fileName);
		result = true;
	} 
	else if (fileName.endsWith(FritzingPartExtension)) {
		notYetImplemented(tr("directly loading parts"));
	}  
	else if (fileName.endsWith(FritzingBundledPartExtension)) {
		loadBundledPart(fileName);
		result = true;
	}

	if (result) {
		this->show();
	}

	return result;
}

void MainWindow::mainLoad(const QString & fileName, const QString & displayName) {

	if (m_fileProgressDialog) {
		m_fileProgressDialog->setMaximum(200);
		m_fileProgressDialog->setValue(102);
	}
	this->show();
	showAllFirstTimeHelp(false);
	ProcessEventBlocker::processEvents();


	QString displayName2 = displayName;
	if (displayName.isEmpty()) {
		QFileInfo fileInfo(fileName);
		displayName2 = fileInfo.fileName();
	}

	if (m_fileProgressDialog) {
		m_fileProgressDialog->setMessage(tr("loading %1 (model)").arg(displayName2));
		m_fileProgressDialog->setValue(110);
	}
	ProcessEventBlocker::processEvents();


	QList<ModelPart *> modelParts;

	connect(m_sketchModel, SIGNAL(loadedViews(ModelBase *, QDomElement &)),
				this, SLOT(loadedViewsSlot(ModelBase *, QDomElement &)), Qt::DirectConnection);
	connect(m_sketchModel, SIGNAL(loadedRoot(const QString &, ModelBase *, QDomElement &)),
				this, SLOT(loadedRootSlot(const QString &, ModelBase *, QDomElement &)), Qt::DirectConnection);
	m_sketchModel->load(fileName, m_paletteModel, modelParts);
	DebugDialog::debug("core loaded");
	disconnect(m_sketchModel, SIGNAL(loadedViews(ModelBase *, QDomElement &)),
				this, SLOT(loadedViewsSlot(ModelBase *, QDomElement &)));
	disconnect(m_sketchModel, SIGNAL(loadedRoot(const QString &, ModelBase *, QDomElement &)),
				this, SLOT(loadedRootSlot(const QString &, ModelBase *, QDomElement &)));

	ProcessEventBlocker::processEvents();
	if (m_fileProgressDialog) {
		m_fileProgressDialog->setValue(155);
		m_fileProgressDialog->setMessage(tr("loading %1 (breadboard)").arg(displayName2));
	}

	QList<long> newIDs;
	m_breadboardGraphicsView->loadFromModelParts(modelParts, BaseCommand::SingleView, NULL, false, NULL, false, newIDs);

	ProcessEventBlocker::processEvents();
	if (m_fileProgressDialog) {
		m_fileProgressDialog->setValue(170);
		m_fileProgressDialog->setMessage(tr("loading %1 (pcb)").arg(displayName2));
	}

	newIDs.clear();
	m_pcbGraphicsView->loadFromModelParts(modelParts, BaseCommand::SingleView, NULL, false, NULL, false, newIDs);

	ProcessEventBlocker::processEvents();
	if (m_fileProgressDialog) {
		m_fileProgressDialog->setValue(185);
		m_fileProgressDialog->setMessage(tr("loading %1 (schematic)").arg(displayName2));
	}

	newIDs.clear();
	m_schematicGraphicsView->loadFromModelParts(modelParts, BaseCommand::SingleView, NULL, false, NULL, false, newIDs);

	ProcessEventBlocker::processEvents();
	if (m_fileProgressDialog) {
		m_fileProgressDialog->setValue(198);
	}

}

void MainWindow::copy() {
	if (m_currentGraphicsView == NULL) return;
	m_currentGraphicsView->copy();
}

void MainWindow::cut() {
	if (m_currentGraphicsView == NULL) return;
	m_currentGraphicsView->cut();
}

void MainWindow::pasteInPlace() {
	pasteAux(true);
}

void MainWindow::paste() {
	pasteAux(false);
}

void MainWindow::pasteAux(bool pasteInPlace)
{
	if (m_currentGraphicsView == NULL) return;

	QClipboard *clipboard = QApplication::clipboard();
	if (clipboard == NULL) {
		// shouldn't happen
		return;
	}

	const QMimeData* mimeData = clipboard->mimeData(QClipboard::Clipboard);
	if (mimeData == NULL) return;

   	if (!mimeData->hasFormat("application/x-dnditemsdata")) return;

    QByteArray itemData = mimeData->data("application/x-dnditemsdata");
	QList<ModelPart *> modelParts;
	QHash<QString, QRectF> boundingRects;
	if (m_sketchModel->paste(m_paletteModel, itemData, modelParts, boundingRects, false)) {
		QUndoCommand * parentCommand = new QUndoCommand("Paste");

		QRectF r;
		QRectF boundingRect = boundingRects.value(m_breadboardGraphicsView->viewName(), r);
		QList<long> newIDs;
		m_breadboardGraphicsView->loadFromModelParts(modelParts, BaseCommand::SingleView, parentCommand, true, pasteInPlace ? &r : &boundingRect, false, newIDs);

		boundingRect = boundingRects.value(m_pcbGraphicsView->viewName(), r);
		newIDs.clear();
		m_pcbGraphicsView->loadFromModelParts(modelParts, BaseCommand::SingleView, parentCommand, true, pasteInPlace ? &r : &boundingRect, false, newIDs);

		boundingRect = boundingRects.value(m_schematicGraphicsView->viewName(), r);
		newIDs.clear();
		m_schematicGraphicsView->loadFromModelParts(modelParts, BaseCommand::SingleView, parentCommand, true, pasteInPlace ? &r : &boundingRect, false, newIDs);

		foreach (long id, newIDs) {
			new IncLabelTextCommand(m_breadboardGraphicsView, id, parentCommand);
		}

		m_undoStack->push(parentCommand);
	}
}

void MainWindow::duplicate() {
	if (m_currentGraphicsView == NULL) return;

	m_currentGraphicsView->copy();
	paste();

	//m_currentGraphicsView->duplicate();
}

void MainWindow::doDelete() {
	//DebugDialog::debug(QString("invoking do delete") );

	if (m_currentGraphicsView != NULL) {
		m_currentGraphicsView->deleteSelected(retrieveWire());
	}
}

void MainWindow::selectAll() {
	if (m_currentGraphicsView != NULL) {
		m_currentGraphicsView->selectDeselectAllCommand(true);
	}
}

void MainWindow::deselect() {
	if (m_currentGraphicsView != NULL) {
		m_currentGraphicsView->selectDeselectAllCommand(false);
	}
}

void MainWindow::about()
{
	AboutBox::showAbout();
}

void MainWindow::tipsAndTricks()
{
	TipsAndTricks::showTipsAndTricks();
}

void MainWindow::createActions()
{
	m_raiseWindowAct = new QAction(m_fwFilename, this);
	m_raiseWindowAct->setCheckable(true);
	connect( m_raiseWindowAct, SIGNAL(triggered()), this, SLOT(raiseAndActivate()));
	updateRaiseWindowAction();

    createFileMenuActions();
    createEditMenuActions();
    createPartMenuActions();
    createViewMenuActions();
    createWindowMenuActions();
    createHelpMenuActions();
	createTraceMenuActions();
}

void MainWindow::createFileMenuActions() {
	m_newAct = new QAction(tr("New"), this);
	m_newAct->setShortcut(tr("Ctrl+N"));
	m_newAct->setStatusTip(tr("Create a new sketch"));
	connect(m_newAct, SIGNAL(triggered()), this, SLOT(createNewSketch()));

	m_openAct = new QAction(tr("&Open..."), this);
	m_openAct->setShortcut(tr("Ctrl+O"));
	m_openAct->setStatusTip(tr("Open a sketch"));
	connect(m_openAct, SIGNAL(triggered()), this, SLOT(mainLoad()));

	createOpenRecentMenu();
	createOpenExampleMenu();
	createCloseAction();
	createExportActions();
	createOrderFabAct();

	QString name;
	QString path;
	QStringList args;
	if (externalProcess(name, path, args)) {
		m_launchExternalProcessAct = new QAction(name, this);
		m_launchExternalProcessAct->setStatusTip(tr("Shell launch %1").arg(path));
		connect(m_launchExternalProcessAct, SIGNAL(triggered()), this, SLOT(launchExternalProcess()));
	}

#ifndef QT_NO_DEBUG
    m_exceptionAct = new QAction(tr("throw test exception"), this);
    m_exceptionAct->setStatusTip(tr("throw a fake exception to see what happens"));
    connect(m_exceptionAct, SIGNAL(triggered()), this, SLOT(throwFakeException()));
#endif

	m_quitAct = new QAction(tr("&Quit"), this);
	m_quitAct->setShortcut(tr("Ctrl+Q"));
	m_quitAct->setStatusTip(tr("Quit the application"));
	connect(m_quitAct, SIGNAL(triggered()), qApp, SLOT(closeAllWindows2()));
	m_quitAct->setMenuRole(QAction::QuitRole);

}

void MainWindow::createOpenExampleMenu() {
	m_openExampleMenu = new QMenu(tr("&Open Example"), this);
	QString folderPath = FolderUtils::getApplicationSubFolderPath("sketches")+"/";
	populateMenuFromXMLFile(m_openExampleMenu, m_openExampleActions, folderPath, "index.xml");
}

void MainWindow::populateMenuFromXMLFile(QMenu *parentMenu, QStringList &actionsTracker, const QString &folderPath, const QString &indexFileName) 
{
	QDomDocument dom;
	QFile file(folderPath+indexFileName);
	dom.setContent(&file);

	QDomElement domElem = dom.documentElement();
	QDomElement indexDomElem = domElem.firstChildElement("sketches");
	QDomElement taxonomyDomElem = domElem.firstChildElement("categories");

	QHash<QString, struct SketchDescriptor *> index = indexAvailableElements(indexDomElem, folderPath, actionsTracker);
	QList<SketchDescriptor *> sketchDescriptors(index.values());
	qSort(sketchDescriptors.begin(), sketchDescriptors.end(), sortSketchDescriptors);

	if (sketchDescriptors.size() > 0) {
		// set up the "all" category
		QDomElement all = dom.createElement("category");
		all.setAttribute("name", tr("All"));
		taxonomyDomElem.appendChild(all);
		foreach (SketchDescriptor * sketchDescriptor, sketchDescriptors) {
			QDomElement sketch = dom.createElement("sketch");
			sketch.setAttribute("id", sketchDescriptor->id);
			all.appendChild(sketch);
		}
	}
	populateMenuWithIndex(index, parentMenu, taxonomyDomElem);
	foreach (SketchDescriptor * sketchDescriptor, index.values()) {
		delete sketchDescriptor;
	}
}

QHash<QString, struct SketchDescriptor *> MainWindow::indexAvailableElements(QDomElement &domElem, const QString &srcPrefix, QStringList & actionsTracker) {
	QHash<QString, struct SketchDescriptor *> retval;
	QDomElement sketch = domElem.firstChildElement("sketch");
	while(!sketch.isNull()) {
		const QString id = sketch.attribute("id");
		const QString name = sketch.attribute("name");
		QString srcAux = sketch.attribute("src");
		// if it's an absolute path, don't prefix it
		const QString src = QFileInfo(srcAux).exists()? srcAux: srcPrefix+srcAux;
		if(QFileInfo(src).exists()) {
			actionsTracker << name;
			QAction * action = new QAction(name, this);
			action->setData(src);
			connect(action,SIGNAL(triggered()),this,SLOT(openRecentOrExampleFile()));
			retval[id] = new SketchDescriptor(id,name,src, action);
		}
		sketch = sketch.nextSiblingElement("sketch");
	}
	return retval;
}

void MainWindow::populateMenuWithIndex(const QHash<QString, struct SketchDescriptor *>  &index, QMenu * parentMenu, QDomElement &domElem) {
	// note: the <sketch> element here is not the same as the <sketch> element in indexAvailableElements()
	QDomElement e = domElem.firstChildElement();
	while(!e.isNull()) {
		if (e.nodeName() == "sketch") {
			QString id = e.attribute("id");
			if (!id.isEmpty()) {
				if(index[id]) {
					SketchDescriptor * sketchDescriptor = index[id];
					parentMenu->addAction(sketchDescriptor->action);
				} 
				else 
				{
					qWarning() << tr("MainWindow::populateMenuWithIndex: couldn't load example with id='%1'").arg(id);
				}
			}
		} 
		else if (e.nodeName() == "category") {
			QString name = e.attribute("name");
			QMenu * currMenu = new QMenu(name, parentMenu);
			parentMenu->addMenu(currMenu);
			populateMenuWithIndex(index, currMenu, e);
		}
		else if (e.nodeName() == "separator") {
			parentMenu->addSeparator();
		}
		else if (e.nodeName() == "url") {
			QAction * action = new QAction(e.attribute("name"), this);
			action->setData(e.attribute("href"));
			connect(action, SIGNAL(triggered()), this, SLOT(openURL()));
			parentMenu->addAction(action);
		}
		e = e.nextSiblingElement();
	}
}

void MainWindow::populateMenuFromFolderContent(QMenu * parentMenu, const QString &path) {
	QDir *currDir = new QDir(path);
	QStringList content = currDir->entryList(QDir::AllEntries | QDir::NoDotAndDotDot);
	if(content.size() > 0) {
		for(int i=0; i < content.size(); i++) {
			QString currFile = content.at(i);
			QString currFilePath = currDir->absoluteFilePath(currFile);
			if(QFileInfo(currFilePath).isDir()) {
				QMenu * currMenu = new QMenu(currFile, parentMenu);
				parentMenu->addMenu(currMenu);
				populateMenuFromFolderContent(currMenu, currFilePath);
			} else {
				QString actionText = QFileInfo(currFilePath).completeBaseName();
				m_openExampleActions << actionText;
				QAction * currAction = new QAction(actionText, this);
				currAction->setData(currFilePath);
				connect(currAction,SIGNAL(triggered()),this,SLOT(openRecentOrExampleFile()));
				parentMenu->addAction(currAction);
			}
		}
	} else {
		parentMenu->setEnabled(false);
	}
	delete currDir;
}

void MainWindow::createOpenRecentMenu() {
	m_openRecentFileMenu = new QMenu(tr("&Open Recent Files"), this);

	for (int i = 0; i < MaxRecentFiles; ++i) {
		m_openRecentFileActs[i] = new QAction(this);
		m_openRecentFileActs[i]->setVisible(false);
		connect(m_openRecentFileActs[i], SIGNAL(triggered()),this, SLOT(openRecentOrExampleFile()));
	}


    for (int i = 0; i < MaxRecentFiles; ++i) {
    	m_openRecentFileMenu->addAction(m_openRecentFileActs[i]);
    }
    updateRecentFileActions();
}

void MainWindow::updateFileMenu() {
	updateRecentFileActions();
	m_orderFabAct->setEnabled(true);
}

void MainWindow::updateRecentFileActions() {
	QSettings settings;
	QStringList files = settings.value("recentFileList").toStringList();
	int ix = 0;
	for (int i = 0; i < files.size(); ++i) {
		QFileInfo finfo(files[i]);
		if (!finfo.exists()) continue;

		QString text = tr("&%1 %2").arg(ix + 1).arg(finfo.fileName());
		m_openRecentFileActs[ix]->setText(text);
		m_openRecentFileActs[ix]->setData(files[i]);
		m_openRecentFileActs[ix]->setVisible(true);
		if (++ix >= (int) MaxRecentFiles) {
			break;
		}
	}

	for (int j = ix; j < MaxRecentFiles; ++j) {
		m_openRecentFileActs[j]->setVisible(false);
	}

	m_openRecentFileMenu->setEnabled(ix > 0);
}

void MainWindow::createEditMenuActions() {
	m_undoAct = m_undoGroup->createUndoAction(this, tr("Undo"));
	m_undoAct->setShortcuts(QKeySequence::Undo);
	m_undoAct->setText(tr("Undo"));

	m_redoAct = m_undoGroup->createRedoAction(this, tr("Redo"));
	m_redoAct->setShortcuts(QKeySequence::Redo);
	m_redoAct->setText(tr("Redo"));

	m_cutAct = new QAction(tr("&Cut"), this);
	m_cutAct->setShortcut(tr("Ctrl+X"));
	m_cutAct->setStatusTip(tr("Cut selection"));
	connect(m_cutAct, SIGNAL(triggered()), this, SLOT(cut()));

	m_copyAct = new QAction(tr("&Copy"), this);
	m_copyAct->setShortcut(tr("Ctrl+C"));
	m_copyAct->setStatusTip(tr("Copy selection"));
	connect(m_copyAct, SIGNAL(triggered()), this, SLOT(copy()));

	m_pasteAct = new QAction(tr("&Paste"), this);
	m_pasteAct->setShortcut(tr("Ctrl+V"));
	m_pasteAct->setStatusTip(tr("Paste clipboard contents"));
	connect(m_pasteAct, SIGNAL(triggered()), this, SLOT(paste()));

	m_pasteInPlaceAct = new QAction(tr("Paste in Place"), this);
	m_pasteInPlaceAct->setShortcut(tr("Ctrl+B"));
	m_pasteInPlaceAct->setStatusTip(tr("Paste clipboard contents in place"));
	connect(m_pasteInPlaceAct, SIGNAL(triggered()), this, SLOT(pasteInPlace()));

	m_duplicateAct = new QAction(tr("&Duplicate"), this);
	m_duplicateAct->setShortcut(tr("Ctrl+D"));
	m_duplicateAct->setStatusTip(tr("Duplicate selection"));
	connect(m_duplicateAct, SIGNAL(triggered()), this, SLOT(duplicate()));

	m_deleteAct = new QAction(tr("&Delete"), this);
	m_deleteAct->setStatusTip(tr("Delete selection"));
	connect(m_deleteAct, SIGNAL(triggered()), this, SLOT(doDelete()));
	#ifdef Q_WS_MAC
		m_deleteAct->setShortcut(Qt::Key_Backspace);
	#else
		m_deleteAct->setShortcut(QKeySequence::Delete);
	#endif
	m_deleteWireAct = new WireAction(m_deleteAct);
	m_deleteWireAct->setText("&Delete Wire");
	connect(m_deleteWireAct, SIGNAL(triggered()), this, SLOT(doDelete()));

	m_selectAllAct = new QAction(tr("&Select All"), this);
	m_selectAllAct->setShortcut(tr("Ctrl+A"));
	m_selectAllAct->setStatusTip(tr("Select all elements"));
	connect(m_selectAllAct, SIGNAL(triggered()), this, SLOT(selectAll()));

	m_deselectAct = new QAction(tr("&Deselect"), this);
	m_deselectAct->setStatusTip(tr("Deselect"));
	connect(m_deselectAct, SIGNAL(triggered()), this, SLOT(deselect()));

	m_addNoteAct = new QAction(tr("Add Note"), this);
	m_addNoteAct->setStatusTip(tr("Add a note"));
	connect(m_addNoteAct, SIGNAL(triggered()), this, SLOT(addNote()));

	m_preferencesAct = new QAction(tr("&Preferences..."), this);
	m_preferencesAct->setStatusTip(tr("Show the application's about box"));
	m_preferencesAct->setMenuRole(QAction::PreferencesRole);						// make sure this is added to the correct menu on mac
	connect(m_preferencesAct, SIGNAL(triggered()), QApplication::instance(), SLOT(preferences()));
}

void MainWindow::createPartMenuActions() {
	m_createNewPart = new QAction(tr("&New"), this);
	m_createNewPart->setShortcut(tr("Alt+Ctrl+N"));
	m_createNewPart->setStatusTip(tr("Create new part"));
	connect(m_createNewPart, SIGNAL(triggered()), this, SLOT(createNewPart()));

	m_openInPartsEditorAct = new QAction(tr("&Edit"), this);
	m_openInPartsEditorAct->setShortcut(tr("Ctrl+Return"));
	m_openInPartsEditorAct->setStatusTip(tr("Open the old parts editor"));
	connect(m_openInPartsEditorAct, SIGNAL(triggered()), this, SLOT(openInPartsEditor()));

	m_addToBinMenu = new QMenu(tr("&Add to bin..."), this);
	m_addToBinMenu->setStatusTip(tr("Add selected part to bin"));

	m_disconnectAllAct = new QAction(tr("Disconnect All Wires"), this);
	m_disconnectAllAct->setStatusTip(tr("Disconnect all wires connected to this connector"));
	connect(m_disconnectAllAct, SIGNAL(triggered()), this, SLOT(disconnectAll()));

#ifndef QT_NO_DEBUG
	m_infoViewOnHoverAction = new QAction(tr("Update InfoView on hover"), this);
	m_infoViewOnHoverAction->setCheckable(true);
	bool infoViewOnHover = true;
	m_infoViewOnHoverAction->setChecked(infoViewOnHover);
	setInfoViewOnHover(infoViewOnHover);
	connect(m_infoViewOnHoverAction, SIGNAL(toggled(bool)), this, SLOT(setInfoViewOnHover(bool)));

	m_exportNormalizedSvgAction = new QAction(tr("Export Normalized SVG"), this);
	m_exportNormalizedSvgAction->setStatusTip(tr("Export 1000 dpi SVG of this part in this view"));
	connect(m_exportNormalizedSvgAction, SIGNAL(triggered()), this, SLOT(exportNormalizedSVG()));
	
	m_exportNormalizedFlattenedSvgAction = new QAction(tr("Export Normalized Flattened SVG"), this);
	m_exportNormalizedFlattenedSvgAction->setStatusTip(tr("Export 1000 dpi Flattened SVG of this part in this view"));
	connect(m_exportNormalizedFlattenedSvgAction, SIGNAL(triggered()), this, SLOT(exportNormalizedFlattenedSVG()));
#endif


	m_rotate45cwAct = new QAction(tr("Rotate 45\x00B0 Clockwise"), this);
	m_rotate45cwAct->setStatusTip(tr("Rotate current selection 45 degrees clockwise"));
	connect(m_rotate45cwAct, SIGNAL(triggered()), this, SLOT(rotate45cw()));

	m_rotate90cwAct = new QAction(tr("Rotate 90\x00B0 Clockwise"), this);
	m_rotate90cwAct->setStatusTip(tr("Rotate the selected parts by 90 degrees clockwise"));
	connect(m_rotate90cwAct, SIGNAL(triggered()), this, SLOT(rotate90cw()));

	m_rotate180Act = new QAction(tr("Rotate 180\x00B0"), this);
	m_rotate180Act->setStatusTip(tr("Rotate the selected parts by 180 degrees"));
	connect(m_rotate180Act, SIGNAL(triggered()), this, SLOT(rotate180()));

	m_rotate90ccwAct = new QAction(tr("Rotate 90\x00B0 Counter Clockwise"), this);
	m_rotate90ccwAct->setStatusTip(tr("Rotate current selection 90 degrees counter clockwise"));
	connect(m_rotate90ccwAct, SIGNAL(triggered()), this, SLOT(rotate90ccw()));

	m_rotate45ccwAct = new QAction(tr("Rotate 45\x00B0 Counter Clockwise"), this);
	m_rotate45ccwAct->setStatusTip(tr("Rotate current selection 45 degrees counter clockwise"));
	connect(m_rotate45ccwAct, SIGNAL(triggered()), this, SLOT(rotate45ccw()));

	m_flipHorizontalAct = new QAction(tr("&Flip Horizontal"), this);
	m_flipHorizontalAct->setStatusTip(tr("Flip current selection horizontally"));
	connect(m_flipHorizontalAct, SIGNAL(triggered()), this, SLOT(flipHorizontal()));

	m_flipVerticalAct = new QAction(tr("&Flip Vertical"), this);
	m_flipVerticalAct->setStatusTip(tr("Flip current selection vertically"));
	connect(m_flipVerticalAct, SIGNAL(triggered()), this, SLOT(flipVertical()));

	m_bringToFrontAct = new QAction(tr("Bring to Front"), this);
	m_bringToFrontAct->setShortcut(tr("Shift+Ctrl+]"));
    m_bringToFrontAct->setStatusTip(tr("Bring selected object(s) to front of their layer"));
    connect(m_bringToFrontAct, SIGNAL(triggered()), this, SLOT(bringToFront()));
	m_bringToFrontWireAct = new WireAction(m_bringToFrontAct);
    connect(m_bringToFrontWireAct, SIGNAL(triggered()), this, SLOT(bringToFront()));

	m_bringForwardAct = new QAction(tr("Bring Forward"), this);
	m_bringForwardAct->setShortcut(tr("Ctrl+]"));
    m_bringForwardAct->setStatusTip(tr("Bring selected object(s) forward in their layer"));
    connect(m_bringForwardAct, SIGNAL(triggered()), this, SLOT(bringForward()));
	m_bringForwardWireAct = new WireAction(m_bringForwardAct);
    connect(m_bringForwardWireAct, SIGNAL(triggered()), this, SLOT(bringForward()));

	m_sendBackwardAct = new QAction(tr("Send Backward"), this);
	m_sendBackwardAct->setShortcut(tr("Ctrl+["));
    m_sendBackwardAct->setStatusTip(tr("Send selected object(s) back in their layer"));
    connect(m_sendBackwardAct, SIGNAL(triggered()), this, SLOT(sendBackward()));
	m_sendBackwardWireAct = new WireAction(m_sendBackwardAct);
    connect(m_sendBackwardWireAct, SIGNAL(triggered()), this, SLOT(sendBackward()));

	m_sendToBackAct = new QAction(tr("Send to Back"), this);
	m_sendToBackAct->setShortcut(tr("Shift+Ctrl+["));
    m_sendToBackAct->setStatusTip(tr("Send selected object(s) to the back of their layer"));
    connect(m_sendToBackAct, SIGNAL(triggered()), this, SLOT(sendToBack()));
	m_sendToBackWireAct = new WireAction(m_sendToBackAct);
    connect(m_sendToBackWireAct, SIGNAL(triggered()), this, SLOT(sendToBack()));

	m_moveLockAct = new QAction(tr("Lock Part"), this);
    m_moveLockAct->setStatusTip(tr("Prevent a part from being moved"));
	m_moveLockAct->setCheckable(true);
    connect(m_moveLockAct, SIGNAL(triggered()), this, SLOT(moveLock()));

	m_selectMoveLockAct = new QAction(tr("Select All Locked Parts"), this);
    m_selectMoveLockAct->setStatusTip(tr("Select all parts that can't be moved"));
    connect(m_selectMoveLockAct, SIGNAL(triggered()), this, SLOT(selectMoveLock()));

	m_showAllLayersAct = new QAction(tr("&Show All Layers"), this);
	m_showAllLayersAct->setStatusTip(tr("Show all the available layers for the current view"));
	connect(m_showAllLayersAct, SIGNAL(triggered()), this, SLOT(showAllLayers()));

	m_hideAllLayersAct = new QAction(tr("&Hide All Layers"), this);
	m_hideAllLayersAct->setStatusTip(tr("Hide all the layers of the current view"));
	connect(m_hideAllLayersAct, SIGNAL(triggered()), this, SLOT(hideAllLayers()));

	m_showPartLabelAct = new QAction(tr("&Show part label"), this);
	m_showPartLabelAct->setStatusTip(tr("Show or hide the label for the selected parts"));
	m_showPartLabelAct->setCheckable(true);
	connect(m_showPartLabelAct, SIGNAL(triggered()), this, SLOT(showPartLabels()));

	m_loadBundledPart = new QAction(tr("&Import..."), this);
	m_loadBundledPart->setStatusTip(tr("Import a part"));
	connect(m_loadBundledPart, SIGNAL(triggered()), this, SLOT(loadBundledPart()));

	m_saveBundledPart = new QAction(tr("&Export..."), this);
	m_saveBundledPart->setStatusTip(tr("Export selected part"));
	connect(m_saveBundledPart, SIGNAL(triggered()), this, SLOT(saveBundledPart()));

	m_addBendpointAct = new BendpointAction(tr("Add Bendpoint"), this);
	m_addBendpointAct->setStatusTip(tr("Add a bendpoint to the selected wire"));
	connect(m_addBendpointAct, SIGNAL(triggered()), this, SLOT(addBendpoint()));

	m_flattenCurveAct = new BendpointAction(tr("Straighten Curve"), this);
	m_flattenCurveAct->setStatusTip(tr("Straighten the curve of the selected wire"));
	connect(m_flattenCurveAct, SIGNAL(triggered()), this, SLOT(flattenCurve()));

    m_selectAllObsoleteAct = new QAction(tr("Select outdated parts"), this);
    m_selectAllObsoleteAct->setStatusTip(tr("Select outdated parts"));
	connect(m_selectAllObsoleteAct, SIGNAL(triggered()), this, SLOT(selectAllObsolete()));

    m_swapObsoleteAct = new QAction(tr("Update selected parts"), this);
    m_swapObsoleteAct->setStatusTip(tr("Update selected parts"));
	connect(m_swapObsoleteAct, SIGNAL(triggered()), this, SLOT(swapObsolete()));

    m_openProgramWindowAct = new QAction(tr("Open programming window"), this);
    m_openProgramWindowAct->setStatusTip(tr("Open microcontroller programming window"));
	connect(m_openProgramWindowAct, SIGNAL(triggered()), this, SLOT(openProgramWindow()));
}

void MainWindow::createViewMenuActions() {
	m_zoomInAct = new QAction(tr("&Zoom In"), this);
	m_zoomInAct->setShortcut(tr("Ctrl++"));
	m_zoomInAct->setStatusTip(tr("Zoom in"));
	connect(m_zoomInAct, SIGNAL(triggered()), this, SLOT(zoomIn()));

	// instead of creating a filter to grab the shortcut, let's create a new action
	// and append it to the window
	QAction *zoomInAux = new QAction(this);
	zoomInAux->setShortcut(tr("Ctrl+="));
	connect(zoomInAux, SIGNAL(triggered()), this, SLOT(zoomIn()));
	this->addAction(zoomInAux);

	m_zoomOutAct = new QAction(tr("&Zoom Out"), this);
	m_zoomOutAct->setShortcut(tr("Ctrl+-"));
	m_zoomOutAct->setStatusTip(tr("Zoom out"));
	connect(m_zoomOutAct, SIGNAL(triggered()), this, SLOT(zoomOut()));

	m_fitInWindowAct = new QAction(tr("&Fit in Window"), this);
	m_fitInWindowAct->setShortcut(tr("Ctrl+0"));
	m_fitInWindowAct->setStatusTip(tr("Fit in window"));
	connect(m_fitInWindowAct, SIGNAL(triggered()), this, SLOT(fitInWindow()));

	m_actualSizeAct = new QAction(tr("&Actual Size"), this);
	m_actualSizeAct->setStatusTip(tr("Actual (real world physical) size"));
	connect(m_actualSizeAct, SIGNAL(triggered()), this, SLOT(actualSize()));

	m_100PercentSizeAct = new QAction(tr("100% Size"), this);
	m_100PercentSizeAct->setShortcut(tr("Shift+Ctrl+0"));
	m_100PercentSizeAct->setStatusTip(tr("100% (pixel) size"));
	connect(m_100PercentSizeAct, SIGNAL(triggered()), this, SLOT(hundredPercentSize()));

	m_alignToGridAct = new QAction(tr("Align to Grid"), this);
	m_alignToGridAct->setStatusTip(tr("Align items to grid when dragging"));
	m_alignToGridAct->setCheckable(true);
	connect(m_alignToGridAct, SIGNAL(triggered()), this, SLOT(alignToGrid()));

	m_showGridAct = new QAction(tr("Show Grid"), this);
	m_showGridAct->setStatusTip(tr("Show the grid"));
	m_showGridAct->setCheckable(true);
	connect(m_showGridAct, SIGNAL(triggered()), this, SLOT(showGrid()));

	m_showBreadboardAct = new QAction(tr("&Show Breadboard"), this);
	m_showBreadboardAct->setShortcut(tr("Ctrl+1"));
	m_showBreadboardAct->setStatusTip(tr("Show the breadboard view"));
	connect(m_showBreadboardAct, SIGNAL(triggered()), this, SLOT(showBreadboardView()));

	m_showSchematicAct = new QAction(tr("&Show Schematic"), this);
	m_showSchematicAct->setShortcut(tr("Ctrl+2"));
	m_showSchematicAct->setStatusTip(tr("Show the schematic view"));
	connect(m_showSchematicAct, SIGNAL(triggered()), this, SLOT(showSchematicView()));

	m_showPCBAct = new QAction(tr("&Show PCB"), this);
	m_showPCBAct->setShortcut(tr("Ctrl+3"));
	m_showPCBAct->setStatusTip(tr("Show the PCB view"));
	connect(m_showPCBAct, SIGNAL(triggered()), this, SLOT(showPCBView()));

	m_showPartsBinIconViewAct = new QAction(tr("Show Parts Bin Icon View"), this);
	m_showPartsBinIconViewAct->setStatusTip(tr("Display the parts bin in an icon view"));
	connect(m_showPartsBinIconViewAct, SIGNAL(triggered()), this, SLOT(showPartsBinIconView()));

	m_showPartsBinListViewAct = new QAction(tr("Show Parts Bin List View"), this);
	m_showPartsBinListViewAct->setStatusTip(tr("Display the parts bin in a list view"));
	connect(m_showPartsBinListViewAct, SIGNAL(triggered()), this, SLOT(showPartsBinListView()));

}

void MainWindow::createWindowMenuActions() {
	m_minimizeAct = new QAction(tr("&Minimize"), this);
	m_minimizeAct->setShortcut(tr("Ctrl+M"));
	m_minimizeAct->setStatusTip(tr("Minimize current window"));
	connect(m_minimizeAct, SIGNAL(triggered(bool)), this, SLOT(minimize()));

	/*
	m_toggleToolbarAct = new QAction(tr("&Toolbar"), this);
	m_toggleToolbarAct->setShortcut(tr("Shift+Ctrl+T"));
	m_toggleToolbarAct->setCheckable(true);
	m_toggleToolbarAct->setChecked(true);
	m_toggleToolbarAct->setStatusTip(tr("Toggle Toolbar visibility"));
	connect(m_toggleToolbarAct, SIGNAL(triggered(bool)), this, SLOT(toggleToolbar(bool)));
	*/

    m_toggleDebuggerOutputAct = new QAction(tr("Debugger Output"), this);
    m_toggleDebuggerOutputAct->setCheckable(true);
   	connect(m_toggleDebuggerOutputAct, SIGNAL(triggered(bool)), this, SLOT(toggleDebuggerOutput(bool)));
}

void MainWindow::createHelpMenuActions() {
	m_openHelpAct = new QAction(tr("Online Tutorials"), this);
	m_openHelpAct->setShortcut(tr("Ctrl+?"));
	m_openHelpAct->setStatusTip(tr("Open Fritzing help"));
	connect(m_openHelpAct, SIGNAL(triggered(bool)), this, SLOT(openHelp()));

	m_openDonateAct = new QAction(tr("Donate to Fritzing"), this);
	m_openDonateAct->setStatusTip(tr("Open Fritzing donation web page"));
	connect(m_openDonateAct, SIGNAL(triggered(bool)), this, SLOT(openDonate()));

	m_examplesAct = new QAction(tr("Online Projects Gallery"), this);
	m_examplesAct->setStatusTip(tr("Open Fritzing examples"));
	connect(m_examplesAct, SIGNAL(triggered(bool)), this, SLOT(openExamples()));

	m_partsRefAct = new QAction(tr("Online Parts Reference"), this);
	m_partsRefAct->setStatusTip(tr("Open Parts Reference"));
	connect(m_partsRefAct, SIGNAL(triggered(bool)), this, SLOT(openPartsReference()));

	m_showInViewHelpAct = new QAction(tr("First Time Help"), this);
	m_showInViewHelpAct->setStatusTip(tr("Show or Hide First Time Help"));
	m_showInViewHelpAct->setCheckable(true);
	m_showInViewHelpAct->setChecked(true);
	connect(m_showInViewHelpAct, SIGNAL(triggered(bool)), this, SLOT(showInViewHelp()));

	/*m_visitFritzingDotOrgAct = new QAction(tr("Visit fritzing.org"), this);
	m_visitFritzingDotOrgAct->setStatusTip(tr("www.fritzing.org"));
	connect(m_visitFritzingDotOrgAct, SIGNAL(triggered(bool)), this, SLOT(visitFritzingDotOrg()));*/

	m_checkForUpdatesAct = new QAction(tr("Check for updates..."), this);
	m_checkForUpdatesAct->setStatusTip(tr("Check whether a newer version of Fritzing is available for download"));
	connect(m_checkForUpdatesAct, SIGNAL(triggered()), QApplication::instance(), SLOT(checkForUpdates()));

	m_aboutAct = new QAction(tr("&About"), this);
	m_aboutAct->setStatusTip(tr("Show the application's about box"));
	connect(m_aboutAct, SIGNAL(triggered()), this, SLOT(about()));
	m_aboutAct->setMenuRole(QAction::AboutRole);

	m_tipsAndTricksAct = new QAction(tr("Tips, Tricks and Shortcuts"), this);
	m_tipsAndTricksAct->setStatusTip(tr("Display some handy Fritzing tips and tricks"));
	connect(m_tipsAndTricksAct, SIGNAL(triggered()), this, SLOT(tipsAndTricks()));

	m_aboutQtAct = new QAction(tr("&About Qt"), this);
	m_aboutQtAct->setStatusTip(tr("Show Qt's about box"));
	connect(m_aboutQtAct, SIGNAL(triggered()), qApp, SLOT(aboutQt()));

	m_reportBugAct = new QAction(tr("Report a bug..."), this);
	m_reportBugAct->setStatusTip(tr("Report a but you've found in Fritzing"));
	connect(m_reportBugAct, SIGNAL(triggered()), this, SLOT(reportBug()));

	m_enableDebugAct = new QAction(tr("Enable debugging log"), this);
	m_enableDebugAct->setStatusTip(tr("Report a but you've found in Fritzing"));
	m_enableDebugAct->setCheckable(true);
	m_enableDebugAct->setChecked(DebugDialog::enabled());
	connect(m_enableDebugAct, SIGNAL(triggered()), this, SLOT(enableDebug()));

	m_importFilesFromPrevInstallAct = new QAction(tr("&Import parts and bins from old version..."), this);
	m_importFilesFromPrevInstallAct->setStatusTip(tr("Import parts and bins from previous installation"));
	connect(m_importFilesFromPrevInstallAct, SIGNAL(triggered()), this, SLOT(importFilesFromPrevInstall()));
}

void MainWindow::createMenus()
{
    m_fileMenu = menuBar()->addMenu(tr("&File"));
    m_fileMenu->addAction(m_newAct);
    m_fileMenu->addAction(m_openAct);
    m_fileMenu->addMenu(m_openRecentFileMenu);
    m_fileMenu->addMenu(m_openExampleMenu);

    m_fileMenu->addSeparator();
    m_fileMenu->addAction(m_closeAct);
    m_fileMenu->addAction(m_saveAct);
    m_fileMenu->addAction(m_saveAsAct);
    m_fileMenu->addAction(m_shareOnlineAct);

	if (m_orderFabEnabled) {
		m_fileMenu->addAction(m_orderFabAct);
	}

    m_fileMenu->addSeparator();
	m_exportMenu = m_fileMenu->addMenu(tr("&Export"));
    //m_fileMenu->addAction(m_pageSetupAct);
    m_fileMenu->addAction(m_printAct);

	QString name;
	QString path;
	QStringList args;
	if (externalProcess(name, path, args)) {
		m_fileMenu->addSeparator();
		m_fileMenu->addAction(m_launchExternalProcessAct);
	}

#ifndef QT_NO_DEBUG
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(m_exceptionAct);
#endif

	m_fileMenu->addSeparator();
	m_fileMenu->addAction(m_quitAct);
    connect(m_fileMenu, SIGNAL(aboutToShow()), this, SLOT(updateFileMenu()));

	QMenu * imageMenu = m_exportMenu->addMenu(tr("as Image"));
	imageMenu->addAction(m_exportPngAct);
	imageMenu->addAction(m_exportJpgAct);
	imageMenu->addSeparator();
	imageMenu->addAction(m_exportSvgAct);
	imageMenu->addAction(m_exportPdfAct);
	imageMenu->addAction(m_exportPsAct);

	QMenu * productionMenu = m_exportMenu->addMenu(tr("for Production"));
	productionMenu->addAction(m_exportEtchablePdfAct);
	productionMenu->addAction(m_exportEtchableSvgAct);
	productionMenu->addAction(m_exportEtchablePdfFlipAct);
	productionMenu->addAction(m_exportEtchableSvgFlipAct);
	productionMenu->addSeparator();
	productionMenu->addAction(m_exportGerberAct);

	m_exportMenu->addAction(m_exportBomAct);
	m_exportMenu->addAction(m_exportNetlistAct);

	//m_exportMenu->addAction(m_exportEagleAct);

    m_editMenu = menuBar()->addMenu(tr("&Edit"));
    m_editMenu->addAction(m_undoAct);
    m_editMenu->addAction(m_redoAct);
    m_editMenu->addSeparator();
    m_editMenu->addAction(m_cutAct);
    m_editMenu->addAction(m_copyAct);
    m_editMenu->addAction(m_pasteAct);
    m_editMenu->addAction(m_pasteInPlaceAct);
    m_editMenu->addAction(m_duplicateAct);
    m_editMenu->addAction(m_deleteAct);
    m_editMenu->addSeparator();
    m_editMenu->addAction(m_selectAllAct);
    m_editMenu->addAction(m_deselectAct);
    m_editMenu->addSeparator();
    m_editMenu->addAction(m_addNoteAct);
    m_editMenu->addSeparator();
    m_editMenu->addAction(m_preferencesAct);
    updateEditMenu();
    connect(m_editMenu, SIGNAL(aboutToShow()), this, SLOT(updateEditMenu()));

    m_partMenu = menuBar()->addMenu(tr("&Part"));
    connect(m_partMenu, SIGNAL(aboutToShow()), this, SLOT(updatePartMenu()));

    m_partMenu->addAction(m_createNewPart);
	m_partMenu->addAction(m_loadBundledPart);
	m_partMenu->addSeparator();
	m_partMenu->addAction(m_openInPartsEditorAct);
	m_partMenu->addAction(m_saveBundledPart);

	m_partMenu->addSeparator();
	m_partMenu->addAction(m_flipHorizontalAct);
	m_partMenu->addAction(m_flipVerticalAct);
	m_rotateMenu = m_partMenu->addMenu(tr("Rotate"));
	m_zOrderMenu = m_partMenu->addMenu(tr("Raise and Lower"));
	m_zOrderWireMenu = new QMenu(m_zOrderMenu);
	m_zOrderWireMenu->setTitle(m_zOrderMenu->title());
	m_partMenu->addAction(m_moveLockAct);
	m_partMenu->addAction(m_selectMoveLockAct);
	
	m_partMenu->addSeparator();
	m_partMenu->addMenu(m_addToBinMenu);
	m_partMenu->addAction(m_showPartLabelAct);
	m_partMenu->addSeparator();
	m_partMenu->addAction(m_selectAllObsoleteAct);
	m_partMenu->addAction(m_swapObsoleteAct);

	m_rotateMenu->addAction(m_rotate45cwAct);
	m_rotateMenu->addAction(m_rotate90cwAct);
	m_rotateMenu->addAction(m_rotate180Act);
	m_rotateMenu->addAction(m_rotate90ccwAct);
	m_rotateMenu->addAction(m_rotate45ccwAct);

	m_zOrderMenu->addAction(m_bringToFrontAct);
	m_zOrderMenu->addAction(m_bringForwardAct);
	m_zOrderMenu->addAction(m_sendBackwardAct);
	m_zOrderMenu->addAction(m_sendToBackAct);

	m_zOrderWireMenu->addAction(m_bringToFrontWireAct);
	m_zOrderWireMenu->addAction(m_bringForwardWireAct);
	m_zOrderWireMenu->addAction(m_sendBackwardWireAct);
	m_zOrderWireMenu->addAction(m_sendToBackWireAct);


    m_viewMenu = menuBar()->addMenu(tr("&View"));
    m_viewMenu->addAction(m_zoomInAct);
    m_viewMenu->addAction(m_zoomOutAct);
    m_viewMenu->addAction(m_fitInWindowAct);
    m_viewMenu->addAction(m_actualSizeAct);
    m_viewMenu->addAction(m_100PercentSizeAct);
	m_viewMenu->addSeparator();

    m_viewMenu->addAction(m_alignToGridAct);
    m_viewMenu->addAction(m_showGridAct);
	m_viewMenu->addSeparator();

    m_viewMenu->addAction(m_showBreadboardAct);
    m_viewMenu->addAction(m_showSchematicAct);
    m_viewMenu->addAction(m_showPCBAct);
    m_viewMenu->addSeparator();
    m_viewMenu->addAction(m_showPartsBinIconViewAct);
    m_viewMenu->addAction(m_showPartsBinListViewAct);
    m_viewMenu->addSeparator();
    connect(m_viewMenu, SIGNAL(aboutToShow()), this, SLOT(updateLayerMenu()));
    m_numFixedActionsInViewMenu = m_viewMenu->actions().size();

    m_windowMenu = menuBar()->addMenu(tr("&Window"));
	m_windowMenu->addAction(m_minimizeAct);
	m_windowMenu->addSeparator();
	//m_windowMenu->addAction(m_toggleToolbarAct);
	updateWindowMenu();
	connect(m_windowMenu, SIGNAL(aboutToShow()), this, SLOT(updateWindowMenu()));

	m_pcbTraceMenu = menuBar()->addMenu(tr("&Routing"));
	m_pcbTraceMenu->addAction(m_autorouteAct);
	m_pcbTraceMenu->addAction(m_designRulesCheckAct);
	m_pcbTraceMenu->addAction(m_autorouterSettingsAct);

	QMenu * groundFillMenu = m_pcbTraceMenu->addMenu(tr("Ground Fill"));

	groundFillMenu->addAction(m_copperFillAct);
	groundFillMenu->addAction(m_groundFillAct);
	groundFillMenu->addAction(m_removeGroundFillAct);
	groundFillMenu->addAction(m_setGroundFillSeedsAct);
	groundFillMenu->addAction(m_clearGroundFillSeedsAct);
	//m_pcbTraceMenu->addAction(m_updateRoutingStatusAct);
	m_pcbTraceMenu->addSeparator();

	m_pcbTraceMenu->addAction(m_activeLayerBothAct);
	m_pcbTraceMenu->addAction(m_activeLayerBottomAct);
	m_pcbTraceMenu->addAction(m_activeLayerTopAct);
	m_pcbTraceMenu->addSeparator();

	m_pcbTraceMenu->addAction(m_changeTraceLayerAct);
	m_pcbTraceMenu->addAction(m_excludeFromAutorouteAct);
	m_pcbTraceMenu->addSeparator();

	m_pcbTraceMenu->addAction(m_selectAllTracesAct);
	m_pcbTraceMenu->addAction(m_selectAllExcludedTracesAct);
	m_pcbTraceMenu->addAction(m_selectAllIncludedTracesAct);
	m_pcbTraceMenu->addAction(m_selectAllJumperItemsAct);
	m_pcbTraceMenu->addAction(m_selectAllViasAct);
	m_pcbTraceMenu->addAction(m_selectAllCopperFillAct);

	m_schematicTraceMenu = menuBar()->addMenu(tr("&Routing"));
	m_schematicTraceMenu->addAction(m_autorouteAct);
	m_schematicTraceMenu->addAction(m_excludeFromAutorouteAct);
	m_schematicTraceMenu->addAction(m_selectAllTracesAct);
	m_schematicTraceMenu->addAction(m_selectAllExcludedTracesAct);
	m_schematicTraceMenu->addAction(m_selectAllIncludedTracesAct);
	//m_schematicTraceMenu->addAction(m_updateRoutingStatusAct);

#ifndef QT_NO_DEBUG
	m_schematicTraceMenu->addAction(m_tidyWiresAct);
#endif

	m_breadboardTraceMenu = menuBar()->addMenu(tr("&Routing"));
	m_breadboardTraceMenu->addAction(m_selectAllWiresAct);

	updateTraceMenu();
	connect(m_pcbTraceMenu, SIGNAL(aboutToShow()), this, SLOT(updateTraceMenu()));
	connect(m_schematicTraceMenu, SIGNAL(aboutToShow()), this, SLOT(updateTraceMenu()));
	connect(m_breadboardTraceMenu, SIGNAL(aboutToShow()), this, SLOT(updateTraceMenu()));


    menuBar()->addSeparator();

    m_helpMenu = menuBar()->addMenu(tr("&Help"));
    m_helpMenu->addAction(m_showInViewHelpAct);
    m_helpMenu->addAction(m_openHelpAct);
    m_helpMenu->addAction(m_examplesAct);
    m_helpMenu->addAction(m_partsRefAct);
	m_helpMenu->addSeparator();
	m_helpMenu->addAction(m_checkForUpdatesAct);
	m_helpMenu->addAction(m_importFilesFromPrevInstallAct);
	m_helpMenu->addSeparator();
	m_helpMenu->addAction(m_reportBugAct);
	m_helpMenu->addAction(m_enableDebugAct);
	m_helpMenu->addSeparator();
	m_helpMenu->addAction(m_aboutAct);
    m_helpMenu->addAction(m_openDonateAct);
	m_helpMenu->addAction(m_tipsAndTricksAct);
#ifndef QT_NO_DEBUG
	m_helpMenu->addAction(m_aboutQtAct);
#endif
}

void MainWindow::updateLayerMenu(bool resetLayout) {

	QList<QAction *> actions;

	if (m_binManager) {
		m_showPartsBinIconViewAct->setEnabled(true);
		m_showPartsBinListViewAct->setEnabled(true);
		actions << m_showPartsBinIconViewAct << m_showPartsBinListViewAct;
		setActionsIcons(m_binManager->currentViewIsIconView() ? 0 : 1, actions);
	}
	else {
		m_showPartsBinIconViewAct->setEnabled(false);
		m_showPartsBinListViewAct->setEnabled(false);
	}

	removeActionsStartingAt(m_viewMenu, m_numFixedActionsInViewMenu);
    m_viewMenu->addAction(m_showAllLayersAct);
    m_viewMenu->addAction(m_hideAllLayersAct);

	if (m_currentGraphicsView == NULL) return;

	m_alignToGridAct->setChecked(m_currentGraphicsView->alignedToGrid());
	m_showGridAct->setChecked(m_currentGraphicsView->showingGrid());
	
	LayerHash viewLayers = m_currentGraphicsView->viewLayers();
	LayerList keys = viewLayers.keys();

	// make sure they're in ascending order when inserting into the menu
	qSort(keys.begin(), keys.end());

	foreach (ViewLayer::ViewLayerID key, keys) {
		ViewLayer * viewLayer = viewLayers.value(key);
		//DebugDialog::debug(QString("Layer: %1 is %2").arg(viewLayer->action()->text()).arg(viewLayer->action()->isEnabled()));
    	if (viewLayer != NULL) {
			if (viewLayer->parentLayer()) continue;
			m_viewMenu->addAction(viewLayer->action());
			disconnect(viewLayer->action(), SIGNAL(triggered()), this, SLOT(updateLayerMenu()));
			connect(viewLayer->action(), SIGNAL(triggered()), this, SLOT(updateLayerMenu()));
		}
	}

	m_hideAllLayersAct->setEnabled(false);
	m_showAllLayersAct->setEnabled(false);

	if (keys.count() <= 0) return;

	ViewLayer *prev = viewLayers.value(keys[0]);
	if (prev == NULL) {
		// jrc: I think prev == NULL is actually a side effect from an earlier bug
		// but I haven't figured out the cause yet
		// at any rate, when this bug occurs, keys[0] is some big negative number that looks like an
		// uninitialized or scrambled layerID
		DebugDialog::debug(QString("updateAllLayersActions keys[0] failed %1").arg(keys[0]) );
		return;
	}

	bool sameState = prev->action()->isChecked();
	bool checked = prev->action()->isChecked();
	//DebugDialog::debug(QString("Layer: %1 is %2").arg(prev->action()->text()).arg(prev->action()->isChecked()));
	for (int i = 1; i < keys.count(); i++) {
		ViewLayer *viewLayer = viewLayers.value(keys[i]);
		//DebugDialog::debug(QString("Layer: %1 is %2").arg(viewLayer->action()->text()).arg(viewLayer->action()->isChecked()));
		if (viewLayer != NULL) {
			if (prev != NULL && prev->action()->isChecked() != viewLayer->action()->isChecked() ) {
				// if the actions aren't all checked or unchecked I don't bother about the "checked" variable
				sameState = false;
				break;
			} 
			else {
				sameState = true;
				checked = viewLayer->action()->isChecked();
			}
			prev = viewLayer;
		}
	}

	//DebugDialog::debug(QString("sameState: %1").arg(sameState));
	//DebugDialog::debug(QString("checked: %1").arg(checked));
	if (sameState) {
		if(checked) {
			m_hideAllLayersAct->setEnabled(true);
		} 
		else {
			m_showAllLayersAct->setEnabled(true);
		}
	} 
	else {
		m_showAllLayersAct->setEnabled(true);
		m_hideAllLayersAct->setEnabled(true);
	}

	if (resetLayout) {
		m_layerPalette->resetLayout(viewLayers, keys);
	}
	m_layerPalette->updateLayerPalette(viewLayers, keys);
}

void MainWindow::updateWireMenu() {
	// assumes update wire menu is only called when right-clicking a wire
	// and that wire is cached by the menu in Wire::mousePressEvent

	Wire * wire = m_activeWire;
	m_activeWire = NULL;

	if (wire) {
		enableAddBendpointAct(wire);
	}

	bool enableAll = true;
	bool deleteOK = false;
	bool createTraceOK = false;
	bool excludeOK = false;
	bool enableZOK = true;
	bool gotRat = false;
	bool ctlOK = false;

	if (wire != NULL) {
		
		if (wire->getRatsnest()) {
			QList<ConnectorItem *> ends;
			Wire * jt = wire->findTraced(m_currentGraphicsView->getTraceFlag(), ends);
			createTraceOK = (jt == NULL) || (!jt->getTrace());
			deleteOK = true;
			gotRat = true;
			enableZOK = false;
		}
		else if (wire->getTrace()) {
			deleteOK = true;
			excludeOK = true;
			m_excludeFromAutorouteWireAct->setChecked(!wire->getAutoroutable());
			if (m_currentGraphicsView == m_pcbGraphicsView && m_currentGraphicsView->boardLayers() > 1) {
				if (wire->canSwitchLayers()) {
					ctlOK = true;
				}
			}

		}
		else {
			deleteOK = true;
		}
	}

	if (wire) {
		m_wireColorMenu->setEnabled(true);
		foreach (QAction * action, m_wireColorMenu->actions()) {
			QString colorName = action->data().toString();
			action->setChecked(colorName.compare(wire->colorString()) == 0);
		}
	}
	else {
		m_wireColorMenu->setEnabled(false);
	}

	m_bringToFrontWireAct->setWire(wire);
	m_bringForwardWireAct->setWire(wire);
	m_sendBackwardWireAct->setWire(wire);
	m_sendToBackWireAct->setWire(wire);
	m_createTraceWireAct->setWire(wire);
	m_createWireWireAct->setWire(wire);
	m_deleteWireAct->setWire(wire);
	m_excludeFromAutorouteWireAct->setWire(wire);

	m_bringToFrontWireAct->setEnabled(enableZOK);
	m_bringForwardWireAct->setEnabled(enableZOK);
	m_sendBackwardWireAct->setEnabled(enableZOK);
	m_sendToBackWireAct->setEnabled(enableZOK);
	m_createTraceWireAct->setEnabled(enableAll && createTraceOK);
	m_createWireWireAct->setEnabled(enableAll && createTraceOK);
	m_deleteWireAct->setEnabled(enableAll && deleteOK);
	m_excludeFromAutorouteWireAct->setEnabled(enableAll && excludeOK);

	m_changeTraceLayerAct->setEnabled(ctlOK);

	if (gotRat) {
		m_deleteWireAct->setText(tr("Delete Ratsnest Line"));
	}
	else {
		m_deleteWireAct->setText(tr("Delete Wire"));
	}


}

void MainWindow::updatePartMenu() {
	if (m_currentGraphicsView == NULL) return;

	ItemCount itemCount = m_currentGraphicsView->calcItemCount();

	bool enable = true;
	bool zenable = true;

	if (itemCount.selCount <= 0) {
		zenable = enable = false;
	}
	else {
		if (itemCount.itemsCount == itemCount.selCount) {
			// if all items are selected
			// z-reordering is a no-op
			zenable = false;
		}
	}

	//DebugDialog::debug(QString("enable layer actions %1").arg(enable));
	m_bringToFrontAct->setEnabled(zenable);
	m_bringForwardAct->setEnabled(zenable);
	m_sendBackwardAct->setEnabled(zenable);
	m_sendToBackAct->setEnabled(zenable);

	m_moveLockAct->setEnabled(itemCount.selCount > 0 && itemCount.selCount > itemCount.wireCount);
	m_moveLockAct->setChecked(itemCount.moveLockCount > 0);
	m_selectMoveLockAct->setEnabled(true);

	m_showPartLabelAct->setEnabled((itemCount.hasLabelCount > 0) && enable);
	m_showPartLabelAct->setChecked(itemCount.visLabelCount == itemCount.hasLabelCount);

	bool renable = (itemCount.selRotatable > 0);
	bool renable45 = (itemCount.sel45Rotatable > 0);

	//DebugDialog::debug(QString("enable rotate (2) %1").arg(enable));

	m_rotate90cwAct->setEnabled(renable && enable);
	m_rotate180Act->setEnabled(renable && enable);
	m_rotate90ccwAct->setEnabled(renable && enable);
	m_rotate45ccwAct->setEnabled(renable && renable45 && enable && m_currentGraphicsView != m_schematicGraphicsView);
	m_rotate45cwAct->setEnabled(renable && renable45 && enable && m_currentGraphicsView != m_schematicGraphicsView);

	m_flipHorizontalAct->setEnabled(enable && (itemCount.selHFlipable > 0) && (m_currentGraphicsView != m_pcbGraphicsView));
	m_flipVerticalAct->setEnabled(enable && (itemCount.selVFlipable > 0) && (m_currentGraphicsView != m_pcbGraphicsView));

	updateItemMenu();
	updateEditMenu();

	if (itemCount.selCount == 1) {
		enableAddBendpointAct(m_currentGraphicsView->scene()->selectedItems()[0]);
	}

	// TODO: only enable if there is an obsolete part in the sketch
	m_selectAllObsoleteAct->setEnabled(true);
	m_swapObsoleteAct->setEnabled(itemCount.obsoleteCount > 0);
	m_openProgramWindowAct->setEnabled(true);
}

void MainWindow::updateTransformationActions() {
	if (m_currentGraphicsView == NULL) return;

	ItemCount itemCount = m_currentGraphicsView->calcItemCount();
	bool enable = (itemCount.selRotatable > 0);

	//DebugDialog::debug(QString("enable rotate (1) %1").arg(enable));

	m_rotate90cwAct->setEnabled(enable);
	m_rotate180Act->setEnabled(enable);
	m_rotate90ccwAct->setEnabled(enable);
	m_rotate45ccwAct->setEnabled(enable);
	foreach(SketchToolButton* rotateButton, m_rotateButtons) {
		rotateButton->setEnabled(enable);
	}

	m_flipHorizontalAct->setEnabled((itemCount.selHFlipable > 0) && (m_currentGraphicsView != m_pcbGraphicsView));
	m_flipVerticalAct->setEnabled((itemCount.selVFlipable > 0) && (m_currentGraphicsView != m_pcbGraphicsView));

	enable = m_flipHorizontalAct->isEnabled() || m_flipVerticalAct->isEnabled();
	foreach(SketchToolButton* flipButton, m_flipButtons) {
		flipButton->setEnabled(enable);
	}
}

void MainWindow::updateItemMenu() {
	if (m_currentGraphicsView == NULL) return;

	ConnectorItem * activeConnectorItem = m_activeConnectorItem;
	m_activeConnectorItem = NULL;

	QList<QGraphicsItem *> items = m_currentGraphicsView->scene()->selectedItems();

	int selCount = 0;
	ItemBase * itemBase = NULL;
	foreach(QGraphicsItem * item, items) {
		ItemBase * ib = ItemBase::extractTopLevelItemBase(item);
		if (ib == NULL) continue;

		selCount++;
		if (selCount == 1) itemBase = ib;
		else if (selCount > 1) break;
	}

	PaletteItem *selected = qobject_cast<PaletteItem *>(itemBase);
	bool enabled = (selCount == 1) && (selected != NULL);
	m_addToBinMenu->setEnabled(enabled);
	m_addToBinMenu->clear();
	if(enabled) {
		QList<QAction*> acts = m_binManager->openedBinsActions(selectedModuleID());
		m_addToBinMenu->addActions(acts);
	}

	m_saveBundledPart->setEnabled(enabled && !selected->modelPart()->isCore());


	// can't open wire in parts editor
	enabled &= selected != NULL && itemBase != NULL && itemBase->canEditPart();
	m_openInPartsEditorAct->setEnabled(enabled);

	m_disconnectAllAct->setEnabled(enabled && m_currentGraphicsView->canDisconnectAll() && (itemBase->rightClickedConnector() != NULL));

	bool gfsEnabled = false;
	if (activeConnectorItem) {
		if (activeConnectorItem->attachedToItemType() != ModelPart::CopperFill) {
			gfsEnabled = true;
			m_setOneGroundFillSeedAct->setChecked(activeConnectorItem->isGroundFillSeed());
		}
	}
	m_setOneGroundFillSeedAct->setEnabled(gfsEnabled);
	m_setOneGroundFillSeedAct->setConnectorItem(activeConnectorItem);
}

void MainWindow::updateEditMenu() {
	QClipboard *clipboard = QApplication::clipboard();
	m_pasteAct->setEnabled(false);
	m_pasteInPlaceAct->setEnabled(false);
	if (clipboard != NULL) {
		const QMimeData *mimeData = clipboard->mimeData(QClipboard::Clipboard);
		if (mimeData != NULL) {
			if (mimeData->hasFormat("application/x-dnditemsdata")) {
				m_pasteAct->setEnabled(true);
				m_pasteInPlaceAct->setEnabled(true);
				//DebugDialog::debug(QString("paste enabled: true"));
			}
		}
	}

	if (m_currentGraphicsView != NULL) {
		const QList<QGraphicsItem *> items =  m_currentGraphicsView->scene()->selectedItems();
		bool copyActsEnabled = false;
		bool deleteActsEnabled = false;
		foreach (QGraphicsItem * item, items) {
			if (m_currentGraphicsView->canDeleteItem(item, items.count())) {
				deleteActsEnabled = true;
			}
			if (m_currentGraphicsView->canCopyItem(item, items.count())) {
				copyActsEnabled = true;
			}
		}

		//DebugDialog::debug(QString("enable cut/copy/duplicate/delete %1 %2 %3").arg(copyActsEnabled).arg(deleteActsEnabled).arg(m_currentWidget->viewIdentifier()) );
		m_deleteAct->setEnabled(deleteActsEnabled);
		m_deleteAct->setText(tr("Delete"));
		m_cutAct->setEnabled(deleteActsEnabled && copyActsEnabled);
		m_copyAct->setEnabled(copyActsEnabled);
		m_duplicateAct->setEnabled(copyActsEnabled);
	}
}

void MainWindow::updateTraceMenu() {
	bool jiEnabled = false;
	bool viaEnabled = false;
	bool tEnabled = false;
	bool exEnabled = false;
	bool exChecked = true;
	bool twEnabled = false;
	bool gfEnabled = false;
	bool gfrEnabled = false;
	bool ctlEnabled = false;
	bool arEnabled = false;
	bool gfsEnabled = false;

	if (m_currentGraphicsView != NULL) {
		QList<QGraphicsItem *> items = m_currentGraphicsView->scene()->items();
		foreach (QGraphicsItem * item, items) {
			Wire * wire = dynamic_cast<Wire *>(item);
			if (wire == NULL) {
				if (m_currentGraphicsView != m_pcbGraphicsView) continue;

				ItemBase * itemBase = dynamic_cast<ItemBase *>(item);
				if (itemBase == NULL) continue;
				if (!itemBase->isEverVisible()) continue;
				

				if (!gfsEnabled) {
					gfsEnabled = itemBase->itemType() != ModelPart::CopperFill && itemBase->hasConnectors();
				}

				switch (itemBase->itemType()) {
					case ModelPart::Board:
					case ModelPart::ResizableBoard:
						gfEnabled = true;
						break;
					case ModelPart::Jumper:
						jiEnabled = true;
						if (itemBase->isSelected()) {
							exEnabled = true;
							if (qobject_cast<JumperItem *>(itemBase->layerKinChief())->getAutoroutable()) {
								exChecked = false;
							}
						}
						break;
					case ModelPart::Via:
						viaEnabled = true;
						if (itemBase->isSelected()) {
							exEnabled = true;
							if (qobject_cast<Via *>(itemBase->layerKinChief())->getAutoroutable()) {
								exChecked = false;
							}
						}
						break;
					case ModelPart::CopperFill:
						gfrEnabled = true;
					default:
						break;
				}

				continue;
			}

			if (!wire->isEverVisible()) continue;
			if (wire->getRatsnest()) {
				//rEnabled = true;
				//if (wire->isSelected()) {
					//ctEnabled = true;
				//}
			}
			else if (wire->isTraceType(m_currentGraphicsView->getTraceFlag())) {
				arEnabled = true;
				tEnabled = true;
				twEnabled = true;
				if (wire->isSelected()) {
					exEnabled = true;
					if (wire->getAutoroutable()) {
						exChecked = false;
					}
				}
				if (m_currentGraphicsView == m_pcbGraphicsView && m_currentGraphicsView->boardLayers() > 1) {
					if (wire->canSwitchLayers()) {
						ctlEnabled = true;
					}
				}
			}
		}
	}

	if (!arEnabled) {
		if (m_currentGraphicsView != NULL) {
			arEnabled = m_currentGraphicsView->hasAnyNets(); 
		}
	}

	m_excludeFromAutorouteAct->setEnabled(exEnabled);
	m_excludeFromAutorouteAct->setChecked(exChecked);
	m_changeTraceLayerAct->setEnabled(ctlEnabled);
	m_autorouteAct->setEnabled(arEnabled);
	m_orderFabAct->setEnabled(true);
	m_exportEtchablePdfAct->setEnabled(true);
	m_exportEtchablePdfFlipAct->setEnabled(true);
	m_exportEtchableSvgAct->setEnabled(true);
	m_exportEtchableSvgFlipAct->setEnabled(true);
	m_selectAllTracesAct->setEnabled(tEnabled);
	m_selectAllWiresAct->setEnabled(tEnabled);
	m_selectAllCopperFillAct->setEnabled(gfrEnabled);
	m_selectAllExcludedTracesAct->setEnabled(tEnabled);
	m_selectAllIncludedTracesAct->setEnabled(tEnabled);
	m_selectAllJumperItemsAct->setEnabled(jiEnabled);
	m_selectAllViasAct->setEnabled(viaEnabled);
	m_tidyWiresAct->setEnabled(twEnabled);
	m_groundFillAct->setEnabled(gfEnabled);
	m_copperFillAct->setEnabled(gfEnabled);
	m_removeGroundFillAct->setEnabled(gfrEnabled);

	// TODO: set and clear enabler logic
	m_setGroundFillSeedsAct->setEnabled(gfsEnabled);
	m_clearGroundFillSeedsAct->setEnabled(gfsEnabled);

	m_designRulesCheckAct->setEnabled(true);
	m_autorouterSettingsAct->setEnabled(m_currentGraphicsView == m_pcbGraphicsView);
	m_updateRoutingStatusAct->setEnabled(true);
}

void MainWindow::zoomIn() {
	m_zoomSlider->zoomIn();
}

void MainWindow::zoomOut() {
	m_zoomSlider->zoomOut();
}

void MainWindow::fitInWindow() {
	if (m_currentGraphicsView == NULL) return;

	double newZoom = m_currentGraphicsView->fitInWindow();
	m_zoomSlider->setValue(newZoom);
}

void MainWindow::hundredPercentSize() {
	m_currentGraphicsView->absoluteZoom(100);
	m_zoomSlider->setValue(100);
}

void MainWindow::actualSize() {
	QMessageBox::information(this, tr("Actual Size"),
				tr("It doesn't seem to be possible to automatically determine the actual physical size of the monitor, so "
				"'actual size' as currently implemented is only a guess. "
				"Your best bet would be to drag out a ruler part, then place a real (physical) ruler on top and zoom until they match up."
				));


	int dpi = this->physicalDpiX();
	int l = this->logicalDpiX();

	DebugDialog::debug(QString("actual size %1 %2").arg(dpi).arg(l));

	// remember the parameter to the next two functions is a percent
	m_currentGraphicsView->absoluteZoom(dpi * 100.0 / FSvgRenderer::printerScale());
	m_zoomSlider->setValue(dpi * 100.0 / FSvgRenderer::printerScale());
}

void MainWindow::showBreadboardView() {
	this->m_tabWidget->setCurrentIndex(0);
}

void MainWindow::showSchematicView() {
	this->m_tabWidget->setCurrentIndex(1);

}

void MainWindow::showPCBView() {
	this->m_tabWidget->setCurrentIndex(2);
}

void MainWindow::showPartsBinIconView() {
	if (m_binManager) m_binManager->toIconView();
}

void MainWindow::showPartsBinListView() {
	if (m_binManager) m_binManager->toListView();
}

void MainWindow::openHelp() {
	QDesktopServices::openUrl(QString("http://fritzing.org/learning"));
}

void MainWindow::openDonate() {
	QDesktopServices::openUrl(QString("http://fritzing.org/shop/donations/"));
}

void MainWindow::openExamples() {
	QDesktopServices::openUrl(QString("http://fritzing.org/projects"));
}

void MainWindow::openPartsReference() {
	QDesktopServices::openUrl(QString("http://fritzing.org/parts"));
}

void MainWindow::visitFritzingDotOrg() {
	 QDesktopServices::openUrl(QString("http://www.fritzing.org"));
}

void MainWindow::reportBug() {
	 QDesktopServices::openUrl(QString("http://code.google.com/p/fritzing/issues"));
}

void MainWindow::enableDebug() {
	DebugDialog::setEnabled(m_enableDebugAct->isChecked());
	if (!m_windowMenu->actions().contains(m_toggleDebuggerOutputAct)) {
	    m_windowMenu->insertSeparator(m_windowMenuSeparator);
		m_windowMenu->insertAction(m_windowMenuSeparator, m_toggleDebuggerOutputAct);
		toggleDebuggerOutput(true);
	}
}

void MainWindow::createNewPart() {
	openPartsEditor(NULL);
}

void MainWindow::openPartsEditor(PaletteItem * paletteItem) {
	ModelPart* modelPart = paletteItem? paletteItem->modelPart(): NULL;
	long id = paletteItem? paletteItem->id(): -1;
	QWidget *partsEditor = getPartsEditor(modelPart, id, paletteItem, NULL);
	if (partsEditor == NULL) return;

	partsEditor->show();
	partsEditor->raise();
}

PartsEditorMainWindow* MainWindow::getPartsEditor(ModelPart *modelPart, long _id, ItemBase * fromItem, class PartsBinPaletteWidget* requester) {
	QMessageBox::StandardButton answer = QMessageBox::question(
            this,
            tr("Parts Editor"),
            tr("A new Parts Editor is under construction. The old Parts Editor is still available, but the code is pretty buggy. So use it at your own risk.\n\nOpen the old Parts Editor?"),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::Yes
    );
    // TODO: make button texts translatable
    if (answer != QMessageBox::Yes) {
        return NULL;
    }		

	static long nextId = -1;
	long id = _id==-1? nextId--: _id;

	PartsEditorMainWindow *mainPartsEditorWindow = new PartsEditorMainWindow(this);
	if (fromItem != NULL) {
		ItemBase * ii = m_breadboardGraphicsView->addItemAuxTemp(modelPart, fromItem->viewLayerSpec(), ViewGeometry(), ItemBase::getNextID(), NULL, true, ViewIdentifierClass::IconView, true);
		if (ii != NULL) {
			m_breadboardGraphicsView->scene()->removeItem(ii);
			if (!ii->hasCustomSVG()) {
				delete ii;
				ii = NULL;
			}
		}

		ItemBase * bb = m_breadboardGraphicsView->findItem(_id);
		if (bb) bb = bb->layerKinChief();
		if (bb != NULL && !bb->hasCustomSVG()) bb = NULL;
		ItemBase * ss = m_schematicGraphicsView->findItem(_id);
		if (ss) ss = ss->layerKinChief();
		if (ss != NULL && !ss->hasCustomSVG()) ss = NULL;
		ItemBase * pp = m_pcbGraphicsView->findItem(_id);
		if (pp) pp = pp->layerKinChief();
		if (pp != NULL && !pp->hasCustomSVG()) pp = NULL;
		mainPartsEditorWindow->setViewItems(ii, bb, ss, pp);
	}

	mainPartsEditorWindow->setup(id, modelPart, (modelPart!=NULL), fromItem);

	connect(mainPartsEditorWindow, SIGNAL(partUpdated(const QString&, long, bool)), this, SLOT(loadPart(const QString&, long, bool)));
	connect(mainPartsEditorWindow, SIGNAL(closed(long)), this, SLOT(partsEditorClosed(long)));
	connect(mainPartsEditorWindow, SIGNAL(alienPartUsed()), this, SLOT(acceptAlienFiles()));

	connect(this, SIGNAL(aboutToClose()), mainPartsEditorWindow, SLOT(parentAboutToClose()));
	connect(mainPartsEditorWindow, SIGNAL(changeActivationSignal(bool, QWidget *)), qApp, SLOT(changeActivation(bool, QWidget *)), Qt::DirectConnection);
	connect(mainPartsEditorWindow, SIGNAL(destroyed(QObject *)), qApp, SLOT(topLevelWidgetDestroyed(QObject *)));

	m_partsEditorWindows.insert(id, mainPartsEditorWindow);
	if(requester) m_binsWithPartsEditorRequests.insert(id,requester);

	return mainPartsEditorWindow;
}

void MainWindow::partsEditorClosed(long id) {
	m_partsEditorWindows.remove(id);
	m_binsWithPartsEditorRequests.remove(id);
}

void MainWindow::openInPartsEditor() {
	if (m_currentGraphicsView == NULL) return;

	PaletteItem *selectedPart = m_currentGraphicsView->getSelectedPart();
	PartsEditorMainWindow * window = m_partsEditorWindows.value(selectedPart->id());

	if(window != NULL) {
		window->raise();
	} else {
		openPartsEditor(selectedPart);
	}
}

void MainWindow::createNewSketch() {
    MainWindow* mw = newMainWindow(m_paletteModel, m_refModel, "", true, true);
    mw->move(x()+CascadeFactorX,y()+CascadeFactorY);
	ProcessEventBlocker::processEvents();

	mw->addDefaultParts();
    mw->show();
	mw->hideTempBin();

    QSettings settings;
    settings.remove("lastOpenSketch");
    mw->clearFileProgressDialog();
}

void MainWindow::minimize() {
	this->showMinimized();
}

void MainWindow::toggleToolbar(bool toggle) {
	Q_UNUSED(toggle);
	/*if(toggle) {
		this->m_fileToolBar->show();
		this->m_editToolBar->show();
	} else {
		this->m_fileToolBar->hide();
		this->m_editToolBar->hide();
	}*/
}

void MainWindow::togglePartLibrary(bool toggle) {
	if(toggle) {
		m_binManager->show();
	} else {
		m_binManager->hide();
	}
}

void MainWindow::toggleInfo(bool toggle) {
	if(toggle) {
		((QDockWidget*)m_infoView->parent())->show();
	} else {
		((QDockWidget*)m_infoView->parent())->hide();
	}
}

void MainWindow::toggleNavigator(bool toggle) {
	if(toggle) {
		((QDockWidget*)m_miniViewContainerBreadboard->parent())->show();
		((QDockWidget*)m_miniViewContainerSchematic->parent())->show();
		((QDockWidget*)m_miniViewContainerPCB->parent())->show();
	} else {
		((QDockWidget*)m_miniViewContainerBreadboard->parent())->hide();
		((QDockWidget*)m_miniViewContainerSchematic->parent())->hide();
		((QDockWidget*)m_miniViewContainerPCB->parent())->hide();
	}
}

void MainWindow::toggleUndoHistory(bool toggle) {
	if(toggle) {
		((QDockWidget*)m_undoView->parent())->show();
	} else {
		((QDockWidget*)m_undoView->parent())->hide();
	}
}

void MainWindow::toggleDebuggerOutput(bool toggle) {
	if (toggle) {
		DebugDialog::showDebug();
	} 
	else 
	{
	}
}

void MainWindow::updateWindowMenu() {
	m_toggleDebuggerOutputAct->setChecked(DebugDialog::visible());
	foreach (QWidget * widget, QApplication::topLevelWidgets()) {
		MainWindow * mainWindow = qobject_cast<MainWindow *>(widget);
		if (mainWindow == NULL) continue;

		QAction *action = mainWindow->raiseWindowAction();
		action->setChecked(action == m_raiseWindowAct);
		m_windowMenu->addAction(action);
	}

	//m_windowMenu->addSeparator();
	//m_windowMenu->addAction(m_openProgramWindowAct);

}

void MainWindow::pageSetup() {
	notYetImplemented(tr("Page Setup"));
}

void MainWindow::notYetImplemented(QString action) {
	QMessageBox::warning(this, tr("Fritzing"),
				tr("Sorry, \"%1\" has not been implemented yet").arg(action));
}

void MainWindow::rotateIncCW() {
	if (m_currentGraphicsView == NULL) return;

	if (m_rotate45cwAct->isEnabled()) {
		rotate45cw();
	}
	else if (m_rotate90cwAct->isEnabled()) {
		rotate90cw();
	}
}

void MainWindow::rotateIncCWRubberBand() {
	if (m_currentGraphicsView == NULL) return;

	if (m_rotate45cwAct->isEnabled()) {
		m_currentGraphicsView->rotateX(45, true);
	}
	else if (m_rotate90cwAct->isEnabled()) {
		m_currentGraphicsView->rotateX(90, true);
	}
}

void MainWindow::rotateIncCCW() {
	if (m_currentGraphicsView == NULL) return;

	if (m_rotate45ccwAct->isEnabled()) {
		m_currentGraphicsView->rotateX(315, true);
	}
	else if (m_rotate90ccwAct->isEnabled()) {
		m_currentGraphicsView->rotateX(270, true);
	}
}

void MainWindow::rotateIncCCWRubberBand() {
	if (m_currentGraphicsView == NULL) return;

	if (m_rotate45ccwAct->isEnabled()) {
		m_currentGraphicsView->rotateX(315, true);
	}
	else if (m_rotate90ccwAct->isEnabled()) {
		m_currentGraphicsView->rotateX(270, true);
	}
}

void MainWindow::rotate90cw() {
	if (m_currentGraphicsView == NULL) return;

	m_currentGraphicsView->rotateX(90, false);
}

void MainWindow::rotate90ccw() {
	if (m_currentGraphicsView == NULL) return;

	m_currentGraphicsView->rotateX(270, false);
}

void MainWindow::rotate45ccw() {
	if (m_currentGraphicsView == NULL) return;

	m_currentGraphicsView->rotateX(315, false);
}

void MainWindow::rotate45cw() {
	if (m_currentGraphicsView == NULL) return;

	m_currentGraphicsView->rotateX(45, false);
}

void MainWindow::rotate180() {
	if (m_currentGraphicsView == NULL) return;

	m_currentGraphicsView->rotateX(180, false);
}

void MainWindow::flipHorizontal() {
	m_currentGraphicsView->flipX(Qt::Horizontal, false);
}

void MainWindow::flipVertical() {
	m_currentGraphicsView->flipX(Qt::Vertical, false);
}

void MainWindow::sendToBack() {
	if (m_currentGraphicsView == NULL) return;

	m_currentGraphicsView->sendToBack();
}

void MainWindow::sendBackward() {
	if (m_currentGraphicsView == NULL) return;

	m_currentGraphicsView->sendBackward();
}

void MainWindow::bringForward() {
	if (m_currentGraphicsView == NULL) return;

	m_currentGraphicsView->bringForward();
}

void MainWindow::bringToFront() {
	if (m_currentGraphicsView == NULL) return;

	m_currentGraphicsView->bringToFront();
}

void MainWindow::showAllLayers() {
	if (m_currentGraphicsView == NULL) return;

	m_currentGraphicsView->setAllLayersVisible(true);
	updateLayerMenu();
}

void MainWindow::hideAllLayers() {
	if (m_currentGraphicsView == NULL) return;

	m_currentGraphicsView->setAllLayersVisible(false);
	updateLayerMenu();
}

void MainWindow::openURL() {
	QAction *action = qobject_cast<QAction *>(sender());
	if (action == NULL) return;

	QString href = action->data().toString();
	if (href.isEmpty()) return;

	QDesktopServices::openUrl(href);
}

void MainWindow::openRecentOrExampleFile() {
	QAction *action = qobject_cast<QAction *>(sender());
	if (action) {
		QString filename = action->data().toString();
		if (alreadyOpen(filename)) {
			return;
		}

		if (!QFileInfo(filename).exists()) {
			QMessageBox::warning(NULL, tr("Fritzing"), tr("File '%1' not found").arg(filename));
			return;
		}

		MainWindow* mw = newMainWindow(m_paletteModel, m_refModel, action->data().toString(), true, true);
		bool readOnly = m_openExampleActions.contains(action->text());
		mw->setReadOnly(readOnly);
		mw->loadWhich(filename,!readOnly,!readOnly,"");
		mw->clearFileProgressDialog();
		closeIfEmptySketch(mw);
	}
}

void MainWindow::removeActionsStartingAt(QMenu * menu, int start) {
	QList<QAction*> actions = menu->actions();

	if(start == 0) {
		menu->clear();
	} else {
		for(int i=start; i < actions.size(); i++) {
			menu->removeAction(actions.at(i));
		}
	}
}

void MainWindow::hideShowTraceMenu() {
	m_pcbTraceMenu->menuAction()->setVisible(m_currentGraphicsView == m_pcbGraphicsView);
	m_schematicTraceMenu->menuAction()->setVisible(m_currentGraphicsView == m_schematicGraphicsView);
	m_breadboardTraceMenu->menuAction()->setVisible(m_currentGraphicsView == m_breadboardGraphicsView);
}

void MainWindow::createTraceMenuActions() {
	m_autorouteAct = new QAction(tr("&Autoroute"), this);
	m_autorouteAct->setStatusTip(tr("Autoroute..."));
	m_autorouteAct->setShortcut(tr("Shift+Ctrl+A"));
	connect(m_autorouteAct, SIGNAL(triggered()), this, SLOT(autoroute()));

	createOrderFabAct();

	m_activeLayerBothAct = new QAction(tr("Set both copper layers clickable"), this);
	m_activeLayerBothAct->setStatusTip(tr("Set both copper layers clickable"));
	m_activeLayerBothAct->setShortcut(tr("Shift+Ctrl+3"));
	connect(m_activeLayerBothAct, SIGNAL(triggered()), this, SLOT(activeLayerBoth()));

	m_activeLayerTopAct = new QAction(tr("Set copper top layer clickable"), this);
	m_activeLayerTopAct->setStatusTip(tr("Set copper top layer clickable"));
	m_activeLayerTopAct->setShortcut(tr("Shift+Ctrl+2"));
	connect(m_activeLayerTopAct, SIGNAL(triggered()), this, SLOT(activeLayerTop()));

	m_activeLayerBottomAct = new QAction(tr("Set copper bottom layer clickable"), this);
	m_activeLayerBottomAct->setStatusTip(tr("Set copper bottom layer clickable"));
	m_activeLayerBottomAct->setShortcut(tr("Shift+Ctrl+1"));
	connect(m_activeLayerBottomAct, SIGNAL(triggered()), this, SLOT(activeLayerBottom()));

	QAction * traceAct = new QAction(tr("&Create trace from ratsnest"), this);
	traceAct->setStatusTip(tr("Create a trace from the ratsnest line"));
	m_createTraceWireAct = new WireAction(traceAct);
	connect(m_createTraceWireAct, SIGNAL(triggered()), this, SLOT(createTrace()));
	traceAct = new QAction(tr("&Create wire from ratsnest"), this);
	traceAct->setStatusTip(tr("Create a wire from the ratsnest line"));
	m_createWireWireAct = new WireAction(traceAct);
	connect(m_createWireWireAct, SIGNAL(triggered()), this, SLOT(createTrace()));

	m_excludeFromAutorouteAct = new QAction(tr("Do not autoroute"), this);
	m_excludeFromAutorouteAct->setStatusTip(tr("When autorouting, do not rip up this trace wire, via, or jumper item"));
	connect(m_excludeFromAutorouteAct, SIGNAL(triggered()), this, SLOT(excludeFromAutoroute()));
	m_excludeFromAutorouteAct->setCheckable(true);
	m_excludeFromAutorouteWireAct = new WireAction(m_excludeFromAutorouteAct);
	connect(m_excludeFromAutorouteWireAct, SIGNAL(triggered()), this, SLOT(excludeFromAutoroute()));

    m_changeTraceLayerAct = new QAction(tr("Move to other side of the board"), this);
	m_changeTraceLayerAct->setStatusTip(tr("Move selected traces to the other side of the board (note: the 'first' trace will be moved and the rest will follow to the same side)"));
	connect(m_changeTraceLayerAct, SIGNAL(triggered()), this, SLOT(changeTraceLayer()));

	m_selectAllTracesAct = new QAction(tr("Select All Traces"), this);
	m_selectAllTracesAct->setStatusTip(tr("Select all trace wires"));
	connect(m_selectAllTracesAct, SIGNAL(triggered()), this, SLOT(selectAllTraces()));

	m_selectAllWiresAct = new QAction(tr("Select All Wires"), this);
	m_selectAllWiresAct->setStatusTip(tr("Select all wires"));
	connect(m_selectAllWiresAct, SIGNAL(triggered()), this, SLOT(selectAllTraces()));

	m_selectAllCopperFillAct = new QAction(tr("Select All CopperFill"), this);
	m_selectAllCopperFillAct->setStatusTip(tr("Select all copper fill items"));
	connect(m_selectAllCopperFillAct, SIGNAL(triggered()), this, SLOT(selectAllCopperFill()));

	m_updateRoutingStatusAct = new QAction(tr("Force Update Routing Status and Ratsnests"), this);
	m_updateRoutingStatusAct->setStatusTip(tr("Recalculate routing status and ratsnest wires (in case the auto-update isn't working correctly)"));
	connect(m_updateRoutingStatusAct, SIGNAL(triggered()), this, SLOT(updateRoutingStatus()));

	m_selectAllExcludedTracesAct = new QAction(tr("Select All \"Don't Autoroute\" Traces"), this);
	m_selectAllExcludedTracesAct->setStatusTip(tr("Select all trace wires excluded from autorouting"));
	connect(m_selectAllExcludedTracesAct, SIGNAL(triggered()), this, SLOT(selectAllExcludedTraces()));

	m_selectAllIncludedTracesAct = new QAction(tr("Select All Autoroutable Traces"), this);
	m_selectAllIncludedTracesAct->setStatusTip(tr("Select all trace wires that can be changed during autorouting"));
	connect(m_selectAllIncludedTracesAct, SIGNAL(triggered()), this, SLOT(selectAllIncludedTraces()));

	m_selectAllJumperItemsAct = new QAction(tr("Select All Jumpers"), this);
	m_selectAllJumperItemsAct->setStatusTip(tr("Select all jumper item parts"));
	connect(m_selectAllJumperItemsAct, SIGNAL(triggered()), this, SLOT(selectAllJumperItems()));

	m_selectAllViasAct = new QAction(tr("Select All Vias"), this);
	m_selectAllViasAct->setStatusTip(tr("Select all via parts"));
	connect(m_selectAllViasAct, SIGNAL(triggered()), this, SLOT(selectAllVias()));

	m_tidyWiresAct = new QAction(tr("Tidy Wires"), this);
	m_tidyWiresAct->setStatusTip(tr("Tidy selected wires"));
	connect(m_tidyWiresAct, SIGNAL(triggered()), this, SLOT(tidyWires()));

	m_groundFillAct = new QAction(tr("Ground Fill"), this);
	m_groundFillAct->setStatusTip(tr("Fill empty regions of the copper layer--fill will include all traces connected to a GROUND"));
	connect(m_groundFillAct, SIGNAL(triggered()), this, SLOT(groundFill()));

	m_copperFillAct = new QAction(tr("Copper Fill"), this);
	m_copperFillAct->setStatusTip(tr("Fill empty regions of the copper layer--not including traces connected to a GROUND"));
	connect(m_copperFillAct, SIGNAL(triggered()), this, SLOT(copperFill()));

	m_removeGroundFillAct = new QAction(tr("Remove Copper Fill"), this);
	m_removeGroundFillAct->setStatusTip(tr("Remove the copper fill"));
	connect(m_removeGroundFillAct, SIGNAL(triggered()), this, SLOT(removeGroundFill()));

	m_setGroundFillSeedsAct = new QAction(tr("Choose Ground Fill Seed(s)..."), this);
	m_setGroundFillSeedsAct->setStatusTip(tr("Fill empty regions of the copper layer--fill will include all traces connected to the seeds"));
	connect(m_setGroundFillSeedsAct, SIGNAL(triggered()), this, SLOT(setGroundFillSeeds()));

	m_setOneGroundFillSeedAct = new ConnectorItemAction(tr("Set Ground Fill Seed"), this);
	m_setOneGroundFillSeedAct->setStatusTip(tr("Treat this connector and its connections as a 'ground' during ground fill."));
	m_setOneGroundFillSeedAct->setCheckable(true);
	connect(m_setOneGroundFillSeedAct, SIGNAL(triggered()), this, SLOT(setOneGroundFillSeed()));

	m_clearGroundFillSeedsAct = new ConnectorItemAction(tr("Clear Ground Fill Seeds"), this);
	m_clearGroundFillSeedsAct->setStatusTip(tr("Clear ground fill seeds--enable copper fill only."));
	connect(m_clearGroundFillSeedsAct, SIGNAL(triggered()), this, SLOT(clearGroundFillSeeds()));

	m_designRulesCheckAct = new QAction(tr("Design Rules Check"), this);
	m_designRulesCheckAct->setStatusTip(tr("Select any parts that are too close together for safe board production (w/in 10 mil)"));
	m_designRulesCheckAct->setShortcut(tr("Shift+Ctrl+D"));
	connect(m_designRulesCheckAct, SIGNAL(triggered()), this, SLOT(designRulesCheck()));

	m_autorouterSettingsAct = new QAction(tr("Autorouter settings..."), this);
	m_autorouterSettingsAct->setStatusTip(tr("Set autorouting parameters..."));
	connect(m_autorouterSettingsAct, SIGNAL(triggered()), this, SLOT(autorouterSettings()));
}

void MainWindow::activeLayerBoth() {
	PCBSketchWidget * pcbSketchWidget = qobject_cast<PCBSketchWidget *>(m_currentGraphicsView);
	if (pcbSketchWidget == NULL) return;

	pcbSketchWidget->setLayerActive(ViewLayer::Copper1, true);
	pcbSketchWidget->setLayerActive(ViewLayer::Copper0, true);
	AutoCloseMessageBox::showMessage(this, tr("Copper Top and Copper Bottom layers are both active"));
	updateActiveLayerButtons();
}

void MainWindow::activeLayerTop() {
	PCBSketchWidget * pcbSketchWidget = qobject_cast<PCBSketchWidget *>(m_currentGraphicsView);
	if (pcbSketchWidget == NULL) return;

	pcbSketchWidget->setLayerActive(ViewLayer::Copper1, true);
	pcbSketchWidget->setLayerActive(ViewLayer::Copper0, false);
	AutoCloseMessageBox::showMessage(this, tr("Copper Top layer is active"));
	updateActiveLayerButtons();
}

void MainWindow::activeLayerBottom() {
	PCBSketchWidget * pcbSketchWidget = qobject_cast<PCBSketchWidget *>(m_currentGraphicsView);
	if (pcbSketchWidget == NULL) return;

	pcbSketchWidget->setLayerActive(ViewLayer::Copper1, false);
	pcbSketchWidget->setLayerActive(ViewLayer::Copper0, true);
	AutoCloseMessageBox::showMessage(this, tr("Copper Bottom layer is active"));
	updateActiveLayerButtons();
}

void MainWindow::toggleActiveLayer() 
{
	PCBSketchWidget * pcbSketchWidget = qobject_cast<PCBSketchWidget *>(m_currentGraphicsView);
	if (pcbSketchWidget == NULL) return;

	int index = activeLayerIndex();
	switch (index) {
		case 0:
			activeLayerBottom();
			return;
		case 1:
			activeLayerTop();
			return;
		case 2:
			activeLayerBoth();
			return;
		default:
			return;
	}
}


void MainWindow::createOrderFabAct() {
	if (m_orderFabAct != NULL) return;

	m_orderFabAct = new QAction(tr("Order a PCB..."), this);
	m_orderFabAct->setStatusTip(tr("Order a PCB created from your sketch--from fabulous Fritzing Fab"));
	connect(m_orderFabAct, SIGNAL(triggered()), this, SLOT(orderFab()));
}

void MainWindow::autoroute() {
	PCBSketchWidget * pcbSketchWidget = qobject_cast<PCBSketchWidget *>(m_currentGraphicsView);
	if (pcbSketchWidget == NULL) return;

	dynamic_cast<SketchAreaWidget *>(pcbSketchWidget->parent())->routingStatusLabel()->setText(tr("Autorouting..."));

	bool copper0Active = pcbSketchWidget->layerIsActive(ViewLayer::Copper0);
	bool copper1Active = pcbSketchWidget->layerIsActive(ViewLayer::Copper1);

	AutorouteProgressDialog progress(tr("Autorouting Progress..."), true, true, true, pcbSketchWidget, this);
	progress.setModal(true);
	progress.show();
	QRect pr = progress.frameGeometry();
	QRect wr = this->frameGeometry();
	progress.move(wr.right() - pr.width(), pr.top());

	pcbSketchWidget->scene()->clearSelection();
	pcbSketchWidget->setIgnoreSelectionChangeEvents(true);
	CMRouter * autorouter = new CMRouter(pcbSketchWidget);

	connect(autorouter, SIGNAL(wantTopVisible()), this, SLOT(activeLayerTop()), Qt::DirectConnection);
	connect(autorouter, SIGNAL(wantBottomVisible()), this, SLOT(activeLayerBottom()), Qt::DirectConnection);
	connect(autorouter, SIGNAL(wantBothVisible()), this, SLOT(activeLayerBoth()), Qt::DirectConnection);

	connect(&progress, SIGNAL(cancel()), autorouter, SLOT(cancel()), Qt::DirectConnection);
	connect(&progress, SIGNAL(skip()), autorouter, SLOT(cancelTrace()), Qt::DirectConnection);
	connect(&progress, SIGNAL(stop()), autorouter, SLOT(stopTracing()), Qt::DirectConnection);
	connect(&progress, SIGNAL(spinChange(int)), autorouter, SLOT(setMaxCycles(int)), Qt::DirectConnection);

	connect(autorouter, SIGNAL(setMaximumProgress(int)), &progress, SLOT(setMaximum(int)), Qt::DirectConnection);
	connect(autorouter, SIGNAL(setProgressValue(int)), &progress, SLOT(setValue(int)), Qt::DirectConnection);
	connect(autorouter, SIGNAL(setProgressMessage(const QString &)), &progress, SLOT(setMessage(const QString &)));
	connect(autorouter, SIGNAL(setCycleMessage(const QString &)), &progress, SLOT(setSpinLabel(const QString &)));
	connect(autorouter, SIGNAL(setCycleCount(int)), &progress, SLOT(setSpinValue(int)));

	ProcessEventBlocker::processEvents();
	ProcessEventBlocker::block();

	autorouter->start();
	pcbSketchWidget->setIgnoreSelectionChangeEvents(false);

	delete autorouter;

	pcbSketchWidget->setLayerActive(ViewLayer::Copper1, copper1Active);
	pcbSketchWidget->setLayerActive(ViewLayer::Copper0, copper0Active);
	updateActiveLayerButtons();

	ProcessEventBlocker::unblock();
}

void MainWindow::createTrace() {
	m_currentGraphicsView->createTrace(retrieveWire());
}

void MainWindow::excludeFromAutoroute() {
	Wire * wire = retrieveWire();
	PCBSketchWidget * pcbSketchWidget = qobject_cast<PCBSketchWidget *>(m_currentGraphicsView);
	if (pcbSketchWidget == NULL) return;

	pcbSketchWidget->excludeFromAutoroute(wire == NULL ? m_excludeFromAutorouteAct->isChecked() : m_excludeFromAutorouteWireAct->isChecked());
}

void MainWindow::selectAllTraces() {
	m_currentGraphicsView->selectAllWires(m_currentGraphicsView->getTraceFlag());
}

void MainWindow::updateRoutingStatus() {
	RoutingStatus routingStatus;
	routingStatus.zero();
	m_currentGraphicsView->updateRoutingStatus(NULL, routingStatus, true);
}

void MainWindow::selectAllExcludedTraces() {
	m_pcbGraphicsView->selectAllExcludedTraces();
}

void MainWindow::selectAllIncludedTraces() {
	m_pcbGraphicsView->selectAllIncludedTraces();
}

void MainWindow::selectAllJumperItems() {
	m_currentGraphicsView->selectAllItemType(ModelPart::Jumper);
}

void MainWindow::selectAllCopperFill() {
	m_currentGraphicsView->selectAllItemType(ModelPart::CopperFill);
}

void MainWindow::selectAllVias() {
	m_currentGraphicsView->selectAllItemType(ModelPart::Via);
}

void MainWindow::notClosableForAWhile() {
	m_dontClose = true;

	QTimer::singleShot(500, this, SLOT(ensureClosable()));
}

void MainWindow::ensureClosable() {
	m_dontClose = false;
}

void MainWindow::showPartLabels() {
	if (m_currentGraphicsView == NULL) return;

	m_currentGraphicsView->showPartLabels(m_showPartLabelAct->isChecked());
}

void MainWindow::addNote() {
	if (m_currentGraphicsView == NULL) return;

	ViewGeometry vg;
	vg.setRect(0, 0, Note::initialMinWidth, Note::initialMinHeight);
	QPointF tl = m_currentGraphicsView->mapToScene(QPoint(0, 0));
	QSizeF vpSize = m_currentGraphicsView->viewport()->size();
	tl.setX(tl.x() + ((vpSize.width() - Note::initialMinWidth) / 2.0));
	tl.setY(tl.y() + ((vpSize.height() - Note::initialMinHeight) / 2.0));
	vg.setLoc(tl);

	QUndoCommand * parentCommand = new QUndoCommand(tr("Add Note"));
	m_currentGraphicsView->stackSelectionState(false, parentCommand);
	m_currentGraphicsView->scene()->clearSelection();
	new AddItemCommand(m_currentGraphicsView, BaseCommand::SingleView, ModuleIDNames::NoteModuleIDName, m_currentGraphicsView->defaultViewLayerSpec(), vg, ItemBase::getNextID(), false, -1, parentCommand);
	m_undoStack->push(parentCommand);
}

bool MainWindow::alreadyOpen(const QString & fileName) {
    foreach (QWidget * widget, QApplication::topLevelWidgets()) {
        MainWindow * mainWindow = qobject_cast<MainWindow *>(widget);
        if (mainWindow == NULL) continue;

		// don't load two copies of the same file
		if (mainWindow->fileName().compare(fileName) == 0) {
			mainWindow->raise();
			return true;
		}
    }

	return false;
}

void MainWindow::enableAddBendpointAct(QGraphicsItem * graphicsItem) {
	m_addBendpointAct->setEnabled(false);
	m_flattenCurveAct->setEnabled(false);

	Wire * wire = dynamic_cast<Wire *>(graphicsItem);
	if (wire == NULL) return;
	if (wire->getRatsnest()) return;

	m_flattenCurveAct->setEnabled(wire->isCurved());

	BendpointAction * bendpointAction = qobject_cast<BendpointAction *>(m_addBendpointAct);
	FGraphicsScene * scene = qobject_cast<FGraphicsScene *>(graphicsItem->scene());
	if (scene != NULL) {
		bendpointAction->setLastLocation(scene->lastContextMenuPos());
	}

	bool enabled = false;
	if (m_currentGraphicsView->lastHoverEnterConnectorItem()) {
		bendpointAction->setText(tr("Remove Bendpoint"));
		bendpointAction->setLastHoverEnterConnectorItem(m_currentGraphicsView->lastHoverEnterConnectorItem());
		bendpointAction->setLastHoverEnterItem(NULL);
		enabled = true;
	}
	else if (m_currentGraphicsView->lastHoverEnterItem()) {
		bendpointAction->setText(tr("Add Bendpoint"));
		bendpointAction->setLastHoverEnterItem(m_currentGraphicsView->lastHoverEnterItem());
		bendpointAction->setLastHoverEnterConnectorItem(NULL);
		enabled = true;
	}
	else {
		bendpointAction->setLastHoverEnterItem(NULL);
		bendpointAction->setLastHoverEnterConnectorItem(NULL);
	}

	m_addBendpointAct->setEnabled(enabled);
}

void MainWindow::addBendpoint()
{
	BendpointAction * bendpointAction = qobject_cast<BendpointAction *>(m_addBendpointAct);

	m_currentGraphicsView->addBendpoint(bendpointAction->lastHoverEnterItem(),
										bendpointAction->lastHoverEnterConnectorItem(),
										bendpointAction->lastLocation());
}

void MainWindow::flattenCurve()
{
	BendpointAction * bendpointAction = qobject_cast<BendpointAction *>(m_addBendpointAct);

	m_currentGraphicsView->flattenCurve(bendpointAction->lastHoverEnterItem(),
										bendpointAction->lastHoverEnterConnectorItem(),
										bendpointAction->lastLocation());
}


void MainWindow::importFilesFromPrevInstall() {
	QString prevInstallPath = QFileDialog::getExistingDirectory(
			this,
			tr("Please choose the previous Fritzing folder..."),
            QDir::homePath(),
            QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
	if(prevInstallPath.isNull()) return;
	if(!QFileInfo(prevInstallPath+"/parts").exists()) {
		QMessageBox::critical(
			this, QObject::tr("Fritzing"),
			tr("The folder \"%1\" isn't a Friting installation folder").arg(prevInstallPath));
		return;
	}

	QString userDataPath = FolderUtils::getUserDataStorePath();

	// replicate dirs
	QStringList foldersToCopy = FolderUtils::getUserDataStoreFolders();
	foreach(QString folder, foldersToCopy) {
        FolderUtils::replicateDir(QDir(prevInstallPath+folder), QDir(userDataPath+folder));
	}

	// cleanup old bins
	QDir dataStoreBins(userDataPath);
	dataStoreBins.cd("bins");
	QStringList binsToRemove;
	binsToRemove
		<< "allParts.fzb" << "artreenoBin.fzb"
		<< "E6SetBin.fzb" << "pin_headers.fzb";
	foreach(QString binToRemove, binsToRemove) {
		dataStoreBins.remove(binToRemove);
	}

	// make sure to add the old my_parts.fzp to the folder
	QString myPartsBinRelPath = "/bins/my_parts.fzb";
	QFile myOldPartsBinFile(prevInstallPath+myPartsBinRelPath);
	if(myOldPartsBinFile.exists()) {
		QDateTime now = QDateTime::currentDateTime();
		QString newNamePostfix = QString("__imported_on__%1.fzb").arg(now.toString("yyyy-MM-dd_hh-mm-ss"));
		myOldPartsBinFile.copy(userDataPath+myPartsBinRelPath.replace(".fzb",newNamePostfix));
	}

	QMessageBox::information(
		this, QObject::tr("Fritzing"),
		tr("You will have to restart Fritzing in order to use the imported parts"));
}

void MainWindow::tidyWires() {
	m_currentGraphicsView->tidyWires();
}

void MainWindow::copperFill() {
	groundFillAux(false);
}

void MainWindow::groundFill()
{
	groundFillAux(true);
}

void MainWindow::groundFillAux(bool fillGroundTraces)
{
	// TODO:
	//		what about leftover temp files from crashes?
	//		clear ground plane when anything changes
	//		some polygons can be combined
	//		remove old ground plane modules from paletteModel and database

	if (m_pcbGraphicsView == NULL) return;

    FileProgressDialog fileProgress(tr("Generating %1 fill...").arg(fillGroundTraces ? tr("ground") : tr("copper")), 0, this);
	QUndoCommand * parentCommand = new QUndoCommand(fillGroundTraces ? tr("Ground Fill") : tr("Copper Fill"));
	if (m_pcbGraphicsView->groundFill(fillGroundTraces, parentCommand)) {
		m_undoStack->push(parentCommand);
	}
	else {
		delete parentCommand;
	}
}

void MainWindow::removeGroundFill(bool force) {
	QSet<ItemBase *> toDelete;
	foreach (QGraphicsItem * item, m_pcbGraphicsView->scene()->items()) {
		ItemBase * itemBase = dynamic_cast<ItemBase *>(item);
		if (itemBase == NULL) continue;

		if (isGroundFill(itemBase)) {
			toDelete.insert(itemBase->layerKinChief());
		}
	}

	if (toDelete.count() <= 0) return;

	QSet<Wire *> wiresToDelete;
	foreach (ItemBase * itemBase, toDelete) {
		foreach (ConnectorItem * fromConnectorItem, itemBase->cachedConnectorItems()) {
			foreach (ConnectorItem * toConnectorItem, fromConnectorItem->connectedToItems()) {
				if (toConnectorItem->attachedToItemType() != ModelPart::Wire) continue;
				
				Wire * wire = qobject_cast<Wire *>(toConnectorItem->attachedTo());
				if (wire == NULL) continue;

				if (!wire->getTrace()) continue;

				QList<Wire *> wires;
				QList<ConnectorItem *> ends;
				wire->collectChained(wires, ends);
				foreach (Wire * w, wires) {
					wiresToDelete.insert(w);
				}
			}
		}
	}

	if (wiresToDelete.count() > 0) {
		DebugDialog::debug("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n"
							"Wires attached to copper fill\n"
							"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
		if (!force) {
			QMessageBox::StandardButton answer = QMessageBox::question(
                    this,
                    tr("Copper Fill Traces"),
                    tr("There are traces connected to copper fill which will also be removed. Proceed?"),
                    QMessageBox::Yes | QMessageBox::No,
                    QMessageBox::Yes
            );
            // TODO: make button texts translatable
            if (answer != QMessageBox::Yes) {
                    return;
            }		
		}

	}

	QUndoCommand * parentCommand = new QUndoCommand(tr("Remove copper fill"));

	new CleanUpWiresCommand(m_pcbGraphicsView, CleanUpWiresCommand::UndoOnly, parentCommand);

	foreach (Wire * wire, wiresToDelete) {
		toDelete.insert(wire);
	}

	QList<Wire *> wires = wiresToDelete.toList();
	m_pcbGraphicsView->makeWiresChangeConnectionCommands(wires, parentCommand);

	foreach (ItemBase * itemBase, toDelete) {
		m_pcbGraphicsView->makeDeleteItemCommand(itemBase, BaseCommand::CrossView, parentCommand);
	}

	new CleanUpWiresCommand(m_pcbGraphicsView, CleanUpWiresCommand::RedoOnly, parentCommand);

	m_undoStack->push(parentCommand);
}

bool MainWindow::isGroundFill(ItemBase * itemBase) {
	return (itemBase->itemType() == ModelPart::CopperFill);
}


QMenu *MainWindow::breadboardItemMenu() {
	QMenu *menu = new QMenu(QObject::tr("Part"), this);
	menu->addMenu(m_rotateMenu);
	return viewItemMenuAux(menu);
}

QMenu *MainWindow::schematicItemMenu() {
	QMenu *menu = new QMenu(QObject::tr("Part"), this);
	menu->addMenu(m_rotateMenu);
	menu->addAction(m_flipHorizontalAct);
	menu->addAction(m_flipVerticalAct);
	return viewItemMenuAux(menu);
}

QMenu *MainWindow::pcbItemMenu() {
	QMenu *menu = new QMenu(QObject::tr("Part"), this);
	menu->addMenu(m_rotateMenu);
	menu = viewItemMenuAux(menu);
	menu->addSeparator();
	menu->addAction(m_setOneGroundFillSeedAct);
	menu->addAction(m_clearGroundFillSeedsAct);
	return menu;
}

QMenu *MainWindow::breadboardWireMenu() {
	QMenu *menu = new QMenu(QObject::tr("Wire"), this);
	menu->addMenu(m_zOrderWireMenu);
	menu->addSeparator();
	m_wireColorMenu = menu->addMenu(tr("&Wire Color"));
	foreach(QString colorName, Wire::colorNames) {
		QString colorValue = Wire::colorTrans.value(colorName);
		QAction * action = new QAction(colorName, this);
		m_wireColorMenu->addAction(action);
		action->setData(colorValue);
		action->setCheckable(true);
		connect(action, SIGNAL(triggered(bool)), this, SLOT(changeWireColor(bool)));
	}
	menu->addAction(m_createWireWireAct);
	menu->addSeparator();
	menu->addAction(m_deleteWireAct);
	menu->addSeparator();
	menu->addAction(m_addBendpointAct);
	menu->addAction(m_flattenCurveAct);

#ifndef QT_NO_DEBUG
	menu->addSeparator();
	menu->addAction(m_infoViewOnHoverAction);
#endif

    connect( menu, SIGNAL(aboutToShow()), this, SLOT(updateWireMenu()));

	return menu;
}

QMenu *MainWindow::pcbWireMenu() {
	QMenu *menu = new QMenu(QObject::tr("Wire"), this);
	menu->addMenu(m_zOrderWireMenu);
	menu->addSeparator();
	menu->addAction(m_changeTraceLayerAct);	
	menu->addAction(m_createTraceWireAct);
	menu->addAction(m_excludeFromAutorouteWireAct);
	menu->addSeparator();
	menu->addAction(m_deleteWireAct);
	menu->addSeparator();
	menu->addAction(m_addBendpointAct);
	menu->addAction(m_flattenCurveAct);

#ifndef QT_NO_DEBUG
	menu->addSeparator();
	menu->addAction(m_infoViewOnHoverAction);
#endif

    connect(menu, SIGNAL(aboutToShow()), this, SLOT(updateWireMenu()));

	return menu;
}

QMenu *MainWindow::schematicWireMenu() {
	QMenu *menu = new QMenu(QObject::tr("Wire"), this);
	menu->addMenu(m_zOrderWireMenu);
	menu->addSeparator();
	menu->addAction(m_createTraceWireAct);
	menu->addAction(m_excludeFromAutorouteWireAct);
	menu->addSeparator();
	menu->addAction(m_deleteWireAct);
	menu->addSeparator();
	menu->addAction(m_addBendpointAct);
#ifndef QT_NO_DEBUG
	menu->addSeparator();
	menu->addAction(m_infoViewOnHoverAction);
#endif

    connect( menu, SIGNAL(aboutToShow()), this, SLOT(updateWireMenu()));

	return menu;
}

QMenu *MainWindow::viewItemMenuAux(QMenu* menu) {
	menu->addMenu(m_zOrderMenu);
	menu->addAction(m_moveLockAct);
	menu->addSeparator();
	menu->addAction(m_copyAct);
	menu->addAction(m_duplicateAct);
	menu->addAction(m_deleteAct);
#ifndef QT_NO_DEBUG
	menu->addSeparator();
	menu->addAction(m_disconnectAllAct);
#endif
	menu->addSeparator();
	menu->addAction(m_openInPartsEditorAct);
	menu->addMenu(m_addToBinMenu);
	menu->addSeparator();
	menu->addAction(m_showPartLabelAct);
#ifndef QT_NO_DEBUG
	menu->addSeparator();
	menu->addAction(m_infoViewOnHoverAction);
	menu->addAction(m_exportNormalizedSvgAction);
	menu->addAction(m_exportNormalizedFlattenedSvgAction);
#endif

    connect(
    	menu,
    	SIGNAL(aboutToShow()),
    	this,
    	SLOT(updatePartMenu())
    );

    return menu;
}

void MainWindow::changeWireColor(bool checked) {
	if (checked == false) {
		// choosing the same color again (assuming this action can only apply to a single wire at a time)
		return;
	}

	QAction * action = qobject_cast<QAction *>(sender());
	if (action == NULL) return;

	QString colorName = action->data().toString();
	if (colorName.isEmpty()) return;

	m_currentGraphicsView->changeWireColor(colorName);
}

QString MainWindow::constructFileName(const QString & differentiator, const QString & suffix) {
	QString fn = QFileInfo(m_fwFilename).completeBaseName();
	fn += "_" + (differentiator.isEmpty() ? m_currentGraphicsView->getShortName() : differentiator);
	return fn + suffix;
}

void MainWindow::startSaveInstancesSlot(const QString & fileName, ModelPart *, QXmlStreamWriter & streamWriter) {
	Q_UNUSED(fileName);

	if (m_backingUp) {
		streamWriter.writeTextElement("originalFileName", m_fwFilename);
	}

	if (m_linkedProgramFiles.count() > 0) {
		streamWriter.writeStartElement("programs");
		QSettings settings;
		streamWriter.writeAttribute("pid", settings.value("pid").toString());
		foreach (LinkedFile * linkedFile, m_linkedProgramFiles) {
			streamWriter.writeStartElement("program");
			streamWriter.writeAttribute("language", linkedFile->language);
			streamWriter.writeAttribute("programmer", linkedFile->programmer);
			streamWriter.writeCharacters(linkedFile->linkedFilename);
			streamWriter.writeEndElement();
		}
		streamWriter.writeEndElement();
	}

	streamWriter.writeStartElement("views");
	QList<SketchWidget *> views;
	views << m_breadboardGraphicsView << m_schematicGraphicsView << m_pcbGraphicsView;
	foreach  (SketchWidget * sketchWidget, views) {
		streamWriter.writeStartElement("view");
		streamWriter.writeAttribute("name", ViewIdentifierClass::viewIdentifierXmlName(sketchWidget->viewIdentifier()));
		streamWriter.writeAttribute("backgroundColor", sketchWidget->background().name());
		streamWriter.writeEndElement();
	}
	streamWriter.writeEndElement();
}


void MainWindow::loadedRootSlot(const QString & fname, ModelBase *, QDomElement & root) {
	if (root.isNull()) return;

	QDomElement programs = root.firstChildElement("programs");
	if (programs.isNull()) return;

	QString thatPid = programs.attribute("pid");
	QSettings settings;
	QString thisPid = settings.value("pid").toString();
	bool sameMachine = thatPid.isEmpty() || (!thisPid.isEmpty() && (thatPid.compare(thisPid) == 0));
	QFileInfo fileInfo(fname);
	QDir dir = fileInfo.absoluteDir();

	QDomElement program = programs.firstChildElement("program");
	while (!program.isNull()) {
		bool obsolete = false;
		bool inBundle = false;
		QString text;
		TextUtils::findText(program, text);
		if (!text.isEmpty()) {
			QString language = program.attribute("language");
			QString programmer = program.attribute("programmer");
			QString path;
			if (thatPid.isEmpty()) {
				// pre 0.7.0 relative path
				QFileInfo newFileInfo(text);
				dir.cd(newFileInfo.dir().path());
				path = dir.absoluteFilePath(newFileInfo.fileName());
				obsolete = true;
			}
			else {
				path = text;
			}

			LinkedFile * linkedFile = new LinkedFile;
			QFileInfo info(path);
			if (!(sameMachine && info.exists())) {
				inBundle = true;
				path = dir.absoluteFilePath(info.fileName());
			}
			linkedFile->linkedFilename = path;
			linkedFile->language = language;
			linkedFile->programmer = programmer;
			linkedFile->fileFlags = LinkedFile::NoFlag;
			if (sameMachine) linkedFile->fileFlags |= LinkedFile::SameMachineFlag;
			if (obsolete) linkedFile->fileFlags |= LinkedFile::ObsoleteFlag;
			if (inBundle) linkedFile->fileFlags |= LinkedFile::InBundleFlag;
			if (this->m_readOnly) linkedFile->fileFlags |= LinkedFile::ReadOnlyFlag;
			
			m_linkedProgramFiles.append(linkedFile);
		}
		program = program.nextSiblingElement("program");
	}

}


void MainWindow::loadedViewsSlot(ModelBase *, QDomElement & views) {
	if (views.isNull()) return;

	QDomElement view = views.firstChildElement("view");
	while (!view.isNull()) {
		QString name = view.attribute("name");
		ViewIdentifierClass::ViewIdentifier viewIdentifier = ViewIdentifierClass::idFromXmlName(name);
		QString colorName = view.attribute("backgroundColor");
		QColor color;
		color.setNamedColor(colorName);
		if (color.isValid()) {
			switch (viewIdentifier) {
				case ViewIdentifierClass::BreadboardView:
					m_breadboardGraphicsView->setBackground(color);
					break;
				case ViewIdentifierClass::SchematicView:
					m_schematicGraphicsView->setBackground(color);
					break;
				case ViewIdentifierClass::PCBView:
					m_pcbGraphicsView->setBackground(color);
					break;
				default:
					break;
			}
		}

		view = view.nextSiblingElement("view");
	}
}

void MainWindow::disconnectAll() {
	m_currentGraphicsView->disconnectAll();
}

bool MainWindow::externalProcess(QString & name, QString & path, QStringList & args) {
	emit externalProcessSignal(name, path, args);

	if (path.isEmpty()) return false;

	if (name.isEmpty()) {
		name = tr("Launch %1...").arg(path);
	}

	return true;
}

void MainWindow::launchExternalProcess() {
	QString name;
	QString path;
	QStringList args;
	if (!externalProcess(name, path, args)) return;

	args.append("-sketch");
	args.append(fileName());
	m_externalProcessOutput.clear();

	QFileInfo f = QFileInfo(path);
	QProcess * process = new QProcess(this);
	process->setWorkingDirectory(f.dir().absolutePath());
	process->setProcessChannelMode(QProcess::MergedChannels);
	process->setReadChannel(QProcess::StandardOutput);

	connect(process, SIGNAL(error(QProcess::ProcessError)), this, SLOT(processError(QProcess::ProcessError)));
	connect(process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(processFinished(int, QProcess::ExitStatus)));
	connect(process, SIGNAL(readyReadStandardOutput()), this, SLOT(processReadyRead()));
	connect(process, SIGNAL(stateChanged(QProcess::ProcessState)), this, SLOT(processStateChanged(QProcess::ProcessState)));

	process->start(path, args);
}


void MainWindow::processError(QProcess::ProcessError processError) {
	DebugDialog::debug(QString("process error %1").arg(processError));
}

void MainWindow::processFinished(int exitCode, QProcess::ExitStatus exitStatus) {
	DebugDialog::debug(QString("process finished %1 %2").arg(exitCode).arg(exitStatus));

	QString name, path;
	QStringList args;
	externalProcess(name, path, args);
	QMessageBox::information(this, name, QString(m_externalProcessOutput));

	sender()->deleteLater();
}

void MainWindow::processReadyRead() {
	QByteArray byteArray = qobject_cast<QProcess *>(sender())->readAllStandardOutput();
	m_externalProcessOutput.append(byteArray);

	DebugDialog::debug(byteArray.data());
}

void MainWindow::processStateChanged(QProcess::ProcessState newState) {
	switch(newState) {
		case QProcess::Running:
			DebugDialog::debug(QString("process running"));
			break;
		case QProcess::Starting:
			DebugDialog::debug(QString("process starting"));
			break;
		case QProcess::NotRunning:
			DebugDialog::debug(QString("process not running"));
			break;
	}
}

void MainWindow::shareOnline() {
	QDesktopServices::openUrl(QString("http://fritzing.org/projects/create"));
}


void MainWindow::selectAllObsolete() {
	selectAllObsolete(true);
}

void MainWindow::selectAllObsolete(bool displayFeedback) {
	int obs = m_currentGraphicsView->selectAllObsolete();
	if (!displayFeedback) return;

	if (obs <= 0) {
        QMessageBox::information(this, tr("Fritzing"), tr("No outdated parts found.\nAll your parts are up-to-date.") );
    } 
	else {
        QMessageBox::StandardButton answer = QMessageBox::question(
                this,
                tr("Outdated parts"),
                tr("Found %n outdated parts. Do you want to update them now?", "", obs),
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::Yes
        );
        // TODO: make button texts translatable
        if (answer == QMessageBox::Yes) {
            swapObsolete();
        }
	}
}

ModelPart * MainWindow::findReplacedby(ModelPart * originalModelPart) {
	ModelPart * newModelPart = originalModelPart;
	while (true) {
		QString newModuleID = newModelPart->replacedby();
		if (newModuleID.isEmpty()) {
			return ((newModelPart == originalModelPart) ? NULL : newModelPart);
		}

		ModelPart * tempModelPart = this->m_refModel->retrieveModelPart(newModuleID);
		if (tempModelPart == NULL) {
			// something's screwy
			return NULL;
		}

		newModelPart = tempModelPart;
	}
}

void MainWindow::swapObsolete() {
	swapObsolete(true);
}

void MainWindow::swapObsolete(bool displayFeedback) {

	QSet<ItemBase *> itemBases;
	foreach (QGraphicsItem * item, m_currentGraphicsView->scene()->selectedItems()) {
		ItemBase * itemBase = dynamic_cast<ItemBase *>(item);
		if (itemBase == NULL) continue;
		if (!itemBase->isObsolete()) continue;

		itemBase = itemBase->layerKinChief();
		itemBases.insert(itemBase);
	}

	if (itemBases.count() <= 0) return;

	QUndoCommand* parentCommand = new QUndoCommand();
        int count = 0;

	foreach (ItemBase * itemBase, itemBases) {
		ModelPart * newModelPart = findReplacedby(itemBase->modelPart());
		if (newModelPart == NULL) {
			QMessageBox::information(
				this,
				tr("Sorry!"),
				tr( "unable to find replacement for %1.\n").arg(itemBase->title())
			);
			continue;
		}

		count++;
		long newID = swapSelectedAuxAux(itemBase, newModelPart->moduleID(), itemBase->viewLayerSpec(), parentCommand);
		if (itemBase->modelPart()) {
			// special case for swapping old resistors.
			QString resistance = itemBase->modelPart()->properties().value("resistance", "");
			if (!resistance.isEmpty()) {
				QChar r = resistance.at(resistance.length() - 1);
				ushort ohm = r.unicode();
				if (ohm == 8486) {
					// ends with the ohm symbol
					resistance.chop(1);
				}
			}
			QString footprint = itemBase->modelPart()->properties().value("footprint", "");
			if (!resistance.isEmpty() && !footprint.isEmpty()) {
				new SetResistanceCommand(m_currentGraphicsView, newID, resistance, resistance, footprint, footprint, parentCommand);
			}

			// special case for swapping LEDs
			if (newModelPart->moduleID().contains(ModuleIDNames::ColorLEDModuleIDName)) {
				QString oldColor = itemBase->modelPart()->properties().value("color");
				QString newColor;
				if (oldColor.contains("red", Qt::CaseInsensitive)) {
					newColor = "Red (633nm)";
				}
				else if (oldColor.contains("blue", Qt::CaseInsensitive)) {
					newColor = "Blue (430nm)";
				}
				else if (oldColor.contains("yellow", Qt::CaseInsensitive)) {
					newColor = "Yellow (585nm)";
				}
				else if (oldColor.contains("green", Qt::CaseInsensitive)) {
					newColor = "Green (555nm)";
				}
				else if (oldColor.contains("white", Qt::CaseInsensitive)) {
					newColor = "White (4500K)";
				}

				if (newColor.length() > 0) {
					new SetPropCommand(m_currentGraphicsView, newID, "color", newColor, newColor, true, parentCommand);
				}
			}

		}
	}


	if (count == 0) {
		delete parentCommand;
	}
	else {
        parentCommand->setText(tr("Update %1 part(s)", "").arg(count));
		m_undoStack->push(parentCommand);
	}

	if (displayFeedback) {
		QMessageBox::information(this, tr("Fritzing"), tr("Successfully updated %1 part(s).\n"
                                                          "Please check all views for potential side-effects.").arg(count) );
	}
	DebugDialog::debug(QString("updated %1 obsolete in %2").arg(count).arg(m_fwFilename));
}

void MainWindow::throwFakeException() {
    throw "fake exception";
}

void MainWindow::alignToGrid() {
	if (m_currentGraphicsView == NULL) return;

	m_currentGraphicsView->alignToGrid(m_alignToGridAct->isChecked());
}

void MainWindow::showGrid() {
	if (m_currentGraphicsView == NULL) return;

	m_currentGraphicsView->showGrid(m_showGridAct->isChecked());
}

void MainWindow::openProgramWindow() {
	if (m_programWindow) {
		m_programWindow->setVisible(true);
		m_programWindow->raise();
		return;
	}

	m_programWindow = new ProgramWindow();
	connect(m_programWindow, SIGNAL(linkToProgramFile(const QString &, const QString &, const QString &, bool, bool)), 
			this, SLOT(linkToProgramFile(const QString &, const QString &, const QString &, bool, bool)));
	connect(m_programWindow, SIGNAL(changeActivationSignal(bool, QWidget *)), qApp, SLOT(changeActivation(bool, QWidget *)), Qt::DirectConnection);
	connect(m_programWindow, SIGNAL(destroyed(QObject *)), qApp, SLOT(topLevelWidgetDestroyed(QObject *)));

	QFileInfo fileInfo(m_fwFilename);
	m_programWindow->setup(m_linkedProgramFiles, fileInfo.absoluteDir().absolutePath());
	m_programWindow->setVisible(true);
}

void MainWindow::linkToProgramFile(const QString & filename, const QString & language, const QString & programmer, bool addLink, bool strong) {
#ifdef Q_WS_WIN
	Qt::CaseSensitivity sensitivity = Qt::CaseInsensitive;
#else
	Qt::CaseSensitivity sensitivity = Qt::CaseSensitive;
#endif

	if (addLink && strong) {
		bool gotOne = false;
		foreach (LinkedFile * linkedFile, m_linkedProgramFiles) {
			if (linkedFile->linkedFilename.compare(filename, sensitivity) == 0) {
				if (linkedFile->language != language) {
					linkedFile->language = language;
					this->setWindowModified(true);
				}
				if (linkedFile->programmer != programmer) {
					linkedFile->programmer = programmer;
					this->setWindowModified(true);
				}
				gotOne = true;
				break;
			}
		}
		if (!gotOne) {
			LinkedFile * linkedFile = new LinkedFile;
			linkedFile->linkedFilename = filename;
			linkedFile->language = language;
			linkedFile->programmer = programmer;
			m_linkedProgramFiles.append(linkedFile);
			this->setWindowModified(true);
		}
		return;
	}
	else {
		for (int i = 0; i < m_linkedProgramFiles.count(); i++) {
			LinkedFile * linkedFile = m_linkedProgramFiles.at(i);
			if (linkedFile->linkedFilename.compare(filename, sensitivity) == 0) {
				if (strong) {
					m_linkedProgramFiles.removeAt(i);
					this->setWindowModified(true);
				}
				else {
					if (linkedFile->language != language) {
						linkedFile->language = language;
						this->setWindowModified(true);
					}
					if (linkedFile->programmer != programmer) {
						linkedFile->programmer = programmer;
						this->setWindowModified(true);
					}
				}
				return;
			}
		}
	}
}

void MainWindow::designRulesCheck() 
{
	if (m_currentGraphicsView == NULL) return;

	PCBSketchWidget * pcbSketchWidget = qobject_cast<PCBSketchWidget *>(m_currentGraphicsView);
	if (pcbSketchWidget == NULL) return;
	
	CMRouter cmRouter(pcbSketchWidget);
	QString message;
	bool result = cmRouter.drc(message);

	if (result) {
		QMessageBox::information(this, tr("Fritzing"), message);
	}
	else {
		QMessageBox::warning(this, tr("Fritzing"), message);
	}

	cmRouter.drcClean();
}

void MainWindow::changeTraceLayer() {
	if (m_currentGraphicsView == NULL) return;
	if (m_currentGraphicsView != m_pcbGraphicsView) return;

	m_pcbGraphicsView->changeTraceLayer();
}

void MainWindow::updateNet() {
	m_currentGraphicsView->updateNet(retrieveWire());
}

Wire * MainWindow::retrieveWire() {
	WireAction * wireAction = qobject_cast<WireAction *>(sender());
	if (wireAction == NULL) return NULL;

	return wireAction->wire();
}

ConnectorItem * MainWindow::retrieveConnectorItem() {
	ConnectorItemAction * connectorItemAction = qobject_cast<ConnectorItemAction *>(sender());
	if (connectorItemAction == NULL) return NULL;

	return connectorItemAction->connectorItem();
}

void MainWindow::moveLock()
{
	bool moveLock = true;
	
	foreach (QGraphicsItem  * item, m_currentGraphicsView->scene()->selectedItems()) {
		ItemBase * itemBase = ItemBase::extractTopLevelItemBase(item);
		if (itemBase == NULL) continue;
		if (itemBase->itemType() == ModelPart::Wire) continue;

		if (itemBase->moveLock()) {
			moveLock = false;
			break;
		}
	}

	ItemBase * viewedItem = m_infoView->currentItem();
	foreach (QGraphicsItem  * item, m_currentGraphicsView->scene()->selectedItems()) {
		ItemBase * itemBase = ItemBase::extractTopLevelItemBase(item);
		if (itemBase == NULL) continue;
		if (itemBase->itemType() == ModelPart::Wire) continue;
		
		itemBase->setMoveLock(moveLock);
		if (viewedItem && viewedItem->layerKinChief() == itemBase->layerKinChief()) {
			m_currentGraphicsView->viewItemInfo(itemBase);
		}
	}
}

void MainWindow::selectMoveLock()
{
	m_currentGraphicsView->selectAllMoveLock();
}

void MainWindow::autorouterSettings() {
	if (m_currentGraphicsView != m_pcbGraphicsView) return;

	m_pcbGraphicsView->autorouterSettings();
}

void MainWindow::orderFab() 
{
	QDesktopServices::openUrl(QString("http://fab.fritzing.org/"));
}

void MainWindow::setGroundFillSeeds() {
	m_pcbGraphicsView->setGroundFillSeeds();
}

void MainWindow::clearGroundFillSeeds() {
	m_pcbGraphicsView->clearGroundFillSeeds();
}

void MainWindow::setOneGroundFillSeed() {
	ConnectorItemAction * action = qobject_cast<ConnectorItemAction *>(sender());
	if (action == NULL) return;

	ConnectorItem * connectorItem = action->connectorItem();
	if (connectorItem == NULL) return;

	GroundFillSeedCommand * command = new GroundFillSeedCommand(m_pcbGraphicsView, NULL);
	command->addItem(connectorItem->attachedToID(), connectorItem->connectorSharedID(), action->isChecked());

	m_undoStack->push(command);
}
