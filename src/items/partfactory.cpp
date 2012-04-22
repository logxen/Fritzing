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

$Revision: 5899 $:
$Author: cohen@irascible.com $:
$Date: 2012-03-09 01:52:17 -0800 (Fri, 09 Mar 2012) $

********************************************************************/

#include "partfactory.h"
#include "../viewgeometry.h"
#include "../model/modelpart.h"
#include "../model/modelbase.h"
#include "paletteitem.h"
#include "symbolpaletteitem.h"
#include "wire.h"
#include "virtualwire.h"
#include "tracewire.h"
#include "jumperitem.h"
#include "resizableboard.h"
#include "logoitem.h"
#include "schematicframe.h"
#include "resistor.h"
#include "moduleidnames.h"
#include "mysterypart.h"
#include "groundplane.h"
#include "note.h"
#include "ruler.h"
#include "dip.h"
#include "pinheader.h"
#include "screwterminal.h"
#include "hole.h"
#include "via.h"
#include "pad.h"
#include "capacitor.h"
#include "perfboard.h"
#include "breadboard.h"
#include "stripboard.h"
#include "led.h"
#include "../utils/folderutils.h"
#include "../utils/lockmanager.h"

static QString PartFactoryFolderPath;
static QHash<QString, LockedFile *> LockedFiles;

ItemBase * PartFactory::createPart( ModelPart * modelPart, ViewLayer::ViewLayerSpec viewLayerSpec, ViewIdentifierClass::ViewIdentifier viewIdentifier, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu, QMenu * wireMenu, bool doLabel)
{
	modelPart->setModelIndexFromMultiplied(id);			// make sure the model index is synched with the id; this is not always the case when parts are first created.
	ItemBase * itemBase = createPartAux(modelPart, viewIdentifier, viewGeometry, id, itemMenu, wireMenu, doLabel);
	if (itemBase) {
		itemBase->setViewLayerSpec(viewLayerSpec);
	}
	return itemBase;
}

