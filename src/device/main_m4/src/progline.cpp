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

#include <stdio.h>
#include "progline.h"
#include "equeue.h"
#include "pixy_init.h"
#include "camera.h"
#include "conncomp.h"
#include "param.h"
#include "line.h"
#include "smlink.hpp"
#include "misc.h"
#include "pixyvals.h"
#include "exec.h"
#include "serial.h"
#include <string.h>

REGISTER_PROG(ProgLine, PROG_NAME_LINE, "perform line tracking", PROG_LINE_MIN_TYPE, PROG_LINE_MAX_TYPE);

const char *ProgLine::m_views[] =
{
	LINE_RM_MINIMAL_STR,
	LINE_RM_PRIMARY_FEATURES_STR,
	LINE_RM_ALL_FEATURES_STR,
};

ProgLine::ProgLine(uint8_t progIndex)
{
	cam_setMode(CAM_MODE1);	
	line_open(progIndex);
	// setup qqueue and M0
	exec_runM0(2);
	
	// if view is valid, set it (default view is set in line_open
	if (m_view>=0)
		setView(m_view);	
	
}

ProgLine::~ProgLine()
{
	exec_stopM0();	
	line_close();
}
	
int ProgLine::loop(char *status)
{
	line_process();

	return 0;
}

int ProgLine::getView(uint16_t index, const char **name)
{
	uint16_t n = sizeof(m_views)/sizeof(char *);

	if (index>=n)
		return -1;

	*name = m_views[index];

	return index==line_getRenderMode(); // return 1 if it's the current view, 0 otherwise		
}

int ProgLine::setView(uint16_t index)
{
	return line_setRenderMode(index);
}

int ProgLine::packet(uint8_t type, const uint8_t *data, uint8_t len, bool checksum)
{
	int res=SER_ERROR_INVALID_REQUEST; 
	
	if (type==TYPE_REQUEST_GET_FEATURES)
	{
		if (len==2) // one byte in request
			sendLineData(*(uint8_t *)data, *(uint8_t *)(data+1), checksum);
		else
			ser_sendError(SER_ERROR_INVALID_REQUEST, checksum);
			
		return 0;
	}
	else if (type==TYPE_REQUEST_SET_MODE)
	{
		if (len==1)
			res = line_setMode(*(int8_t *)data);
		if (res>=0)
			ser_sendResult(res, checksum);
		else
			ser_sendError(res, checksum);
		return 0;
	}
	else if (type==TYPE_REQUEST_SET_NEXT_TURN_ANGLE)
	{
		if (len==2)
			res = line_setNextTurnAngle(*(int16_t *)data);
		if (res>=0)
			ser_sendResult(res, checksum);
		else
			ser_sendError(res, checksum);
		return 0;
	}
	else if (type==TYPE_REQUEST_SET_DEFAULT_TURN_ANGLE)
	{
		if (len==2)
			res = line_setDefaultTurnAngle(*(int16_t *)data);
		if (res>=0)
			ser_sendResult(res, checksum);
		else
			ser_sendError(res, checksum);
		return 0;
	}
	else if (type==TYPE_REQUEST_SET_VECTOR)
	{
		if (len==1)
			res = line_setVector(*(int8_t *)data);
		if (res>=0)
			ser_sendResult(res, checksum);
		else
			ser_sendError(res, checksum);		
		return 0;
	}
	else if (type==TYPE_REQUEST_REVERSE_VECTOR)
	{
		if (len==0)
			res = line_reversePrimary();
		if (res>=0)
			ser_sendResult(res, checksum);
		else
			ser_sendError(res, checksum);
		return 0;
	}		
		
	// nothing rings a bell, return error
	return -1;
}


void ProgLine::getResolution(uint16_t *width, uint16_t *height, uint8_t type)
{
	*width = LINE_GRID_WIDTH;
	*height = LINE_GRID_HEIGHT;
}

void ProgLine::sendLineData(uint8_t requestType, uint8_t typeMap, bool checksum)
{
	uint8_t *txData;
	uint32_t len;
	int res;

	len = ser_getTx(&txData);
	
	if (requestType==GET_PRIMARY_FEATURES)
		res = line_getPrimaryFrame(typeMap, txData, len);
	else //if (requestType==GET_ALL_FEATURES)
		res = line_getAllFrame(typeMap, txData, len);
	
	if (res<0)
		ser_sendError(SER_ERROR_BUSY, checksum);
	else
		ser_setTx(TYPE_RESPONSE_GET_FEATURES, res, checksum);
}


