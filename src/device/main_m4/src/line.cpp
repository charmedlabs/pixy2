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

#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <debug.h>
#include <string.h>
#include <math.h>
#include "debug.h"
#include "calc.h"
#include "pixy_init.h"
#include "camera.h"
#include "cameravals.h"
#include "smlink.hpp"
#include "param.h"
#include "line.h"
#include "led.h"
#include "exec.h"
#include "misc.h"
#include "calc.h"
#include "simplelist.h"

#define ABS(x)      ((x)<0 ? -(x) : (x))
#define SIGN(x)     ((x)>=0 ? 1 : -1)

#define LINE_BUFSIZE    CAM_RES3_WIDTH/2

Equeue *g_equeue;

static uint16_t *g_lineBuf;

static uint16_t g_minLineWidth;
static uint16_t g_maxLineWidth;
static uint32_t g_minLineLength2; // squared
static uint32_t g_minLineLength; // not squared
static uint16_t g_maxMergeDist; 

static uint16_t g_extractionDist;

static EqueueFields g_savedEqueue;

static uint16_t g_dist;
static uint16_t g_thresh;
static uint16_t g_hThresh;
static uint8_t g_whiteLine;
static uint8_t g_go;
static uint8_t g_repeat;
static ChirpProc g_getEdgesM0 = -1;
static ChirpProc g_setEdgeParamsM0 = -1;

LineGridNode *g_lineGrid;
static uint8_t *g_lineGridMem;
static LineSeg *g_lineSegs;
static uint8_t *g_lineSegsMem;
static uint8_t g_pointsPerSeg;
static LineSegIndex g_lineSegIndex;
static float g_maxError;
static uint8_t g_lineIndex;
static SimpleListNode<Line2> **g_lines;
static uint32_t g_maxSegTanAngle;
static uint32_t g_maxEquivTanAngle;
static uint32_t g_maxTrackingTanAngle;

static SimpleList<Line2> g_linesList;
static SimpleList<Point> g_nodesList;
static SimpleList<Nadir> g_nadirsList;
static SimpleList<Intersection> g_intersectionsList;

static uint8_t g_barcodeIndex;
static BarCode **g_candidateBarcodes;
static DecodedBarCode *g_votedBarcodes;
static uint8_t *g_votedBarcodesMem;
static uint8_t g_votedBarcodeIndex;
static uint16_t g_maxCodeDist;
static uint16_t g_minVotingThreshold;

static SimpleList<Tracker<Line2> > g_lineTrackersList;
static Tracker<FrameIntersection> g_primaryIntersection;
static bool g_newIntersection;

static SimpleList<Tracker<DecodedBarCode> > g_barCodeTrackersList;
static uint8_t g_barCodeTrackerIndex;
static uint8_t g_lineTrackerIndex;
static uint8_t g_primaryLineIndex;
static uint8_t g_primaryPointMap;
static Point g_goalPoint;
static Point g_primaryPoint;
static bool g_primaryActive;
static uint32_t g_maxLineCompare;

static LineState g_lineState;

static uint8_t g_renderMode;

static uint8_t g_lineFiltering;
static uint8_t g_barcodeFiltering;

static bool g_frameFlag;
static bool g_primaryMutex;
static bool g_allMutex;

static int16_t g_nextTurnAngle;
static int16_t g_defaultTurnAngle;
static uint8_t g_delayedTurn;
static bool g_newTurnAngle;
static bool g_manualVectorSelect;
static uint8_t g_manualVectorSelecIndextActive;
static uint8_t g_manualVectorSelectIndex;
static bool g_reversePrimary;

bool checkGraph(int val, uint8_t suppress0=0, uint8_t suppress1=0, SimpleListNode<Intersection> *intern=NULL);

Line2 *findLine(uint8_t index);
Line2 *findTrackedLine(uint8_t index);
uint8_t trackedLinesWithPoint(const Point &p);

void line_shadowCallback(const char *id, const uint16_t &val);

uint32_t tanDiffAbs1000(const Point &p00, const Point &p01, const Point &p10, const Point &p11, bool min=false)
{
	// find tangent of angle difference between the two lines
	int16_t xdiff0, ydiff0;
	int16_t xdiff1, ydiff1;
	int32_t x, y, res;

	xdiff0 = p01.m_x - p00.m_x;
	ydiff0 = p01.m_y - p00.m_y;
	xdiff1 = p11.m_x - p10.m_x;
	ydiff1 = p11.m_y - p10.m_y;

	x = xdiff0*xdiff1 + ydiff0*ydiff1;
	y = ydiff0*xdiff1 - ydiff1*xdiff0;

	// min==true says that we don't care how the lines stack up, just return the abs of the minimum.
	// min==false says that we care of the angles open beyond 90 degrees, in which case return a big value.
	if ((min&&x==0) || (!min&&x<=0))
		return 1000000;

	res = y*1000/x;
	res = ABS(res);
	return res;
}

uint32_t tanAbs1000(const Point &p0, const Point &p1)
{
	// find tangent of angle difference between the two lines
	int16_t xdiff, ydiff;
	int32_t res;

	xdiff = p1.m_x - p0.m_x;
	ydiff = p1.m_y - p0.m_y;

	if (xdiff==0)
		return 1000000;

	res = ydiff*1000/xdiff;
	res = ABS(res);
	return res;
}

bool xdirection(const Point &p0, const Point &p1)
{
    int16_t xdiff, ydiff;

    xdiff = p1.m_x - p0.m_x;
    ydiff = p1.m_y - p0.m_y;

    return abs(xdiff)>abs(ydiff);
}

static const ProcModule g_module[] =
{
	END
};

void line_shadowCallback(const char *id, const void *val)
{
	int responseInt;
	bool callM0 = false;
	uint16_t leading, trailing;
	
	if (strcmp(id, "Edge distance")==0)
	{
		g_dist = *(uint16_t *)val;
		callM0 = true;
	}
	else if (strcmp(id, "Edge threshold")==0)
	{
		g_thresh = *(uint16_t *)val;
		g_hThresh = g_thresh*LINE_HTHRESH_RATIO;	
		callM0 = true;
	}
	else if (strcmp(id, "Minimum line width")==0)
		g_minLineWidth = *(uint16_t *)val;
	else if (strcmp(id, "Maximum line width")==0)
		g_maxLineWidth = *(uint16_t *)val;
	else if (strcmp(id, "Line extraction distance")==0)
		g_extractionDist = *(uint16_t *)val;
	else if (strcmp(id, "Maximum merge distance")==0)
		g_maxMergeDist = *(uint16_t *)val;
	else if (strcmp(id, "Minimum line length")==0)
	{
		g_minLineLength = *(uint16_t *)val;
		g_minLineLength2 = g_minLineLength*g_minLineLength; // squared
	}
	else if (strcmp(id, "Maximum line compare")==0)
		g_maxLineCompare = *(uint32_t *)val; 
	else if (strcmp(id, "White line")==0)
		g_whiteLine = *(uint8_t *)val;
	else if (strcmp(id, "Manual vector select")==0)
		g_manualVectorSelect = *(uint8_t *)val;
	else if (strcmp(id, "Line filtering")==0)
		g_lineFiltering = *(uint8_t *)val;
	else if (strcmp(id, "Intersection filtering")==0)
	{
		uint8_t v;
		v = *(uint8_t *)val;
		leading = v*LINE_FILTERING_MULTIPLIER;
		trailing = (leading+1)>>1;
		g_primaryIntersection.setTiming(leading, trailing); 
	}
	else if (strcmp(id, "Barcode filtering")==0)
		g_barcodeFiltering = *(uint8_t *)val;
	else if (strcmp(id, "Delayed turn")==0)
		g_delayedTurn = *(uint8_t *)val;
	else if (strcmp(id, "Go")==0)
		g_go = *(uint8_t *)val;
	else if (strcmp(id, "Repeat")==0)
		g_repeat = *(uint8_t *)val;

	if (callM0)
		g_chirpM0->callSync(g_setEdgeParamsM0, UINT16(g_dist), UINT16(g_thresh), UINT16(g_hThresh), END_OUT_ARGS, &responseInt, END_IN_ARGS);
}



int line_loadParams(int8_t progIndex)
{	
	int i, responseInt=-1;
	char id[32], desc[128];
	uint16_t leading, trailing;
	
	// add params
	if (progIndex>=0)
	{
		prm_add("Edge distance", PROG_FLAGS(progIndex) | PRM_FLAG_SLIDER, PRM_PRIORITY_4,
			"@c Expert @m 1 @M 15 Sets the distance between pixels when computing edges (default " STRINGIFY(LINE_EDGE_DIST_DEFAULT) ")", UINT16(LINE_EDGE_DIST_DEFAULT), END);
		prm_setShadowCallback("Edge distance", (ShadowCallback)line_shadowCallback);

		prm_add("Edge threshold", PROG_FLAGS(progIndex) | PRM_FLAG_SLIDER, PRM_PRIORITY_5,
			"@c Tuning @m 1 @M 150 Sets edge detection threshold (default " STRINGIFY(LINE_EDGE_THRESH_DEFAULT) ")", UINT16(LINE_EDGE_THRESH_DEFAULT), END);
		prm_setShadowCallback("Edge threshold", (ShadowCallback)line_shadowCallback);

		prm_add("Minimum line width", PROG_FLAGS(progIndex) | PRM_FLAG_SLIDER, PRM_PRIORITY_5, 
			"@c Tuning @m 0 @M 100 Sets minimum detected line width " STRINGIFY(LINE_MIN_WIDTH) ")", UINT16(LINE_MIN_WIDTH), END);
		prm_setShadowCallback("Minimum line width", (ShadowCallback)line_shadowCallback);

		prm_add("Maximum line width", PROG_FLAGS(progIndex) | PRM_FLAG_SLIDER, PRM_PRIORITY_5, 
			"@c Tuning @m 1 @M 250 Sets maximum detected line width " STRINGIFY(LINE_MAX_WIDTH) ")", UINT16(LINE_MAX_WIDTH), END);
		prm_setShadowCallback("Maximum line width", (ShadowCallback)line_shadowCallback);	

		prm_add("Line extraction distance", PROG_FLAGS(progIndex) | PRM_FLAG_SLIDER, PRM_PRIORITY_4,
			"@c Expert @m 1 @M 25 Sets the distance to search when extracting lines (default " STRINGIFY(LINE_EXTRACTION_DIST_DEFAULT) ")", UINT16(LINE_EXTRACTION_DIST_DEFAULT), END);
		prm_setShadowCallback("Line extraction distance", (ShadowCallback)line_shadowCallback);

		prm_add("Maximum merge distance", PROG_FLAGS(progIndex) | PRM_FLAG_SLIDER, PRM_PRIORITY_4, 
			"@c Expert @m 0 @M 25 Sets the search distance for merging lines (default " STRINGIFY(LINE_MAX_MERGE_DIST) ")", UINT16(LINE_MAX_MERGE_DIST), END);
		prm_setShadowCallback("Maximum merge distance", (ShadowCallback)line_shadowCallback);

		prm_add("Minimum line length", PROG_FLAGS(progIndex) | PRM_FLAG_SLIDER, PRM_PRIORITY_4, 
			"@c Expert @m 1 @M 50 Sets the minimum line length (default " STRINGIFY(LINE_MIN_LENGTH) ")", UINT16(LINE_MIN_LENGTH), END);
		prm_setShadowCallback("Minimum line length", (ShadowCallback)line_shadowCallback);

		prm_add("Maximum line compare", PROG_FLAGS(progIndex) | PRM_FLAG_SLIDER, PRM_PRIORITY_4, 
			"@c Expert @m 1 @M 10000 Sets the maximum distance between lines for them to be considered the same line between frames (default " STRINGIFY(LINE_MAX_COMPARE) ")", UINT32(LINE_MAX_COMPARE), END);
		prm_setShadowCallback("Maximum line compare", (ShadowCallback)line_shadowCallback);

		prm_add("White line", PROG_FLAGS(progIndex) | PRM_FLAG_CHECKBOX, PRM_PRIORITY_5, 
			"@c Tuning If this is set, Pixy will look for light lines on dark background.  If this is not set, Pixy will look for dark lines on light background (default false)", UINT8(0), END);
		prm_setShadowCallback("White line", (ShadowCallback)line_shadowCallback);

		prm_add("Line filtering", PROG_FLAGS(progIndex) | PRM_FLAG_SLIDER, PRM_PRIORITY_4-1, 
			"@c Expert @m 0 @M 30 Sets the amount of noise filtering for line detection (default " STRINGIFY(LINE_LINE_FILTERING) ")", UINT8(LINE_LINE_FILTERING), END);
		prm_setShadowCallback("Line filtering", (ShadowCallback)line_shadowCallback);

		prm_add("Intersection filtering", PROG_FLAGS(progIndex) | PRM_FLAG_SLIDER, PRM_PRIORITY_4-2, 
			"@c Expert @m 0 @M 30 Sets the amount of noise filtering for intersection detection (default " STRINGIFY(LINE_INTERSECTION_FILTERING) ")", UINT8(LINE_INTERSECTION_FILTERING), END);
		prm_setShadowCallback("Intersection filtering", (ShadowCallback)line_shadowCallback);

		prm_add("Barcode filtering", PROG_FLAGS(progIndex) | PRM_FLAG_SLIDER, PRM_PRIORITY_4-3, 
			"@c Expert @m 0 @M 30 Sets the amount of noise filtering for barcode detection (default " STRINGIFY(LINE_BARCODE_FILTERING) ")", UINT8(LINE_BARCODE_FILTERING), END);
		prm_setShadowCallback("Barcode filtering", (ShadowCallback)line_shadowCallback);

		prm_add("Default turn angle", PROG_FLAGS(progIndex) | PRM_FLAG_SIGNED, PRM_PRIORITY_4-4, 
			"@c Expert Sets the turn angle that Pixy chooses by default if next turn angle is not set.  Does not apply if Delayed turn is set (default 0)", UINT16(0), END);
		
		prm_add("Delayed turn", PROG_FLAGS(progIndex) | PRM_FLAG_CHECKBOX, PRM_PRIORITY_4-5, 
			"@c Expert If true, Pixy will wait for client to choose turn direction after detecting intersection, otherwise Pixy will choose default turn angle or next turn angle after detecting intersection. (default false)", UINT8(0), END);
		prm_setShadowCallback("Delayed turn", (ShadowCallback)line_shadowCallback);

		prm_add("Manual vector select", PROG_FLAGS(progIndex) | PRM_FLAG_CHECKBOX, PRM_PRIORITY_4-6, 
			"@c Expert If false, Pixy will automatically choose the primary vector for tracking. If true, the user selects the primary vectory by calling SelectVector (default false)", UINT8(0), END);
		prm_setShadowCallback("Manual vector select", (ShadowCallback)line_shadowCallback);

		prm_add("Go", PROG_FLAGS(progIndex) | PRM_FLAG_CHECKBOX  
			| PRM_FLAG_INTERNAL, 
			PRM_PRIORITY_4,
			"@c Expert Debug flag. (default false)", UINT8(0), END);
		prm_setShadowCallback("Go", (ShadowCallback)line_shadowCallback);

		prm_add("Repeat", PROG_FLAGS(progIndex) | PRM_FLAG_CHECKBOX 
			| PRM_FLAG_INTERNAL, 
			PRM_PRIORITY_4,
			"@c Expert Debug flag. (default false)", UINT8(0), END);
		prm_setShadowCallback("Repeat", (ShadowCallback)line_shadowCallback);
	
		for (i=0; i<16; i++)
		{
			sprintf(id, "Barcode label %d", i);
			sprintf(desc, "@c Barcode_Labels Sets the label for barcodes that match barcode pattern %d.", i);
			prm_add(id, PROG_FLAGS(progIndex), PRM_PRIORITY_3-i, desc, STRING(""), END);
		}
	}
	
	// load params
	prm_get("Edge distance", &g_dist, END);	
	prm_get("Edge threshold", &g_thresh, END);	
	g_hThresh = g_thresh*LINE_HTHRESH_RATIO;
	prm_get("Minimum line width", &g_minLineWidth, END);
	prm_get("Maximum line width", &g_maxLineWidth, END);
	prm_get("Line extraction distance", &g_extractionDist, END);
	prm_get("Maximum merge distance", &g_maxMergeDist, END);
	prm_get("Minimum line length", &g_minLineLength, END);
	g_minLineLength2 = g_minLineLength*g_minLineLength; // square it 
	prm_get("Maximum line compare", &g_maxLineCompare, END);
	prm_get("White line", &g_whiteLine, END);
	prm_get("Intersection filtering", &g_lineFiltering, END);
	leading = g_lineFiltering*LINE_FILTERING_MULTIPLIER;
	trailing = (leading+1)>>1;
	g_primaryIntersection.setTiming(leading, trailing); 
	prm_get("Line filtering", &g_lineFiltering, END);
	prm_get("Barcode filtering", &g_barcodeFiltering, END);
	prm_get("Default turn angle", &g_defaultTurnAngle, END);
	prm_get("Delayed turn", &g_delayedTurn, END);
	prm_get("Manual vector select", &g_manualVectorSelect, END);
	prm_get("Go", &g_go, END);	
	prm_get("Repeat", &g_repeat, END);	
	
	g_chirpM0->callSync(g_setEdgeParamsM0, UINT16(g_dist), UINT16(g_thresh), UINT16(g_hThresh), END_OUT_ARGS, &responseInt, END_IN_ARGS);
	
	return responseInt;
}

int line_init(Chirp *chirp)
{		
	chirp->registerModule(g_module);	

	g_getEdgesM0 = g_chirpM0->getProc("getEdges", NULL);
	g_setEdgeParamsM0 = g_chirpM0->getProc("setEdgeParams", NULL);
	
	if (g_getEdgesM0<0 || g_setEdgeParamsM0<0)
		return -1;

	return 0;
}

