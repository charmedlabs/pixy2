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

Pixy2 pixy;

void setup()
{
  Serial.begin(115200);
  Serial.print("Starting...\n");

  pixy.init();
}

void loop()
{
  pixy.setServos(PIXY_RCS_MIN_POS, PIXY_RCS_MIN_POS);
  pixy.setLED(255, 0, 0); // red

  delay(1000);

  pixy.setServos(PIXY_RCS_CENTER_POS, PIXY_RCS_CENTER_POS);
  pixy.setLED(0, 255, 0); // green

  delay(1000);

  pixy.setServos(PIXY_RCS_MAX_POS, PIXY_RCS_MAX_POS);
  pixy.setLED(0, 0, 255); // blue

  delay(1000);  
}

