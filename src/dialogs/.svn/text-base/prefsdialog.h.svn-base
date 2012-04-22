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


#ifndef PREFSDIALOG_H
#define PREFSDIALOG_H

#include <QDialog>
#include <QFileInfoList>
#include <QHash>
#include <QLabel>
#include <QTabWidget>
#include <QRadioButton>
#include <QLineEdit>
#include <QDoubleValidator>


struct ViewInfoThing
{
	QLineEdit * lineEdit;
	QDoubleValidator * validator;
	QRadioButton * mmButton;
	QRadioButton * inButton;
	double defaultGridSize;
	QString viewName;
	QString shortName;
	int index;
	QColor currentBColor;
	QColor standardBColor;
	bool curvy;
};


class PrefsDialog : public QDialog
{
	Q_OBJECT

public:
	PrefsDialog(const QString & language, QWidget *parent = 0);
	~PrefsDialog();

	bool cleared();
	QHash<QString, QString> & settings();
	QHash<QString, QString> & tempSettings();
	void initLayout(QFileInfoList & list);
	void initViewInfo(int index, const QString & viewName, const QString & shortName, double defaultSize, QColor current, QColor standard, bool curvy);

protected:
	QWidget * createLanguageForm(QFileInfoList & list);
	QWidget* createOtherForm();
	QWidget* createColorForm();
	QWidget * createZoomerForm();
	QWidget * createAutosaveForm();
	void updateWheelText();
	void initGeneral(QWidget * general, QFileInfoList & list);
	void initBreadboard(QWidget *, ViewInfoThing *);
	void initSchematic(QWidget *, ViewInfoThing *);
	void initPCB(QWidget *, ViewInfoThing *);
	QWidget * createGridSizeForm(ViewInfoThing *);
	QWidget * createBackgroundColorForm(ViewInfoThing *);
	void updateGridSize(ViewInfoThing *);
	QWidget * createCurvyForm(ViewInfoThing *);

protected slots:
	void changeLanguage(int);
	void clear();
	void setConnectedColor();
	void setUnconnectedColor();
	void changeWheelBehavior();
	void toggleAutosave(bool);
	void changeAutosavePeriod(int);
	void units(bool);
	void restoreDefault();
	void setBackgroundColor();
	void gridEditingFinished();
	void curvyChanged();

protected:
	QTabWidget * m_tabWidget;
	QWidget * m_general;
	QWidget * m_breadboard;
	QWidget * m_schematic;
	QWidget * m_pcb;
	QLabel * m_wheelLabel;
	QHash<QString, QString> m_settings;
	QHash<QString, QString> m_tempSettings;
	QString m_name;
	class TranslatorListModel * m_translatorListModel;
	bool m_cleared;
	int m_wheelMapping;
	ViewInfoThing m_viewInfoThings[3];
};

#endif 