int line_open(int8_t progIndex)
{
	g_linesList.clear();
	g_nodesList.clear();
	g_nadirsList.clear();
	g_intersectionsList.clear();
	g_lineTrackersList.clear();
	g_barCodeTrackersList.clear();
	
	g_lineGridMem = (uint8_t *)malloc(LINE_GRID_WIDTH*LINE_GRID_HEIGHT*sizeof(LineGridNode)+CAM_PREBUF_LEN+8); // +8 for extra memory at the end because little overruns sometimes happen
	g_lineGrid = (LineGridNode *)(g_lineGridMem+CAM_PREBUF_LEN);
	
	g_lines = (SimpleListNode<Line2> **)malloc(LINE_MAX_LINES*sizeof(SimpleListNode<Line2> *));
	g_lineSegsMem = (uint8_t *)malloc(LINE_MAX_SEGMENTS*sizeof(LineSeg)+CAM_PREBUF_LEN);
	g_lineSegs = (LineSeg *)(g_lineSegsMem+CAM_PREBUF_LEN);
	
	g_candidateBarcodes = (BarCode **)malloc(LINE_MMC_CANDIDATE_BARCODES*sizeof(BarCode *));
	
	g_votedBarcodesMem = (uint8_t *)malloc(LINE_MMC_VOTED_BARCODES*sizeof(DecodedBarCode)+CAM_PREBUF_LEN);
	g_votedBarcodes = (DecodedBarCode *)(g_votedBarcodesMem+CAM_PREBUF_LEN);
	
	g_lineBuf = (uint16_t *)malloc(LINE_BUFSIZE*sizeof(uint16_t)); 
	g_equeue = new (std::nothrow) Equeue;

	g_maxSegTanAngle = tan(M_PI/4)*1000;
	g_maxEquivTanAngle = tan(M_PI/10)*1000;
	g_maxTrackingTanAngle = tan(M_PI/10)*1000;
	g_pointsPerSeg = 12;
	g_maxError = 0.9;
	
	g_barcodeIndex = 0;
	g_votedBarcodeIndex	 = 0;
	g_maxCodeDist = 15*15;
	g_minVotingThreshold = 128; // divide by 256, so 128 is 1/2
	g_barCodeTrackerIndex = 0;
	g_lineTrackerIndex = 0;
	
	g_lineState = LINE_STATE_ACQUIRING;
	g_primaryActive = false;
	g_newIntersection = false;
	g_delayedTurn = false;
	g_defaultTurnAngle = 0;
	g_nextTurnAngle = 0;
	g_newTurnAngle = false;
	g_manualVectorSelect = false;
	g_manualVectorSelecIndextActive = false;

	g_reversePrimary = false;
	
	g_renderMode = LINE_RM_ALL_FEATURES;
	
	if (g_equeue==NULL || g_lineBuf==NULL || g_lineGridMem==NULL || g_lineSegsMem==NULL || 
		g_lines==NULL || g_candidateBarcodes==NULL || g_votedBarcodesMem==NULL)
	{
		cprintf(0, "Line memory error\n");
		line_close();
		return -1;
	}
	
	g_repeat = 0;
	
	g_frameFlag = false;
	
	return line_loadParams(progIndex);
}

void line_close()
{
	if (g_equeue)
		delete g_equeue;
	if (g_lineBuf)
		free(g_lineBuf);
	if (g_lineGridMem)
		free(g_lineGridMem);
	if (g_lineSegsMem)
		free(g_lineSegsMem);
	if (g_lines)
		free(g_lines);
	if (g_candidateBarcodes)
		free(g_candidateBarcodes);
	if (g_votedBarcodesMem)
		free(g_votedBarcodesMem);
	g_linesList.clear();
	g_nodesList.clear();
	g_nadirsList.clear();
	g_intersectionsList.clear();
	g_lineTrackersList.clear();
	g_barCodeTrackersList.clear();
}

int32_t line_getEdges()
{
	int32_t responseInt = -1;
	// forward call to M0, get edges
	g_chirpM0->callSync(g_getEdgesM0, UINT32(SRAM1_LOC+CAM_PREBUF_LEN), END_OUT_ARGS, &responseInt, END_IN_ARGS);
		
	return responseInt;
}


int line_hLine(uint8_t row, uint16_t *buf, uint32_t len)
{
	uint16_t j, index, bit0, bit1, col0, col1, lineWidth;

	// copy a lot of code to reduce branching, make it faster
	if (g_whiteLine) // pos neg
	{
		for (j=0; buf[j]<EQ_HSCAN_LINE_START && buf[j+1]<EQ_HSCAN_LINE_START && j<len; j++)
		{
			bit0 = buf[j]&EQ_NEGATIVE;
			bit1 = buf[j+1]&EQ_NEGATIVE;
			col0 = buf[j]&~EQ_NEGATIVE;
			col1 = buf[j+1]&~EQ_NEGATIVE;
			if (bit0==0 && bit1!=0)
			{
				lineWidth = col1 - col0;
				if (g_minLineWidth<lineWidth && lineWidth<g_maxLineWidth)
				{
					index = LINE_GRID_INDEX((((col0+col1)>>1) + g_dist)>>3, row>>1);
					if (index<LINE_GRID_WIDTH*LINE_GRID_HEIGHT+8)
						g_lineGrid[index] |= LINE_NODE_FLAG_HLINE;
					else
						cprintf(0, "high index\n");
				}
			}
		}
	}
	else // black line
	{
 		for (j=0; buf[j]<EQ_HSCAN_LINE_START && buf[j+1]<EQ_HSCAN_LINE_START && j<len; j++)
		{
			bit0 = buf[j]&EQ_NEGATIVE;
			bit1 = buf[j+1]&EQ_NEGATIVE;
			col0 = buf[j]&~EQ_NEGATIVE;
			col1 = buf[j+1]&~EQ_NEGATIVE;
			if (bit0!=0 && bit1==0)
			{
				lineWidth = col1 - col0;
				if (g_minLineWidth<lineWidth && lineWidth<g_maxLineWidth)
				{
					index = LINE_GRID_INDEX((((col0+col1)>>1) + g_dist)>>3, row>>1);
					if (index<LINE_GRID_WIDTH*LINE_GRID_HEIGHT+8)
						g_lineGrid[index] |= LINE_NODE_FLAG_HLINE;
					else
						cprintf(0, "high index\n");
				}
			}
		}
	}
	
	return 0;
}

int line_vLine(uint8_t row, uint8_t *vstate, uint16_t *buf, uint32_t len)
{
	uint16_t i, index, bit0, col0, lineWidth;

	if (g_whiteLine)
	{
		for (i=0; buf[i]<EQ_HSCAN_LINE_START && i<len; i++)
		{
			bit0 = buf[i]&EQ_NEGATIVE;
			col0 = (buf[i]&~EQ_NEGATIVE)>>2;
			if (bit0==0) // pos
				vstate[col0] = row+1;
			else // bit0!=0, neg
			{
				if (vstate[col0]!=0)
				{
					lineWidth = (row - (vstate[col0]-1))<<2; // multiply by 4 because vertical is subsampled by 4
					if (g_minLineWidth<lineWidth && lineWidth<g_maxLineWidth && col0<LINE_VSIZE)
					{
						index = LINE_GRID_INDEX(col0>>1, (row - (lineWidth>>3))>>1);
						if (index<LINE_GRID_WIDTH*LINE_GRID_HEIGHT+8)
							g_lineGrid[index] |= LINE_NODE_FLAG_VLINE;
						else
							cprintf(0, "high index\n");
					}
					vstate[col0] = 0;
				}
			}
		}
	}
	else // black line
	{
		for (i=0; buf[i]<EQ_HSCAN_LINE_START && i<len; i++)
		{
			bit0 = buf[i]&EQ_NEGATIVE;
			col0 = (buf[i]&~EQ_NEGATIVE)>>2;
			if (bit0!=0) // neg
				vstate[col0] = row+1;
			else // bit0==0, pos
			{
				if (vstate[col0]!=0)
				{
					lineWidth = (row - (vstate[col0]-1))<<2; // multiply by 4 because vertical is subsampled by 4
					if (g_minLineWidth<lineWidth && lineWidth<g_maxLineWidth && col0<LINE_VSIZE)
					{
						index = LINE_GRID_INDEX(col0>>1, (row - (lineWidth>>3))>>1);
						if (index<LINE_GRID_WIDTH*LINE_GRID_HEIGHT+8)
							g_lineGrid[index] |= LINE_NODE_FLAG_VLINE;
						else
							cprintf(0, "high index\n");
					}
					vstate[col0] = 0;
				}
			}
		}
	}
	return 0;
}

int line_sendLineGrid(uint8_t renderFlags)
{
	uint32_t len;
	uint8_t *gridData = (uint8_t *)g_lineGridMem + CAM_PREBUF_LEN - CAM_FRAME_HEADER_LEN;
	
	// fill buffer contents manually for return data 
	len = Chirp::serialize(g_chirpUsb, (uint8_t *)gridData, LINE_GRID_WIDTH*LINE_GRID_HEIGHT*sizeof(LineGridNode), HTYPE(FOURCC('L','I','N','G')), HINT8(renderFlags), UINT16(LINE_GRID_WIDTH), UINT16(LINE_GRID_HEIGHT), UINTS16_NO_COPY(LINE_GRID_WIDTH*LINE_GRID_HEIGHT), END);
	if (len!=CAM_FRAME_HEADER_LEN)
		return -1;
	
	g_chirpUsb->useBuffer((uint8_t *)gridData, CAM_FRAME_HEADER_LEN+LINE_GRID_WIDTH*LINE_GRID_HEIGHT*sizeof(LineGridNode)); 
	
	return 0;
}

float lineSegError(Point ps[], uint8_t points, uint8_t *maxIndex)
{
	int8_t i, xdiff, ydiff;
	float e, fe, maxe, error, slope;
	
	xdiff = ps[points-1].m_x - ps[0].m_x;
	ydiff = ps[points-1].m_y - ps[0].m_y;
	
	if (ABS(ydiff)>ABS(xdiff)) // vertical
	{
		slope = (float)xdiff/ydiff;
		for (i=1, error=0.0, maxe=-1.0; i<points; i++)
		{
			e = ps[i].m_x - (ps[0].m_x + (ps[i].m_y-ps[0].m_y)*slope);
			fe = fabsf(e);
			if (fe>maxe)
			{
				*maxIndex = i;
				maxe = fe;
			}
			error += e;
		}
	}
	else // horizontal
	{
		slope = (float)ydiff/xdiff;
		for (i=1, error=0.0, maxe=-1.0; i<points; i++)
		{
			e = ps[i].m_y - (ps[0].m_y + (ps[i].m_x-ps[0].m_x)*slope);
			fe = fabsf(e);
			if (fe>maxe)
			{
				*maxIndex = i;
				maxe = fe;
			}
			error += e;
		}
	}

	error = fabsf(error)/(points-1);
	
	return error;
}

	
void cleanGrid(Point ps[], uint8_t points)
{
	uint8_t i;
	int8_t xdiff, ydiff;
	int16_t j;
	
	// don't put bogus indexes in grid
	if (g_lineIndex>=LINE_MAX_LINES)
		return;
	
	xdiff = ps[points-1].m_x - ps[0].m_x;
	ydiff = ps[points-1].m_y - ps[0].m_y;
	
	if (ABS(ydiff)>ABS(xdiff)) // vertical
	{
		for (i=0; i<points-1; i++) // don't clean around the last point, otherwise we might not be able to pick up where we left off
		{
			j = LINE_GRID_INDEX_P(ps[i]);

			g_lineGrid[j] &= ~LINE_NODE_LINE_MASK;
			g_lineGrid[j] |= g_lineIndex;
			if (ps[i].m_x>0)
				g_lineGrid[j-1] |= LINE_NODE_FLAG_NULL;
			if (ps[i].m_x>1)
				g_lineGrid[j-2] |= LINE_NODE_FLAG_NULL;
			if (ps[i].m_x<LINE_GRID_WIDTH-1)
				g_lineGrid[j+1] |= LINE_NODE_FLAG_NULL;
			if (ps[i].m_x<LINE_GRID_WIDTH-2)
				g_lineGrid[j+2] |= LINE_NODE_FLAG_NULL;
		}
	}
	else // horizontal
	{
		for (i=0; i<points-1; i++) // don't clean around the last point, otherwise we might not be able to pick up where we left off
		{
			j = LINE_GRID_INDEX_P(ps[i]);
			
			g_lineGrid[j] &= ~LINE_NODE_LINE_MASK;
			g_lineGrid[j] |= g_lineIndex;
			if (ps[i].m_y>0)
				g_lineGrid[j-LINE_GRID_WIDTH] |= LINE_NODE_FLAG_NULL;
			if (ps[i].m_y>1)
				g_lineGrid[j-LINE_GRID_WIDTH-LINE_GRID_WIDTH] |= LINE_NODE_FLAG_NULL;
			if (ps[i].m_y<LINE_GRID_HEIGHT-1)
				g_lineGrid[j+LINE_GRID_WIDTH] |= LINE_NODE_FLAG_NULL;
			if (ps[i].m_y<LINE_GRID_HEIGHT-2)
				g_lineGrid[j+LINE_GRID_WIDTH+LINE_GRID_WIDTH] |= LINE_NODE_FLAG_NULL;
		}
	}
	// mark last point
	j = LINE_GRID_INDEX_P(ps[i]);
	g_lineGrid[j] &= ~LINE_NODE_LINE_MASK;
	g_lineGrid[j] |= g_lineIndex;
}

int addline(const Point &p0, const Point &p1)
{
	SimpleListNode<Line2> *linen;
	Line2 line;

	if (g_lineIndex>=LINE_MAX_LINES)
		return -2;
	
	line.m_p0 = p0;
	line.m_p1 = p1; 
	line.m_index = g_lineIndex;
	linen = g_linesList.add(line);
	if (linen==NULL)
		return -1;
	g_lines[g_lineIndex++] = linen;	
	g_nodesList.add(p0);
	g_nodesList.add(p1);		
	return 0;
}	

LineSegIndex finishGridNode(Point ps[], uint8_t points, LineSegIndex prevSeg, Point &p0)
{
	float error;
	LineSeg *ls0, *ls1;
	uint8_t maxIndex;
	
	if (points==0 || g_lineSegIndex>=LINE_MAX_SEGMENTS-1 || g_lineIndex>=LINE_MAX_LINES-1)
		return prevSeg; // return prevSeg to help logic in extractLines
	
	// check error of segment, split if necessary
	error = lineSegError(ps, points, &maxIndex);
	if (error<g_maxError || points<=5)
	{
		ls0 = &g_lineSegs[g_lineSegIndex++];
		ls0->reset();
		ls0->m_p0.m_x = ps[0].m_x;
		ls0->m_p0.m_y = ps[0].m_y;
		ls0->m_p1.m_x = ps[points-1].m_x;
		ls0->m_p1.m_y = ps[points-1].m_y;
		ls0->m_ls0 = prevSeg;
		ls0->m_line = g_lineIndex;
		cleanGrid(ps, points);
	}
	else // split 
	{		
		ls0 = &g_lineSegs[g_lineSegIndex++];
		ls0->reset();
		ls0->m_p0.m_x = ps[0].m_x;
		ls0->m_p0.m_y = ps[0].m_y;
		ls0->m_p1.m_x = ps[maxIndex].m_x;
		ls0->m_p1.m_y = ps[maxIndex].m_y;
		ls0->m_ls0 = prevSeg;
		ls0->m_ls1 = g_lineSegIndex;
		ls0->m_line = g_lineIndex;
		cleanGrid(ps, maxIndex+1);

		// split the line
		addline(p0, ps[maxIndex+1]);
		p0 = ps[maxIndex+1];
		
		ls1 = &g_lineSegs[g_lineSegIndex++];
		ls1->reset();
		ls1->m_p0.m_x = ps[maxIndex].m_x;
		ls1->m_p0.m_y = ps[maxIndex].m_y;
		ls1->m_p1.m_x = ps[points-1].m_x;
		ls1->m_p1.m_y = ps[points-1].m_y;
		ls1->m_ls0 = g_lineSegIndex-2;
		ls1->m_line = g_lineIndex;
		cleanGrid(ps+maxIndex+1, points-(maxIndex+1));
	}
	return g_lineSegIndex-1;
}

bool ydirUp(Point &p, uint16_t &i, uint8_t &points, Point ps[])
{
	if (p.m_y==0)
		return false;

	if (g_lineGrid[i-LINE_GRID_WIDTH]&LINE_NODE_FLAG_1 && !(g_lineGrid[i-LINE_GRID_WIDTH]&LINE_NODE_FLAG_NULL))
	{
		p.m_y--;
		ps[points] = p;
		i += -LINE_GRID_WIDTH;
		g_lineGrid[i] |= LINE_NODE_FLAG_NULL;
		points++;
		return true;
	}
	else if (p.m_x>0 && g_lineGrid[i-LINE_GRID_WIDTH-1]&LINE_NODE_FLAG_1 && !(g_lineGrid[i-LINE_GRID_WIDTH-1]&LINE_NODE_FLAG_NULL))
	{
		p.m_y--;
		p.m_x--;
		ps[points] = p;
		i += -LINE_GRID_WIDTH-1;
		g_lineGrid[i] |= LINE_NODE_FLAG_NULL;
		points++;
		return true;
	}
	else if (p.m_x<LINE_GRID_WIDTH-1 && g_lineGrid[i-LINE_GRID_WIDTH+1]&LINE_NODE_FLAG_1 && !(g_lineGrid[i-LINE_GRID_WIDTH+1]&LINE_NODE_FLAG_NULL))
	{
		p.m_y--;
		p.m_x++;
		ps[points] = p;
		i += -LINE_GRID_WIDTH+1;
		g_lineGrid[i] |= LINE_NODE_FLAG_NULL;
		points++;
		return true;
	}
	else // no points in the up direction
		return false;
}

