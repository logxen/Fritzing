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

#include "prefsdialog.h"
#include "../debugdialog.h"
#include "translatorlistmodel.h"
#include "../items/itembase.h"
#include "../utils/clickablelabel.h"
#include "setcolordialog.h"
#include "../sketch/zoomablegraphicsview.h"
#include "../mainwindow.h"

#include <QFormLayout>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QLocale>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QRadioButton>
#include <QSpinBox>
#include <QSettings>
#include <QLineEdit>

#define MARGIN 5

PrefsDialog::PrefsDialog(const QString & language, QWidget *parent)
	: QDialog(parent)
{
	this->setWindowTitle(QObject::tr("Preferences"));

	m_name = language;
	m_cleared = false;
	m_wheelMapping = (int) ZoomableGraphicsView::wheelMapping();
}

PrefsDialog::~PrefsDialog()
{
}

void PrefsDialog::initViewInfo(int index, const QString & viewName, const QString & shortName, double defaultSize, QColor current, QColor standard, bool curvy) 
{
	m_viewInfoThings[index].index = index;
	m_viewInfoThings[index].defaultGridSize = defaultSize;
	m_viewInfoThings[index].viewName = viewName;
	m_viewInfoThings[index].shortName = shortName;
	m_viewInfoThings[index].currentBColor = current;
	m_viewInfoThings[index].standardBColor = standard;
	m_viewInfoThings[index].curvy = curvy;
}

void PrefsDialog::initLayout(QFileInfoList & list)
{
	m_tabWidget = new QTabWidget();
	m_general = new QWidget();
	m_breadboard = new QWidget();
	m_schematic = new QWidget();
	m_pcb = new QWidget();

	m_tabWidget->addTab(m_general, tr("General"));
	m_tabWidget->addTab(m_breadboard, m_viewInfoThings[0].viewName);
	m_tabWidget->addTab(m_schematic, m_viewInfoThings[1].viewName);
	m_tabWidget->addTab(m_pcb, m_viewInfoThings[2].viewName);

	QVBoxLayout * vLayout = new QVBoxLayout(this);
	vLayout->addWidget(m_tabWidget);

	initGeneral(m_general, list);

	initBreadboard(m_breadboard, &m_viewInfoThings[0]);
	initSchematic(m_schematic, &m_viewInfoThings[1]);
	initPCB(m_pcb, &m_viewInfoThings[2]);

    QDialogButtonBox * buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
	buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

	vLayout->addWidget(buttonBox);

}

void PrefsDialog::initGeneral(QWidget * widget, QFileInfoList & list)
{
	QVBoxLayout * vLayout = new QVBoxLayout(widget);

	// TODO: if no translation files found, don't put up the translation part of this dialog

	vLayout->addWidget(createLanguageForm(list));
	vLayout->addWidget(createColorForm());
	vLayout->addWidget(createZoomerForm());
	vLayout->addWidget(createAutosaveForm());

#ifndef QT_NO_DEBUG
	vLayout->addWidget(createOtherForm());
#endif

	widget->setLayout(vLayout);
}

void PrefsDialog::initBreadboard(QWidget * widget, ViewInfoThing * viewInfoThing)
{
	QVBoxLayout * vLayout = new QVBoxLayout(widget);

	QWidget * w1 = createGridSizeForm(viewInfoThing);
	vLayout->addWidget(w1);
	
	QWidget * w2 = createBackgroundColorForm(viewInfoThing);
	vLayout->addWidget(w2);

	QWidget * w3 = createCurvyForm(viewInfoThing);
	vLayout->addWidget(w3);

	vLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Preferred, QSizePolicy::Expanding));

	widget->setLayout(vLayout);
}

void PrefsDialog::initSchematic(QWidget * widget, ViewInfoThing * viewInfoThing)
{
	QVBoxLayout * vLayout = new QVBoxLayout(widget);

	QWidget * w1 = createGridSizeForm(viewInfoThing);
	vLayout->addWidget(w1);

	QWidget * w2 = createBackgroundColorForm(viewInfoThing);
	vLayout->addWidget(w2);

	vLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Preferred, QSizePolicy::Expanding));

	widget->setLayout(vLayout);
}

