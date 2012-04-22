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

$Revision: 5759 $:
$Author: cohen@irascible.com $:
$Date: 2012-01-15 22:45:36 -0800 (Sun, 15 Jan 2012) $

********************************************************************/

#include "palettemodel.h"
#include <QFile>
#include <QMessageBox>
#include <QApplication>
#include <QDir>
#include <QDomElement>

#include "../debugdialog.h"
#include "modelpart.h"
#include "../version/version.h"
#include "../layerattributes.h"
#include "../utils/folderutils.h"
#include "../utils/textutils.h"
#include "../items/moduleidnames.h"

bool PaletteModel::CreateAllPartsBinFile = false;  // now generating the all parts bin in advance using a python script:  [fritzing]/part-gen-scripts/misc_scripts/genAllParts.py

bool PaletteModel::CreateNonCorePartsBinFile = false;
bool PaletteModel::CreateContribPartsBinFile = true;
bool PaletteModel::CreateTempPartsBinFile = true;

static bool JustAppendAllPartsInstances = false;
static bool FirstTime = true;
static bool FirstTimeWrite = true;

QString PaletteModel::AllPartsBinFilePath = ___emptyString___;
QString PaletteModel::NonCorePartsBinFilePath = ___emptyString___;
QString PaletteModel::ContribPartsBinFilePath = ___emptyString___;
QString PaletteModel::TempPartsBinFilePath = ___emptyString___;

static QString FritzingContribPath;

const static QString InstanceTemplate(
        		"\t\t<instance moduleIdRef=\"%1\" path=\"%2\">\n"
				"\t\t\t<views>\n"
        		"\t\t\t\t<iconView layer=\"icon\">\n"
        		"\t\t\t\t\t<geometry z=\"-1\" x=\"-1\" y=\"-1\"></geometry>\n"
        		"\t\t\t\t</iconView>\n"
        		"\t\t\t</views>\n"
        		"\t\t</instance>\n");

PaletteModel::PaletteModel() : ModelBase(true) {
	m_loadedFromFile = false;
	m_loadingCore = false;
	m_loadingContrib = false;
}

PaletteModel::PaletteModel(bool makeRoot, bool doInit, bool fastLoad) : ModelBase( makeRoot ) {
	m_loadedFromFile = false;
	m_loadingCore = false;
	m_loadingContrib = false;

	if (doInit) {
		initParts(fastLoad);
	}
}

void PaletteModel::initParts(bool fastLoad) {
	QDir * dir = FolderUtils::getApplicationSubFolder("parts");
	if (dir == NULL) {
	    QMessageBox::information(NULL, QObject::tr("Fritzing"),
	                             QObject::tr("Parts folder not found.") );
		return;
	}

	FritzingContribPath = dir->absoluteFilePath("contrib");
	delete dir;

	loadParts(fastLoad);
	if (m_root == NULL) {
	    QMessageBox::information(NULL, QObject::tr("Fritzing"),
	                             QObject::tr("No parts found.") );
	}
}

void PaletteModel::initNames() {
	AllPartsBinFilePath = FolderUtils::getApplicationSubFolderPath("bins")+"/allParts.dbg" + FritzingBinExtension;
	NonCorePartsBinFilePath = FolderUtils::getUserDataStorePath("bins")+"/nonCoreParts" + FritzingBinExtension;
	ContribPartsBinFilePath = FolderUtils::getUserDataStorePath("bins")+"/contribParts" + FritzingBinExtension;
	TempPartsBinFilePath = FolderUtils::getUserDataStorePath("bins")+"/tempParts" + FritzingBinExtension;
}

ModelPart * PaletteModel::retrieveModelPart(const QString & moduleID) {
	ModelPart * modelPart = m_partHash.value(moduleID, NULL);
	if (modelPart != NULL) return modelPart;

	if (m_referenceModel != NULL) {
		return m_referenceModel->retrieveModelPart(moduleID);
	}

	return NULL;
}

bool PaletteModel::containsModelPart(const QString & moduleID) {
	return m_partHash.contains(moduleID);
}