bool xdirLeft(Point &p, uint16_t &i, uint8_t &points, Point ps[])
{
	if (p.m_x>0 && g_lineGrid[i-1]&LINE_NODE_FLAG_1 && !(g_lineGrid[i-1]&LINE_NODE_FLAG_NULL))
	{
		p.m_x--;
		ps[points] = p;
		i+= -1;
		g_lineGrid[i] |= LINE_NODE_FLAG_NULL;
		points++;
		return true;
	}
	else if (p.m_x>0 && p.m_y>0 && g_lineGrid[i-LINE_GRID_WIDTH-1]&LINE_NODE_FLAG_1 && !(g_lineGrid[i-LINE_GRID_WIDTH-1]&LINE_NODE_FLAG_NULL))
	{
		p.m_y--;
		p.m_x--;
		ps[points] = p;
		i += -LINE_GRID_WIDTH-1;
		g_lineGrid[i] |= LINE_NODE_FLAG_NULL;
		points++;
		return true;
	}
	else if (p.m_x>0 && p.m_y<LINE_GRID_HEIGHT-1 && g_lineGrid[i+LINE_GRID_WIDTH-1]&LINE_NODE_FLAG_1 && !(g_lineGrid[i+LINE_GRID_WIDTH-1]&LINE_NODE_FLAG_NULL))
	{
		p.m_y++;
		p.m_x--;
		ps[points] = p;
		i += LINE_GRID_WIDTH-1;
		g_lineGrid[i] |= LINE_NODE_FLAG_NULL;
		points++;
		return true;
	}
	else
		return false;
}

bool xdirRight(Point &p, uint16_t &i, uint8_t &points, Point ps[])
{
	if (p.m_x<LINE_GRID_WIDTH-1 && g_lineGrid[i+1]&LINE_NODE_FLAG_1 && !(g_lineGrid[i+1]&LINE_NODE_FLAG_NULL))
	{
		p.m_x++;
		ps[points] = p;
		i += 1;
		g_lineGrid[i] |= LINE_NODE_FLAG_NULL;
		points++;
		return true;
	}
	else if (p.m_x<LINE_GRID_WIDTH-1 && p.m_y>0 && g_lineGrid[i-LINE_GRID_WIDTH+1]&LINE_NODE_FLAG_1 && !(g_lineGrid[i-LINE_GRID_WIDTH+1]&LINE_NODE_FLAG_NULL))
	{
		p.m_y--;
		p.m_x++;
		ps[points] = p;
		i += -LINE_GRID_WIDTH+1;
		g_lineGrid[i] |= LINE_NODE_FLAG_NULL;
		points++;
		return true;
	}
	else if (p.m_x<LINE_GRID_WIDTH-1 && p.m_y<LINE_GRID_HEIGHT-1 && g_lineGrid[i+LINE_GRID_WIDTH+1]&LINE_NODE_FLAG_1 && !(g_lineGrid[i+LINE_GRID_WIDTH+1]&LINE_NODE_FLAG_NULL))
	{
		p.m_y++;
		p.m_x++;
		ps[points] = p;
		i += LINE_GRID_WIDTH+1;
		g_lineGrid[i] |= LINE_NODE_FLAG_NULL;
		points++;
		return true;
	}
	else
		return false;
}

void extractLineSegments(const Point &p)
{
	bool ydir = true;
	bool rdir = true;
	uint16_t i = LINE_GRID_INDEX_P(p);
	Point ps[LINE_MAX_SEGMENT_POINTS];
	uint8_t points = 0;
	LineSegIndex prevSeg = -1, thisSeg = -1;
	uint32_t angle;
	Point p2 = p; // current point	
	Point p0 = p; // first point of line
	
	// nullify current point
	ps[0] = p2;
	g_lineGrid[i] |= LINE_NODE_FLAG_NULL;
	points++;
		
	while(1)
	{
		if (ydir)
		{
			if (!ydirUp(p2, i, points, ps))
			{
				if (!xdirLeft(p2, i, points, ps))
				{
					if (!xdirRight(p2, i, points, ps))
						goto end;
					else
					{
						ydir = false;
						rdir = true;
					}
				}
				else
				{
					ydir = false;
					rdir = false;
				}				
			}
		}
		else if (!ydir && rdir)
		{
			if (!xdirRight(p2, i, points, ps))
			{
				if (!ydirUp(p2, i, points, ps))
					goto end;
				else
					ydir = true;
			}
		}
		else if (!ydir && !rdir)
		{
			if (!xdirLeft(p2, i, points, ps))
			{
				if (!ydirUp(p2, i, points, ps))
					goto end;
				else
					ydir = true;
			}
		}
		
		if (points>=g_pointsPerSeg)
		{
			thisSeg = finishGridNode(ps, points, prevSeg, p0);
			if (prevSeg>=0)
			{
				if (thisSeg-prevSeg>1) // deal with bisected segment 
				{
					g_lineSegs[prevSeg].m_ls1 = thisSeg-1; // link up prev segment with this one
					// look at angle between segments, add if it exceeds a threshold 
					angle = tanDiffAbs1000(g_lineSegs[prevSeg].m_p0, g_lineSegs[prevSeg].m_p1, g_lineSegs[thisSeg-1].m_p0, g_lineSegs[thisSeg-1].m_p1);
				}
				else
				{
					g_lineSegs[prevSeg].m_ls1 = thisSeg; // link up prev segment with this one
					// look at angle between segments, add if it exceeds a threshold 
					angle = tanDiffAbs1000(g_lineSegs[prevSeg].m_p0, g_lineSegs[prevSeg].m_p1, g_lineSegs[thisSeg].m_p0, g_lineSegs[thisSeg].m_p1);
				}
				if (angle>g_maxSegTanAngle)
				{
					// split the line here
					addline(p0, g_lineSegs[prevSeg].m_p1);
					if (g_lineIndex<LINE_MAX_LINES)
					{
						g_lineSegs[thisSeg].m_line = g_lineIndex; // set line seg line index to new index
						cleanGrid(ps, points); // set nodes to new line index
					}
					p0 = ps[0];
				}
			}
			prevSeg = thisSeg;
			points = 0;
		}
	}

	end:
	// should we use the current segment?
	// we should if there were previous segments or if the current segment is bigger than 2
	if (prevSeg>=0 || points>2)
	{		
		thisSeg = finishGridNode(ps, points, prevSeg, p0);	
		if (prevSeg>=0)
			g_lineSegs[prevSeg].m_ls1 = thisSeg;
		prevSeg = thisSeg;
	}
	
	// add line
	if (prevSeg>=0 && g_lineIndex<LINE_MAX_LINES)
	{
		if (addline(p0, g_lineSegs[prevSeg].m_p1)<0)
			return;
	}		
}

void extractLineSegments()
{
	int8_t i, j;
	uint16_t k;
	LineGridNode node;
	
	for (i=LINE_GRID_HEIGHT-1; i>=0; i--) // bottom-up
	{
		k = i*LINE_GRID_WIDTH;
		for (j=0; j<LINE_GRID_WIDTH; j++, k++)
		{
			node = g_lineGrid[k];
			if (node&LINE_NODE_FLAG_1 && !(node&LINE_NODE_FLAG_NULL))
				// we could do some analysis here to find the end of the continuous train of pixels, then asses which direction 
				// the line is headed, upper-right, upper-left if it's horizontal
				extractLineSegments(Point(j, i)); 
		}
	}
}


void addNadir(const Point &p0, const Point &p1)
{
	uint16_t dist;
	SimpleListNode<Nadir> *i;
	Nadir n;
	uint8_t i0, i1, is;
	Point pp0, pp1;
	
	// calc distance between points
	dist = p0.dist2(p1);
	
	// find lines
	i0 = LINE_GRID_LINE_P(p0);
	i1 = LINE_GRID_LINE_P(p1);
	
	if (i0==i1)
		return;
	if (i0<i1)
	{
		pp0 = p0;
		pp1 = p1;
	}
	else // swap
	{
		pp0 = p1;
		pp1 = p0;
		is = i0;
		i0 = i1;
		i1 = is;
	}
	
	// search nadir list for this pair, if it exists, and it's closer, replace it.
	for (i=g_nadirsList.m_first; i!=NULL; i=i->m_next)
	{
		if (i0==LINE_GRID_LINE_P(i->m_object.m_points[0]) && i1==LINE_GRID_LINE_P(i->m_object.m_points[1]))
		{
			if (dist < i->m_object.m_dist)
			{
				i->m_object.m_dist = dist;
				i->m_object.m_points[0] = pp0;
				i->m_object.m_points[1] = pp1;
				i->m_object.m_pavg = pp0;
				i->m_object.m_pavg.avg(pp1);
			}
			return;
		}
	}
			
	// if it isn't in the list, add it.
	n.m_points[0] = pp0;
	n.m_points[1] = pp1;
	n.m_n = 2;
	n.m_dist = dist;
	n.m_pavg = pp0;
	n.m_pavg.avg(pp1);
	g_nadirsList.add(n);
}

void search(const Point &p, uint8_t radius)
{
	int8_t i, r;
	uint16_t j, k;
	uint8_t i0, i1;
	
	j = LINE_GRID_INDEX(p.m_x, p.m_y);
	i0 = g_lineGrid[j]&LINE_NODE_LINE_MASK;
	
	// search up
	r = p.m_y - radius;
	if (r<0) 
		r = 0;
	for (i=p.m_y-1, k=j-LINE_GRID_WIDTH; i>=r; i--, k-=LINE_GRID_WIDTH)
	{
		i1 = g_lineGrid[k]&LINE_NODE_LINE_MASK;
		if (i1==0 || i0==i1)
			continue;
		addNadir(p, Point(p.m_x, i));
	}
	// search down
	r = p.m_y + radius;
	if (r>LINE_GRID_HEIGHT) 
		r = LINE_GRID_HEIGHT;
	for (i=p.m_y+1, k=j+LINE_GRID_WIDTH; i<r; i++, k+=LINE_GRID_WIDTH)
	{
		i1 = g_lineGrid[k]&LINE_NODE_LINE_MASK;
		if (i1==0 || i0==i1)
			continue;
		addNadir(p, Point(p.m_x, i));
	}
	
	// search left
	r = p.m_x - radius;
	if (r<0)
		r = 0;
	for (i=p.m_x-1, k=j-1; i>=r; i--, k--)
	{
		i1 = g_lineGrid[k]&LINE_NODE_LINE_MASK;
		if (i1==0 || i0==i1)
			continue;
		addNadir(p, Point(i, p.m_y));
	}
	// search right
	r = p.m_x + radius;
	if (r>LINE_GRID_WIDTH)
		r = LINE_GRID_WIDTH;
	for (i=p.m_x+1, k=j+1; i<r; i++, k++)
	{
		i1 = g_lineGrid[k]&LINE_NODE_LINE_MASK;
		if (i1==0 || i0==i1)
			continue;
		addNadir(p, Point(i, p.m_y));
	}	
}


void findNadirs()
{
	SimpleListNode<Point> *i, *j;
	uint16_t dist;
	uint8_t li, lj;
	uint16_t maxMergeDist2=g_maxMergeDist*g_maxMergeDist;
	
	// do n^2 search (or n*(n-1)/2 search)
	for (i=g_nodesList.m_first; i!=NULL; i=i->m_next)
	{
		li = LINE_GRID_LINE_P(i->m_object); 
		for (j=i->m_next; j!=NULL; j=j->m_next)
		{
			lj = LINE_GRID_LINE_P(j->m_object); 
			if (li==lj) // if we're the same line, don't bother.  We're looking for a greater line index
				continue;
			dist = i->m_object.dist2(j->m_object);
			if (dist<=maxMergeDist2)
				// in general li<lj because of the ordering of the labeling of the nodes and lines
				// this is important because of the grid search below.  We'll ignore cases where li>=lj
				// in the interest of efficiency.
				addNadir(i->m_object, j->m_object);
		}
	}
	
	// do grid search 
	for (i=g_nodesList.m_first; i!=NULL; i=i->m_next)
		search(i->m_object, g_maxMergeDist);
}

void reduceNadirs()
{
	SimpleListNode<Nadir> *i, *j, *inext, *jnext;
	uint32_t dist;
	uint16_t n, maxMergeDist2=(g_maxMergeDist*g_maxMergeDist*12)>>3; // multiply by a scaling factor of 1.5
	
	while(1)
	{
		// perform n^2 search, looking for nadirs that are close to each other
		n = 0;
		for (i=g_nadirsList.m_first; i!=NULL; i=inext)
		{
			inext = i->m_next;
			for (j=i->m_next; j!=NULL; j=jnext)
			{
				jnext = j->m_next;
				dist = i->m_object.m_pavg.dist2(j->m_object.m_pavg);
				if (dist<maxMergeDist2)
				{
					i->m_object.merge(j->m_object); // merge j -> i
					g_nadirsList.remove(j);
					if (inext==j)
						inext = jnext;
					n++;
				}
			}
		}
		if (n==0) // no change, return...
			break;
	}
}

// line1.m_p0 is center (intersection)
// line1.m_p1 is outer point of line1
// p01 is outer point of line0
void addLine(SimpleListNode<Nadir> *nadirs, uint8_t li0, const Point &p01, SimpleListNode<Line2> *linen1)
{
	SimpleListNode<Intersection> *j;
	uint8_t i, li1 = linen1->m_object.m_index;
	int8_t diffx, diffy;
	bool horiz;
	const Line2 &line1 = linen1->m_object;

	// determine direction
	diffx = p01.m_x - line1.m_p1.m_x;
	diffy = p01.m_y - line1.m_p1.m_y;
	
	horiz = ABS(diffx)>ABS(diffy);

	// look through intersections list for lines that are affected
	for (j=g_intersectionsList.m_first; j!=NULL; j=j->m_next)
	{
		for (i=0; i<j->m_object.m_n; i++)
		{
			// line li0 has changed.  It's had one of its endpoints set to the new intersection
			if (j->m_object.m_lines[i]->m_object.m_index==li0)
			{
				// We only need to look at line1.m_p1 since it's the only point that's an endpoint and therefore possibly coincident with this intersection
				// if line.m_p1 is coincident with the intersection, we need to change the line pointer
				if (j->m_object.m_p.equals(line1.m_p1))
				{
					j->m_object.m_lines[i] = linen1;
					linen1->m_object.m_i1 = j; // if we change the line in an intersection, we need to change the line to point to the intersection
				}
			}
		}
	}

	// look through remaining nadir list for points->lines that are affected
	for (; nadirs!=NULL; nadirs=nadirs->m_next)
	{
		for (i=0; i<nadirs->m_object.m_n; i++)
		{
			const Point &pt = nadirs->m_object.m_points[i];
			//if (LINE_GRID_INDEX_P(pt)>LINE_GRID_HEIGHT*LINE_GRID_WIDTH)
			//	printf("*** %d\n");
			if (li0==LINE_GRID_LINE_P(pt))
			{
				if (horiz)
				{
					if (SIGN(pt.m_x-line1.m_p0.m_x)==SIGN(line1.m_p1.m_x-line1.m_p0.m_x))
					{
						LINE_GRID_P(pt) &= ~LINE_NODE_LINE_MASK;
						LINE_GRID_P(pt) |= li1;
					}
				}
				else // vertical
				{
					if (SIGN(pt.m_y-line1.m_p0.m_y)==SIGN(line1.m_p1.m_y-line1.m_p0.m_y))
					{
						LINE_GRID_P(pt) &= ~LINE_NODE_LINE_MASK;
						LINE_GRID_P(pt) |= li1;
					}
				}
			}			
		}
	}
}

bool breakLine(SimpleListNode<Nadir> *nadirs, uint8_t li, SimpleListNode<Intersection> *intern)
{
	// orig line linen0   p0                m_p               p1
	// broken line        p0 linen0 p1 intersection p0 linen1 p1
	Line2 line2;
	SimpleListNode<Line2> *linen1;
	SimpleListNode<Line2> *linen0 = g_lines[li];
	checkGraph(__LINE__, 1, 2);
	
	if (g_lineIndex>=LINE_MAX_LINES || intern->m_object.m_n>LINE_MAX_INTERSECTION_LINES-2 || 
		linen0->m_object.m_p0.equals(linen0->m_object.m_p1)) // this last case is a crazy case the fouls up the addLine() logic below
		return false;
	
	// create new line, add it to intersection
	line2.m_p1 = linen0->m_object.m_p1;
	line2.m_p0 = intern->m_object.m_p;
	line2.m_index = g_lineIndex;
	linen1 = g_linesList.add(line2);

	if (linen1==NULL)
		return false;

	// modify first line
	linen0->m_object.m_p1 = intern->m_object.m_p;
	intern->m_object.addLine(linen0, intern, 1);

	g_lines[g_lineIndex++] = linen1;
	intern->m_object.addLine(linen1, intern, 0);
	
	// feedforward new line to nadirs we haven't seen yet
	addLine(nadirs, li, linen0->m_object.m_p0, linen1); 
	
	checkGraph(__LINE__, 1, 2);
	
	return true;
}

uint8_t removeLine(SimpleListNode<Line2> *linen, SimpleListNode<Intersection> *intern)
{
	uint8_t i, j, m;
	
	// remove line from intersection
	for (i=m=0; intern && i<intern->m_object.m_n; i++)
	{
		if (intern->m_object.m_lines[i]==linen)
		{
			intern->m_object.m_n--;
			// shift lines down
			for (j=i; j<intern->m_object.m_n; j++)
				intern->m_object.m_lines[j] = intern->m_object.m_lines[j+1];
			i--;
			m++;
		}
	}
	return m;
}


void removeLine(SimpleListNode<Line2> *linen)
{	
	removeLine(linen, linen->m_object.m_i0);
	removeLine(linen, linen->m_object.m_i1);
	
#if 0
	// remove line from g_lines
	for (i=0; i<g_lineIndex; i++)
	{
		if (g_lines[i]==linen)
		{
			g_lines[i] = NULL;
			break;
		}
	}
#endif
	
	// remove line from g_linesList
	g_linesList.remove(linen);
	
}

