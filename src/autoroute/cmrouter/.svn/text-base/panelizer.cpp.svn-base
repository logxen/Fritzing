/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2009 Fachhochschule Potsdam - http://fh-potsdam.de

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

$Revision: 2979 $:
$Author$:
$Date$

********************************************************************/

#include "panelizer.h"
#include "../../debugdialog.h"
#include "../../sketch/pcbsketchwidget.h"
#include "../../utils/textutils.h"
#include "../../utils/graphicsutils.h"
#include "../../utils/folderutils.h"
#include "../../utils/folderutils.h"
#include "../../items/resizableboard.h"
#include "../../items/logoitem.h"
#include "../../items/groundplane.h"
#include "../../fsvgrenderer.h"
#include "../../fapplication.h"
#include "../../svg/gerbergenerator.h"
#include "../../autoroute/cmrouter/cmrouter.h"
#include "../../referencemodel/referencemodel.h"
#include "../../version/version.h"

#include "tileutils.h"

#include <QFile>
#include <QDomDocument>
#include <QDomElement>
#include <QDir>
#include <qmath.h>
#include <limits>

bool areaGreaterThan(PanelItem * p1, PanelItem * p2)
{
	return p1->boardSizeInches.width() * p1->boardSizeInches.height() > p2->boardSizeInches.width() * p2->boardSizeInches.height();
}

bool rotateGreaterThan(PanelItem * p1, PanelItem * p2)
{
	Q_UNUSED(p2);
	if (!p1->rotate90) return true;

	return false;
}

int allSpaces(Tile * tile, UserData userData) {
	QList<Tile*> * tiles = (QList<Tile*> *) userData;
	if (TiGetType(tile) == Tile::SPACE) {
		tiles->append(tile);
		return 0;
	}

	tiles->clear();
	return 1;			// stop the search
}

int allObstacles(Tile * tile, UserData userData) {
	if (TiGetType(tile) == Tile::OBSTACLE) {
		QList<Tile*> * obstacles = (QList<Tile*> *) userData;
		obstacles->append(tile);

	}

	return 0;
}

static int PlanePairIndex = 0;

static double Worst = std::numeric_limits<double>::max() / 4;

struct LayerThing {
    LayerList layerList;
    QString name;
    SVG2gerber::ForWhy forWhy;
    QString suffix;

    LayerThing(const QString & n, LayerList ll, SVG2gerber::ForWhy fw, const QString & s) {
            layerList = ll;
            name = n;
            forWhy = fw;
            suffix = s;
    }

};

int roomOn(Tile * tile, TileRect & tileRect, BestPlace * bestPlace)
{
	int w = tileRect.xmaxi - tileRect.xmini;
	int h = tileRect.ymaxi - tileRect.ymini;
	if (bestPlace->width <= w && bestPlace->height <= h) {
		bestPlace->bestTile = tile;
		return 1;
	}

	TileRect temp;
	temp.xmini = tileRect.xmini;
	temp.xmaxi = temp.xmini + bestPlace->width;
	temp.ymini = tileRect.ymini;
	temp.ymaxi = temp.ymini + bestPlace->height;
	QList<Tile*> spaces;
	TiSrArea(tile, bestPlace->plane, &temp, allSpaces, &spaces);
	if (spaces.count()) {
		bestPlace->bestTile = tile;
		return 1;
	}

	return 0;
}

int roomAnywhere(Tile * tile, UserData userData) 
{
	if (TiGetType(tile) != Tile::SPACE) return 0;

	BestPlace * bestPlace = (BestPlace *) userData;
	TileRect tileRect;
	TiToRect(tile, &tileRect);

	return roomOn(tile, tileRect, bestPlace);
}

int roomOnTop(Tile * tile, UserData userData) 
{
	if (TiGetType(tile) != Tile::SPACE) return 0;

	BestPlace * bestPlace = (BestPlace *) userData;
	TileRect tileRect;
	TiToRect(tile, &tileRect);

	if (tileRect.ymini != bestPlace->maxRect.ymini) return 0;

	return roomOn(tile, tileRect, bestPlace);
}

int roomOnBottom(Tile * tile, UserData userData) 
{
	if (TiGetType(tile) != Tile::SPACE) return 0;

	BestPlace * bestPlace = (BestPlace *) userData;
	TileRect tileRect;
	TiToRect(tile, &tileRect);

	if (tileRect.ymaxi != bestPlace->maxRect.ymaxi) return 0;

	return roomOn(tile, tileRect, bestPlace);
}

int roomOnLeft(Tile * tile, UserData userData) 
{
	if (TiGetType(tile) != Tile::SPACE) return 0;

	BestPlace * bestPlace = (BestPlace *) userData;
	TileRect tileRect;
	TiToRect(tile, &tileRect);

	if (tileRect.xmini != bestPlace->maxRect.xmini) return 0;

	return roomOn(tile, tileRect, bestPlace);
}

int roomOnRight(Tile * tile, UserData userData) 
{
	if (TiGetType(tile) != Tile::SPACE) return 0;

	BestPlace * bestPlace = (BestPlace *) userData;
	TileRect tileRect;
	TiToRect(tile, &tileRect);

	if (tileRect.xmaxi != bestPlace->maxRect.xmaxi) return 0;

	return roomOn(tile, tileRect, bestPlace);
}

/////////////////////////////////////////////////////////////////////////////////

