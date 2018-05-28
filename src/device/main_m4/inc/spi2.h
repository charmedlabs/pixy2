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

#ifndef SPI2_H
#define SPI2_H

#include "lpc43xx_ssp.h"
#include "iserial.h"

#define SPI2_RECEIVEBUF_SIZE   	64

#define SS_ASSERT()  			LPC_SGPIO->GPIO_OUTREG = 0;
#define SS_NEGATE() 			LPC_SGPIO->GPIO_OUTREG = 1<<14;


class Spi2 : public Iserial
{
public:
	Spi2();

	// Iserial methods
	virtual int open();
	virtual int close();
	virtual int receive(uint8_t *buf, uint32_t len);
	virtual int receiveLen();

	void slaveHandler();
	void timerHandler();
	void setAutoSlaveSelect(bool ass);

private:
	ReceiveQ<uint8_t> m_rq;

	bool m_autoSlaveSelect;
};

void spi2_init();
void spi2_deinit();

extern Spi2 *g_spi2;

#endif