void PaletteModel::updateOrAddModelPart(const QString & moduleID, ModelPart *newOne) {
	ModelPart *oldOne = m_partHash.value(moduleID, NULL);
	if(oldOne) {
		oldOne->copy(newOne);
	} else {
		m_partHash.insert(moduleID, newOne);
	}
}

void PaletteModel::loadParts(bool fastLoad) {
	QStringList nameFilters;
	nameFilters << "*" + FritzingPartExtension;

	JustAppendAllPartsInstances = true;   
	/// !!!!!!!!!!!!!!!!  "JustAppendAllPartsInstances = CreateAllPartsBinFile"
	/// !!!!!!!!!!!!!!!!  is incorrect
	/// !!!!!!!!!!!!!!!!  this flag was originally set up because sometimes we were appending a
	/// !!!!!!!!!!!!!!!!  single instance into an already existing file,
	/// !!!!!!!!!!!!!!!!  so simply appending new items as text gave us xml errors.
	/// !!!!!!!!!!!!!!!!  The problem was that there was no easy way to set the flag directly on the actual
	/// !!!!!!!!!!!!!!!!  function being used:  PaletteModel::LoadPart(), though maybe this deserves another look.
	/// !!!!!!!!!!!!!!!!  However, since we're starting from scratch in LoadParts, we can use the much faster 
	/// !!!!!!!!!!!!!!!!  file append method.  Since CreateAllPartsBinFile is false in release mode,
	/// !!!!!!!!!!!!!!!!  Fritzing was taking forever to start up.


	if (FirstTime) {
		writeCommonBinsHeader();
	}

	int totalPartCount = 0;
	emit loadedPart(0, totalPartCount);


	QDir * dir1 = FolderUtils::getApplicationSubFolder("parts");
	if (dir1 != NULL) {
		countParts(*dir1, nameFilters, totalPartCount);
	}
	QDir dir2(FolderUtils::getUserDataStorePath("parts"));
	countParts(dir2, nameFilters, totalPartCount);
	QDir dir3(":/resources/parts");
	countParts(dir3, nameFilters, totalPartCount);


	int loadingPart = 0;
	if (dir1 != NULL) {
		loadPartsAux(*dir1, nameFilters, loadingPart, totalPartCount, fastLoad);
		delete dir1;
	}

	loadPartsAux(dir2, nameFilters, loadingPart, totalPartCount, fastLoad);
	loadPartsAux(dir3, nameFilters, loadingPart, totalPartCount, fastLoad);  

	if (FirstTime) {
		writeCommonBinsFooter();
	}
	
	JustAppendAllPartsInstances = false;   
	/// !!!!!!!!!!!!!!!!  "JustAppendAllPartsInstances = !CreateAllPartsBinFile"
	/// !!!!!!!!!!!!!!!!  is incorrect
	/// !!!!!!!!!!!!!!!!  See above.  We simply want to restore the default, so that other functions calling
	/// !!!!!!!!!!!!!!!!  writeInstanceInCommonBin via LoadPart() will use the slower DomDocument methods,
	/// !!!!!!!!!!!!!!!!  since in that case we are appending to an already existing file.

	FirstTime = false;

}

void PaletteModel::writeCommonBinsHeader() {
	writeCommonBinsHeaderAux(CreateAllPartsBinFile, AllPartsBinFilePath, "All Parts");
	writeCommonBinsHeaderAux(CreateNonCorePartsBinFile, NonCorePartsBinFilePath, "All my parts");
	writeCommonBinsHeaderAux(CreateContribPartsBinFile, ContribPartsBinFilePath, "Contributed Parts");
	writeCommonBinsHeaderAux(CreateTempPartsBinFile, TempPartsBinFilePath, "Temporary Parts");
}

void PaletteModel::writeCommonBinsHeaderAux(bool doIt, const QString &filename, const QString &binName) {
	QString header =
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" + 
		TextUtils::CreatedWithFritzingXmlComment +
		QString("<module fritzingVersion='%1' icon='%2.png'>\n").arg(Version::versionString()).arg(binName) +
		QString("\t<title>%1</title>\n").arg(binName) +
		"\t<instances>\n";
	writeCommonBinAux(header, QFile::WriteOnly, doIt, filename);
}