void Panelizer::panelize(FApplication * app, const QString & panelFilename) 
{
	QFile file(panelFilename);

	QString errorStr;
	int errorLine;
	int errorColumn;

	DebugDialog::setEnabled(true);

	QDomDocument domDocument;
	if (!domDocument.setContent(&file, true, &errorStr, &errorLine, &errorColumn)) {
		DebugDialog::debug(QString("Unable to parse '%1': '%2' line:%3 column:%4").arg(panelFilename).arg(errorStr).arg(errorLine).arg(errorColumn));
		return;
	}

	QDomElement root = domDocument.documentElement();
	if (root.isNull() || root.tagName() != "panelizer") {
		DebugDialog::debug(QString("root element is not 'panelizer'"));
		return;
	}

	PanelParams panelParams;
	if (!initPanelParams(root, panelParams)) return;

	QDir outputDir(panelParams.outputFolder);
	outputDir.mkdir("svg");
	outputDir.mkdir("gerber");
	outputDir.mkdir("fz");

	QDir svgDir(outputDir);
	svgDir.cd("svg");
	if (!svgDir.exists()) {
		DebugDialog::debug(QString("unable to create svg folder in '%1'").arg(panelParams.outputFolder));
		return;
	}

	DebugDialog::debug(QString("svg folder '%1'\n").arg(svgDir.absolutePath()));

	QDir gerberDir(outputDir);
	gerberDir.cd("gerber");
	if (!gerberDir.exists()) {
		DebugDialog::debug(QString("unable to create gerber folder in '%1'").arg(panelParams.outputFolder));
		return;
	}

	DebugDialog::debug(QString("gerber folder '%1'\n").arg(gerberDir.absolutePath()));

	QDir fzDir(outputDir);
	fzDir.cd("fz");
	if (!fzDir.exists()) {
		DebugDialog::debug(QString("unable to create fz folder in '%1'").arg(panelParams.outputFolder));
		return;
	}

	DebugDialog::debug(QString("fz folder '%1'\n").arg(fzDir.absolutePath()));


	QDomElement boards = root.firstChildElement("boards");
	QDomElement board = boards.firstChildElement("board");
	if (board.isNull()) {
		DebugDialog::debug(QString("no <board> elements found"));
		return;
	}

	QHash<QString, QString> fzzFilePaths;
	QDomElement paths = root.firstChildElement("paths");
	QDomElement path = paths.firstChildElement("path");
	if (path.isNull()) {
		DebugDialog::debug(QString("no <path> elements found"));
		return;
	}

	collectFiles(path, fzzFilePaths);
	if (fzzFilePaths.count() == 0) {
		DebugDialog::debug(QString("no fzz files found in paths"));
		return;
	}
	
	board = boards.firstChildElement("board");
	if (!checkBoards(board, fzzFilePaths)) return;

	app->createUserDataStoreFolderStructure();
	app->registerFonts();
	app->loadReferenceModel();

	if (!app->loadBin("")) {
		DebugDialog::debug(QString("load bin failed"));
		return;
	}

	QHash<QString, PanelItem *> refPanelItems;
	board = boards.firstChildElement("board");
	if (!openWindows(board, fzzFilePaths, app, panelParams, fzDir, refPanelItems)) return;

	QList<PanelItem *> insertPanelItems;
	int optionalCount = 0;
	foreach (PanelItem * panelItem, refPanelItems.values()) {
		for (int i = 0; i < panelItem->required; i++) {
			PanelItem * copy = new PanelItem(panelItem);
			insertPanelItems.append(copy);
		}
		optionalCount += panelItem->maxOptional;
	}

	QList<PlanePair *> planePairs;
	planePairs << makePlanePair(panelParams);

	qSort(insertPanelItems.begin(), insertPanelItems.end(), areaGreaterThan);
	bestFit(insertPanelItems, panelParams, planePairs);

	addOptional(optionalCount, refPanelItems, insertPanelItems, panelParams, planePairs);

	foreach (PlanePair * planePair, planePairs) {
		planePair->layoutSVG += "</svg>";
		QString fname = svgDir.absoluteFilePath(QString("%1.panel_%2.layout.svg").arg(panelParams.prefix).arg(planePair->index));
		QFile outfile(fname);
		if (outfile.open(QIODevice::WriteOnly | QIODevice::Text)) {
			QTextStream out(&outfile);
			out.setCodec("UTF-8");
			out << planePair->layoutSVG;
			outfile.close();
		}
	}

	// so we only have to rotate once
	qSort(insertPanelItems.begin(), insertPanelItems.end(), rotateGreaterThan);

	QList<LayerThing> layerThingList;
	layerThingList.append(LayerThing("outline", ViewLayer::outlineLayers(), SVG2gerber::ForOutline, GerberGenerator::OutlineSuffix));  
	layerThingList.append(LayerThing("copper_top", ViewLayer::copperLayers(ViewLayer::Top), SVG2gerber::ForCopper, GerberGenerator::CopperTopSuffix));
	layerThingList.append(LayerThing("copper_bottom", ViewLayer::copperLayers(ViewLayer::Bottom), SVG2gerber::ForCopper, GerberGenerator::CopperBottomSuffix));
	layerThingList.append(LayerThing("mask_top", ViewLayer::maskLayers(ViewLayer::Top), SVG2gerber::ForMask, GerberGenerator:: MaskTopSuffix));
	layerThingList.append(LayerThing("mask_bottom", ViewLayer::maskLayers(ViewLayer::Bottom), SVG2gerber::ForMask, GerberGenerator::MaskBottomSuffix));
	layerThingList.append(LayerThing("silk_top", ViewLayer::silkLayers(ViewLayer::Top), SVG2gerber::ForSilk, GerberGenerator::SilkTopSuffix));
	layerThingList.append(LayerThing("silk_bottom", ViewLayer::silkLayers(ViewLayer::Bottom), SVG2gerber::ForSilk, GerberGenerator::SilkBottomSuffix));
	layerThingList.append(LayerThing("drill", ViewLayer::drillLayers(), SVG2gerber::ForDrill, GerberGenerator::DrillSuffix));
	  
	QHash<QString, bool> rotated;
	foreach(PanelItem * panelItem, refPanelItems.values()) {
		rotated.insert(panelItem->path, false);
	}

	foreach (PlanePair * planePair, planePairs) {
		for (int i = 0; i < layerThingList.count(); i++) {
			planePair->svgs << TextUtils::makeSVGHeader(1, GraphicsUtils::StandardFritzingDPI, panelParams.panelWidth, panelParams.panelHeight);
		}

		foreach (PanelItem * panelItem, insertPanelItems) {
			if (panelItem->planePair != planePair) continue;

			DebugDialog::debug(QString("placing %1 on panel %2").arg(panelItem->boardName).arg(planePair->index));

			try {
				if ((panelItem->rotate90 && !rotated.value(panelItem->path)) || (panelItem->rotate90 == false && rotated.value(panelItem->path)))
				{
					// try to minimize rotations by keeping state
					rotated.insert(panelItem->path, !rotated.value(panelItem->path));
					panelItem->window->pcbView()->selectAllItems(true, false);
					DebugDialog::debug(QString("rotating 90:%1 %2").arg(panelItem->path).arg((long) panelItem, 0, 16));
					QMatrix matrix = panelItem->board->matrix();
					DebugDialog::debug(QString("\tmatrix m11:%1 m12:%2 m21:%3 m22:%4 dx:%5 dy:%6").arg(matrix.m11()).arg(matrix.m12()).arg(matrix.m21()).arg(matrix.m22()).arg(matrix.dx()).arg(matrix.dy()));
					panelItem->window->pcbView()->rotateX(90, false);
					matrix = panelItem->board->matrix();
					DebugDialog::debug(QString("\tmatrix m11:%1 m12:%2 m21:%3 m22:%4 dx:%5 dy:%6").arg(matrix.m11()).arg(matrix.m12()).arg(matrix.m21()).arg(matrix.m22()).arg(matrix.dx()).arg(matrix.dy()));
				}

				QSizeF imageSize;
				bool empty;

				QString maskTop;
				QString maskBottom;

				for (int i = 0; i < planePair->svgs.count(); i++) {					
					SVG2gerber::ForWhy forWhy = layerThingList.at(i).forWhy;
					QString name = layerThingList.at(i).name;
					QList<ItemBase *> copperLogoItems;
					if (forWhy == SVG2gerber::ForMask) {
						panelItem->window->pcbView()->hideCopperLogoItems(copperLogoItems);
					}
					QString one = panelItem->window->pcbView()->renderToSVG(FSvgRenderer::printerScale(), layerThingList.at(i).layerList, true, imageSize, panelItem->board, GraphicsUtils::StandardFritzingDPI, false, false, false, empty);
					
					QString clipString;
					
					switch (forWhy) {
						case SVG2gerber::ForOutline:
							one = GerberGenerator::cleanOutline(one);
							break;
						case SVG2gerber::ForMask:
							panelItem->window->pcbView()->restoreCopperLogoItems(copperLogoItems);
							one = TextUtils::expandAndFill(one, "black", GerberGenerator::MaskClearanceMils * 2);
							forWhy = SVG2gerber::ForCopper;
							if (name.contains("bottom")) {
								maskBottom = one;
							}
							else {
								maskTop = one;
							}
							break;
						case SVG2gerber::ForSilk:
							if (name.contains("bottom")) {
								clipString = maskBottom;
							}
							else {
								clipString = maskTop;
							}
							break;
						default:
							break;
					}
					
					one = GerberGenerator::clipToBoard(one, panelItem->board, name, forWhy, clipString);
					if (one.isEmpty()) continue;

					int left = one.indexOf("<svg");
					left = one.indexOf(">", left + 1);
					int right = one.lastIndexOf("<");
					one = QString("<g transform='translate(%1,%2)'>\n").arg(panelItem->x * GraphicsUtils::StandardFritzingDPI).arg(panelItem->y * GraphicsUtils::StandardFritzingDPI) + 
									one.mid(left + 1, right - left - 1) + 
									"</g>\n";

					planePair->svgs.replace(i, planePair->svgs.at(i) + one);
				}
			}
			catch (const char * msg) {
				DebugDialog::debug(QString("panelizer error 1 %1 %2").arg(panelItem->boardName).arg(msg));
			}
			catch (const QString & msg) {
				DebugDialog::debug(QString("panelizer error 2 %1 %2").arg(panelItem->boardName).arg(msg));
			}
			catch (...) {
				DebugDialog::debug(QString("panelizer error 3 %1").arg(panelItem->boardName));
			}

		}

		DebugDialog::debug("after placement");
		QString prefix = QString("%1.panel_%2").arg(panelParams.prefix).arg(planePair->index);
		for (int i = 0; i < planePair->svgs.count(); i++) {
			if (planePair->svgs.at(i).isEmpty()) continue;

			planePair->svgs.replace(i, planePair->svgs.at(i) + "</svg>");

			QString fname = svgDir.absoluteFilePath(QString("%1.%2.svg").arg(prefix).arg(layerThingList.at(i).name));
			QFile outfile(fname);
			if (outfile.open(QIODevice::WriteOnly | QIODevice::Text)) {
				QTextStream out(&outfile);
				out.setCodec("UTF-8");
				out << planePair->svgs.at(i);
				outfile.close();
			}

			QString suffix = layerThingList.at(i).suffix;
			DebugDialog::debug("converting " + prefix + " " + suffix);
			QSizeF svgSize(panelParams.panelWidth, panelParams.panelHeight);
			SVG2gerber::ForWhy forWhy = layerThingList.at(i).forWhy;
			if (forWhy == SVG2gerber::ForMask) forWhy = SVG2gerber::ForCopper;
			GerberGenerator::doEnd(planePair->svgs.at(i), 2, layerThingList.at(i).name, forWhy, svgSize, gerberDir.absolutePath(), prefix, suffix, false, false);
			DebugDialog::debug("after converting " + prefix + " " + suffix);
		}

		QDomDocument doc;
		QString merger = planePair->svgs.at(0);  // outline layer
		merger.replace("black", "#90f0a0");
		merger.replace("#000000", "#90f0a0");
		merger.replace("fill-opacity=\"0.5\"", "fill-opacity=\"1\"");
		TextUtils::mergeSvg(doc, merger, "");
		merger = planePair->svgs.at(5);			// silktop layer
		merger.replace("black", "#909090");
		merger.replace("#000000", "#909090");
		TextUtils::mergeSvg(doc, merger, "");		
		merger = planePair->layoutSVG;				// layout
		merger.replace("'red'", "'none'");		// hide background rect
		TextUtils::mergeSvg(doc, merger, "");
		merger = TextUtils::mergeSvgFinish(doc);
		QString fname = svgDir.absoluteFilePath(QString("%1.%2.svg").arg(prefix).arg("identification"));
		QFile outfile(fname);
		if (outfile.open(QIODevice::WriteOnly | QIODevice::Text)) {
			QTextStream out(&outfile);
			out.setCodec("UTF-8");
			out << merger;
			outfile.close();
		}
	}

	foreach (PanelItem * panelItem, refPanelItems.values()) {
		panelItem->window->close();
	}
}

