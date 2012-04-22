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

$Revision: 5931 $:
$Author: cohen@irascible.com $:
$Date: 2012-03-28 13:18:24 -0700 (Wed, 28 Mar 2012) $

********************************************************************/

#include <QSizeGrip>
#include <QStatusBar>
#include <QtDebug>
#include <QApplication>

#include "dockmanager.h"
#include "navigator/triplenavigator.h"
#include "utils/fsizegrip.h"
#include "viewswitcher/viewswitcherdockwidget.h"
#include "utils/misc.h"
#include "partsbinpalette/binmanager/binmanager.h"
#include "infoview/htmlinfoview.h"
#include "layerpalette.h"

const int DockManager::PartsBinDefaultHeight = 240;
const int DockManager::PartsBinMinHeight = 100;
const int DockManager::InfoViewDefaultHeight = 220;
const int DockManager::InfoViewMinHeight = 50;
const int DockManager::NavigatorDefaultHeight = 60;
const int DockManager::NavigatorMinHeight = 40;
const int DockManager::UndoHistoryDefaultHeight = 70;
const int DockManager::UndoHistoryMinHeight = UndoHistoryDefaultHeight;
const int DockManager::DockDefaultWidth = 250;
const int DockManager::DockMinWidth = 130;
const int DockManager::DockDefaultHeight = 50;
const int DockManager::DockMinHeight = 30;

FDockWidget * makeViewSwitcherDock(const QString & title, QWidget * parent) {
	return new ViewSwitcherDockWidget(title, parent);
}

DockManager::DockManager(MainWindow *mainWindow)
	: QObject(mainWindow)
{
	m_mainWindow = mainWindow;
	m_mainWindow->setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);
	m_mainWindow->setDockOptions(QMainWindow::AnimatedDocks);
	m_mainWindow->m_sizeGrip = new FSizeGrip(mainWindow);

	m_topDock = NULL;
	m_bottomDock = NULL;
	m_dontKeepMargins = true;

	m_oldTopDockStyle = ___emptyString___;
	m_oldBottomDockStyle = ___emptyString___;
}

void DockManager::dockChangeActivation(bool activate, QWidget * originator) {
	Q_UNUSED(activate);
	Q_UNUSED(originator);
	if (!m_mainWindow->m_closing) {
		m_mainWindow->m_sizeGrip->rearrange();
	}

}

void DockManager::createBinAndInfoViewDocks() {
	m_mainWindow->m_infoView = new HtmlInfoView();
	DebugDialog::debug("after html view");

	m_mainWindow->m_binManager = new BinManager(m_mainWindow->m_refModel, m_mainWindow->m_infoView, m_mainWindow->m_undoStack, m_mainWindow);

	DebugDialog::debug("after bin manager");
	if (m_mainWindow->m_paletteModel->loadedFromFile()) {
		m_mainWindow->m_binManager->loadFromModel(m_mainWindow->m_paletteModel);
	} else {
		m_mainWindow->m_binManager->setPaletteModel(m_mainWindow->m_paletteModel);
	}
}

