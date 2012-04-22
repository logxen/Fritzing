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

You should have received a copy of the GNU General Public Licensetriple
along with Fritzing.  If not, see <http://www.gnu.org/licenses/>.

********************************************************************

$Revision$:
$Author$:
$Date$

********************************************************************/

#ifndef ITEMBASE_H
#define ITEMBASE_H

#include <QXmlStreamWriter>
#include <QPointF>
#include <QSize>
#include <QHash>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsSvgItem>
#include <QPointer>
#include <QUrl>
#include <QMap>
#include <QTimer>
#include <QCursor>

#include "../viewgeometry.h"
#include "../viewlayer.h"
#include "../viewidentifierclass.h"
#include "../utils/misc.h"

class ConnectorItem;

typedef QMultiHash<ConnectorItem *, ConnectorItem *> ConnectorPairHash;

typedef bool (*SkipCheckFunction)(ConnectorItem *);

class ItemBase : public QGraphicsSvgItem
{
Q_OBJECT

public:
	enum PluralType {
		Singular,
		Plural,
		NotSure
	};

public:
	ItemBase(class ModelPart*, ViewIdentifierClass::ViewIdentifier, const ViewGeometry &, long id, QMenu * itemMenu);
	virtual ~ItemBase();

	qint64 id() const;
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
	double z();
	virtual void saveGeometry() = 0;
	ViewGeometry & getViewGeometry();
	ViewGeometry::WireFlags wireFlags() const;
	virtual bool itemMoved() = 0;
	QSizeF size();
	class ModelPart * modelPart();
	void setModelPart(class ModelPart *);
	class ModelPartShared * modelPartShared();
	virtual void writeXml(QXmlStreamWriter &) {}
	virtual void saveInstance(QXmlStreamWriter &);
	virtual void saveInstanceLocation(QXmlStreamWriter &) = 0;
	virtual void writeGeometry(QXmlStreamWriter &);
	virtual void moveItem(ViewGeometry &) = 0;
	virtual void setItemPos(QPointF & pos);
	virtual void rotateItem(double degrees);
	virtual void flipItem(Qt::Orientations orientation);
	void transformItem(const QTransform &);
	virtual void transformItem2(const QMatrix &);
	virtual void removeLayerKin();
	ViewIdentifierClass::ViewIdentifier viewIdentifier();
	QString & viewIdentifierName();
	ViewLayer::ViewLayerID viewLayerID() const;
	void setViewLayerID(ViewLayer::ViewLayerID, const LayerHash & viewLayers);
	void setViewLayerID(const QString & layerName, const LayerHash & viewLayers);
	bool topLevel();

	void collectConnectors(ConnectorPairHash & connectorHash, SkipCheckFunction);