void Panelizer::bestFit(QList<PanelItem *> & insertPanelItems, PanelParams & panelParams, QList<PlanePair *> & planePairs)
{
	foreach (PanelItem * panelItem, insertPanelItems) {
		bestFitOne(panelItem, panelParams, planePairs, true);
	}
}

bool Panelizer::bestFitOne(PanelItem * panelItem, PanelParams & panelParams, QList<PlanePair *> & planePairs, bool createNew)
{
	//DebugDialog::debug(QString("panel %1").arg(panelItem->boardName));
	BestPlace bestPlace1, bestPlace2;
	bestPlace1.bestTile = bestPlace2.bestTile = NULL;
	bestPlace1.rotate90 = bestPlace2.rotate90 = false;
	bestPlace1.width = bestPlace2.width = realToTile(panelItem->boardSizeInches.width() + panelParams.panelSpacing);
	bestPlace1.height = bestPlace2.height = realToTile(panelItem->boardSizeInches.height() + panelParams.panelSpacing);
	bestPlace1.bestArea = bestPlace2.bestArea = Worst;
	int ppix = 0;
	while (ppix < planePairs.count()) {
		PlanePair *  planePair = planePairs.at(ppix);
		bestPlace1.plane = planePair->thePlane;
		bestPlace2.plane = planePair->thePlane90;
		bestPlace1.maxRect = planePair->tilePanelRect;
		TiSrArea(NULL, planePair->thePlane, &planePair->tilePanelRect, placeBestFit, &bestPlace1);
		if (bestPlace1.bestTile == NULL) {
			bestPlace2.maxRect = planePair->tilePanelRect90;
			TiSrArea(NULL, planePair->thePlane90, &planePair->tilePanelRect90, placeBestFit, &bestPlace2);
		}
		if (bestPlace1.bestTile == NULL && bestPlace2.bestTile == NULL ) {
			if (++ppix < planePairs.count()) {
				// try next panel
				continue;
			}

			if (!createNew) {
				return false;
			}

			// create next panel
			planePair = makePlanePair(panelParams);
			planePairs << planePair;
			DebugDialog::debug(QString("ran out of room placing %1").arg(panelItem->boardName));
			continue;
		}

		bool use2 = false;
		if (bestPlace1.bestTile == NULL) {
			use2 = true;
		}
		else if (bestPlace2.bestTile == NULL) {
		}
		else {
			// never actually get here
			use2 = bestPlace2.bestArea < bestPlace1.bestArea;
		}

		if (use2) {
			tileUnrotate90(bestPlace2.bestTileRect, bestPlace1.bestTileRect);
			bestPlace1.rotate90 = !bestPlace2.rotate90;
		}

		panelItem->x = tileToReal(bestPlace1.bestTileRect.xmini) ;
		panelItem->y = tileToReal(bestPlace1.bestTileRect.ymini);
		panelItem->rotate90 = bestPlace1.rotate90;

		DebugDialog::debug(QString("setting rotate90:%1 %2").arg(panelItem->rotate90).arg(panelItem->path));
		panelItem->planePair = planePair;

		TileRect tileRect;
		tileRect.xmini = bestPlace1.bestTileRect.xmini;
		tileRect.ymini = bestPlace1.bestTileRect.ymini;
		if (bestPlace1.rotate90) {
			tileRect.xmaxi = tileRect.xmini + bestPlace1.height;
			tileRect.ymaxi = tileRect.ymini + bestPlace1.width;
		}
		else {
			tileRect.ymaxi = tileRect.ymini + bestPlace1.height;
			tileRect.xmaxi = tileRect.xmini + bestPlace1.width;
		}

		double w = panelItem->boardSizeInches.width();
		double h = panelItem->boardSizeInches.height();
		if (panelItem->rotate90) {
			w = h;
			h = panelItem->boardSizeInches.width();
		}

		planePair->layoutSVG += QString("<rect x='%1' y='%2' width='%3' height='%4' stroke='none' fill='red'/>\n")
			.arg(panelItem->x * GraphicsUtils::StandardFritzingDPI)
			.arg(panelItem->y * GraphicsUtils::StandardFritzingDPI)
			.arg(GraphicsUtils::StandardFritzingDPI * w)
			.arg(GraphicsUtils::StandardFritzingDPI * h);

		QStringList strings = QFileInfo(panelItem->path).completeBaseName().split("_");
		double cx = GraphicsUtils::StandardFritzingDPI * (panelItem->x + (w / 2));
		int fontSize1 = 250;
		int fontSize2 = 150;
		int fontSize = fontSize1;
		double cy = GraphicsUtils::StandardFritzingDPI * (panelItem->y + (h  / 2));
		cy -= ((strings.count() - 1) * fontSize2 / 2);
		foreach (QString string, strings) {
			planePair->layoutSVG += QString("<text x='%1' y='%2' anchor='middle' font-family='OCRA' stroke='none' fill='#000000' text-anchor='middle' font-size='%3'>%4</text>\n")
				.arg(cx)
				.arg(cy)
				.arg(fontSize)
				.arg(string);
			cy += fontSize;
			if (fontSize == fontSize1) fontSize = fontSize2;
		}

		TiInsertTile(planePair->thePlane, &tileRect, NULL, Tile::OBSTACLE);
		TileRect tileRect90;
		tileRotate90(tileRect, tileRect90);
		TiInsertTile(planePair->thePlane90, &tileRect90, NULL, Tile::OBSTACLE);

		return true;
	}

	DebugDialog::debug("bestFitOne should never reach here");
	return false;
}

