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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QUndoView>
#include <QUndoGroup>
#include <QToolButton>
#include <QPushButton>
#include <QStackedWidget>
#include <QSizeGrip>
#include <QProcess>
#include <QDockWidget>

#include "fritzingwindow.h"
#include "sketchareawidget.h"
#include "viewlayer.h"
#include "sketchtoolbutton.h"
#include "program/programwindow.h"
#include "svg/svg2gerber.h"
#include "viewswitcher/viewswitcher.h"

QT_BEGIN_NAMESPACE
class QAction;
class QListWidget;
class QMenu;
QT_END_NAMESPACE

class Helper;
class DockManager;
class FSizeGrip;

bool sortPartList(ItemBase * b1, ItemBase * b2);

static const QString ORDERFABENABLED = "OrderFabEnabled";

class SwapTimer : public QTimer
{
Q_OBJECT

public:
	SwapTimer();

	void setAll(const QString & family, const QString & prop, QMap<QString, QString> &  propsMap, ItemBase *);
	const QString & family();
	const QString & prop();
	QMap<QString, QString> propsMap();
	ItemBase * itemBase();

protected:
	QString m_family;
	QString m_prop;
	QMap<QString, QString> m_propsMap;
	QPointer <ItemBase> m_itemBase;
};

class MainWindow : public FritzingWindow
{
    Q_OBJECT
public:
    MainWindow(class PaletteModel *, ReferenceModel *refModel, QWidget * parent);
    MainWindow(QFile & fileToLoad);
	~MainWindow();

    void mainLoad(const QString & fileName, const QString & displayName);
	bool loadWhich(const QString & fileName, bool setAsLastOpened, bool addToRecent, const QString & displayName);
	void notClosableForAWhile();
	QAction *raiseWindowAction();
	QSizeGrip *sizeGrip();
	QStatusBar *realStatusBar();
	void showAllFirstTimeHelp(bool show);
	void enableCheckUpdates(bool enabled);

	class PartsEditorMainWindow* getPartsEditor(ModelPart *modelPart, long id, ItemBase * fromItem, class PartsBinPaletteWidget* requester);
	ModelPart *loadPartFromFile(const QString& newPartPath, bool connectorsChanged=false);
	void addDefaultParts();
    void init(class PaletteModel *, ReferenceModel *refModel, bool lockFiles);
	void showFileProgressDialog(const QString & path);
	void setFileProgressPath(const QString & path);
	void clearFileProgressDialog();
	class FileProgressDialog * fileProgressDialog();

	const QString &selectedModuleID();

	void saveDocks();
	void restoreDocks();

	void redrawSketch();