	virtual void busConnectorItems(class Bus * bus, QList<ConnectorItem *> & items);
	virtual void setHidden(bool hidden);
	bool hidden();
	virtual void setInactive(bool inactivate);
	bool inactive();
	ConnectorItem * findConnectorItemWithSharedID(const QString & connectorID, ViewLayer::ViewLayerSpec);
	void updateConnections(ConnectorItem *);
	virtual void updateConnections();
	virtual const QString & title() const;
	bool getRatsnest();
	const QHash<QString, QPointer<class Bus> > & buses();
	int itemType() const;					// wanted this to return ModelPart::ItemType but couldn't figure out how to get it to compile
	virtual bool sticky();
	virtual void setSticky(bool);
	virtual void addSticky(ItemBase *, bool stickem);
	virtual ItemBase * stickingTo();
	virtual QList< QPointer<ItemBase> > stickyList();
	virtual bool alreadySticking(ItemBase * itemBase);
	virtual bool stickyEnabled();
	ConnectorItem * anyConnectorItem();
	const QString & instanceTitle() const;
	QString label();
	virtual void updateTooltip();
	void setTooltip();
	void removeTooltip();
	bool hasConnectors();
	bool hasNonConnectors();
	bool hasConnections();
	bool canFlip(Qt::Orientations);
	bool canFlipHorizontal();
	void setCanFlipHorizontal(bool);
	bool canFlipVertical();
	void setCanFlipVertical(bool);
	virtual void clearModelPart();
	virtual bool hasPartLabel();
	ViewLayer::ViewLayerID partLabelViewLayerID();
	void clearPartLabel();
	bool isPartLabelVisible();
	void restorePartLabel(QDomElement & labelGeometry, ViewLayer::ViewLayerID);				// on loading from a file
	void movePartLabel(QPointF newPos, QPointF newOffset);												// coming down from the command object
	void partLabelMoved(QPointF oldPos, QPointF oldOffset, QPointF newPos, QPointF newOffset);			// coming up from the label
	void partLabelSetHidden(bool hide);
	void rotateFlipPartLabel(double degrees, Qt::Orientations);				// coming up from the label
	void doRotateFlipPartLabel(double degrees, Qt::Orientations);			// coming down from the command object
	QString makePartLabelSvg(bool blackOnly, double dpi, double printerScale);
	QPointF partLabelScenePos();
	QRectF partLabelSceneBoundingRect();
	bool isSwappable();
	void mousePressEvent(QGraphicsSceneMouseEvent *event);
	virtual void collectWireConnectees(QSet<class Wire *> & wires);
	virtual bool collectFemaleConnectees(QSet<ItemBase *> & items);
	void prepareGeometryChange();
	virtual void resetID();
	void updateConnectionsAux();
	virtual ItemBase * lowerConnectorLayerVisible(ItemBase *);
	void hoverEnterEvent( QGraphicsSceneHoverEvent * event );
	void hoverLeaveEvent( QGraphicsSceneHoverEvent * event );
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);
	virtual void figureHover();
	virtual QString retrieveSvg(ViewLayer::ViewLayerID, QHash<QString, QString> & svgHash, bool blackOnly, double dpi);
	virtual void slamZ(double newZ);
	bool isEverVisible();
	void setEverVisible(bool);
	virtual bool connectionIsAllowed(ConnectorItem *);
	virtual bool collectExtraInfo(QWidget * parent, const QString & family, const QString & prop, const QString & value, bool swappingEnabled, QString & returnProp, QString & returnValue, QWidget * & returnWidget);
	virtual QString getProperty(const QString & key);
	ConnectorItem * rightClickedConnector();
	virtual bool canEditPart();
	virtual bool hasCustomSVG();
	virtual void setProp(const QString & prop, const QString & value);
	QString prop(const QString & p);
	bool isObsolete();
	void prepareProps();
	void resetValues(const QString & family, const QString & prop);
	const QString & filename();
	void setFilename(const QString &);
	virtual PluralType isPlural();
	ViewLayer::ViewLayerSpec viewLayerSpec() const;
	void setViewLayerSpec(ViewLayer::ViewLayerSpec);
	virtual void calcRotation(QTransform & rotation, QPointF center, ViewGeometry &);
    void updateConnectors();
	const QString & moduleID();
	bool moveLock();
	virtual void setMoveLock(bool);
	void debugInfo(const QString & msg) const;
	virtual void addedToScene(bool temporary);
	virtual bool hasPartNumberProperty();
	void collectPropsMap(QString & family, QMap<QString, QString> &);
	virtual bool rotationAllowed();
	virtual bool rotation45Allowed();
	void ensureUniqueTitle(const QString &title, bool force);
	virtual void setDropOffset(QPointF offset);
	bool hasRubberBandLeg() const;
	void killRubberBandLeg();
	bool sceneEvent(QEvent *event);
	void clearConnectorItemCache();
	const QList<ConnectorItem *> & cachedConnectorItems();
	const QList<ConnectorItem *> & cachedConnectorItemsConst() const;
	bool inHover();
	virtual QRectF boundingRectWithoutLegs() const;
    QRectF boundingRect() const;
    virtual QPainterPath hoverShape() const;
	virtual const QCursor * getCursor(Qt::KeyboardModifiers);
	class PartLabel * partLabel();
	virtual void doneLoading();

