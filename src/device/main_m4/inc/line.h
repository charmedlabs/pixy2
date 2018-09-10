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

#ifndef _LINE_H
#define _LINE_H

#include "equeue.h"
#include "simplelist.h"
#include "tracker.h"

#define LINE_EDGE_DIST_DEFAULT            4
#define LINE_EDGE_THRESH_DEFAULT          35
#define LINE_EXTRACTION_DIST_DEFAULT      13
#define LINE_MAX_MERGE_DIST               6
#define LINE_MIN_LENGTH                   10
#define LINE_MIN_WIDTH                    0
#define LINE_MAX_WIDTH                    100
#define LINE_LINES_PER_COL_PITCH          (LINE_LINES_PER_COL+1) // +1 because of length byte at the beginning
#define LINE_VSIZE			              (CAM_RES3_WIDTH/4)
#define LINE_MAX_COMPARE                  5000

#define LINE_NODE_FLAG_V                  EQ_NODE_FLAG_V

#define LINE_HTHRESH_RATIO	              3/5

#define LINE_GRID_WIDTH_REDUCTION         3
#define LINE_GRID_HEIGHT_REDUCTION        1
#define LINE_GRID_WIDTH                   (CAM_RES3_WIDTH>>LINE_GRID_WIDTH_REDUCTION)
#define LINE_GRID_HEIGHT                  (CAM_RES3_HEIGHT>>LINE_GRID_HEIGHT_REDUCTION)

#define LINE_GRID_INDEX(x, y)             (LINE_GRID_WIDTH*(y)+(x))
#define LINE_GRID_INDEX_P(p)              LINE_GRID_INDEX(p.m_x, p.m_y)
#define LINE_GRID(x, y)                   g_lineGrid[LINE_GRID_INDEX(x, y)]
#define LINE_GRID_P(p)                    LINE_GRID(p.m_x, p.m_y)
#define LINE_GRID_LINE(x, y)              (LINE_GRID(x, y)&LINE_NODE_LINE_MASK)
#define LINE_GRID_LINE_P(p)               (LINE_GRID_P(p)&LINE_NODE_LINE_MASK)


#define LINE_NODE_LINE_MASK               0x03ff
#define LINE_NODE_FLAG_HLINE              EQ_NODE_FLAG_HLINE
#define LINE_NODE_FLAG_VLINE              EQ_NODE_FLAG_VLINE
#define LINE_NODE_FLAG_1                  (LINE_NODE_FLAG_HLINE | LINE_NODE_FLAG_VLINE)
#define LINE_NODE_FLAG_NULL               0x8000
#define LINE_MAX_SEGMENTS                 0x100
#define LINE_MAX_LINES                    0x80
#define LINE_MAX_SEGMENT_POINTS           32
#define LINE_MAX_INTERSECTION_LINES       8 // needs to be an even number
#define LINE_MAX_FRAME_INTERSECTION_LINES 6 // needs to be an even numbe

#define LINE_DEBUG_BENCHMARK              1  // not bit
#define LINE_DEBUG_LAYERS                 2  // bit
#define LINE_DEBUG_GRAPH_CHECK            4  // bit
#define LINE_DEBUG_TRACKING               8 // bit

#define	LINE_RM_MINIMAL                   0
#define	LINE_RM_MINIMAL_STR               "Primary features, no backgound"  
#define	LINE_RM_PRIMARY_FEATURES          1
#define	LINE_RM_PRIMARY_FEATURES_STR      "Primary features"
#define	LINE_RM_ALL_FEATURES              2
#define	LINE_RM_ALL_FEATURES_STR          "All features" 


#define LINE_MMC_BITS                     4
#define LINE_MMC_MIN_EDGES                (4+LINE_MMC_BITS)
#define LINE_MMC_MAX_EDGES                (2+(LINE_MMC_BITS*2))

#define LINE_MMC_CANDIDATE_BARCODES       32
#define LINE_MMC_VOTED_BARCODES           8
#define LINE_MMC_VTSIZE                   8  // voting table size
#define LINE_MMC_VBOUNDARY                0.25
#define LINE_MMC_HBOUNDARY                0.1

#define LINE_MIN_ACQUISITION_TAN_ANGLE    500
#define LINE_MIN_ACQUISITION_LENGTH2      15*15

// define 2 bits to describe which of the two points we are referring to on a line -- is it the upper-most point?
// or the right-most point?
#define LINE_HT_UP                        0x01
#define LINE_HT_DOWN                      0x02
#define LINE_HT_LEFT                      0x04
#define LINE_HT_RIGHT                     0x08

#define LINE_FILTERING_MULTIPLIER         16

#define LINE_LINE_FILTERING				  1
#define LINE_INTERSECTION_FILTERING       1
#define LINE_BARCODE_FILTERING            1

#define LINE_MIN_INTERSECTION_DETECT      (LINE_GRID_HEIGHT/4) 

#define LINE_FR_VECTOR_LINES                 0x01
#define LINE_FR_INTERSECTION                 0x02
#define LINE_FR_BARCODE                      0x04

#define LINE_FR_FLAG_INTERSECTION            0x04

#define LINE_MODEMAP_TURN_DELAYED            0x01
#define LINE_MODEMAP_MANUAL_SELECT_VECTOR    0x02
#define LINE_MODEMAP_WHITE_LINE              0x80