	// if we consider a part as the smallest ("atomic") entity inside
	// fritzing, then this functions may help with the bundle tasks
	// on the complex entities: sketches, bins, modules (?)
	void saveBundledNonAtomicEntity(QString &filename, const QString &extension, Bundler *bundler, const QList<ModelPart*> &partsToSave, bool askForFilename, const QString & destFolderPath, bool saveModel, bool deleteLeftovers);
	void loadBundledNonAtomicEntity(const QString &filename, Bundler *bundler, bool addToBin, bool dontAsk);
	void saveAsShareable(const QString & path, bool saveModel);

	
	void setCurrentFile(const QString &fileName, bool addToRecent, bool setAsLastOpened);
	void setReportMissingModules(bool);
	QList<SketchWidget *> sketchWidgets();
	void setCloseSilently(bool);
	void exportToGerber(const QString & outputDir);
	class PCBSketchWidget * pcbView();
	void noBackup();
	void swapSelectedAux(ItemBase * itemBase, const QString & moduleID);
	bool saveAsAux(const QString & fileName);
	void swapObsolete(bool displayFeedback);
	void selectAllObsolete(bool displayFeedback);
	void hideTempBin();
	const QString & fritzingVersion();

public:
	static void initNames();
	static MainWindow * newMainWindow(PaletteModel * paletteModel, ReferenceModel *refModel, const QString & displayPath, bool showProgress, bool lockFiles);
	static void setAutosavePeriod(int);
	static void setAutosaveEnabled(bool);

signals:
	void alienPartsDismissed();
	void aboutToClose();
	void viewSwitched(int);
	void mainWindowMoved(QWidget *);
	void changeActivationSignal(bool activate, QWidget * originator);
	void externalProcessSignal(QString & name, QString & path, QStringList & args);

public slots:
	void ensureClosable();
	ModelPart* loadBundledPart(const QString &fileName, bool addToBin=true);
	void partsEditorClosed(long id);
	void importFilesFromPrevInstall();
	void acceptAlienFiles();
	void statusMessage(QString message, int timeout);
	void warnSMD(const QString & moduleID);
    void showPCBView();
	void groundFill();
	void removeGroundFill(bool force = false);
	void copperFill();
	void setOneGroundFillSeed();
	void setGroundFillSeeds();	
	void clearGroundFillSeeds();
	void changeBoardLayers(int layers, bool doEmit);
	void swapOne(ItemBase * itemBase, const QString & moduleID);
	void selectAllObsolete();
	void swapObsolete();
	void dropTempSlot(ModelPart *, QWidget *);

protected slots:
	void mainLoad();
	void openRecentOrExampleFile();
    void print();
    void doExport();
	void exportEtchable();
    void about();
	void tipsAndTricks();
    void copy();
    void cut();
    void paste();
	void pasteInPlace();
    void duplicate();
    void doDelete();
    void selectAll();
    void deselect();
    void zoomIn();
    void zoomOut();
    void fitInWindow();
    void actualSize();
	void hundredPercentSize();
    void alignToGrid();
    void showGrid();
    void showBreadboardView();
    void showSchematicView();
	void showPartsBinIconView();
	void showPartsBinListView();
    void updateEditMenu();
    void updateLayerMenu(bool resetLayout = false);
    void updatePartMenu();
    void updateWireMenu();
	void updateFileMenu();
    void updateTransformationActions();
	void updateRecentFileActions();
	void updateTraceMenu();
    void tabWidget_currentChanged(int index);
    void createNewPart();
    void createNewSketch();
    void minimize();
    void toggleToolbar(bool toggle);
    void togglePartLibrary(bool toggle);
    void toggleInfo(bool toggle);
    void toggleNavigator(bool toggle);
    void toggleUndoHistory(bool toggle);
	void toggleDebuggerOutput(bool toggle);
	void openHelp();
	void openDonate();
	void openExamples();
	void openPartsReference();
	void visitFritzingDotOrg();
	void updateWindowMenu();
	void pageSetup();
	void sendToBack();
	void sendBackward();
	void bringForward();
	void bringToFront();
	void rotate90cw();
	void rotate90ccw();
	void rotate180();
	void rotate45ccw();
	void rotate45cw();
	void rotateIncCCW();
	void rotateIncCW();
	void rotateIncCCWRubberBand();
	void rotateIncCWRubberBand();
	void flipHorizontal();
	void flipVertical();
	void showAllLayers();
	void hideAllLayers();
	void showInViewHelp();
	void addBendpoint();
	void flattenCurve();
	void disconnectAll();

	void openInPartsEditor();
	void openPartsEditor(PaletteItem *);

	void updateZoomSlider(double zoom);
	void updateZoomOptionsNoMatterWhat(double zoom);
	void updateViewZoom(double newZoom);

	void loadPart(const QString &newPartPath, long partsEditorId, bool connectorsChanged);

	void setInfoViewOnHover(bool infoViewOnHover);
	void updateItemMenu();

	void autoroute();
	void orderFab();
	void activeLayerTop();
	void activeLayerBottom();
	void activeLayerBoth();
	void toggleActiveLayer();
	void createTrace();
	void excludeFromAutoroute();
	void selectAllTraces();
	void selectAllCopperFill();
	void updateRoutingStatus();
	void selectAllExcludedTraces();
	void selectAllIncludedTraces();
	void selectAllJumperItems();
	void selectAllVias();