void DockManager::createDockWindows()
{
	QWidget * widget = new QWidget();
	widget->setMinimumHeight(0);
	widget->setMaximumHeight(0);
	FDockWidget * dock = makeDock(tr("View Switcher"), widget, 0,  0, Qt::RightDockWidgetArea, makeViewSwitcherDock);
	ViewSwitcherDockWidget * vsd = qobject_cast<ViewSwitcherDockWidget *>(dock);
	m_mainWindow->m_viewSwitcherDock = vsd;
	connect(m_mainWindow, SIGNAL(mainWindowMoved(QWidget *)), vsd, SLOT(windowMoved(QWidget *)));

#ifndef QT_NO_DEBUG
	//dock->setStyleSheet("background-color: red;");
	//m_mainWindow->m_viewSwitcher->setStyleSheet("background-color: blue;");
#endif

	FDockWidget* partsDock = makeDock(BinManager::Title, m_mainWindow->m_binManager, PartsBinMinHeight, PartsBinDefaultHeight/*, Qt::LeftDockWidgetArea*/);
	m_mainWindow->m_binManager->dockedInto(partsDock);
    makeDock(tr("Inspector"), m_mainWindow->m_infoView, InfoViewMinHeight, InfoViewDefaultHeight);

	makeDock(tr("Undo History"), m_mainWindow->m_undoView, UndoHistoryMinHeight, UndoHistoryDefaultHeight)->hide();
    m_mainWindow->m_undoView->setMinimumSize(DockMinWidth, UndoHistoryMinHeight);

	m_mainWindow->m_tripleNavigator = new TripleNavigator(m_mainWindow);
	
    m_mainWindow->m_navigators << (m_mainWindow->m_miniViewContainerBreadboard = new MiniViewContainer(m_mainWindow->m_tripleNavigator));
	m_mainWindow->m_miniViewContainerBreadboard->filterMousePress();
	connect(m_mainWindow->m_miniViewContainerBreadboard, SIGNAL(navigatorMousePressedSignal(MiniViewContainer *)),
								m_mainWindow, SLOT(currentNavigatorChanged(MiniViewContainer *)));

    m_mainWindow->m_navigators << (m_mainWindow->m_miniViewContainerSchematic = new MiniViewContainer(m_mainWindow->m_tripleNavigator));
	m_mainWindow->m_miniViewContainerSchematic->filterMousePress();
	connect(m_mainWindow->m_miniViewContainerSchematic, SIGNAL(navigatorMousePressedSignal(MiniViewContainer *)),
								m_mainWindow, SLOT(currentNavigatorChanged(MiniViewContainer *)));

    m_mainWindow->m_navigators << (m_mainWindow->m_miniViewContainerPCB = new MiniViewContainer(m_mainWindow->m_tripleNavigator));
	m_mainWindow->m_miniViewContainerPCB->filterMousePress();
	connect(m_mainWindow->m_miniViewContainerPCB, SIGNAL(navigatorMousePressedSignal(MiniViewContainer *)),
								m_mainWindow, SLOT(currentNavigatorChanged(MiniViewContainer *)));


	m_mainWindow->m_tripleNavigator->addView(m_mainWindow->m_miniViewContainerBreadboard, tr("Breadboard"));
	m_mainWindow->m_tripleNavigator->addView(m_mainWindow->m_miniViewContainerSchematic, tr("Schematic"));
	m_mainWindow->m_tripleNavigator->addView(m_mainWindow->m_miniViewContainerPCB, tr("PCB"));
	m_mainWindow->m_navigatorDock = makeDock(tr("Navigator"), m_mainWindow->m_tripleNavigator, NavigatorMinHeight, NavigatorDefaultHeight);
	m_mainWindow->m_navigatorDock->hide();

    makeDock(tr("Layers"), m_mainWindow->m_layerPalette, DockMinWidth, DockMinHeight)->hide();
    m_mainWindow->m_undoView->setMinimumSize(DockMinWidth, DockMinHeight);

    m_mainWindow->m_windowMenu->addSeparator();
    m_mainWindow->m_windowMenu->addAction(m_mainWindow->m_openProgramWindowAct);

#ifndef QT_NO_DEBUG
    m_mainWindow->m_windowMenu->addSeparator();
    m_mainWindow->m_windowMenu->addAction(m_mainWindow->m_toggleDebuggerOutputAct);
#endif

    m_mainWindow->m_windowMenuSeparator = m_mainWindow->m_windowMenu->addSeparator();
}

FDockWidget * DockManager::makeDock(const QString & title, QWidget * widget, int dockMinHeight, int dockDefaultHeight, Qt::DockWidgetArea area, DockFactory dockFactory) {
	FDockWidget * dock = ((dockFactory) ? dockFactory(title, m_mainWindow) : new FDockWidget(title, m_mainWindow));
    dock->setObjectName(title);
    dock->setWidget(widget);
    widget->setParent(dock);
    widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    connect(dock, SIGNAL(positionChanged()), this, SLOT(keepMargins()));
    connect(dock, SIGNAL(topLevelChanged(bool)), this, SLOT(keepMargins()));
    connect(dock, SIGNAL(visibilityChanged(bool)), this, SLOT(keepMargins()));

	return dockIt(dock, dockMinHeight, dockDefaultHeight, area);
}

