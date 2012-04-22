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

$Revision: 5929 $:
$Author: cohen@irascible.com $:
$Date: 2012-03-28 03:13:01 -0700 (Wed, 28 Mar 2012) $

********************************************************************/

#include "fapplication.h"
#include "debugdialog.h"
#include "utils/misc.h"
#include "mainwindow.h"
#include "fsplashscreen.h"
#include "version/version.h"
#include "dialogs/prefsdialog.h"
#include "help/helper.h"
#include "partseditor/partseditormainwindow.h"
#include "fsvgrenderer.h"
#include "version/versionchecker.h"
#include "version/updatedialog.h"
#include "itemdrag.h"
#include "viewswitcher/viewswitcher.h"
#include "items/wire.h"
#include "partsbinpalette/binmanager/binmanager.h"
#include "help/tipsandtricks.h"
#include "utils/folderutils.h"
#include "utils/lockmanager.h"
#include "dialogs/translatorlistmodel.h"
#include "partsbinpalette/svgiconwidget.h"
#include "items/moduleidnames.h"
#include "partsbinpalette/searchlineedit.h"
#include "utils/ratsnestcolors.h"
#include "utils/cursormaster.h"
#include "utils/textutils.h"
#include "infoview/htmlinfoview.h"
#include "svg/gedaelement2svg.h"
#include "svg/kicadmodule2svg.h"
#include "svg/kicadschematic2svg.h"
#include "svg/gerbergenerator.h"
#include "installedfonts.h"
#include "items/pinheader.h"
#include "items/partfactory.h"
#include "dialogs/recoverydialog.h"
#include "lib/qtsysteminfo/QtSystemInfo.h"
#include "processeventblocker.h"
#include "autoroute/cmrouter/panelizer.h"

// dependency injection :P
#include "referencemodel/sqlitereferencemodel.h"
#define CurrentReferenceModel SqliteReferenceModel

#include <QSettings>
#include <QKeyEvent>
#include <QFileInfo>
#include <QDesktopServices>
#include <QLocale>
#include <QFileOpenEvent>
#include <QThread>
#include <QMessageBox>
#include <QTextStream>
#include <QFontDatabase>
#include <QtDebug>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QMultiHash>

static QNetworkAccessManager * NetworkAccessManager = NULL;

#ifdef LINUX_32
#define PLATFORM_NAME "linux-32bit"
#endif
#ifdef LINUX_64
#define PLATFORM_NAME "linux-64bit"
#endif
#ifdef Q_WS_WIN
#define PLATFORM_NAME "windows"
#endif
#ifdef Q_WS_MAC
#ifdef QT_MAC_USE_COCOA
#define PLATFORM_NAME "mac-os-x-105"
#else
#define PLATFORM_NAME "mac-os-x-104"
#endif
#endif

#ifdef Q_WS_WIN
#ifndef QT_NO_DEBUG
#define WIN_DEBUG
#endif
#endif

int FApplication::RestartNeeded = 9999;

QSet<QString> InstalledFonts::InstalledFontsList;
QMultiHash<QString, QString> InstalledFonts::InstalledFontsNameMapper;   // family name to filename; SVG files seem to have to use filename

static const double LoadProgressStart = 0.085;
static const double LoadProgressEnd = 0.6;

//////////////////////////

FApplication::FApplication( int & argc, char ** argv) : QApplication(argc, argv)
{
	MainWindow::RestartNeeded = FApplication::RestartNeeded;
	m_spaceBarIsPressed = false;
	m_mousePressed = false;
	m_referenceModel = NULL;
	m_paletteBinModel = NULL;
	m_started = false;
	m_updateDialog = NULL;
	m_lastTopmostWindow = NULL;
	m_serviceType = NoService;
	m_splash = NULL;

	m_arguments = arguments();
}

