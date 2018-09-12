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
#include "progblobs.h"
#include "pixy_init.h"
#include "camera.h"
#include "led.h"
#include "conncomp.h"
#include "serial.h"
#include "rcservo.h"
#include "exec.h"
#include "serial.h"
#include "misc.h"
#include "smlink.hpp"
#include "button.h"
#include "calc.h"

REGISTER_PROG(ProgBlobs, PROG_NAME_BLOBS, "perform color connected components analysis", PROG_BLOBS_MIN_TYPE, PROG_BLOBS_MAX_TYPE);

uint8_t ProgBlobs::m_state;
uint32_t ProgBlobs::m_timer;
uint8_t ProgBlobs::m_buttonState;
uint8_t ProgBlobs::m_index;
bool ProgBlobs::m_ledPipe;
uint8_t ProgBlobs::renderState;

#define VIEW_BLOCKS                 0
#define VIEW_BLOCKS_VIDEO           1
#define VIEW_BLOCKS_VIDEO_PIXELS    2

#define SA_GAIN                     0.015f
#define G_GAIN                      1.10f

const char *ProgBlobs::m_views[] =
{
	"Blocks",
	"Blocks, video",
	"Blocks, video, detected pixels"
};

const ActionScriptlet ProgBlobs::m_actions[]=
{
	{
	"Set signature 1...", 
	"cam_getFrame 0x21 0 0 " STRINGIFY(CAM_RES2_WIDTH) " " STRINGIFY(CAM_RES2_HEIGHT) "\n"
    "cc_setSigRegion 0 1\n"
	"run\n"
	},
	{
	"Set signature 2...", 
	"cam_getFrame 0x21 0 0 " STRINGIFY(CAM_RES2_WIDTH) " " STRINGIFY(CAM_RES2_HEIGHT) "\n"
    "cc_setSigRegion 0 2\n"
	"run\n"
	},
	{
	"Set signature 3...", 
	"cam_getFrame 0x21 0 0 " STRINGIFY(CAM_RES2_WIDTH) " " STRINGIFY(CAM_RES2_HEIGHT) "\n"
    "cc_setSigRegion 0 3\n"
	"run\n"
	},
	{
	"Set signature 4...", 
	"cam_getFrame 0x21 0 0 " STRINGIFY(CAM_RES2_WIDTH) " " STRINGIFY(CAM_RES2_HEIGHT) "\n"
    "cc_setSigRegion 0 4\n"
	"run\n"
	},
	{
	"Set signature 5...", 
	"cam_getFrame 0x21 0 0 " STRINGIFY(CAM_RES2_WIDTH) " " STRINGIFY(CAM_RES2_HEIGHT) "\n"
    "cc_setSigRegion 0 5\n"
	"run\n"
	},
	{
	"Set signature 6...", 
	"cam_getFrame 0x21 0 0 " STRINGIFY(CAM_RES2_WIDTH) " " STRINGIFY(CAM_RES2_HEIGHT) "\n"
    "cc_setSigRegion 0 6\n"
	"run\n"
	},
	{
	"Set signature 7...", 
	"cam_getFrame 0x21 0 0 " STRINGIFY(CAM_RES2_WIDTH) " " STRINGIFY(CAM_RES2_HEIGHT) "\n"
    "cc_setSigRegion 0 7\n"
	"run\n"
	},

	{
	"Set CC signature 1...", 
	"cam_getFrame 0x21 0 0 " STRINGIFY(CAM_RES2_WIDTH) " " STRINGIFY(CAM_RES2_HEIGHT) "\n"
    "cc_setSigRegion 1 1\n"
	"run\n"
	},
	{
	"Set CC signature 2...", 
	"cam_getFrame 0x21 0 0 " STRINGIFY(CAM_RES2_WIDTH) " " STRINGIFY(CAM_RES2_HEIGHT) "\n"
    "cc_setSigRegion 1 2\n"
	"run\n"
	},
	{
	"Set CC signature 3...", 
	"cam_getFrame 0x21 0 0 " STRINGIFY(CAM_RES2_WIDTH) " " STRINGIFY(CAM_RES2_HEIGHT) "\n"
    "cc_setSigRegion 1 3\n"
	"run\n"
	},
	{
	"Set CC signature 4...", 
	"cam_getFrame 0x21 0 0 " STRINGIFY(CAM_RES2_WIDTH) " " STRINGIFY(CAM_RES2_HEIGHT) "\n"
    "cc_setSigRegion 1 4\n"
	"run\n"
	},
	{
	"Set CC signature 5...", 
	"cam_getFrame 0x21 0 0 " STRINGIFY(CAM_RES2_WIDTH) " " STRINGIFY(CAM_RES2_HEIGHT) "\n"
    "cc_setSigRegion 1 5\n"
	"run\n"
	},
	{
	"Set CC signature 6...", 
	"cam_getFrame 0x21 0 0 " STRINGIFY(CAM_RES2_WIDTH) " " STRINGIFY(CAM_RES2_HEIGHT) "\n"
    "cc_setSigRegion 1 6\n"
	"run\n"
	},
	{
	"Set CC signature 7...", 
	"cam_getFrame 0x21 0 0 " STRINGIFY(CAM_RES2_WIDTH) " " STRINGIFY(CAM_RES2_HEIGHT) "\n"
    "cc_setSigRegion 1 7\n"
	"run\n"
	},
	{
	"Clear all signatures", 
    "cc_clearAllSig\n"
	"run\n"
	}
};


