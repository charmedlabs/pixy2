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
#include "progvideo.h"
#include "pixy_init.h"
#include "camera.h"
#include "blobs.h"
#include "conncomp.h"
#include "param.h"
#include "led.h"
#include "smlink.hpp"
#include "misc.h"
#include "pixyvals.h"
#include <string.h>

REGISTER_PROG(ProgVideo, PROG_NAME_VIDEO, "continuous stream of raw camera frames");

ProgVideo::ProgVideo(uint8_t progIndex)
{	
	if (g_execArg==0)
		cam_setMode(CAM_MODE0);
	else
		cam_setMode(CAM_MODE1);

	// run m0 
	exec_runM0(1);
	SM_OBJECT->currentLine = 0;
	SM_OBJECT->stream = 1;
}

ProgVideo::~ProgVideo()
{
	exec_stopM0();
}

int ProgVideo::loop(char *status)
{
	while(SM_OBJECT->currentLine < CAM_RES2_HEIGHT-2)
	{
		if (SM_OBJECT->stream==0)
			printf("not streaming\n");
	}
	SM_OBJECT->stream = 0; // pause after frame grab is finished
	
	// send over USB 
	if (g_execArg==0)
		cam_sendFrame(g_chirpUsb, CAM_RES2_WIDTH, CAM_RES2_HEIGHT);
	else
		sendCustom();
	// resume streaming
	SM_OBJECT->currentLine = 0;
	SM_OBJECT->stream = 1; // resume streaming

	return 0;
}

void ProgVideo::sendCustom(uint8_t renderFlags)
{
	uint32_t fourcc;

	// cooked mode
	if (g_execArg==1) 
		cam_sendFrame(g_chirpUsb, CAM_RES2_WIDTH, CAM_RES2_HEIGHT, RENDER_FLAG_FLUSH, FOURCC('C','M','V','2'));
	//  experimental mode, for new monmodules, etc.
	else if (100<=g_execArg && g_execArg<200) 
	{
		fourcc = FOURCC('E','X',((g_execArg%100)/10 + '0'), ((g_execArg%10) + '0'));
		cam_sendFrame(g_chirpUsb, CAM_RES2_WIDTH, CAM_RES2_HEIGHT, RENDER_FLAG_FLUSH, fourcc);
	}
	// undefined, just send plain raw frame (BA81)
	else 
		cam_sendFrame(g_chirpUsb, CAM_RES2_WIDTH, CAM_RES2_HEIGHT);

}



