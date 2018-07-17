#include <string.h>
#include "libpixyusb2.h"

Pixy2  pixy_instance;

int init()
{
  return  pixy_instance.init();
}

int ccc_get_blocks (int  max_blocks, struct Block *  blocks)
{
  int  number_of_blocks_copied;
  int  blocks_available;
  int  index;
  
  pixy_instance.ccc.getBlocks();
  blocks_available = pixy_instance.ccc.numBlocks;

  number_of_blocks_copied = (max_blocks >= blocks_available ? blocks_available : max_blocks);   

  for (index = 0; index != number_of_blocks_copied; ++index)  {
    memcpy (&blocks[index], &pixy_instance.ccc.blocks[index], sizeof(Block));
  }

  return number_of_blocks_copied;
}