void replaceLine(SimpleListNode<Line2> *linen, SimpleListNode<Line2> *linenx)
{
	uint8_t i;
	SimpleListNode<Intersection> *j;

#if 0	
	for (i=0; i<g_lineIndex; i++)
	{
		if (g_lines[i]==linenx)
		{
			g_lines[i] = linen;
			break;
		}
	}
#endif
	
	// remove line from existing intersections
	for (j=g_intersectionsList.m_first; j!=NULL; j=j->m_next)
	{
		for (i=0; i<j->m_object.m_n; i++)
		{
			if (j->m_object.m_lines[i]==linenx)
			{
				j->m_object.m_lines[i] = linen;
//				break;
			}
		}
	}
	
	g_linesList.remove(linenx);
}


void formIntersections()
{
	SimpleListNode<Nadir> *i;
	uint8_t j, li;

	checkGraph(__LINE__);
	
	// go through nadir list and break up the lines
	for (i=g_nadirsList.m_first; i!=NULL; i=i->m_next)
	{
		SimpleListNode<Intersection> *intern = new (std::nothrow) SimpleListNode<Intersection>;
		if (intern==NULL)
			return;
		Intersection *inter = &intern->m_object;
		inter->m_p = i->m_object.m_pavg;
		// look at each point/line in the list
		// If it's an endpoint, we extend the line to the intersection point (avg)
		// if it's not endpoint, we need to break the line and form 2 lines, each with the endpoint at the intersection
		
		for (j=inter->m_n=0; j<i->m_object.m_n && inter->m_n<LINE_MAX_INTERSECTION_LINES; j++)
		{
			Line2 *line;
			SimpleListNode<Line2> *linen;
			const Point &pt = i->m_object.m_points[j];
			li = LINE_GRID_LINE_P(pt); 
			if (li==0)
				continue;
			linen = g_lines[li];
			line = &linen->m_object;
			checkGraph(__LINE__, 1, 2, intern);
			if (pt.equals(line->m_p0)) // matches p0
			{
				line->m_p0 = inter->m_p; // extend line
				if (line->m_i0) // if we are already pointing to another intersection, remove and add ours
					removeLine(linen, line->m_i0);
				inter->addLine(linen, intern, 0);
				checkGraph(__LINE__, 0, 0, intern);
				checkGraph(__LINE__, 1, 2);
			}
			else if (pt.equals(line->m_p1)) // matches p1
			{
				line->m_p1 = inter->m_p; // extend line
				if (line->m_i1) // if we are already pointing to another intersection, remove and add ours
					removeLine(linen, line->m_i1);
				inter->addLine(linen, intern, 1);
				checkGraph(__LINE__, 0, 0, intern);
				checkGraph(__LINE__, 1, 2);
			}
			else
			{
				breakLine(i->m_next, li, intern);
				checkGraph(__LINE__, 0, 0, intern);
				checkGraph(__LINE__, 1, 2);
			}
		}
		g_intersectionsList.add(intern);
		checkGraph(__LINE__);
	}
}	

bool validIntersection(SimpleListNode<Intersection> *intern)
{
	SimpleListNode<Intersection> *i;
	
	if (intern==NULL)
		return true; // valid

	for (i=g_intersectionsList.m_first; i!=NULL; i=i->m_next)
	{
		if (i==intern)
			return true;
	}
	return false;
}

bool validLine(SimpleListNode<Line2> *linen)
{
	SimpleListNode<Line2> *i;
	
	if (linen==NULL)
		return true; // valid

	for (i=g_linesList.m_first; i!=NULL; i=i->m_next)
	{
		if (i==linen)
			return true;
	}
	return false;
}


#define CHECK(cond, index)   if(!(cond) && suppress0!=index && suppress1!=index) {check=index; goto end;} 

bool checkGraph(int val, uint8_t suppress0, uint8_t suppress1, SimpleListNode<Intersection> *intern)
{
	SimpleListNode<Line2> *i;
	SimpleListNode<Intersection> *j;
	uint8_t k, n, h, check=0;	
	
	if (!(g_debug&LINE_DEBUG_GRAPH_CHECK))
		return true;
	
	if (intern)
		goto intersection;
	
	// go through line list, make sure all intersection pointers are valid 
	// by finding them in the list, make sure the line is in the intersection's list
	for (i=g_linesList.m_first; i!=NULL; i=i->m_next)
	{
		CHECK(validIntersection(i->m_object.m_i0), 1);
		CHECK(validIntersection(i->m_object.m_i1), 2);
		if (i->m_object.m_i0)
		{
			for (k=n=0; k<i->m_object.m_i0->m_object.m_n; k++)
			{
				if (i->m_object.m_i0->m_object.m_lines[k]==i)
					n++;
			}
			CHECK(n==1, 3);
		}
		if (i->m_object.m_i1)
		{
			for (k=n=0; k<i->m_object.m_i1->m_object.m_n; k++)
			{
				if (i->m_object.m_i1->m_object.m_lines[k]==i)
					n++;
			}
			CHECK(n==1, 4);
		}		
	}
	
	intersection:
	// go through intersection list, make sure each intersection's lines are valid
	// make sure each line points back to the intersection
	for (j=g_intersectionsList.m_first; j!=NULL; j=j->m_next)
	{
		if (intern)
			j = intern;
		for (k=n=0; k<j->m_object.m_n; k++)
		{
			CHECK(validLine(j->m_object.m_lines[k]), 5);
			CHECK(j->m_object.m_lines[k]->m_object.m_i0 != j->m_object.m_lines[k]->m_object.m_i1, 6);
			CHECK(j->m_object.m_lines[k]->m_object.m_i0==j || j->m_object.m_lines[k]->m_object.m_i1==j, 7);
			if (j->m_object.m_lines[k]->m_object.m_i0==j)
				CHECK(j->m_object.m_p.equals(j->m_object.m_lines[k]->m_object.m_p0), 8);
			if (j->m_object.m_lines[k]->m_object.m_i1==j)
				CHECK(j->m_object.m_p.equals(j->m_object.m_lines[k]->m_object.m_p1), 9);			
			for (h=k+1; h<j->m_object.m_n; h++)
			{
				if (j->m_object.m_lines[k]==j->m_object.m_lines[h])
					n++;
			}
			CHECK(n==0, 10);
		}
		if (intern)
			break;
	}
	
	end:
	if (check!=0)
	{
		cprintf(0, "fail %d %d\n", check, val);
		return false;
	}
		
	return true;
}

uint16_t removeShortLinesIntersections()
{
	uint8_t i, si;
	SimpleListNode<Intersection> *j;
	uint16_t n, m, length, shortest;
	
	// look through intersections removing short lines
	// we remove the shortest lines first because we want to preserve the next-longest line for when we get to lines==2
	for (j=g_intersectionsList.m_first, m=0; j!=NULL; j=j->m_next)
	{
		while(1)
		{
			checkGraph(__LINE__);
			n = 0;
			// If there are 2 lines in the intersection, no reason to trim (we'll merge them)
			//if (j->m_object.m_n<=2)
			//	break;
			
			// find shortest line in intersection
			for (i=0, shortest=0xffff, si=0; i<j->m_object.m_n; i++)
			{
				length = j->m_object.m_lines[i]->m_object.length2();
				if (length<shortest)
				{
					si = i;
					shortest = length;
				}
			}
				
			// if it's shorter than g_minLineLength2, possibly remove it 
			if (shortest<g_minLineLength2)
			{
				SimpleListNode<Line2> *linen = j->m_object.m_lines[si];
				// one of the intersection pointers needs to be NULL, the other is our intersection
				// if they are both non-NULL, this line bridges 2 intersections, so we can't remove
				if (linen->m_object.m_i0==NULL || linen->m_object.m_i1==NULL)
				{
					removeLine(linen);
					n++;
					m++;
				}
			}
			checkGraph(__LINE__);
			if (n==0)
				break;
		}
	}
	return m;
}

bool equivalentLines(const Line2 &line0, const Line2 &line1)
{
	return ((line0.m_p0.equals(line1.m_p0) && line0.m_p1.equals(line1.m_p1)) ||
		(line0.m_p0.equals(line1.m_p1) && line0.m_p1.equals(line1.m_p0)));
}

bool minAngleLines(const Line2 &line0, const Line2 &line1, const Point &p, uint32_t minAngle)
{
	Point p01, p11;
	
	if (p.equals(line0.m_p0))
		p01 = line0.m_p1;
	else
		p01 = line0.m_p0;
	if (p.equals(line1.m_p0))
		p11 = line1.m_p1;
	else
		p11 = line1.m_p0;
	
	return (tanDiffAbs1000(p, p01, p, p11) <= minAngle);
}

uint16_t removeRedundantLinesIntersections()
{
	uint8_t i, j, n;
	SimpleListNode<Intersection> *k;

	for (k=g_intersectionsList.m_first, n=0; k!=NULL; k=k->m_next)
	{
		// perform n^2 search of lines within this intersection
		for (i=0; i<k->m_object.m_n; i++)
		{
			for (j=i+1; j<k->m_object.m_n; j++)
			{
				// Check for equivalent lines...
				if (equivalentLines(k->m_object.m_lines[i]->m_object, k->m_object.m_lines[j]->m_object))
				{
					removeLine(k->m_object.m_lines[j]);
					n++;
					j--; // list was shifted down, decrement so we don't skip an entry
				}
				// Check for lines with a small angle between them
				else if (minAngleLines(k->m_object.m_lines[i]->m_object, k->m_object.m_lines[j]->m_object, k->m_object.m_p, g_maxEquivTanAngle))
				{
					// lines i and j have a small angle between them.  Which do we delete?  Which line is longer?  What if the shorter one is connected to another intersection?
					// if i is longer than j
					if (k->m_object.m_lines[i]->m_object.length2() > k->m_object.m_lines[j]->m_object.length2())
					{
						// remove line j, since it's shorter, unless it's connected...
						if (k->m_object.m_lines[j]->m_object.m_i0==NULL || k->m_object.m_lines[j]->m_object.m_i1==NULL)
						{
							removeLine(k->m_object.m_lines[j]);
							n++;
							j--; // list was shifted down, decrement so we don't skip an entry
						}
						// otherwise remove line i, if it's not connected (we know that it's connected this intersection, so if it has at least 1 NULL intersection pointer...)
						else if (k->m_object.m_lines[i]->m_object.m_i0==NULL || k->m_object.m_lines[i]->m_object.m_i1==NULL)
						{
							removeLine(k->m_object.m_lines[i]);
							n++;
							i--; // list was shifted down, decrement so we don't skip an entry
							break; // then exit, since it affects our i-th index
						}
					}
					else // Line j is longer than i
					{
						// remove line i, since it's shorter, unless it's connected... 
						if (k->m_object.m_lines[i]->m_object.m_i0==NULL || k->m_object.m_lines[i]->m_object.m_i1==NULL)
						{
							removeLine(k->m_object.m_lines[i]);
							n++;
							i--; // list was shifted down, decrement so we don't skip an entry
							break; // then exit, since it affects our i-th index
						}
						// otherwise remove line j, if it's not connected
						else if (k->m_object.m_lines[j]->m_object.m_i0==NULL || k->m_object.m_lines[j]->m_object.m_i1==NULL)
						{
							removeLine(k->m_object.m_lines[j]);
							n++;
							j--; // list was shifted down, decrement so we don't skip an entry
						}
					}
				}
			}
		}
	}
	checkGraph(__LINE__);
	
	return n;
}

uint16_t simplifyIntersections()
{
	uint16_t n;
	SimpleListNode<Intersection> *j, *jnext;

	checkGraph(__LINE__);

	// look through intersections dealing with 0, 1 and 2-line intersections
	for (j=g_intersectionsList.m_first, n=0; j!=NULL; j=jnext)
	{
		jnext = j->m_next;
		if (j->m_object.m_n==0)
		{
			g_intersectionsList.remove(j);
			checkGraph(__LINE__);
			n++;
		}
		else if (j->m_object.m_n==1)
		{
			if (j->m_object.m_lines[0]->m_object.m_i0==j)
				j->m_object.m_lines[0]->m_object.m_i0 = NULL;
			if (j->m_object.m_lines[0]->m_object.m_i1==j)
				j->m_object.m_lines[0]->m_object.m_i1 = NULL;
			g_intersectionsList.remove(j);
			checkGraph(__LINE__);
			n++;
		}
		else if (j->m_object.m_n==2 && 
			j->m_object.m_p.m_x>g_minLineLength && j->m_object.m_p.m_x<LINE_GRID_WIDTH-g_minLineLength && // do not simplify intersections that are
			j->m_object.m_p.m_y>g_minLineLength && j->m_object.m_p.m_y<LINE_GRID_HEIGHT-g_minLineLength) // close to any edge because they don't look like intersections.
		{
			SimpleListNode<Line2> *linen0 = j->m_object.m_lines[0];
			SimpleListNode<Line2> *linen1 = j->m_object.m_lines[1];
			g_intersectionsList.remove(j);
			if (linen0->m_object.m_i0==j)
			{
				if (linen1->m_object.m_i0==j)
				{
					// this case will lead to an intersection with two of the same line pointers, so get rid of linen1, proceed...
					if (linen0->m_object.m_i1==linen1->m_object.m_i1 && linen1->m_object.m_i1!=NULL)
					{
						linen0->m_object.m_i0 = NULL;
						removeLine(linen1);
					}
					else
					{
						linen0->m_object.m_p0 = linen1->m_object.m_p1;
						linen0->m_object.m_i0 = linen1->m_object.m_i1;
						replaceLine(linen0, linen1);
					}
				}
				else
				{
					// this case will lead to an intersection with two of the same line pointers, so get rid of linen1, proceed...
					if (linen0->m_object.m_i1==linen1->m_object.m_i0 && linen1->m_object.m_i0!=NULL)
					{
						linen0->m_object.m_i0 = NULL;
						removeLine(linen1);
					}
					else
					{
						linen0->m_object.m_p0 = linen1->m_object.m_p0;
						linen0->m_object.m_i0 = linen1->m_object.m_i0;
						replaceLine(linen0, linen1);
					}
				}
			}
			else
			{
				if (linen1->m_object.m_i0==j)
				{
					// this case will lead to an intersection with two of the same line pointers, so get rid of linen1, proceed...
					if (linen0->m_object.m_i0==linen1->m_object.m_i1 && linen1->m_object.m_i1!=NULL)
					{
						linen0->m_object.m_i1 = NULL;
						removeLine(linen1);
					}
					else
					{
						linen0->m_object.m_p1 = linen1->m_object.m_p1;
						linen0->m_object.m_i1 = linen1->m_object.m_i1;
						replaceLine(linen0, linen1);
					}
				}
				else
				{
					// this case will lead to an intersection with two of the same line pointers, so get rid of linen1, proceed...
					if (linen0->m_object.m_i0==linen1->m_object.m_i0 && linen1->m_object.m_i0!=NULL)
					{
						linen0->m_object.m_i1 = NULL;
						removeLine(linen1);
					}
					else
					{
						linen0->m_object.m_p1 = linen1->m_object.m_p0;
						linen0->m_object.m_i1 = linen1->m_object.m_i0;
						replaceLine(linen0, linen1);
					}
				}
			}
			checkGraph(__LINE__);
			n++;
		}
	}
	return n;
}

void cleanIntersections()
{
	uint16_t n;
	
	while(1)
	{
		n = 0;
		n += removeShortLinesIntersections(); 
		n += removeRedundantLinesIntersections();
		n += simplifyIntersections();
		
		if (n==0)
			break;
	}
}

void removeMinLines(uint16_t minLineLength)
{
	SimpleListNode<Line2> *i;
		
	for (i=g_linesList.m_first; i!=NULL; i=i->m_next)
	{
		if (i->m_object.length2()<minLineLength && i->m_object.m_i0==NULL && i->m_object.m_i1==NULL)
			g_linesList.remove(i);
	}
}


int sendLineSegments(uint8_t renderFlags)
{
	uint32_t len;
	uint8_t *lineData = (uint8_t *)g_lineSegsMem + CAM_PREBUF_LEN - CAM_FRAME_HEADER_LEN;
	
	// fill buffer contents manually for return data 
	len = Chirp::serialize(g_chirpUsb, (uint8_t *)lineData, LINE_MAX_SEGMENTS*sizeof(LineSeg), HTYPE(FOURCC('L','I','S','G')), HINT8(renderFlags), UINT16(LINE_GRID_WIDTH), UINT16(LINE_GRID_HEIGHT), UINTS8_NO_COPY(g_lineSegIndex*sizeof(LineSeg)), END);
	if (len!=CAM_FRAME_HEADER_LEN)
		return -1;
	
	g_chirpUsb->useBuffer((uint8_t *)lineData, CAM_FRAME_HEADER_LEN+g_lineSegIndex*sizeof(LineSeg)); 
	
	return 0;
}

void sendPoints(const SimpleList<Point> &points, uint8_t renderFlags, const char *desc)
{
	SimpleListNode<Point> *i;

	CRP_SEND_XDATA(g_chirpUsb, HTYPE(FOURCC('N','A','D','F')), INT8(RENDER_FLAG_START), STRING(desc), INT16(LINE_GRID_WIDTH), INT16(LINE_GRID_HEIGHT), END);
	
	for (i=points.m_first; i!=NULL; i=i->m_next)		
		CRP_SEND_XDATA(g_chirpUsb, HTYPE(FOURCC('N','A','D','S')), INTS8(2, &i->m_object), END);	
	
	CRP_SEND_XDATA(g_chirpUsb, HTYPE(FOURCC('N','A','D','F')), INT8(renderFlags), STRING(desc), INT16(LINE_GRID_WIDTH), INT16(LINE_GRID_HEIGHT), END);
}