	void shareOnline();
	void saveBundledPart(const QString &moduleId=___emptyString___);
	QStringList saveBundledAux(ModelPart *mp, const QDir &destFolder);
	void loadBundledPart();

	void binSaved(bool hasAlienParts);
	void routingStatusSlot(SketchWidget *, const RoutingStatus &);

	void applyReadOnlyChange(bool isReadOnly);
	void currentNavigatorChanged(class MiniViewContainer *);
	void viewSwitchedTo(int viewIndex);

	void raiseAndActivate();
	void activateWindowAux();
	void showPartLabels();
	void addNote();
	void reportBug();
	void enableDebug();
	void tidyWires();
	void changeWireColor(bool checked);

	void startSaveInstancesSlot(const QString & fileName, ModelPart *, QXmlStreamWriter &);
	void loadedViewsSlot(ModelBase *, QDomElement & views);
	void loadedRootSlot(const QString & filename, ModelBase *, QDomElement & views);
	void exportNormalizedSVG();
	void exportNormalizedFlattenedSVG();

	void launchExternalProcess();
	bool externalProcess(QString & name, QString & path, QStringList & args);
	void processError(QProcess::ProcessError processError);
	void processFinished(int exitCode, QProcess::ExitStatus exitStatus);
	void processReadyRead();
	void processStateChanged(QProcess::ProcessState newState);
    void throwFakeException();

	void dropPaste(SketchWidget *);

	void openProgramWindow();
	void linkToProgramFile(const QString & filename, const QString & language, const QString & programmer, bool addLink, bool strong);
	void designRulesCheck();
	void subSwapSlot(SketchWidget *, ItemBase *, ViewLayer::ViewLayerSpec, QUndoCommand * parentCommand);
	void updateLayerMenuSlot();
	bool save();
	bool saveAs();
	void backupSketch();
	void undoStackCleanChanged(bool isClean);
	void autosaveNeeded(int index = 0);
	void firstTimeHelpHidden();
	void changeTraceLayer();
	void routingStatusLabelMousePress(QMouseEvent*);
	void routingStatusLabelMouseRelease(QMouseEvent*);
	void updateNet();
	void selectMoveLock();
	void moveLock();
	void showNavigator();
	void autorouterSettings();
	void warnSMDReally();
	void boardDeletedSlot();
	void cursorLocationSlot(double, double);
	void locationLabelClicked();
	void swapSelectedMap(const QString & family, const QString & prop, QMap<QString, QString> & currPropsMap, ItemBase *);
	void swapSelectedDelay(const QString & family, const QString & prop, QMap<QString, QString> & currPropsMap, ItemBase *);
	void swapSelectedTimeout();
	void filenameIfSlot(QString & filename);
	void openURL();
	void setActiveWire(Wire *);
	void setActiveConnectorItem(ConnectorItem *);

protected:
	void initSketchWidget(SketchWidget *);

    void createActions();
    void createFileMenuActions();
    void createExportActions();
    void createOrderFabAct();
    void createOpenExampleMenu();
    void populateMenuFromXMLFile(QMenu *parentMenu, QStringList &actionsTracker, const QString &folderPath, const QString &indexFileName);
    QHash<QString, struct SketchDescriptor *> indexAvailableElements(QDomElement &domElem, const QString &srcPrefix, QStringList & actionsTracker);
    void populateMenuWithIndex(const QHash<QString, struct SketchDescriptor *> &, QMenu * parentMenu, QDomElement &domElem);
    void populateMenuFromFolderContent(QMenu *parentMenu, const QString &path);
    void createOpenRecentMenu();
    void createEditMenuActions();
    void createPartMenuActions();
    void createViewMenuActions();
    void createWindowMenuActions();
    void createHelpMenuActions();
    void createMenus();
    void createToolBars();
    void createStatusBar();
	void connectPairs();
	void connectPair(SketchWidget * signaller, SketchWidget * slotter);
	void closeEvent(QCloseEvent * event);
	void saveAsAuxAux(const QString & fileName);
	void printAux(QPrinter &printer, bool removeBackground, bool paginate);
	void exportAux(QString fileName, QImage::Format format, bool removeBackground);
	void notYetImplemented(QString action);
	bool eventFilter(QObject *obj, QEvent *event);
	void setActionsIcons(int index, QList<QAction *> &);
	void exportToEagle();
	void exportToGerber();
    void exportBOM();
    void exportNetlist();
    void exportSvg(double res, bool selectedItems, bool flatten);
	void exportSvgWatermark(QString & svg, double res);
	void exportEtchable(bool wantPDF, bool wantSVG, bool flip);

