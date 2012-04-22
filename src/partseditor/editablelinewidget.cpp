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



#include "editablelinewidget.h"
#include "../utils/misc.h"

EditableLineWidget::EditableLineWidget(QString text, WaitPushUndoStack *undoStack, QWidget *parent, QString title, bool edited, bool noSpacing)
	: AbstractEditableLabelWidget(text, undoStack, parent, title, edited, noSpacing) {
	m_lineEdit = new QLineEdit(this);
	toStandardMode();
}

void EditableLineWidget::setValidator(const QValidator * v ) {
	m_lineEdit->setValidator(v);
}

void EditableLineWidget::setText(const QString &text) {
	m_lineEdit->setText(text);
	m_label->setText(text);
}
QString EditableLineWidget::editionText() {
	return m_lineEdit->text();
}
void EditableLineWidget::setEditionText(QString text) {
	m_lineEdit->setText(text);
}
QWidget* EditableLineWidget::myEditionWidget() {
	return m_lineEdit;
}

void EditableLineWidget::setEmptyTextToEdit() {
	m_lineEdit->setText("");
}
