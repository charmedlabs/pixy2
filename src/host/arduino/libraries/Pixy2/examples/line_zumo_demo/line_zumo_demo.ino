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
#include <ZumoBuzzer.h>

// Zumo speeds, maximum allowed is 400
#define ZUMO_FAST        200
#define ZUMO_SLOW        150
#define X_CENTER         (pixy.frameWidth/2)

Pixy2 pixy;
ZumoMotors motors;
ZumoBuzzer buzzer;

PIDLoop headingLoop(5000, 0, 0, false);

void setup() 
{
  Serial.begin(115200);
  Serial.print("Starting...\n");
  motors.setLeftSpeed(0);
  motors.setRightSpeed(0);

  pixy.init();
  // Turn on both lamps, upper and lower for maximum exposure
  pixy.setLamp(1, 1);
  // change to the line_tracking program.  Note, changeProg can use partial strings, so for example,
  // you can change to the line_tracking program by calling changeProg("line") instead of the whole
  // string changeProg("line_tracking")
  pixy.changeProg("line");

  // look straight and down
  pixy.setServos(500, 1000);
}


void loop()
{
  int8_t res;
  int32_t error; 
  int left, right;
  char buf[96];

  // Get latest data from Pixy, including main vector, new intersections and new barcodes.
  res = pixy.line.getMainFeatures();

  // If error or nothing detected, stop motors
  if (res<=0) 
  {
    motors.setLeftSpeed(0);
    motors.setRightSpeed(0);
    buzzer.playFrequency(500, 50, 15);
    Serial.print("stop ");
    Serial.println(res);
    return;
  }

  // We found the vector...
  if (res&LINE_VECTOR)
  {
    // Calculate heading error with respect to m_x1, which is the far-end of the vector,
    // the part of the vector we're heading toward.
    error = (int32_t)pixy.line.vectors->m_x1 - (int32_t)X_CENTER;

    pixy.line.vectors->print();

    // Perform PID calcs on heading error.
    headingLoop.update(error);

    // separate heading into left and right wheel velocities.
    left = headingLoop.m_command;
    right = -headingLoop.m_command;

    // If vector is heading away from us (arrow pointing up), things are normal.
    if (pixy.line.vectors->m_y0 > pixy.line.vectors->m_y1)
    {
      // ... but slow down a little if intersection is present, so we don't miss it.
      if (pixy.line.vectors->m_flags&LINE_FLAG_INTERSECTION_PRESENT)
      {
        left += ZUMO_SLOW;
        right += ZUMO_SLOW;
      }
      else // otherwise, pedal to the metal!
      {
        left += ZUMO_FAST;
        right += ZUMO_FAST;
      }    
    }
    else  // If the vector is pointing down, or down-ish, we need to go backwards to follow.
    {
      left -= ZUMO_SLOW;
      right -= ZUMO_SLOW;  
    } 
    motors.setLeftSpeed(left);
    motors.setRightSpeed(right);
  }

  // If intersection, do nothing (we've already set the turn), but acknowledge with a beep.
  if (res&LINE_INTERSECTION)
  {
    buzzer.playFrequency(1000, 100, 15);
    pixy.line.intersections->print();
  }

  // If barcode, acknowledge with beep, and set left or right turn accordingly. 
  // When calling setNextTurn(), Pixy will "execute" the turn upon the next intersection, 
  // making the left or right branch in the intersection the new main vector, depending on 
  // the angle passed to setNextTurn(). The robot will then follow the branch.  
  // If the turn is not set, Pixy will choose the straight(est) path by default, but 
  // the default turn can be changed too by calling setDefaultTurn(). The default turn
  // is normally 0 (straight).   
  if (res&LINE_BARCODE)
  {
    buzzer.playFrequency(2000, 100, 15);
    pixy.line.barcodes->print();
    // code==0 is our left-turn sign
    if (pixy.line.barcodes->m_code==0)
      pixy.line.setNextTurn(90); // 90 degrees is a left turn 
    // code==5 is our right-turn sign
    else if (pixy.line.barcodes->m_code==5)
      pixy.line.setNextTurn(-90); // -90 is a right turn 
  }
}
