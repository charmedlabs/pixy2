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

#include "debug_frmwrk.h"
#include "chirp.h"
#include "frame_m0.h"
#include "assembly.h"
#include "exec_m0.h"
#include "smlink.h"
#include "equeue.h"

//#define DEBUG_SYNC
#define CAM_PCLK_MASK   0x2000

#define ALIGN(v, n)  ((uint32_t)v&((n)-1) ? ((uint32_t)v&~((n)-1))+(n) : (uint32_t)v)

uint16_t g_lineStore[3*CAM_RES2_WIDTH+16];
uint32_t g_timer;
uint16_t g_hblank, g_hactive;
uint16_t g_vblank, g_vactive;
uint32_t g_vprev;

void vsync()
{
	int v = 0, h = 0;

	while(1)
	{
		h = 0;
		while(CAM_VSYNC()!=0);
		while(1) // vsync low
		{
			while(CAM_HSYNC()==0)
			{
				if (CAM_VSYNC()!=0)
					goto end;
			}
			while(CAM_HSYNC()!=0); //grab data
			h++;
		}
end:
		v++;
		//if (v%25==0)
			//printf("%d %d\n", v, h);
	}
}


_ASM_FUNC void sync(uint32_t *gpioIn, uint32_t clkMask, uint32_t *gpioBit, uint32_t bitMask)
{
	_ASM_START
	_ASM(PUSH	{r4-r6})
_ASM_LABEL(start)
	_ASM(LDR 	r4, [r0]) // high
	//
	_ASM(LDR 	r5, [r0]) // low
	//
	_ASM(BICS	r4, r5)
	_ASM(LDR 	r5, [r0]) // low
	//
	_ASM(BICS	r4, r5)
	_ASM(NOP)
	_ASM(LDR 	r5, [r0]) // high
	//
	_ASM(LDR 	r6, [r0]) // low
	// 
	_ASM(ANDS	r4, r5)		
	_ASM(LDR 	r5, [r0]) // low
	//
	_ASM(BICS	r4, r6)	
	_ASM(BICS	r4, r5)		
	_ASM(LDR 	r5, [r0]) // high
	//
	_ASM(ANDS	r4, r5)	
	_ASM(TST	r4, r1)
	_ASM(BEQ	start)
	// in-phase begins here

	// generate hsync bit
	_ASM(MOVS	r5, #0x1)
	_ASM(LSLS	r5, #9)

	_ASM(NOP)				 	// 1
	_ASM(NOP)				 	// 1
#ifdef DEBUG_SYNC
	_ASM(MOVS	r4, #0)
	_ASM(STR 	r4, [r2]) // reset bit
	//
#else
	_ASM(NOP)				 	// 1
	_ASM(NOP)				 	// 1
	_ASM(NOP)				 	// 1
#endif

	// wait for hsync to go high
_ASM_LABEL(loop)
	_ASM(LDR 	r4, [r0]) 	// 2
	// 
	_ASM(TST	r4, r5)		// 1
	_ASM(BEQ	loop)		// 3

	_ASM(NOP)				 	// 1

	// phase sample
	_ASM(LDR 	r4, [r0]) // high
	//
	_ASM(TST	r4, r1)
	_ASM(BNE	skip)
	_ASM(NOP)
	_ASM(NOP)
	_ASM(NOP)
	_ASM(NOP)
	_ASM(NOP)
_ASM_LABEL(skip)

#ifdef DEBUG_SYNC
	_ASM(STR 	r3, [r2]) // set bit
	//
#else
	_ASM(NOP)
	_ASM(NOP)
#endif
	_ASM(NOP)
	_ASM(NOP)
	_ASM(NOP)  // stripey
	_ASM(NOP)  // good
//	_ASM(NOP) // stripey
//	_ASM(NOP)  // stripey

	_ASM(POP		{r4-r6})
#ifdef KEIL
	_ASM(BX 	lr)
#endif
	_ASM(NOP)				 	// this is needed for lineM0R2, there seems to be a program memory interleaving issue(?)
	_ASM_END
}

_ASM_FUNC void lineM0R1(uint32_t *gpio, uint8_t *memory, uint32_t xoffset, uint32_t xwidth)
{
	_ASM_START
 	_ASM_IMPORT(callSync)

#ifdef KEIL
	_ASM(PUSH	{r4-r5, lr})
#else
	_ASM(PUSH	{r4-r5})
#endif

	// add width to memory pointer so we can compare
	_ASM(ADDS	r3, r1)

	_ASM(PUSH	{r0-r3}) // save args
	_ASM(BL.W	callSync) // get pixel sync
	_ASM(POP	{r0-r3})	// restore args

	// for pixel timing
	_ASM(NOP)				 

		// skip pixels
_ASM_LABEL(dest220)
	_ASM(SUBS	r2, #0x1)	    // 1
	_ASM(NOP)				 	// 1
	_ASM(NOP)				 	// 1
	_ASM(NOP)				 	// 1
	_ASM(NOP)				 	// 1
	_ASM(NOP)				 	// 1
	_ASM(NOP)				 	// 1
	_ASM(NOP)				 	// 1
	_ASM(NOP)				 	// 1
	_ASM(BGE	dest220)		// 3


	_ASM(LDRB 	r2, [r0]) 	  // 0
	_ASM(STRB 	r2, [r1, #0x00])
	_ASM(NOP)
	_ASM(NOP)
	
	_ASM(LDRB 	r2, [r0]) 	  // 0
	_ASM(STRB 	r2, [r1, #0x01])
	_ASM(NOP)
	_ASM(NOP)

_ASM_LABEL(loop110)
	_ASM(LDRB 	r2, [r0]) 	  // 0
	_ASM(STRB 	r2, [r1, #0x2])

	_ASM(ADDS	r1, #0x03)
	_ASM(NOP)

	_ASM(LDRB 	r2, [r0])	  // 0
	_ASM(STRB 	r2, [r1, #0x0])

	_ASM(CMP		r1, r3)

	_ASM(LDRB 	r2, [r0])	  // -1
	_ASM(STRB 	r2, [r1, #0x1]) 

	_ASM(BLT		loop110)

	// wait for hsync to go low (end of line)
_ASM_LABEL(dest130)
	_ASM(LDR 	r5, [r0]) 	// 2
	_ASM(TST		r5, r4)		// 1
	_ASM(BNE		dest130)		// 3

#ifdef KEIL
	_ASM(POP		{r4-r5, pc})
#else
	_ASM(POP		{r4-r5})
#endif
	_ASM_END
}

_ASM_FUNC void lineM0R2(uint32_t *gpio, uint16_t *memory, uint32_t xoffset, uint32_t xwidth)
{
	_ASM_START
	_ASM_IMPORT(callSync)

#ifdef KEIL
	_ASM(PUSH	{r4-r5, lr})
#else
	_ASM(PUSH	{r4-r5})
#endif

	// add width to memory pointer so we can compare
	_ASM(LSLS	r3, #1)
	_ASM(ADDS	r3, r1)

	_ASM(PUSH	{r0-r3}) // save args
	_ASM(BL.W	callSync) // get pixel sync
	_ASM(POP	{r0-r3})	// restore args

	// skip pixels
_ASM_LABEL(dest8)
	_ASM(SUBS	r2, #0x1)		// 1
	_ASM(NOP)				 	// 1
	_ASM(NOP)				 	// 1
	_ASM(NOP)				 	// 1
	_ASM(NOP)				 	// 1
	_ASM(NOP)				 	// 1
	_ASM(NOP)				 	// 1
	_ASM(NOP)				 	// 1
	_ASM(NOP)				 	// 1
	_ASM(BGE	dest8)			// 3

	_ASM(LDRB 	r2, [r0])
	//
	_ASM(NOP)
	_ASM(NOP) 	  
	_ASM(NOP)
	_ASM(NOP)

	_ASM(LDRB 	r4, [r0])
	// 	  
	_ASM(NOP)
	_ASM(NOP) 	  
	_ASM(NOP)
	_ASM(NOP)


_ASM_LABEL(loop3)
	_ASM(LDRB 	r5, [r0]) 	  
	//
	_ASM(ADDS   r5, r2)
	_ASM(NOP)
	_ASM(STRH 	r5, [r1, #0x00])  // replace with STRH
	//

	_ASM(LDRB 	r5, [r0])
	// 	  
	_ASM(ADDS   r5, r4)
	_ASM(NOP)
	_ASM(STRH 	r5, [r1, #0x02])  // replace with STRH, 0x02
	//

	_ASM(LDRB 	r2, [r0])
	//
	_ASM(ADDS	r1, #0x04) // replace with 0x04
	_ASM(NOP) 	  		 
	_ASM(NOP)
	_ASM(NOP)

	_ASM(LDRB 	r4, [r0])
	// 	  
	_ASM(CMP	r1, r3)
	_ASM(BLT	loop3)

#ifdef KEIL
	_ASM(POP	{r4-r5, pc})
#else
	_ASM(POP	{r4-r5})
#endif
_ASM_END
}

_ASM_FUNC void mergeM0R2(uint16_t *line0, uint16_t *line1, uint8_t *memory, uint32_t xwidth)
{
	_ASM_START

	_ASM(PUSH	{r4-r7})

	_ASM(ADDS	r7, r2, r3)
	_ASM(MOVS	r4, #0x00)

_ASM_LABEL(loop5)	
	_ASM(LDR	r5, [r0, r4])
	//
	_ASM(LDR	r6, [r1, r4])
	//
	_ASM(ADDS	r5, r6)
	_ASM(LSRS	r5, #0x02)
	_ASM(STRB	r5, [r2])
	//
	_ASM(LSRS	r5, #0x10)
	_ASM(STRB	r5, [r2, #0x01])
	//
	_ASM(ADDS	r2, #0x02)
	_ASM(ADDS 	r4, #0x04)
	_ASM(CMP	r2, r7)
	_ASM(BLT 	loop5)

 	_ASM(POP	{r4-r7})
#ifdef KEIL
	_ASM(BX 	lr)
#endif

	_ASM_END
}

_ASM_FUNC void	lineM0R3(uint32_t *gpio, uint16_t width, uint8_t *memy, uint8_t *memc)
{
	_ASM_START
	_ASM_IMPORT(callSync)

#ifdef KEIL
	_ASM(PUSH	{r4-r5, lr})
#else
	_ASM(PUSH	{r4-r5})
#endif
	// r0: gpio
	// r1: width
	// r2: memy
	// r3: memc
	// r4: scratch
	// r5: scratch
	
	_ASM(PUSH	{r0-r3}) // save args
	_ASM(BL.W	callSync) // get pixel sync
	_ASM(POP	{r0-r3})	// restore args

	// variable delay --- get correct phase for sampling
	_ASM(NOP)
	_ASM(NOP)
	_ASM(NOP)
	_ASM(NOP)
	_ASM(NOP)
	_ASM(NOP)
	_ASM(NOP)
	_ASM(NOP)
	_ASM(NOP)
	_ASM(NOP)

	_ASM(LDRB 	r4, [r0]) // blueA
	//
	_ASM(ADDS 	r1, r2) // calculate end of line memory
	_ASM(NOP)
	_ASM(NOP)
	_ASM(NOP)
	
_ASM_LABEL(loop_bg)
	_ASM(LDRB		r5, [r0]) // greenA
	//
	_ASM(STRB		r4, [r3, #0])
	//
	_ASM(STRB		r5, [r2, #0])
	//
	_ASM(LDRB		r4, [r0]) // blueB
	//
	_ASM(STRB		r4, [r3, #2])
	//
	_ASM(CMP 		r2, r1)
	_ASM(BGE		out_bg)
	_ASM(LDRB		r4, [r0]) // greenB
	//
	_ASM(STRB		r4, [r2, #2])
	//
	_ASM(ADDS		r2, #4)
	_ASM(ADDS		r3, #4)
	_ASM(LDRB		r4, [r0]) // blueA
	//
	_ASM(NOP)	
	_ASM(B			loop_bg)

_ASM_LABEL(out_bg)

#ifdef KEIL
	_ASM(POP	{r4-r5, pc})
#else
	_ASM(POP	{r4-r5})
#endif
_ASM_END
}



_ASM_FUNC void vdelay1()
{
	_ASM_START
	_ASM(NOP)
	_ASM(BX 	lr)
	_ASM_END
}
_ASM_FUNC void vdelay2()
{
	_ASM_START
	_ASM(NOP)
	_ASM(NOP)
	_ASM(BX 	lr)
	_ASM_END
}

_ASM_FUNC void vdelay3()
{
	_ASM_START
	_ASM(NOP)
	_ASM(NOP)
	_ASM(NOP)
	_ASM(BX 	lr)
	_ASM_END
}

_ASM_FUNC void vdelay4()
{
	_ASM_START
	_ASM(NOP)
	_ASM(NOP)
	_ASM(NOP)
	_ASM(NOP)
	_ASM(BX 	lr)
	_ASM_END
}

uint16_t g_dist = 4;
uint16_t g_thresh = 20;
uint16_t g_hThresh = 20*3/5;

#define ENQUEUE_START() \
	uint16_t *data = g_equeue->data; \
	uint16_t writeIndex = g_equeue->writeIndex; \
	uint16_t produced = 0 

#define ENQUEUE(val) \
	data[writeIndex++] = val; \
	produced++; \
	if (writeIndex==EQ_MEM_SIZE) \
		writeIndex = 0 

#define ENQUEUE_END() \
	g_equeue->writeIndex = writeIndex; \
	g_equeue->produced += produced 

int hScan(uint8_t *memy, uint8_t *memc)
{
	int16_t i;
	int16_t end, diff;
	ENQUEUE_START();
	
	if (eq_free()<CAM_RES3_WIDTH/2+CAM_RES3_HEIGHT)
		return -1;
	ENQUEUE(EQ_HSCAN_LINE_START);

	i = -1;
	end = CAM_RES3_WIDTH - g_dist;

	// state 0, looking for either edge
loop0:
	i++;
	if (i>=end)
		goto loopex;
	diff = memy[i+g_dist]-memy[i];
	if (-g_thresh>=diff)
		goto edge0;
	if (diff>=g_thresh)
		goto edge1;
	goto loop0;

	// found neg edge
edge0:
	ENQUEUE(i | EQ_NEGATIVE);
	i+=2;

	// state 1, looking for end of edge or pos edge
loop1:
	i++;
	if (i>=end)
		goto loopex;
	diff = memy[i+g_dist]-memy[i];
	if (-g_hThresh<diff)
		goto loop0;
	if (diff>=g_thresh)
		goto edge1;
	goto loop1;

	// found pos edge
edge1:
	ENQUEUE(i);
	i+=2;
	
	// state 2, looking for end of edge or neg edge
loop2:
	i++;
	if (i>=end)
		goto loopex;
	diff = memy[i+g_dist]-memy[i];
	if (diff<g_hThresh)
		goto loop0;
	if (-g_thresh>=diff)	
		goto edge0;
	goto loop2;

loopex:
	ENQUEUE_END();
	return 0;
	
}


int vScan(uint8_t *memy, uint8_t *memc)
{
	int16_t i;
	int16_t diff;
	uint8_t *line0;
	ENQUEUE_START();
	
	if (eq_free()<CAM_RES3_WIDTH/2+CAM_RES3_HEIGHT)
		return -1;
	ENQUEUE(EQ_VSCAN_LINE_START);

	i = -4;
	line0 = memy - ((g_dist+5)>>2)*CAM_RES3_WIDTH;

loop:
	i+=4;
	if (i>=CAM_RES3_WIDTH)
		goto loopex;
	diff = memy[i]-line0[i];
	if (-g_thresh>=diff)
		goto edge0;
	if (diff>=g_thresh)
		goto edge1;

	goto loop;

edge0:
	ENQUEUE(i | EQ_NEGATIVE);
	goto loop;

edge1:
  ENQUEUE(i);
	goto loop;

loopex:
	ENQUEUE_END();
	return 0;
}


void skipLine()
{
	while(!CAM_HSYNC());
	while(CAM_HSYNC());
}


void skipLines(uint32_t lines)
{
	uint32_t line;
	uint32_t timer = 0;
	
	// wait for remainder of frame to pass
	while(CAM_VSYNC())
		setTimer(&timer); 
	// vsync asserted
	while(!CAM_VSYNC());
	// write blanking time if we witnessed the whole thing
	if (timer)
		SM_OBJECT->blankTime = getTimer(timer);

	// write frame period
	timer = getTimer(g_timer);
	timer >>= 4; // shift by 4 to get larger periods in there.  So the period is in 16usec units.
	if (timer>=0xffff)
		timer = 0xffff;
	SM_OBJECT->frameTime = timer; 
	setTimer(&g_timer);
	
	// skip lines
	for (line=0; line<lines; line++)
		skipLine();
	
	// reset vsync so we don't accidentally set timer early
	g_vprev = 1;
}

// This function finds 'missed" vsync transitions so we can get a good
// value for the frame period. 
void trackVsync()
{
	uint32_t v;
	
	v = CAM_VSYNC();
	if (v && !g_vprev)
		setTimer(&g_timer);
	g_vprev = v;
}		

	
void callSync(void)
{
	sync((uint32_t *)&CAM_PORT, CAM_PCLK_MASK, (uint32_t *)&LPC_GPIO_PORT->PIN[5], 0x04);
}

void (*vda[])(void) = {vdelay1, vdelay2, vdelay3, vdelay4};
 
void grabM0R1(uint32_t xoffset, uint32_t yoffset, uint32_t xwidth, uint32_t ywidth, uint8_t *memory)
{
	uint32_t line;
	static int vindex = 0;
	vindex++;

	(*vda[vindex&0x03])();

	xoffset >>= 1;
	yoffset |= 1;

	skipLines(yoffset);
	for (line=0; line<ywidth; line++, memory+=xwidth)
		lineM0R1((uint32_t *)&CAM_PORT, memory, xoffset, xwidth); // wait, grab, wait
}

void grabM0R2(uint32_t xoffset, uint32_t yoffset, uint32_t xwidth, uint32_t ywidth, uint8_t *memory)
{
	uint32_t line;
	uint16_t *lineStore;
	lineStore = (uint16_t *)ALIGN(g_lineStore, 2);

	skipLines(yoffset*2+1);
	for (line=0; line<ywidth; line+=2, memory+=xwidth*2)
	{
		// update line count
		SM_OBJECT->currentLine = line;

		// CAM_HSYNC is negated here
		lineM0R2((uint32_t *)&CAM_PORT, lineStore, xoffset, xwidth); 
		while(CAM_HSYNC());
		lineM0R2((uint32_t *)&CAM_PORT, lineStore+xwidth, xoffset, xwidth); 
		while(CAM_HSYNC());
		lineM0R2((uint32_t *)&CAM_PORT, lineStore+2*xwidth, xoffset, xwidth); 
	   	mergeM0R2(lineStore, lineStore+2*xwidth, memory, xwidth);
		lineM0R2((uint32_t *)&CAM_PORT, lineStore, xoffset, xwidth); 
	   	mergeM0R2(lineStore+xwidth, lineStore, memory+xwidth, xwidth);
	}
}

// hblank   16
// hactive  19
// hblank   16
// hactive  19
// hblank   16
#define MAX_SYNC_TIME  3 // useconds	

int32_t grabM0R3(uint8_t *memy)
{
	int32_t minTime;	
	uint32_t line;
	uint32_t timer;
	int32_t time;
	uint8_t *memc = memy+CAM_RES3_WIDTH*CAM_RES3_HEIGHT+16;
	uint32_t maxScanTime = 3*g_hblank+2*g_hactive-MAX_SYNC_TIME;
	uint32_t fourLineTime = 4*g_hblank+4*g_hactive-MAX_SYNC_TIME;
	uint8_t scan;
	
	minTime = 100;
		
	skipLines(1);
	for (line=0, scan=1; line<CAM_RES3_HEIGHT; line++, memy+=CAM_RES3_WIDTH)
	{
		// CAM_HSYNC is negated here
		lineM0R3((uint32_t *)&CAM_PORT, CAM_RES3_WIDTH, memy+1, memc); 
		while(CAM_HSYNC());
		lineM0R3((uint32_t *)&CAM_PORT, CAM_RES3_WIDTH, memc+1, memy); 
		while(CAM_HSYNC());
		
		if (scan)
		{
			setTimer(&timer);
			if (hScan(memy, memc)<0)
			{
				eq_enqueue(EQ_HSCAN_LINE_START);
				scan = 0;
			}
			if (line>=(g_dist+5)>>2 && vScan(memy, memc)<0)
			{
				eq_enqueue(EQ_VSCAN_LINE_START);
				scan = 0;
			}
			time = maxScanTime - getTimer(timer);
			
			if (time<0)
			{
				time += fourLineTime;
				setTimer(&timer);
				line++, memy+=CAM_RES3_WIDTH;
				eq_enqueue(EQ_HSCAN_LINE_START);
				if (line>=(g_dist+5)>>2)
					eq_enqueue(EQ_VSCAN_LINE_START);
				time -= getTimer(timer);
			}
			if (minTime>time)
				minTime = time;
			if (time>0)
				delayus(time);
		}	
		else
		{
			eq_enqueue(EQ_HSCAN_LINE_START);
			if (line>=(g_dist+5)>>2)
				eq_enqueue(EQ_VSCAN_LINE_START);
			SM_OBJECT->currentLine = line;
			
			while(!CAM_HSYNC());
			while(CAM_HSYNC());
			while(!CAM_HSYNC());
			while(CAM_HSYNC());
		}
		// update line count
		SM_OBJECT->currentLine = line;	
	}
	eq_enqueue(EQ_FRAME_END);

	return 0;
}


int32_t getFrame(uint8_t *type, uint32_t *memory, uint16_t *xoffset, uint16_t *yoffset, uint16_t *xwidth, uint16_t *ywidth)
{
	//printf("M0: grab %d %d %d %d %d\n", *type, *xoffset, *yoffset, *xwidth, *ywidth);
	uint8_t type2 = *type>>4;
	
	if (type2==2)
		grabM0R2(*xoffset, *yoffset, *xwidth, *ywidth, (uint8_t *)*memory);
	else if (type2==1)
		grabM0R1(*xoffset, *yoffset, *xwidth, *ywidth, (uint8_t *)*memory);
	else 
		return -1;
	return 0;
}

int32_t getEdges(uint32_t *memory)
{
	return grabM0R3((uint8_t *)*memory);
}
	
int32_t getTiming()
{
	uint32_t timer0, timer1;
	
	// blanking
	while(!CAM_VSYNC());
	// active
	while(CAM_VSYNC());
	setTimer(&timer0);
	while(!CAM_VSYNC());
	g_vblank = getTimer(timer0);
	setTimer(&timer0);
	// active

	while(!CAM_HSYNC());
	while(CAM_HSYNC());
	setTimer(&timer1);
	while(!CAM_HSYNC());
	g_hblank = getTimer(timer1);
	setTimer(&timer1);
	while(CAM_HSYNC());
	g_hactive = getTimer(timer1);

	while(CAM_VSYNC());
	g_vactive = getTimer(timer0);
	
	CRP_RETURN(UINT16(g_hblank), UINT16(g_hactive), UINT16(g_vblank), UINT16(g_vactive)); 
	return 0;
}

int32_t setEdgeParams(uint16_t *dist, uint16_t *thresh, uint16_t *hThresh)
{
	g_dist = *dist;
	g_thresh = *thresh;
	g_hThresh = *hThresh;
	
	return 0;
}

int frame_init(void)
{
	uint32_t line, frame=0;

	chirpSetProc("getFrame", (ProcPtr)getFrame);
	chirpSetProc("getEdges", (ProcPtr)getEdges);
	chirpSetProc("getTiming", (ProcPtr)getTiming);
	chirpSetProc("setEdgeParams", (ProcPtr)setEdgeParams);
	
#ifdef DEBUG_SYNC
	LPC_GPIO_PORT->DIR[5] |= 0x0004;
#endif

#if 0
	while(1)
	{
		if (frame%100==0)
		{
			_DBD32(frame); 
			_DBG(" ");
			_DBG("\n");
		}		
		grabM0R3((uint8_t *)SRAM1_LOC, NULL, NULL, NULL, 0);
		frame++;
	}
#endif
#if 0	
	while(1)
	{
		line=0;
		while(CAM_VSYNC())
		{
			while(!CAM_HSYNC()&&CAM_VSYNC());
			while(CAM_HSYNC()&&CAM_VSYNC());
			line++;
		} 
		while(!CAM_VSYNC());
		frame++;
		if (frame%50==0)
		{
			_DBD32(frame); 
			_DBG(" ");
			_DBD32(line);
			_DBG("\n");
		}
	}
#endif
#if 0
	{
		static int vindex = 0;
		LPC_GPIO_PORT->DIR[5] |= 0x0004;
		while(1)
		{
			while(CAM_VSYNC())
			{
				//LPC_GPIO_PORT->PIN[5] &= ~0x0004;
				while(CAM_HSYNC()&&CAM_VSYNC());
				(*vda[vindex&0x03])();
				//callSync2();
				callSync3();
				//LPC_GPIO_PORT->PIN[5] |= 0x0004;
				vindex++;
				while(!CAM_HSYNC()&&CAM_VSYNC());
			}
			while(!CAM_VSYNC());
		}
	}
#endif		
	return 0;	
}