	QList<QWidget*> getButtonsForView(ViewIdentifierClass::ViewIdentifier viewId);
	const QString untitledFileName();
	int &untitledFileCount();
	const QString fileExtension();
	QString getExtensionString();
	QStringList getExtensions();
	const QString defaultSaveFolder();
	bool undoStackIsEmpty();

	void createTraceMenuActions();
	void hideShowTraceMenu();

	QList<ModelPart*> moveToPartsFolder(QDir &unzipDir, MainWindow* mw, bool addToBin, bool addToAlien, const QString & prefixFolder, const QString &destFolder);
	void copyToSvgFolder(const QFileInfo& file, bool addToAlien, const QString & prefixFolder, const QString &destFolder);
	ModelPart* copyToPartsFolder(const QFileInfo& file, bool addToBin, bool addToAlien, const QString & prefixFolder, const QString &destFolder);

	void closeIfEmptySketch(MainWindow* mw);
	bool whatToDoWithAlienFiles();
	void backupExistingFileIfExists(const QString &destFilePath);
	void recoverBackupedFiles();
	void resetTempFolder();

	QMenu *breadboardItemMenu();
	QMenu *schematicItemMenu();
	QMenu *pcbItemMenu();
	QMenu *pcbWireMenu();
	QMenu *schematicWireMenu();
	QMenu *breadboardWireMenu();

	QMenu *viewItemMenuAux(QMenu* menu);

	void createZoomOptions(SketchAreaWidget* parent);
	SketchToolButton *createRotateButton(SketchAreaWidget *parent);
	SketchToolButton *createShareButton(SketchAreaWidget *parent);
	SketchToolButton *createFlipButton(SketchAreaWidget *parent);
	SketchToolButton *createAutorouteButton(SketchAreaWidget *parent);
	SketchToolButton *createOrderFabButton(SketchAreaWidget *parent);
	QWidget *createActiveLayerButton(SketchAreaWidget *parent);
	class ExpandingLabel * createRoutingStatusLabel(SketchAreaWidget *);
	SketchToolButton *createExportEtchableButton(SketchAreaWidget *parent);
	SketchToolButton *createNoteButton(SketchAreaWidget *parent);
	QWidget *createToolbarSpacer(SketchAreaWidget *parent);
	SketchAreaWidget *currentSketchArea();
	const QString fritzingTitle();

	void updateRaiseWindowAction();

	void moveEvent(QMoveEvent * event);
	bool event(QEvent *);
	void resizeEvent(QResizeEvent * event);
	QString genIcon(SketchWidget *, LayerList &  partViewLayerIDs, LayerList & wireViewLayerIDs);

	bool alreadyOpen(const QString & fileName);
	bool loadCustomBoardShape();
	void svgMissingLayer(const QString & layername, const QString & path);
	long swapSelectedAuxAux(ItemBase * itemBase, const QString & moduleID, ViewLayer::ViewLayerSpec viewLayerSpec, QUndoCommand * parentCommand);
	bool swapSpecial(const QString & prop, QMap<QString, QString> & currPropsMap);

	void enableAddBendpointAct(QGraphicsItem *);
	class FileProgressDialog * exportProgress();
	QString constructFileName(const QString & differentiator, const QString & extension);
	bool isGroundFill(ItemBase * itemBase);