public:
	virtual void getConnectedColor(ConnectorItem *, QBrush * &, QPen * &, double & opacity, double & negativePenWidth, bool & negativeOffsetRect);
	virtual void getUnconnectedColor(ConnectorItem *, QBrush * &, QPen * &, double & opacity, double & negativePenWidth, bool & negativeOffsetRect);
	virtual void getNormalColor(ConnectorItem *, QBrush * &, QPen * &, double & opacity, double & negativePenWidth, bool & negativeOffsetRect);
	virtual void getChosenColor(ConnectorItem *, QBrush * &, QPen * &, double & opacity, double & negativePenWidth, bool & negativeOffsetRect);
	virtual void getHoverColor(ConnectorItem *, QBrush * &, QPen * &, double & opacity, double & negativePenWidth, bool & negativeOffsetRect);
	virtual void getEqualPotentialColor(ConnectorItem *, QBrush * &, QPen * &, double & opacity, double & negativePenWidth, bool & negativeOffsetRect);

protected:
	static QPen normalPen;
	static QPen hoverPen;
	static QPen connectedPen;
	static QPen unconnectedPen;
	static QPen chosenPen;
	static QPen equalPotentialPen;
	static QBrush hoverBrush;
	static QBrush normalBrush;
	static QBrush connectedBrush;
	static QBrush unconnectedBrush;
	static QBrush chosenBrush;
	static QBrush equalPotentialBrush;
	static const double normalConnectorOpacity;

public:
	static QColor connectedColor();
	static QColor unconnectedColor();
	static QColor standardConnectedColor();
	static QColor standardUnconnectedColor();
	static void setConnectedColor(QColor &);
	static void setUnconnectedColor(QColor &);

public:
	virtual void hoverEnterConnectorItem(QGraphicsSceneHoverEvent * event, ConnectorItem * item);
	virtual void hoverLeaveConnectorItem(QGraphicsSceneHoverEvent * event, ConnectorItem * item);
	virtual void hoverMoveConnectorItem(QGraphicsSceneHoverEvent * event, class ConnectorItem * item);
	void hoverEnterConnectorItem();
	void hoverLeaveConnectorItem();
	virtual void connectorHover(ConnectorItem *, ItemBase *, bool hovering);
	virtual void clearConnectorHover();
	virtual void hoverUpdate();
	virtual bool filterMousePressConnectorEvent(ConnectorItem *, QGraphicsSceneMouseEvent *);
	virtual void mousePressConnectorEvent(ConnectorItem *, QGraphicsSceneMouseEvent *);
	virtual void mouseDoubleClickConnectorEvent(ConnectorItem *);
	virtual void mouseMoveConnectorEvent(ConnectorItem *, QGraphicsSceneMouseEvent *);
	virtual void mouseReleaseConnectorEvent(ConnectorItem *, QGraphicsSceneMouseEvent *);
	virtual bool acceptsMousePressConnectorEvent(ConnectorItem *, QGraphicsSceneMouseEvent *);
	virtual bool acceptsMouseDoubleClickConnectorEvent(ConnectorItem *, QGraphicsSceneMouseEvent *);
	virtual bool acceptsMouseMoveConnectorEvent(ConnectorItem *, QGraphicsSceneMouseEvent *);
	virtual bool acceptsMouseReleaseConnectorEvent(ConnectorItem *, QGraphicsSceneMouseEvent *);
	virtual void connectionChange(ConnectorItem * onMe, ConnectorItem * onIt, bool connect);
	virtual void connectedMoved(ConnectorItem * from, ConnectorItem * to);
	virtual ItemBase * layerKinChief();
	virtual const QList<ItemBase *> & layerKin();
	virtual void findConnectorsUnder() = 0;
	virtual ConnectorItem* newConnectorItem(class Connector *connector);
	virtual ConnectorItem* newConnectorItem(ItemBase * layerkin, Connector *connector);

	virtual void setInstanceTitle(const QString &title);
	void updatePartLabelInstanceTitle();

