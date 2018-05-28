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

#include <math.h>
#include <stdio.h>
#include "pixy_init.h"
#include "button.h"
#include "camera.h"
#include "blobs.h"
#include "led.h"
#include "misc.h"
#include "colorlut.h"
#include "conncomp.h"
#include "exec.h"
#include "calc.h"


static uint8_t g_goto;
static uint8_t g_index;
static uint32_t g_timer;

void reset();

bool selectProgram(uint8_t progs, uint8_t *selectedProg)
{
	uint32_t bt; 

	if (progs<=1)
		return 0;

	while(1)
	{
		bt = button();

		switch(g_goto)
		{
		case 0:  // wait for nothing
			setTimer(&g_timer);
			g_index = *selectedProg + 1; // start at red
			g_goto = 1;
			setLED(g_index);
			break;

		case 1:	// wait for button down
			if (bt)
			{
				setTimer(&g_timer);
				setLED(g_index);
				g_goto = 2;
			}
			else if (getTimer(g_timer)>BT_PROG_TIMEOUT)
			{
				g_index = *selectedProg+1;
				flashLED(g_index, 4); 
				reset();
				return false;
			}
			break;

		case 2: // cycle through choices, wait for button up
			if (!bt)
			{
				*selectedProg = g_index-1; // save g_index
				flashLED(g_index, 4); 
				reset(); // resets g_index
				return true;
			}
			else if (getTimer(g_timer)>BT_INDEX_CYCLE_TIMEOUT)
			{
				setTimer(&g_timer);
				g_index++;
				if (g_index==progs+1)
					g_index = 1;

				setLED(g_index);
			}							   
			break;

		default:
			reset();
		}
	}
}

void reset()
{
	g_index = 0;
	led_set(0);
	g_goto = 0;
}

void flashLED(uint8_t colorIndex, uint8_t flashes)
{
	 int i;

	 for (i=0; i<flashes; i++)
	 {
		led_set(0);
		delayus(BT_FLASH_TIMEOUT); // flash for just a little bit
		led_set(g_colors[colorIndex]);
		delayus(BT_FLASH_TIMEOUT); // flash for just a little bit
	 }
	 	
}


void setLED(uint8_t colorIndex)
{
	if (colorIndex>7)
		return;

	led_set(0);
	delayus(BT_FLASH_TIMEOUT); // flash for just a little bit
	led_set(g_colors[colorIndex]);
}

			