void sendNadirs(const SimpleList<Nadir> &nadirs, uint8_t renderFlags, const char *desc)
{
	SimpleListNode<Nadir> *i;
	uint32_t pi;
	Point ps[LINE_MAX_INTERSECTION_LINES];

	CRP_SEND_XDATA(g_chirpUsb, HTYPE(FOURCC('N','A','D','F')), INT8(RENDER_FLAG_START), STRING(desc), INT16(LINE_GRID_WIDTH), INT16(LINE_GRID_HEIGHT), END);
	
	for (i=nadirs.m_first; i!=NULL; i=i->m_next)
	{
		ps[0] = i->m_object.m_pavg;
		
		for (pi=0; pi<i->m_object.m_n; pi++)
			ps[pi+1] = i->m_object.m_points[pi];
		
		CRP_SEND_XDATA(g_chirpUsb, HTYPE(FOURCC('N','A','D','S')), INTS8((i->m_object.m_n+1)*2, ps), END);	
	}
	CRP_SEND_XDATA(g_chirpUsb, HTYPE(FOURCC('N','A','D','F')), INT8(renderFlags), STRING(desc), INT16(LINE_GRID_WIDTH), INT16(LINE_GRID_HEIGHT), END);
}

void sendLines(const SimpleList<Line2> &lines, uint8_t renderFlags, const char *desc)
{
	SimpleListNode<Line2> *n;
	uint8_t i;
	
	CRP_SEND_XDATA(g_chirpUsb, HTYPE(FOURCC('L','I','S','F')), INT8(RENDER_FLAG_START), STRING(desc), INT16(LINE_GRID_WIDTH), INT16(LINE_GRID_HEIGHT), END);

	for(n=lines.m_first, i=0; n; n=n->m_next, i++)
		CRP_SEND_XDATA(g_chirpUsb, HTYPE(FOURCC('L','I','S','S')), INT8(1), INT8(i), INT16(n->m_object.m_p0.m_x), INT16(n->m_object.m_p0.m_y), INT16(n->m_object.m_p1.m_x), INT16(n->m_object.m_p1.m_y), END);
	
	CRP_SEND_XDATA(g_chirpUsb, HTYPE(FOURCC('L','I','S','F')), INT8(renderFlags), STRING(desc), INT16(CAM_RES3_WIDTH), INT16(CAM_RES3_HEIGHT), END);	
}

void sendTrackedLines(const SimpleList<Tracker<Line2> > &lines, uint8_t renderFlags, const char *desc)
{
	SimpleListNode<Tracker<Line2> > *n;
	Line2 *line;
	uint8_t i, mode;
	
	CRP_SEND_XDATA(g_chirpUsb, HTYPE(FOURCC('L','I','S','F')), INT8(RENDER_FLAG_START), STRING(desc), INT16(LINE_GRID_WIDTH), INT16(LINE_GRID_HEIGHT), END);

	for(n=lines.m_first, i=0; n; n=n->m_next, i++)
	{
		mode = n->m_object.get()==NULL ? 1 : 0;
		line = & n->m_object.m_object;
		CRP_SEND_XDATA(g_chirpUsb, HTYPE(FOURCC('L','I','S','S')), INT8(mode), INT8(n->m_object.m_index), INT16(line->m_p0.m_x), INT16(line->m_p0.m_y), INT16(line->m_p1.m_x), INT16(line->m_p1.m_y), END);
	}
	
	CRP_SEND_XDATA(g_chirpUsb, HTYPE(FOURCC('L','I','S','F')), INT8(renderFlags), STRING(desc), INT16(CAM_RES3_WIDTH), INT16(CAM_RES3_HEIGHT), END);	
}
	
void sendIntersections(const SimpleList<Intersection> &lines, uint8_t renderFlags, const char *desc)
{
	SimpleListNode<Intersection> *i;

	CRP_SEND_XDATA(g_chirpUsb, HTYPE(FOURCC('N','A','D','F')), INT8(RENDER_FLAG_START), STRING(desc), INT16(LINE_GRID_WIDTH), INT16(LINE_GRID_HEIGHT), END);
	
	for (i=lines.m_first; i!=NULL; i=i->m_next)
		CRP_SEND_XDATA(g_chirpUsb, HTYPE(FOURCC('N','A','D','S')), INTS8(2, &i->m_object.m_p), END);	
	CRP_SEND_XDATA(g_chirpUsb, HTYPE(FOURCC('N','A','D','F')), INT8(renderFlags), STRING(desc), INT16(LINE_GRID_WIDTH), INT16(LINE_GRID_HEIGHT), END);	
}

void sendPrimaryFeatures(uint8_t renderFlags)
{
	uint8_t n=0;
	Line2 *primary=NULL;
	
	if (g_lineState==LINE_STATE_TRACKING)
	{	
		primary = findTrackedLine(g_primaryLineIndex);
		if (g_primaryIntersection.m_state!=TR_INVALID)
			n = g_primaryIntersection.m_object.m_n;
	}

	if (primary)
	{
		if (g_goalPoint.equals(primary->m_p0))
			CRP_SEND_XDATA(g_chirpUsb, HTYPE(FOURCC('P','V','I','0')), INT8(renderFlags), INT16(LINE_GRID_WIDTH), INT16(LINE_GRID_HEIGHT), 
				INT8(primary->m_p1.m_x), INT8(primary->m_p1.m_y), INT8(primary->m_p0.m_x), INT8(primary->m_p0.m_y), INT8(n), END);
		else
			CRP_SEND_XDATA(g_chirpUsb, HTYPE(FOURCC('P','V','I','0')), INT8(renderFlags), INT16(LINE_GRID_WIDTH), INT16(LINE_GRID_HEIGHT), 
				INT8(primary->m_p0.m_x), INT8(primary->m_p0.m_y), INT8(primary->m_p1.m_x), INT8(primary->m_p1.m_y), INT8(n), END);
	}
	else // send null message so it always shows up as a layer
		CRP_SEND_XDATA(g_chirpUsb, HTYPE(FOURCC('P','V','I','0')), INT8(renderFlags), INT16(LINE_GRID_WIDTH), INT16(LINE_GRID_HEIGHT), 
			INT8(0), INT8(0), INT8(0), INT8(0), INT8(0), END);
		
	
}

int16_t voteCodes(BarCodeCluster *cluster)
{
    uint8_t votes[LINE_MMC_VTSIZE];
    int16_t vals[LINE_MMC_VTSIZE];
    int16_t val;
    uint16_t i, j, max, maxIndex;

	if (cluster->m_n<=1)
		return -1;
	
    for (i=0; i<LINE_MMC_VTSIZE; i++)
        votes[i] = 0;

    // tally votes
    for (i=0; i<cluster->m_n; i++)
    {
        val = g_candidateBarcodes[cluster->m_indexes[i]]->m_val;
        if (val<0)
            continue;
        // find index or empty location
        for (j=0; j<LINE_MMC_VTSIZE; j++)
        {
            if (votes[j]==0)
            {
                vals[j] = val;
                break;
            }
            if (vals[j]==val)
                break;
        }
        if (j>=LINE_MMC_VTSIZE)
            continue;
        // add vote
        votes[j]++;
    }

    // find winner
    for (i=0, max=0; i<LINE_MMC_VTSIZE; i++)
    {
        if (votes[i]==0) // we've reached end
            break;
        if (votes[i]>max)
        {
            max = votes[i];
            maxIndex = i;
        }
    }

    if (max<=1) // no valid codes, and you need at least 2 votes
        return -2;
	if ((max<<8)/cluster->m_n<g_minVotingThreshold)
		return -3;
    return vals[maxIndex];
}

uint32_t dist2_4(const Point16 &p0, const Point16 &p1)
{
		int32_t diffx, diffy;
		
		diffx = p1.m_x - p0.m_x;
		diffy = (p1.m_y - p0.m_y)*4;
		
		return diffx*diffx + diffy*diffy;	
}

void clusterCodes()
{
    BarCodeCluster *clusters[LINE_MMC_VOTED_BARCODES];
    uint8_t i, j, numClusters = 0;
    int16_t val;
    int32_t dist;

    for (i=0; i<g_barcodeIndex; i++)
    {
        for (j=0; j<numClusters; j++)
        {
            dist = dist2_4 (g_candidateBarcodes[i]->m_p0, clusters[j]->m_p1);
            if (dist<g_maxCodeDist)
                break;
        }
        if (j>=LINE_MMC_VOTED_BARCODES) // table is full, move onto next code
            continue;
        if (j>=numClusters) // new entry
        {
            clusters[j] = new (std::nothrow) BarCodeCluster;
			if (clusters[j]==NULL)
				break;
            // reset positions
            clusters[j]->m_p0 = g_candidateBarcodes[i]->m_p0;
            clusters[j]->m_p1 = g_candidateBarcodes[i]->m_p0;
            numClusters++;
        }
        //cprintf(" add %d %d, %d %d %d %d", j, i, g_candidateBarcodes[i]->m_p0.m_x, g_candidateBarcodes[i]->m_p0.m_y,
         //      clusters[j]->m_p1.m_x, clusters[j]->m_p1.m_y);
        clusters[j]->addCode(i);
        // update width, position
        clusters[j]->updateWidth(g_candidateBarcodes[i]->m_width);
        clusters[j]->m_p1 = g_candidateBarcodes[i]->m_p0;
    }

#if 0
	for (i=0; i<numClusters; i++)
		cprintf(0, "%d: %d %d %d %d\n", i, clusters[i]->m_p0.m_x, clusters[i]->m_p0.m_y, clusters[i]->m_p1.m_x, clusters[i]->m_p1.m_y);
#endif
    // vote
    for (i=0, g_votedBarcodeIndex=0; i<numClusters; i++)
    {
        if (g_votedBarcodeIndex>=LINE_MMC_VOTED_BARCODES)
            break; // out of table space
        val = voteCodes(clusters[i]);
        if (val<0)
            continue;
        g_votedBarcodes[g_votedBarcodeIndex].m_val = val;
        g_votedBarcodes[g_votedBarcodeIndex].m_outline.m_xOffset = clusters[i]->m_p0.m_x + g_dist;
        g_votedBarcodes[g_votedBarcodeIndex].m_outline.m_yOffset = clusters[i]->m_p0.m_y;
        g_votedBarcodes[g_votedBarcodeIndex].m_outline.m_width = clusters[i]->m_width + 1;
        g_votedBarcodes[g_votedBarcodeIndex].m_outline.m_height = clusters[i]->m_p1.m_y - clusters[i]->m_p0.m_y + 1;
		g_votedBarcodes[g_votedBarcodeIndex].m_tracker = NULL;
        g_votedBarcodeIndex++;
    }

	
#if 0
    for (i=0; i<g_votedBarcodeIndex; i++)
	cprintf(0, "%d: %d, %d %d %d %d", i, g_votedBarcodes[i].m_val,
               g_votedBarcodes[i].m_outline.m_xOffset, g_votedBarcodes[i].m_outline.m_yOffset,
               g_votedBarcodes[i].m_outline.m_width, g_votedBarcodes[i].m_outline.m_height);
#endif
	
    for (i=0; i<numClusters; i++)
        delete clusters[i];

    for (i=0; i<g_barcodeIndex; i++)
        delete g_candidateBarcodes[i];
}

int32_t decodeCode(BarCode *bc, uint16_t dec)
{
    uint8_t i, bits;
    uint16_t val, bit, width, minWidth, maxWidth;
    bool flag;

    for (i=1, bits=0, val=0, flag=false, minWidth=0xffff, maxWidth=0; i<bc->m_n && bits<LINE_MMC_BITS; i++)
    {
        bit = bc->m_edges[i]&EQ_NEGATIVE;
        if ((bit==0 && (i&1)) || (bit && (i&1)==0))
            return -1;

        width = bc->m_edges[i]&~EQ_NEGATIVE;
        if (width<minWidth)
            minWidth = width;
        if (width>maxWidth)
            maxWidth = width;
        if (width<dec)
        {
            if (flag)
            {
                val <<= 1;
                if (bit==0)
                    val |= 1;
                bits++;
                flag = false;
            }
            else
                flag = true;
        }
        else if (flag) // wide with flag, must be error
            return 0;
        else // wide
        {
            val <<= 1;
            if (bit==0)
                val |= 1;
            bits++;
        }
    }
    if (bits!=LINE_MMC_BITS)
        return 0;
    if (maxWidth/minWidth>10)
        return -2;
    bc->m_val = val;
    return 1;
}

int comp8(const void *a, const void *b)
{
  return *(uint8_t *)a - *(uint8_t *)b;
}

bool decodeCode(BarCode *bc)
{
    int32_t res;
	uint8_t halfPeriodsLUT[1<<LINE_MMC_BITS], edges[LINE_MMC_MAX_EDGES-1];
	uint8_t i, j, lastBit, bit, bits, transitions, ftrans, gap, maxGap, threshold;
	
	for (i=0; i<1<<LINE_MMC_BITS; i++)
	{
		// count 01 transitions
		// count them by looking at lsb first, so we're looking at 10 transitions
		for (j=0, bits=i, lastBit=0, transitions=0, ftrans=0; j<LINE_MMC_BITS; j++, bits>>=1, lastBit=bit)
		{
			bit = bits&0x01;
			if (bit==0 && lastBit!=0)
			{
				if (i==1)
					ftrans = 1;
				transitions++;
			}
		}
		halfPeriodsLUT[i] = 9;
		if (ftrans)
			halfPeriodsLUT[i] += 2;
		else if ((i&0x01)==0)
			halfPeriodsLUT[i] += 1;
	}
	
	// copy edges
	for (i=0; i<bc->m_n; i++)
		edges[i] = bc->m_edges[i];
	
	// sort 
	qsort(edges, bc->m_n, sizeof(uint8_t), comp8);
	
	// find biggest gap
	for (i=0, maxGap=0; i<bc->m_n-1; i++)
	{
		gap = edges[i+1] - edges[i];
		if (gap>maxGap)
		{
			maxGap = gap;
			threshold = (edges[i+1] + edges[i])>>1;
		}
	}
	
	// is the biggest gap big enough?
	// It needs to be at least a quarter of the smallest gap
	if (maxGap < (edges[0]>>2) + 1)
		threshold = edges[bc->m_n-1] + 1;  // it's not big enough, make threshold outside our gap range
	
    res = decodeCode(bc, threshold);
    if (res==1)
		return true;
    else // error
		return false;
}


bool detectCode(uint16_t *edges, uint16_t len, bool begin, BarCode *bc)
{
    uint16_t col00, col0, col1, col01, width0, width, qWidth;
    uint8_t e;

    col00 = edges[0]&~EQ_NEGATIVE;
    col01 = edges[1]&~EQ_NEGATIVE;
    width0 = col01 - col00;
    qWidth = width0<<2; // use to calc front-porch and back-porch

    // check front quiet period
    if (!begin)
    {
        width = col00 - (edges[-1]&~EQ_NEGATIVE);
        if (width<qWidth)
            return false;
    }


    // first determine if we have enough edges
    for (e=0, bc->m_n=0; e<len-1; e++)
    {
        if  (e>=LINE_MMC_MAX_EDGES)
            return false; // too many edges for valid code
        col0 = edges[e]&~EQ_NEGATIVE;
        // correct
        //if ((edges[e]&EQ_NEGATIVE)==0)
        //    col0--;
        col1 = edges[e+1]&~EQ_NEGATIVE;

        width = col1 - col0;
        if (width>qWidth) // back-porch?
            break;
        // save edge
        bc->m_edges[e] = width | (edges[e+1]&EQ_NEGATIVE);
        bc->m_n++;
        bc->m_width = col1 - col00;
    }

    if (e<LINE_MMC_MIN_EDGES-1)
        return false; // too few edges

    bc->m_p0.m_x = col00;
    return true;
}

void detectCodes(uint8_t row, uint16_t *edges, uint32_t len)
{
	bool res;
	bool begin;
	uint16_t j, k, bit0, bit1;
    BarCode *bc;

	if (len<LINE_MMC_MIN_EDGES)
		return;

	bc = new (std::nothrow) BarCode;
	if (bc==NULL)
		return;
	
	for (j=0, begin=true, k=0; j<len-1 && edges[j]<EQ_HSCAN_LINE_START && edges[j+1]<EQ_HSCAN_LINE_START; j++, begin=false, k++)
	{
		bit0 = edges[j]&EQ_NEGATIVE;
		bit1 = edges[j+1]&EQ_NEGATIVE;
		if (bit0!=0 && bit1==0 && len>=LINE_MMC_MIN_EDGES-1+k)
        {
			bc->m_p0.m_y = row;

			if (detectCode(&edges[j], len-j, begin, bc))
			{
                if (g_barcodeIndex>=LINE_MMC_CANDIDATE_BARCODES)
					goto end;
				res = decodeCode(bc);
#if 0
                cprintf(0, "%d %d: %d %d: %d, %d %d %d %d %d %d %d %d %d", bc->m_p0.m_x, bc->m_p0.m_y, res, bc->m_val,
                           bc->m_n, bc->m_edges[0]&~EQ_NEGATIVE, bc->m_edges[1]&~EQ_NEGATIVE, bc->m_edges[2]&~EQ_NEGATIVE, bc->m_edges[3]&~EQ_NEGATIVE, bc->m_edges[4]&~EQ_NEGATIVE,
                            bc->m_edges[5]&~EQ_NEGATIVE, bc->m_edges[6]&~EQ_NEGATIVE, bc->m_edges[7]&~EQ_NEGATIVE, bc->m_edges[8]&~EQ_NEGATIVE);
#endif
				if (res)
				{
					g_candidateBarcodes[g_barcodeIndex++] = bc;
					bc = new (std::nothrow) BarCode;
					if (bc==NULL)
						return;
				}
			}
		}
	}

	end:	
	delete bc;
}

