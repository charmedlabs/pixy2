#include "libpixyusb2.h"

Link2USB::Link2USB()
{
  m_link = NULL;
  m_chirp = NULL;
  m_stopped = false;
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
  for (i=0; i<(int)length && i<RBUF_LEN-4; i++)
    m_rbuf[i+4] = data[i];
    
  return 0;
}
  
int Link2USB::callChirp(const char *func, ...)
{
  va_list  arguments;
  int      return_value;

  va_start (arguments, func);
  return_value = callChirp (func, arguments);
  va_end (arguments);

  return return_value;
}

int Link2USB::callChirp (const char *  func, va_list  args)
{
  ChirpProc  function_id;
  int        return_value;
  va_list    arguments;

  va_copy (arguments, args);

  // Request chirp function id for 'func'. //
  function_id = m_chirp->getProc (func);

  // Was there an error requesting function id? //
  if (function_id < 0) {
    // Request error //
    va_end (arguments);

    return CRP_RES_ERROR_INVALID_COMMAND;
  }

  // Execute chirp synchronous remote function call //
  return_value = m_chirp->call (SYNC, function_id, arguments);
  va_end (arguments);

  return return_value;
}

int Link2USB::stop()
{
  int res, response;
  char *status;
  
  res = callChirp("stop", END_OUT_ARGS, &response, END_IN_ARGS);
  if (res<0)
    return res;
  while(1)
  {
    res = callChirp("running", END_OUT_ARGS, &response, &status, END_IN_ARGS);
    if (res<0)
      return res;
    if (response==0)
    {
      m_stopped = true;
      return 0;
    }
  }
}

int Link2USB::resume()
{
  int res, response;
  
  res = callChirp("run", END_OUT_ARGS, &response, END_IN_ARGS);
  if (res<0)
    return res;

  m_stopped = false;
  return response;
}

int Link2USB::getRawFrame(uint8_t **bayerFrame)
{
  int32_t res, response, fourcc, length;
  uint8_t renderflags;
  uint16_t width, height; 

  if (!m_stopped)
    return -10; // call stop() before getting frame!

  res = callChirp("cam_getFrame", // String id for remote procedure
		  UINT8(0x21), // mode
		  UINT16(0), // xoffset
		  UINT16(0), // yoffset
		  UINT16(PIXY2_RAW_FRAME_WIDTH), // width
		  UINT16(PIXY2_RAW_FRAME_HEIGHT), // height
		  END_OUT_ARGS, // separator
		  &response, // return value
		  &fourcc,
		  &renderflags,
		  &width,
		  &height,
		  &length,
		  bayerFrame, // frame pointer
		  END_IN_ARGS);
  if (res<0)
    return res;
  return response;
}
	     