void PaletteModel::writeCommonBinsFooter() {
	writeCommonBinsFooterAux(CreateAllPartsBinFile, AllPartsBinFilePath);
	writeCommonBinsFooterAux(CreateNonCorePartsBinFile, NonCorePartsBinFilePath);
	writeCommonBinsFooterAux(CreateContribPartsBinFile, ContribPartsBinFilePath);
	writeCommonBinsFooterAux(CreateTempPartsBinFile, TempPartsBinFilePath);
}

void PaletteModel::writeCommonBinsFooterAux(bool doIt, const QString &filename) {
	QString footer = "\t</instances>\n</module>\n";
	writeCommonBinAux(footer, QFile::Append, doIt, filename);
}

void PaletteModel::writeCommonBinInstance(const QString &moduleID, const QString &path, bool doIt, const QString &filename) {
	if (!doIt) return;
	
	QString pathAux = path;
	pathAux.remove(FolderUtils::getApplicationSubFolderPath("")+"/");

	if (JustAppendAllPartsInstances) {
		QString instance = InstanceTemplate.arg(moduleID).arg("");
		writeCommonBinAux(instance, QFile::Append, doIt, filename);
	}
	else {
		QString errorStr;
		int errorLine;
		int errorColumn;
		QDomDocument domDocument;

		QFile file(filename);
		if (!domDocument.setContent(&file, true, &errorStr, &errorLine, &errorColumn)) {
			return;
		}

		QDomElement root = domDocument.documentElement();
   		if (root.isNull()) {
   			return;
		}

		if (root.tagName() != "module") {
			return;
		}

		QDomElement instances = root.firstChildElement("instances");
		if (instances.isNull()) return;

		QDomElement instance = domDocument.createElement("instance");
		instances.appendChild(instance);
		instance.setAttribute("moduleIdRef", moduleID);
		instance.setAttribute("path", "");
		QDomElement views = domDocument.createElement("views");
		instance.appendChild(views);
		QDomElement iconView = domDocument.createElement("iconView");
		views.appendChild(iconView);
		iconView.setAttribute("layer", "icon");
		QDomElement geometry = domDocument.createElement("geometry");
		iconView.appendChild(geometry);
		geometry.setAttribute("x", "-1");
		geometry.setAttribute("y", "-1");
		geometry.setAttribute("z", "-1");
		writeCommonBinAux(domDocument.toString(), QFile::WriteOnly, doIt, filename);
	}
}

void PaletteModel::writeCommonBinAux(const QString &textToWrite, QIODevice::OpenMode openMode, bool doIt, const QString &filename) {
	if(!doIt) return;

	if (FirstTimeWrite) {
		FirstTimeWrite = false;
		QFileInfo info(filename);
		QDir dir = info.absoluteDir();
		dir.mkpath(info.absolutePath());
	}

	QFile file(filename);
	if (file.open(openMode | QFile::Text)) {
		QTextStream out(&file);
		out.setCodec("UTF-8");
		out << textToWrite;
		file.close();
	}
}

void PaletteModel::countParts(QDir & dir, QStringList & nameFilters, int & partCount) {
    QStringList list = dir.entryList(nameFilters, QDir::Files | QDir::NoSymLinks);
	partCount += list.size();

    QStringList dirs = dir.entryList(QDir::AllDirs | QDir::NoSymLinks | QDir::NoDotAndDotDot);
    for (int i = 0; i < dirs.size(); ++i) {
    	QString temp2 = dirs[i];
       	dir.cd(temp2);

    	countParts(dir, nameFilters, partCount);
    	dir.cdUp();
    }
}

void PaletteModel::loadPartsAux(QDir & dir, QStringList & nameFilters, int & loadingPart, int totalPartCount, bool fastLoad) {
    QString temp = dir.absolutePath();
    QFileInfoList list = dir.entryInfoList(nameFilters, QDir::Files | QDir::NoSymLinks);
    for (int i = 0; i < list.size(); ++i) {
        QFileInfo fileInfo = list.at(i);
        QString path = fileInfo.absoluteFilePath ();
        //DebugDialog::debug(QString("part path:%1 core? %2").arg(path).arg(m_loadingCore? "true" : "false"));
        loadPart(path, false, fastLoad);
		emit loadedPart(++loadingPart, totalPartCount);
        //DebugDialog::debug("loadedok");
    }

    QStringList dirs = dir.entryList(QDir::AllDirs | QDir::NoSymLinks | QDir::NoDotAndDotDot);
    for (int i = 0; i < dirs.size(); ++i) {
    	QString temp2 = dirs[i];
       	dir.cd(temp2);

       	//if(temp2 == "core" || temp2=="contrib" || temp2=="user") {
			m_loadingCore = temp2=="core";
			m_loadingContrib = temp2=="contrib";
       	//}

    	loadPartsAux(dir, nameFilters, loadingPart, totalPartCount, fastLoad);
    	dir.cdUp();
    }
}

