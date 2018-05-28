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

#include "equeue.h"
#include "pixyvals.h"

struct EqueueFields *g_equeue = (struct EqueueFields *)EQ_LOC;

uint32_t eq_enqueue(uint16_t val)
{
    if (eq_free()>0)
    {
        g_equeue->data[g_equeue->writeIndex++] = val;
        g_equeue->produced++;
		if (g_equeue->writeIndex==EQ_MEM_SIZE)
			g_equeue->writeIndex = 0;
        return 1;
    }
    return 0;
}

uint16_t eq_free(void)
{
    uint16_t len = g_equeue->produced - g_equeue->consumed;
	return EQ_MEM_SIZE-len;
} 
