#ifndef __GLOBALS_H__
#define __GLOBALS_H__

typedef unsigned char color;

typedef struct
    {
	/**
	 *  <B>The ORDER of the Red, Blue and Green variables in this structure matters
	 *  because of the way that the ChanFS f_read fills this structure. If you change
	 *  the order the colors will be swapped.</B>
	 */
	color Red;
	color Blue;
	color Green;

    } RBG;



#endif

