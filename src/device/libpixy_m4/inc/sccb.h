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

#ifndef _SCCB_H
#define _SCCB_H

#include "lpc43xx.h"

#define SCCB_DELAY		  100

#define CLK_MASK          (1<<1)
#define DATA_MASK         (1<<0)
#define DIR_REG 		  LPC_GPIO_PORT->DIR[0]
#define DATA_REG  		  LPC_GPIO_PORT->PIN[0]

class CSccb
	{
public:
	CSccb(uint8_t dev);
	void Write8(uint16_t addr, uint8_t val);
	void Write16(uint16_t addr, uint16_t val);
	void Write32(uint16_t addr, uint32_t val);
	uint8_t Read8(uint16_t addr);
	uint16_t Read16(uint16_t addr);
	uint32_t Read32(uint16_t addr);
	void Reset();

private:
	void WriteReg(uint32_t val, uint8_t n);
	void WriteReg(uint16_t addr, uint32_t val, uint8_t n);
	uint32_t ReadReg(uint16_t addr, uint8_t n);
	inline void PortWrite(uint8_t clk, uint8_t data)
		{
		volatile uint32_t d;
		uint32_t bits = DATA_REG;
		if (clk)
			bits |= CLK_MASK;
		else
			bits &= ~CLK_MASK;
		if(data)
			bits |= DATA_MASK;
		else		 
			bits &= ~DATA_MASK;
		
		DATA_REG = bits; 	
		for (d=0; d<SCCB_DELAY; d++);
		}

	inline void Drive()
		{
		volatile uint32_t d;
		DIR_REG |= DATA_MASK;
		for (d=0; d<SCCB_DELAY; d++);
		}

	inline void TriState()
		{
		volatile uint32_t d;
		DIR_REG &= ~DATA_MASK;
		for (d=0; d<SCCB_DELAY; d++);
		}

	inline void Start()
		{
		PortWrite(1, 1);	// data, clk high
		Drive();	// take data out of tristate
		PortWrite(1, 0);	// data low
		PortWrite(0, 0);	// data low
		}

	inline void Stop()
		{
		PortWrite(1, 0);	// clk high, data low
		PortWrite(1, 1);	// clk, data high
		TriState();	// put data into tristate
		}

	uint8_t m_dev;
	uint16_t m_drive;
	};

#endif
