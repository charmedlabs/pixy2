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

#include "progpt.h"
#include "progblobs.h"
#include "pixy_init.h"
#include "camera.h"
#include "qqueue.h"
#include "spi.h"
#include "rcservo.h"
#include "cameravals.h"
#include "conncomp.h"
#include "param.h"
#include "smlink.hpp"
#include "exec.h"

extern Qqueue *g_qqueue;
extern Blobs *g_blobs;


#ifndef LEGO
REGISTER_PROG(ProgPt, PROG_NAME_PANTILT, "perform pan/tilt tracking", PROG_BLOBS_MIN_TYPE, PROG_BLOBS_MAX_TYPE);
#endif

ServoLoop ProgPt::m_panLoop(PAN_AXIS);
ServoLoop ProgPt::m_tiltLoop(TILT_AXIS);

int16_t ProgPt::m_index = -1;

void ProgPt::shadowCallback(const char *id, const uint32_t &val)
{
	getVals();
}

void ProgPt::getVals()
{
	int32_t pgain, dgain; 
	
	prm_get("Pan P gain", &pgain, END);
	prm_get("Pan D gain", &dgain, END);
	m_panLoop.setGains(pgain, dgain);

	prm_get("Tilt P gain", &pgain, END);
	prm_get("Tilt D gain", &dgain, END);
	m_tiltLoop.setGains(pgain, dgain);
}

ProgPt::ProgPt(uint8_t progIndex)
{ 
	prm_add("Pan P gain", PROG_FLAGS(progIndex) | PRM_FLAG_SIGNED | PRM_FLAG_SLIDER, PRM_PRIORITY_5+4,
		"@c Tuning @m -1500 @M 1500 Pan axis proportional gain (default 350)", INT32(350), END);
	prm_setShadowCallback("Pan P gain", (ShadowCallback)ProgPt::shadowCallback);
	prm_add("Pan D gain", PROG_FLAGS(progIndex) | PRM_FLAG_SIGNED | PRM_FLAG_SLIDER, PRM_PRIORITY_5+3, 
		"@c Tuning @m -1500 @M 1500 Pan axis derivative gain (default 600)", INT32(600), END);
	prm_setShadowCallback("Pan D gain", (ShadowCallback)ProgPt::shadowCallback);
	prm_add("Tilt P gain", PROG_FLAGS(progIndex) | PRM_FLAG_SIGNED | PRM_FLAG_SLIDER, PRM_PRIORITY_5+2, 
		"@c Tuning @m -1500 @M 1500 Tilt axis proportional gain (default 500)", INT32(500), END);
	prm_setShadowCallback("Tilt P gain", (ShadowCallback)ProgPt::shadowCallback);
	prm_add("Tilt D gain", PROG_FLAGS(progIndex) | PRM_FLAG_SIGNED | PRM_FLAG_SLIDER, PRM_PRIORITY_5+1, 
		"@c Tuning @m -1500 @M 1500 Tilt axis derivative gain (default 700)", INT32(700), END);
	prm_setShadowCallback("Tilt D gain", (ShadowCallback)ProgPt::shadowCallback);
	
	getVals();	
	
	m_view = 0;
	ProgBlobs::staticInit(progIndex);
}

ProgPt::~ProgPt()
{
	ProgBlobs::staticExit();
	
	rcs_setPos(PAN_AXIS, 500);
	rcs_setPos(TILT_AXIS, 500);
}

void ProgPt::acquire()
{
	BlobA *blob;
	
	blob = g_blobs->getMaxBlob();
	if (blob && blob->m_tracker && blob->m_tracker->m_age>30)
		m_index = blob->m_tracker->m_index;
}
	
BlobA *ProgPt::track()
{
	SimpleList<Tracker<BlobA> > *blobsList;
	SimpleListNode<Tracker<BlobA> > *i;	
	
	// create blobs
	g_blobs->blobify();

	blobsList = g_blobs->getBlobs();

	if (m_index==-1)
		acquire();

	if (m_index>=0)
	{
		for (i=blobsList->m_first; i!=NULL; i=i->m_next)
		{
			if (i->m_object.m_index==m_index)
				return &i->m_object.m_object;
		}
		m_index = -1; // invalidate 
	}
	return NULL;
}

	
int ProgPt::loop(char *status)
{
	int32_t panError, tiltError;
	uint16_t x, y;
	BlobA *blob;
	SimpleList<Tracker<BlobA> > *blobsList;

	if (ProgBlobs::handleButton(status))
		return 1; // 1 indicates override state

	SM_OBJECT->stream = 0; // don't capture raw frames, so we can double framerate

	blob = track();
	
	if (blob)
	{
		x = blob->m_left + (blob->m_right - blob->m_left)/2;
		y = blob->m_top + (blob->m_bottom - blob->m_top)/2;

		panError = X_CENTER-x;
		tiltError = y-Y_CENTER;

		m_panLoop.update(panError);
		m_tiltLoop.update(tiltError);
	}

	// send blobs
	blobsList = g_blobs->getBlobs();
	cc_sendBlobs(g_chirpUsb, blobsList);

	cc_setLED();
	
	return 0;
}

int ProgPt::getAction(uint16_t index, const char **name, const char **scriptlet)
{
	return ProgBlobs::staticGetAction(index, name, scriptlet);
}

int ProgPt::packet(uint8_t type, const uint8_t *data, uint8_t len, bool checksum)
{
	return ProgBlobs::staticPacket(type, data, len, checksum);
}


ServoLoop::ServoLoop(uint8_t axis, uint32_t pgain, uint32_t dgain)
{
	m_axis = axis;
	m_pgain = pgain;
	m_dgain = dgain;
	m_prevError = 0x80000000;
	reset();
}

void ServoLoop::update(int32_t error)
{
	int32_t vel;

	if (m_prevError!=0x80000000)
	{	
		vel = (error*m_pgain + (error - m_prevError)*m_dgain)/1000;
		m_pos += vel;
		if (m_pos>RCS_MAX_POS) 
			m_pos = RCS_MAX_POS; 
		else if (m_pos<RCS_MIN_POS) 
			m_pos = RCS_MIN_POS;

		rcs_setPos(m_axis, m_pos);
		//cprintf("%d %d %d\n", m_axis, m_pos, vel);
	}
	m_prevError = error;
}

void ServoLoop::reset()
{
	m_pos = RCS_CENTER_POS;
	rcs_setPos(m_axis, m_pos);
}

void ServoLoop::setGains(int32_t pgain, int32_t dgain)
{
	m_pgain = pgain;
	m_dgain = dgain;	
}