bool FApplication::init() {

	//foreach (QString argument, m_arguments) {
		//DebugDialog::debug(QString("argument %1").arg(argument));
	//}

	m_serviceType = NoService;

	QList<int> toRemove;
	for (int i = 0; i < m_arguments.length(); i++) {
		if ((m_arguments[i].compare("-h", Qt::CaseInsensitive) == 0) || 
			(m_arguments[i].compare("-help", Qt::CaseInsensitive) == 0) || 
			(m_arguments[i].compare("--help", Qt::CaseInsensitive) == 0)) 
		{
			return false;
		}

		if ((m_arguments[i].compare("-e", Qt::CaseInsensitive) == 0) ||
			(m_arguments[i].compare("-examples", Qt::CaseInsensitive) == 0)||
			(m_arguments[i].compare("--examples", Qt::CaseInsensitive) == 0)) {
			m_serviceType = ExampleService;
			m_outputFolder = " ";					// otherwise program will bail out
			toRemove << i;
		}

		if ((m_arguments[i].compare("-d", Qt::CaseInsensitive) == 0) ||
			(m_arguments[i].compare("-debug", Qt::CaseInsensitive) == 0)||
			(m_arguments[i].compare("--debug", Qt::CaseInsensitive) == 0)) {
			DebugDialog::setEnabled(true);
			toRemove << i;
		}

		if (i + 1 >= m_arguments.length()) continue;

		if ((m_arguments[i].compare("-f", Qt::CaseInsensitive) == 0) ||
			(m_arguments[i].compare("-folder", Qt::CaseInsensitive) == 0)||
			(m_arguments[i].compare("--folder", Qt::CaseInsensitive) == 0))
		{
			FolderUtils::setApplicationPath(m_arguments[i + 1]);
			// delete these so we don't try to process them as files later
			toRemove << i << i + 1;
		}

		if ((m_arguments[i].compare("-geda", Qt::CaseInsensitive) == 0) ||
			(m_arguments[i].compare("--geda", Qt::CaseInsensitive) == 0)) {
			m_serviceType = GedaService;
			m_outputFolder = m_arguments[i + 1];
			toRemove << i << i + 1;
		}

		if ((m_arguments[i].compare("-kicad", Qt::CaseInsensitive) == 0) ||
			(m_arguments[i].compare("--kicad", Qt::CaseInsensitive) == 0)) {
			m_serviceType = KicadFootprintService;
			m_outputFolder = m_arguments[i + 1];
			toRemove << i << i + 1;
		}

		if ((m_arguments[i].compare("-kicadschematic", Qt::CaseInsensitive) == 0) ||
			(m_arguments[i].compare("--kicadschematic", Qt::CaseInsensitive) == 0)) {
			m_serviceType = KicadSchematicService;
			m_outputFolder = m_arguments[i + 1];
			toRemove << i << i + 1;
		}

		if ((m_arguments[i].compare("-g", Qt::CaseInsensitive) == 0) ||
			(m_arguments[i].compare("-gerber", Qt::CaseInsensitive) == 0)||
			(m_arguments[i].compare("--gerber", Qt::CaseInsensitive) == 0)) {
			m_serviceType = GerberService;
			m_outputFolder = m_arguments[i + 1];
			toRemove << i << i + 1;
		}

		if ((m_arguments[i].compare("-p", Qt::CaseInsensitive) == 0) ||
			(m_arguments[i].compare("-panel", Qt::CaseInsensitive) == 0)||
			(m_arguments[i].compare("--panel", Qt::CaseInsensitive) == 0)) {
			m_serviceType = PanelizerService;
			m_panelFilename = m_arguments[i + 1];
			m_outputFolder = " ";					// otherwise program will bail out
			toRemove << i << i + 1;
		}

		if ((m_arguments[i].compare("-i", Qt::CaseInsensitive) == 0) ||
			(m_arguments[i].compare("-inscription", Qt::CaseInsensitive) == 0)||
			(m_arguments[i].compare("--inscription", Qt::CaseInsensitive) == 0)) {
			m_serviceType = InscriptionService;
			m_panelFilename = m_arguments[i + 1];
			m_outputFolder = " ";					// otherwise program will bail out
			toRemove << i << i + 1;
		}

		if (m_arguments[i].compare("-ep", Qt::CaseInsensitive) == 0) {
			m_externalProcessPath = m_arguments[i + 1];
			toRemove << i << i + 1;
		}

		if (m_arguments[i].compare("-eparg", Qt::CaseInsensitive) == 0) {
			m_externalProcessArgs << m_arguments[i + 1];
			toRemove << i << i + 1;
		}

		if (m_arguments[i].compare("-epname", Qt::CaseInsensitive) == 0) {
			m_externalProcessName = m_arguments[i + 1];
			toRemove << i << i + 1;
		}

	}

	while (toRemove.count() > 0) {
		int ix = toRemove.takeLast();
		m_arguments.removeAt(ix);
	}

	m_started = false;
	m_updateDialog = NULL;
	m_lastTopmostWindow = NULL;

	connect(&m_activationTimer, SIGNAL(timeout()), this, SLOT(updateActivation()));
	m_activationTimer.setInterval(10);
	m_activationTimer.setSingleShot(true);

	QCoreApplication::setOrganizationName("Fritzing");
	QCoreApplication::setOrganizationDomain("fritzing.org");
	QCoreApplication::setApplicationName("Fritzing");

	installEventFilter(this);

	// tell app where to search for plugins (jpeg export and sql lite)
	m_libPath = FolderUtils::getLibraryPath();
	QApplication::addLibraryPath(m_libPath);	

	/*QFile file("libpath.txt");
	if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		QTextStream out(&file);
		out << m_libPath;
		file.close();
	}*/

	// !!! translator must be installed before any widgets are created !!!
	m_translationPath = FolderUtils::getApplicationSubFolderPath("translations");

	bool loaded = findTranslator(m_translationPath);
	Q_UNUSED(loaded);

	Q_INIT_RESOURCE(phoenixresources);

	MainWindow::initNames();
	FSvgRenderer::calcPrinterScale();
	ViewIdentifierClass::initNames();
	RatsnestColors::initNames();
	Wire::initNames();
	ItemBase::initNames();
	ViewLayer::initNames();
	Connector::initNames();
	Helper::initText();
	PartsEditorMainWindow::initText();
	BinManager::initNames();
	PaletteModel::initNames();
	SvgIconWidget::initNames();
	PinHeader::initNames();
	CursorMaster::initCursors();

	return true;
}

FApplication::~FApplication(void)
{
	cleanupBackups();
		
	clearModels();

	if (m_updateDialog) {
		delete m_updateDialog;
	}

	FSvgRenderer::cleanup();
	ViewIdentifierClass::cleanup();
	ViewLayer::cleanup();
	ItemBase::cleanup();
	Wire::cleanup();
	DebugDialog::cleanup();
	ViewSwitcher::cleanup();
	ItemDrag::cleanup();
	Version::cleanup();
	TipsAndTricks::cleanup();
	TranslatorListModel::cleanup();
	FolderUtils::cleanup();
	SearchLineEdit::cleanup();
	RatsnestColors::cleanup();
	HtmlInfoView::cleanup();
	SvgIconWidget::cleanup();
	PartFactory::cleanup();
}

void FApplication::clearModels() {
	if (m_paletteBinModel) {
		m_paletteBinModel->clearPartHash();
		delete m_paletteBinModel;
	}
	if (m_referenceModel) {
		m_referenceModel->clearPartHash();
		delete m_referenceModel;
	}
}

bool FApplication::spaceBarIsPressed() {
	return ((FApplication *) qApp)->m_spaceBarIsPressed;
}


bool FApplication::eventFilter(QObject *obj, QEvent *event)
{
	// check whether the space bar is down.

	Q_UNUSED(obj);

	switch (event->type()) {
		case QEvent::MouseButtonPress:
			m_mousePressed = true;
			break;
		case QEvent::MouseButtonRelease:
			m_mousePressed = false;
			break;
		case QEvent::KeyPress:
			{
				if (!m_mousePressed) {
					QKeyEvent * kevent = static_cast<QKeyEvent *>(event);
					if (!kevent->isAutoRepeat() && (kevent->key() == Qt::Key_Space)) {
						m_spaceBarIsPressed = true;
						emit spaceBarIsPressedSignal(true);
					}
				}
			}
			break;
		case QEvent::KeyRelease:
			{
				if (m_spaceBarIsPressed) {
					QKeyEvent * kevent = static_cast<QKeyEvent *>(event);
					if (!kevent->isAutoRepeat() && (kevent->key() == Qt::Key_Space)) {
						m_spaceBarIsPressed = false;
						emit spaceBarIsPressedSignal(false);
					}
				}
			}
			break;
		default:
			break;
	}

	return false;
}


bool FApplication::event(QEvent *event)
{
    switch (event->type()) {
		case QEvent::FileOpen:
			{
				QString path = static_cast<QFileOpenEvent *>(event)->file();
				DebugDialog::debug(QString("file open %1").arg(path));
				if (m_started) {
					loadNew(path);
				}
				else {
					m_filesToLoad.append(path);
				}

			}
			return true;
		default:
			return QApplication::event(event);
    }
}

bool FApplication::findTranslator(const QString & translationsPath) {
	QSettings settings;
	QString suffix = settings.value("language").toString();
	if (suffix.isEmpty()) {
		suffix = QLocale::system().name();	   // Returns the language and country of this locale as a string of the form "language_country", where language is a lowercase, two-letter ISO 639 language code, and country is an uppercase, two-letter ISO 3166 country code.
	}
	else {
		QLocale::setDefault(QLocale(suffix));
	}

    bool loaded = m_translator.load(QString("fritzing_") + suffix, translationsPath);
	if (loaded) {
		QApplication::installTranslator(&m_translator);
	}

	return loaded;
}

