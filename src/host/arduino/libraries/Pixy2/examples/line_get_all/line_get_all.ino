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

  // we need to initialize the pixy object
  pixy.init();
  // Change to line tracking program
  pixy.changeProg("line");
}

void loop()
{
  int8_t i;
  char buf[128];
 
  pixy.line.getAllFeatures();

  // print all vectors
  for (i=0; i<pixy.line.numVectors; i++)
  {
    sprintf(buf, "line %d: ", i);
    Serial.print(buf);
    pixy.line.vectors[i].print();
  }
  
  // print all intersections
  for (i=0; i<pixy.line.numIntersections; i++)
  {
    sprintf(buf, "intersection %d: ", i);
    Serial.print(buf);
    pixy.line.intersections[i].print();
  }
  
  // print all barcodes
  for (i=0; i<pixy.line.numBarcodes; i++)
  {
    sprintf(buf, "barcode %d: ", i);
    Serial.print(buf);
    pixy.line.barcodes[i].print();
  }
}