	QString getBoardSilkscreenSvg(ItemBase * board, int res, QSizeF & imageSize);
	QString mergeBoardSvg(QString & svg, ItemBase * board, int res, QSizeF & imageSize, bool flip);

	bool wannaRestart();

	void updateActiveLayerButtons();
	int activeLayerIndex();
	bool hasLinkedProgramFiles(const QString & filename, QStringList & linkedProgramFiles);
	void pasteAux(bool pasteInPlace);

	void routingStatusLabelMouse(QMouseEvent*, bool show);
	class Wire * retrieveWire();
	class ConnectorItem * retrieveConnectorItem();
	QString getBomProps(ItemBase *);
	ModelPart * findReplacedby(ModelPart * originalModelPart);
	void groundFillAux(bool fillGroundTraces);
	void connectStartSave(bool connect);
	void loadBundledSketch(const QString &fileName, bool addToRecent, bool setAsLastOpened);

protected:
	static void removeActionsStartingAt(QMenu *menu, int start=0);
	static void setAutosave(int, bool);

protected:

	QUndoGroup *m_undoGroup;
	QUndoView *m_undoView;

	QPointer<SketchAreaWidget> m_breadboardWidget;
	QPointer<class BreadboardSketchWidget> m_breadboardGraphicsView;

	QPointer<SketchAreaWidget> m_schematicWidget;
	QPointer<class SchematicSketchWidget> m_schematicGraphicsView;

	QPointer<SketchAreaWidget> m_pcbWidget;
	QPointer<class PCBSketchWidget> m_pcbGraphicsView;

    QPointer<class BinManager> m_binManager;
    QPointer<class MiniViewContainer> m_miniViewContainerBreadboard;
    QPointer<class MiniViewContainer> m_miniViewContainerSchematic;
    QPointer<class MiniViewContainer> m_miniViewContainerPCB;
	QList <class MiniViewContainer *> m_navigators;
	QPointer<QStackedWidget> m_tabWidget;
    QPointer<class PaletteModel> m_paletteModel;
    QPointer<ReferenceModel> m_refModel;
    QPointer<class SketchModel> m_sketchModel;
    QPointer<class HtmlInfoView> m_infoView;
    QPointer<QToolBar> m_toolbar;

    QHash <long,class PartsEditorMainWindow*> m_partsEditorWindows;
    QHash <long,class PartsBinPaletteWidget*> m_binsWithPartsEditorRequests;

    bool m_closing;
	bool m_dontClose;
    bool m_firstOpen;

    QPointer<SketchAreaWidget> m_currentWidget;
    QPointer<SketchWidget> m_currentGraphicsView;

    //QToolBar *m_fileToolBar;
    //QToolBar *m_editToolBar;

    QAction *m_raiseWindowAct;

    // Fritzing Menu
    QMenu *m_fritzingMenu;
    QAction *m_aboutAct;
    QAction *m_tipsAndTricksAct;
    QAction *m_preferencesAct;
    QAction *m_quitAct;
    QAction *m_exceptionAct;

    // File Menu
    enum { MaxRecentFiles = 10 };

	QMenu *m_fileMenu;
	QAction *m_newAct;
	QAction *m_openAct;
	QMenu *m_openRecentFileMenu;
    QAction *m_openRecentFileActs[MaxRecentFiles];
	QMenu *m_openExampleMenu;
	QAction *m_saveAct;
	QAction *m_saveAsAct;
	QAction *m_pageSetupAct;
	QAction *m_printAct;
	QAction *m_shareOnlineAct;

	QAction * m_launchExternalProcessAct;

	QMenu *m_zOrderMenu;
	QMenu *m_zOrderWireMenu;
	QAction *m_bringToFrontAct;
	QAction *m_bringForwardAct;
	QAction *m_sendBackwardAct;
	QAction *m_sendToBackAct;
	class WireAction *m_bringToFrontWireAct;
	class WireAction *m_bringForwardWireAct;
	class WireAction *m_sendBackwardWireAct;
	class WireAction *m_sendToBackWireAct;