void FApplication::registerFonts() {
	registerFont(":/resources/fonts/DroidSans.ttf", true);
	registerFont(":/resources/fonts/DroidSans-Bold.ttf", false);
	registerFont(":/resources/fonts/DroidSansMono.ttf", false);
	//registerFont(":/resources/fonts/ocra10.ttf", true);
	registerFont(":/resources/fonts/OCRA.otf", true);

	/*	
		QFontDatabase database;
		QStringList families = database.families (  );
		foreach (QString string, families) {
			DebugDialog::debug(string);			// should print out the name of the fonts you loaded
		}
	*/	
}

ReferenceModel * FApplication::loadReferenceModel() {
	m_referenceModel = new CurrentReferenceModel();	
	ItemBase::setReferenceModel(m_referenceModel);
	connect(m_referenceModel, SIGNAL(loadedPart(int, int)), this, SLOT(loadedPart(int, int)));
	m_referenceModel->loadAll(true);								// this is very slow
	m_paletteBinModel = new PaletteModel(true, false, false);
	//DebugDialog::debug("after new palette model");
	return m_referenceModel;
}

bool FApplication::loadBin(QString binToOpen) {
	binToOpen = binToOpen.isNull() || binToOpen.isEmpty()? BinManager::CorePartsBinLocation: binToOpen;

	if (!m_paletteBinModel->load(binToOpen, m_referenceModel)) {
		if(binToOpen == BinManager::CorePartsBinLocation
		   || !m_paletteBinModel->load(BinManager::CorePartsBinLocation, m_referenceModel)) {
			return false;
		}
	}

	return true;
}

MainWindow * FApplication::loadWindows(int & loaded, bool lockFiles) {
	// our MainWindows use WA_DeleteOnClose so this has to be added to the heap (via new) rather than the stack (for local vars)
	MainWindow * mainWindow = MainWindow::newMainWindow(m_paletteBinModel, m_referenceModel, "", false, lockFiles);   // this is also slow
	mainWindow->setReportMissingModules(false);

	loaded = 0;
	initFilesToLoad();
	foreach (QString file, m_filesToLoad) {
		loadOne(mainWindow, file, loaded++);
	}

	//DebugDialog::debug("after argc");

	return mainWindow;
}

int FApplication::serviceStartup() {

	if (m_outputFolder.isEmpty()) {
		return -1;
	}

	switch (m_serviceType) {
		case GedaService:
			runGedaService();
			return 0;
	
		case KicadFootprintService:
			runKicadFootprintService();
			return 0;

		case KicadSchematicService:
			runKicadSchematicService();
			return 0;

		case GerberService:
			runGerberService();
			return 0;

		case PanelizerService:
			runPanelizerService();
			return 0;

		case InscriptionService:
			runInscriptionService();
			return 0;

		case ExampleService:
			runExampleService();
			return 0;

		default:
			DebugDialog::debug("unknown service");
			return -1;
	}
}


void FApplication::runGerberService()
{
	createUserDataStoreFolderStructure();

	registerFonts();
	loadReferenceModel();
	if (!loadBin("")) {
		return;
	}

	QDir dir(m_outputFolder);
	QString s = dir.absolutePath();
	QStringList filters;
	filters << "*" + FritzingBundleExtension;
	QStringList filenames = dir.entryList(filters, QDir::Files);
	foreach (QString filename, filenames) {
		QString filepath = dir.absoluteFilePath(filename);
		int loaded = 0;
		MainWindow * mainWindow = loadWindows(loaded, false);
		mainWindow->noBackup();
		m_started = true;

		FolderUtils::setOpenSaveFolderAux(m_outputFolder);
		if (mainWindow->loadWhich(filepath, false, false, "")) {
			mainWindow->exportToGerber(m_outputFolder);
		}

		mainWindow->setCloseSilently(true);
		mainWindow->close();
	}
}

void FApplication::runGedaService() {
	try {
		QDir dir(m_outputFolder);
		QStringList filters;
		filters << "*.fp";
		QStringList filenames = dir.entryList(filters, QDir::Files);
		foreach (QString filename, filenames) {
			QString filepath = dir.absoluteFilePath(filename);
			QString newfilepath = filepath;
			newfilepath.replace(".fp", ".svg");
			GedaElement2Svg geda;
			QString svg = geda.convert(filepath, false);
			QFile file(newfilepath);
			if (file.open(QFile::WriteOnly)) {
				QTextStream stream(&file);
				stream.setCodec("UTF-8");
				stream << svg;
				file.close();
			}
		}
	}
	catch (const QString & msg) {
		DebugDialog::debug(msg);
	}
	catch (...) {
		DebugDialog::debug("who knows");
	}
}

void FApplication::runKicadFootprintService() {
	QDir dir(m_outputFolder);
	QStringList filters;
	filters << "*.mod";
	QStringList filenames = dir.entryList(filters, QDir::Files);
	foreach (QString filename, filenames) {
		QString filepath = dir.absoluteFilePath(filename);
		QStringList moduleNames = KicadModule2Svg::listModules(filepath);
		foreach (QString moduleName, moduleNames) {
			KicadModule2Svg kicad;
			try {
				QString svg = kicad.convert(filepath, moduleName, false);
				if (svg.isEmpty()) {
					DebugDialog::debug("svg is empty " + filepath + " " + moduleName);
					continue;
				}
				foreach (QChar c, QString("<>:\"/\\|?*")) {
					moduleName.remove(c);
				}

				QString newFilePath = dir.absoluteFilePath(moduleName + "_" + filename);
				newFilePath.replace(".mod", ".svg");
				QFile file(newFilePath);
				if (file.open(QFile::WriteOnly)) {
					QTextStream stream(&file);
					stream.setCodec("UTF-8");
					stream << svg;
					file.close();
				}
				else {
					DebugDialog::debug("unable to open file " + newFilePath);
				}
			}
			catch (const QString & msg) {
				DebugDialog::debug(msg);
			}
			catch (...) {
				DebugDialog::debug("who knows");
			}
		}
	}
}

