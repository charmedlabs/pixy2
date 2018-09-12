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

#ifndef _PROGLINE_H
#define _PROGLINE_H

#include "exec.h"

#define PROG_LINE_MIN_TYPE                       0x30
#define PROG_LINE_MAX_TYPE                       0x3f

#define TYPE_REQUEST_GET_FEATURES                0x30
#define TYPE_RESPONSE_GET_FEATURES               0x31
#define TYPE_REQUEST_SET_MODE                    0x36
#define TYPE_REQUEST_SET_VECTOR                  0x38
#define TYPE_REQUEST_SET_NEXT_TURN_ANGLE         0x3a
#define TYPE_REQUEST_SET_DEFAULT_TURN_ANGLE      0x3c
#define TYPE_REQUEST_REVERSE_VECTOR              0x3e

#define GET_PRIMARY_FEATURES                     0x00
#define GET_ALL_FEATURES                         0x01

#define PROG_NAME_LINE               "line_tracking"

class ProgLine : public Prog
{
public:
	ProgLine(uint8_t progIndex);
	virtual ~ProgLine();
	
	virtual int loop(char *status);
	virtual int getView(uint16_t index, const char **name);
	virtual int setView(uint16_t index);
	virtual int packet(uint8_t type, const uint8_t *data, uint8_t len, bool checksum);
	virtual void getResolution(uint16_t *width, uint16_t *height, uint8_t type);

private:
	static const char *m_views[];	
	void sendLineData(uint8_t requestType, uint8_t typeMap, bool checksum);
};


#endif