void ProgBlobs::staticInit(uint8_t progIndex)
{
	uint8_t c;
	
	// setup camera mode
	cam_setMode(CAM_MODE1);
	
	// open connected components code 
	cc_open();
	
	// setup qqueue and M0
	SM_OBJECT->streamState = 0;
	g_qqueue->flush();
	exec_runM0(0);

	renderState = 0;

	// flush serial receive queue
	while(ser_getSerial()->receive(&c, 1));

	m_state = 0; // reset recv state machine
	resetButtonState(); 
	
	// if m_view is invalid, set default view
	if (m_view<0)
		staticSetView(VIEW_BLOCKS_VIDEO);
}

void ProgBlobs::staticExit()
{
	exec_stopM0();
	cc_close();
}

void ProgBlobs::resetButtonState()
{
	m_buttonState = 0;
	m_index = 0;
	m_ledPipe = false;
	led_set(0);
	led_setMaxCurrent(g_ledBrightness); // restore default brightness
}

// this routine needs to provide good feedback to the user as to whether the camera sees and segments the object correctly.
// The key is to integrate the "growing algorithm" such that the growing algorithm is executed continuously and feedback about
// the "goodness" of the grown region is returned.  We're choosing goodness to be some combination of size (because the bigger 
// the grown region the better) and saturation (because more saturation the more likely we've found the intended target, vs 
// the background, which is typically not saturated.)  
// In general with an RGB LED, you can only communicate 2 things--- brightness and hue, so you have 2 dof to play with....   
void ProgBlobs::scaleLED(uint32_t r, uint32_t g, uint32_t b, uint32_t n)
{
	uint32_t max, min, current, sat; 

#if 0  // it seems that green is a little attenuated on this sensor
	uint32_t t = (uint32_t)(G_GAIN*g);
	if (t>255)
		g = 255;
	else
		g = t;
#endif

   	// find min
	min = MIN(r, g);
	min = MIN(min, b);

	// find max
	max = MAX(r, g);
	max = MAX(max, b);

	// subtract min and form saturation from the distance from origin
	sat = sqrt((float)((r-min)*(r-min) + (g-min)*(g-min) + (b-min)*(b-min)));
	if (sat>30) // limit saturation to prevent things from getting too bright
		sat = 30;
	if (sat<10) // anything less than 15 is pretty uninteresting, no sense in displaying....
		current = 0;
	else
	{
		//sat2 = exp(sat/13.0f);
		//current = (uint32_t)(SAT_GAIN*sat2) + (uint32_t)(AREA_GAIN*n) + (uint32_t)(SA_GAIN*n*sat2);
		current = (uint32_t)(SA_GAIN*n*sat);
	}
	if (current>LED_MAX_CURRENT/5)
		current = LED_MAX_CURRENT/5;
	led_setMaxCurrent(current);

#if 0
	// find reasonable bias to subtract out
	bias = min*75/100;
	r -= bias;
	g -= bias;
	b -= bias;
	
	// saturate
	m = 255.0f/(max-bias);
	r = (uint8_t)(m*r);
	g = (uint8_t)(m*g);
	b = (uint8_t)(m*b);
#endif
#if 1
	// saturate
	rgbUnpack(saturate(rgbPack(r, g, b)), &r, &g, &b);
#endif
	//cprintf("r %d g %d b %d min %d max %d sat %d sat2 %d n %d\n", r, g, b, min, max, sat, sat2, n);
	led_setRGB(r, g, b);	 	
}
	

