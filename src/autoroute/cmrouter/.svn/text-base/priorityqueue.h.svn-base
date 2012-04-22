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

Based on code from http://kunalmaemo.blogspot.com/2010/09/simple-priority-queue-with-qt.html

********************************************************************

$Revision$:
$Author$:
$Date$

********************************************************************/

#ifndef PRIORITYQUEUE_H
#define PRIORITYQUEUE_H

#include <QQueue>
#include <QDebug>

typedef int Priority;

template<class T>

class PriorityQueue
{
public:

    PriorityQueue(){}

    void enqueue(Priority priority, T value)
    {
        Item<T> item(priority, value);
		// TODO: make this a binsert
        for(int i = 0 ; i < _queue.count() ; ++i ) {
            const Item<T>& otherItem = _queue[i];
            if( priority <= otherItem._priority )  {
                _queue.insert(i,item);
                return;
            }
        }
        _queue.append(item);
    }

	void clear() {
		_queue.clear();
	}

	void append(T value) {
		Item<T> item(0.0, value);
		_queue.append(item);
	}

    T dequeue()
    {
        const Item<T>& item = _queue.dequeue();
        return item._value;
    }

	void resetPriority(int ix, Priority newPriority)
	{
		const Item<T>& item = _queue.at(ix);
		Item<T> newitem(newPriority, item._value);
		_queue.removeAt(ix);
		_queue.append(newitem);
	}

	bool removeOne(T value) {
        for(int i = 0 ; i < _queue.count() ; ++i ) {
            const Item<T>& item = _queue[i];
            if( value == item._value )  {
                _queue.removeAt(i);
                return true;
            }
        }

		return false;
	}

	T at(int ix) 
	{
        const Item<T>& item = _queue.at(ix);
        return item._value;
	}

    int count()
    {
        return _queue.count();
    }

public:
	void sort() {
		qSort(_queue.begin(),_queue.end(), PriorityQueue::byPriority);
	}

private:

    template<class C>
    struct Item
    {
        Priority _priority;
        C _value;

        Item(Priority priority, C value)
        {
            _priority = priority;
            _value = value;
        }
    };

    QQueue< Item<T > > _queue;

	static bool byPriority(Item<T> & i1, Item<T> & i2)
	{
		return i1._priority > i2._priority;
	}

};

#endif // PRIORITYQUEUE_H