enum LineState
{
	LINE_STATE_ACQUIRING, 
	LINE_STATE_TRACKING
};

struct BarCodeCluster
{
    BarCodeCluster()
    {
        m_n = 0;
    }

    void addCode(uint8_t index)
    {
        if (m_n>=LINE_MMC_CANDIDATE_BARCODES)
            return;
        m_indexes[m_n] = index;
        m_n++;
    }

    void updateWidth(uint16_t width)
    {
        m_width = (m_width*(m_n-1) + width)/m_n; // recursive averager
    }

    uint8_t m_indexes[LINE_MMC_CANDIDATE_BARCODES];
    uint8_t m_n;
    Point16 m_p0;
    Point16 m_p1;
    uint16_t m_width;
};

struct BarCode
{
    BarCode()
    {
        m_n = 0;
    }

    Point16 m_p0;
    uint16_t m_width;
    int16_t m_val;
    uint16_t m_edges[LINE_MMC_MAX_EDGES];
    uint8_t m_n;
};


struct DecodedBarCode
{
    RectA m_outline;
    int16_t m_val;
	Tracker<DecodedBarCode> *m_tracker;
};


typedef uint16_t LineGridNode;

extern LineGridNode *g_lineGrid;

struct Nadir
{
	Nadir()
	{
		m_n = 0;
		m_dist = 0;
	}
	
	void merge(const Nadir &n)
	{
		uint16_t xavg, yavg;
		uint8_t i, j, li, lj;
		
		// look through list, make sure there are no duplicate lines
		for (i=0; i<n.m_n; i++)
		{
			if (m_n>=LINE_MAX_INTERSECTION_LINES)
				return;
			li = LINE_GRID_LINE_P(n.m_points[i]);
			for (j=0; j<m_n; j++)
			{
				lj = LINE_GRID_LINE_P(m_points[j]);
				if (li==lj)
					break;
			}
			if (j==m_n) // we reached end of list
				m_points[m_n++] = n.m_points[i];
		}
		// average all points
		for (i=xavg=yavg=0; i<m_n; i++)
		{
			xavg += m_points[i].m_x;
			yavg += m_points[i].m_y;
		}
		
		m_pavg.m_x = (xavg+(m_n>>1))/m_n;
		m_pavg.m_y = (yavg+(m_n>>1))/m_n;
		
	}

	Point m_points[LINE_MAX_INTERSECTION_LINES];
	Point m_pavg;
	uint8_t m_n;
	uint16_t m_dist;
};


struct Intersection;

struct Line2
{
	Line2()
	{
		m_i0 = m_i1 = NULL;
		m_tracker = NULL;
		m_index = 0;
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
	uint8_t m_index;
	SimpleListNode<Intersection> *m_i0;
	SimpleListNode<Intersection> *m_i1;
	Tracker<Line2> *m_tracker;
};


struct Intersection
{
	Intersection()
	{
	}

	Intersection(const Point &p)
	{
		m_p = p;
	}
	
	bool addLine(SimpleListNode<Line2> *linen, SimpleListNode<Intersection> *intern, uint8_t pi)
	{
		uint8_t i;
		
		if (linen==NULL || intern==NULL || m_n>=LINE_MAX_INTERSECTION_LINES)
			return false;
		for (i=0; i<m_n; i++)
		{
			if (m_lines[i]==linen)
				return false;
		}
		m_lines[m_n++] = linen;
		if (pi==0)
			linen->m_object.m_i0 = intern;
		else
			linen->m_object.m_i1 = intern;
		return true;
	}
	
	Point m_p; 
	uint8_t m_n;
	SimpleListNode<Line2> *m_lines[LINE_MAX_INTERSECTION_LINES];
};

struct FrameIntersectionLine
{
	uint8_t m_index;
	uint8_t m_reserved;
	int16_t m_angle;
};

struct FrameIntersection
{
	uint8_t m_x;
	uint8_t m_y;
	uint8_t m_n;
	uint8_t m_reserved;
	FrameIntersectionLine m_lines[LINE_MAX_FRAME_INTERSECTION_LINES];
};

struct FrameLine
{
	uint8_t m_x0;
	uint8_t m_y0;
	uint8_t m_x1;
	uint8_t m_y1;
	uint8_t m_index;
	uint8_t m_flags;
};

struct FrameCode
{
	uint8_t m_x;
	uint8_t m_y;
	uint8_t m_flags;
	uint8_t m_code;
};



int line_init(Chirp *chirp);
int line_open(int8_t progIndex);
int line_loadParams(int8_t progIndex);
void line_close();
int line_process();
int line_setRenderMode(uint8_t mode);
uint8_t line_getRenderMode();
int line_getPrimaryFrame(uint8_t typeMap, uint8_t *buf, uint16_t len);
int line_getAllFrame(uint8_t typeMap, uint8_t *buf, uint16_t len);
int line_setMode(int8_t mode);
int line_setNextTurnAngle(int16_t angle);
int line_setDefaultTurnAngle(int16_t angle);
int line_setVector(uint8_t index);
int line_reversePrimary();
int line_legoLineData(uint8_t *buf, uint32_t buflen);

int32_t line_streamEdgesLines(const uint8_t &bitmap);

#endif
