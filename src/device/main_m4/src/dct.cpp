#include "globals.h"
#include "dct.h"

/**
 * @brief Fast Discrete Cosine Transform converts 8x8 pixel block into frequencies. The lowest frequencies are at the upper-left corner.
 *  The input and output could point at the same array, in this case the data will be overwritten.
 *  @param  pixels - 8x8 pixel array;
 *  @param  data   - 8x8 frequency block;
 *  @return        - Nothing
 */

void dct(short pixels[8][8], short data[8][8])
    {
    short rows[8][8];
    unsigned i;

    static const short // Ci = cos(i*PI/16)*(1 << 14);
    C1 = 16070, C2 = 15137, C3 = 13623, C4 = 11586, C5 = 9103, C6 = 6270, C7 =
	    3197;

    // simple but fast DCT - 22*16 multiplication 28*16 additions and 8*16 shifts.

    /* transform rows */
    for (i = 0; i < 8; i++)
	{
	short s07, s16, s25, s34, s0734, s1625;
	short d07, d16, d25, d34, d0734, d1625;

	s07 = pixels[i][0] + pixels[i][7];
	d07 = pixels[i][0] - pixels[i][7];
	s16 = pixels[i][1] + pixels[i][6];
	d16 = pixels[i][1] - pixels[i][6];
	s25 = pixels[i][2] + pixels[i][5];
	d25 = pixels[i][2] - pixels[i][5];
	s34 = pixels[i][3] + pixels[i][4];
	d34 = pixels[i][3] - pixels[i][4];

	rows[i][1] = (C1 * d07 + C3 * d16 + C5 * d25 + C7 * d34) >> 14;
	rows[i][3] = (C3 * d07 - C7 * d16 - C1 * d25 - C5 * d34) >> 14;
	rows[i][5] = (C5 * d07 - C1 * d16 + C7 * d25 + C3 * d34) >> 14;
	rows[i][7] = (C7 * d07 - C5 * d16 + C3 * d25 - C1 * d34) >> 14;

	s0734 = s07 + s34;
	d0734 = s07 - s34;
	s1625 = s16 + s25;
	d1625 = s16 - s25;

	rows[i][0] = (C4 * (s0734 + s1625)) >> 14;
	rows[i][4] = (C4 * (s0734 - s1625)) >> 14;

	rows[i][2] = (C2 * d0734 + C6 * d1625) >> 14;
	rows[i][6] = (C6 * d0734 - C2 * d1625) >> 14;
	}

    /* transform columns */
    for (i = 0; i < 8; i++)
	{
	short s07, s16, s25, s34, s0734, s1625;
	short d07, d16, d25, d34, d0734, d1625;

	s07 = rows[0][i] + rows[7][i];
	d07 = rows[0][i] - rows[7][i];
	s16 = rows[1][i] + rows[6][i];
	d16 = rows[1][i] - rows[6][i];
	s25 = rows[2][i] + rows[5][i];
	d25 = rows[2][i] - rows[5][i];
	s34 = rows[3][i] + rows[4][i];
	d34 = rows[3][i] - rows[4][i];

	data[1][i] = (C1 * d07 + C3 * d16 + C5 * d25 + C7 * d34) >> 16;
	data[3][i] = (C3 * d07 - C7 * d16 - C1 * d25 - C5 * d34) >> 16;
	data[5][i] = (C5 * d07 - C1 * d16 + C7 * d25 + C3 * d34) >> 16;
	data[7][i] = (C7 * d07 - C5 * d16 + C3 * d25 - C1 * d34) >> 16;

	s0734 = s07 + s34;
	d0734 = s07 - s34;
	s1625 = s16 + s25;
	d1625 = s16 - s25;

	data[0][i] = (C4 * (s0734 + s1625)) >> 16;
	data[4][i] = (C4 * (s0734 - s1625)) >> 16;

	data[2][i] = (C2 * d0734 + C6 * d1625) >> 16;
	data[6][i] = (C6 * d0734 - C2 * d1625) >> 16;
	}
    }
