#include <string.h>
#include <vector>
#include "libpixyusb2.h"

Pixy2 pixy_instance;

int demosaic(uint16_t width, uint16_t height, const uint8_t *bayerImage, uint8_t *image);

int init()
{
  return pixy_instance.init();
}

int change_prog(const char *program_name)
{
  return pixy_instance.changeProg(program_name);
}

int get_frame_width()
{
  return pixy_instance.frameWidth;
}

int get_frame_height()
{
  return pixy_instance.frameHeight;
}

void video_get_RGB(int X, int Y, uint8_t *Red, uint8_t *Green, uint8_t *Blue)
{
  pixy_instance.video.getRGB(X, Y, Red, Green, Blue);
}

void video_get_raw_frame(uint8_t *rgb_frame, int size)
{
  uint8_t *bayer_frame;
  uint8_t decoded_rgb_frame[size];

  // need to call stop() befroe calling getRawFrame().
  // Note, you can call getRawFrame multiple times after calling stop().
  // That is, you don't need to call stop() each time.
  pixy_instance.m_link.stop();

  // grab raw frame, BGGR Bayer format, 1 byte per pixel
  pixy_instance.m_link.getRawFrame(&bayer_frame);

  demosaic(PIXY2_RAW_FRAME_WIDTH, PIXY2_RAW_FRAME_HEIGHT, bayer_frame, decoded_rgb_frame);

  for (int index = 0; index < size; index++)
  {
    memcpy(&rgb_frame[index], &decoded_rgb_frame[index], sizeof(uint8_t));
  }

  // Resume currently running program
  pixy_instance.m_link.resume();
}

int ccc_get_blocks(int max_blocks, struct Block *blocks)
{
  int number_of_blocks_copied;
  int blocks_available;
  int index;

  // Get number of blocks detected //
  pixy_instance.ccc.getBlocks();
  blocks_available = pixy_instance.ccc.numBlocks;

  // Copy the maximum amount of blocks that can be copied //
  number_of_blocks_copied = (max_blocks >= blocks_available ? blocks_available : max_blocks);

  // Copy blocks //
  for (index = 0; index != number_of_blocks_copied; ++index)
  {
    memcpy(&blocks[index], &pixy_instance.ccc.blocks[index], sizeof(Block));
  }

  return number_of_blocks_copied;
}

void line_get_all_features()
{
  pixy_instance.line.getAllFeatures();
}

void line_get_main_features()
{
  pixy_instance.line.getMainFeatures();
}

int line_get_vectors(int max_vectors, struct Vector *vectors)
{
  int number_of_vectors_copied;
  int vectors_available;
  int index;

  vectors_available = pixy_instance.line.numVectors;
  number_of_vectors_copied = (max_vectors >= vectors_available ? vectors_available : max_vectors);

  // Copy vectors //
  for (index = 0; index != number_of_vectors_copied; ++index)
  {
    memcpy(&vectors[index], &pixy_instance.line.vectors[index], sizeof(Vector));
  }

  return number_of_vectors_copied;
}

int line_get_intersections(int max_intersections, struct Intersection *intersections)
{
  int number_of_intersections_copied;
  int intersections_available;
  int index;

  intersections_available = pixy_instance.line.numIntersections;
  number_of_intersections_copied = (max_intersections >= intersections_available ? intersections_available : max_intersections);

  // Copy intersections //
  for (index = 0; index != number_of_intersections_copied; ++index)
  {
    memcpy(&intersections[index], &pixy_instance.line.intersections[index], sizeof(Intersection));
  }

  return number_of_intersections_copied;
}

int line_get_barcodes(int max_barcodes, struct Barcode *barcodes)
{
  int number_of_barcodes_copied;
  int barcodes_available;
  int index;

  barcodes_available = pixy_instance.line.numBarcodes;
  number_of_barcodes_copied = (max_barcodes >= barcodes_available ? barcodes_available : max_barcodes);

  // Copy barcodes //
  for (index = 0; index != number_of_barcodes_copied; ++index)
  {
    memcpy(&barcodes[index], &pixy_instance.line.barcodes[index], sizeof(Barcode));
  }

  return number_of_barcodes_copied;
}

int demosaic(uint16_t width, uint16_t height, const uint8_t *bayerImage, uint8_t *image)
{
  uint32_t x, y, xx, yy, r, g, b;
  uint8_t *pixel0, *pixel;

  for (y = 0; y < height; y++)
  {
    yy = y;
    if (yy == 0)
      yy++;
    else if (yy == height - 1)
      yy--;
    pixel0 = (uint8_t *)bayerImage + yy * width;
    for (x = 0; x < width; x++, image += 3)
    {
      xx = x;
      if (xx == 0)
        xx++;
      else if (xx == width - 1)
        xx--;
      pixel = pixel0 + xx;
      if (yy & 1)
      {
        if (xx & 1)
        {
          r = *pixel;
          g = (*(pixel - 1) + *(pixel + 1) + *(pixel + width) + *(pixel - width)) >> 2;
          b = (*(pixel - width - 1) + *(pixel - width + 1) + *(pixel + width - 1) + *(pixel + width + 1)) >> 2;
        }
        else
        {
          r = (*(pixel - 1) + *(pixel + 1)) >> 1;
          g = *pixel;
          b = (*(pixel - width) + *(pixel + width)) >> 1;
        }
      }
      else
      {
        if (xx & 1)
        {
          r = (*(pixel - width) + *(pixel + width)) >> 1;
          g = *pixel;
          b = (*(pixel - 1) + *(pixel + 1)) >> 1;
        }
        else
        {
          r = (*(pixel - width - 1) + *(pixel - width + 1) + *(pixel + width - 1) + *(pixel + width + 1)) >> 2;
          g = (*(pixel - 1) + *(pixel + 1) + *(pixel + width) + *(pixel - width)) >> 2;
          b = *pixel;
        }
      }

      *(image) = r;
      *(image + 1) = g;
      *(image + 2) = b;
    }
  }
}

void set_lamp(int upper, int lower)
{
  pixy_instance.setLamp(upper, lower);
}

void set_servos(int S1_Position, int S2_Position)
{
  pixy_instance.setServos(S1_Position, S2_Position);
}
