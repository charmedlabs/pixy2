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

#include <pixy_init.h>
#include "sccb.h"

//CSccb g_sccb(0, 0);

CSccb::CSccb(uint8_t dev)
{
	m_dev = dev;
	DIR_REG |= CLK_MASK;

	TriState();	// set clock as output, data tristate
	PortWrite(1, 0);	// set clock high (idle)	 
}


void CSccb::Reset()
{
}


void CSccb::WriteReg(uint32_t val, uint8_t n)
{
	uint8_t i, j, tVal;
	uint8_t *pVal = (uint8_t *)&val;
	 
	for (i=0; i<n; i++)
	{
		tVal = pVal[n-1-i]; // little endian
		for (j=0; j<8; j++, tVal<<=1)
		{
			if (tVal&0x80) // send msb first
			{
				PortWrite(0, 1);
				PortWrite(1, 1);
				PortWrite(0, 1);
			}
			else
			{
				PortWrite(0, 0);
				PortWrite(1, 0);
				PortWrite(0, 0);
			}
		}
		// send "don't care" bit
		TriState();
		PortWrite(0, 0);
		PortWrite(1, 0);
		PortWrite(0, 0);
		Drive();
	}
}


void CSccb::WriteReg(uint16_t addr, uint32_t val, uint8_t n)
{
	//printf("%x %x %d\n", addr, val, n);
	Start();
	WriteReg(m_dev, 1);
	WriteReg(addr, 2);
	WriteReg(val, n);
	Stop();
}


void CSccb::Write8(uint16_t addr, uint8_t val)
{
	WriteReg(addr, val, 1);
}

void CSccb::Write16(uint16_t addr, uint16_t val)
{
	WriteReg(addr, val, 2);
}

void CSccb::Write32(uint16_t addr, uint32_t val)
{
	WriteReg(addr, val, 4);
}


uint32_t CSccb::ReadReg(uint16_t addr, uint8_t n)
{
	uint8_t i, j, a;
	uint32_t val, tVal;
	uint8_t *pVal = (uint8_t *)&val;

	// 2 phase
	Start();
	WriteReg(m_dev, 1);
	WriteReg(addr, 2);
	Stop();

	Start();
	WriteReg(m_dev+1, 1);

	for (i=0, val=0; i<n; i++)
	{
		TriState();
		for (j=0, tVal=0; j<8; j++)
		{
			tVal <<= 1;
			PortWrite(1, 0);
			if (DATA_REG&DATA_MASK)
				tVal |= 1;
			PortWrite(0, 0);
		}
		pVal[n-1-i] = tVal; // little endian

		// send A or NA bit
		if (i==n-1)
			a = 1; // NA
		else 
			a = 0; // A

		PortWrite(0, a);
		Drive();
		PortWrite(1, a);
		PortWrite(0, a);
	}
	Stop();

	return val;
}

uint8_t CSccb::Read8(uint16_t addr)
{
	return ReadReg(addr, 1); 
}

uint16_t CSccb::Read16(uint16_t addr)
{
	return ReadReg(addr, 2); 
}

uint32_t CSccb::Read32(uint16_t addr)
{
	return ReadReg(addr, 4); 
}