ModelPart * PaletteModel::loadPart(const QString & path, bool update, bool fastLoad) {
    QFile file(path);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(NULL, QObject::tr("Fritzing"),
                             QObject::tr("Cannot read file %1:\n%2.")
                             .arg(path)
                             .arg(file.errorString()));
        return NULL;
    }

	//DebugDialog::debug(QString("loading %2 %1").arg(path).arg(QTime::currentTime().toString("HH:mm:ss.zzz")));

	ModelPart::ItemType type = ModelPart::Part;
	QString moduleID;
	QString propertiesText;
	QDomDocument* domDocument = NULL;
	QString title, label, date, author, description, taxonomy, replacedby, version, url;
	QStringList tags;
	QStringList displayKeys;
	QHash<QString, QString> properties;
	QMultiHash<ViewIdentifierClass::ViewIdentifier, ViewLayer::ViewLayerID> hasViewFor;
	QHash<ViewIdentifierClass::ViewIdentifier, QString> hasBaseNameFor;

	if (fastLoad) {
		QXmlStreamReader xml(&file);
		xml.setNamespaceProcessing(false);
		ViewIdentifierClass::ViewIdentifier viewIdentifier = ViewIdentifierClass::IconView;
		while (!xml.atEnd()) {
			bool done = false;
			switch (xml.readNext()) {
				case QXmlStreamReader::StartElement:
				{
					QString name = xml.name().toString();
					if (name.compare("module") == 0) {
						moduleID = xml.attributes().value("moduleId").toString();
					}
					else if (name.compare("title") == 0) {
						title = xml.readElementText();
					}
					else if (name.compare("tag") == 0) {
						QString tag = xml.readElementText();
						tags.append(tag);
					}
					else if (name.compare("property") == 0) {
						QString name = xml.attributes().value("name").toString().toLower().trimmed();
						QString showInLabel = xml.attributes().value("showInLabel").toString();
						QString value = xml.readElementText();
						if (value.isNull()) {
							value = "";
						}
						if (!showInLabel.isEmpty()) displayKeys.append(name);
						properties.insert(name, value);
						propertiesText += (name + value);
					}
					else if (name.compare("label") == 0) {
						label = xml.readElementText();
					}
					else if (name.compare("author") == 0) {
						author = xml.readElementText();
					}
					else if (name.compare("version") == 0) {
						replacedby = xml.attributes().value("replacedby").toString();
						version = xml.readElementText();
					}
					else if (name.compare("description") == 0) {
						description = xml.readElementText();
					}
					else if (name.compare("url") == 0) {
						url = xml.readElementText();
					}
					else if (name.compare("taxonomy") == 0) {
						taxonomy = xml.readElementText();
					}
					else if (name.compare("date") == 0) {
						date = xml.readElementText();
					}
					else if (name.compare("breadboardView") == 0) {
						viewIdentifier = ViewIdentifierClass::BreadboardView;
					}
					else if (name.compare("schematicView") == 0) {
						viewIdentifier = ViewIdentifierClass::SchematicView;
					}
					else if (name.compare("pcbView") == 0) {
						viewIdentifier = ViewIdentifierClass::PCBView;
					}
					else if (name.compare("iconView") == 0) {
						viewIdentifier = ViewIdentifierClass::IconView;
					}
					else if (name.compare("layers") == 0) {
						hasBaseNameFor.insert(viewIdentifier, xml.attributes().value("image").toString());
					}
					else if (name.compare("layer") == 0) {
						QString layerName = xml.attributes().value("layerId").toString();
						ViewLayer::ViewLayerID viewLayerID = ViewLayer::viewLayerIDFromXmlString(layerName);
						if (ViewIdentifierClass::viewHasLayer(viewIdentifier, viewLayerID)) {
							hasViewFor.insert(viewIdentifier, viewLayerID);
						}
						//else {
							//DebugDialog::debug(QString("missing view layer %3: vid:%1 %4 vlid:%2").arg(viewIdentifier).arg(viewLayerID).arg(moduleID).arg(layerName));
						//}
					}
					else if (name.compare("connectors") == 0) {
						done = true;
					}
				}
                break;
            default:
                break;
			}
			if (done) break;
		}

	}
	else {
		QString errorStr;
		int errorLine;
		int errorColumn;
		domDocument = new QDomDocument();

		if (!domDocument->setContent(&file, true, &errorStr, &errorLine, &errorColumn)) {
			QMessageBox::information(NULL, QObject::tr("Fritzing"),
								 QObject::tr("Parse error (2) at line %1, column %2:\n%3\n%4")
								 .arg(errorLine)
								 .arg(errorColumn)
								 .arg(errorStr)
								 .arg(path));
			delete domDocument;
			return NULL;
		}

		QDomElement root = domDocument->documentElement();
   		if (root.isNull()) {
			//QMessageBox::information(NULL, QObject::tr("Fritzing"), QObject::tr("The file is not a Fritzing file (8)."));
   			return NULL;
		}

		if (root.tagName() != "module") {
			//QMessageBox::information(NULL, QObject::tr("Fritzing"), QObject::tr("The file is not a Fritzing file (9)."));
			return NULL;
		}

		moduleID = root.attribute("moduleId");
		if (moduleID.isNull() || moduleID.isEmpty()) {
			//QMessageBox::information(NULL, QObject::tr("Fritzing"), QObject::tr("The file is not a Fritzing file (10)."));
			return NULL;
		}

		// check if it's a wire
		QDomElement properties = root.firstChildElement("properties");
		propertiesText = properties.text();

		QDomElement t = root.firstChildElement("title");
		TextUtils::findText(t, title);
	}

	// FIXME: properties is nested right now
	if (moduleID.compare(ModuleIDNames::WireModuleIDName) == 0) {
		type = ModelPart::Wire;
	}
	else if (moduleID.compare(ModuleIDNames::JumperModuleIDName) == 0) {
		type = ModelPart::Jumper;
	}
	else if (moduleID.endsWith(ModuleIDNames::LogoImageModuleIDName)) {
		type = ModelPart::Logo;
	}
	else if (moduleID.endsWith(ModuleIDNames::LogoTextModuleIDName)) {
		type = ModelPart::Logo;
	}
	else if (moduleID.compare(ModuleIDNames::GroundPlaneModuleIDName) == 0) {
		type = ModelPart::CopperFill;
	}
	else if (moduleID.compare(ModuleIDNames::NoteModuleIDName) == 0) {
		type = ModelPart::Note;
	}
	else if (moduleID.compare(ModuleIDNames::JustPowerModuleIDName) == 0) {
		type = ModelPart::Symbol;
	}
	else if (moduleID.compare(ModuleIDNames::PowerModuleIDName) == 0) {
		type = ModelPart::Symbol;
	}
	else if (moduleID.compare(ModuleIDNames::GroundModuleIDName) == 0) {
		type = ModelPart::Symbol;
	}
	else if (moduleID.compare(ModuleIDNames::RulerModuleIDName) == 0) {
		type = ModelPart::Ruler;
	}
	else if (moduleID.compare(ModuleIDNames::ViaModuleIDName) == 0) {
		type = ModelPart::Via;
	}
	else if (moduleID.compare(ModuleIDNames::HoleModuleIDName) == 0) {
		type = ModelPart::Hole;
	}
	else if (moduleID.endsWith(ModuleIDNames::PerfboardModuleIDName)) {
		type = ModelPart::Breadboard;
	}
	else if (moduleID.endsWith(ModuleIDNames::StripboardModuleIDName)) {
		type = ModelPart::Breadboard;
	}
	else if (propertiesText.contains("breadboard", Qt::CaseInsensitive)) {
		type = ModelPart::Breadboard;
	}
	else if (propertiesText.contains("plain vanilla pcb", Qt::CaseInsensitive)) {
		if (propertiesText.contains("shield", Qt::CaseInsensitive) || title.contains("custom", Qt::CaseInsensitive)) {
			type = ModelPart::Board;
		}
		else {
			type = ModelPart::ResizableBoard;
		}
	}

	ModelPart * modelPart = new ModelPart(domDocument, path, type);
	if (modelPart == NULL) return NULL;

	modelPart->setCore(m_loadingCore);
	modelPart->setContrib(m_loadingContrib);

	if (m_partHash.value(moduleID, NULL)) {
		if(!update) {
			QMessageBox::warning(NULL, QObject::tr("Fritzing"),
							 QObject::tr("The part '%1' at '%2' does not have a unique module id '%3'.")
							 .arg(modelPart->title())
							 .arg(path)
							 .arg(moduleID));
			return NULL;
		} else {
			m_partHash[moduleID]->copyStuff(modelPart);
		}
	} else {
		m_partHash.insert(moduleID, modelPart);
	}

    if (m_root == NULL) {
		 m_root = modelPart;
	}
	else {
    	modelPart->setParent(m_root);
   	}

	if (fastLoad) {
		ModelPartShared * modelPartShared = modelPart->modelPartShared();
		modelPartShared->setModuleID(moduleID);
		modelPartShared->setPartlyLoaded(true);
		modelPartShared->setTitle(title);
		modelPartShared->setDate(date);
		modelPartShared->setAuthor(author);
		//if (label.isEmpty()) {
			//DebugDialog::debug(QString("empty label %1").arg(path));
		//}
		modelPartShared->setLabel(label);
		modelPartShared->setDescription(description);
		modelPartShared->setUrl(url);
		modelPartShared->setTaxonomy(taxonomy);
		modelPartShared->setVersion(version);
		modelPartShared->setReplacedby(replacedby);
		modelPartShared->setTags(tags);
		modelPartShared->setProperties(properties);
		modelPartShared->setDisplayKeys(displayKeys);
		foreach (ViewIdentifierClass::ViewIdentifier viewIdentifier, hasViewFor.uniqueKeys()) {
			foreach (ViewLayer::ViewLayerID viewLayerID, hasViewFor.values(viewIdentifier)) {
				modelPartShared->setHasViewFor(viewIdentifier, viewLayerID);
			}
		}
		foreach (ViewIdentifierClass::ViewIdentifier viewIdentifier, hasBaseNameFor.keys()) {
			modelPartShared->setHasBaseNameFor(viewIdentifier, hasBaseNameFor.value(viewIdentifier));
		}
	}
	else {
		modelPart->modelPartShared()->flipSMDAnd();
	}

	if (FirstTime) {
		// make sure saving to the common bin takes place after fastLoad
		//DebugDialog::debug(QString("all parts %1").arg(JustAppendAllPartsInstances));
		writeCommonBinInstance(moduleID,path,CreateAllPartsBinFile,AllPartsBinFilePath);

		if (modelPart->isContrib()) {
			//if (path.startsWith(FritzingContribPath, Qt::CaseInsensitive)) {
				writeCommonBinInstance(moduleID,path,CreateContribPartsBinFile,ContribPartsBinFilePath);
			//}
		}

		//DebugDialog::debug(QString("non core parts %1").arg(JustAppendAllPartsInstances));
		if (!modelPart->isCore() && !modelPart->isObsolete()) {
			writeCommonBinInstance(moduleID,path,CreateNonCorePartsBinFile,NonCorePartsBinFilePath);
		}
	}

    return modelPart;
}