ItemBase * PartFactory::createPartAux( ModelPart * modelPart, ViewIdentifierClass::ViewIdentifier viewIdentifier, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu, QMenu * wireMenu, bool doLabel)
{
	switch (modelPart->itemType()) {
		case ModelPart::Wire:
		{
			bool ratsnest = viewGeometry.getRatsnest();
			if (ratsnest) {
				return new VirtualWire(modelPart, viewIdentifier, viewGeometry, id, wireMenu);		
			}
			if (viewGeometry.getAnyTrace()) {
				TraceWire * traceWire = new TraceWire(modelPart, viewIdentifier, viewGeometry, id, wireMenu);
				return traceWire;
			}
			return new Wire(modelPart, viewIdentifier, viewGeometry, id, wireMenu, false);

		}
		case ModelPart::Note:
			return new Note(modelPart, viewIdentifier, viewGeometry, id, NULL);
		case ModelPart::CopperFill:
			return new GroundPlane(modelPart, viewIdentifier, viewGeometry, id, itemMenu, doLabel);
		case ModelPart::Jumper:
			return new JumperItem(modelPart, viewIdentifier, viewGeometry, id, itemMenu, doLabel);
		case ModelPart::ResizableBoard:
			return new ResizableBoard(modelPart, viewIdentifier, viewGeometry, id, itemMenu, doLabel);
		case ModelPart::Board:
			return new Board(modelPart, viewIdentifier, viewGeometry, id, itemMenu, doLabel);
		case ModelPart::Logo:
			if (modelPart->moduleID().startsWith("copper", Qt::CaseInsensitive)) {
				return new CopperLogoItem(modelPart, viewIdentifier, viewGeometry, id, itemMenu, doLabel);
			}
			return new LogoItem(modelPart, viewIdentifier, viewGeometry, id, itemMenu, doLabel);
		case ModelPart::Ruler:
			return new Ruler(modelPart, viewIdentifier, viewGeometry, id, itemMenu, doLabel);
		case ModelPart::Symbol:
			return new SymbolPaletteItem(modelPart, viewIdentifier, viewGeometry, id, itemMenu, doLabel);
		case ModelPart::Via:
			return new Via(modelPart, viewIdentifier, viewGeometry, id, itemMenu, doLabel);
		case ModelPart::Hole:
			return new Hole(modelPart, viewIdentifier, viewGeometry, id, itemMenu, doLabel);
		default:
			{
				QString moduleID = modelPart->moduleID();
				if (moduleID.endsWith(ModuleIDNames::ModuleIDNameSuffix)) {
					if (moduleID.endsWith(ModuleIDNames::ResistorModuleIDName)) {
						return new Resistor(modelPart, viewIdentifier, viewGeometry, id, itemMenu, doLabel);
					}
					if (moduleID.endsWith(ModuleIDNames::CapacitorModuleIDName)) {
						return new Capacitor(modelPart, viewIdentifier, viewGeometry, id, itemMenu, doLabel);
					}
					if (moduleID.endsWith(ModuleIDNames::CrystalModuleIDName)) {
						return new Capacitor(modelPart, viewIdentifier, viewGeometry, id, itemMenu, doLabel);
					}
					if (moduleID.endsWith(ModuleIDNames::ThermistorModuleIDName)) {
						return new Capacitor(modelPart, viewIdentifier, viewGeometry, id, itemMenu, doLabel);
					}
					if (moduleID.endsWith(ModuleIDNames::ZenerDiodeModuleIDName)) {
						return new Capacitor(modelPart, viewIdentifier, viewGeometry, id, itemMenu, doLabel);
					}
					if (moduleID.endsWith(ModuleIDNames::PotentiometerModuleIDName)) {
						return new Capacitor(modelPart, viewIdentifier, viewGeometry, id, itemMenu, doLabel);
					}
					if (moduleID.endsWith(ModuleIDNames::InductorModuleIDName)) {
						return new Capacitor(modelPart, viewIdentifier, viewGeometry, id, itemMenu, doLabel);
					}
					if (moduleID.endsWith(ModuleIDNames::ColorLEDModuleIDName)) {
						return new LED(modelPart, viewIdentifier, viewGeometry, id, itemMenu, doLabel);
					}
					if (moduleID.endsWith(ModuleIDNames::ColorFluxLEDModuleIDName)) {
						return new LED(modelPart, viewIdentifier, viewGeometry, id, itemMenu, doLabel);
					}
					if (moduleID.endsWith(ModuleIDNames::LEDModuleIDName)) {
						return new Capacitor(modelPart, viewIdentifier, viewGeometry, id, itemMenu, doLabel);
					}
					if (moduleID.endsWith(ModuleIDNames::PerfboardModuleIDName)) {
						return new Perfboard(modelPart, viewIdentifier, viewGeometry, id, itemMenu, doLabel);
					}
					if (moduleID.endsWith(ModuleIDNames::StripboardModuleIDName)) {
						return new Stripboard(modelPart, viewIdentifier, viewGeometry, id, itemMenu, doLabel);
					}
					if (moduleID.endsWith(ModuleIDNames::SchematicFrameModuleIDName)) {
						return new SchematicFrame(modelPart, viewIdentifier, viewGeometry, id, itemMenu, doLabel);
					}
					if (moduleID.compare(ModuleIDNames::PadModuleIDName) == 0) {
						return new Pad(modelPart, viewIdentifier, viewGeometry, id, itemMenu, doLabel);
					}
					// must get the subclasses first
					if (modelPart->itemType() == ModelPart::Breadboard) {
						return new Breadboard(modelPart, viewIdentifier, viewGeometry, id, itemMenu, doLabel);
					}

				}
				QString family = modelPart->properties().value("family", "");
				if (family.compare("mystery part", Qt::CaseInsensitive) == 0) {
					return new MysteryPart(modelPart, viewIdentifier, viewGeometry, id, itemMenu, doLabel);
				}
				if (family.compare("screw terminal", Qt::CaseInsensitive) == 0) {
					return new ScrewTerminal(modelPart, viewIdentifier, viewGeometry, id, itemMenu, doLabel);
				}
				if (family.compare("pin header", Qt::CaseInsensitive) == 0) {
					return new PinHeader(modelPart, viewIdentifier, viewGeometry, id, itemMenu, doLabel);
				}
				if (family.compare("generic IC", Qt::CaseInsensitive) == 0) {
					return new Dip(modelPart, viewIdentifier, viewGeometry, id, itemMenu, doLabel);
				}
				return new PaletteItem(modelPart, viewIdentifier, viewGeometry, id, itemMenu, doLabel);

			}
	}
}