FDockWidget *DockManager::dockIt(FDockWidget* dock, int dockMinHeight, int dockDefaultHeight, Qt::DockWidgetArea area) {
	dock->setAllowedAreas(area);
	m_mainWindow->addDockWidget(area, dock);
    m_mainWindow->m_windowMenu->addAction(dock->toggleViewAction());

    dock->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	dock->setMinimumSize(DockMinWidth, dockMinHeight);
	dock->resize(DockDefaultWidth, dockDefaultHeight);
	connect(dock, SIGNAL(dockChangeActivationSignal(bool, QWidget *)), this, SLOT(dockChangeActivation(bool, QWidget *)));
	connect(dock, SIGNAL(destroyed(QObject *)), qApp, SLOT(topLevelWidgetDestroyed(QObject *)));
    connect(dock, SIGNAL(dockChangeActivationSignal(bool, QWidget *)), qApp, SLOT(changeActivation(bool, QWidget *)), Qt::DirectConnection);

    m_docks << dock;

    return dock;
}

FDockWidget *DockManager::newTopWidget() {
	int topMostY = 10000;
	FDockWidget *topWidget = NULL;
	foreach(FDockWidget* dock, m_docks) {
		if(/*!dock->isFloating() && dock->isVisible() &&*/
			m_mainWindow->dockWidgetArea(dock) == Qt::RightDockWidgetArea
			&& dock->pos().y() < topMostY) {
			topMostY = dock->pos().y();
			topWidget = dock;
		}
	}
	return topWidget;
}

FDockWidget *DockManager::newBottomWidget() {
	int bottomMostY = -1;
	FDockWidget *bottomWidget = NULL;
	foreach(FDockWidget* dock, m_docks) {
		if(!dock->isFloating() && dock->isVisible() &&
			m_mainWindow->dockWidgetArea(dock) == Qt::RightDockWidgetArea
			&& dock->pos().y() > bottomMostY) {
			bottomMostY = dock->pos().y();
			bottomWidget = dock;
		}
	}
	return bottomWidget;
}

void DockManager::keepMargins() {
	if (m_dontKeepMargins) return;

	/*FDockWidget* newTopWidget = this->newTopWidget();
	if(m_topDock != newTopWidget) {
		removeMargin(m_topDock);
		m_topDock = newTopWidget;
		if(m_topDock) m_oldTopDockStyle = m_topDock->styleSheet();
		addTopMargin(m_topDock);
	}*/

	FDockWidget* newBottomWidget = this->newBottomWidget();
	if(m_bottomDock != newBottomWidget) {
		removeMargin(m_bottomDock);
		m_bottomDock = newBottomWidget;
		if(m_bottomDock) m_oldBottomDockStyle = m_bottomDock->styleSheet();
		addBottomMargin(m_bottomDock);
		m_mainWindow->m_sizeGrip->raise();
	}
}


void DockManager::removeMargin(FDockWidget* dock) {
	if(dock) {
		TripleNavigator *tn = qobject_cast<TripleNavigator*>(dock->widget());
		if(tn) {
			tn->showBottomMargin(false);
		} else {
			dockMarginAux(dock, "", m_oldBottomDockStyle);
		}
	}
}

void DockManager::addTopMargin(FDockWidget* dock) {
	if(dock) dockMarginAux(dock, "topMostDock", dock->widget()->styleSheet());
}

void DockManager::addBottomMargin(FDockWidget* dock) {
	if(dock) {
		TripleNavigator *tn = qobject_cast<TripleNavigator*>(dock->widget());
		if(tn) {
			tn->showBottomMargin(true);
		} else if(qobject_cast<BinManager*>(dock->widget())) {
			// already has enought space
		} else {
			dockMarginAux(dock, "bottomMostDock", dock->widget()->styleSheet());
		}
	}
}


void DockManager::dockMarginAux(FDockWidget* dock, const QString &name, const QString &style) {
	if(dock) {
		dock->widget()->setObjectName(name);
		dock->widget()->setStyleSheet(style);
		dock->setStyleSheet(dock->styleSheet());
	} else {
		qWarning() << tr("Couldn't get the dock widget");
	}

}

void DockManager::dontKeepMargins() {
	m_dontKeepMargins = true;
}