PlanePair * Panelizer::makePlanePair(PanelParams & panelParams)
{
	PlanePair * planePair = new PlanePair;

	// for debugging
	planePair->layoutSVG = TextUtils::makeSVGHeader(1, GraphicsUtils::StandardFritzingDPI, panelParams.panelWidth, panelParams.panelHeight);
	planePair->index = PlanePairIndex++;

	Tile * bufferTile = TiAlloc();
	TiSetType(bufferTile, Tile::BUFFER);
	TiSetBody(bufferTile, NULL);

	QRectF panelRect(0, 0, panelParams.panelWidth + panelParams.panelSpacing - panelParams.panelBorder, 
							panelParams.panelHeight + panelParams.panelSpacing - panelParams.panelBorder);

    SETLEFT(bufferTile, fasterRealToTile(panelRect.left() - 10));
    SETYMIN(bufferTile, fasterRealToTile(panelRect.top() - 10));		

	planePair->thePlane = TiNewPlane(bufferTile);

    SETRIGHT(bufferTile, fasterRealToTile(panelRect.right() + 10));
	SETYMAX(bufferTile, fasterRealToTile(panelRect.bottom() + 10));		

	qrectToTile(panelRect, planePair->tilePanelRect);
	TiInsertTile(planePair->thePlane, &planePair->tilePanelRect, NULL, Tile::SPACE); 

	QMatrix matrix90;
	matrix90.rotate(90);
	QRectF panelRect90 = matrix90.mapRect(panelRect);

	Tile * bufferTile90 = TiAlloc();
	TiSetType(bufferTile90, Tile::BUFFER);
	TiSetBody(bufferTile90, NULL);

    SETLEFT(bufferTile90, fasterRealToTile(panelRect90.left() - 10));
    SETYMIN(bufferTile90, fasterRealToTile(panelRect90.top() - 10));		

	planePair->thePlane90 = TiNewPlane(bufferTile90);

    SETRIGHT(bufferTile90, fasterRealToTile(panelRect.right() + 10));
	SETYMAX(bufferTile90, fasterRealToTile(panelRect.bottom() + 10));		

	qrectToTile(panelRect90, planePair->tilePanelRect90);
	TiInsertTile(planePair->thePlane90, &planePair->tilePanelRect90, NULL, Tile::SPACE); 

	return planePair;
}