QString PartFactory::getSvgFilename(ModelPart * modelPart, const QString & expectedFileName) {
	Q_UNUSED(modelPart);

	if (expectedFileName.startsWith("pcb/dip_", Qt::CaseInsensitive)) {
		return getSvgFilenameAux(expectedFileName, &Dip::makePcbSvg);
	}

	if (expectedFileName.startsWith("breadboard/mystery_part_", Qt::CaseInsensitive)) {
		return getSvgFilenameAux(expectedFileName, &MysteryPart::makeBreadboardSvg);
	}

	if (expectedFileName.startsWith("breadboard/screw_terminal_", Qt::CaseInsensitive)) {
		return getSvgFilenameAux(expectedFileName, &ScrewTerminal::makeBreadboardSvg);
	}

	if (expectedFileName.startsWith("schematic/screw_terminal_", Qt::CaseInsensitive)) {
		return getSvgFilenameAux(expectedFileName, &ScrewTerminal::makeSchematicSvg);
	}

	if (expectedFileName.startsWith("pcb/screw_terminal_", Qt::CaseInsensitive)) {
		return getSvgFilenameAux(expectedFileName, &ScrewTerminal::makePcbSvg);
	}

	if (expectedFileName.startsWith("breadboard/generic_sip_", Qt::CaseInsensitive)) {
		return getSvgFilenameAux(expectedFileName, &Dip::makeBreadboardSvg);
	}

	if (expectedFileName.startsWith("schematic/mystery_part_", Qt::CaseInsensitive)) {
		return getSvgFilenameAux(expectedFileName, &MysteryPart::makeSchematicSvg);
	}

	if (expectedFileName.startsWith("schematic/generic_sip_", Qt::CaseInsensitive)) {
		return getSvgFilenameAux(expectedFileName, &MysteryPart::makeSchematicSvg);
	}

	if (expectedFileName.startsWith("schematic/generic_ic_dip_", Qt::CaseInsensitive)) {
		return getSvgFilenameAux(expectedFileName, &Dip::makeSchematicSvg);
	}

	if (expectedFileName.startsWith("breadboard/generic_ic_dip_", Qt::CaseInsensitive)) {
		return getSvgFilenameAux(expectedFileName, &Dip::makeBreadboardSvg);
	}

	if (expectedFileName.startsWith("pcb/nsjumper_", Qt::CaseInsensitive)) {
		return getSvgFilenameAux(expectedFileName, &PinHeader::makePcbSvg);
	}

	if (expectedFileName.startsWith("pcb/jumper_", Qt::CaseInsensitive)) {
		return getSvgFilenameAux(expectedFileName, &PinHeader::makePcbSvg);
	}

	if (expectedFileName.startsWith("pcb/shrouded_", Qt::CaseInsensitive)) {
		return getSvgFilenameAux(expectedFileName, &PinHeader::makePcbSvg);
	}

	if (expectedFileName.contains("pin_header", Qt::CaseInsensitive)) {
		if (expectedFileName.contains("schematic", Qt::CaseInsensitive)) {
			return getSvgFilenameAux(expectedFileName, &PinHeader::makeSchematicSvg);
		}
		else if (expectedFileName.contains("bread", Qt::CaseInsensitive)) {
			return getSvgFilenameAux(expectedFileName, &PinHeader::makeBreadboardSvg);		
		}
		else if (expectedFileName.contains("pcb", Qt::CaseInsensitive)) {
			return getSvgFilenameAux(expectedFileName, &PinHeader::makePcbSvg);		
		}
	}

	if (expectedFileName.contains("perfboard", Qt::CaseInsensitive)) {
		if (expectedFileName.contains("icon")) return expectedFileName;

		return getSvgFilenameAux(expectedFileName, &Perfboard::makeBreadboardSvg);		
	}

	if (expectedFileName.contains("stripboard", Qt::CaseInsensitive)) {
		if (expectedFileName.contains("icon")) return expectedFileName;

		return getSvgFilenameAux(expectedFileName, &Stripboard::makeBreadboardSvg);		
	}

	return "";
}

QString PartFactory::getSvgFilenameAux(const QString & expectedFileName, QString (*getSvg)(const QString &))
{
	QString path = FolderUtils::getApplicationSubFolderPath("parts") + "/"+ ItemBase::SvgFilesDir + "/core/";
	if (QFileInfo(path + expectedFileName).exists()) return expectedFileName;

	path = PartFactoryFolderPath + "/svg/core/" + expectedFileName;
	QFile file(path);
	if (file.exists()) {
		return path;
	} 

	QString svg = (*getSvg)(expectedFileName);
	if (file.open(QFile::WriteOnly)) {
		QTextStream stream(&file);
		stream.setCodec("UTF-8");
		stream << svg;
		file.close();
		return path;
	}

	return "";
}

