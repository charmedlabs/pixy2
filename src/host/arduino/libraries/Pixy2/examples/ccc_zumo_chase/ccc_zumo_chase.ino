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

#include <Pixy2.h>
#include <PIDLoop.h>
#include <ZumoMotors.h>

// this limits how fast Zumo travels forward (400 is max possible for Zumo)
#define MAX_TRANSLATE_VELOCITY  250

Pixy2 pixy;
ZumoMotors motors;

PIDLoop panLoop(350, 0, 600, true);
PIDLoop tiltLoop(500, 0, 700, true);
PIDLoop rotateLoop(300, 600, 300, false);
PIDLoop translateLoop(400, 800, 300, false);

void setup()
{
  Serial.begin(115200);
  Serial.print("Starting...\n");
  
  // initialize motor objects
  motors.setLeftSpeed(0);
  motors.setRightSpeed(0);
  
  // need to initialize pixy object
  pixy.init();
  // user color connected components program
  pixy.changeProg("color_connected_components");
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


void loop()
{  
  static int16_t index = -1;
  int32_t panOffset, tiltOffset, headingOffset, left, right;
  Block *block=NULL;
  
  pixy.ccc.getBlocks();

  if (index==-1) // search....
  {
    Serial.println("Searching for block...");
    index = acquireBlock();
    if (index>=0)
      Serial.println("Found block!");
 }
  // If we've found a block, find it, track it
  if (index>=0)
     block = trackBlock(index);

  // If we're able to track it, move motors
  if (block)
  {
    // calculate pan and tilt errors
    panOffset = (int32_t)pixy.frameWidth/2 - (int32_t)block->m_x;
    tiltOffset = (int32_t)block->m_y - (int32_t)pixy.frameHeight/2;  

    // calculate how to move pan and tilt servos
    panLoop.update(panOffset);
    tiltLoop.update(tiltOffset);

    // move servos
    pixy.setServos(panLoop.m_command, tiltLoop.m_command);

    // calculate translate and rotate errors
    panOffset += panLoop.m_command - PIXY_RCS_CENTER_POS;
    tiltOffset += tiltLoop.m_command - PIXY_RCS_CENTER_POS - PIXY_RCS_CENTER_POS/2 + PIXY_RCS_CENTER_POS/8;

    rotateLoop.update(panOffset);
    translateLoop.update(-tiltOffset);

    // keep translation velocity below maximum
    if (translateLoop.m_command>MAX_TRANSLATE_VELOCITY)
      translateLoop.m_command = MAX_TRANSLATE_VELOCITY;

    // calculate left and right wheel velocities based on rotation and translation velocities
    left = -rotateLoop.m_command + translateLoop.m_command;
    right = rotateLoop.m_command + translateLoop.m_command;

    // set wheel velocities
    motors.setLeftSpeed(left);
    motors.setRightSpeed(right);

    // print the block we're tracking -- wait until end of loop to reduce latency
    block->print();
  }  
  else // no object detected, stop motors, go into search state
  {
    rotateLoop.reset();
    translateLoop.reset();
    motors.setLeftSpeed(0);
    motors.setRightSpeed(0);
    index = -1; // set search state
  }
}


