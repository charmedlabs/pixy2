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
#ifndef _EQUEUE_H
#define _EQUEUE_H
#include <stdint.h>

#define EQ_LOC        SRAM4_LOC
#define EQ_SIZE       0x3c00

#define EQ_MEM_SIZE  ((EQ_SIZE-sizeof(struct EqueueFields)+sizeof(uint16_t))/sizeof(uint16_t))

#define EQ_HSCAN_LINE_START 0xff00
#define EQ_VSCAN_LINE_START 0xff01
#define EQ_ERROR            0xfffe
#define EQ_FRAME_END        0xffff
#define EQ_NEGATIVE         0x8000

#define EQ_STREAM_EDGES     0x0001
#define EQ_STREAM_LINES	    0x0002

#define EQ_NODE_SIZE        12

#define EQ_NODE_FLAG_HLINE  0x0400
#define EQ_NODE_FLAG_VLINE  0x0800


typedef int16_t LineSegIndex;

#ifdef __cplusplus
struct Point
{
	Point()
	{
	}
	
	Point(uint8_t x, uint8_t y)
	{
		m_x = x;
		m_y = y;
	}
	
	bool equals(const Point &p) const
	{
		return p.m_x==m_x && p.m_y==m_y;
	}
	
	uint16_t dist2(const Point &p) const
	{
		int16_t diffx, diffy;
		
		diffx = p.m_x - m_x;
		diffy = p.m_y - m_y;
		
		return diffx*diffx + diffy*diffy;
	}
	
	void avg(const Point &p)
	{
		m_x = (m_x+p.m_x)>>1;
		m_y = (m_y+p.m_y)>>1;
	}
	
	uint8_t m_x;
	uint8_t m_y;
};

struct LineSeg;

struct LineSeg
{
	void reset()
	{
		m_ls0 = m_ls1 = -1;
		m_line = 0;
	}
		
	uint16_t length2() const
	{
		int16_t diffx, diffy;
		
		diffx = m_p1.m_x - m_p0.m_x;
		diffy = m_p1.m_y - m_p0.m_y;
		
		return diffx*diffx + diffy*diffy;
	}

	Point m_p0; 
	Point m_p1;
	LineSegIndex m_ls0;
	LineSegIndex m_ls1;
	uint8_t m_line;
};

#endif


struct EqueueFields
{
    volatile uint16_t readIndex;
    volatile uint16_t writeIndex;

    volatile uint16_t produced;
    volatile uint16_t consumed;

    // (array size below doesn't matter-- we're just going to cast a pointer to this struct)
    uint16_t data[1]; // data
};

#ifdef __cplusplus  // M4 is C++ and the "consumer" of data

class Equeue
{
public:
	Equeue();
	~Equeue();

	void reset();
	uint32_t dequeue(uint16_t *val);
	uint32_t queued()
	{
		return m_fields->produced - m_fields->consumed;
	}

	uint32_t readLine(uint16_t *mem, uint32_t size, bool *eof, bool *error);
	void flush();

	EqueueFields *m_fields;
};

#else //  M0 is C and the "producer" of data (Qvals)

uint32_t eq_enqueue(uint16_t val);
uint16_t eq_free(void);

extern struct EqueueFields *g_equeue;

#endif

#endif
