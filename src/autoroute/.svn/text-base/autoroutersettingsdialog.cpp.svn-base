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


///////////////////////////////////////

// todo: 
//	save and reload as settings
//	enable/disable custom on radio presses
//	change wording on custom via
//	actually modify the autorouter
//	enable single vs. double-sided settings


///////////////////////////////////////

#include "autoroutersettingsdialog.h"

#include <QLabel>
#include <QGroupBox>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QSettings>
#include <QComboBox>

#include "../items/tracewire.h"
#include "../items/hole.h"
#include "../fsvgrenderer.h"
#include "../sketch/pcbsketchwidget.h"
#include "../utils/textutils.h"
#include "../utils/graphicsutils.h"


const QString AutorouterSettingsDialog::AutorouteTraceWidth = "autorouteTraceWidth";

AutorouterSettingsDialog::AutorouterSettingsDialog(QWidget *parent) : QDialog(parent) 
{
	{
		QSettings settings;
		m_traceWidth = settings.value(AutorouteTraceWidth, "0").toInt();
		if (m_traceWidth == 0) {
			m_traceWidth = GraphicsUtils::pixels2mils(Wire::STANDARD_TRACE_WIDTH, FSvgRenderer::printerScale());
			settings.setValue(AutorouteTraceWidth, m_traceWidth);
		}
	}

	Hole::initHoleSettings(m_holeSettings);
	m_holeSettings.ringThicknessRange = Via::ringThicknessRange;
	m_holeSettings.holeDiameterRange = Via::holeDiameterRange;

	PCBSketchWidget::getDefaultViaSize(m_holeSettings.ringThickness, m_holeSettings.holeDiameter);

	this->setWindowTitle(QObject::tr("Auorouter Settings"));

	QVBoxLayout * windowLayout = new QVBoxLayout();
	this->setLayout(windowLayout);

	QGroupBox * prodGroupBox = new QGroupBox(tr("Production type"), this);
	QVBoxLayout * prodLayout = new QVBoxLayout();
	prodGroupBox->setLayout(prodLayout);

	m_homebrewButton = new QRadioButton(tr("homebrew"), this); 
	connect(m_homebrewButton, SIGNAL(clicked(bool)), this, SLOT(production(bool)));

	m_professionalButton = new QRadioButton(tr("professional"), this); 
	connect(m_professionalButton, SIGNAL(clicked(bool)), this, SLOT(production(bool)));

	m_customButton = new QRadioButton(tr("custom"), this); 
	connect(m_customButton, SIGNAL(clicked(bool)), this, SLOT(production(bool)));

	m_customFrame = new QFrame(this);
	QHBoxLayout * customFrameLayout = new QHBoxLayout(this);
	m_customFrame->setLayout(customFrameLayout);

	customFrameLayout->addSpacing(10);

	QFrame * innerFrame = new QFrame(this);
	QVBoxLayout * innerFrameLayout = new QVBoxLayout(this);
	innerFrame->setLayout(innerFrameLayout);

	QGroupBox * viaGroupBox = new QGroupBox("Via size", this);
	QVBoxLayout * viaLayout = new QVBoxLayout();
	viaGroupBox->setLayout(viaLayout);

	QWidget * viaWidget = Hole::createHoleSettings(viaGroupBox, m_holeSettings, true, "");

	connect(m_holeSettings.sizesComboBox, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(changeHoleSize(const QString &)));
	connect(m_holeSettings.mmRadioButton, SIGNAL(toggled(bool)), this, SLOT(changeUnits(bool)));
	connect(m_holeSettings.inRadioButton, SIGNAL(toggled(bool)), this, SLOT(changeUnits(bool)));
	connect(m_holeSettings.diameterEdit, SIGNAL(editingFinished()), this, SLOT(changeDiameter()));
	connect(m_holeSettings.thicknessEdit, SIGNAL(editingFinished()), this, SLOT(changeThickness()));

	enableCustom(initRadios());

	QGroupBox * traceGroupBox = new QGroupBox(tr("Trace width"), this);
	QBoxLayout * traceLayout = new QVBoxLayout();
	traceGroupBox->setLayout(traceLayout);

	m_traceWidthComboBox = TraceWire::createWidthComboBox(m_traceWidth, traceGroupBox);
	connect(m_traceWidthComboBox, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(widthEntry(const QString &)));

    QDialogButtonBox * buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
	buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(acceptAnd()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

	viaLayout->addWidget(viaWidget);
	traceLayout->addWidget(m_traceWidthComboBox);

	innerFrameLayout->addWidget(viaGroupBox);
	innerFrameLayout->addWidget(traceGroupBox);

	customFrameLayout->addWidget(innerFrame);

	prodLayout->addWidget(m_homebrewButton);
	prodLayout->addWidget(m_professionalButton);
	prodLayout->addWidget(m_customButton);
	prodLayout->addWidget(m_customFrame);

	windowLayout->addWidget(prodGroupBox);

	windowLayout->addSpacerItem(new QSpacerItem(1, 10, QSizePolicy::Preferred, QSizePolicy::Expanding));

	windowLayout->addWidget(buttonBox);
	
}

AutorouterSettingsDialog::~AutorouterSettingsDialog() {
}

void AutorouterSettingsDialog::production(bool checked) {
	Q_UNUSED(checked);

	QString units;
	if (sender() == m_homebrewButton) {
		enableCustom(false);
		changeHoleSize(sender()->property("holesize").toString() + "," + sender()->property("ringthickness").toString());
		setTraceWidth(16);
	}
	else if (sender() == m_professionalButton) {
		enableCustom(false);
		changeHoleSize(sender()->property("holesize").toString() + "," + sender()->property("ringthickness").toString());
		setTraceWidth(24);
	}
	else if (sender() == m_customButton) {
		enableCustom(true);
	}	
}

void AutorouterSettingsDialog::acceptAnd() {
	QSettings settings;
	settings.setValue(Hole::AutorouteViaHoleSize, m_holeSettings.holeDiameter);
	Hole::DefaultAutorouteViaHoleSize = m_holeSettings.holeDiameter;
	settings.setValue(Hole::AutorouteViaRingThickness, m_holeSettings.ringThickness);
	Hole::DefaultAutorouteViaRingThickness = m_holeSettings.ringThickness;
	settings.setValue(AutorouteTraceWidth, m_traceWidth);
	
	accept();
}

void AutorouterSettingsDialog::restoreDefault() {
	//m_inButton->setChecked(true);
	//m_mmButton->setChecked(false);
}

void AutorouterSettingsDialog::enableCustom(bool enable) 
{
	m_holeSettings.diameterEdit->setEnabled(enable);
	m_holeSettings.thicknessEdit->setEnabled(enable);
	m_holeSettings.mmRadioButton->setEnabled(enable);
	m_holeSettings.inRadioButton->setEnabled(enable);
	m_holeSettings.sizesComboBox->setEnabled(enable);
	m_customFrame->setVisible(enable);
}

bool AutorouterSettingsDialog::initRadios() 
{
	bool custom = true;
	foreach (QString name, Hole::HoleSizes.keys()) {
		QStringList values = Hole::HoleSizes.value(name).split(",");
		QString ringThickness = values[1];
		QString holeSize = values[0];
		if (!name.isEmpty() && !ringThickness.isEmpty() && !holeSize.isEmpty()) {
			QRadioButton * button = NULL;
			if (name.contains("homebrew", Qt::CaseInsensitive)) button = m_homebrewButton;
			else if (name.contains("professional", Qt::CaseInsensitive)) button = m_professionalButton;
			if (button) {
				button->setProperty("ringthickness", ringThickness);
				button->setProperty("holesize", holeSize);
				if (ringThickness.compare(m_holeSettings.ringThickness, Qt::CaseInsensitive) == 0 && holeSize.compare(m_holeSettings.holeDiameter, Qt::CaseInsensitive) == 0) {
					button->setChecked(true);
					custom = false;
				}
			}
		}
	}	

	m_customButton->setChecked(custom);

	return custom;
}

void AutorouterSettingsDialog::widthEntry(const QString & text) {
	int w = TraceWire::widthEntry(text, sender());
	if (w == 0) return;

	m_traceWidth = w;
}

void AutorouterSettingsDialog::changeHoleSize(const QString & newSize) {
	QString s = newSize;
	Hole::setHoleSize(s, false, m_holeSettings);
}

void AutorouterSettingsDialog::changeUnits(bool) 
{
	QString newVal = Hole::changeUnits(m_holeSettings.currentUnits(), m_holeSettings);
}

void AutorouterSettingsDialog::changeDiameter() 
{
	if (Hole::changeDiameter(m_holeSettings, sender())) {
		QLineEdit * edit = qobject_cast<QLineEdit *>(sender());
		changeHoleSize(edit->text() + m_holeSettings.currentUnits() + "," + m_holeSettings.ringThickness);
	}
}

void AutorouterSettingsDialog::changeThickness() 
{
	if (Hole::changeThickness(m_holeSettings, sender())) {
		QLineEdit * edit = qobject_cast<QLineEdit *>(sender());
		changeHoleSize(m_holeSettings.holeDiameter + "," + edit->text() + m_holeSettings.currentUnits());
	}	
}


void AutorouterSettingsDialog::setTraceWidth(int width)
{
	for (int i = 0; i > m_traceWidthComboBox->count(); i++) {
		if (m_traceWidthComboBox->itemData(i).toInt() == width) {
			m_traceWidthComboBox->setCurrentIndex(i);
			return;
		}
	}
}