void Panelizer::collectFiles(QDomElement & path, QHash<QString, QString> & fzzFilePaths)
{
	while (!path.isNull()) {
		QDomNode node = path.firstChild();
		if (!node.isText()) {
			DebugDialog::debug(QString("missing text in <path> element"));
			return;
		}

		QString p = node.nodeValue();
		QDir dir(p);
		if (!dir.exists()) {
			DebugDialog::debug(QString("Directory '%1' doesn't exist").arg(p));
			return;
		}

		QStringList filepaths;
        QStringList filters("*" + FritzingBundleExtension);
        FolderUtils::collectFiles(dir, filters, filepaths);
		foreach (QString filepath, filepaths) {
			QFileInfo fileInfo(filepath);

			fzzFilePaths.insert(fileInfo.fileName(), filepath);
		}

		path = path.nextSiblingElement("path");
	}
}

bool Panelizer::checkBoards(QDomElement & board, QHash<QString, QString> & fzzFilePaths)
{
	while (!board.isNull()) {
		QString boardname = board.attribute("name");
		//DebugDialog::debug(QString("board %1").arg(boardname));
		bool ok;
		int test = board.attribute("requiredCount", "").toInt(&ok);
		if (!ok) {
			DebugDialog::debug(QString("requiredCount for board '%1' not an integer: '%2'").arg(boardname).arg(board.attribute("requiredCount")));
			return false;
		}

		test = board.attribute("maxOptionalCount", "").toInt(&ok);
		if (!ok) {
			DebugDialog::debug(QString("maxOptionalCount for board '%1' not an integer: '%2'").arg(boardname).arg(board.attribute("maxOptionalCount")));
			return false;
		}

		QString path = fzzFilePaths.value(boardname, "");
		if (path.isEmpty()) {
			DebugDialog::debug(QString("File for board '%1' not found in search paths").arg(boardname));
			return false;
		}

		board = board.nextSiblingElement("board");
	}

	return true;
}

