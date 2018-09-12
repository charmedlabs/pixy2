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
// This file is for defining the Block struct and the Pixy template class version 2.
// (TPixy2).  TPixy takes a communication link as a template parameter so that 
// all communication modes (SPI, I2C and UART) can share the same code.  
//

#ifndef _PIXY2VIDEO_H
#define _PIXY2VIDEO_H

#define VIDEO_REQUEST_GET_RGB   0x70

template <class LinkType> class TPixy2;

template <class LinkType> class Pixy2Video
{
public:
  Pixy2Video(TPixy2<LinkType> *pixy)
  {
    m_pixy = pixy;
  }	  
 
  int8_t getRGB(uint16_t x, uint16_t y, uint8_t *r, uint8_t *g, uint8_t *b, bool saturate=true);
  
private:
  TPixy2<LinkType> *m_pixy;
  
};

template <class LinkType> int8_t Pixy2Video<LinkType>::getRGB(uint16_t x, uint16_t y, uint8_t *r, uint8_t *g, uint8_t *b, bool saturate=true)
{
  uint32_t res;
  
  *(int16_t *)(m_pixy->m_bufPayload + 0)= x;
  *(int16_t *)(m_pixy->m_bufPayload + 2)= y;
  *(m_pixy->m_bufPayload + 4)= saturate;
  m_pixy->m_length = 5;
  m_pixy->m_type = VIDEO_REQUEST_GET_RGB;
  m_pixy->sendPacket();
  if (m_pixy->recvPacket()==0 && m_pixy->m_type==PIXY_TYPE_RESPONSE_RESULT && m_pixy->m_length==4)
  {
    res = *(uint32_t *)m_pixy->m_buf;
	*b = res&0xff;
	res >>= 8;
	*g = res&0xff;
	res >>= 8;
	*r = res&0xff;
    return 0;
  }
  else
      return PIXY_RESULT_ERROR;  // some kind of bitstream error
}

#endif