void FApplication::runKicadSchematicService() {
	QDir dir(m_outputFolder);
	QStringList filters;
	filters << "*.lib";
	QStringList filenames = dir.entryList(filters, QDir::Files);
	foreach (QString filename, filenames) {
		QString filepath = dir.absoluteFilePath(filename);
		QStringList defNames = KicadSchematic2Svg::listDefs(filepath);
		foreach (QString defName, defNames) {
			KicadSchematic2Svg kicad;
			try {
				QString svg = kicad.convert(filepath, defName);
				if (svg.isEmpty()) {
					DebugDialog::debug("svg is empty " + filepath + " " + defName);
					continue;
				}
				foreach (QChar c, QString("<>:\"/\\|?*")) {
					defName.remove(c);
				}

				//DebugDialog::debug(QString("converting %1 %2").arg(defName).arg(filename));

				QString newFilePath = dir.absoluteFilePath(defName + "_" + filename);
				newFilePath.replace(".lib", ".svg");
				QFile file(newFilePath);
				if (file.open(QFile::WriteOnly)) {
					QTextStream stream(&file);
					stream.setCodec("UTF-8");
					stream << svg;
					file.close();
				}
				else {
					DebugDialog::debug("unable to open file " + newFilePath);
				}
			}
			catch (const QString & msg) {
				DebugDialog::debug(msg);
			}
			catch (...) {
				DebugDialog::debug("who knows");
			}
		}
	}
}

int FApplication::startup(bool firstRun)
{
	//DebugDialog::setEnabled(true);

	QString splashName = ":/resources/images/splash/splash_screen_start.png";
	QDateTime now = QDateTime::currentDateTime();
	if (now.date().month() == 4 && now.date().day() == 1) {
		QString aSplashName = ":/resources/images/splash/april1st.png";
		QFileInfo info(aSplashName);
		if (info.exists()) {
			splashName = aSplashName;
		}
	}

	QPixmap pixmap(splashName);
	FSplashScreen splash(pixmap);
	m_splash = &splash;
	ProcessEventBlocker::processEvents();								// seems to need this (sometimes?) to display the splash screen

	initSplash(splash);
	ProcessEventBlocker::processEvents();

	// DebugDialog::debug("Data Location: "+QDesktopServices::storageLocation(QDesktopServices::DataLocation));

	if(firstRun) {		
		registerFonts();

		if (m_progressIndex >= 0) splash.showProgress(m_progressIndex, LoadProgressStart);
		ProcessEventBlocker::processEvents();

		#ifdef Q_WS_WIN
			// associate .fz file with fritzing app on windows (xp only--vista is different)
			// TODO: don't change settings if they're already set?
			// TODO: only do this at install time?
			QSettings settings1("HKEY_CLASSES_ROOT\\Fritzing", QSettings::NativeFormat);
			settings1.setValue(".", "Fritzing Application");
			foreach (QString extension, fritzingExtensions()) {
				QSettings settings2("HKEY_CLASSES_ROOT\\" + extension, QSettings::NativeFormat);
				settings2.setValue(".", "Fritzing");
			}
			QSettings settings3("HKEY_CLASSES_ROOT\\Fritzing\\shell\\open\\command", QSettings::NativeFormat);
			settings3.setValue(".", QString("\"%1\" \"%2\"")
							   .arg(QDir::toNativeSeparators(QApplication::applicationFilePath()))
							   .arg("%1") );
		#endif


		cleanFzzs();
	} 
	else 
	{
		clearModels();
		FSvgRenderer::cleanup();
	}

	loadReferenceModel();

	QString prevVersion;
	{
		// put this in a block so that QSettings is closed
		QSettings settings;
		prevVersion = settings.value("version").toString();
		QString currVersion = Version::versionString();
		if(prevVersion != currVersion) {
			QVariant pid = settings.value("pid");
			settings.clear();
			if (!pid.isNull()) {
				settings.setValue("pid", pid);
			}
		}
	}

	//bool fabEnabled = settings.value(ORDERFABENABLED, QVariant(false)).toBool();
	//if (!fabEnabled) {
		NetworkAccessManager = new QNetworkAccessManager(this);
		connect(NetworkAccessManager, SIGNAL(finished(QNetworkReply *)), this, SLOT(gotOrderFab(QNetworkReply *)));

		NetworkAccessManager->get(QNetworkRequest(QUrl(QString("http://fab.fritzing.org/launched%1").arg(makeRequestParamsString()))));
	//}

	if (m_progressIndex >= 0) splash.showProgress(m_progressIndex, LoadProgressEnd);

	createUserDataStoreFolderStructure();

	//DebugDialog::debug("after createUserDataStoreFolderStructure");

	if (m_progressIndex >= 0) splash.showProgress(m_progressIndex, 0.65);
	ProcessEventBlocker::processEvents();

	QSettings settings;
	if (!loadBin(settings.value("lastBin").toString())) {
			// TODO: we're really screwed, what now?
		QMessageBox::warning(NULL, QObject::tr("Fritzing"), QObject::tr("Friting cannot load the parts bin"));
		return -1;
	}

	if (m_progressIndex >= 0) splash.showProgress(m_progressIndex, 0.825);
	ProcessEventBlocker::processEvents();

	m_updateDialog = new UpdateDialog();
	connect(m_updateDialog, SIGNAL(enableAgainSignal(bool)), this, SLOT(enableCheckUpdates(bool)));
	checkForUpdates(false);

	if (m_progressIndex >= 0) splash.showProgress(m_progressIndex, 0.875);

	DebugDialog::debug("load something");
	loadSomething(firstRun, prevVersion);
	m_started = true;

	if (m_progressIndex >= 0) splash.showProgress(m_progressIndex, 0.99);
	ProcessEventBlocker::processEvents();

	m_splash = NULL;
	return 0;
}

void FApplication::registerFont(const QString &fontFile, bool reallyRegister) {
	int id = QFontDatabase::addApplicationFont(fontFile);
	if(id > -1 && reallyRegister) {
		QStringList familyNames = QFontDatabase::applicationFontFamilies(id);
		QFileInfo finfo(fontFile);
		foreach (QString family, familyNames) {
			InstalledFonts::InstalledFontsNameMapper.insert(family, finfo.baseName());
			InstalledFonts::InstalledFontsList << family;
			//DebugDialog::debug("registering font family: "+family);
		}
	}
}

void FApplication::finish()
{
	QString currVersion = Version::versionString();
	QSettings settings;
    settings.setValue("version", currVersion);
}

void FApplication::loadNew(QString path) {
	MainWindow * mw = MainWindow::newMainWindow(m_paletteBinModel, m_referenceModel, path, true, true);
	if (!mw->loadWhich(path, false, true, "")) {
		mw->close();
	}
	mw->clearFileProgressDialog();
}

void FApplication::loadOne(MainWindow * mw, QString path, int loaded) {
	if (loaded == 0) {
		mw->showFileProgressDialog(path);
		mw->loadWhich(path, true, true, "");
	}
	else {
		loadNew(path);
	}
}

