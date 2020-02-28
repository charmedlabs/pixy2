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

#ifndef _PIXY2LINE_H
#define _PIXY2LINE_H

#define LINE_REQUEST_GET_FEATURES                0x30
#define LINE_RESPONSE_GET_FEATURES               0x31
#define LINE_REQUEST_SET_MODE                    0x36
#define LINE_REQUEST_SET_VECTOR                  0x38
#define LINE_REQUEST_SET_NEXT_TURN_ANGLE         0x3a
#define LINE_REQUEST_SET_DEFAULT_TURN_ANGLE      0x3c
#define LINE_REQUEST_REVERSE_VECTOR              0x3e

#define LINE_GET_MAIN_FEATURES                   0x00
#define LINE_GET_ALL_FEATURES                    0x01

#define LINE_MODE_TURN_DELAYED                   0x01
#define LINE_MODE_MANUAL_SELECT_VECTOR           0x02
#define LINE_MODE_WHITE_LINE                     0x80

// features
#define LINE_VECTOR                              0x01
#define LINE_INTERSECTION                        0x02
#define LINE_BARCODE                             0x04
#define LINE_ALL_FEATURES                        (LINE_VECTOR | LINE_INTERSECTION | LINE_BARCODE)

#define LINE_FLAG_INVALID                        0x02
#define LINE_FLAG_INTERSECTION_PRESENT           0x04

#define LINE_MAX_INTERSECTION_LINES              6

struct Vector
{
  void print()
  {
    char buf[64];
    sprintf(buf, "vector: (%d %d) (%d %d) index: %d flags %d", m_x0, m_y0, m_x1, m_y1, m_index, m_flags);
	  Serial.println(buf);
  }
  
  uint8_t m_x0;
  uint8_t m_y0;
  uint8_t m_x1;
  uint8_t m_y1;
  uint8_t m_index;
  uint8_t m_flags;
};

struct IntersectionLine
{
  uint8_t m_index;
  uint8_t m_reserved;
  int16_t m_angle;
};

struct Intersection
{
  void print()
  {
    char buf[64];
    uint8_t i;
    sprintf(buf, "intersection: (%d %d)", m_x, m_y);
	  Serial.println(buf);
    for (i=0; i<m_n; i++)
    {
      sprintf(buf, "  %d: index: %d angle: %d", i, m_intLines[i].m_index, m_intLines[i].m_angle);
	    Serial.println(buf);
    }
  }
  
  uint8_t m_x;
  uint8_t m_y;
	
  uint8_t m_n;
  uint8_t m_reserved;
  IntersectionLine m_intLines[LINE_MAX_INTERSECTION_LINES];
};

struct Barcode
{
  void print()
  {
    char buf[64];
    sprintf(buf, "Barcode: (%d %d), val: %d flags: %d", m_x, m_y, m_code, m_flags);
    Serial.println(buf);
  }
  
  uint8_t m_x;
  uint8_t m_y;
  uint8_t m_flags;
  uint8_t m_code;
};

template <class LinkType> class TPixy2;

template <class LinkType> class Pixy2Line
{
public:
  Pixy2Line(TPixy2<LinkType> *pixy)
  {
    m_pixy = pixy;
  }	  
 
  int8_t getMainFeatures(uint8_t features=LINE_ALL_FEATURES, bool wait=true)
  {
    return getFeatures(LINE_GET_MAIN_FEATURES, features, wait); 
  }
  
  int8_t getAllFeatures(uint8_t features=LINE_ALL_FEATURES, bool wait=true)
  {
    return getFeatures(LINE_GET_ALL_FEATURES, features, wait);   
  }
  
  int8_t setMode(uint8_t mode);
  int8_t setNextTurn(int16_t angle);
  int8_t setDefaultTurn(int16_t angle);
  int8_t setVector(uint8_t index);
  int8_t reverseVector();
  
  uint8_t numVectors;
  Vector *vectors;
  
  uint8_t numIntersections;
  Intersection *intersections;

  uint8_t numBarcodes;
  Barcode *barcodes;

private:
  int8_t getFeatures(uint8_t type, uint8_t features, bool wait);
  TPixy2<LinkType> *m_pixy;
  
};


