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
// This sketch is a good place to start if you're just getting started with 
// Pixy and Arduino.  This program simply prints the detected object blocks 
// (including color codes) through the serial console.  It uses the Arduino's 
// ICSP SPI port.  For more information go here:
//
// https://docs.pixycam.com/wiki/doku.php?id=wiki:v2:hooking_up_pixy_to_a_microcontroller_-28like_an_arduino-29
//
  
  
// NOTE, THIS PROGRAM REQUIRES FIRMWARE VERSION 3.0.11 OR GREATER 
#include <Pixy2.h>
#include <stdio.h>

// This is the main Pixy object 
Pixy2 pixy;

void setup()
{
  Serial.begin(115200);
  Serial.print("Starting...\n");
  
  // we must initialize the pixy object
  pixy.init();
  // Getting the RGB pixel values requires the 'video' program
  pixy.changeProg("video");
}

void loop()
{ 
  uint8_t r, g, b; 
  
  // get RGB value at center of frame
  if (pixy.video.getRGB(pixy.frameWidth/2, pixy.frameHeight/2, &r, &g, &b)==0)
  {
    Serial.print("red:");
    Serial.print(r);
    Serial.print(" green:");
    Serial.print(g);
    Serial.print(" blue:");
    Serial.println(b);
  }
}