int sendCodes(uint8_t renderFlags)
{
	uint32_t len;
	uint8_t *codeData = (uint8_t *)g_votedBarcodesMem + CAM_PREBUF_LEN - CAM_FRAME_HEADER_LEN;
	
	// fill buffer contents manually for return data 
	len = Chirp::serialize(g_chirpUsb, (uint8_t *)codeData, LINE_MMC_VOTED_BARCODES*sizeof(DecodedBarCode), HTYPE(FOURCC('C','O','D','E')), HINT8(renderFlags), UINT16(CAM_RES3_WIDTH), UINT16(CAM_RES3_HEIGHT), UINTS8_NO_COPY(g_votedBarcodeIndex*sizeof(DecodedBarCode)), END); 
	if (len!=CAM_FRAME_HEADER_LEN)
		return -1;
	
	g_chirpUsb->useBuffer((uint8_t *)codeData, CAM_FRAME_HEADER_LEN+g_votedBarcodeIndex*sizeof(DecodedBarCode)); 
	
	return 0;
}

int sendTrackedCodes(uint8_t renderFlags)
{
	SimpleListNode<Tracker<DecodedBarCode> > *i;
	
	CRP_SEND_XDATA(g_chirpUsb, HTYPE(FOURCC('B','C','0','F')), INT8(RENDER_FLAG_START), STRING("Tracked barcodes"), UINT16(CAM_RES3_WIDTH), UINT16(CAM_RES3_HEIGHT), END);

	for (i=g_barCodeTrackersList.m_first; i!=NULL; i=i->m_next)
	{
		CRP_SEND_XDATA(g_chirpUsb, HTYPE(FOURCC('B','C','0','S')), UINT8(i->m_object.m_index), UINT16(i->m_object.m_object.m_val), 
			UINT16(i->m_object.m_object.m_outline.m_xOffset), UINT16(i->m_object.m_object.m_outline.m_yOffset), UINT16(i->m_object.m_object.m_outline.m_width), UINT16(i->m_object.m_object.m_outline.m_height), END);
	}
	CRP_SEND_XDATA(g_chirpUsb, HTYPE(FOURCC('B','C','0','F')), INT8(renderFlags), STRING("Tracked barcodes"), INT16(CAM_RES3_WIDTH), INT16(CAM_RES3_HEIGHT), END);			
	
	return 0;
}

void clearGrid(const RectB rect)
{
	uint8_t i, j;
	uint16_t index;
	uint16_t height;
	int16_t r0;
	
	height = rect.m_bottom - rect.m_top;
	height *= 4;
	
	
	r0 = rect.m_top-height;
	if (r0<0)
		r0 = 0;
	for (i=r0; i<=rect.m_bottom; i++)
	{
		for (j=rect.m_left, index=i*LINE_GRID_WIDTH+rect.m_left; j<=rect.m_right; j++, index++)
			g_lineGrid[index] = 0;
	}
}

void clearGrid()
{
	// remove edges 
	int16_t val, i;
	uint16_t v, h;
	RectB rect;
	
	for (i=0; i<g_votedBarcodeIndex; i++)
	{
		h = LINE_MMC_HBOUNDARY*g_votedBarcodes[i].m_outline.m_width;
		if (h>>LINE_GRID_WIDTH_REDUCTION==0)
			h = 1<<LINE_GRID_WIDTH_REDUCTION; // ensure minimum of 1 pixel in grid
		v = LINE_MMC_VBOUNDARY*g_votedBarcodes[i].m_outline.m_height;
		if (v>>LINE_GRID_HEIGHT_REDUCTION==0)
			v = 1<<LINE_GRID_HEIGHT_REDUCTION; // ensure minimum of 1 pixel in grid
		
		val = (g_votedBarcodes[i].m_outline.m_xOffset - h)>>LINE_GRID_WIDTH_REDUCTION;
		if (val<0)
			val = 0;
		rect.m_left = val;
		
		val = (g_votedBarcodes[i].m_outline.m_xOffset + g_votedBarcodes[i].m_outline.m_width + h)>>LINE_GRID_WIDTH_REDUCTION;
		if (val>=LINE_GRID_WIDTH)
			val = LINE_GRID_WIDTH-1;
		rect.m_right = val;
		
		val = (g_votedBarcodes[i].m_outline.m_yOffset - v)>>LINE_GRID_HEIGHT_REDUCTION;
		if (val<0)
			val = 0;
		rect.m_top = val;
		
		val = (g_votedBarcodes[i].m_outline.m_yOffset + g_votedBarcodes[i].m_outline.m_height + v)>>LINE_GRID_HEIGHT_REDUCTION;
		if (val>=LINE_GRID_HEIGHT)
			val = LINE_GRID_HEIGHT-1;
		rect.m_bottom = val;
		
		clearGrid(rect); 
	}	
}


uint32_t compareLines(const Line2 &line0, const Line2 &line1)
{
	uint16_t d0, d1, d2, d3, da, db;
	uint32_t angle, df;
	
	d0 = line0.m_p0.dist2(line1.m_p0);
	d1 = line0.m_p0.dist2(line1.m_p1);
	d2 = line0.m_p1.dist2(line1.m_p0);
	d3 = line0.m_p1.dist2(line1.m_p1);
	
	da = MIN(d0, d1);
	db = MIN(d2, d3);
	
	df = da+db;
	
	// compare to max line distance
	if (df>g_maxLineCompare)
		return TR_MAXVAL;

	// test angle
	angle = tanDiffAbs1000(line0.m_p0, line0.m_p1, line1.m_p0, line1.m_p1, true);
	if (angle>g_maxTrackingTanAngle)
		return TR_MAXVAL;
		
	return da+db;
}

uint16_t handleLineTracking2()
{
	uint32_t val, min;
	uint16_t n=0;
	SimpleListNode<Tracker<Line2> > *i;
	SimpleListNode<Line2> *j;
	uint8_t k;
	Line2 *minLine;
	bool priority;
	
	// go through list, find best candidates
	for (i=g_lineTrackersList.m_first; i!=NULL; i=i->m_next)
	{
		// calculate priority -- primary lines and lines in the primary intersection
		priority = false;
		if (g_primaryActive && g_primaryLineIndex==i->m_object.m_index)
			priority = true;
		else if (g_primaryIntersection.m_state!=TR_INVALID)
		{
			for (k=0; k<g_primaryIntersection.m_object.m_n; k++)
			{
				if (g_primaryIntersection.m_object.m_lines[k].m_index==i->m_object.m_index)
				{
					priority = true;
					break;
				}
			}
		}
		// already found minimum, continue
		if (i->m_object.m_minVal!=TR_MAXVAL)
			continue;
		
		// find min
		for (j=g_linesList.m_first, min=TR_MAXVAL, minLine=NULL; j!=NULL; j=j->m_next)
		{
			val = compareLines(i->m_object.m_object, j->m_object);
			if (!priority) 
				val <<= 12; // we scale up by 4096 because this exceeds pretty much all possible compare values due to geometry
			// find minimum, but if line is already chosen, make sure we're a better match
			if (val<min && i->m_object.swappable(val, &j->m_object))
			{
				min = val;
				minLine = &j->m_object;
			}
		}
		if (minLine)
		{
			// if this minimum line already has a tracker, see which is better
			if (minLine->m_tracker)
			{
				minLine->m_tracker->resetMin(); // reset tracker pointed to by current minline
				minLine->m_tracker = &i->m_object; 
				i->m_object.setMin(minLine, min);
				n++;
			}
			else
			{
				minLine->m_tracker = &i->m_object; // update tracker pointer			
				i->m_object.setMin(minLine, min);
				n++;
			}
		}
	}
	return n;
}


void handleLineTracking()
{
	SimpleListNode<Tracker<Line2> > *i, *inext;
	SimpleListNode<Line2> *j;
	uint16_t leading, trailing;
	
	// reset tracking table
	// Note, we don't need to reset g_linesList entries (e.g. m_tracker) because these are renewed  
	for (i=g_lineTrackersList.m_first; i!=NULL; i=i->m_next)
		i->m_object.resetMin();
	
	// do search, find minimums
	while(handleLineTracking2());
	
	// go through, update tracker, remove entries that are no longer valid
	for (i=g_lineTrackersList.m_first; i!=NULL; i=inext)
	{
		inext = i->m_next;
		
		if (i->m_object.update()&TR_EVENT_INVALIDATED)
			g_lineTrackersList.remove(i); // no longer valid?  remove from tracker list
	}	
	
	// find new candidates
	for (j=g_linesList.m_first; j!=NULL; j=j->m_next)
	{
		if (j->m_object.m_tracker==NULL)
		{
			SimpleListNode<Tracker<Line2> > *n;
			leading = g_lineFiltering*LINE_FILTERING_MULTIPLIER;
			trailing = (leading+1)>>1;
			n = g_lineTrackersList.add(Tracker<Line2>(j->m_object, g_lineTrackerIndex++, leading, trailing));
			if (n==NULL)
			{
				cprintf(0, "hlt\n");
				break;
			}
			j->m_object.m_tracker = &n->m_object; // point back to tracker
		}
	}
	if (g_debug&LINE_DEBUG_TRACKING)
	{
		uint8_t n;
		cprintf(0, "Trackers\n");
		for (i=g_lineTrackersList.m_first, n=0; i!=NULL; i=i->m_next, n++)
			cprintf(0, "   %d: %d %d (%d %d)(%d %d)\n", n, i->m_object.m_index, i->m_object.m_state, 
			i->m_object.m_object.m_p0.m_x, i->m_object.m_object.m_p0.m_y, i->m_object.m_object.m_p1.m_x, i->m_object.m_object.m_p1.m_y);
	}
}

uint32_t compareBarCodes(const DecodedBarCode &c0, const DecodedBarCode &c1)
{
	int32_t x, y;
	
	// different values are different
	if (c0.m_val!=c1.m_val)
		return TR_MAXVAL;
	
	// find distance between 
	x = c0.m_outline.m_xOffset + (c0.m_outline.m_width>>1);
	y = c0.m_outline.m_yOffset + (c0.m_outline.m_height>>1);
	x -= c1.m_outline.m_xOffset + (c1.m_outline.m_width>>1);
	y -= c1.m_outline.m_yOffset + (c1.m_outline.m_height>>1);
	
	return x*x + y*y; 
}

uint16_t handleBarCodeTracking2()
{
	uint32_t val, min;
	uint16_t n=0;
	SimpleListNode<Tracker<DecodedBarCode> > *i;
	uint8_t j;
	DecodedBarCode *minCode;
	
	// go through list, find best candidates
	for (i=g_barCodeTrackersList.m_first; i!=NULL; i=i->m_next)
	{
		// already found minimum, continue
		if (i->m_object.m_minVal!=TR_MAXVAL)
			continue;
		
		// find min
		for (j=0, min=TR_MAXVAL, minCode=NULL; j<g_votedBarcodeIndex; j++)
		{
			val = compareBarCodes(i->m_object.m_object, g_votedBarcodes[j]);
			// find minimum, but if line is already chosen, make sure we're a better match
			if (val<min && i->m_object.swappable(val, &g_votedBarcodes[j]))
			{
				min = val;
				minCode = &g_votedBarcodes[j];
			}
		}
		if (minCode)
		{
			// if this minimum line already has a tracker, see which is better
			if (minCode->m_tracker)
			{
				minCode->m_tracker->resetMin(); // reset tracker pointed to by current minline
				minCode->m_tracker = &i->m_object; 
				i->m_object.setMin(minCode, min);
				n++;
			}
			else
			{
				minCode->m_tracker = &i->m_object; // update tracker pointer			
				i->m_object.setMin(minCode, min);
				n++;
			}
		}
	}
	return n;
}

void handleBarCodeTracking()
{
	SimpleListNode<Tracker<DecodedBarCode> > *i, *inext;
	uint8_t j;
	uint16_t leading, trailing;
	
	// reset tracking table
	// Note, we don't need to reset g_linesList entries (e.g. m_tracker) because these are renewed  
	for (i=g_barCodeTrackersList.m_first; i!=NULL; i=i->m_next) 
		i->m_object.resetMin();
	
	// do search, find minimums
	while(handleBarCodeTracking2());
	
	// go through, update tracker, remove entries that are no longer valid
	for (i=g_barCodeTrackersList.m_first; i!=NULL; i=inext)
	{
		inext = i->m_next;
		
		if (i->m_object.update()&TR_EVENT_INVALIDATED)
			g_barCodeTrackersList.remove(i); // no longer valid?  remove from tracker list
	}	
	
	// find new candidates
	for (j=0; j<g_votedBarcodeIndex; j++)
	{
		if (g_votedBarcodes[j].m_tracker==NULL)
		{
			SimpleListNode<Tracker<DecodedBarCode> > *n;
			leading = g_barcodeFiltering*LINE_FILTERING_MULTIPLIER;
			trailing = (leading+1)>>1;
			n = g_barCodeTrackersList.add(Tracker<DecodedBarCode>(g_votedBarcodes[j], g_barCodeTrackerIndex++, leading, trailing));
			if (n==NULL)
			{
				cprintf(0, "hlt\n");
				break;
			}
			g_votedBarcodes[j].m_tracker = &n->m_object; // point back to tracker
		}
	}
	if (g_debug&LINE_DEBUG_TRACKING)
	{
		uint8_t n;
		cprintf(0, "Barcodes\n");
		for (i=g_barCodeTrackersList.m_first, n=0; i!=NULL; i=i->m_next, n++)
			cprintf(0, "   %d: %d %d %d (%d %d %d %d)\n", n, i->m_object.m_index, i->m_object.m_state, i->m_object.m_object.m_val,
			i->m_object.m_object.m_outline.m_height, i->m_object.m_object.m_outline.m_width, 
			i->m_object.m_object.m_outline.m_xOffset, i->m_object.m_object.m_outline.m_yOffset);
	}
}

Line2 *findLine(uint8_t index)
{
	SimpleListNode<Line2> *i;
	
	for (i=g_linesList.m_first; i!=NULL; i=i->m_next)
	{
		if (i->m_object.m_tracker && i->m_object.m_tracker->m_index==index)
			return &i->m_object;
	}
	return NULL;
}


Line2 *findTrackedLine(uint8_t index)
{
	SimpleListNode<Tracker<Line2> > *i;
	
	for (i=g_lineTrackersList.m_first; i!=NULL; i=i->m_next)
	{
		if (i->m_object.m_index==index)
			return &i->m_object.m_object;
	}
	return NULL;
}

uint8_t trackedLinesWithPoint(const Point &p)
{
	uint8_t n;
	Line2 *line;
	
	SimpleListNode<Tracker<Line2> > *i;
	
	for (i=g_lineTrackersList.m_first, n=0; i!=NULL; i=i->m_next)
	{
		line = &i->m_object.m_object;
		if (line && (line->m_p0.equals(p) || line->m_p1.equals(p))) 
			n++;
	}
	return n;
}

bool compareIntersections(const Intersection &i0, const FrameIntersection &i1)
{
#if 0
	uint8_t i, n;
	
	// count valid lines
	for (i=0, n=0; i<i0.m_n; i++)
	{
		if (i0.m_lines[i]->m_object.m_tracker && i0.m_lines[i]->m_object.m_tracker->get()!=NULL)
			n++;
	}
	
	// are valid lines in i0 equal to number of lines in i1?
	return n==i1.m_n;
#else
	return i0.m_n==i1.m_n;
#endif
}

int16_t getAngle(const Line2 &line, const Point &p)
{
	float x, y;
	int16_t xdiff, ydiff, res;
	
	// find diff's using the point that's not p
	// Note that y axis is pointing down, so we invert.
	if (p.equals(line.m_p0)) 
	{
		xdiff = line.m_p1.m_x - p.m_x;
		ydiff = p.m_y - line.m_p1.m_y;
	}
	else
	{
		xdiff = line.m_p0.m_x - p.m_x;
		ydiff = p.m_y - line.m_p0.m_y;
	}
	// rotate axes 90 degrees so 0 is pointing straight up
	x = ydiff;
	y = -xdiff;
	
	res = (int16_t)(atan2(y, x)*180/(float)M_PI);
	
	return res;
}

int compareAngle(const void *a, const void *b)
{
  return (((FrameIntersectionLine *)a)->m_angle - ((FrameIntersectionLine *)b)->m_angle );
}

void formatIntersection(const Intersection &intersection, FrameIntersection *fintersection, bool tracked)
{
	uint8_t i, j;
		
	for (i=0, j=0; i<intersection.m_n && j<LINE_MAX_FRAME_INTERSECTION_LINES; i++)
	{
		// only add valid lines to fintersection
		if (!tracked || intersection.m_lines[i]->m_object.m_tracker)// && intersection.m_lines[i]->m_object.m_tracker->get()!=NULL))
		{	
			if (tracked)
				fintersection->m_lines[j].m_index = intersection.m_lines[i]->m_object.m_tracker->m_index;
			else
				fintersection->m_lines[j].m_index = 0;				
			fintersection->m_lines[j].m_angle = getAngle(intersection.m_lines[i]->m_object, intersection.m_p);   
			fintersection->m_lines[j].m_reserved = 0;
			j++;
		}
	}
	// zero-out rest of lines
	for (i=j; i<LINE_MAX_FRAME_INTERSECTION_LINES; i++)
	{	
		fintersection->m_lines[i].m_index = 0;
		fintersection->m_lines[i].m_angle = 0;   
		fintersection->m_lines[i].m_reserved = 0;
	}
	
	fintersection->m_x = intersection.m_p.m_x;
	fintersection->m_y = intersection.m_p.m_y;
	fintersection->m_n = j;
	
	// sort lines based on angles
	qsort(fintersection->m_lines, fintersection->m_n, sizeof(FrameIntersectionLine), compareAngle);
}


uint8_t updatePrimaryIntersection(SimpleListNode<Intersection> *intern)
{
	FrameIntersection fint;
	
	g_primaryIntersection.resetMin();
	
	if (intern && 
		intern->m_object.m_p.m_y>g_minLineLength*1.4 && // only consider intersections that are sufficiently far from the top
		intern->m_object.m_p.m_y<LINE_GRID_HEIGHT-g_minLineLength*1.4) // and bottom
	{
		if (g_primaryIntersection.m_state==TR_INVALID || compareIntersections(intern->m_object, g_primaryIntersection.m_object))
		{
			formatIntersection(intern->m_object, &fint, true);
			g_primaryIntersection.setMin(&fint, 0);
		}
	}
		
	return g_primaryIntersection.update();
}