	// Export Menu
	QMenu *m_exportMenu;
	QAction *m_exportJpgAct;
	QAction *m_exportPsAct;
	QAction *m_exportPngAct;
	QAction *m_exportPdfAct;
	QAction *m_exportEagleAct;
	QAction *m_exportGerberAct;
	QAction *m_exportEtchablePdfAct;
	QAction *m_exportEtchableSvgAct;
	QAction *m_exportEtchablePdfFlipAct;
	QAction *m_exportEtchableSvgFlipAct;
	QAction *m_exportBomAct;
	QAction *m_exportNetlistAct;
	QAction *m_exportSvgAct;

    // Edit Menu
    QMenu *m_editMenu;
    QAction *m_undoAct;
    QAction *m_redoAct;
    QAction *m_cutAct;
    QAction *m_copyAct;
    QAction *m_pasteAct;
    QAction *m_pasteInPlaceAct;
    QAction *m_duplicateAct;
    QAction *m_deleteAct;
    class WireAction *m_deleteWireAct;
    QAction *m_selectAllAct;
    QAction *m_deselectAct;
	QAction *m_addNoteAct;

    // Part Menu
    QMenu *m_partMenu;
    QAction *m_createNewPart;
	QAction *m_infoViewOnHoverAction;
	QAction *m_exportNormalizedSvgAction;
	QAction *m_exportNormalizedFlattenedSvgAction;
    QAction *m_openInPartsEditorAct;
    QMenu *m_addToBinMenu;


	QMenu *m_rotateMenu;
	QAction *m_rotate90cwAct;
	QAction *m_rotate180Act;
	QAction *m_rotate90ccwAct;
	QAction *m_rotate45ccwAct;
	QAction *m_rotate45cwAct;

	QAction *m_moveLockAct;
	QAction *m_selectMoveLockAct;
	QAction *m_flipHorizontalAct;
	QAction *m_flipVerticalAct;
	QAction *m_showPartLabelAct;
	QAction *m_loadBundledPart;
	QAction *m_saveBundledPart;
	QAction *m_disconnectAllAct;
	QAction *m_selectAllObsoleteAct;
	QAction *m_swapObsoleteAct;
	QAction * m_openProgramWindowAct;

	QAction *m_addBendpointAct;
	QAction *m_flattenCurveAct;

    QAction *m_showAllLayersAct;
	QAction *m_hideAllLayersAct;

    // View Menu
    QMenu *m_viewMenu;
    QAction *m_zoomInAct;
    QAction *m_zoomOutAct;
    QAction *m_fitInWindowAct;
    QAction *m_actualSizeAct;
    QAction *m_100PercentSizeAct;
    QAction *m_alignToGridAct;
    QAction *m_showGridAct;
    QAction *m_showBreadboardAct;
    QAction *m_showSchematicAct;
    QAction *m_showPCBAct;
	QAction *m_showPartsBinIconViewAct;
	QAction *m_showPartsBinListViewAct;
    //QAction *m_toggleToolbarAct;
    int m_numFixedActionsInViewMenu;

    // Window Menu
	QMenu *m_windowMenu;
	QAction *m_minimizeAct;
	QAction *m_togglePartLibraryAct;
	QAction *m_toggleInfoAct;
	QAction *m_toggleNavigatorAct;
	QAction *m_toggleUndoHistoryAct;
	QAction *m_toggleDebuggerOutputAct;
	QAction *m_windowMenuSeparator;

