#include "libpixyusb2.h"

Link2USB::Link2USB()
{
  m_link = NULL;
  m_chirp = NULL;
}

Link2USB::~Link2USB()
{
  close();
}
  
int8_t Link2USB::open(uint32_t arg)
{
  int8_t res;

  if (m_link!=NULL)
    return -1;

  m_link = new USBLink();
  res = m_link->open();
  if (res<0)
    return res;
  m_chirp = new Chirp(false, true);
  res = m_chirp->setLink(m_link);
  if (res<0)
    return res;
  m_packet = m_chirp->getProc("ser_packet");
  if (m_packet<0)
    return -1;
  return 0;
}
	
void Link2USB::close()
{
  if (m_chirp)
  {
    delete m_chirp;
    m_chirp = NULL;
  }
  if (m_link)
  {
    m_link->close();
    delete m_link;
    m_link = NULL;
  }
}
    
int16_t Link2USB::recv(uint8_t *buf, uint8_t len, uint16_t *cs)
{
  int i;
  if (m_rbufLen-m_rbufIndex<len)
    return 0;

  for (i=0; i<len; i++, m_rbufIndex++)
    buf[i] = m_rbuf[m_rbufIndex];

  return 0;
}
    
int16_t Link2USB::send(uint8_t *buf, uint8_t len)
{
  int32_t response;
  uint8_t type;
  uint32_t length;
  uint8_t *data;
  int i, res;
    
  res = m_chirp->callSync(m_packet, UINT8(buf[2]), UINTS8(buf[3], buf+4), END_OUT_ARGS,
     &response, &type, &length, &data, END_IN_ARGS);
  if (res<0)
    return res;
  if (response<0)
    return response;

  m_rbufIndex = 0;
  m_rbufLen = length+4;
  *(uint16_t *)m_rbuf = PIXY_NO_CHECKSUM_SYNC;
  m_rbuf[2] = type;
  m_rbuf[3] = length;
  for (i=0; i<length && i<RBUF_LEN-4; i++)
    m_rbuf[i+4] = data[i];
    
  return 0;
}
  
int Link2USB::callChirp(const char *func, ...)
{
  return 0;
}