void FApplication::preferences() {
	QDir dir(m_translationPath);
	QStringList nameFilters;
	nameFilters << "*.qm";
    QFileInfoList list = dir.entryInfoList(nameFilters, QDir::Files | QDir::NoSymLinks);
	QSettings settings;
	QString language = settings.value("language").toString();
	if (language.isEmpty()) {
		language = QLocale::system().name();
	}

	MainWindow * mainWindow = NULL;
	foreach (MainWindow * mw, orderedTopLevelMainWindows()) {
		mainWindow = mw;
		break;
	}

	if (mainWindow == NULL) return;			// shouldn't happen (maybe on the mac)

	PrefsDialog prefsDialog(language, NULL);			// TODO: use the topmost MainWindow as parent
	int ix = 0;
	foreach (SketchWidget * sketchWidget, mainWindow->sketchWidgets()) {
		prefsDialog.initViewInfo(ix++,  sketchWidget->viewName(), sketchWidget->getShortName(), 
									sketchWidget->defaultGridSizeInches(), 
									sketchWidget->background(), sketchWidget->standardBackground(),
									sketchWidget->curvyWires());
	}

	prefsDialog.initLayout(list);
	if (QDialog::Accepted == prefsDialog.exec()) {
		updatePrefs(prefsDialog);
	}
}

void FApplication::updatePrefs(PrefsDialog & prefsDialog)
{
	QSettings settings;

	if (prefsDialog.cleared()) {
		settings.clear();
		return;
	}

	QHash<QString, QString> hash = prefsDialog.settings();
	QList<MainWindow *> mainWindows = orderedTopLevelMainWindows();
	foreach (QString key, hash.keys()) {
		settings.setValue(key, hash.value(key));
		if (key.compare("connectedColor") == 0) {
			QColor c(hash.value(key));
			ItemBase::setConnectedColor(c);
			foreach (MainWindow * mainWindow, mainWindows) {
				mainWindow->redrawSketch();
			}
		}
		else if (key.compare("unconnectedColor") == 0) {
			QColor c(hash.value(key));
			ItemBase::setUnconnectedColor(c);
			foreach (MainWindow * mainWindow, mainWindows) {
				mainWindow->redrawSketch();
			}
		}
		else if (key.compare("wheelMapping") == 0) {
			ZoomableGraphicsView::setWheelMapping((ZoomableGraphicsView::WheelMapping) hash.value(key).toInt());
		}
		else if (key.compare("autosavePeriod") == 0) {
			MainWindow::setAutosavePeriod(hash.value(key).toInt());
		}
		else if (key.compare("autosaveEnabled") == 0) {
			MainWindow::setAutosaveEnabled(hash.value(key).toInt());
		}
		else if (key.contains("gridsize", Qt::CaseInsensitive)) {
			foreach (MainWindow * mainWindow, mainWindows) {
				foreach (SketchWidget * sketchWidget, mainWindow->sketchWidgets()) {
					if (key.contains(sketchWidget->viewName())) {
						sketchWidget->initGrid();
					}
				}
			}
		}
		else if (key.contains("curvy", Qt::CaseInsensitive)) {
			foreach (MainWindow * mainWindow, mainWindows) {
				foreach (SketchWidget * sketchWidget, mainWindow->sketchWidgets()) {
					if (key.contains(sketchWidget->getShortName())) {
						sketchWidget->setCurvyWires(hash.value(key).compare("1") == 0);
					}
				}
			}
		}
	}

	hash = prefsDialog.tempSettings();
	foreach (QString key, hash.keys()) {
		if (key.contains("background", Qt::CaseInsensitive)) {
			foreach (MainWindow * mainWindow, mainWindows) {
				foreach (SketchWidget * sketchWidget, mainWindow->sketchWidgets()) {
					if (key.contains(sketchWidget->getShortName())) {
						sketchWidget->setBackground(hash.value(key));
					}
				}
			}
		}
	}
}

void FApplication::initSplash(FSplashScreen & splash) {
	QPixmap logo(":/resources/images/splash/fhp_logo_small.png");
	QPixmap progress(":/resources/images/splash/splash_progressbar.png");

	m_progressIndex = splash.showPixmap(progress, "progress");
	if (m_progressIndex >= 0) splash.showProgress(m_progressIndex, 0);

	// put this above the progress indicator
	splash.showPixmap(logo, "fhpLogo");

	QString msg1 = QObject::tr("<font face='Lucida Grande, Tahoma, Sans Serif' size='2' color='#eaf4ed'>"
							   "&#169; 2007-%1 Fachhochschule Potsdam"
							   "</font>")
							.arg(Version::year());
	splash.showMessage(msg1, "fhpText", Qt::AlignLeft | Qt::AlignTop);

	QString macBuildType;
#ifdef Q_WS_MAC
#ifdef QT_MAC_USE_COCOA
	macBuildType = " Cocoa";
#else
	macBuildType = " Carbon";
#endif
#endif
	QString msg2 = QObject::tr("<font face='Lucida Grande, Tahoma, Sans Serif' size='2' color='#eaf4ed'>"
							   "Version %1.%2.%3 (%4%5)%6"
							   "</font>")
						.arg(Version::majorVersion())
						.arg(Version::minorVersion())
						.arg(Version::minorSubVersion())
						.arg(Version::modifier())
						.arg(Version::shortDate())
						.arg(macBuildType);
	splash.showMessage(msg2, "versionText", Qt::AlignRight | Qt::AlignTop);
    splash.show();
}

struct Thing {
        QString moduleID;
        ViewIdentifierClass::ViewIdentifier viewIdentifier;
        ViewLayer::ViewLayerID viewLayerID;
};



void FApplication::checkForUpdates() {
	checkForUpdates(true);
}

void FApplication::checkForUpdates(bool atUserRequest)
{
	if (atUserRequest) {
		enableCheckUpdates(false);
	}

	VersionChecker * versionChecker = new VersionChecker();

	QSettings settings;
	if (!atUserRequest) {
		// if I've already been notified about these updates, don't bug me again
		QString lastMainVersionChecked = settings.value("lastMainVersionChecked").toString();
		if (!lastMainVersionChecked.isEmpty()) {
			versionChecker->ignore(lastMainVersionChecked, false);
		}
		QString lastInterimVersionChecked = settings.value("lastInterimVersionChecked").toString();
		if (!lastInterimVersionChecked.isEmpty()) {
			versionChecker->ignore(lastInterimVersionChecked, true);
		}
	}

    QString atom = QString("http://fritzing.org/download/feed/atom/%1/%2")
		.arg(PLATFORM_NAME)
		.arg(makeRequestParamsString());
    DebugDialog::debug(atom);
    versionChecker->setUrl(atom);
	m_updateDialog->setAtUserRequest(atUserRequest);
	m_updateDialog->setVersionChecker(versionChecker);

	if (atUserRequest) {
		m_updateDialog->show();
	}
}

