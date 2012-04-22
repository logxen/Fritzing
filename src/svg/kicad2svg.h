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

$Revision: 4353 $:
$Author: cohen@irascible.com $:
$Date: 2010-07-12 01:57:33 -0700 (Mon, 12 Jul 2010) $

********************************************************************/

#ifndef KICAD2SVG_H
#define KICAD2SVG_H

#include "x2svg.h"

class Kicad2Svg : public X2Svg
{

public:
	Kicad2Svg();

	QString makeMetadata(const QString & filename, const QString & type, const QString & name);
	QString endMetadata();

protected:
	QString m_title;
	QString m_description;
};

#endif // KICAD2SVG_H