void ProgBlobs::ledPipe()
{
	Points points;
	RGBPixel rgb;
	uint32_t color, r, g, b, n;
	g_blobs->m_clut.growRegion(g_rawFrame, Point16(CAM_RES2_WIDTH/2, CAM_RES2_HEIGHT/2), &points);	
	cc_sendPoints(points, CL_GROW_INC, CL_GROW_INC, g_chirpUsb);

	IterPixel ip(g_rawFrame, &points);
	color = ip.averageRgb(&n);

	rgbUnpack(color, &r, &g, &b);
	scaleLED(r, g, b, n);
}

void ProgBlobs::handleButtonStatus(char *str)
{
	if (str==NULL)
		return;
	
	if (m_buttonState<3)
		sprintf(str, "Waiting for button selection");
	else if (m_ledPipe)
		sprintf(str, "Signature teaching mode for signature %d, press button again to finish.", m_index);
	else
		sprintf(str, "Setting white balance, press button again to exit.");
}

void ProgBlobs::setSignature()
{
	int res;

	// grow region, create model, save
	res = cc_setSigPoint(0, m_index, CAM_RES2_WIDTH/2, CAM_RES2_HEIGHT/2);
	if (res<0)
		return;
	exec_sendEvent(g_chirpUsb, EVT_PARAM_CHANGE);
	flashLED(m_index, 4); 
}