void FApplication::enableCheckUpdates(bool enabled)
{
	DebugDialog::debug("before enable check updates");
    foreach (QWidget *widget, QApplication::topLevelWidgets()) {
        MainWindow *mainWindow = qobject_cast<MainWindow *>(widget);
		if (mainWindow) {
			mainWindow->enableCheckUpdates(enabled);
		}
	}
	DebugDialog::debug("after enable check updates");
}


void FApplication::createUserDataStoreFolderStructure() {
	// make sure that the folder structure for parts and bins, exists
	QString userDataStorePath = FolderUtils::getUserDataStorePath();
	QDir dataStore(userDataStorePath);
	QStringList dataFolders = FolderUtils::getUserDataStoreFolders();
	foreach(QString folder, dataFolders) {
		if(!QFileInfo(dataStore.absolutePath()+folder).exists()) {
			QString folderaux = folder.startsWith("/")? folder.remove(0,1): folder;
			dataStore.mkpath(folder);
		}
	}

    copyBin(BinManager::MyPartsBinLocation, BinManager::MyPartsBinTemplateLocation);
    copyBin(BinManager::SearchBinLocation, BinManager::SearchBinTemplateLocation);
	PartFactory::initFolder();

}

void FApplication::copyBin(const QString & dest, const QString & source) {
    if(QFileInfo(dest).exists()) return;

    // this copy action, is not working on windows, because is a resources file
    if(!QFile(source).copy(dest)) {
#ifdef Q_WS_WIN // may not be needed from qt 4.5.2 on
        DebugDialog::debug("Failed to copy a file from the resources");
        ProcessEventBlocker::processEvents();
        QDir binsFolder = QFileInfo(dest).dir().absolutePath();
        QStringList binFiles = binsFolder.entryList(QDir::AllEntries | QDir::NoDotAndDotDot);
        foreach(QString binName, binFiles) {
            if(binName.startsWith("qt_temp.")) {
                QString filePath = binsFolder.absoluteFilePath(binName);
                bool success = QFile(filePath).rename(dest);
                Q_UNUSED(success);
                break;
            }
        }
#endif
    }
    QFlags<QFile::Permission> ps = QFile::permissions(dest);
    QFile::setPermissions(
        dest,
        QFile::WriteOwner | QFile::WriteUser | ps
#ifdef Q_WS_WIN
        | QFile::WriteOther | QFile::WriteGroup
#endif

    );
}

void FApplication::changeActivation(bool activate, QWidget * originator) {
	if (!activate) return;

	//DebugDialog::debug(QString("change activation %1 %2").arg(activate).arg(originator->metaObject()->className()));

	FritzingWindow * fritzingWindow = qobject_cast<FritzingWindow *>(originator);
	if (fritzingWindow == NULL) {
		fritzingWindow = qobject_cast<FritzingWindow *>(originator->parent());
	}
	if (fritzingWindow == NULL) return;

	m_orderedTopLevelWidgets.removeOne(fritzingWindow);
	m_orderedTopLevelWidgets.push_front(fritzingWindow);

	m_activationTimer.stop();
	m_activationTimer.start();
}

void FApplication::updateActivation() {
	//DebugDialog::debug("updating activation");

	FritzingWindow * prior = m_lastTopmostWindow; 
	m_lastTopmostWindow = NULL;
	if (m_orderedTopLevelWidgets.count() > 0) {
		m_lastTopmostWindow = qobject_cast<FritzingWindow *>(m_orderedTopLevelWidgets.at(0));
	}

	if (prior == m_lastTopmostWindow) {
		//DebugDialog::debug("done updating activation");
		return;
	}

	//DebugDialog::debug(QString("last:%1, new:%2").arg((long) prior, 0, 16).arg((long) m_lastTopmostWindow.data(), 0, 16));

	MainWindow * priorMainWindow = qobject_cast<MainWindow *>(prior);
	if (priorMainWindow != NULL) {			
		priorMainWindow->saveDocks();
	}

	MainWindow * lastTopmostMainWindow = qobject_cast<MainWindow *>(m_lastTopmostWindow);
	if (lastTopmostMainWindow != NULL) {
		lastTopmostMainWindow->restoreDocks();
		//DebugDialog::debug("restoring active window");
	}

	//DebugDialog::debug("done 2 updating activation");

}

void FApplication::topLevelWidgetDestroyed(QObject * object) {
	QWidget * widget = qobject_cast<QWidget *>(object);
	if (widget) {
		m_orderedTopLevelWidgets.removeOne(widget);
	}
}

void FApplication::closeAllWindows2() {
/*
Ok, near as I can tell, here's what's going on.  When you quit fritzing, the function 
QApplication::closeAllWindows() is invoked.  This goes through the top-level window 
list in random order and calls close() on each window, until some window says "no".  
The QGraphicsProxyWidgets must contain top-level windows, and at least on the mac, their response to close() 
seems to be setVisible(false).  The random order explains why different icons 
disappear, or sometimes none at all.  

So the hack for now is to call the windows in non-random order.

Eventually, maybe the SvgIconWidget class could be rewritten so that it's not using QGraphicsProxyWidget, 
which is really not intended for hundreds of widgets. 

(SvgIconWidget has been rewritten)
*/


// this code modified from QApplication::closeAllWindows()


    bool did_close = true;
    QWidget *w;
    while((w = QApplication::activeModalWidget()) && did_close) {
        if(!w->isVisible())
            break;
        did_close = w->close();
    }
	if (!did_close) return;

    QWidgetList list = QApplication::topLevelWidgets();
    for (int i = 0; did_close && i < list.size(); ++i) {
        w = list.at(i);
        FritzingWindow *fWindow = qobject_cast<FritzingWindow *>(w);
		if (fWindow == NULL) continue;

        if (w->isVisible() && w->windowType() != Qt::Desktop) {
            did_close = w->close();
            list = QApplication::topLevelWidgets();
            i = -1;
        }
    }
	if (!did_close) return;

    list = QApplication::topLevelWidgets();
    for (int i = 0; did_close && i < list.size(); ++i) {
        w = list.at(i);
        if (w->isVisible() && w->windowType() != Qt::Desktop) {
            did_close = w->close();
            list = QApplication::topLevelWidgets();
            i = -1;
        }
    }

}

bool FApplication::runAsService() {
	return ((FApplication *) qApp)->m_serviceType != NoService;
}

void FApplication::loadedPart(int loaded, int total) {
	if (total == 0) return;
	if (m_splash == NULL) return;

	//DebugDialog::debug(QString("loaded %1 %2").arg(loaded).arg(total));
	if (m_progressIndex >= 0) m_splash->showProgress(m_progressIndex, LoadProgressStart + ((LoadProgressEnd - LoadProgressStart) * loaded / (double) total));
}

void FApplication::externalProcessSlot(QString &name, QString &path, QStringList &args) {
	name = m_externalProcessName;
	path = m_externalProcessPath;
	args = m_externalProcessArgs;
}

