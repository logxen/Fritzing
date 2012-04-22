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

$Revision: 5879 $:
$Author: cohen@irascible.com $:
$Date: 2012-02-25 13:14:13 -0800 (Sat, 25 Feb 2012) $

********************************************************************/

#ifndef GERBERGENERATOR_H
#define GERBERGENERATOR_H

#include <QString>

#include "../viewlayer.h"
#include "svg2gerber.h"

class GerberGenerator
{

public:
	static void exportToGerber(const QString & filename, const QString & exportDir, class ItemBase * board, class PCBSketchWidget *, bool displayMessageBoxes);
	static QString clipToBoard(QString svgString, QRectF & boardRect, const QString & layerName, SVG2gerber::ForWhy, const QString & clipString);
	static QString clipToBoard(QString svgString, ItemBase * board, const QString & layerName, SVG2gerber::ForWhy, const QString & clipString);
	static int doEnd(const QString & svg, int boardLayers, const QString & layerName, SVG2gerber::ForWhy forWhy, QSizeF svgSize, 
						const QString & exportDir, const QString & prefix, const QString & suffix, bool displayMessageBoxes, bool chopPrefix);
	static QString cleanOutline(const QString & svgOutline);

public:
	static const QString SilkTopSuffix;
	static const QString SilkBottomSuffix;
	static const QString CopperTopSuffix;
	static const QString CopperBottomSuffix;
	static const QString MaskTopSuffix;
	static const QString MaskBottomSuffix;
	static const QString DrillSuffix;
	static const QString OutlineSuffix;
	static const QString MagicBoardOutlineID;

	static const double MaskClearanceMils;		

protected:
	static int doSilk(LayerList silkLayerIDs, const QString & silkName, const QString & gerberSuffix, ItemBase * board, PCBSketchWidget * sketchWidget, const QString & filename, const QString & exportDir, bool displayMessageBoxes, const QString & clipString);
	static int doMask(LayerList maskLayerIDs, const QString & maskName, const QString & gerberSuffix, ItemBase * board, PCBSketchWidget * sketchWidget, const QString & filename, const QString & exportDir, bool displayMessageBoxes, QString & clipString);
	static int doCopper(ItemBase * board, PCBSketchWidget * sketchWidget, LayerList & viewLayerIDs, const QString & copperName, const QString & copperSuffix, const QString & filename, const QString & exportDir, bool displayMessageBoxes);
	static int doDrill(ItemBase * board, PCBSketchWidget * sketchWidget, const QString & filename, const QString & exportDir, bool displayMessageBoxes);
	static void displayMessage(const QString & message, bool displayMessageBoxes);
	static bool saveEnd(const QString & layerName, const QString & exportDir, const QString & prefix, const QString & suffix, bool displayMessageBoxes, bool chopPrefix, SVG2gerber & gerber);


};

#endif // GERBERGENERATOR_H
