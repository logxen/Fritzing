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




#ifndef SKETCHTOOLBUTTON_H_
#define SKETCHTOOLBUTTON_H_

#include <QToolButton>

#include "utils/abstractstatesbutton.h"

class SketchToolButton : public QToolButton, public AbstractStatesButton {
	Q_OBJECT
	public:
		SketchToolButton(const QString &imageName, QWidget *parent, QAction* defaultAction);
		SketchToolButton(const QString &imageName, QWidget *parent, QList<QAction*> menuActions);

		void updateEnabledState();

	signals:
		void menuUpdateNeeded();

	protected slots:
		void setEnabledIconAux();

	protected:
		QString imagePrefix();
		void setImage(const QPixmap & pixmap);
		void setupIcons(const QString &imageName);

		void actionEvent(QActionEvent *event);
		void mousePressEvent(QMouseEvent *event);
		void mouseReleaseEvent(QMouseEvent *event);
		void changeEvent(QEvent *event);

	protected:
		QString m_imageName;
};

#endif /* SKETCHTOOLBUTTON_H_ */