bool FApplication::notify(QObject *receiver, QEvent *e)
{
    try {
        //qDebug() << QString("notify %1 %2").arg(receiver->metaObject()->className()).arg(e->type());
        return QApplication::notify(receiver, e);
    }
	catch (char const *str) {
        QMessageBox::critical(NULL, tr("Fritzing failure"), tr("Fritzing caught an exception %1 from %2 in event %3")
			.arg(str).arg(receiver->objectName()).arg(e->type()));
	}
    catch (...) {
        QMessageBox::critical(NULL, tr("Fritzing failure"), tr("Fritzing caught an exception from %1 in event %2").arg(receiver->objectName()).arg(e->type()));
    }
	closeAllWindows2();
	QApplication::exit(-1);
	abort();
    return false;
}

void FApplication::loadSomething(bool firstRun, const QString & prevVersion) {
    // At this point we're trying to determine what sketches to load which are from one of the following sources:
    // Only one of these sources will actually provide sketches to load and they're listed in order of priority:

    //		We found sketch backups to recover
	//		it's a restart (!firstRun)
	//		there's a previous version (open an empty sketch)
	//		files were double-clicked
    //		The last opened sketch
    //		A new blank sketch

	initFilesToLoad();   // sets up m_filesToLoad from the command line on PC and Linux; mac uses a FileOpen event instead

	initBackups();

	DebugDialog::debug("checking for backups");
    QList<MainWindow*> sketchesToLoad = recoverBackups();

	bool loadPrevious = false;
	if (sketchesToLoad.isEmpty()) {
		loadPrevious = !prevVersion.isEmpty() && Version::greaterThan(prevVersion, Version::FirstVersionWithDetachedUserData);
	}

	DebugDialog::debug(QString("load previous %1").arg(loadPrevious));

	if (!loadPrevious && sketchesToLoad.isEmpty()) {
		if (!firstRun) {
			DebugDialog::debug(QString("not first run"));
			sketchesToLoad = loadLastOpenSketch();
		}
	}

	if (!loadPrevious && sketchesToLoad.isEmpty()) {
		// Check for double-clicked files to load
		DebugDialog::debug(QString("check files to load %1").arg(m_filesToLoad.count()));

        foreach (QString filename, m_filesToLoad) {
            DebugDialog::debug(QString("Loading non-service file %1").arg(filename));
            MainWindow *mainWindow = MainWindow::newMainWindow(m_paletteBinModel, m_referenceModel, filename, true, true);
            mainWindow->loadWhich(filename, true, true, "");
            sketchesToLoad << mainWindow;
        }
	}

    // Find any previously open sketches to reload
    if (!loadPrevious && sketchesToLoad.isEmpty()) {
		DebugDialog::debug(QString("load last open"));
		sketchesToLoad = loadLastOpenSketch();
	}

	MainWindow * newBlankSketch = NULL;
	if (sketchesToLoad.isEmpty()) {
		DebugDialog::debug(QString("create empty sketch"));
		newBlankSketch = MainWindow::newMainWindow(m_paletteBinModel, m_referenceModel, "", true, true);
		if (newBlankSketch) {
			// make sure to start an empty sketch with a board
			newBlankSketch->addDefaultParts();   // do this before call to show()
			sketchesToLoad << newBlankSketch;
		}
	}

	DebugDialog::debug(QString("finish up sketch loading"));

    // Finish loading the sketches and show them to the user
	foreach (MainWindow* sketch, sketchesToLoad) {
		sketch->show();
		sketch->clearFileProgressDialog();
	}

	if (loadPrevious) {
		doLoadPrevious(newBlankSketch);
	}
	else if (newBlankSketch) {
		newBlankSketch->hideTempBin();
	}
}

QList<MainWindow *> FApplication::loadLastOpenSketch() {
	QList<MainWindow *> sketches;
	QSettings settings;
	if(settings.value("lastOpenSketch").isNull()) return sketches;

	QString lastSketchPath = settings.value("lastOpenSketch").toString();
	if(!QFileInfo(lastSketchPath).exists()) {
		settings.remove("lastOpenSketch");
		return sketches;
	}

    DebugDialog::debug(QString("Loading last open sketch %1").arg(lastSketchPath));
    settings.remove("lastOpenSketch");				// clear the preference, in case the load crashes
    MainWindow *mainWindow = MainWindow::newMainWindow(m_paletteBinModel, m_referenceModel, lastSketchPath, true, true);
    mainWindow->loadWhich(lastSketchPath, true, true, "");
    sketches << mainWindow;
    settings.setValue("lastOpenSketch", lastSketchPath);	// the load works, so restore the preference
	return sketches;
}

void FApplication::doLoadPrevious(MainWindow * sketchWindow) {
	// Here we check if files need to be imported from an earlier version.
    // This should be done before any files are loaded as it requires a restart.
    // As this can generate UI it should come after the splash screen has closed.

    QMessageBox messageBox(sketchWindow);
    messageBox.setWindowTitle(tr("Import files from previous version?"));
    messageBox.setText(tr("Do you want to import parts and bins that you have created with earlier versions of Fritzing?\n"));
    messageBox.setInformativeText(tr("\nNote: You can import them later using the \"Help\" > \"Import parts and bins "
									"from old version...\" menu action."));
    messageBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    messageBox.setDefaultButton(QMessageBox::Cancel);
    messageBox.setIcon(QMessageBox::Question);
    messageBox.setWindowModality(Qt::WindowModal);
    messageBox.setButtonText(QMessageBox::Ok, tr("Import"));
    messageBox.setButtonText(QMessageBox::Cancel, tr("Do not import now"));
    QMessageBox::StandardButton answer = (QMessageBox::StandardButton) messageBox.exec();

    if(answer == QMessageBox::Ok) {
		 sketchWindow->importFilesFromPrevInstall();
    }
}

