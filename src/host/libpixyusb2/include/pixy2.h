//
// PIXY2 C API
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

#ifndef __PIXY_H__
#define __PIXY_H__

#include <stdio.h>
#include "pixydefs.h"

#ifdef __cplusplus
extern "C"
{
#endif

///////////////
/// GLOBALS ///
///////////////

struct Block
{
  void print(char *buf)
  {
    int i, j;
    char sig[6], d;
    bool flag;
    if (type > CCC_MAX_SIGNATURE)
    {
      // Color Code (CC) //

      // convert signature number to an octal string
      for (i=12, j=0, flag=false; i>=0; i-=3)
      {
        d = (signature>>i)&0x07;
        if (d>0 && !flag)
          flag = true;
        if (flag)
          sig[j++] = d + '0';
      }
      sig[j] = '\0';
      sprintf(buf, "CC block! sig: %s (%d decimal) x: %d y: %d width: %d height: %d angle %d", sig, signature, x, y, width, height, angle);
    }
    else // regular block.  Note, angle is always zero, so no need to print
      sprintf(buf, "sig: %d x: %d y: %d width: %d height: %d", signature, x, y, width, height);		
  }

  uint16_t type;
  uint16_t signature;
  uint16_t x;
  uint16_t y;
  uint16_t width;
  uint16_t height;
  int16_t  angle;
};

///////////////////////////
/// FUNCTION PROTOTYPES ///
///////////////////////////

/**
  @brief Creates a connection with Pixy.
  @return  0                         Success
  @return  PIXY_ERROR_USB_IO         USB Error: I/O
  @return  PIXY_ERROR_NOT_FOUND      USB Error: Pixy not found
  @return  PIXY_ERROR_USB_BUSY       USB Error: Busy
  @return  PIXY_ERROR_USB_NO_DEVICE  USB Error: No device
*/
int pixy2_init();

/**
  @brief Terminates connection with Pixy.
*/
void pixy2_close();

/**
  @brief      Copies up to 'max_blocks' number of Blocks to the address pointed
              to by 'blocks'.
  @param[in]  max_blocks  Maximum number of Blocks to copy to the address pointed to
                          by 'blocks'.
  @param[out] blocks      Address of an array in which to copy the blocks to.
                          The array must be large enough to write 'max_blocks' number
                          of Blocks to.
  @return  Non-negative                  Success: Number of blocks copied
  @return  PIXY_ERROR_USB_IO             USB Error: I/O
  @return  PIXY_ERROR_NOT_FOUND          USB Error: Pixy not found
  @return  PIXY_ERROR_USB_BUSY           USB Error: Busy
  @return  PIXY_ERROR_USB_NO_DEVICE      USB Error: No device
  @return  PIXY_ERROR_INVALID_PARAMETER  Invalid pararmeter specified
*/
int pixy2_get_blocks(uint16_t max_blocks, struct Block * blocks);

#ifdef __cplusplus
}
#endif

#endif
