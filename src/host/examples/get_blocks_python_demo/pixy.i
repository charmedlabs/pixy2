%module pixy

%include "stdint.i"
%include "carrays.i"

%{
#define SWIG_FILE_WITH_INIT
#include "util.h"
#include "TPixy2.h"
#include "Pixy2Line.h"
#include "Pixy2CCC.h"
#include "libpixyusb2.h"
%}

%array_class(struct Block, BlockArray);
%array_class(struct Vector, VectorArray);
%array_class(struct IntersectionLine, IntesectionLineArray);

%inline %{
extern int init();
extern int ccc_get_blocks (int  max_blocks, BlockArray *  blocks);
%}

struct Block
{
  uint16_t  m_signature;
  uint16_t  m_x;
  uint16_t  m_y;
  uint16_t  m_width;
  uint16_t  m_height;
  int16_t   m_angle;
  uint8_t   m_index;
  uint8_t   m_age;
};

struct Vector
{
  uint8_t  m_x0;
  uint8_t  m_y0;
  uint8_t  m_x1;
  uint8_t  m_y1;
  uint8_t  m_index;
  uint8_t  m_flags;
};

struct IntersectionLine
{
  uint8_t  m_index;
  uint8_t  m_reserved;
  int16_t  m_angle;
};