void PrefsDialog::initPCB(QWidget * widget, ViewInfoThing * viewInfoThing)
{
	QVBoxLayout * vLayout = new QVBoxLayout(widget);

	QWidget * w1 = createGridSizeForm(viewInfoThing);
	vLayout->addWidget(w1);

	QWidget * w2 = createBackgroundColorForm(viewInfoThing);
	vLayout->addWidget(w2);

	vLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Preferred, QSizePolicy::Expanding));

	widget->setLayout(vLayout);
}

QWidget * PrefsDialog::createZoomerForm() {
	QGroupBox * zoomer = new QGroupBox(tr("Mouse Wheel Behavior"), this );

	QHBoxLayout * zhlayout = new QHBoxLayout();
	zhlayout->setSpacing(5);

#ifdef Q_WS_MAC
	QString cKey = tr("Command");
#else
	QString cKey = tr("Control");
#endif

	m_wheelLabel = new QLabel(this);
	m_wheelLabel->setWordWrap(true);
	updateWheelText();
	zhlayout->addWidget(m_wheelLabel);

	QPushButton * pushButton = new QPushButton(tr("Change Wheel Behavior"), this);
	connect(pushButton, SIGNAL(clicked()), this, SLOT(changeWheelBehavior()));
	zhlayout->addWidget(pushButton);

	zoomer->setLayout(zhlayout);

	return zoomer;
}

QWidget * PrefsDialog::createAutosaveForm() {
	QGroupBox * autosave = new QGroupBox(tr("Autosave"), this );

	QHBoxLayout * zhlayout = new QHBoxLayout();
	zhlayout->setSpacing(5);

	QCheckBox * box = new QCheckBox(tr("Autosave every:"));
	box->setChecked(MainWindow::AutosaveEnabled);
	zhlayout->addWidget(box);

	zhlayout->addSpacerItem(new QSpacerItem(0,0,QSizePolicy::Expanding));

	QSpinBox * spinBox = new QSpinBox;
	spinBox->setMinimum(1);
	spinBox->setMaximum(60);
	spinBox->setValue(MainWindow::AutosaveTimeoutMinutes);
	spinBox->setMaximumWidth(80);
	zhlayout->addWidget(spinBox);

	QLabel * label = new QLabel(tr("minutes"));
	zhlayout->addWidget(label);

	autosave->setLayout(zhlayout);

	connect(box, SIGNAL(clicked(bool)), this, SLOT(toggleAutosave(bool)));
	connect(spinBox, SIGNAL(valueChanged(int)), this, SLOT(changeAutosavePeriod(int)));


	return autosave;
}


QWidget * PrefsDialog::createLanguageForm(QFileInfoList & list) 
{
	QGroupBox * formGroupBox = new QGroupBox(tr("Language"));
    QFormLayout *layout = new QFormLayout();

	QLabel * languageLabel = new QLabel(this);
	languageLabel->setWordWrap(true);
	languageLabel->setText(QObject::tr("<b>Language</b>"));
	
	QComboBox* comboBox = new QComboBox(this);
	m_translatorListModel = new TranslatorListModel(list, this);
	comboBox->setModel(m_translatorListModel);
	comboBox->setCurrentIndex(m_translatorListModel->findIndex(m_name));
	connect(comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(changeLanguage(int)));

	layout->addRow(languageLabel, comboBox);	

	QLabel * ll = new QLabel(this);
	ll->setFixedWidth(250);
	ll->setMinimumHeight(45);
	ll->setWordWrap(true);
	ll->setText(QObject::tr("Please note that a new language setting will not take effect "
		"until the next time you run Fritzing."));
	layout->addRow(ll);

	formGroupBox->setLayout(layout);
	return formGroupBox;
}

QWidget* PrefsDialog::createColorForm() 
{
	QGroupBox * formGroupBox = new QGroupBox(tr("Colors"));
    QFormLayout *layout = new QFormLayout();

	QLabel * c1 = new QLabel(this);
	c1->setWordWrap(true);
	c1->setText(QObject::tr("<b>Connected highlight color</b>"));

	QColor connectedColor = ItemBase::connectedColor();
	ClickableLabel * cl1 = new ClickableLabel(tr("%1 (click to change...)").arg(connectedColor.name()), this);
	connect(cl1, SIGNAL(clicked()), this, SLOT(setConnectedColor()));
	cl1->setPalette(QPalette(connectedColor));
	cl1->setAutoFillBackground(true);
	cl1->setMargin(MARGIN);
	layout->addRow(c1, cl1);

	QLabel * c2 = new QLabel(this);
	c2->setWordWrap(true);
	c2->setText(QObject::tr("<b>Unconnected highlight color</b>"));

	QColor unconnectedColor = ItemBase::unconnectedColor();
	ClickableLabel * cl2 = new ClickableLabel(tr("%1 (click to change...)").arg(unconnectedColor.name()), this);
	connect(cl2, SIGNAL(clicked()), this, SLOT(setUnconnectedColor()));
	cl2->setPalette(QPalette(unconnectedColor));
	cl2->setAutoFillBackground(true);
	cl2->setMargin(MARGIN);
	layout->addRow(c2, cl2);

	formGroupBox->setLayout(layout);
	return formGroupBox;
}

