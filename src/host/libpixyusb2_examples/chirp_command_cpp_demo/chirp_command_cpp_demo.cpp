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

#define  COLOR_STEP_VALUE               5

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

  // Catch CTRL+C (SIGINT) signals //
  signal (SIGINT, handle_SIGINT);

  printf ("=============================================================\n");
  printf ("= PIXY2 Chirp Command Demo                                  =\n");
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


  printf ("Hit <CTRL+C> to exit...\n");
  
  {
    uint8_t   Purple_Color      = 0xFF;
    uint8_t   Green_Color       = 0x00;
    bool      Green_Increasing  = true;
    uint32_t  Color;
   
    // Exit on <CTRL+C> signal //
    while (run_flag)
    {
      // Set LED using "led_set" chirp command
      //
      // Input is 32 bit unsigned: 00RRGGBB (hex)
      // RR : 8-bit Red component
      // GG : 8-bit Green component
      // BB : 8-bit Blue component
      pixy.m_link.callChirp("led_set", INT32(Color), END_OUT_ARGS, &Result, END_IN_ARGS);

      if (Result)
      {
        printf ("callChirp() Error: %d\n", Result);
        return Result;
      }

      // Update color value //
      if (Green_Increasing)
      {
        // Green is increasing //

        if (Green_Color == 0xFF)
        {
          // Green is maxed out //
          Green_Increasing = false;
        }
        else
        {
          Green_Color  += COLOR_STEP_VALUE;
          Purple_Color -= COLOR_STEP_VALUE;
        }
      }
      else
      {
        // Purple is increasing //

        if (Purple_Color == 0xFF)
        {
          // Purple is maxed out //
          Green_Increasing = true;
        }
        else
        {
          Green_Color  -= COLOR_STEP_VALUE;
          Purple_Color += COLOR_STEP_VALUE;
        }
      }

      // Create 32-bit RGB color value //
      Color = (Purple_Color << 16) + (Green_Color << 8) + Purple_Color;
    }
  }

  printf ("PIXY2 Chirp Command Demo Exit\n");
}

