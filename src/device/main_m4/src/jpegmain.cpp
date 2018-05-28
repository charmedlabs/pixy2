#include <stdint.h>
#include <debug.h>
#include <pixytypes.h>
#include "globals.h"
#include "jpegenc.h"
#include "dct.h"


static uint8_t *g_outBuf;
static uint32_t g_outSize;
static uint32_t g_outIndex;
 
/*****************************************************************************
 * Public functions
 ****************************************************************************/


/*
 * @brief Function for writing to the JPEG output file
 * @param buff[] - Buffer that holds the data to be written
 * @param size   -  The number of bytes to be written
 * @return       - Nothing
 */
void write_jpeg(const unsigned char buff[], const unsigned size)
    {
	uint32_t i;

   	for (i=0; i<size && g_outIndex<g_outSize; i++)
		g_outBuf[g_outIndex++] = buff[i];
    }


void convertYUV(const Frame8 *frame, uint16_t x, uint16_t y, short Y8x8[4][8][8], short Cb8x8[8][8], short Cr8x8[8][8])
{
	uint8_t xx, yy;
	uint8_t R0, G0, B0;
	uint8_t R1, G1, B1;
	uint8_t R2, G2, B2;
	uint8_t R3, G3, B3;
	uint8_t RA, GA, BA;
	uint8_t *pixel0, *pixel; 
	uint16_t width = frame->m_width;

	pixel0 = frame->m_pixels + y*width + x;
	for (yy=0; yy<16; yy+=2, pixel0+=width<<1)
	{
		for (xx=0, pixel=pixel0; xx<16; xx+=2, pixel+=2)
		{
			// calc RGB using interpolation for all 4 pixels 
    		R0 = (*(pixel-width-1)+*(pixel-width+1)+*(pixel+width-1)+*(pixel+width+1))>>2; 
            G0 = (*(pixel-1)+*(pixel+1)+*(pixel+width)+*(pixel-width))>>2; 
            B0 = *pixel;

          	R1 = (*(pixel-width+1)+*(pixel+width+1))>>1;
            G1 = *(pixel+1);
            B1 = (*(pixel-1+1)+*(pixel+1+1))>>1;

           	R2 = (*(pixel-1+width)+*(pixel+1+width))>>1; 
            G2 = *(pixel+width); 
            B2 = (*(pixel-width+width)+*(pixel+width+width))>>1; 

            R3 = *(pixel+width+1); 
            G3 = (*(pixel-1+width+1)+*(pixel+1+width+1)+*(pixel+width+width+1)+*(pixel-width+width+1))>>2; 
            B3 = (*(pixel-width-1+width+1)+*(pixel-width+1+width+1)+*(pixel+width-1+width+1)+*(pixel+width+1+width+1))>>2; 

			RA = (R0+R1+R2+R3)>>2;
			GA = (G0+G1+G2+G3)>>2;
			BA = (B0+B1+B2+B3)>>2;

			// calc YUV
			if (yy>=8)
			{
				if (xx>=8)
				{
					Y8x8[3][yy-8][xx-8] = RGB2Y(R0, G0, B0) - 128;
					Y8x8[3][yy-8][xx+1-8] = RGB2Y(R1, G1, B1) - 128;
					Y8x8[3][yy+1-8][xx-8] = RGB2Y(R2, G2, B2) - 128;
					Y8x8[3][yy+1-8][xx+1-8] = RGB2Y(R3, G3, B3) - 128;
				}
				else
				{
					Y8x8[2][yy-8][xx] = RGB2Y(R0, G0, B0) - 128;
					Y8x8[2][yy-8][xx+1] = RGB2Y(R1, G1, B1) - 128;
					Y8x8[2][yy+1-8][xx] = RGB2Y(R2, G2, B2) - 128;
					Y8x8[2][yy+1-8][xx+1] = RGB2Y(R3, G3, B3) - 128;
				}
			}
			else
			{
				if (xx>=8)
				{
					Y8x8[1][yy][xx-8] = RGB2Y(R0, G0, B0) - 128;
					Y8x8[1][yy][xx+1-8] = RGB2Y(R1, G1, B1) - 128;
					Y8x8[1][yy+1][xx-8] = RGB2Y(R2, G2, B2) - 128;
					Y8x8[1][yy+1][xx+1-8] = RGB2Y(R3, G3, B3) - 128;
				}
				else
				{
					Y8x8[0][yy][xx] = RGB2Y(R0, G0, B0) - 128;
					Y8x8[0][yy][xx+1] = RGB2Y(R1, G1, B1) - 128;
					Y8x8[0][yy+1][xx] = RGB2Y(R2, G2, B2) - 128;
					Y8x8[0][yy+1][xx+1] = RGB2Y(R3, G3, B3) - 128;
				}
			}
		    Cb8x8[yy>>1][xx>>1] = (short)RGB2Cb(RA, GA, BA) - 128;
		    Cr8x8[yy>>1][xx>>1] = (short)RGB2Cr(RA, GA, BA) - 128;
		}
	} 
}

int jpeg_encode(const Frame8 *frame, uint8_t quality, uint8_t *out, uint32_t *size)
    {

    short Y8x8[4][8][8]; // four 8x8 blocks - 16x16
    short Cb8x8[8][8];
    short Cr8x8[8][8];

    unsigned short x, y;

	g_outBuf = out;
	g_outSize = *size;
	g_outIndex = 0;

    /*
     * Process the bitmap image data in 16x16 blocks, (16x16 because of chroma subsampling)
     * The resulting image will be truncated on the right/down side if its width/height is not N*16.
     * The data is written into <file_jpg> file by write_jpeg() function which Huffman encoder uses
     * to flush its output, so this file should be opened before the call of huffman_start().
     */
	setQuality(quality);

    huffman_start(frame->m_height & -16, frame->m_width & -16);

    for (y = 0; y < frame->m_height - 15; y += 16)
	{
	for (x = 0; x < frame->m_width - 15; x += 16)
	    {
		convertYUV(frame, x, y, Y8x8, Cb8x8, Cr8x8); 

#if 1
	    // 1 Y-compression
	    dct(Y8x8[0], Y8x8[0]);
	    huffman_encode(HUFFMAN_CTX_Y, (short*) Y8x8[0]);
	    // 2 Y-compression
	    dct(Y8x8[1], Y8x8[1]);
	    huffman_encode(HUFFMAN_CTX_Y, (short*) Y8x8[1]);
	    // 3 Y-compression
	    dct(Y8x8[2], Y8x8[2]);
	    huffman_encode(HUFFMAN_CTX_Y, (short*) Y8x8[2]);
	    // 4 Y-compression
	    dct(Y8x8[3], Y8x8[3]);
	    huffman_encode(HUFFMAN_CTX_Y, (short*) Y8x8[3]);
	    // Cb-compression
	    dct(Cb8x8, Cb8x8);
	    huffman_encode(HUFFMAN_CTX_Cb, (short*) Cb8x8);
	    // Cr-compression
	    dct(Cr8x8, Cr8x8);
	    huffman_encode(HUFFMAN_CTX_Cr, (short*) Cr8x8);
#endif
	    }
	}
    huffman_stop();

	*size = g_outIndex;
    return 0;

    }
