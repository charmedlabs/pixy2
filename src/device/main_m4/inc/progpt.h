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

#ifndef _PROGPT_H
#define _PROGPT_H

#include "exec.h"
#include "blobs.h"

#define PAN_AXIS    0
#define TILT_AXIS   1
#define X_CENTER    (CAM_RES2_WIDTH/2)
#define Y_CENTER    (CAM_RES2_HEIGHT/2)

#define PROG_NAME_PANTILT  "pan_tilt_demo"
class ServoLoop
{
public:
	ServoLoop(uint8_t axis, uint32_t pgain=0, uint32_t dgain=0);

	void update(int32_t error);
	void reset();
	void setGains(int32_t pgain, int32_t dgain);

private:
	int32_t m_pos;
	int32_t m_prevError;
	uint8_t m_axis;
	int32_t m_pgain;
	int32_t m_dgain;
};

class ProgPt : public Prog
{
public:
	ProgPt(uint8_t progIndex);
	virtual ~ProgPt();
	
	virtual int loop(char *status);
	virtual int getAction(uint16_t index, const char **name, const char **scriptlet);
	virtual int packet(uint8_t type, const uint8_t *data, uint8_t len, bool checksum);

	static void acquire();
	static BlobA *track();

private:
	static void shadowCallback(const char *id, const uint32_t &val);
	static void getVals();
	
	static ServoLoop m_panLoop;
	static ServoLoop m_tiltLoop;

	static int16_t m_index;
};



#endif
