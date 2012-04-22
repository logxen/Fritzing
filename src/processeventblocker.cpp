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

$Revision: 4542 $:
$Author: cohen@irascible.com $:
$Date: 2010-10-14 22:15:26 -0700 (Thu, 14 Oct 2010) $

********************************************************************/

#include "processeventblocker.h"
#include <QApplication>

ProcessEventBlocker * ProcessEventBlocker::m_singleton = new ProcessEventBlocker();

ProcessEventBlocker::ProcessEventBlocker()
{
	m_count = 0;
}

void ProcessEventBlocker::processEvents() {
	m_singleton->_processEvents();
}

bool ProcessEventBlocker::isProcessing() {
	return m_singleton->_isProcessing();
}

void ProcessEventBlocker::block() {
	return m_singleton->_inc(1);
}

void ProcessEventBlocker::unblock() {
	return m_singleton->_inc(-1);
}

void ProcessEventBlocker::_processEvents() {
	m_mutex.lock();
	m_count++;
	m_mutex.unlock();
	QApplication::processEvents();
	m_mutex.lock();
	m_count--;
	m_mutex.unlock();
}

bool ProcessEventBlocker::_isProcessing() {
	m_mutex.lock();
	bool result = m_count > 0;
	m_mutex.unlock();
	return result;
}

void ProcessEventBlocker::_inc(int i) {
	m_mutex.lock();
	m_count += i;
	m_mutex.unlock();
}



