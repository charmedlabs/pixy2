#include "libpixyusb2.h"

Pixy2 pixy;

void loop()
{
    
  int i; 
  // grab blocks!
  pixy.ccc.getBlocks();
  
  // If there are detect blocks, print them!
  if (pixy.ccc.numBlocks)
  {
    printf("Detected %d\n", pixy.ccc.numBlocks);
    for (i=0; i<pixy.ccc.numBlocks; i++)
    {
      printf("  block %d: ", i);
      pixy.ccc.blocks[i].print();
    }
  }
  pixy.m_link.callChirp("hello");
}

int main()
{
  int res;
  res = pixy.init();
  if (res<0)
  {
    printf("unable to open pixy object\n");
    return 0;
  }
  while(1)
    loop();
}