bool ProgBlobs::handleButton(char *status)
{
	uint32_t bt;
	bool res;
	
	bt = button();

   	if (m_ledPipe) // if ledpipe, grab frame, but don't flush 
	{
		cam_getFrameChirpFlags(CAM_GRAB_M1R2, 0, 0, CAM_RES2_WIDTH, CAM_RES2_HEIGHT, g_chirpUsb, RENDER_FLAG_BLEND);
		ledPipe();
	}
	else if (m_buttonState!=0) // else grab frame and flush
		cam_getFrameChirpFlags(CAM_GRAB_M1R2, 0, 0, CAM_RES2_WIDTH, CAM_RES2_HEIGHT, g_chirpUsb);

	switch(m_buttonState)
	{
	case 0:  // wait for button press
		if (bt)
		{
			setTimer(&m_timer);
			led_setMaxCurrent(g_ledBrightness); // restore default brightness
			m_buttonState = 1;
			led_set(0);
		}
		break;

	case 1: // wait for button timeout
		if (!bt)
			m_buttonState = 0;
		else if (getTimer(m_timer)>BT_INITIAL_BUTTON_TIMEOUT)
		{
			if (cam_getAWB())
				m_index = 1;
			else
				m_index = 0;
			setTimer(&m_timer);
			setLED(m_index);
			m_buttonState = 2;
		}
		break;

	case 2: // wait and increment index 
		if (!bt)
		{
			flashLED(m_index, 3);
			setTimer(&m_timer);
			if (m_index==0)
				cam_setAWB(1);
			else
				m_ledPipe = true;
			m_buttonState = 3;
		}
		else if (getTimer(m_timer)>BT_INDEX_CYCLE_TIMEOUT)
		{
			setTimer(&m_timer);
			m_index++;
			if (m_index==CL_NUM_SIGNATURES+1)
				m_index = 0;

			setLED(m_index);
		}							   
		break;

	case 3: // wait for button down
		if (bt)
		{
			setTimer(&m_timer);
			m_buttonState = 4;
		}
		else if (getTimer(m_timer)>BT_LIGHTPIPE_TIMEOUT) // abort
			resetButtonState();
		break;

	case 4: // wait for button up
		if (!bt)
		{
			if (m_index==0)
			{
				cam_setAWB(0);
				flashLED(m_index, 4); 
			}
			else
				setSignature();
			resetButtonState(); // done	
		}
		else if (getTimer(m_timer)>BT_INITIAL_BUTTON_TIMEOUT)
		{
 			if (m_index==0)
				cam_setAWB(0);

			resetButtonState();
			m_buttonState = 5;
		}
	 	break;

	case 5: // wait for button up only
		if (!bt)
			resetButtonState();
		break;

	default:
		resetButtonState();
	}	
	
	res = m_buttonState!=0;
	
	if (res)
		handleButtonStatus(status);

	return res; 
}

	
int ProgBlobs::staticLoop(char *status)
{
	static uint32_t drop = 0;
	SimpleList<Tracker<BlobA> > *blobsList;

	// handle button-teach 
	if (handleButton(status))
		return 1; // 1 indicates override state

	if (status==NULL) // no gui
		SM_OBJECT->stream = 0; // don't capture raw frames, so we can double framerate
	else
	{		
		if (m_view==VIEW_BLOCKS)
			SM_OBJECT->stream = 0; // don't capture raw frames, so we can double framerate
		else
			SM_OBJECT->stream = 1; // capture raw frames in addition to blocks
	}
	
	// create blobs
	g_blobs->sendDetectedPixels(m_view==VIEW_BLOCKS_VIDEO_PIXELS && renderState);
	if (g_blobs->blobify()<0)
	{
		DBG("drop %d\n", drop++);
		// unhang M0 
		SM_OBJECT->streamState = 0;
		return 0;
	}
	// handle received data immediately
	if (g_interface!=SER_INTERFACE_LEGO && g_oldProtocol)
		handleRecv();

	// send blobs
	blobsList = g_blobs->getBlobs();
	if (SM_OBJECT->stream)
	{	
		if (renderState) // renderState prevents us from rendering an isolate blobs image (no background image)
		{
			cc_sendBlobs(g_chirpUsb, blobsList, RENDER_FLAG_BLEND | RENDER_FLAG_FLUSH);
			renderState = 0;
		}
	}
	else // no background, render blobs image by itself
		//cc_sendBlobs(g_chirpUsb, blobs, numBlobs, ccBlobs, numCCBlobs, RENDER_FLAG_FLUSH);
		cc_sendBlobs(g_chirpUsb, blobsList, RENDER_FLAG_FLUSH);
				
	cc_setLED();
			
	if (SM_OBJECT->stream)
	{
		// when we're streaming raw video, we could get in a state where there is stale qqueue data, so we flush
		g_qqueue->flush(); 
		// wait for state==1
		while(SM_OBJECT->streamState==0);
		// send frame over USB 
		cam_sendFrame(g_chirpUsb, CAM_RES2_WIDTH, CAM_RES2_HEIGHT, RENDER_FLAG_BLEND, FOURCC('B','A','8','1'));
		renderState = 1; // indicate that we've rendered backgound image
		SM_OBJECT->streamState = 0;
	}
	
	return 0;
}

