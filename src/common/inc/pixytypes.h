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

#ifndef PIXYTYPES_H
#define PIXYTYPES_H

#include <stdint.h>

#define CL_NUM_SIGNATURES            7

#define RENDER_FLAG_FLUSH            0x01	// add to stack, render immediately
#define RENDER_FLAG_BLEND            0x02	// blend with a previous images in image stack
#define RENDER_FLAG_START            0x04	// indicate start of frame of data

#define PRM_FLAG_INTERNAL            0x00000001
#define PRM_FLAG_ADVANCED            0x00000002
#define PRM_FLAG_HEX_FORMAT          0x00000010
#define PRM_FLAG_SIGNED              0x00000080

// render-specific flags
#define PRM_FLAG_SLIDER              0x00000100
#define PRM_FLAG_CHECKBOX            0x00000200

// events
#define EVT_PARAM_CHANGE             1
#define EVT_RENDER_FLUSH             2
#define EVT_PROG_CHANGE              3

// text message flags
#define TM_FLAG_PRIORITY_NORMAL      0
#define TM_FLAG_PRIORITY_HIGH        1

#define PIXY_TYPE_REPONSE_ERROR              0x03
#define PIXY_TYPE_RESPONSE_BLOCKS            0x21
#define PIXY_TYPE_REQUEST_BLOCKS             0x20
#define PIXY_TYPE_REQUEST_LINES              0x30
#define PIXY_TYPE_RESPONSE_LINES             0x31

#define TM_IMPORTANT               1

struct ColorSignature
{
	ColorSignature()
	{
		m_uMin = m_uMax = m_uMean = m_vMin = m_vMax = m_vMean = m_type = 0;
	}	

    int32_t m_uMin;
    int32_t m_uMax;
    int32_t m_uMean;
    int32_t m_vMin;
    int32_t m_vMax;
    int32_t m_vMean;
	uint32_t m_rgb;
	uint32_t m_type;
};


struct Point16
{
    Point16()
    {
        m_x = m_y = 0;
    }

    Point16(int16_t x, int16_t y)
    {
        m_x = x;
        m_y = y;
    }

	uint32_t dist2(const Point16 &p) const
	{
		int32_t diffx, diffy;
		
		diffx = p.m_x - m_x;
		diffy = p.m_y - m_y;
		
		return diffx*diffx + diffy*diffy;
	}
	
    int16_t m_x;
    int16_t m_y;
};

struct Point32
{
    Point32()
    {
        m_x = m_y = 0;
    }

    Point32(int32_t x, int32_t y)
    {
        m_x = x;
        m_y = y;
    }

    int32_t m_x;
    int32_t m_y;
};

struct Frame8
{
    Frame8()
    {
        m_pixels = (uint8_t *)NULL;
        m_width = m_height = 0;
    }

    Frame8(uint8_t *pixels, uint16_t width, uint16_t height)
    {
        m_pixels = pixels;
        m_width = width;
        m_height = height;
    }

    uint8_t *m_pixels;
    int16_t m_width;
    int16_t m_height;
};

struct RectA
{
    RectA()
    {
        m_xOffset = m_yOffset = m_width = m_height = 0;
    }

    RectA(uint16_t xOffset, uint16_t yOffset, uint16_t width, uint16_t height)
    {
        m_xOffset = xOffset;
        m_yOffset = yOffset;
        m_width = width;
        m_height = height;
    }

    uint16_t m_xOffset;
    uint16_t m_yOffset;
    uint16_t m_width;
    uint16_t m_height;
};

struct RectB
{
    RectB()
    {
        m_left = m_right = m_top = m_bottom = 0;
    }

    RectB(uint16_t left, uint16_t right, uint16_t top, uint16_t bottom)
    {
        m_left = left;
        m_right = right;
        m_top = top;
        m_bottom = bottom;
    }

    uint16_t m_left;
    uint16_t m_right;
    uint16_t m_top;
    uint16_t m_bottom;
};



struct BlobA2
{
    BlobA2()
    {
        m_model = m_left = m_right = m_top = m_bottom = 0;
    }

    BlobA2(uint16_t model, uint16_t left, uint16_t right, uint16_t top, uint16_t bottom)
    {
        m_model = model;
        m_left = left;
        m_right = right;
        m_top = top;
        m_bottom = bottom;
    }

    uint16_t m_model;
    uint16_t m_left;
    uint16_t m_right;
    uint16_t m_top;
    uint16_t m_bottom;
};

struct BlobB2
{
    BlobB2()
    {
        m_model = m_left = m_right = m_top = m_bottom = m_angle = 0;
    }

    BlobB2(uint16_t model, uint16_t left, uint16_t right, uint16_t top, uint16_t bottom, int16_t angle)
    {
        m_model = model;
        m_left = left;
        m_right = right;
        m_top = top;
        m_bottom = bottom;
        m_angle = angle;
    }

    uint16_t m_model;
    uint16_t m_left;
    uint16_t m_right;
    uint16_t m_top;
    uint16_t m_bottom;
    int16_t m_angle;
};

struct BlobC
{
    BlobC()
    {
        m_model = m_x = m_y = m_width = m_height = m_angle = 0;
    }

    BlobC(uint16_t model, uint16_t x, uint16_t y, uint16_t width, uint16_t height, int16_t angle)
    {
        m_model = model;
        m_x = x;
        m_y = y;
        m_width = width;
        m_height = height;
        m_angle = angle;
		m_index = m_age = 0;
    }
	
    uint16_t m_model;
    uint16_t m_x;
    uint16_t m_y;
    uint16_t m_width;
    uint16_t m_height;
    int16_t m_angle;
	uint8_t m_index;
	uint8_t m_age;
};


struct HuePixel
{
    HuePixel()
    {
        m_u = m_v = 0;
    }

    HuePixel(int8_t u, int8_t v)
    {
        m_u = u;
        m_v = v;
    }

    int8_t m_u;
    int8_t m_v;
};

struct Fpoint
{
    Fpoint()
    {
        m_x = m_y = 0.0;
    }

    Fpoint(float x, float y)
    {
        m_x = x;
        m_y = y;
    }

    float m_x;
    float m_y;
};

struct UVPixel
{
    UVPixel()
    {
        m_u = m_v = 0;
    }

    UVPixel(int32_t u, int32_t v)
    {
        m_u = u;
        m_v = v;
    }

    int32_t m_u;
    int32_t m_v;
};

struct RGBPixel
{
    RGBPixel()
    {
        m_r = m_g = m_b = 0;
    }

    RGBPixel(uint8_t r, uint8_t g, uint8_t b)
    {
        m_r = r;
        m_g = g;
        m_b = b;
    }

    uint8_t m_r;
	uint8_t m_g;
	uint8_t m_b;
};


struct Line
{
    Line()
    {
        m_slope = m_yi = 0.0;
    }
    Line(float slope, float yi)
    {
        m_slope = slope;
        m_yi = yi;
    }

    float m_slope;
    float m_yi;
};

typedef long long longlong;

#endif // PIXYTYPES_H
