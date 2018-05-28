#ifndef _LIBPIXYUSB2_H
#define _LIBPIXYUSB2_H

#include <stdio.h>
#include "../../../common/inc/chirp.hpp"

#define RBUF_LEN      0x200

#include <string.h>
#include <malloc.h>
#include <stdint.h>
#include "usblink.h"
#include "util.h"
#include "TPixy2.h"

class Link2USB
{
public:
  Link2USB();
  ~Link2USB();
  
  int8_t open(uint32_t arg);
  void close();
    
  int16_t recv(uint8_t *buf, uint8_t len, uint16_t *cs=NULL);
  int16_t send(uint8_t *buf, uint8_t len);
  
  int callChirp(const char *func, ...);
  
private:
  Chirp *m_chirp;
  USBLink *m_link;
  ChirpProc m_packet;
  uint8_t m_rbuf[RBUF_LEN];
  uint16_t m_rbufIndex;
  uint16_t m_rbufLen;
};

typedef TPixy2<Link2USB> Pixy2;

#endif