template <class LinkType> int8_t Pixy2Line<LinkType>::getFeatures(uint8_t type,  uint8_t features, bool wait)
{
  int8_t res;
  uint8_t offset, fsize, ftype, *fdata;
  
  vectors = NULL;
  numVectors = 0;
  intersections = NULL;
  numIntersections = 0;
  barcodes = NULL;
  numBarcodes = 0;
  
  while(1)
  {
    // fill in request data
    m_pixy->m_length = 2;
    m_pixy->m_type = LINE_REQUEST_GET_FEATURES;
    m_pixy->m_bufPayload[0] = type;
    m_pixy->m_bufPayload[1] = features;
 
    // send request
    m_pixy->sendPacket();
    if (m_pixy->recvPacket()==0)
    {     
      if (m_pixy->m_type==LINE_RESPONSE_GET_FEATURES)
      {
        // parse line response
		    for (offset=0, res=0; m_pixy->m_length>offset; offset+=fsize+2)
        {
          ftype = m_pixy->m_buf[offset];
          fsize = m_pixy->m_buf[offset+1];
          fdata = &m_pixy->m_buf[offset+2]; 
          if (ftype==LINE_VECTOR)
          {
            vectors = (Vector *)fdata;
            numVectors = fsize/sizeof(Vector);
            res |= LINE_VECTOR;
		      }
		      else if (ftype==LINE_INTERSECTION)
          {
            intersections = (Intersection *)fdata;
            numIntersections = fsize/sizeof(Intersection);
            res |= LINE_INTERSECTION;
          }
 		      else if (ftype==LINE_BARCODE)
          {
            barcodes = (Barcode *)fdata;
            numBarcodes = fsize/sizeof(Barcode);;
            res |= LINE_BARCODE;
          }
          else
            break; // parse error
        }
        return res;
      }
      else if (m_pixy->m_type==PIXY_TYPE_RESPONSE_ERROR)
      {
		    // if it's not a busy response, return the error
        if ((int8_t)m_pixy->m_buf[0]!=PIXY_RESULT_BUSY)
		      return m_pixy->m_buf[0];
	      else if (!wait) // we're busy
          return PIXY_RESULT_BUSY; // new data not available yet
      }
    }
    else
      return PIXY_RESULT_ERROR;  // some kind of bitstream error
  
    // If we're waiting for frame data, don't thrash Pixy with requests.
    // We can give up half a millisecond of latency (worst case)	
    delayMicroseconds(500);
  }
}

template <class LinkType> int8_t Pixy2Line<LinkType>::setMode(uint8_t mode)
{
  uint32_t res;

  *(int8_t *)m_pixy->m_bufPayload = mode;
  m_pixy->m_length = 1;
  m_pixy->m_type = LINE_REQUEST_SET_MODE;
  m_pixy->sendPacket();
  if (m_pixy->recvPacket()==0 && m_pixy->m_type==PIXY_TYPE_RESPONSE_RESULT && m_pixy->m_length==4)
  {
    res = *(uint32_t *)m_pixy->m_buf;
    return (int8_t)res;	
  }
  else
      return PIXY_RESULT_ERROR;  // some kind of bitstream error
}


template <class LinkType> int8_t Pixy2Line<LinkType>::setNextTurn(int16_t angle)
{
  uint32_t res;

  *(int16_t *)m_pixy->m_bufPayload = angle;
  m_pixy->m_length = 2;
  m_pixy->m_type = LINE_REQUEST_SET_NEXT_TURN_ANGLE;
  m_pixy->sendPacket();
  if (m_pixy->recvPacket()==0 && m_pixy->m_type==PIXY_TYPE_RESPONSE_RESULT && m_pixy->m_length==4)
  {
    res = *(uint32_t *)m_pixy->m_buf;
    return (int8_t)res;	
  }
  else
      return PIXY_RESULT_ERROR;  // some kind of bitstream error
}

template <class LinkType>  int8_t Pixy2Line<LinkType>::setDefaultTurn(int16_t angle)
{
  uint32_t res;

  *(int16_t *)m_pixy->m_bufPayload = angle;
  m_pixy->m_length = 2;
  m_pixy->m_type = LINE_REQUEST_SET_DEFAULT_TURN_ANGLE;
  m_pixy->sendPacket();
  if (m_pixy->recvPacket()==0 && m_pixy->m_type==PIXY_TYPE_RESPONSE_RESULT && m_pixy->m_length==4)
  {
    res = *(uint32_t *)m_pixy->m_buf;
    return (int8_t)res;  
  }
  else
      return PIXY_RESULT_ERROR;  // some kind of bitstream error
}

template <class LinkType> int8_t Pixy2Line<LinkType>::setVector(uint8_t index)
{
  uint32_t res;

  *(int8_t *)m_pixy->m_bufPayload = index;
  m_pixy->m_length = 1;
  m_pixy->m_type = LINE_REQUEST_SET_VECTOR;
  m_pixy->sendPacket();
  if (m_pixy->recvPacket()==0 && m_pixy->m_type==PIXY_TYPE_RESPONSE_RESULT && m_pixy->m_length==4)
  {
    res = *(uint32_t *)m_pixy->m_buf;
    return (int8_t)res;  
  }
  else
      return PIXY_RESULT_ERROR;  // some kind of bitstream error
}


template <class LinkType> int8_t Pixy2Line<LinkType>::reverseVector()
{
  uint32_t res;

  m_pixy->m_length = 0;
  m_pixy->m_type = LINE_REQUEST_REVERSE_VECTOR;
  m_pixy->sendPacket();
  if (m_pixy->recvPacket()==0 && m_pixy->m_type==PIXY_TYPE_RESPONSE_RESULT && m_pixy->m_length==4)
  {
    res = *(uint32_t *)m_pixy->m_buf;
    return (int8_t)res;  
  }
  else
      return PIXY_RESULT_ERROR;  // some kind of bitstream error
}

#endif