public slots:
	void showPartLabel(bool show, ViewLayer *);
	void hidePartLabel();
	void partLabelChanged(const QString &newText);
	void swapEntry(const QString & text);

public:
	static bool zLessThan(ItemBase * & p1, ItemBase * & p2);
	static qint64 getNextID();
	static qint64 getNextID(qint64 fromIndex);
	static class FSvgRenderer * setUpImage(class ModelPart * modelPart, ViewIdentifierClass::ViewIdentifier, ViewLayer::ViewLayerID, ViewLayer::ViewLayerSpec, class LayerAttributes &, QString & error);
	static QString getSvgFilename(ModelPart *, const QString & baseName); 

protected:
	void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
	void mouseReleaseEvent(QGraphicsSceneMouseEvent *event );
	virtual void paintHover(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget); 
	virtual void paintHover(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget, const QPainterPath & shape); 
    virtual void paintSelected(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
	virtual void paintBody(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

	QVariant itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant & value);

	virtual QStringList collectValues(const QString & family, const QString & prop, QString & value);

	void setInstanceTitleTooltip(const QString& text);
	virtual void setDefaultTooltip();
	void setInstanceTitleAux(const QString & title);
	void saveLocAndTransform(QXmlStreamWriter & streamWriter);

protected:
	static bool getFlipDoc(ModelPart * modelPart, const QString & filename, ViewLayer::ViewLayerID viewLayerID, ViewLayer::ViewLayerSpec, QDomDocument &);
	static bool fixCopper1(ModelPart * modelPart, const QString & filename, ViewLayer::ViewLayerID viewLayerID, ViewLayer::ViewLayerSpec, QDomDocument &);

protected:
 	QSizeF m_size;
	qint64 m_id;
	ViewGeometry m_viewGeometry;
	QPointer<ModelPart> m_modelPart;
	ViewIdentifierClass::ViewIdentifier m_viewIdentifier;
	ViewLayer::ViewLayerID m_viewLayerID;
	int m_connectorHoverCount;
	int m_connectorHoverCount2;
	int m_hoverCount;
	bool m_hidden;
	bool m_inactive;
	bool m_sticky;
	QHash< long, QPointer<ItemBase> > m_stickyList;
	QMenu *m_itemMenu;
	bool m_canFlipHorizontal;
	bool m_canFlipVertical;
	bool m_zUninitialized;
	QPointer<class PartLabel> m_partLabel;
	bool m_spaceBarWasPressed;
	bool m_hoverEnterSpaceBarWasPressed;
	bool m_everVisible;
	ConnectorItem * m_rightClickedConnector;
	QMap<QString, QString> m_propsMap;
	QString m_filename;
	ViewLayer::ViewLayerSpec m_viewLayerSpec;
	bool m_moveLock;
	bool m_hasRubberBandLeg;
	QList<ConnectorItem *> m_cachedConnectorItems;
	QGraphicsSvgItem * m_moveLockItem;

protected:
	static long nextID;
	static QPointer<class ReferenceModel> referenceModel;

public:
	static const QString ITEMBASE_FONT_PREFIX;
	static const QString ITEMBASE_FONT_SUFFIX;
	static QHash<QString, QString> TranslatedPropertyNames;
	static QString SvgFilesDir;
	const static QColor hoverColor;
	const static double hoverOpacity;
	const static QColor connectorHoverColor;
	const static double connectorHoverOpacity;

public:
	static void initNames();
	static void cleanup();
	static ItemBase * extractTopLevelItemBase(QGraphicsItem * thing);
	static QString partInstanceDefaultTitle;
	static QList<ItemBase *> emptyList;
	static QString translatePropertyName(const QString & key);
	static void setReferenceModel(class ReferenceModel *);


};


Q_DECLARE_METATYPE( ItemBase* );			// so we can stash them in a QVariant


#endif
