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
// Arduino SPI with slave select link class

#ifndef _PIXY2ESP32_H
#define _PIXY2ESP32_SS_H

#include "TPixy2.h"
#include "SPI.h"

#define PIXY_SPI_CLOCKRATE       8000000

class Link2SPI_SS
{
public:
  int8_t open(uint32_t arg)
  {
    if (arg==PIXY_DEFAULT_ARGVAL)
      ssPin = SS; // default slave select pin
	else
	  ssPin = arg;
    pinMode(ssPin, OUTPUT);
    SPI.begin();
    SPI.beginTransaction(SPISettings(PIXY_SPI_CLOCKRATE, MSBFIRST, SPI_MODE3));
	return 0;
  }
	
  void close()
  {
    SPI.endTransaction();
  }
    
  int16_t recv(uint8_t *buf, uint8_t len, uint16_t *cs=NULL)
  {
    uint8_t i;
    if (cs)
      *cs = 0;
    digitalWrite(ssPin, LOW);
    for (i=0; i<len; i++)
    {
      buf[i] = SPI.transfer(0x00);
      if (cs)
        *cs += buf[i];
    }
    digitalWrite(ssPin, HIGH);
    return len;
  }
    
  int16_t send(uint8_t *buf, uint8_t len)
  {
    uint8_t i;
    digitalWrite(ssPin, LOW);
    for (i=0; i<len; i++)
      SPI.transfer(buf[i]);
    digitalWrite(ssPin, HIGH);
    return len;
  }

  void setArg(uint16_t arg)
  {
  }

private:
  uint16_t ssPin;
};


typedef TPixy2<Link2SPI_SS> Pixy2SPI_SS;

#endif
