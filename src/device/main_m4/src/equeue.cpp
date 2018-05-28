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

#include <string.h>
#include "../inc/equeue.h" // need the relative path because Qt has the same file!
#include <pixyvals.h>
#include "pixy_init.h"

Equeue::Equeue()
{
    m_fields = (EqueueFields *)EQ_LOC;
    reset();
}

Equeue::~Equeue()
{
}

void Equeue::reset()
{
    memset((void *)m_fields, 0, sizeof(EqueueFields));
}

uint32_t Equeue::dequeue(uint16_t *val)
{
    uint16_t len = m_fields->produced - m_fields->consumed;
    if (len)
    {
        *val = m_fields->data[m_fields->readIndex++];
        m_fields->consumed++;
        if (m_fields->readIndex==EQ_MEM_SIZE)
            m_fields->readIndex = 0;
        return 1;
    }
    return 0;
}

// first word is always a code hscan or vscan
// read until next code
// if code is an error, eat it and return error
// if we run out of data, we return 0
uint32_t Equeue::readLine(uint16_t *mem, uint32_t size, bool *eof, bool *error)
{
	uint16_t len = m_fields->produced - m_fields->consumed;
	uint16_t i, j;
	uint8_t codes;
	
	for (i=0, j=m_fields->readIndex, codes=0, *eof=false, *error=false; true; i++)
	{
		if (i>=len || i>=size)
			return 0;
		
		mem[i] = m_fields->data[j];
		
		if (m_fields->data[j]>=EQ_HSCAN_LINE_START)
		{
			if (m_fields->data[j]==EQ_ERROR)
			{
				i++; // return and eat error code
				*error = true;
				break;
			}
			else if (m_fields->data[j]==EQ_FRAME_END)
			{
				i++; // return and eat eof code
				*eof = true;
				break;
			}
			codes++;
			if (codes>=2)
				// don't return code
				break;
		}
		j++;
		if (j==EQ_MEM_SIZE)
			j = 0;
	}
		
	// flush what we read
	m_fields->consumed += i;
	m_fields->readIndex += i;
	if (m_fields->readIndex>=EQ_MEM_SIZE)	
		m_fields->readIndex -= EQ_MEM_SIZE;

	return i;
}

void Equeue::flush()
{
    uint16_t len = m_fields->produced - m_fields->consumed;

    m_fields->consumed += len;
    m_fields->readIndex += len;
    if (m_fields->readIndex>=EQ_MEM_SIZE)
        m_fields->readIndex -= EQ_MEM_SIZE;
}