bool Panelizer::openWindows(QDomElement & board, QHash<QString, QString> & fzzFilePaths, FApplication * app, PanelParams & panelParams, QDir & fzDir, QHash<QString, PanelItem *> & refPanelItems)
{
	while (!board.isNull()) {
		int required = board.attribute("requiredCount", "").toInt();
		int optional = board.attribute("maxOptionalCount", "").toInt();
		if (required == 0 && optional == 0) {
			board = board.nextSiblingElement("board"); 
			continue;
		}

		QString boardName = board.attribute("name");
		QString path = fzzFilePaths.value(boardName, "");
		int loaded = 0;
		MainWindow * mainWindow = app->loadWindows(loaded, false);
		mainWindow->noBackup();

		FolderUtils::setOpenSaveFolderAux(fzDir.absolutePath());

		if (!mainWindow->loadWhich(path, false, false, "")) {
			DebugDialog::debug(QString("failed to load '%1'").arg(path));
			return false;
		}

		mainWindow->showPCBView();
		
		QList<ItemBase *> boards = mainWindow->pcbView()->findBoard();
		
		if (boards.count() == 0) {
			DebugDialog::debug(QString("no board found in '%1'").arg(path));
			return false;
		}
		if (boards.count() > 1) {
			DebugDialog::debug(QString("multiple boards found in '%1'").arg(path));
			return false;
		}
		ItemBase * boardItem = boards.at(0);

		foreach (QGraphicsItem * item, mainWindow->pcbView()->scene()->items()) {
			ItemBase * itemBase = dynamic_cast<ItemBase *>(item);
			if (itemBase == NULL) continue;

			itemBase->setMoveLock(false);
		}

		PanelItem * panelItem = new PanelItem;
		panelItem->boardName = boardName;
		panelItem->window = mainWindow;
		panelItem->path = path;
		panelItem->board = boardItem;
		panelItem->required = required;
		panelItem->maxOptional = optional;

		QRectF sbr = boardItem->layerKinChief()->sceneBoundingRect();
		panelItem->boardSizeInches = sbr.size() / FSvgRenderer::printerScale();
		DebugDialog::debug(QString("board size inches c %1, %2, %3")
				.arg(panelItem->boardSizeInches.width())
				.arg(panelItem->boardSizeInches.height())
				.arg(path));

		/*
		QSizeF boardSize = boardItem->size();
		ResizableBoard * resizableBoard = qobject_cast<ResizableBoard *>(boardItem->layerKinChief());
		if (resizableBoard != NULL) {
			panelItem->boardSizeInches = resizableBoard->getSizeMM() / 25.4;
			DebugDialog::debug(QString("board size inches a %1, %2, %3")
				.arg(panelItem->boardSizeInches.width())
				.arg(panelItem->boardSizeInches.height())
				.arg(path), boardItem->sceneBoundingRect());
		}
		else {
			panelItem->boardSizeInches = boardSize / FSvgRenderer::printerScale();
			DebugDialog::debug(QString("board size inches b %1, %2, %3")
				.arg(panelItem->boardSizeInches.width())
				.arg(panelItem->boardSizeInches.height())
				.arg(path), boardItem->sceneBoundingRect());
		}
		*/

		bool tooBig = false;
		if (panelItem->boardSizeInches.width() >= panelParams.panelWidth) {
			tooBig = panelItem->boardSizeInches.width() >= panelParams.panelHeight;
			if (!tooBig) {
				tooBig = panelItem->boardSizeInches.height() >= panelParams.panelWidth;
			}
		}

		if (!tooBig) {
			if (panelItem->boardSizeInches.height() >= panelParams.panelHeight) {
				tooBig = panelItem->boardSizeInches.height() >= panelParams.panelWidth;
				if (!tooBig) {
					tooBig = panelItem->boardSizeInches.width() >= panelParams.panelHeight;
				}
			}
		}

		if (tooBig) {
			DebugDialog::debug(QString("board is too big for panel '%1'").arg(path));
			return false;
		}


		refPanelItems.insert(boardName, panelItem);
		mainWindow->setCloseSilently(true);

		board = board.nextSiblingElement("board");
	}

	return true;
}

bool Panelizer::initPanelParams(QDomElement & root, PanelParams & panelParams)
{
	panelParams.outputFolder = root.attribute("outputFolder");
	QDir outputDir(panelParams.outputFolder);
	if (!outputDir.exists()) {
		DebugDialog::debug(QString("Output folder '%1' doesn't exist").arg(panelParams.outputFolder));
		return false;
	}

	panelParams.prefix = root.attribute("prefix");
	if (panelParams.prefix.isEmpty()) {
		DebugDialog::debug(QString("Output file prefix not specified"));
		return false;
	}

	bool ok;
	panelParams.panelWidth = TextUtils::convertToInches(root.attribute("width"), &ok, false);
	if (!ok) {
		DebugDialog::debug(QString("Can't parse panel width '%1'").arg(root.attribute("width")));
		return false;
	}

	panelParams.panelHeight = TextUtils::convertToInches(root.attribute("height"), &ok, false);
	if (!ok) {
		DebugDialog::debug(QString("Can't parse panel height '%1'").arg(root.attribute("height")));
		return false;
	}

	panelParams.panelSpacing = TextUtils::convertToInches(root.attribute("spacing"), &ok, false);
	if (!ok) {
		DebugDialog::debug(QString("Can't parse panel spacing '%1'").arg(root.attribute("spacing")));
		return false;
	}

	panelParams.panelBorder = TextUtils::convertToInches(root.attribute("border"), &ok, false);
	if (!ok) {
		DebugDialog::debug(QString("Can't parse panel border '%1'").arg(root.attribute("border")));
		return false;
	}

	return true;

}

