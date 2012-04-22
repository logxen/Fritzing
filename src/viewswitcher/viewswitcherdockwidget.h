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

$Revision: 5721 $:
$Author: cohen@irascible.com $:
$Date: 2012-01-03 07:53:58 -0800 (Tue, 03 Jan 2012) $

********************************************************************/


#ifndef VIEWSWITCHERDOCKWIDGET_H
#define VIEWSWITCHERDOCKWIDGET_H

#include "../fdockwidget.h"


class ViewSwitcherDockWidget : public FDockWidget
{
    Q_OBJECT

public:
    ViewSwitcherDockWidget(const QString & title, QWidget * parent = 0);
	~ViewSwitcherDockWidget();
	
	void setViewSwitcher(class ViewSwitcher *);
	void setVisible(bool visible);
	void restorePreference();
	void prestorePreference();

public slots:
	void windowMoved(QWidget *);
	void topLevelChangedSlot(bool topLevel);

protected slots:
	void savePreference();

protected:
	void calcWithin();
	bool event(QEvent *event);
	void resizeEvent(QResizeEvent * event);
	void topLevelChangedSlotAux(bool topLevel);

protected:
	class ViewSwitcher * m_viewSwitcher;
	QPoint m_offsetFromParent;
	bool m_within;
	QBitmap * m_bitmap;

};

#endif
