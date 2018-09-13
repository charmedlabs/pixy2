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

#include "debug.h"
#include <pixyvals.h>
#include "chirp.h"
#include "exec_m0.h"
#include "rls_m0.h"
#include "qqueue.h"
#include "smlink.h"
#include "frame_m0.h"
#include "rls_m0.h"

uint8_t g_running = 0;
uint8_t g_run = 0;
int8_t g_program = -1;

void setTimer(uint32_t *timer)
{
	*timer = LPC_TIMER2->TC;
}

uint32_t getTimer(uint32_t timer)
{
	uint32_t result; 
	result = LPC_TIMER2->TC-timer;	

	return result;
}

void delayus(uint32_t us)
{
	uint32_t timer;
	
	setTimer(&timer);
	
	while(getTimer(timer)<us);
}

int exec_init(void)
{
	chirpSetProc("run", (ProcPtr)exec_run);
	chirpSetProc("stop", (ProcPtr)exec_stop);
	chirpSetProc("running", (ProcPtr)exec_running);
		
	return 0;	
}

uint32_t exec_running(void)
{
	return (uint32_t)g_running;
}

int32_t exec_stop(void)
{
	g_run = 0;
	return 0;
}

int32_t exec_run(uint8_t *prog)
{
	g_program = *prog;
	g_run = 1;
	g_running = 1;		
	return 0;
}

#define LUT_MEMORY_SIZE		0x1000 // bytes

void setup0()
{
}

uint32_t g_m0mem = SRAM1_LOC + SRAM1_SIZE - LUT_MEMORY_SIZE - (MAX_NEW_QVALS_PER_LINE*sizeof(Qval) + CAM_RES2_WIDTH*8 + 64); 
uint32_t g_lut = SRAM1_LOC + SRAM1_SIZE - LUT_MEMORY_SIZE;

void loop0()
{
	if (SM_OBJECT->stream)
	{
		if (SM_OBJECT->streamState==0)
		{
			getRLSFrame(&g_m0mem, &g_lut);	
			grabM0R2(0, 0, CAM_RES2_WIDTH, CAM_RES2_HEIGHT, (uint8_t *)SRAM1_LOC+CAM_PREBUF_LEN);
			SM_OBJECT->streamState = 1;
		}
		// else wait
	}
	else
		getRLSFrame(&g_m0mem, &g_lut);	
}

void setup1()
{
	SM_OBJECT->stream = 1;
	SM_OBJECT->currentLine = 0;
	SM_OBJECT->frameTime = 0;
	SM_OBJECT->blankTime = 0;
}

void loop1()
{
	if (SM_OBJECT->stream)
		grabM0R2(0, 0, CAM_RES2_WIDTH, CAM_RES2_HEIGHT, (uint8_t *)SRAM1_LOC+CAM_PREBUF_LEN);
}

void setup2()
{
	setup1();
}

void loop2()
{
	if (SM_OBJECT->stream)
		grabM0R3((uint8_t *)SRAM1_LOC+CAM_PREBUF_LEN);
}


void exec_loop(void)
{
#if 0
#include "frame_m0.h"
	uint32_t line, frame=0;
	while(1)
	{
		while(!g_run)
			chirpService();

		setup0();
		while(g_run)
		{
			line=0;
			while(CAM_VSYNC())
			{
				//while(!CAM_HSYNC()&&CAM_VSYNC());
				//while(CAM_HSYNC()&&CAM_VSYNC());
				//line++;
			} 
			while(!CAM_VSYNC());
			frame++;
			if (frame%100==0)
			{
				_DBD32(frame); 
				_DBG(" ");
				//_DBD32(line);
				_DBG("\n");
			}
		}
		g_running = 0;
	}
#endif
#if 0
	uint32_t i = 0;
	while(1)
	{
		while(!g_run)
			chirpService();

		setup0();
		while(g_run)
		{
			loop0();
			i++;
			if (i%100==0)
			{
				_DBD32(i); _DBG("\n");
			}
			chirpService();
		}
		// set variable to indicate we've stopped
		g_running = 0;
	}
#endif
#if 1
	while(1)
	{
		while(!g_run)
			chirpService();
		 	
		if (g_program==0)
			setup0();
		else if (g_program==1)
			setup1();
		else
			setup2();
		
		while(g_run)
		{
			if (g_program==0)
				loop0();
			else if (g_program==1)
				loop1();
			else
				loop2();
			
			// find missing vsync transitions
			trackVsync();			
			
			chirpService();
		}
		// set variable to indicate we've stopped
		g_running = 0;
	}
#endif
}
