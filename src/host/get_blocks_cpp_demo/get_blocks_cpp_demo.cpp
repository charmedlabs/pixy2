#include "libpixyusb2.h"

Pixy2 pixy;

void  get_blocks()
{
  int  Block_Index;

  // Query Pixy for blocks //
  pixy.ccc.getBlocks();

  // Were blocks detected? //
  if (pixy.ccc.numBlocks)
  {
    // Blocks detected - print them! //

    printf ("Detected %d block(s)\n", pixy.ccc.numBlocks);

    for (Block_Index = 0; Block_Index < pixy.ccc.numBlocks; ++Block_Index)
    {
      printf ("  Block %d: ", Block_Index + 1);
      pixy.ccc.blocks[Block_Index].print();
    }
  }
}

int main()
{
  int  Result;

  printf ("Connecting to Pixy2...");

  Result = pixy.init();

  if (Result < 0)
  {
    printf ("Error\n");
    printf ("pixy.init() returned %d\n", Result);
    return Result;
  }

  printf ("Success\n");

  while(1)
  {
    get_blocks();
  }
}