int Panelizer::placeBestFit(Tile * tile, UserData userData) {
	if (TiGetType(tile) != Tile::SPACE) return 0;

	BestPlace * bestPlace = (BestPlace *) userData;
	TileRect tileRect;
	TiToRect(tile, &tileRect);
	int w = tileRect.xmaxi - tileRect.xmini;
	int h = tileRect.ymaxi - tileRect.ymini;
	if (bestPlace->width > w && bestPlace->height > w) {
		return 0;
	}

	int fitCount = 0;
	bool fit[4];
	double area[4];
	for (int i = 0; i < 4; i++) {
		fit[i] = false;
		area[i] = Worst;
	}

	if (w >= bestPlace->width && h >= bestPlace->height) {
		fit[0] = true;
		area[0] = (w * h) - (bestPlace->width * bestPlace->height);
		fitCount++;
	}
	if (h >= bestPlace->width && w >= bestPlace->height) {
		fit[1] = true;
		area[1] = (w * h) - (bestPlace->width * bestPlace->height);
		fitCount++;
	}

	if (!fit[0] && w >= bestPlace->width) {
		// see if adjacent tiles below are open
		TileRect temp;
		temp.xmini = tileRect.xmini;
		temp.xmaxi = temp.xmini + bestPlace->width;
		temp.ymini = tileRect.ymini;
		temp.ymaxi = temp.ymini + bestPlace->height;
		if (temp.ymaxi < bestPlace->maxRect.ymaxi) {
			QList<Tile*> spaces;
			TiSrArea(tile, bestPlace->plane, &temp, allSpaces, &spaces);
			if (spaces.count()) {
				int y = temp.ymaxi;
				foreach (Tile * t, spaces) {
					if (YMAX(t) > y) y = YMAX(t);			// find the bottom of the lowest open tile
				}
				fit[2] = true;
				fitCount++;
				area[2] = (w * (y - temp.ymini)) - (bestPlace->width * bestPlace->height);
			}
		}
	}

	if (!fit[1] && w >= bestPlace->height) {
		// see if adjacent tiles below are open
		TileRect temp;
		temp.xmini = tileRect.xmini;
		temp.xmaxi = temp.xmini + bestPlace->height;
		temp.ymini = tileRect.ymini;
		temp.ymaxi = temp.ymini + bestPlace->width;
		if (temp.ymaxi < bestPlace->maxRect.ymaxi) {
			QList<Tile*> spaces;
			TiSrArea(tile, bestPlace->plane, &temp, allSpaces, &spaces);
			if (spaces.count()) {
				int y = temp.ymaxi;
				foreach (Tile * t, spaces) {
					if (YMAX(t) > y) y = YMAX(t);			// find the bottom of the lowest open tile
				}
				fit[3] = true;
				fitCount++;
				area[3] = (w * (y - temp.ymini)) - (bestPlace->width * bestPlace->height);
			}
		}
	}

	if (fitCount == 0) return 0;

	// area is white space remaining after board has been inserteds
	
	int result = -1;
	for (int i = 0; i < 4; i++) {
		if (area[i] < bestPlace->bestArea) {
			result = i;
			break;
		}
	}
	if (result < 0) return 0;			// current bestArea is better

	bestPlace->bestTile = tile;
	bestPlace->bestTileRect = tileRect;
	if (fitCount == 1 || (bestPlace->width == bestPlace->height)) {
		if (fit[0] || fit[2]) {
			bestPlace->rotate90 = false;
		}
		else {
			bestPlace->rotate90 = true;
		}
		bestPlace->bestArea = area[result];
		return 0;
	}

	if (TiGetType(BL(tile)) == Tile::BUFFER) {
		// this is a leftmost tile
		// select for creating the biggest area after putting in the tile;
		double a1 = (w - bestPlace->width) * (bestPlace->height);
		double a2 = (h - bestPlace->height) * w;
		double a = qMax(a1, a2);
		double b1 = (w - bestPlace->height) * (bestPlace->width);
		double b2 = (h - bestPlace->width) * w;
		double b = qMax(b1, b2);
		bestPlace->rotate90 = (a < b);
		if (bestPlace->rotate90) {
			bestPlace->bestArea = fit[1] ? area[1] : area[3];
		}
		else {
			bestPlace->bestArea = fit[0] ? area[0] : area[2];
		}

		return 0;
	}

	TileRect temp;
	temp.xmini = bestPlace->maxRect.xmini;
	temp.xmaxi = tileRect.xmini - 1;
	temp.ymini = tileRect.ymini;
	temp.ymaxi = tileRect.ymaxi;
	QList<Tile*> obstacles;
	TiSrArea(tile, bestPlace->plane, &temp, allObstacles, &obstacles);
	int maxBottom = 0;
	foreach (Tile * obstacle, obstacles) {
		if (YMAX(obstacle) > maxBottom) maxBottom = YMAX(obstacle);
	}

	if (tileRect.ymini + bestPlace->width <= maxBottom && tileRect.ymini + bestPlace->height <= maxBottom) {
		// use the max length 
		if (bestPlace->width >= bestPlace->height) {
			bestPlace->rotate90 = true;
			bestPlace->bestArea = fit[1] ? area[1] : area[3];
		}
		else {
			bestPlace->rotate90 = false;
			bestPlace->bestArea = fit[0] ? area[0] : area[2];
		}

		return 0;
	}

	if (tileRect.ymini + bestPlace->width > maxBottom && tileRect.ymini + bestPlace->height > maxBottom) {
		// use the min length
		if (bestPlace->width <= bestPlace->height) {
			bestPlace->rotate90 = true;
			bestPlace->bestArea = fit[1] ? area[1] : area[3];
		}
		else {
			bestPlace->rotate90 = false;
			bestPlace->bestArea = fit[0] ? area[0] : area[2];
		}

		return 0;
	}

	if (tileRect.ymini + bestPlace->width <= maxBottom) {
		bestPlace->rotate90 = true;
		bestPlace->bestArea = fit[1] ? area[1] : area[3];
		return 0;
	}

	bestPlace->rotate90 = false;
	bestPlace->bestArea = fit[0] ? area[0] : area[2];
	return 0;
}