#if 0
void updatePrimaryPoint(const Line2 &primary)
{
	g_primaryActive = primary.m_tracker->get()!=NULL;
	
	if (xdirection(primary.m_p0, primary.m_p1))
	{
		if (((LINE_HT_LEFT | LINE_HT_RIGHT)&g_primaryPointMap)==0)
		{
			if (g_primaryPointMap&LINE_HT_UP)
			{
				if (primary.m_p0.m_y < primary.m_p1.m_y) // p0 is primary
				{
					if (primary.m_p0.m_x < primary.m_p1.m_x) 
						g_primaryPointMap |= LINE_HT_LEFT;
					else // p0 > p1
						g_primaryPointMap |= LINE_HT_RIGHT;					
				}
				else // p1 is primary
				{
					if (primary.m_p1.m_x < primary.m_p0.m_x) 
						g_primaryPointMap |= LINE_HT_LEFT;
					else // p1 > p0
						g_primaryPointMap |= LINE_HT_RIGHT;					
				}
			}
			else // g_primaryPointMap&LINE_HT_DOWN
			{
				if (primary.m_p0.m_y > primary.m_p1.m_y) // p0 is primary 
				{
					if (primary.m_p0.m_x < primary.m_p1.m_x) 
						g_primaryPointMap |= LINE_HT_LEFT;
					else // p0 > p1
						g_primaryPointMap |= LINE_HT_RIGHT;					
				}
				else // p1 is primary
				{
					if (primary.m_p1.m_x < primary.m_p0.m_x) 
						g_primaryPointMap |= LINE_HT_LEFT;
					else // p1 > p0
						g_primaryPointMap |= LINE_HT_RIGHT;					
				}
			}
		}
		if (g_primaryPointMap&LINE_HT_LEFT)
		{
			if (primary.m_p0.m_x < primary.m_p1.m_x) // p0 is primary 
			{
				g_primaryPoint = primary.m_p0;
				g_goalPoint = primary.m_p1;
			}
			else // p1 is primary
			{
				g_primaryPoint = primary.m_p1;
				g_goalPoint = primary.m_p0;
			}
		}
		else // g_primaryPointMap&LINE_HT_RIGHT
		{
			if (primary.m_p0.m_x > primary.m_p1.m_x) // p0 is primary 
			{
				g_primaryPoint = primary.m_p0;
				g_goalPoint = primary.m_p1;
			}
			else // p1 is primary
			{
				g_primaryPoint = primary.m_p1;
				g_goalPoint = primary.m_p0;
			}
		}
	}
	else // y direction
	{
		if (((LINE_HT_UP | LINE_HT_DOWN)&g_primaryPointMap)==0)
		{
			if (g_primaryPointMap&LINE_HT_LEFT)
			{
				if (primary.m_p0.m_x < primary.m_p1.m_x) // p0 is primary
				{
					if (primary.m_p0.m_y < primary.m_p1.m_y) 
						g_primaryPointMap |= LINE_HT_UP;
					else // p0 > p1
						g_primaryPointMap |= LINE_HT_DOWN;					
				}
				else // p1 is primary
				{
					if (primary.m_p1.m_y < primary.m_p0.m_y) 
						g_primaryPointMap |= LINE_HT_UP;
					else // p1 > p0
						g_primaryPointMap |= LINE_HT_DOWN;					
				}
			}
			else // g_primaryPointMap&LINE_HT_RIGHT
			{
				if (primary.m_p0.m_x > primary.m_p1.m_x) // p0 is primary 
				{
					if (primary.m_p0.m_y < primary.m_p1.m_y) 
						g_primaryPointMap |= LINE_HT_UP;
					else // p0 > p1
						g_primaryPointMap |= LINE_HT_DOWN;					
				}
				else // p1 is primary
				{
					if (primary.m_p1.m_y < primary.m_p0.m_y) 
						g_primaryPointMap |= LINE_HT_UP;
					else // p1 > p0
						g_primaryPointMap |= LINE_HT_DOWN;					
				}
			}
		}
		if (g_primaryPointMap&LINE_HT_UP)
		{
			if (primary.m_p0.m_y < primary.m_p1.m_y) // p0 is primary 
			{
				g_primaryPoint = primary.m_p0;
				g_goalPoint = primary.m_p1;
			}
			else // p1 is primary
			{
				g_primaryPoint = primary.m_p1;
				g_goalPoint = primary.m_p0;
			}
		}
		else // g_primaryPointMap&LINE_HT_DOWN
		{
			if (primary.m_p0.m_y > primary.m_p1.m_y) // p0 is primary 
			{
				g_primaryPoint = primary.m_p0;
				g_goalPoint = primary.m_p1;
			}
			else // p1 is primary
			{
				g_primaryPoint = primary.m_p1;
				g_goalPoint = primary.m_p0;
			}
		}
	}
}
#else
void updatePrimaryPoint(const Line2 &primary)
{
	g_primaryActive = primary.m_tracker->get()!=NULL;
	
	if (xdirection(primary.m_p0, primary.m_p1))
	{
		if (g_primaryPointMap&LINE_HT_RIGHT)
		{
			g_primaryPointMap = LINE_HT_RIGHT;
			if (primary.m_p0.m_x > primary.m_p1.m_x) // p0 is the primary point
			{
				g_primaryPoint = primary.m_p0;
				g_goalPoint = primary.m_p1;
				// update UP/DOWN flag
				if (primary.m_p0.m_y > primary.m_p1.m_y)
					g_primaryPointMap |= LINE_HT_DOWN;
			}
			else // p1 is the primary point
			{
				g_primaryPoint = primary.m_p1;
				g_goalPoint = primary.m_p0;
				// update UP/DOWN flag
				if (primary.m_p1.m_y > primary.m_p0.m_y)
					g_primaryPointMap |= LINE_HT_DOWN;
			}
		}
		else // LINE_HT_LEFT
		{
			g_primaryPointMap = LINE_HT_LEFT;
			if (primary.m_p0.m_x < primary.m_p1.m_x) // p0 is the primary point
			{
				g_primaryPoint = primary.m_p0;
				g_goalPoint = primary.m_p1;
				// update UP/DOWN flag
				if (primary.m_p0.m_y > primary.m_p1.m_y)
					g_primaryPointMap |= LINE_HT_DOWN;
			}
			else // p1 is the primary point
			{
				g_primaryPoint = primary.m_p1;
				g_goalPoint = primary.m_p0;
				// update UP/DOWN flag
				if (primary.m_p1.m_y > primary.m_p0.m_y)
					g_primaryPointMap |= LINE_HT_DOWN;
			}
		}				
	}
	else // y direction
	{
		if (g_primaryPointMap&LINE_HT_DOWN)
		{
			g_primaryPointMap = LINE_HT_DOWN;
			if (primary.m_p0.m_y > primary.m_p1.m_y) // p0 is the primary point
			{
				g_primaryPoint = primary.m_p0;
				g_goalPoint = primary.m_p1;
				// update LEFT/RIGHT flag
				if (primary.m_p0.m_x > primary.m_p1.m_x)
					g_primaryPointMap |= LINE_HT_RIGHT;
			}
			else // p1 is the primary point
			{
				g_primaryPoint = primary.m_p1;
				g_goalPoint = primary.m_p0;
				// update LEFT/RIGHT flag
				if (primary.m_p1.m_x > primary.m_p0.m_x)
					g_primaryPointMap |= LINE_HT_RIGHT;
			}
		}
		else // LINE_HT_UP
		{
			g_primaryPointMap = LINE_HT_UP;
			if (primary.m_p0.m_y < primary.m_p1.m_y) // p0 is the primary point
			{
				g_primaryPoint = primary.m_p0;
				g_goalPoint = primary.m_p1;
				// update LEFT/RIGHT flag
				if (primary.m_p0.m_x > primary.m_p1.m_x)
					g_primaryPointMap |= LINE_HT_RIGHT;
			}
			else // p1 is the primary point
			{
				g_primaryPoint = primary.m_p1;
				g_goalPoint = primary.m_p0;
				// update LEFT/RIGHT flag
				if (primary.m_p1.m_x > primary.m_p0.m_x)
					g_primaryPointMap |= LINE_HT_RIGHT;
			}
		}
	}
}
#endif

int intersectionTurn()
{
	uint8_t k, mink;
	uint16_t angle, minAngle;
	Line2 *primary;
		
	// find line that best fits our desired turn angle (g_nextTurnAngle), ie, find minimum difference
	for (k=0, minAngle=0xffff; k<g_primaryIntersection.m_object.m_n; k++)
	{
		// calculate difference between angles
		angle = ABS(g_primaryIntersection.m_object.m_lines[k].m_angle - g_nextTurnAngle);
		if (angle<minAngle)
		{
			minAngle = angle;
			mink = k;
		}
	}
	// assign new primary line
	g_primaryLineIndex = g_primaryIntersection.m_object.m_lines[mink].m_index;
	primary = findTrackedLine(g_primaryLineIndex);
	// set g_primaryPointMap
	if (g_primaryIntersection.m_object.m_x==primary->m_p0.m_x && g_primaryIntersection.m_object.m_y==primary->m_p0.m_y) // primary point is p0
	{
		g_primaryPoint = primary->m_p0;
		g_goalPoint = primary->m_p1;
	}
	else // primary point is p1
	{
		g_primaryPoint = primary->m_p1;
		g_goalPoint = primary->m_p0;
	}
	if (xdirection(primary->m_p0, primary->m_p1))
	{
		if (g_primaryPoint.m_x<g_goalPoint.m_x)
			g_primaryPointMap = LINE_HT_LEFT;
		else
			g_primaryPointMap = LINE_HT_RIGHT;							
	}
	else // y direction
	{
		if (g_primaryPoint.m_y<g_goalPoint.m_y)
			g_primaryPointMap = LINE_HT_UP;
		else
			g_primaryPointMap = LINE_HT_DOWN;							
	}
	g_primaryActive = true;
	g_primaryIntersection.m_state = TR_INVALID; // invalidate intersection
	
	// reset angle to default angle
	g_nextTurnAngle = g_defaultTurnAngle;
	g_newTurnAngle = false;
	
	// we always succeed by choosing the closest line
	return 0;	
}

void setPrimaryVector(uint8_t index)
{
	Line2 *line;
	
	line = findTrackedLine(index);
	if (line)
	{
		// choose point lower in the image as the primary point
		g_primaryPointMap = LINE_HT_DOWN;
					
		g_primaryLineIndex = index;
		g_primaryIntersection.m_state = TR_INVALID;
		g_lineState = LINE_STATE_TRACKING;
		// update 
		updatePrimaryPoint(*line);
	}
}

void handleLineState()
{
	uint8_t ymax;
	SimpleListNode<Tracker<Line2> > *i, *max;
	Line2 *line, *primary;
	uint8_t events;
	
	if (g_lineState==LINE_STATE_ACQUIRING)
	{
		if (g_manualVectorSelect) // // wait for user to select
		{
			if (g_manualVectorSelecIndextActive)
			{
				setPrimaryVector(g_manualVectorSelectIndex);
				g_manualVectorSelecIndextActive = false;
			}
			
			return;
		}
		// find line that meets our criteria -- it should be at least 45 degrees past horizontal and reasonably long
		// we will consider lines that are lower in the image first.  The first line to fit the criterial is our line.
		
		// reset 
		for (i=g_lineTrackersList.m_first; i!=NULL; i=i->m_next)
			i->m_object.resetMin();

		while(1)
		{
			for (i=g_lineTrackersList.m_first, ymax=0, max=NULL; i!=NULL; i=i->m_next)
			{
				line = i->m_object.get();
				if (line!=NULL && i->m_object.m_minVal==TR_MAXVAL)
				{
					if (line->m_p0.m_y>ymax)
					{
						ymax = line->m_p0.m_y;
						max = i;
					}
					else if (line->m_p1.m_y>ymax)
					{	
						ymax = line->m_p1.m_y;
						max = i;
					}
				}
			}
			if (max)
			{
				//angle = tanAbs1000(max->m_object.m_object.m_p0, max->m_object.m_object.m_p1);
				//if (angle>=LINE_MIN_ACQUISITION_TAN_ANGLE && max->m_object.m_object.length2()>=LINE_MIN_ACQUISITION_LENGTH2)
				if (max->m_object.m_object.length2()>=LINE_MIN_ACQUISITION_LENGTH2)
				{
					//cprintf("acq %d %d %d %d %d %d\n", max->m_object.m_index, angle, 
					//	max->m_object.m_object.m_p0.m_x, max->m_object.m_object.m_p0.m_y, max->m_object.m_object.m_p1.m_x, max->m_object.m_object.m_p1.m_y);
					
					setPrimaryVector(max->m_object.m_index);
					break;
				}			
				max->m_object.m_minVal = 0; // flag that we've visited
			}
			else
			{
				//cprintf("null\n");
				break;
			}
		}
	}
	else // LINE_STATE_TRACKING
	{
		primary = findTrackedLine(g_primaryLineIndex);
		
		if (primary)
		{
			if (g_reversePrimary)
			{
				g_reversePrimary = false;
				if (g_primaryPointMap&LINE_HT_UP)
				{
					g_primaryPointMap &= ~LINE_HT_UP;
					g_primaryPointMap |= LINE_HT_DOWN;
				}
				else if (g_primaryPointMap&LINE_HT_DOWN)
				{
					g_primaryPointMap &= ~LINE_HT_DOWN;
					g_primaryPointMap |= LINE_HT_UP;
				}
				if (g_primaryPointMap&LINE_HT_RIGHT)
				{
					g_primaryPointMap &= ~LINE_HT_RIGHT;
					g_primaryPointMap |= LINE_HT_LEFT;
				}
				else if (g_primaryPointMap&LINE_HT_LEFT)
				{
					g_primaryPointMap &= ~LINE_HT_LEFT;
					g_primaryPointMap |= LINE_HT_RIGHT;
				}
			}
			// deal with primary point 
			updatePrimaryPoint(*primary);
			
			if (g_debug&LINE_DEBUG_TRACKING)
				cprintf(0, "Primary %d %d (%d %d)(%d %d)\n", g_primaryLineIndex, g_primaryPointMap, 
				g_primaryPoint.m_x, g_primaryPoint.m_y, g_goalPoint.m_x, g_goalPoint.m_y);
			
			// Find primary again in list of lines.  Note if we find "primary" here, it will be the same line as "primary" above.  
			// The only difference is this line will always have a valid intersection.  The "otherIndex" and "otherPoint" found 
			// above will apply here also. 
			primary = findLine(g_primaryLineIndex);
			if (primary)
			{
				if (primary->m_p0.equals(g_goalPoint)) // look for intersection at i0
					events = updatePrimaryIntersection(primary->m_i0); 
				else // look for intersection at i1
					events = updatePrimaryIntersection(primary->m_i1);
				
				// as soon as we see the g_primaryIntersection state transition to valid...
				if (events&TR_EVENT_VALIDATED)
				{
					g_newIntersection = true;
					if (g_debug&LINE_DEBUG_TRACKING)
						cprintf(0, "New intersection\n");
					
					// if turn angle is set before intersection, go ahead and choose turn now
					if (!g_delayedTurn)
						intersectionTurn();
					else // LINE_TURNMODE_DURING_INTERSECTION, we need to reset g_newTurnAngle, so we don't accidentally turn
						g_newTurnAngle = false;
				}
				// deal with turn, if turn mode dictates that choose turn during the intersection
				else if (g_newTurnAngle && g_delayedTurn && g_primaryIntersection.m_state!=TR_INVALID)
					intersectionTurn();
					
				if (events&TR_EVENT_INVALIDATED)
					g_newIntersection = false;

				if ((g_debug&LINE_DEBUG_TRACKING) && g_primaryIntersection.m_state!=TR_INVALID)
					cprintf(0, "Intersection %d (%d %d)\n", g_primaryIntersection.m_state, g_primaryIntersection.m_object.m_x, g_primaryIntersection.m_object.m_y);
			}							
		}
		else // we lost the line
			g_lineState = LINE_STATE_ACQUIRING;
		
	}
}


