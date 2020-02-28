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

#ifndef _LIBPIXYUSB2_H
#define _LIBPIXYUSB2_H

#include <stdio.h>
#include "../../../common/inc/chirp.hpp"

#define RBUF_LEN      0x200

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "usblink.h"
#include "util.h"
#include "TPixy2.h"

#define PIXY2_RAW_FRAME_WIDTH   316
#define PIXY2_RAW_FRAME_HEIGHT  208

class Link2USB
{
public:
  Link2USB ();
  ~Link2USB ();
  
  int8_t open (uint32_t arg);
  void close ();
    
  int16_t recv (uint8_t *buf, uint8_t len, uint16_t *cs=NULL);
  int16_t send (uint8_t *buf, uint8_t len);
  
  int callChirp (const char *func, ...);
  int callChirp (const char *func, va_list  args);
  int stop();
  int resume();
  int getRawFrame(uint8_t **bayerFrame);
  
private:
  Chirp *m_chirp;
  USBLink *m_link;
  ChirpProc m_packet;
  uint8_t m_rbuf[RBUF_LEN];
  uint16_t m_rbufIndex;
  uint16_t m_rbufLen;
  bool m_stopped;
};

typedef TPixy2<Link2USB> Pixy2;

#endif
