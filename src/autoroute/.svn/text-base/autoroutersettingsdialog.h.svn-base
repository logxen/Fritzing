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

$Revision$:
$Author$:
$Date$

********************************************************************/

#ifndef AUTOROUTERSETTINGSDIALOG_H
#define AUTOROUTERSETTINGSDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QDoubleValidator>
#include <QRadioButton>
#include <QGroupBox>

#include "../items/via.h"

class AutorouterSettingsDialog : public QDialog
{
Q_OBJECT

public:
	AutorouterSettingsDialog(QWidget *parent = 0);
	~AutorouterSettingsDialog();

protected slots:
	void acceptAnd();
	void restoreDefault();
	void production(bool);
	void widthEntry(const QString &);

	void changeUnits(bool);
	void changeHoleSize(const QString &);
	void changeDiameter();
	void changeThickness();

protected:
	void enableCustom(bool enable);
	bool initRadios();
	void setTraceWidth(int newWidth);

protected:
	QRadioButton * m_homebrewButton;
	QRadioButton * m_professionalButton;
	QRadioButton * m_customButton;
	HoleSettings m_holeSettings;
	QFrame * m_customFrame;
	QComboBox * m_traceWidthComboBox;
	int m_traceWidth;

public:
	static const QString AutorouteTraceWidth;

};


#endif // AUTOROUTERSETTINGSDIALOG_H
