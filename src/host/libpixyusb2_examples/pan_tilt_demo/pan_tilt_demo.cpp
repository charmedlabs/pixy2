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

#include <signal.h>
#include "libpixyusb2.h"
#include <PIDLoop.h>

Pixy2 pixy;
PIDLoop panLoop(400, 0, 400, true);
PIDLoop tiltLoop(500, 0, 500, true);
static bool  run_flag = true;

void handle_SIGINT(int unused)
{
  // On CTRL+C - abort! //

  run_flag = false;
}


// Take the biggest block (blocks[0]) that's been around for at least 30 frames (1/2 second)
// and return its index, otherwise return -1
int16_t acquireBlock()
{
  if (pixy.ccc.numBlocks && pixy.ccc.blocks[0].m_age>30)
    return pixy.ccc.blocks[0].m_index;

  return -1;
}


// Find the block with the given index.  In other words, find the same object in the current
// frame -- not the biggest object, but he object we've locked onto in acquireBlock()
// If it's not in the current frame, return NULL
Block *trackBlock(uint8_t index)
{
  uint8_t i;

  for (i=0; i<pixy.ccc.numBlocks; i++)
  {
    if (index==pixy.ccc.blocks[i].m_index)
      return &pixy.ccc.blocks[i];
  }

  return NULL;
}



int main()
{  
  int i=0;
  int16_t index=-1;
  char buf[64]; 
  int32_t panOffset, tiltOffset;
  Block *block=NULL;

  // Catch CTRL+C (SIGINT) signals, otherwise the Pixy object
  // won't be cleaned up correctly, leaving Pixy and possibly USB
  // driver in a defunct state.
  signal (SIGINT, handle_SIGINT);

  // need to initialize pixy!
  pixy.init();

  // use ccc program to track objects
  pixy.changeProg("color_connected_components");
  
  while(1)
  {
    pixy.ccc.getBlocks();
    if (index==-1) // search....
    {
      printf("Searching for block...\n");
      index = acquireBlock();
      if (index>=0)
        printf("Found block!\n");
    }
    // If we've found a block, find it, track it
    if (index>=0)
      block = trackBlock(index);

    
    if (block)
    {        
      i++;
    
      if (i%60==0)
        printf("%d\n", i);   
      
      panOffset = (int32_t)pixy.frameWidth/2 - (int32_t)block->m_x;
      tiltOffset = (int32_t)block->m_y - (int32_t)pixy.frameHeight/2;  
  
      panLoop.update(panOffset);
      tiltLoop.update(tiltOffset);
    
      pixy.setServos(panLoop.m_command, tiltLoop.m_command);
    }
    else // no object detected, go into reset state
    {
      panLoop.reset();
      tiltLoop.reset();
      pixy.setServos(panLoop.m_command, tiltLoop.m_command);
      index = -1;
    }

    if (run_flag==false)
      break;
  }

  printf("exiting...\n");
}

