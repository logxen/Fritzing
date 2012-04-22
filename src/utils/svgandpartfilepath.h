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

$Revision: 4183 $:
$Author: cohen@irascible.com $:
$Date: 2010-05-06 13:30:19 -0700 (Thu, 06 May 2010) $

********************************************************************/

#ifndef SVGANDPARTFILEPATH_H_
#define SVGANDPARTFILEPATH_H_

#include "misc.h"
#include "../debugdialog.h"

/*
 * While the svg and parts files, are loaded dynamically from different folders
 * (core, contrib and user), this structure let us keep truck of this structure,
 * without the need to deal with substrings creation.
 * So, for example, if an svg file is in "/fritzing/parts/svg/core/breadboard/svg_file.svg" then
 * absolutePath == "/fritzing/parts/svg/core/breadboard/svg_file.svg"
 * coreContribOrUser == "core"
 * relativePath == "breadboard/svg_file.svg"
 */

class SvgAndPartFilePath  {
public:
	SvgAndPartFilePath() {
		init("", "", "");
	}

	SvgAndPartFilePath(QString absolutePath, QString relativeFilePath) {
		init(absolutePath, "", relativeFilePath);
	}

	SvgAndPartFilePath(QString absolutePath, QString folderInParts, QString relativeFilePath) {
		init(absolutePath, folderInParts, relativeFilePath);
	}

	const QString &absolutePath() {
		return m_absolutePath;
	}
	void setAbsolutePath(const QString &partFolderPath) {
		m_absolutePath = partFolderPath;
	}

	const QString &coreContribOrUser() {
		return m_coreContribOrUser;
	}
	void setCoreContribOrUser(const QString &coreContribOrUser) {
		m_coreContribOrUser = coreContribOrUser;
	}

	const QString &relativePath() {
		return m_relativePath;
	}
	void setRelativePath(const QString &fileRelativePath) {
		m_relativePath = fileRelativePath;
	}

	QString concat() {
		return m_absolutePath+"/"+m_coreContribOrUser+"/"+m_relativePath;
	}

protected:
	void init(QString absolutePath, QString folderInParts, QString relativeFilePath)
	{
		m_absolutePath = absolutePath;
		m_relativePath = relativeFilePath;
		m_coreContribOrUser = folderInParts;
	}



protected:
	QString m_absolutePath;
	QString m_relativePath;
	QString m_coreContribOrUser;
};

#endif /* SVGANDPARTFILEPATH_H_ */