bool PaletteModel::load(const QString & fileName, ModelBase * refModel) {
	QList<ModelPart *> modelParts;
	bool result = ModelBase::load(fileName, refModel, modelParts);
	if (result) {
		m_loadedFromFile = true;
		m_loadedFrom = fileName;
	}
	return result;
}

bool PaletteModel::loadedFromFile() {
	return m_loadedFromFile;
}

QString PaletteModel::loadedFrom() {
	if(m_loadedFromFile) {
		return m_loadedFrom;
	} else {
		return ___emptyString___;
	}
}

ModelPart * PaletteModel::addPart(QString newPartPath, bool addToReference, bool updateIdAlreadyExists) {
	/*ModelPart * modelPart = loadPart(newPartPath, updateIdAlreadyExists);;
	if (m_referenceModel != NULL && addToReference) {
		modelPart = m_referenceModel->addPart(newPartPath, addToReference);
		if (modelPart != NULL) {
			return addModelPart( m_root, modelPart);
		}
	}*/

	ModelPart * modelPart = loadPart(newPartPath, updateIdAlreadyExists, false);
	if (m_referenceModel && addToReference) {
		m_referenceModel->addPart(modelPart,updateIdAlreadyExists);
	}

	return modelPart;
}

void PaletteModel::removePart(const QString &moduleID) {
	ModelPart *mpToRemove = NULL;
	QList<QObject *>::const_iterator i;
    for (i = m_root->children().constBegin(); i != m_root->children().constEnd(); ++i) {
		ModelPart* mp = qobject_cast<ModelPart *>(*i);
		if (mp == NULL) continue;

		if(mp->moduleID() == moduleID) {
			mpToRemove = mp;
			break;
		}
	}
	if(mpToRemove) {
		mpToRemove->setParent(NULL);
		delete mpToRemove;
	}
}