QList<MainWindow *> FApplication::recoverBackups()
{	
	QFileInfoList backupList;
	LockManager::checkLockedFiles("backup", backupList, m_lockedFiles, false, LockManager::FastTime);
	for (int i = backupList.size() - 1; i >=0; i--) {
		QFileInfo fileInfo = backupList.at(i);
		if (!fileInfo.fileName().endsWith(FritzingSketchExtension)) {
			backupList.removeAt(i);
		}
	}

	QList<MainWindow*> recoveredSketches;
    if (backupList.size() == 0) return recoveredSketches;

    RecoveryDialog recoveryDialog(backupList);
	int result = (QMessageBox::StandardButton)recoveryDialog.exec();
    QList<QTreeWidgetItem*> fileItems = recoveryDialog.getFileList();
    DebugDialog::debug(QString("Recovering %1 files from recoveryDialog").arg(fileItems.size()));
    foreach (QTreeWidgetItem * item, fileItems) {
		QString backupName = item->data(0, Qt::UserRole).value<QString>();
        if (result == QDialog::Accepted && item->isSelected()) {
			QString originalBaseName = item->text(0);
            DebugDialog::debug(QString("Loading recovered sketch %1").arg(originalBaseName));
			
			QString originalPath = item->data(1, Qt::UserRole).value<QString>();
			QString fileExt;
			QString bundledFileName = FolderUtils::getSaveFileName(NULL, tr("Please specify an .fzz file name to save to (cancel will delete the backup)"), originalPath, tr("Fritzing (*%1)").arg(FritzingBundleExtension), &fileExt);
			if (!bundledFileName.isEmpty()) {
				MainWindow *currentRecoveredSketch = MainWindow::newMainWindow(m_paletteBinModel, m_referenceModel, originalBaseName, true, true);
    			currentRecoveredSketch->mainLoad(backupName, bundledFileName);
				currentRecoveredSketch->saveAsShareable(bundledFileName, true);
				currentRecoveredSketch->setCurrentFile(bundledFileName, true, true);
				currentRecoveredSketch->showAllFirstTimeHelp(false);
				recoveredSketches << currentRecoveredSketch;

				/*
				if (originalPath.startsWith(untitledFileName())) {
					DebugDialog::debug(QString("Comparing untitled documents: %1 %2").arg(filename).arg(untitledFileName()));
					QRegExp regexp("\\d+");
					int ix = regexp.indexIn(filename);
					int untitledSketchNumber = ix >= 0 ? regexp.cap(0).toInt() : 1;
					untitledSketchNumber++;
					DebugDialog::debug(QString("%1 untitled documents open, currently thinking %2").arg(untitledSketchNumber).arg(UntitledSketchIndex));
					UntitledSketchIndex = UntitledSketchIndex >= untitledSketchNumber ? UntitledSketchIndex : untitledSketchNumber;
				}
				*/



			}
        }

		QFile::remove(backupName);
    }

	return recoveredSketches;
}

void FApplication::initFilesToLoad()
{
	for (int i = 1; i < m_arguments.length(); i++) {
		QFileInfo fileinfo(m_arguments[i]);
		if (fileinfo.exists() && !fileinfo.isDir()) {
			m_filesToLoad << m_arguments[i];
		}
	}
}

void FApplication::initBackups() {
	LockManager::initLockedFiles("backup", MainWindow::BackupFolder, m_lockedFiles, LockManager::FastTime);
}

void FApplication::cleanupBackups() {
	LockManager::releaseLockedFiles(MainWindow::BackupFolder, m_lockedFiles);
}

void FApplication::gotOrderFab(QNetworkReply * networkReply) {
	int responseCode = networkReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
	if (responseCode == 200) {
		QSettings settings;
		settings.setValue(ORDERFABENABLED, QVariant(true));
	}
}

QString FApplication::makeRequestParamsString() {
	QSettings settings;
	if (settings.value("pid").isNull()) {
		settings.setValue("pid", FolderUtils::getRandText());
	}

	QtSystemInfo systemInfo(this);
	QString siVersion(QUrl::toPercentEncoding(Version::versionString()));
	QString siSystemName(QUrl::toPercentEncoding(systemInfo.systemName()));
	QString siSystemVersion(QUrl::toPercentEncoding(systemInfo.systemVersion()));
	QString siKernelName(QUrl::toPercentEncoding(systemInfo.kernelName()));
	QString siKernelVersion(QUrl::toPercentEncoding(systemInfo.kernelVersion()));
	QString siArchitecture(QUrl::toPercentEncoding(systemInfo.architectureName()));
    QString string = QString("?pid=%1&version=%2&sysname=%3&kernname=%4&kernversion=%5arch=%6&sysversion=%7")
		.arg(settings.value("pid").toString())
		.arg(siVersion)
		.arg(siSystemName)
		.arg(siKernelName)
		.arg(siKernelVersion)
		.arg(siArchitecture)
		.arg(siSystemVersion);
	return string;
}

void FApplication::runPanelizerService()
{	
	m_started = true;
	Panelizer::panelize(this, m_panelFilename);
}

void FApplication::runInscriptionService()
{	
	m_started = true;
	Panelizer::inscribe(this, m_panelFilename);
}

QList<MainWindow *> FApplication::orderedTopLevelMainWindows() {
	QList<MainWindow *> mainWindows;
	foreach (QWidget * widget, m_orderedTopLevelWidgets) {
		MainWindow * mainWindow = qobject_cast<MainWindow *>(widget);
		if (mainWindow) mainWindows.append(mainWindow);
	}
	return mainWindows;
}

void FApplication::runExampleService()
{	
	m_started = true;

	createUserDataStoreFolderStructure();
	registerFonts();
	loadReferenceModel();

	if (!loadBin("")) {
		DebugDialog::debug(QString("load bin failed"));
		return;
	}

	QDir sketchesDir(FolderUtils::getApplicationSubFolderPath("sketches"));
	runExampleService(sketchesDir);
}

void FApplication::runExampleService(QDir & dir) {
	QStringList nameFilters;
	nameFilters << ("*" + FritzingBundleExtension);   //  FritzingSketchExtension
	QFileInfoList fileList = dir.entryInfoList(nameFilters, QDir::Files | QDir::NoSymLinks);
	foreach (QFileInfo fileInfo, fileList) {
		QString path = fileInfo.absoluteFilePath();
		DebugDialog::debug("sketch file " + path);

		int loaded = 0;
		MainWindow * mainWindow = loadWindows(loaded, false);
		if (mainWindow == NULL) continue;

		mainWindow->noBackup();

		FolderUtils::setOpenSaveFolderAux(dir.absolutePath());

		if (!mainWindow->loadWhich(path, false, false, "")) {
			DebugDialog::debug(QString("failed to load"));
		}
		else {
			mainWindow->selectAllObsolete(false);
			mainWindow->swapObsolete(false);
			mainWindow->saveAsAux(path);    //   path + "z"
			mainWindow->setCloseSilently(true);
			mainWindow->close();
		}
	}

	QFileInfoList dirList = dir.entryInfoList(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::NoSymLinks);
	foreach (QFileInfo dirInfo, dirList) {
		QDir dir(dirInfo.filePath());
		runExampleService(dir);
	}
}

void FApplication::cleanFzzs() {
	QHash<QString, LockedFile *> lockedFiles;
	QString folder;
	LockManager::initLockedFiles("fzz", folder, lockedFiles, LockManager::SlowTime);
	QFileInfoList backupList;
	LockManager::checkLockedFiles("fzz", backupList, lockedFiles, true, LockManager::SlowTime);
	LockManager::releaseLockedFiles(folder, lockedFiles);
}