QWidget* PrefsDialog::createOtherForm() 
{
	QGroupBox * formGroupBox = new QGroupBox(tr("Debug"));
    QFormLayout *layout = new QFormLayout();

	QLabel * clearLabel = new QLabel(this);
	clearLabel->setFixedWidth(195);
	clearLabel->setWordWrap(true);
	clearLabel->setText(QObject::tr("Clear all saved settings and close this dialog (debug mode only)."));	

	QPushButton * clear = new QPushButton(QObject::tr("Clear"), this);
	clear->setMaximumWidth(220);
	connect(clear, SIGNAL(clicked()), this, SLOT(clear()));

	layout->addRow(clearLabel, clear);	

	formGroupBox->setLayout(layout);
	return formGroupBox;
}

void PrefsDialog::changeLanguage(int index) 
{
	const QLocale * locale = m_translatorListModel->locale(index);
	if (locale) {
		m_name = locale->name();
		m_settings.insert("language", m_name);
	}
}

void PrefsDialog::clear() {
	m_cleared = true;
	accept();
}

bool PrefsDialog::cleared() {
	return m_cleared;
}

void PrefsDialog::setConnectedColor() {
	QColor cc = ItemBase::connectedColor();
	QColor scc = ItemBase::standardConnectedColor();

	SetColorDialog setColorDialog(tr("Connected Highlight"), cc, scc, false, this);
	int result = setColorDialog.exec();
	if (result == QDialog::Rejected) return;

	QColor c = setColorDialog.selectedColor();
	m_settings.insert("connectedColor", c.name());
	ClickableLabel * cl = qobject_cast<ClickableLabel *>(sender());
	if (cl) {
		cl->setPalette(QPalette(c));
	}
}

void PrefsDialog::setUnconnectedColor() {
	QColor cc = ItemBase::unconnectedColor();
	QColor scc = ItemBase::standardUnconnectedColor();

	SetColorDialog setColorDialog(tr("Unconnected Highlight"), cc, scc, false, this);
	int result = setColorDialog.exec();
	if (result == QDialog::Rejected) return;

	QColor c = setColorDialog.selectedColor();
	m_settings.insert("unconnectedColor", c.name());
	ClickableLabel * cl = qobject_cast<ClickableLabel *>(sender());
	if (cl) {
		cl->setPalette(QPalette(c));
	}
}

QHash<QString, QString> & PrefsDialog::settings() {
	return m_settings;
}

QHash<QString, QString> & PrefsDialog::tempSettings() {
	return m_tempSettings;
}

void PrefsDialog::changeWheelBehavior() {
	if (++m_wheelMapping >= ZoomableGraphicsView::WheelMappingCount) {
		m_wheelMapping = 0;
	}

	m_settings.insert("wheelMapping", QString("%1").arg(m_wheelMapping));
	updateWheelText();
}

void PrefsDialog::updateWheelText() {
	QString text;
#ifdef Q_WS_MAC
	QString cKey = tr("Command");
#else
	QString cKey = tr("Control");
#endif

	switch((ZoomableGraphicsView::WheelMapping) m_wheelMapping) {
		case ZoomableGraphicsView::ScrollPrimary:
		default:
			text = tr("no keys down = scroll\nshift key swaps scroll axis\nAlt or %1 key = zoom").arg(cKey);
			break;
		case ZoomableGraphicsView::ZoomPrimary:
			text = tr("no keys down = zoom\nAlt or %1 key = scroll\nshift key swaps scroll axis").arg(cKey);
			break;
	}
	m_wheelLabel->setText(text);
}