void Panelizer::addOptional(int optionalCount, QHash<QString, PanelItem *> & refPanelItems, QList<PanelItem *> & insertPanelItems, PanelParams & panelParams, QList<PlanePair *> & planePairs)
{
	while (optionalCount > 0) {
		int ix = qFloor(qrand() * optionalCount / (double) RAND_MAX);
		int soFar = 0;
		foreach (PanelItem * panelItem, refPanelItems.values()) {
			if (panelItem->maxOptional == 0) continue;

			if (ix >= soFar && ix < soFar + panelItem->maxOptional) {
				PanelItem * copy = new PanelItem(panelItem);
				if (bestFitOne(copy, panelParams, planePairs, false)) {
					// got one
					panelItem->maxOptional--;
					optionalCount--;
					insertPanelItems.append(copy);
				}
				else {
					// don't bother trying this one again
					optionalCount -= panelItem->maxOptional;
					panelItem->maxOptional = 0;
				}
				break;
			}

			soFar += panelItem->maxOptional;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////

void Panelizer::inscribe(FApplication * app, const QString & panelFilename) 
{
	QFile file(panelFilename);

	QString errorStr;
	int errorLine;
	int errorColumn;

	DebugDialog::setEnabled(true);

	QDomDocument domDocument;
	if (!domDocument.setContent(&file, true, &errorStr, &errorLine, &errorColumn)) {
		DebugDialog::debug(QString("Unable to parse '%1': '%2' line:%3 column:%4").arg(panelFilename).arg(errorStr).arg(errorLine).arg(errorColumn));
		return;
	}

	QDomElement root = domDocument.documentElement();
	if (root.isNull() || root.tagName() != "panelizer") {
		DebugDialog::debug(QString("root element is not 'panelizer'"));
		return;
	}

	PanelParams panelParams;
	if (!initPanelParams(root, panelParams)) return;

	QDir outputDir = QDir::temp();
	QDir fzDir(outputDir);
	fzDir.cd("fz");
	if (!fzDir.exists()) {
		DebugDialog::debug(QString("unable to create fz folder in '%1'").arg(panelParams.outputFolder));
		return;
	}

	DebugDialog::debug(QString("fz folder '%1'\n").arg(fzDir.absolutePath()));


	QDomElement boards = root.firstChildElement("boards");
	QDomElement board = boards.firstChildElement("board");
	if (board.isNull()) {
		DebugDialog::debug(QString("no <board> elements found"));
		return;
	}

	QHash<QString, QString> fzzFilePaths;
	QDomElement paths = root.firstChildElement("paths");
	QDomElement path = paths.firstChildElement("path");
	if (path.isNull()) {
		DebugDialog::debug(QString("no <path> elements found"));
		return;
	}

	collectFiles(path, fzzFilePaths);
	if (fzzFilePaths.count() == 0) {
		DebugDialog::debug(QString("no fzz files found in paths"));
		return;
	}
	
	board = boards.firstChildElement("board");
	if (!checkBoards(board, fzzFilePaths)) return;

	app->createUserDataStoreFolderStructure();
	app->registerFonts();
	ReferenceModel * referenceModel = app->loadReferenceModel();

	if (!app->loadBin("")) {
		DebugDialog::debug(QString("load bin failed"));
		return;
	}

	board = boards.firstChildElement("board");
	while (!board.isNull()) {
		MainWindow * mainWindow = inscribeBoard(board, fzzFilePaths, app, fzDir, referenceModel);
		if (mainWindow) {
			mainWindow->setCloseSilently(true);
			mainWindow->close();
		}
		board = board.nextSiblingElement("board");
	}

	// TODO: delete temp fz folder

}

MainWindow * Panelizer::inscribeBoard(QDomElement & board, QHash<QString, QString> & fzzFilePaths, FApplication * app, QDir & fzDir, ReferenceModel * referenceModel)
{
	QString boardName = board.attribute("name");

	QString path = fzzFilePaths.value(boardName, "");

	int loaded = 0;
	MainWindow * mainWindow = app->loadWindows(loaded, false);
	mainWindow->noBackup();

	FolderUtils::setOpenSaveFolderAux(fzDir.absolutePath());

	if (!mainWindow->loadWhich(path, false, false, "")) {
		DebugDialog::debug(QString("failed to load '%1'").arg(path));
		return mainWindow;
	}

	mainWindow->showPCBView();
	QString fritzingVersion = mainWindow->fritzingVersion();
	VersionThing versionThing;
	versionThing.majorVersion = 0;
	versionThing.minorVersion = 7;
	versionThing.minorSubVersion = 0;
	versionThing.releaseModifier = "";

	VersionThing versionThingFz;
	Version::toVersionThing(fritzingVersion, versionThingFz);
	bool oldGround = !Version::greaterThan(versionThing, versionThingFz);
	
		
	QList<ItemBase *> boards = mainWindow->pcbView()->findBoard();	
	if (boards.count() == 0) {
		DebugDialog::debug(QString("no board found in '%1'").arg(path));
		return mainWindow;
	}
	if (boards.count() == 2) {
		DebugDialog::debug(QString("multiple boards found in '%1'").arg(path));
		return mainWindow;
	}
	ItemBase * boardItem = boards.at(0);

	bool filled = false;
	QString fillType = mainWindow->pcbView()->characterizeGroundFill();

	bool swapped = false;
	if (mainWindow->pcbView()->boardLayers() == 1) {
		QMultiHash<QString, QString> properties = boardItem->modelPart()->properties();
		QString family = properties.value("family", "");
		QString shape = properties.value("shape", "");
		if (!shape.isEmpty() && !family.isEmpty()) {
			QMultiHash<QString, QString> ps;
			ps.insert("layers", "2");
			ps.insert("shape", shape);
			QString newModuleID = referenceModel->retrieveModuleId(family, ps, "layers", true); 
			if (!newModuleID.isEmpty()) {
				ModelPart * modelPart = referenceModel->retrieveModelPart(newModuleID);
				if (modelPart) {
					QString newShape = modelPart->properties().value("shape", "");
					if (shape == newShape) {
						mainWindow->swapSelectedAux(boardItem, newModuleID);
						mainWindow->changeBoardLayers(2, true);
						mainWindow->removeGroundFill(true);
						fillType = GroundPlane::fillTypeNone;
						swapped = true;
					}
				}
			}
		}
		if (!swapped) {
			DebugDialog::debug(QString("\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n"
									"unable to convert to double-sided board %1\n"
									"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n").arg(path));
		}
	}

	if (fillType == GroundPlane::fillTypeNone) {
		mainWindow->copperFill();
		filled = true;
	}
	else if ((fillType == GroundPlane::fillTypeGround) && oldGround) {
		mainWindow->removeGroundFill(true);
		mainWindow->groundFill();
		filled = true;
	}


	if (swapped || filled) { 
		mainWindow->saveAsShareable(path, true);
		DebugDialog::debug(QString("%1 swapped:%2 filled:%3").arg(path).arg(swapped).arg(filled));
	}

	return mainWindow;
}
