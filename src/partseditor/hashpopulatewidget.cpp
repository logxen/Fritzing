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



#include <QtAlgorithms>

#include "hashpopulatewidget.h"
#include "../debugdialog.h"
#include "../utils/misc.h"

HashLineEdit::HashLineEdit(QUndoStack *undoStack, const QString &text, bool defaultValue, QWidget *parent) : QLineEdit(text, parent) {
	m_firstText = text;
	m_isDefaultValue = defaultValue;
	if(defaultValue) {
		setStyleSheet("font-style: italic;");
		connect(this,SIGNAL(textChanged(QString)),this,SLOT(updateObjectName()));
	} else {
		setStyleSheet("font-style: normal;");
	}
	m_undoStack = undoStack;

	connect(this,SIGNAL(editingFinished()),this,SLOT(updateStackState()));
}

QString HashLineEdit::textIfSetted() {
	if(m_isDefaultValue && !hasChanged()) {
		return ___emptyString___;
	} else {
		return text();
	}
}

void HashLineEdit::updateObjectName() {
	if(m_isDefaultValue) {
		if(!isModified() && !hasChanged()) {
			setStyleSheet("font-style: italic;");
		} else {
			setStyleSheet("font-style: normal;");
		}
	}
}

void HashLineEdit::updateStackState() {
	if(hasChanged()) {
		m_undoStack->push(new QUndoCommand("dummy parts editor command"));
	}
}

void HashLineEdit::mousePressEvent(QMouseEvent * event) {
	if(m_isDefaultValue && !isModified() && !hasChanged()) {
		setText("");
	}
	QLineEdit::mousePressEvent(event);
}

bool HashLineEdit::hasChanged() {
	return m_firstText != text();
}

void HashLineEdit::focusOutEvent(QFocusEvent * event) {
	if(text().isEmpty()) {
		setText(m_firstText);
	}
	QLineEdit::focusOutEvent(event);
}

HashPopulateWidget::HashPopulateWidget(QString title, QHash<QString,QString> &initValues, const QStringList &readOnlyKeys, QUndoStack *undoStack, QWidget *parent) : QFrame(parent) {
	m_undoStack = undoStack;
	m_currRow = 0;
	m_lastLabel = NULL;
	m_lastValue = NULL;

	QGridLayout *layout = new QGridLayout();
	layout->setColumnStretch(0,0);
	layout->setColumnStretch(1,1);
	layout->setColumnStretch(2,0);

	layout->addWidget(new QLabel(title),m_currRow,m_currRow,0);
	m_currRow++;

	QList<QString> keys = initValues.keys();
	qSort(keys);

	for(int i=0; i < keys.count(); i++) {
		HashLineEdit *name = new HashLineEdit(m_undoStack,keys[i],false,this);
		HashLineEdit *value = new HashLineEdit(m_undoStack,initValues[keys[i]],false,this);

		if(readOnlyKeys.contains(keys[i])) {
			name->setEnabled(false);
		} else {
			HashRemoveButton *button = createRemoveButton(name, value);
			layout->addWidget(button,m_currRow,3);
		}

		layout->addWidget(name,m_currRow,0);
		layout->addWidget(value,m_currRow,1,1,2);

		m_currRow++;
	}

	addRow(layout);

	this->setLayout(layout);
}

HashRemoveButton *HashPopulateWidget::createRemoveButton(HashLineEdit* label, HashLineEdit* value) {
	HashRemoveButton *button = new HashRemoveButton(label, value, this);
	connect(button, SIGNAL(clicked(HashRemoveButton*)), this, SLOT(removeRow(HashRemoveButton*)));
	return button;
}

const QHash<QString,QString> &HashPopulateWidget::hash() {
	for(int i=1 /*i==0 is title*/; i < gridLayout()->rowCount() /*last one is always an empty one*/; i++) {
		QString label = ___emptyString___;
		HashLineEdit *labelEdit = lineEditAt(i,0);
		if(labelEdit) {
			label = labelEdit->textIfSetted();
		}

		QString value = ___emptyString___;
		HashLineEdit *valueEdit = lineEditAt(i,1);
		if(valueEdit) {
			value = valueEdit->textIfSetted();
		}

		if(!label.isEmpty() /*&& !value.isEmpty()*/) {
			m_hash[label] = value;
		}
	}
	return m_hash;
}

HashLineEdit* HashPopulateWidget::lineEditAt(int row, int col) {
	QLayoutItem *litem = gridLayout()->itemAtPosition(row,col);
	return litem ? (HashLineEdit*)litem->widget() : NULL;
}

void HashPopulateWidget::addRow(QGridLayout *layout) {
	if(layout == NULL) {
		layout = gridLayout();
	}

	if(m_lastLabel) {
		disconnect(m_lastLabel,SIGNAL(editingFinished()),this,SLOT(lastRowEditionCompleted()));
	}

	if(m_lastValue) {
		disconnect(m_lastValue,SIGNAL(editingFinished()),this,SLOT(lastRowEditionCompleted()));
	}


	m_lastLabel = new HashLineEdit(m_undoStack,QObject::tr("a label"),true,this);
	layout->addWidget(m_lastLabel,m_currRow,0);
	connect(m_lastLabel,SIGNAL(editingFinished()),this,SLOT(lastRowEditionCompleted()));

	m_lastValue = new HashLineEdit(m_undoStack,QObject::tr("a value"),true,this);
	layout->addWidget(m_lastValue,m_currRow,1,1,2);
	connect(m_lastValue,SIGNAL(editingFinished()),this,SLOT(lastRowEditionCompleted()));

	m_currRow++;

	emit editionStarted();
}

QGridLayout * HashPopulateWidget::gridLayout() {
	return (QGridLayout*)this->layout();
}

void HashPopulateWidget::lastRowEditionCompleted() {
	if(	m_lastLabel && !m_lastLabel->text().isEmpty() && m_lastLabel->hasChanged()) {
		if(m_lastLabel->text().isEmpty() && m_lastValue->text().isEmpty()) {
			// removeRow() ?;
		} else {
			HashRemoveButton *button = createRemoveButton(m_lastLabel, m_lastValue);
			gridLayout()->addWidget(button,m_currRow-1,3);
			addRow();
		}
	}
}

void HashPopulateWidget::removeRow(HashRemoveButton* button) {
	m_undoStack->push(new QUndoCommand("dummy parts editor command"));
	QLayout *lo = layout();
	QList<QWidget*> widgs;
	widgs << button->label() << button->value() << button;
	foreach(QWidget* w, widgs) {
		lo->removeWidget(w);
		w->hide();
	}
	lo->update();

	m_currRow--;
}