	// Trace Menu
	QMenu *m_pcbTraceMenu;
	QMenu *m_schematicTraceMenu;
	QMenu *m_breadboardTraceMenu;
	QAction *m_autorouteAct;
	QAction *m_orderFabAct;
	QAction *m_activeLayerTopAct;
	QAction *m_activeLayerBottomAct;
	QAction *m_activeLayerBothAct;
	class WireAction *m_createTraceWireAct;
	class WireAction *m_createWireWireAct;
	QAction *m_createJumperAct;
	QAction *m_changeTraceLayerAct;
	QAction *m_excludeFromAutorouteAct;
	class WireAction *m_excludeFromAutorouteWireAct;
	QAction *m_selectAllTracesAct;
	QAction *m_selectAllWiresAct;
	QAction *m_selectAllCopperFillAct;
	QAction *m_updateRoutingStatusAct;
	QAction *m_selectAllExcludedTracesAct;
	QAction *m_selectAllIncludedTracesAct;
	QAction *m_selectAllJumperItemsAct;
	QAction *m_selectAllViasAct;
	QAction *m_groundFillAct;
	QAction *m_removeGroundFillAct;
	QAction *m_copperFillAct;
	class ConnectorItemAction *m_setOneGroundFillSeedAct;
	QAction *m_setGroundFillSeedsAct;
	QAction *m_clearGroundFillSeedsAct;
	QAction *m_designRulesCheckAct;
	QAction *m_autorouterSettingsAct;
	QAction *m_tidyWiresAct;

	// Help Menu
    QMenu *m_helpMenu;
    QAction *m_openHelpAct;
    QAction *m_openDonateAct;
    QAction *m_examplesAct;
    QAction *m_partsRefAct;
    QAction *m_showInViewHelpAct;;
    QAction *m_visitFritzingDotOrgAct;
    QAction *m_checkForUpdatesAct;
    QAction *m_aboutQtAct;
    QAction *m_reportBugAct;
	QAction *m_enableDebugAct;
    QAction *m_importFilesFromPrevInstallAct;

	// Wire Color Menu
	QMenu * m_wireColorMenu;

    // Dot icons
    QIcon m_dotIcon;
    QIcon m_emptyIcon;

	QList<SketchToolButton*> m_rotateButtons;
	QList<SketchToolButton*> m_flipButtons;
	QStackedWidget * m_activeLayerButtonWidget;

    bool m_comboboxChanged;
    bool m_restarting;

    QStringList m_alienFiles;
    QString m_alienPartsMsg;
    QStringList m_filesReplacedByAlienOnes;

    QStringList m_openExampleActions;

	QPointer<class TripleNavigator> m_tripleNavigator;
	QDockWidget * m_navigatorDock;
	QPointer<class FSizeGrip> m_sizeGrip;

	friend class Helper;
	friend class DockManager;

	QPointer<class ViewSwitcher> m_viewSwitcher;
	QPointer<class ViewSwitcherDockWidget> m_viewSwitcherDock;

	QPointer<Helper> m_helper;
	QTimer m_setUpDockManagerTimer;
	QPointer<class DockManager> m_dockManager;
	QPointer<class FileProgressDialog> m_fileProgressDialog;
	QPointer<class ZoomSlider> m_zoomSlider;
	QPointer<QLabel> m_locationLabel;
	QString m_locationLabelUnits;

	QByteArray m_externalProcessOutput;

	QPointer<class LayerPalette> m_layerPalette;
	QPointer<class ProgramWindow> m_programWindow;
	QList<LinkedFile *>  m_linkedProgramFiles;
	QString m_backupFileNameAndPath;
	QTimer m_autosaveTimer;
	bool m_autosaveNeeded;
	bool m_backingUp;
	QString m_bundledSketchName;
	RoutingStatus m_routingStatus;
	bool m_smdOneSideWarningGiven;
	bool m_orderFabEnabled;		
	bool m_closeSilently;
	QString m_fzzFolder;
	QHash<QString, struct LockedFile *> m_fzzFiles;
	SwapTimer m_swapTimer;
	QPointer<Wire> m_activeWire;
	QPointer<ConnectorItem> m_activeConnectorItem;

public:
	static int RestartNeeded;
	static int AutosaveTimeoutMinutes;
	static bool AutosaveEnabled;
	static QString BackupFolder;


protected:
	static const QString UntitledSketchName;
	static int UntitledSketchIndex;
	static int CascadeFactorX;
	static int CascadeFactorY;
};


#endif
