/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2010 Fachhochschule Potsdam - http://fh-potsdam.de

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


#ifndef DOCKMANAGER_H_
#define DOCKMANAGER_H_

#include <QObject>

#include "mainwindow.h"

typedef class FDockWidget * (*DockFactory)(const QString & title, QWidget * parent);

class DockManager : public QObject {
	Q_OBJECT
	public:
		DockManager(MainWindow *mainWindow);
		void createBinAndInfoViewDocks();
		void createDockWindows();
		void dontKeepMargins();

	public slots:
		void keepMargins();

	protected slots:
		void dockChangeActivation(bool activate, QWidget * originator);

	protected:
		class FDockWidget * makeDock(const QString & title, QWidget * widget, int dockMinHeight, int dockDefaultHeight, Qt::DockWidgetArea area = Qt::RightDockWidgetArea, DockFactory dockFactory = NULL);
		class FDockWidget * dockIt(FDockWidget* dock, int dockMinHeight, int dockDefaultHeight, Qt::DockWidgetArea area = Qt::RightDockWidgetArea);
		FDockWidget *newTopWidget();
		FDockWidget *newBottomWidget();
		void removeMargin(FDockWidget* dock);
		void addTopMargin(FDockWidget* dock);
		void addBottomMargin(FDockWidget* dock);
		void dockMarginAux(FDockWidget* dock, const QString &name, const QString &style);

	protected:
		MainWindow *m_mainWindow;
		QList<FDockWidget*> m_docks;

		FDockWidget* m_topDock;
		FDockWidget* m_bottomDock;
		QString m_oldTopDockStyle;
		QString m_oldBottomDockStyle;
		bool m_dontKeepMargins;

	public:
                static const int PartsBinDefaultHeight;
                static const int PartsBinMinHeight;
                static const int NavigatorDefaultHeight;
                static const int NavigatorMinHeight;
                static const int InfoViewDefaultHeight;
                static const int InfoViewMinHeight;
                static const int UndoHistoryDefaultHeight;
                static const int UndoHistoryMinHeight;
                static const int DockDefaultWidth;
                static const int DockMinWidth;
                static const int DockDefaultHeight;
                static const int DockMinHeight;
};

#endif /* DOCKMANAGER_H_ */
