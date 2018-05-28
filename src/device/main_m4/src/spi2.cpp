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

#include "pixy_init.h"
#include "misc.h"
#include "spi2.h"
#include "lpc43xx_scu.h"
#include "lpc43xx_timer.h"
#include "serial.h"

Spi2 *g_spi2 = NULL;

void Spi2::slaveHandler()
{
	volatile uint32_t d;
	uint8_t d8; 

	// clear interrupts
	LPC_SSP1->ICR = SSP_INTCFG_RX | SSP_INTCFG_TX | SSP_INTCFG_RT | SSP_INTCFG_ROR;  
	if (m_autoSlaveSelect)
	{
		TIM_Cmd(LPC_TIMER3, ENABLE);
		TIM_ResetCounter(LPC_TIMER3);
	}
	// empty receive fifo
	if (LPC_SSP1->SR&SSP_SR_RNE)
	{
		while(LPC_SSP1->SR&SSP_SR_RNE)
		{
			d = LPC_SSP1->DR; // grab data
			m_rq.write(d);
		}
		ser_rxCallback();
	}			

	// fill transmit fifo
	while(LPC_SSP1->SR&SSP_SR_TNF) 
	{
		if (ser_getByte(&d8))
		{
			LPC_SSP1->DR = (uint32_t)d8;
			// below is an workaround for an LPC4330 bug. If the transmit fifo underrruns and you write  
			// too soon(?) to the DR, there is a chance you might lose the byte. 
			// We can write 2 of the first byte because the first byte is part of the sync code.  If an extra
			// one of these bytes appears in the bitstream, we'll parse it out anyway.  
			if (ser_newPacket()) 
				LPC_SSP1->DR = d8;
		}	
		else 
			LPC_SSP1->DR = 1; // return 0s by default
	}
}

int Spi2::receive(uint8_t *buf, uint32_t len)
{
	uint32_t i;
	uint8_t buf8;
	
	for (i=0; i<len; i++)
	{
		if (m_rq.read(&buf8)==0)
			break;
		buf[i] = buf8;
	}
	return i;
}

int Spi2::receiveLen()
{	
	return m_rq.receiveLen();
}

int Spi2::open()
{
	// configure SGPIO bit so we can toggle slave select (SS)
	LPC_SGPIO->OUT_MUX_CFG14 = 4;
	scu_pinmux(0x1, 3, (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC5); // SSP1_MISO
	scu_pinmux(0x1, 4, (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC5); // SSP1_MOSI 
	scu_pinmux(0x1, 19, (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC1); // SSP1_SCK 

	// set up timer
	TIM_TIMERCFG_Type TIM_ConfigStruct;
	TIM_MATCHCFG_Type TIM_MatchConfigStruct;

	TIM_ConfigStruct.PrescaleOption = TIM_PRESCALE_USVAL;
	TIM_ConfigStruct.PrescaleValue	= 1;

	// use channel 0, MR0
	TIM_MatchConfigStruct.MatchChannel = 0;
	// Enable interrupt when MR0 matches the value in TC register
	TIM_MatchConfigStruct.IntOnMatch   = TRUE;
	//Enable reset on MR0: TIMER will reset if MR0 matches it
	TIM_MatchConfigStruct.ResetOnMatch = TRUE;
	//Stop on MR0 if MR0 matches it
	TIM_MatchConfigStruct.StopOnMatch  = TRUE;
	//Toggle MR0.0 pin if MR0 matches it
	TIM_MatchConfigStruct.ExtMatchOutputType = TIM_EXTMATCH_NOTHING;
	TIM_MatchConfigStruct.MatchValue   = 1000;

	TIM_ConfigMatch(LPC_TIMER3, &TIM_MatchConfigStruct);
	TIM_Init(LPC_TIMER3, TIM_TIMER_MODE,&TIM_ConfigStruct);
	TIM_Cmd(LPC_TIMER3, ENABLE);

	// clear receive queue
	m_rq.clear();

	/* Enable interrupt for timer 3 */
	NVIC_EnableIRQ(TIMER3_IRQn);
	// enable SPI interrupt
	NVIC_EnableIRQ(SSP1_IRQn);

	return 0;
}

int Spi2::close()
{
	// turn off driver for SS
	LPC_SGPIO->GPIO_OENREG = 0;

	// disable SPI interrupt
	NVIC_DisableIRQ(SSP1_IRQn);
	// disable timer 3 interrupt
	NVIC_DisableIRQ(TIMER3_IRQn);
	return 0;
}


void Spi2::setAutoSlaveSelect(bool ass)
{
	m_autoSlaveSelect = ass;
	if (m_autoSlaveSelect)
		LPC_SGPIO->GPIO_OENREG = 1<<14; // use this SGPIO Bit as slave select, so configure as output
	else
		LPC_SGPIO->GPIO_OENREG = 0; // tri-state the SGPIO bit so host can assert slave select
}
	

Spi2::Spi2() : m_rq(SPI2_RECEIVEBUF_SIZE)
{
	uint32_t i;
	volatile uint32_t d;
	SSP_CFG_Type configStruct;

	configStruct.CPHA = SSP_CPHA_SECOND;
	configStruct.CPOL = SSP_CPOL_LO;
	configStruct.ClockRate = 204000000;
	configStruct.Databit = SSP_DATABIT_8;
	configStruct.Mode = SSP_SLAVE_MODE;
	configStruct.FrameFormat = SSP_FRAME_SPI;

	// Initialize SSP peripheral with parameter given in structure above
	SSP_Init(LPC_SSP1, &configStruct);

	// clear receive fifo
	for (i=0; i<8; i++)
		d = LPC_SSP1->DR;

	// Enable SSP peripheral
	SSP_Cmd(LPC_SSP1, ENABLE);
		
	SSP_ClearIntPending(LPC_SSP1, SSP_INTCFG_RX | SSP_INTCFG_TX | SSP_INTCFG_RT | SSP_INTCFG_ROR);
	SSP_IntConfig(LPC_SSP1, SSP_INTCFG_RX | SSP_INTCFG_TX | SSP_INTCFG_RT | SSP_INTCFG_ROR, ENABLE);

	NVIC_SetPriority(SSP1_IRQn, 0);	// high priority interrupt

	setAutoSlaveSelect(false);
}

void Spi2::timerHandler()
{
	SS_NEGATE();
	TIM_ClearIntPending(LPC_TIMER3, TIM_MR0_INT);
	SS_ASSERT();
}

void spi2_init()
{
	g_spi2 = new Spi2();
}

void spi2_deinit()
{
	if (g_spi2)
		delete g_spi2;
	g_spi2 = NULL;
}

extern "C" void TIMER3_IRQHandler(void);


void TIMER3_IRQHandler(void)
{
	g_spi2->timerHandler();	
}