void PrefsDialog::toggleAutosave(bool checked) {
	m_settings.insert("autosaveEnabled", QString("%1").arg(checked));
}

void PrefsDialog::changeAutosavePeriod(int value) {
	m_settings.insert("autosavePeriod", QString("%1").arg(value));
}

QWidget * PrefsDialog::createGridSizeForm(ViewInfoThing * viewInfoThing)
{
	QGroupBox * over = new QGroupBox(tr("Align-to-Grid size"), this);

	QVBoxLayout * vLayout = new QVBoxLayout();

	QLabel * explain = new QLabel(tr("Set the grid size for %1.").arg(viewInfoThing->viewName));
	vLayout->addWidget(explain);

	QGroupBox * groupBox = new QGroupBox(this);

	QHBoxLayout * hLayout = new QHBoxLayout();

	QLabel * label = new QLabel(tr("Grid Size:"));
	hLayout->addWidget(label);

	viewInfoThing->lineEdit = new QLineEdit();
	
	viewInfoThing->lineEdit->setFixedWidth(45);

	viewInfoThing->validator = new QDoubleValidator(viewInfoThing->lineEdit);
	viewInfoThing->validator->setRange(0.001, 1.0, 3);
	viewInfoThing->validator->setNotation(QDoubleValidator::StandardNotation);
	viewInfoThing->lineEdit->setValidator(viewInfoThing->validator);

	hLayout->addWidget(viewInfoThing->lineEdit);

	viewInfoThing->inButton = new QRadioButton(tr("in"), this); 
	hLayout->addWidget(viewInfoThing->inButton);

	viewInfoThing->mmButton = new QRadioButton(tr("mm"), this); 
	hLayout->addWidget(viewInfoThing->mmButton);

	groupBox->setLayout(hLayout);

	vLayout->addWidget(groupBox);
	vLayout->addSpacing(5);

	QPushButton * pushButton = new QPushButton(this);
	pushButton->setText(tr("Restore Default"));
	pushButton->setMaximumWidth(115);
	vLayout->addWidget(pushButton);
	vLayout->addSpacing(10);

	over->setLayout(vLayout);

	QSettings settings;
	QString szString = settings.value(QString("%1GridSize").arg(viewInfoThing->viewName), "").toString();
	if (szString.isEmpty()) {
		viewInfoThing->inButton->setChecked(true);
		viewInfoThing->lineEdit->setText(QString::number(viewInfoThing->defaultGridSize));
	}
	else {
		if (szString.endsWith("mm")) {
			viewInfoThing->mmButton->setChecked(true);
			viewInfoThing->validator->setTop(25.4);
		}
		else {
			viewInfoThing->inButton->setChecked(true);
		}
		szString.chop(2);
		viewInfoThing->lineEdit->setText(szString);
	}

	viewInfoThing->inButton->setProperty("index", viewInfoThing->index);
	pushButton->setProperty("index", viewInfoThing->index);
	viewInfoThing->mmButton->setProperty("index", viewInfoThing->index);
	viewInfoThing->lineEdit->setProperty("index", viewInfoThing->index);

	connect(viewInfoThing->lineEdit, SIGNAL(editingFinished()), this, SLOT(gridEditingFinished()));
	connect(viewInfoThing->inButton, SIGNAL(clicked(bool)), this, SLOT(units(bool)));
	connect(pushButton, SIGNAL(clicked()), this, SLOT(restoreDefault()));
	connect(viewInfoThing->mmButton, SIGNAL(clicked(bool)), this, SLOT(units(bool)));

	return over;
}

void PrefsDialog::gridEditingFinished() {
	ViewInfoThing * viewInfoThing = &m_viewInfoThings[sender()->property("index").toInt()];
	updateGridSize(viewInfoThing);
}

void PrefsDialog::units(bool checked) {
	ViewInfoThing * viewInfoThing = &m_viewInfoThings[sender()->property("index").toInt()];

	QString units;
	if (sender() == viewInfoThing->inButton) {
		units = (checked) ? "in" : "mm";
	}
	else {
		units = (checked) ? "mm" : "in";
	}
	
	if (units.startsWith("mm")) {
		viewInfoThing->validator->setTop(25.4);
		viewInfoThing->lineEdit->setText(QString::number(viewInfoThing->lineEdit->text().toDouble() * 25.4));
	}
	else {
		viewInfoThing->validator->setTop(1.0);
		viewInfoThing->lineEdit->setText(QString::number(viewInfoThing->lineEdit->text().toDouble() / 25.4));
	}

	updateGridSize(viewInfoThing);
}

