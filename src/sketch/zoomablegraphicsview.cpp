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

$Revision: 5775 $:
$Author: cohen@irascible.com $:
$Date: 2012-01-25 10:56:11 -0800 (Wed, 25 Jan 2012) $

********************************************************************/

#include <QWheelEvent>
#include <QScrollBar>
#include <QSettings>
//#include <QGLWidget>

#include "zoomablegraphicsview.h"
#include "../utils/zoomslider.h"
#include "../utils/misc.h"


ZoomableGraphicsView::WheelMapping ZoomableGraphicsView::m_wheelMapping =
#ifdef Q_WS_WIN
	ZoomPrimary;
#else
	ScrollPrimary;
#endif

bool FirstTime = true;

ZoomableGraphicsView::ZoomableGraphicsView( QWidget * parent )
	: QGraphicsView(parent)
{
	m_scaleValue = 100;
	m_maxScaleValue = 2000;
	m_minScaleValue = 1;
	m_acceptWheelEvents = true;
	if (FirstTime) {
		FirstTime = false;
		QSettings settings;
		m_wheelMapping = (WheelMapping) settings.value("wheelMapping", m_wheelMapping).toInt();
		if (m_wheelMapping >= WheelMappingCount) {
			m_wheelMapping = ScrollPrimary;
		}
	}
	//setViewport(new QGLWidget);
}

void ZoomableGraphicsView::wheelEvent(QWheelEvent* event) {
	if (!m_acceptWheelEvents) {
		QGraphicsView::wheelEvent(event);
		return;
	}

	bool doZoom = false;
	bool doHorizontal = false;
	bool doVertical = false;

	bool control = event->modifiers() & Qt::ControlModifier;
	bool alt = event->modifiers() & altOrMetaModifier();
	bool shift = event->modifiers() & Qt::ShiftModifier;

	switch (m_wheelMapping) {
		case ScrollPrimary:
			if (control || alt) doZoom = true;
			else {
				if (event->orientation() == Qt::Horizontal) {
					doHorizontal = true;
				}
				else {
					doVertical = true;
				}
			}
			break;
		case ZoomPrimary:
			if (control || alt) {
				if (event->orientation() == Qt::Horizontal) {
					doHorizontal = true;
				}
				else {
					doVertical = true;
				}
			}
			else doZoom = true;
			break;
		default:
			// shouldn't happen
			return;
	}

	if (shift && (doVertical || doHorizontal)) {
		if (doVertical) {
			doVertical = false;
			doHorizontal = true;
		}
		else {
			doVertical = true;
			doHorizontal = false;
		}
	}

	int numSteps = event->delta() / 8;
	if (doZoom) {
		double delta = ((double) event->delta() / 120) * ZoomSlider::ZoomStep;
		if (delta == 0) return;

		// Scroll zooming relative to the current size
		relativeZoom(2*delta, true);

		emit wheelSignal();
	}
	else if (doVertical) {
		verticalScrollBar()->setValue( verticalScrollBar()->value() - numSteps);
	}
	else if (doHorizontal) {
		horizontalScrollBar()->setValue( horizontalScrollBar()->value() - numSteps);
	}
}

void ZoomableGraphicsView::relativeZoom(double step, bool centerOnCursor) {
	double tempSize = m_scaleValue + step;
	if (tempSize < m_minScaleValue) {
		m_scaleValue = m_minScaleValue;
		emit zoomOutOfRange(m_scaleValue);
		return;
	}
	if (tempSize > m_maxScaleValue) {
		m_scaleValue = m_maxScaleValue;
		emit zoomOutOfRange(m_scaleValue);
		return;
	}
	double tempScaleValue = tempSize/100;

	m_scaleValue = tempSize;

	//QPoint p = QCursor::pos();
	//QPoint q = this->mapFromGlobal(p);
	//QPointF r = this->mapToScene(q);

	QMatrix matrix;
	matrix.scale(tempScaleValue, tempScaleValue);
	if (centerOnCursor) {
		//this->setMatrix(QMatrix().translate(-r.x(), -r.y()) * matrix * QMatrix().translate(r.x(), r.y()));
        this->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
	}
	else {
		this->setTransformationAnchor(QGraphicsView::AnchorViewCenter);
	}
	this->setMatrix(matrix);

	emit zoomChanged(m_scaleValue);
}

void ZoomableGraphicsView::absoluteZoom(double percent) {
	relativeZoom(percent-m_scaleValue, false);
}

double ZoomableGraphicsView::currentZoom() {
	return m_scaleValue;
}

void ZoomableGraphicsView::setAcceptWheelEvents(bool accept) {
	m_acceptWheelEvents = accept;
}

void ZoomableGraphicsView::setWheelMapping(WheelMapping wm) {
	m_wheelMapping = wm;
}

ZoomableGraphicsView::WheelMapping ZoomableGraphicsView::wheelMapping() {
	return m_wheelMapping;
}

