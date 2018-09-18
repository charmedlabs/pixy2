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

#include "libpixyusb2.h"

Pixy2        pixy;


int writePPM(uint16_t width, uint16_t height, uint32_t *image, const char *filename)
{
  int i, j;
  char fn[32];

  sprintf(fn, "%s.ppm", filename);
  FILE *fp = fopen(fn, "wb");
  if (fp==NULL)
    return -1;
  fprintf(fp, "P6\n%d %d\n255\n", width, height);
  for (j=0; j<height; j++)
  {
    for (i=0; i<width; i++)
      fwrite((char *)(image + j*width + i), 1, 3, fp);
  }
  fclose(fp);
  return 0;
}

int demosaic(uint16_t width, uint16_t height, const uint8_t *bayerImage, uint32_t *image)
{
  uint32_t x, y, xx, yy, r, g, b;
  uint8_t *pixel0, *pixel;
  
  for (y=0; y<height; y++)
  {
    yy = y;
    if (yy==0)
      yy++;
    else if (yy==height-1)
      yy--;
    pixel0 = (uint8_t *)bayerImage + yy*width;
    for (x=0; x<width; x++, image++)
    {
      xx = x;
      if (xx==0)
	xx++;
      else if (xx==width-1)
	xx--;
      pixel = pixel0 + xx;
      if (yy&1)
      {
        if (xx&1)
        {
          r = *pixel;
          g = (*(pixel-1)+*(pixel+1)+*(pixel+width)+*(pixel-width))>>2;
          b = (*(pixel-width-1)+*(pixel-width+1)+*(pixel+width-1)+*(pixel+width+1))>>2;
        }
        else
        {
          r = (*(pixel-1)+*(pixel+1))>>1;
          g = *pixel;
          b = (*(pixel-width)+*(pixel+width))>>1;
        }
      }
      else
      {
        if (xx&1)
        {
          r = (*(pixel-width)+*(pixel+width))>>1;
          g = *pixel;
          b = (*(pixel-1)+*(pixel+1))>>1;
        }
        else
        {
          r = (*(pixel-width-1)+*(pixel-width+1)+*(pixel+width-1)+*(pixel+width+1))>>2;
          g = (*(pixel-1)+*(pixel+1)+*(pixel+width)+*(pixel-width))>>2;
          b = *pixel;
        }
      }
      *image = (b<<16) | (g<<8) | r; 
    }
  }
}


int main()
{
  int  Result;
  uint8_t *bayerFrame;
  uint32_t rgbFrame[PIXY2_RAW_FRAME_WIDTH*PIXY2_RAW_FRAME_HEIGHT];
  
  printf ("=============================================================\n");
  printf ("= PIXY2 Get Raw Frame Example                               =\n");
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

  // need to call stop() befroe calling getRawFrame().
  // Note, you can call getRawFrame multiple times after calling stop().
  // That is, you don't need to call stop() each time.
  pixy.m_link.stop();

  // grab raw frame, BGGR Bayer format, 1 byte per pixel
  pixy.m_link.getRawFrame(&bayerFrame);
  // convert Bayer frame to RGB frame
  demosaic(PIXY2_RAW_FRAME_WIDTH, PIXY2_RAW_FRAME_HEIGHT, bayerFrame, rgbFrame);
  // write frame to PPM file for verification
  Result = writePPM(PIXY2_RAW_FRAME_WIDTH, PIXY2_RAW_FRAME_HEIGHT, rgbFrame, "out");
  if (Result==0)
    printf("Write frame to out.ppm\n");
  
  // Call resume() to resume the current program, otherwise Pixy will be left
  // in "paused" state.  
  pixy.m_link.resume();
}
