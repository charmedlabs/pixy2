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

#include "lpc43xx.h"
#include "platform_config.h"

#include "lpc43xx_scu.h"
#include "lpc43xx_cgu.h"
#include "lpc43xx_timer.h"
#include "lpc_types.h"

/**********************************************************************
 ** Function prototypes
 **********************************************************************/
void vIOInit(void);
void clockInit(void);
void fpuInit(void);



/* this function initializes the platform with system level settings */
void platformInit(void) {

	SystemInit();	
	
	/* checks for presence of an FPU unit */
	fpuInit();

	clockInit();
	vIOInit();

	#if (USE_EXT_STATIC_MEM == YES) || (USE_EXT_DYNAMIC_MEM == YES)
	 
	EMC_Init();
	
	#endif

    #if (USE_EXT_FLASH == YES)
	
	// relocate vector table to internal ram
	// updates also VTOR
	relocIrqTable(); 
	
	#endif
}

/*----------------------------------------------------------------------------
  Initialize board specific IO
 *----------------------------------------------------------------------------*/
void vIOInit(void)
{	
	// disable clocks to peripherals we don't use	
	LPC_CCU1->CLK_M4_RITIMER_CFG = 0;	
	LPC_CCU1->CLK_APB3_I2C1_CFG = 0;
	LPC_CCU1->CLK_APB3_ADC1_CFG = 0;
	LPC_CCU1->CLK_APB3_CAN0_CFG = 0;
	LPC_CCU1->CLK_APB1_MOTOCONPWM_CFG = 0;
	LPC_CCU1->CLK_APB1_I2S_CFG = 0;
	LPC_CCU1->CLK_APB1_CAN1_CFG = 0;
	LPC_CCU1->CLK_M4_LCD_CFG = 0;
	LPC_CCU1->CLK_M4_ETHERNET_CFG = 0;
	LPC_CCU1->CLK_M4_EMC_CFG = 0;
	LPC_CCU1->CLK_M4_SDIO_CFG = 0;
	LPC_CCU1->CLK_M4_USB1_CFG = 0;
	LPC_CCU1->CLK_M4_EMCDIV_CFG = 0;

	LPC_CCU1->CLK_M4_USART2_CFG = 0;

	LPC_CCU1->CLK_M4_SSP0_CFG = 0;
	LPC_CCU1->CLK_M4_QEI_CFG = 0;

	LPC_CCU2->CLK_APB0_SSP0_CFG = 0;
	LPC_CCU2->CLK_APB2_USART2_CFG = 0;
	LPC_CCU2->CLK_SDIO_CFG = 0;

	scu_pinmux(0x0, 0,  (MD_PUP | MD_EZI | MD_ZI | MD_EHS), FUNC0); 	     // gpio0[0] siod
	scu_pinmux(0x0, 1,  (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC0); 	     // gpio0[1] sioc
	scu_pinmux(0x1, 15, (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC0); 	     // gpio0[2] PWDN
	scu_pinmux(0x1, 16, (MD_PUP | MD_EZI | MD_ZI | MD_EHS), FUNC0); 	     // gpio0[3] RSTB
	
	scu_pinmux(0x1, 7,  (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC0);          // gpio1[0] Y0 
	scu_pinmux(0x1, 8,  (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC0);          // gpio1[1] Y1
	scu_pinmux(0x1, 9,  (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC0);          // gpio1[2] Y2
	scu_pinmux(0x1, 10, (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC0);          // gpio1[3] Y3
	scu_pinmux(0x1, 11, (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC0); 	     // gpio1[4] Y4
	scu_pinmux(0x1, 12, (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC0); 	     // gpio1[5] Y5
	scu_pinmux(0x1, 13, (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC0); 	     // gpio1[6] Y6
	scu_pinmux(0x1, 14, (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC0); 	     // gpio1[7] Y7
	scu_pinmux(0x1, 6,  (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC0);          // gpio1[9] HSYNC 
	scu_pinmux(0x2, 12, (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC0); 	     // gpio1[12] VSYNC
	scu_pinmux(0x2, 13, (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC0); 	     // gpio1[13] PCLK

	scu_pinmux(0x2, 11, (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC1); 	     // CTOUT_5 RCS0
	scu_pinmux(0x6, 5, (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC0); 	         // GPIO3[4] 

	scu_pinmux(0x1, 1, (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC1); 	         // CTOUT_7 RED
	scu_pinmux(0x1, 2, (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC1); 	         // CTOUT_6 GREEN
	scu_pinmux(0x2, 9, (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC0); 	     	//  gpio1[10] CTOUT_3 WHITE

	scu_pinmux(0x1, 5,  (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC1); 	       // RCS1=FUNC1=CTOUT_10 
	scu_pinmux(0x2, 7, (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC1); 	         // BLUE  FUNC1=CTOUT_1 
	scu_pinmux(0x2, 10, (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC0); 	       // GPIO0[4], PWM1=FUNC1-CTOUT_2

	scu_pinmux(0x2, 3, (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC2); 	         // UART3 TX for debug console 
	scu_pinmux(0x2, 4, (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC4); 	         // GPIO

	scu_pinmux(0x3, 1, (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC4); 	         // gpio5[8] VBUS_EN

	//scu_pinmux(0x5, 6, (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC4); 	         // U1_TXD (output)
	//scu_pinmux(0x5, 7, (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC4); 	         // U1_RXD (input)
	scu_pinmux(0x2, 0, (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC4); 	         // U0_TXD 
	scu_pinmux(0x2, 1, (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC4); 	         // U0_RXD

	scu_pinmux(0x1, 3, (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC5); 	         // SSP1_MISO 
	scu_pinmux(0x1, 4, (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC5); 	         // SSP1_MOSI 
	
	scu_pinmux(0x1, 19, (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC1); 	         // SSP1_SCK 
	scu_pinmux(0x1, 20, (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC1); 	         // SSP1_SSEL 
	scu_pinmux(0x2, 2, (MD_PDN | MD_EZI | MD_ZI | MD_EHS), FUNC4); 	         // gpio5[2] 
	scu_pinmux(0x2, 5, (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC0); // FUNC4 	         // gpio5[5] rev 1.1 SS control
	scu_pinmux(0x3, 2, (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC4); 	         // gpio5[9]
	
	scu_pinmux(0x2, 8, (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC0); 	         // SGPIO15 
	scu_pinmux(0x4, 1, (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC0); 	         // gpio2[1] rev 1.1 

	// 
	scu_pinmux(0x3, 3, (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC3); 	         
	scu_pinmux(0x3, 4, (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC3); 	         
	scu_pinmux(0x3, 5, (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC3); 	         
	scu_pinmux(0x3, 6, (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC3); 	         
	scu_pinmux(0x3, 7, (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC3); 	          
	scu_pinmux(0x3, 8, (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC3); 	          

	//scu_pinmux(0x7, 4, (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC0); 	     // gpio4[9] 
	//scu_pinmux(0x7, 5, (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC0); 	     // gpio4[9] 
	//scu_pinmux(0x7, 6, (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC0); 	     // gpio4[9] 
	//scu_pinmux(0x7, 7, (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC0); 	     // gpio4[9] 

	LPC_SCU_CLK(0) = 1 + (MD_PLN | MD_EZI | MD_ZI | MD_EHS); /*  EXTBUS_CLK0  IDIVB input */
}
	
/*----------------------------------------------------------------------------
  Initialize clocks
 *----------------------------------------------------------------------------*/


#define __CRYSTAL        (12000000UL)    /* Crystal Oscillator frequency */

static void delayus(uint32_t us)
{
	volatile uint32_t i, j;	
	
	for (i=0; i<us; i++)
		for (j=0; j<38; j++);
}

void TIMER1_IRQHandler(void)
{
	TIM_ClearIntPending(LPC_TIMER1, TIM_MR0_INT);
}

void clockInit(void)
{
 	TIM_TIMERCFG_Type TIM_ConfigStruct;
	TIM_MATCHCFG_Type TIM_MatchConfigStruct;

	__disable_irq();
 	/* Set the XTAL oscillator frequency to 12MHz*/
	CGU_SetXTALOSC(__CRYSTAL);
	CGU_EnableEntity(CGU_CLKSRC_XTAL_OSC, ENABLE);
	CGU_EntityConnect(CGU_CLKSRC_XTAL_OSC, CGU_BASE_M3);
	
	/* Set PL160M 12*1 = 12 MHz */
	CGU_EntityConnect(CGU_CLKSRC_XTAL_OSC, CGU_CLKSRC_PLL1);
//	CGU_EntityConnect(CGU_CLKSRC_IRC, CGU_CLKSRC_PLL1);
	CGU_SetPLL1(1);
	CGU_EnableEntity(CGU_CLKSRC_PLL1, ENABLE);

	// setup CLKOUT
	CGU_EntityConnect(CGU_CLKSRC_PLL1, CGU_CLKSRC_IDIVB);
	CGU_EnableEntity(CGU_CLKSRC_IDIVB, ENABLE);
	CGU_SetDIV(CGU_CLKSRC_IDIVB, 12);  // 12 -> 6 pclks per cpu clk, 10 -> 5 pclks
	// set input for CLKOUT to IDIVB
	LPC_CGU->BASE_OUT_CLK &= ~0x0f000000;
	LPC_CGU->BASE_OUT_CLK |= 0x0d000000;

	/* Run SPIFI from PL160M, /2 */
	CGU_EntityConnect(CGU_CLKSRC_PLL1, CGU_CLKSRC_IDIVA);
	CGU_EnableEntity(CGU_CLKSRC_IDIVA, ENABLE);
	CGU_SetDIV(CGU_CLKSRC_IDIVA, 2);
	CGU_EntityConnect(CGU_CLKSRC_IDIVA, CGU_BASE_SPIFI);
	CGU_UpdateClock();

	LPC_CCU1->CLK_M4_EMCDIV_CFG |=    (1<<0) |  (1<<5);		// Turn on clock / 2
	LPC_CREG->CREG6 |= (1<<16);	// EMC divided by 2
    LPC_CCU1->CLK_M4_EMC_CFG |= (1<<0);		// Turn on clock

	CGU_SetPLL1(9);
	/* Run base M3 clock from PL160M, no division */
	CGU_EntityConnect(CGU_CLKSRC_PLL1, CGU_BASE_M3);

	delayus(10000);

	//__enable_irq();
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
	TIM_MatchConfigStruct.MatchValue   = 500000;

	// To start timer 
	TIM_ConfigMatch(LPC_TIMER1, &TIM_MatchConfigStruct);
	TIM_Init(LPC_TIMER1, TIM_TIMER_MODE,&TIM_ConfigStruct);
	TIM_Cmd(LPC_TIMER1, ENABLE);
	
	/* Enable interrupt for timer 0 */
	NVIC_EnableIRQ(TIMER1_IRQn);
	__enable_irq();

	CGU_SetPLL1(17);
	__WFI();

	NVIC_DisableIRQ(TIMER1_IRQn);
	TIM_Cmd(LPC_TIMER1, DISABLE);
	LPC_TIMER1->MCR = 0;

	CGU_UpdateClock();

	LPC_SCU->SFSP3_3 = 0xF3; /* high drive for SCLK */
	/* IO pins */
	LPC_SCU->SFSP3_4=LPC_SCU->SFSP3_5=LPC_SCU->SFSP3_6=LPC_SCU->SFSP3_7 = 0xD3;
	LPC_SCU->SFSP3_8 = 0x13; /* CS doesn't need feedback */
}
