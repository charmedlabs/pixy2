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
%array_class(struct IntersectionLine, IntersectionLineArray);
%array_class(struct Barcode, BarcodeArray);

%inline %{
extern int init();

/*!
  @brief       Select active running program on Pixy
  @param[in]   program_name  "color_connected_components"  Block detection program
                             "line"                        Line feature detection program
*/
extern int change_prog (const char *  program_name);

/*!
  @brief       Copy 'max_blocks' number of blocks to the address 'blocks'.
  @param[in]   max_blocks  Maximum number of blocks that will be copied.
  @param[out]  blocks      Address to copy the blocks data.
  @return      Number of blocks copied to 'blocks'.
*/
extern int ccc_get_blocks (int  max_blocks, BlockArray *  blocks);

extern void line_get_all_features ();

extern void line_get_main_features ();

/*!
  @brief       Copy 'max_intersections' number of intersections to the address 'intersections'.
  @param[in]   max_intersections  Maximum number of intersection objects that will be copied.
  @param[out]  intersections      Address to copy the intersection data.
  @return      Number of intersections copied to 'intersections'.
*/
extern int line_get_intersections (int  max_intersections, IntersectionLineArray *  intersections);

/*!
  @brief       Copy 'max_vectors' number of vectors to the address 'vectors'.
  @param[in]   max_vectors        Maximum number of vector objects that will be copied.
  @param[out]  vectors            Address to copy the vector data.
  @return      Number of vectors copied to 'vectors'.
*/
extern int line_get_vectors (int max_vectors, VectorArray *  vectors);

/*!
  @brief       Copy 'max_barcode' number of barcodes to the address 'barcodes'.
  @param[in]   max_barcodes  Maximum number of barcode objects that will be copied.
  @param[out]  barcodes      Address to copy the barcode data.
  @return      Number of barcode objects copied to 'barcodes'.
*/
extern int line_get_barcodes (int  max_barcodes, BarcodeArray *  barcodes);
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
