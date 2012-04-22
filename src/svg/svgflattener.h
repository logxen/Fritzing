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

$Revision: 5308 $:
$Author: cohen@irascible.com $:
$Date: 2011-07-30 12:09:56 -0700 (Sat, 30 Jul 2011) $

********************************************************************/

#ifndef SVGFLATTENER_H
#define SVGFLATTENER_H

#include "svgfilesplitter.h"
#include <QMatrix>
#include <QSvgRenderer>

class SvgFlattener : public SvgFileSplitter
{
public:
    SvgFlattener();

    void flattenChildren(QDomElement & element);
    void unRotateChild(QDomElement & element,QMatrix transform);

public:
	static void flipSMDSvg(const QString & filename, const QString & svg, QDomDocument & flipDoc, const QString & elementID, const QString & altElementID, double printerScale);

protected:
	static void flipSMDElement(QDomDocument & domDocument, QSvgRenderer & renderer, QDomElement & element, const QString & att, QDomElement altAtt, const QString & altElementID, double printerScale);
    static bool hasOtherTransform(QDomElement & element);
    static bool hasTranslate(QDomElement & element);

protected slots:
    void rotateCommandSlot(QChar command, bool relative, QList<double> & args, void * userData);

};

#endif // SVGFLATTENER_H
