//
// begin license header
//
// This file is part of Pixy CMUcam5 or "Pixy" for short
//
// All Pixy source code is provided under the terms of the
// GNU General Public License v2 (http://www.gnu.org/licenses/gpl-2.0.html).
// Those wishing to use Pixy source code, software and/or
// technologies under different licensing terms should contact us at
// cmucam@cs.cmu.edu. Such licensing terms are available for
// all portions of the Pixy codebase presented here.
//
// end license header
//

#ifndef SIMPLELIST_H
#define SIMPLELIST_H

#include <new>

template <typename Object> class SimpleListNode;

template <typename Object> class SimpleList
{
	public:
	SimpleList()
	{
		m_first = m_last = NULL;
		m_size = 0;
	}
	~SimpleList()
	{
		clear();
	}
	
	void clear()
	{
		SimpleListNode<Object> *n, *temp;

		n = m_first;
		while(n)
		{
			temp = n->m_next;
			delete n;
			n = temp;
		}
		m_first = m_last = NULL;
		m_size = 0;
	}
	
	SimpleListNode<Object> *add(const Object &object)
	{
		SimpleListNode<Object> *node = new (std::nothrow) SimpleListNode<Object>;

		if (node==NULL)
			return NULL;
		
		node->m_object = object;
		add(node);	

		return node;
	}
	
	void add(SimpleListNode<Object> *node)
	{
		m_size++;
		if (m_first==NULL)
			m_first = node;
		else
			m_last->m_next = node;
		m_last = node;
	}
	
	bool remove(SimpleListNode<Object> *node)
	{
		SimpleListNode<Object> *n, *nprev=NULL;
		bool result = false;
		n = m_first;
		while(n)
		{
			if (n==node)
			{
				if (node==m_first)
					m_first = node->m_next;
				if (node==m_last)
					m_last = nprev;
				if (nprev)
					nprev->m_next = n->m_next;
				delete n;
				result = true;
				m_size--;
				break;
			}
			nprev = n;
			n = n->m_next;
		}
		return result;
	}
	
	void merge(SimpleList<Object> *list)
	{
		if (list && list->m_first)
		{
			if (m_last)
				m_last->m_next = list->m_first;
			if (m_first==NULL)
				m_first = list->m_first;
			m_last = list->m_last;
			m_size += list->m_size;
			list->m_first = NULL; // prevent list from being destroyed when it is deleted
			list->m_last = NULL;
		}		
	}
	
	SimpleListNode<Object> *m_first;
	SimpleListNode<Object> *m_last;
	uint16_t m_size;
};

template <typename Object> class SimpleListNode
{
	public:
	SimpleListNode()
	{
		m_next = NULL;
	}
	
	Object m_object;
	SimpleListNode<Object> *m_next;	
};
	

#endif // SIMPLELIST_H