int ProgBlobs::staticGetView(uint16_t index, const char **name)
{
	uint16_t n = sizeof(m_views)/sizeof(char *);

	if (index>=n)
		return -1;

	*name = m_views[index];

	return index==m_view; // return 1 if it's the current view, 0 otherwise		
}

int ProgBlobs::staticSetView(uint16_t index)
{
	uint16_t n = sizeof(m_views)/sizeof(char *);

	if (index>=n)
		return -1;
	
	if (m_view!=VIEW_BLOCKS)
	{
		SM_OBJECT->stream = 1; // capture raw frames in addition to blocks
		delayms(50); // wait for the frame data to come in
	}
	
	return 0;
}

int ProgBlobs::staticGetAction(uint16_t index, const char **name, const char **scriptlet)
{
	int n = sizeof(m_actions)/sizeof(ActionScriptlet);

	// check exec-based actions first
	if (index<n)
	{
		*name = m_actions[index].action;
		*scriptlet = m_actions[index].scriptlet;
		return 0;
	}
	
	return -1;
}

int ProgBlobs::staticPacket(uint8_t type, const uint8_t *data, uint8_t len, bool checksum)
{
	if (type==TYPE_REQUEST_GETBLOBS)
	{
		if (len==2)
			blobsAssemble(data[0], data[1], checksum);
		else
			ser_sendError(SER_ERROR_INVALID_REQUEST, checksum);
			
		return 0;
	}
	
	// nothing rings a bell, return error
	return -1;
}



// legacy code....
void ProgBlobs::handleRecv()
{
	uint8_t i, a;
	static uint16_t w=0xffff;
	static uint8_t lastByte;
	uint16_t s0, s1;
	Iserial *serial = ser_getSerial();

	for (i=0; i<10; i++)
	{
		switch(m_state)
		{	
		case 0: // reset 
			lastByte = 0xff;  // This is not part of any of the sync word most significant bytes
			m_state = 1;
		 	break;

		case 1:	// sync word
			if(serial->receive(&a, 1))
			{
				w = lastByte << 8;
				w |= a;
				lastByte = a;
				m_state = 2;	// compare
			}
			break;

		case 2:	 // receive data byte(s)
			if (w==SYNC_SERVO)
			{	// read rest of data
				if (serial->receiveLen()>=4)
				{
					serial->receive((uint8_t *)&s0, 2);
					serial->receive((uint8_t *)&s1, 2);

					//cprintf("servo %d %d\n", s0, s1);
					rcs_setPos(0, s0);
					rcs_setPos(1, s1);

					m_state = 0;
				}
			}
			else if (w==SYNC_CAM_BRIGHTNESS)
			{
				if(serial->receive(&a, 1))
				{
					cam_setBrightness(a);
					m_state = 0;
				}
			}
			else if (w==SYNC_SET_LED)
			{
				if (serial->receiveLen()>=3)
				{
					uint8_t r, g, b;
					serial->receive(&r, 1);
					serial->receive(&g, 1);
					serial->receive(&b, 1);

					// set override, user is now in control
					cc_setLEDOverride(true);
					led_setRGB(r, g, b);
					//cprintf("%x %x %x\n", r, g ,b);

					m_state = 0;
				}
			}
			else 
				m_state = 1; // try another word, but read only a byte
			break;

		default:
			m_state = 0; // try another whole word
			break;
		}
	}
}



void ProgBlobs::blobsAssemble(uint8_t sigmap, uint8_t n, bool checksum)
{
	uint8_t *txData;
	int res;
	uint32_t len;
	
	// bogus request
	if (sigmap==0)
	{
		ser_sendError(SER_ERROR_INVALID_REQUEST, checksum);
		return;
	}
	
	len = ser_getTx(&txData);
	
	res = g_blobs->getBlobs(sigmap, n, txData, len);

	if (res<0)
		ser_sendError(SER_ERROR_BUSY, checksum);
	else
		ser_setTx(TYPE_RESPONSE_GETBLOBS, res, checksum);
}


