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

#include "misc.h"

#ifndef _TRACKER_H
#define _TRACKER_H

#define TR_DEFAULT_LEADING_THRESH      500 // milliseconds
#define TR_DEFAULT_TRAILING_THRESH     500 // milliseconds
#define TR_MAXVAL                      0xffffffff
#define TR_MAXAGE                      0xff

#define TR_EVENT_INVALIDATED           0x01
#define TR_EVENT_VALIDATED             0x02

enum TrackerState
{
	TR_VALID, 
	TR_TRAILING, // considered valid
	TR_LEADING,  // considered invalid
	TR_INVALID
};

//template <typename Object, bool validFunc(const Object &, const Object &)> struct Tracker
//template <typename Object, uint8_t leadingThresh, uint8_t trailingThresh> struct Tracker
template <typename Object> struct Tracker
{
	Tracker(uint16_t leadingThresh=TR_DEFAULT_LEADING_THRESH, uint16_t trailingThresh=TR_DEFAULT_TRAILING_THRESH)
	{
		setTiming(leadingThresh, trailingThresh);
		reset();
	}
	
	Tracker(Object object, uint8_t index, uint16_t leadingThresh=TR_DEFAULT_LEADING_THRESH, uint16_t trailingThresh=TR_DEFAULT_TRAILING_THRESH)
	{
		m_object = object;
		m_index = index;
		setTiming(leadingThresh, trailingThresh);
		reset();
	}
	
	void reset()
	{
		m_state = TR_LEADING; // start in leading state
		m_minVal = TR_MAXVAL;
		m_minObject = NULL;
		m_events = 0;
		m_eventsShadow = 0;
		m_age = 0;
		setTimerMs(&m_timer);
	}
	
	void setTiming(uint16_t leadingThresh, uint16_t trailingThresh)
	{
		m_leadingThresh = leadingThresh;
		m_trailingThresh = trailingThresh;
	}
	
	void resetMin()
	{
		m_minVal = TR_MAXVAL;
	}
	
	void setMin(Object *object, uint32_t val)
	{
		m_minObject = object;
		m_minVal = val;
	}

	bool swappable(uint32_t min, Object *out)
	{
		if (out->m_tracker==NULL) // no tracker, so we're good 
			return true;
		else if	(min>=out->m_tracker->m_minVal) // tracker exists, but we need to be better...
			return false;
		else if (m_state==TR_VALID || m_state==TR_TRAILING) // if we're going to swap, we're good if we're valid, regardless
			return true;
		else // we (this) is invalid
			return (out->m_tracker->m_state==TR_LEADING || out->m_tracker->m_state==TR_INVALID); // if they are invalid, we're good to swap also
	}
	
	uint8_t update()
	{	
		uint8_t events = 0;
		bool success = m_minVal!=TR_MAXVAL;

		if (success)
			m_object = *m_minObject;
		switch(m_state)
		{
			case TR_INVALID:
				if (success)
				{
					if (m_leadingThresh==0)
					{
						m_state = TR_VALID;
						events |= TR_EVENT_VALIDATED;
						m_events |= TR_EVENT_VALIDATED;
					}
					else
					{
						setTimerMs(&m_timer);
						m_state = TR_LEADING;
					}
				}
				break;
			
			case TR_LEADING:
				if (success)
				{
					if (getTimerMs(m_timer)>=m_leadingThresh)
					{
						m_state = TR_VALID;
						events |= TR_EVENT_VALIDATED;
						m_events |= TR_EVENT_VALIDATED;
					}
				}
				else
				{
					m_state = TR_INVALID;
					events |= TR_EVENT_INVALIDATED;
					m_events |= TR_EVENT_INVALIDATED;
				}
				break;
			
			case TR_VALID:
				if (!success)
				{
					if (m_trailingThresh==0)
					{
						m_state = TR_INVALID;
						events |= TR_EVENT_INVALIDATED;
						m_events |= TR_EVENT_INVALIDATED;
					}
					else
					{
						setTimerMs(&m_timer);
						m_state = TR_TRAILING;
					}
				}
				if (m_age<TR_MAXAGE)
					m_age++;				
				break;
			
			case TR_TRAILING:
				if (success)
				{
					m_state = TR_VALID;
					events |= TR_EVENT_VALIDATED;
					m_events |= TR_EVENT_VALIDATED;
				}
				else
				{
					if (getTimerMs(m_timer)>=m_trailingThresh)
					{
						m_state = TR_INVALID;
						events |= TR_EVENT_INVALIDATED;
						m_events |= TR_EVENT_INVALIDATED;
					}
				}
				if (m_age<TR_MAXAGE)
					m_age++;
				break;
			
			default:
				m_state = TR_INVALID;
				events |= TR_EVENT_INVALIDATED;
				m_events |= TR_EVENT_INVALIDATED;
		}
		return events;
	}
	
	Object *get()
	{
		if (m_state==TR_INVALID || m_state==TR_LEADING)
			return NULL;
		else
			return &m_object;
	}
	
	TrackerState m_state;
	uint8_t m_index;
	uint8_t m_events;
	uint8_t m_eventsShadow;
	uint16_t m_timer;
	uint16_t m_leadingThresh;
	uint16_t m_trailingThresh;
	uint8_t m_age;
	uint32_t m_minVal;
	Object *m_minObject;
	Object m_object;
};


#endif
