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

$Revision: 4183 $:
$Author: cohen@irascible.com $:
$Date: 2010-05-06 13:30:19 -0700 (Thu, 06 May 2010) $

********************************************************************/



#ifndef HASHPOPULATEWIDGET_H_
#define HASHPOPULATEWIDGET_H_

#include <QFrame>
#include <QHash>
#include <QLineEdit>
#include <QGridLayout>
#include <QUndoStack>

#include "baseremovebutton.h"

class HashLineEdit : public QLineEdit {
	Q_OBJECT
	public:
		HashLineEdit(QUndoStack *undoStack, const QString &text, bool defaultValue = false, QWidget *parent = 0);
		bool hasChanged();
		QString textIfSetted();

	protected slots:
		void updateObjectName();
		void updateStackState();

	protected:
		void mousePressEvent(QMouseEvent * event);
		void focusOutEvent(QFocusEvent * event);

		QString m_firstText;
		bool m_isDefaultValue;
		QUndoStack *m_undoStack;
};

class HashRemoveButton : public BaseRemoveButton {
	Q_OBJECT
	public:
		HashRemoveButton(HashLineEdit* label, HashLineEdit* value, QWidget *parent)
			: BaseRemoveButton(parent)
		{
			m_label = label;
			m_value = value;
		}

		HashLineEdit *label() {return m_label;}
		HashLineEdit *value() {return m_value;}

	signals:
		void clicked(HashRemoveButton*);

	protected:
		void clicked() {
			emit clicked(this);
		}
		HashLineEdit *m_label;
		HashLineEdit *m_value;
};

class HashPopulateWidget : public QFrame {
	Q_OBJECT
	public:
		HashPopulateWidget(QString title, QHash<QString,QString> &initValues, const QStringList &readOnlyKeys, QUndoStack *undoStack, QWidget *parent = 0);
		const QHash<QString,QString> & hash();
		HashLineEdit* lineEditAt(int row, int col);

	protected slots:
		void lastRowEditionCompleted();
		void removeRow(HashRemoveButton*);

	signals:
		void editionStarted();

	protected:
		void addRow(QGridLayout *layout = 0);
		QGridLayout* gridLayout();
		HashRemoveButton *createRemoveButton(HashLineEdit* label, HashLineEdit* value);

		QHash<QString,QString> m_hash;

		HashLineEdit *m_lastLabel;
		HashLineEdit *m_lastValue;

		int m_currRow;
		QUndoStack *m_undoStack;
};

#endif /* HASHPOPULATEWIDGET_H_ */
