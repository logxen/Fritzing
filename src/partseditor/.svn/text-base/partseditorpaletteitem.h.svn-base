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



#ifndef PARTSEDITORPALETTEITEM_H_
#define PARTSEDITORPALETTEITEM_H_

#include "../items/paletteitem.h"
#include "../utils/misc.h"
#include "../utils/svgandpartfilepath.h"

class PartsEditorAbstractView;
class PartsEditorView;

class PartsEditorPaletteItem : public PaletteItem {
	Q_OBJECT
	public:
		PartsEditorPaletteItem(PartsEditorView *owner, ModelPart * modelPart, ViewIdentifierClass::ViewIdentifier viewIdentifier);
		PartsEditorPaletteItem(PartsEditorView *owner, ModelPart * modelPart, ViewIdentifierClass::ViewIdentifier viewIdentifier, SvgAndPartFilePath *path);
		~PartsEditorPaletteItem();

		virtual void writeXml(QXmlStreamWriter &);
		virtual void writeXmlLocation(QXmlStreamWriter & streamWriter);
		const QList< QPointer<Connector> > &connectors();
		SvgAndPartFilePath* svgFilePath();
		void setSvgFilePath(SvgAndPartFilePath *sp);

		QDomDocument *svgDom();
		QString flatSvgFilePath();

		void setConnector(const QString &id, Connector *conn);

		void createSvgFile(QString path);
		bool createSvgPath(const QString &modelPartSharedPath, const QString &layerFileName);
		void setItemSVG(const QString &);

	protected:
		bool setUpImage(ModelPart * modelPart, ViewIdentifierClass::ViewIdentifier viewIdentifier, const LayerHash & viewLayers, ViewLayer::ViewLayerID, ViewLayer::ViewLayerSpec, bool doConnectors, LayerAttributes &, QString & error);
		ConnectorItem* newConnectorItem(Connector *connector);
		LayerKinPaletteItem * newLayerKinPaletteItem(
			PaletteItemBase * chief, ModelPart *, ViewIdentifierClass::ViewIdentifier, const ViewGeometry & viewGeometry, long id,
			ViewLayer::ViewLayerID, ViewLayer::ViewLayerSpec, QMenu * itemMenu, const LayerHash & viewLayers
		);
		QString xmlViewLayerID();

		void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
		void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);


		QDomDocument *m_svgDom;
		QString m_originalSvgPath;

		SvgAndPartFilePath *m_svgStrings;
		QList< QPointer<Connector> > m_connectors;
		QList< QPointer<Connector> > m_connectorsTemp;

		PartsEditorView *m_owner;
		bool m_shouldDeletePath;
		QString m_itemSVG;

		LayerList m_extraViewLayers;
};

#endif /* PARTSEDITORPALETTEITEM_H_ */