void PaletteModel::removeParts() {
    QList<ModelPart *> modelParts;
    foreach (QObject * child, m_root->children()) {
        ModelPart * modelPart = qobject_cast<ModelPart *>(child);
        if (modelPart == NULL) continue;

        modelParts.append(modelPart);
    }

    foreach(ModelPart * modelPart, modelParts) {
        modelPart->setParent(NULL);
        delete modelPart;
    }
}

void PaletteModel::clearPartHash() {
	foreach (ModelPart * modelPart, m_partHash.values()) {
		delete modelPart;
	}
	m_partHash.clear();
}

void PaletteModel::setOrdererChildren(QList<QObject*> children) {
	m_root->setOrderedChildren(children);
}

QList<ModelPart *> PaletteModel::search(const QString & searchText, bool allowObsolete) {
	QList<ModelPart *> modelParts;

	QStringList strings = searchText.split(":");
	if (strings.count() > 2) return modelParts;

    search(m_root, strings, modelParts, allowObsolete);
    return modelParts;
}

void PaletteModel::search(ModelPart * modelPart, const QStringList & searchStrings, QList<ModelPart *> & modelParts, bool allowObsolete) {
	// TODO: eventually move all this into the database?
	// or use lucene
	// or google search api


    ModelPart * candidate = NULL;
    if (!candidate && modelPart->title().contains(searchStrings[0], Qt::CaseInsensitive)) {
        candidate = modelPart;
	}
    if (!candidate && modelPart->description().contains(searchStrings[0], Qt::CaseInsensitive)) {
        candidate = modelPart;
	}
    if (!candidate && modelPart->url().contains(searchStrings[0], Qt::CaseInsensitive)) {
        candidate = modelPart;
	}
    if (!candidate && modelPart->author().contains(searchStrings[0], Qt::CaseInsensitive)) {
        candidate = modelPart;
	}
    if (!candidate) {
		foreach (QString string, modelPart->tags()) {
			if (string.contains(searchStrings[0], Qt::CaseInsensitive)) {
                candidate = modelPart;
				break;
			}
		}
	}
    if (!candidate) {
		foreach (QString string, modelPart->properties().values()) {
			if (string.contains(searchStrings[0], Qt::CaseInsensitive)) {
                candidate = modelPart;
				break;
			}
		}
	}
    if (!candidate) {
		foreach (QString string, modelPart->properties().keys()) {
			if (string.contains(searchStrings[0], Qt::CaseInsensitive)) {
                candidate = modelPart;
				break;
			}
		}
	}

    if (candidate && !modelParts.contains(candidate)) {
        if (!allowObsolete && candidate->isObsolete()) {
        }
        else
        {
            modelParts.append(candidate);
        }
    }

	foreach(QObject * child, modelPart->children()) {
		ModelPart * mp = qobject_cast<ModelPart *>(child);
		if (mp == NULL) continue;

        search(mp, searchStrings, modelParts, allowObsolete);
	}
}