QString PartFactory::getFzpFilenameAux(const QString & moduleID, QString (*getFzp)(const QString &))
{
	QString path = PartFactoryFolderPath + "/core/" + moduleID + FritzingPartExtension;
	QFile file(path);
	if (file.exists()) {
		return path;
	}

	QString fzp = (*getFzp)(moduleID);
	if (file.open(QFile::WriteOnly)) {
		QTextStream stream(&file);
		stream.setCodec("UTF-8");
		stream << fzp;
		file.close();
		return path;
	}

	return "";
}


QString PartFactory::getFzpFilename(const QString & moduleID) 
{
	if (moduleID.endsWith(ModuleIDNames::PerfboardModuleIDName)) {
		return getFzpFilenameAux(moduleID, &Perfboard::genFZP);
	}

	if (moduleID.endsWith(ModuleIDNames::StripboardModuleIDName)) {
		return getFzpFilenameAux(moduleID, &Stripboard::genFZP);
	}

	if (moduleID.startsWith("generic_ic_dip")) {
		return getFzpFilenameAux(moduleID, &Dip::genDipFZP);
	}

	if (moduleID.startsWith("screw_terminal")) {
		return getFzpFilenameAux(moduleID, &ScrewTerminal::genFZP);
	}

	if (moduleID.startsWith("generic_sip")) {
		return getFzpFilenameAux(moduleID, &Dip::genSipFZP);
	}

	if (moduleID.contains("_pin_header_") && moduleID.startsWith("generic_")) {
		return getFzpFilenameAux(moduleID, &PinHeader::genFZP);
	}

	if (moduleID.startsWith("mystery_part")) {
		if (moduleID.contains("dip", Qt::CaseInsensitive)) {
			return getFzpFilenameAux(moduleID, &MysteryPart::genDipFZP);
		}
		else {
			return getFzpFilenameAux(moduleID, &MysteryPart::genSipFZP);
		}
	}

	return "";
}

void PartFactory::initFolder()
{
	LockManager::initLockedFiles("partfactory", PartFactoryFolderPath, LockedFiles, LockManager::SlowTime);
	QFileInfoList backupList;
	LockManager::checkLockedFiles("partfactory", backupList, LockedFiles, true, LockManager::SlowTime);
	FolderUtils::makePartFolderHierarchy(PartFactoryFolderPath, "core");
}

void PartFactory::cleanup()
{
	LockManager::releaseLockedFiles(PartFactoryFolderPath, LockedFiles);
}

ModelPart * PartFactory::fixObsoleteModuleID(QDomDocument & domDocument, QDomElement & instance, QString & moduleIDRef, ModelBase * referenceModel) {
	// TODO: less hard-coding
	QRegExp oldDip("generic_ic_dip_(\\d{1,2})_(\\d{3}mil)");
	if (oldDip.indexIn(moduleIDRef) == 0) {
		QString spacing = oldDip.cap(2);
		QString pins = oldDip.cap(1);
		moduleIDRef = QString("generic_ic_dip_%1_300mil").arg(pins);
		ModelPart * modelPart = referenceModel->retrieveModelPart(moduleIDRef);
		if (modelPart != NULL) {
			instance.setAttribute("moduleIdRef", moduleIDRef);
			QDomElement prop = domDocument.createElement("property");
			instance.appendChild(prop);
			prop.setAttribute("name", "spacing");
			prop.setAttribute("value", spacing);
			return modelPart;
		}
	}

	if (moduleIDRef.startsWith("generic_male")) {
		ModelPart * modelPart = referenceModel->retrieveModelPart(moduleIDRef);
		if (modelPart != NULL) {
			instance.setAttribute("moduleIdRef", moduleIDRef);
			QDomElement prop = domDocument.createElement("property");
			instance.appendChild(prop);
			prop.setAttribute("name", "form");
			prop.setAttribute("value", PinHeader::MaleFormString);
			return modelPart;
		}
	}

	if (moduleIDRef.startsWith("generic_rounded_female")) {
		ModelPart * modelPart = referenceModel->retrieveModelPart(moduleIDRef);
		if (modelPart != NULL) {
			instance.setAttribute("moduleIdRef", moduleIDRef);
			QDomElement prop = domDocument.createElement("property");
			instance.appendChild(prop);
			prop.setAttribute("name", "form");
			prop.setAttribute("value", PinHeader::FemaleRoundedFormString);
			return modelPart;
		}
	}

	return NULL;
}

QString PartFactory::folderPath() {
	return PartFactoryFolderPath;
}
