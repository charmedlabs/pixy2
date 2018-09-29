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

Pixy2        pixy;
static bool  run_flag = true;


void handle_SIGINT(int unused)
{
  // On CTRL+C - abort! //

  run_flag = false;
}


int main()
{
  int  Result;
  uint8_t r, g, b;

  // Catch CTRL+C (SIGINT) signals, otherwise the Pixy object
  // won't be cleaned up correctly, leaving Pixy and possibly USB
  // driver in a defunct state.
  signal (SIGINT, handle_SIGINT);

  
  printf ("=============================================================\n");
  printf ("= PIXY2 Get RGB values demo                                 =\n");
  printf ("=============================================================\n");

  printf ("Connecting to Pixy2...");

  // Initialize Pixy2 Connection //
  {
    Result = pixy.init();

    if (Result < 0)
    {
      printf ("Error\n");
      printf ("pixy.init() returned %d\n", Result);
      return Result;
    }

    printf ("Success\n");
  }

  // Get Pixy2 Version information //
  {
    Result = pixy.getVersion();

    if (Result < 0)
    {
      printf ("pixy.getVersion() returned %d\n", Result);
      return Result;
    }

    pixy.version->print();
  }

  while(run_flag)
  {
    pixy.video.getRGB(158, 104, &r, &g, &b);
    printf("r:%hhu g:%hhu b:%hhu\n", r, g, b);
  }

  printf ("PIXY2 Get RGB Demo Exit\n");
}