void PrefsDialog::restoreDefault() {
	ViewInfoThing * viewInfoThing = &m_viewInfoThings[sender()->property("index").toInt()];

	viewInfoThing->inButton->setChecked(true);
	viewInfoThing->mmButton->setChecked(false);
	viewInfoThing->lineEdit->setText(QString::number(viewInfoThing->defaultGridSize));
	updateGridSize(viewInfoThing);
}

QWidget* PrefsDialog::createBackgroundColorForm(ViewInfoThing * viewInfoThing) 
{
	QGroupBox * groupBox = new QGroupBox(tr("Background color"));
    QVBoxLayout *layout = new QVBoxLayout;

	ClickableLabel * cl1 = new ClickableLabel(tr("%1 (click to change...)").arg(viewInfoThing->currentBColor.name()), this);
	cl1->setProperty("index", viewInfoThing->index);
	connect(cl1, SIGNAL(clicked()), this, SLOT(setBackgroundColor()));
	cl1->setPalette(QPalette(viewInfoThing->currentBColor));
	cl1->setAutoFillBackground(true);
	cl1->setMargin(MARGIN);

	layout->addWidget(cl1);

	groupBox->setLayout(layout);
	return groupBox;
}

void PrefsDialog::setBackgroundColor() {
	ViewInfoThing * viewInfoThing = &m_viewInfoThings[sender()->property("index").toInt()];

	QColor cc = viewInfoThing->currentBColor;
	QColor scc = viewInfoThing->standardBColor;

	SetColorDialog setColorDialog(tr("%1 background Color").arg(viewInfoThing->viewName), cc, scc, true, this);
	int result = setColorDialog.exec();
	if (result == QDialog::Rejected) return;

	viewInfoThing->currentBColor = setColorDialog.selectedColor();
	ClickableLabel * cl = qobject_cast<ClickableLabel *>(sender());
	if (cl) {
		cl->setPalette(QPalette(viewInfoThing->currentBColor));
	}

	if (setColorDialog.isPrefsColor()) {
		QSettings settings;
		m_settings.insert(QString("%1BackgroundColor").arg(viewInfoThing->shortName), setColorDialog.selectedColor().name());
	}

	m_tempSettings.insert(QString("%1BackgroundColor").arg(viewInfoThing->shortName), setColorDialog.selectedColor().name());
}

void PrefsDialog::updateGridSize(ViewInfoThing * viewInfoThing) {
	QString units = (viewInfoThing->inButton->isChecked() ? "in" : "mm");
	m_settings.insert(QString("%1GridSize").arg(viewInfoThing->viewName), viewInfoThing->lineEdit->text() + units);
}


QWidget* PrefsDialog::createCurvyForm(ViewInfoThing * viewInfoThing) 
{
	QGroupBox * groupBox = new QGroupBox(tr("Curvy vs. straight wires"));
    QVBoxLayout *layout = new QVBoxLayout;

	QLabel * label = new QLabel(tr("When you mouse-down and drag on a wire or the leg of a part (as opposed to a connector or a bendpoint) "
									"do you want to change the curvature of the wire (or leg) or drag out a new bendpoint?\n\n" 
									"This checkbox sets the default behavior. "
									"You can switch back to the non-default behavior by holding down the Control key (Mac: Command key) when you drag.\n"));
	label->setWordWrap(true);
	layout->addWidget(label);

	QCheckBox * checkbox = new QCheckBox("Curvy wires and legs");
	checkbox->setProperty("index", viewInfoThing->index);
	checkbox->setChecked(viewInfoThing->curvy);
	connect(checkbox, SIGNAL(clicked()), this, SLOT(curvyChanged()));
	layout->addWidget(checkbox);

	groupBox->setLayout(layout);
	return groupBox;
}

void PrefsDialog::curvyChanged() {
	QCheckBox * checkBox = qobject_cast<QCheckBox *>(sender());
	if (checkBox == NULL) return;

	ViewInfoThing * viewInfoThing = &m_viewInfoThings[sender()->property("index").toInt()];
	m_settings.insert(QString("%1CurvyWires").arg(viewInfoThing->shortName), checkBox->isChecked() ? "1" : "0");
}