int line_processMain()
{
	static uint32_t n = 0;
	uint32_t i;
	uint32_t len, tlen;
	bool eof, error;
	int8_t row;
	uint8_t vstate[LINE_VSIZE];
	uint32_t timer;
	SimpleList<uint32_t> timers;
	SimpleListNode<uint32_t> *j;	

	// send frame and data over USB 
	if (g_renderMode!=LINE_RM_MINIMAL)
		cam_sendFrame(g_chirpUsb, CAM_RES3_WIDTH, CAM_RES3_HEIGHT, 
			RENDER_FLAG_BLEND, FOURCC('4', '0', '1', '4'));

	// indicate start of edge data
	if (g_debug&LINE_DEBUG_LAYERS)
		CRP_SEND_XDATA(g_chirpUsb, HTYPE(FOURCC('E','D','G','F')), HINT8(RENDER_FLAG_START), HINT16(CAM_RES3_WIDTH), HINT16(CAM_RES3_HEIGHT), END);
	
	// initialize variables
	g_lineIndex = 1; // set to 1 because 0 means empty...
	g_lineSegIndex = 0;
	g_barcodeIndex = 0;
	memset(vstate, 0, LINE_VSIZE);
	memset(g_lineGrid, 0, LINE_GRID_WIDTH*LINE_GRID_HEIGHT*sizeof(LineGridNode));
	
	
	if (g_debug==LINE_DEBUG_BENCHMARK)
		timers.clear(); // need to clear timers because of repeat flag, leads to memory leak
	
	if (g_debug&LINE_DEBUG_TRACKING)
		cprintf(0, "Frame %d ______\n", n++);
	
	checkGraph(__LINE__);

	if (g_repeat)
		*g_equeue->m_fields = g_savedEqueue;
	else
		g_savedEqueue = *g_equeue->m_fields;
	
	setTimer(&timer);
	for (i=0, row=-1, tlen=0; true; i++)
	{
		while((len=g_equeue->readLine(g_lineBuf, LINE_BUFSIZE, &eof, &error))==0)
		{	
			if (getTimer(timer)>100000)
			{
				error = true;
				cprintf(0, "line hang\n");
				goto outside;
			}
		}
		tlen += len;
		if (g_lineBuf[0]==EQ_HSCAN_LINE_START)
		{
			row++;
			detectCodes(row, g_lineBuf+1, len-1);
			line_hLine(row, g_lineBuf+1, len-1);
		}
		else if (g_lineBuf[0]==EQ_VSCAN_LINE_START)
			line_vLine(row, vstate, g_lineBuf+1, len-1);

		if (g_debug&LINE_DEBUG_LAYERS)
			CRP_SEND_XDATA(g_chirpUsb, HTYPE(FOURCC('E','D','G','S')), UINTS16(len, g_lineBuf), END);
		if (eof || error)
			break;
	}
	
	outside:
	// indicate end of edge data
	if (g_debug&LINE_DEBUG_LAYERS)
		CRP_SEND_XDATA(g_chirpUsb, HTYPE(FOURCC('E','D','G','F')), 
			HINT8(0), HINT16(CAM_RES3_WIDTH), HINT16(CAM_RES3_HEIGHT), END);


	if (g_debug==LINE_DEBUG_BENCHMARK)
		timers.add(getTimer(timer));

	if (g_debug==LINE_DEBUG_BENCHMARK)
		setTimer(&timer);
	clusterCodes();
	if (g_debug==LINE_DEBUG_BENCHMARK)
		timers.add(getTimer(timer));

	clearGrid();
	
	g_linesList.clear();
	g_nodesList.clear();
	g_nadirsList.clear();
	g_intersectionsList.clear();
	
	if (error) // deal with error after we call clustercodes otherwise there's a memory leak
	{
		cprintf(0, "error\n");
		g_equeue->flush();
		
		return -1;
	}
		
	g_allMutex = true;
	g_primaryMutex = true;
	handleBarCodeTracking();
	g_primaryMutex = false;
	
	if (g_debug&LINE_DEBUG_LAYERS)
		sendCodes(0);
	
	sendTrackedCodes(RENDER_FLAG_BLEND);
	
	if (g_debug&LINE_DEBUG_LAYERS)
		line_sendLineGrid(0);
	
	if (g_debug==LINE_DEBUG_BENCHMARK)
		setTimer(&timer);
	extractLineSegments();
	if (g_debug==LINE_DEBUG_BENCHMARK)
		timers.add(getTimer(timer));
	
	if (g_debug&LINE_DEBUG_LAYERS)
	{
		sendLineSegments(0);
		sendPoints(g_nodesList, 0, "nodes");
	}
	
	if (g_debug==LINE_DEBUG_BENCHMARK)
		setTimer(&timer);
	findNadirs();
	if (g_debug==LINE_DEBUG_BENCHMARK)
		timers.add(getTimer(timer));
	
	if (g_debug&LINE_DEBUG_LAYERS)
		sendNadirs(g_nadirsList, 0, "nadir pairs");
	
	if (g_debug==LINE_DEBUG_BENCHMARK)
		setTimer(&timer);
	reduceNadirs();
	if (g_debug==LINE_DEBUG_BENCHMARK)
		timers.add(getTimer(timer));

	checkGraph(__LINE__);

	if (g_debug==LINE_DEBUG_BENCHMARK)
		setTimer(&timer);
	formIntersections();
	if (g_debug==LINE_DEBUG_BENCHMARK)
		timers.add(getTimer(timer));

	checkGraph(__LINE__);

	if (g_debug&LINE_DEBUG_LAYERS)
	{
		sendNadirs(g_nadirsList, 0, "merged nadirs");
		sendLines(g_linesList, 0, "pre-cleaned lines");
		sendIntersections(g_intersectionsList, 0, "pre-cleaned intersections");
	}

	if (g_debug==LINE_DEBUG_BENCHMARK)
		setTimer(&timer);
	cleanIntersections();
	if (g_debug==LINE_DEBUG_BENCHMARK)
		timers.add(getTimer(timer));
		
	checkGraph(__LINE__);

    removeMinLines(g_minLineLength2);
	
	if (g_debug&LINE_DEBUG_TRACKING)
	{
		SimpleListNode<Line2> *ln;
		SimpleListNode<Intersection> *in;
		uint32_t k;
	
		cprintf(0, "Lines\n");
		for (ln=g_linesList.m_first, k=0; ln!=NULL; ln=ln->m_next, k++)
			cprintf(0, "  %d: (%d %d %x)(%d %d %x)\n", k, ln->m_object.m_p0.m_x, ln->m_object.m_p0.m_y, ln->m_object.m_i0, ln->m_object.m_p1.m_x, ln->m_object.m_p1.m_y, ln->m_object.m_i1);
		cprintf(0, "Intersections\n");
		for (in=g_intersectionsList.m_first, k=0; in!=NULL; in=in->m_next, k++)
			cprintf(0, " %d: %d (%d %d) %x\n", k, in->m_object.m_n, in->m_object.m_p.m_x, in->m_object.m_p.m_y, in);
	}
	
	checkGraph(__LINE__);

	if (g_debug&LINE_DEBUG_LAYERS)
		sendLines(g_linesList, 0, "lines");
		
	if (g_debug==LINE_DEBUG_BENCHMARK)
		setTimer(&timer);
	handleLineTracking();
	g_allMutex = false;
	if (g_debug==LINE_DEBUG_BENCHMARK)
		timers.add(getTimer(timer));
	
	if (g_renderMode==LINE_RM_ALL_FEATURES|| (g_debug&LINE_DEBUG_LAYERS))
		sendTrackedLines(g_lineTrackersList, RENDER_FLAG_BLEND, "filtered lines");

	if (g_renderMode==LINE_RM_ALL_FEATURES)
		sendIntersections(g_intersectionsList, RENDER_FLAG_BLEND, "intersections");
	else if (g_debug&LINE_DEBUG_LAYERS)
		sendIntersections(g_intersectionsList, 0, "intersections");
	
	g_primaryMutex = true;
	handleLineState();
	g_primaryMutex = false;	
	
	if (g_debug==LINE_DEBUG_BENCHMARK)
	{
		for (j=timers.m_first, i=0, timer=0; j!=NULL; j=j->m_next, i++)
		{
			timer += j->m_object;
			cprintf(0, "timer %d: %d\n", i, j->m_object);
		}
		cprintf(0, "total: %d\n", timer);
	}

	sendPrimaryFeatures(RENDER_FLAG_BLEND);

	// render whatever we've sent
    exec_sendEvent(g_chirpUsb, EVT_RENDER_FLUSH);
	
	g_frameFlag = true;
	
	return 0;
}

int line_process()
{
	if (!g_repeat)
	{
		if (g_renderMode!=LINE_RM_MINIMAL)
		{
			SM_OBJECT->currentLine = 0;
			SM_OBJECT->stream = 1; // set streaming

			// wait for frame
			while(SM_OBJECT->currentLine < CAM_RES3_HEIGHT-1)
			{
				// After grab starts, reset streaming so we don't accidentally grab the next frame.
				// If we accidentally grab the next frame, we get overruns in the equeue, etc. 
				// This only happens when we're running PixyMon (streaming over USB) and we're 
				// getting hammered with serial interrupts for communication.  This would sometimes
				// lead to this loop erroring -- we miss currentline and it wraps around.
				if (SM_OBJECT->currentLine>1)
					SM_OBJECT->stream = 0;
			}
		}
		else
			SM_OBJECT->stream = 1; // set streaming					
	}
		
	return line_processMain();
}

int line_setRenderMode(uint8_t mode)
{
	g_renderMode = mode;
	if (mode>LINE_RM_ALL_FEATURES)
		return -1;
	return 0;
}

uint8_t line_getRenderMode()
{
	return g_renderMode;
}


int line_getPrimaryFrame(uint8_t typeMap, uint8_t *buf, uint16_t len)
{
	uint16_t length = 0;
	
	// deal with g_frameFlag -- return error when it's false, indicating no new data
	if (!g_frameFlag || g_primaryMutex)
		return -1; // no new data, or busy
	g_frameFlag = false;
	
	// only return primary line if we're tracking and primary line is in active (valid) state
	if (g_lineState==LINE_STATE_TRACKING && g_primaryActive)
	{
		// we assume that we can fit all 3 features in a single packet (255 bytes)
		if (typeMap&LINE_FR_VECTOR_LINES)
		{
			// line information is always present
			FrameLine *line;
			*(uint8_t *)(buf + length) = LINE_FR_VECTOR_LINES;
			*(uint8_t *)(buf + length + 1) = sizeof(FrameLine);
			line = (FrameLine *)(buf + length + 2);
			line->m_x0 = g_primaryPoint.m_x;
			line->m_y0 = g_primaryPoint.m_y;
			line->m_x1 = g_goalPoint.m_x;
			line->m_y1 = g_goalPoint.m_y;
			line->m_index = g_primaryLineIndex; 
			line->m_flags = 0;
			if (g_intersectionsList.m_size)
				line->m_flags |= LINE_FR_FLAG_INTERSECTION;
			length += sizeof(FrameLine) + 2;
		}
		// Intersection information, only present when intersection appears
		if ((typeMap&LINE_FR_INTERSECTION) && g_newIntersection)
		{
			*(uint8_t *)(buf + length) = LINE_FR_INTERSECTION;
			*(uint8_t *)(buf + length + 1) = sizeof(FrameIntersection);
			memcpy(buf+length+2, &g_primaryIntersection.m_object, sizeof(FrameIntersection));
			length += sizeof(FrameIntersection) + 2;
			
			g_newIntersection = false;
		}
	}
	if (typeMap&LINE_FR_BARCODE)
	{
		SimpleListNode<Tracker<DecodedBarCode> > *j;
	
		// go through list, find best candidates
		for (j=g_barCodeTrackersList.m_first; j!=NULL; j=j->m_next)
		{
			if (j->m_object.m_events&TR_EVENT_VALIDATED && !(j->m_object.m_eventsShadow&TR_EVENT_VALIDATED))
			{
				FrameCode *barcode;
	
				*(uint8_t *)(buf + length) = LINE_FR_BARCODE;
				*(uint8_t *)(buf + length + 1) = sizeof(FrameCode);
				barcode = (FrameCode *)(buf + length + 2);
				barcode->m_code = j->m_object.m_object.m_val;
				barcode->m_flags = 0;
				// return center location of code
				barcode->m_x = (j->m_object.m_object.m_outline.m_xOffset + (j->m_object.m_object.m_outline.m_width>>1))>>LINE_GRID_WIDTH_REDUCTION;
				barcode->m_y = (j->m_object.m_object.m_outline.m_yOffset + (j->m_object.m_object.m_outline.m_height>>1))>>LINE_GRID_HEIGHT_REDUCTION;
				length += sizeof(FrameCode) + 2;
				// set shadow so we don't report again
				j->m_object.m_eventsShadow |= TR_EVENT_VALIDATED;
				// only 1 code per frame
				break; 
			}
		}
	}
	return length;
}

int line_getAllFrame(uint8_t typeMap, uint8_t *buf, uint16_t len)
{
	uint16_t length = 0;
	uint8_t plength, *hbuf;
	
	if (!g_frameFlag || g_allMutex)
		return -1; // no new data, or busy
	g_frameFlag = false;
	if (typeMap&LINE_FR_VECTOR_LINES)
	{
		SimpleListNode<Tracker<Line2> > *n;
		Line2 *line;
		FrameLine *fline;
		
		for(n=g_lineTrackersList.m_first, plength=0, hbuf=buf+length; n!=NULL && length<len-sizeof(FrameLine)-2; n=n->m_next)
		{
			fline = (FrameLine *)(buf + length + 2);
			line = &n->m_object.m_object;
			fline->m_x0 = line->m_p0.m_x;
			fline->m_y0 = line->m_p0.m_y;
			fline->m_x1 = line->m_p1.m_x;
			fline->m_y1 = line->m_p1.m_y;
			fline->m_index = n->m_object.m_index;
			fline->m_flags = n->m_object.m_state;
				
			length += sizeof(FrameLine);
			plength += sizeof(FrameLine);
		}
		if (plength>0)
		{
			*(uint8_t *)hbuf = LINE_FR_VECTOR_LINES;
			*(uint8_t *)(hbuf+1) = plength;
			length += 2;
		}
	}
	if (typeMap&LINE_FR_INTERSECTION)
	{
		SimpleListNode<Intersection> *i;
		FrameIntersection *intersection;
		
		for (i=g_intersectionsList.m_first, plength=0, hbuf=buf+length; i!=NULL && length<len-sizeof(FrameIntersection)-2; i=i->m_next)
		{
			intersection = (FrameIntersection *)(buf + length + 2);
			formatIntersection(i->m_object, intersection, true); 
				
			length += sizeof(FrameIntersection);
			plength += sizeof(FrameIntersection);			
		}
		if (plength>0)
		{
			*(uint8_t *)hbuf = LINE_FR_INTERSECTION;
			*(uint8_t *)(hbuf+1) = plength;
			length += 2;
		}
	}
	if (typeMap&LINE_FR_BARCODE)
	{
		SimpleListNode<Tracker<DecodedBarCode> > *j;
		DecodedBarCode *dcode;
		FrameCode *barcode;
		
		// go through list, find best candidates
		for (j=g_barCodeTrackersList.m_first, plength=0, hbuf=buf+length; j!=NULL && length<len-sizeof(FrameCode)-2; j=j->m_next)
		{
			dcode = &j->m_object.m_object;
			barcode = (FrameCode *)(buf + length + 2);
			barcode->m_x = (dcode->m_outline.m_xOffset + (dcode->m_outline.m_width>>1))>>LINE_GRID_WIDTH_REDUCTION;
			barcode->m_y = (dcode->m_outline.m_yOffset + (dcode->m_outline.m_height>>1))>>LINE_GRID_HEIGHT_REDUCTION;
			barcode->m_flags = j->m_object.m_state;
			barcode->m_code = dcode->m_val;
				
			length += sizeof(FrameCode);
			plength += sizeof(FrameCode);
		}
		if (plength>0)
		{
			*(uint8_t *)hbuf = LINE_FR_BARCODE;
			*(uint8_t *)(hbuf+1) = plength;
			length += 2;
		}
	}
	return length;
}

int line_setMode(int8_t modeMap)
{
	g_delayedTurn = (modeMap & LINE_MODEMAP_TURN_DELAYED) ? true : false;
	g_whiteLine = (modeMap & LINE_MODEMAP_WHITE_LINE) ? true : false;
	g_manualVectorSelect = (modeMap & LINE_MODEMAP_MANUAL_SELECT_VECTOR) ? true : false;
	return 0;
}

int line_setNextTurnAngle(int16_t angle)
{	
	g_nextTurnAngle = angle;
	g_newTurnAngle = true;
		
	// else we've just set the angle (for later)	
	return 0;
}

int line_setDefaultTurnAngle(int16_t angle)
{	
	g_defaultTurnAngle = angle;
		
	// else we've just set the angle (for later)	
	return 0;
}

int line_setVector(uint8_t index)
{
	g_manualVectorSelecIndextActive = true;
	g_manualVectorSelectIndex = index;
	
	return 0;
}

int line_reversePrimary()
{
	// can't reverse primary if it's not active
	if (!g_primaryActive)
		return -1;
	
	g_reversePrimary = true;
	return 0;
}

int line_legoLineData(uint8_t *buf, uint32_t buflen)
{
#if 0
	buf[0] = 1;
	buf[1] = 2;
	buf[2] = 3;
	buf[3] = 4;
	
#else
	SimpleListNode<Tracker<DecodedBarCode> > *j;
	uint8_t codeVal;
	uint16_t maxy;
	Line2 *primary;
	uint32_t x;
	static uint8_t lastData[4];

	// override these because LEGO mode doesn't support 
	//sg_delayedTurn = false;
	g_manualVectorSelect = false;
	
	if (g_allMutex || !g_frameFlag) 
	{
		memcpy(buf, lastData, 4); // use last data
		return 4; // busy or no new eata
	}
	
	g_frameFlag = false;
	
	buf[2] = (uint8_t)-1;
	
	// only return primary line if we're tracking and primary line is in active (valid) state
	if (g_lineState==LINE_STATE_TRACKING && g_primaryActive)
	{
		x = (g_goalPoint.m_x *128)/78; // scale to 0 to 128
		buf[0] = x;
		if (g_goalPoint.m_y > g_primaryPoint.m_y)
			buf[3] = 1;
		else
			buf[3] = 0;
	}
	else
	{
		buf[0] = (uint8_t)-1;
		buf[3] = 0;
	}

	for (j=g_barCodeTrackersList.m_first, maxy=0, codeVal=(uint8_t)-1; j!=NULL; j=j->m_next)
	{
		if (j->m_object.m_events&TR_EVENT_VALIDATED)
		{
			if (maxy < j->m_object.m_object.m_outline.m_yOffset)
			{
				maxy = j->m_object.m_object.m_outline.m_yOffset;
				codeVal = j->m_object.m_object.m_val;
			}
		}
	}
	buf[1] = codeVal;
	
	
	primary = findLine(g_primaryLineIndex);
	if (primary)
	{			
		if (primary->m_i1)
			buf[2] = primary->m_i1->m_object.m_n;
		else if (primary->m_i0)
			buf[2] = primary->m_i0->m_object.m_n;
	}
	if (g_newIntersection)
		g_newIntersection = false;
	
	memcpy(lastData, buf, 4);
#endif
	
	return 4;
}


