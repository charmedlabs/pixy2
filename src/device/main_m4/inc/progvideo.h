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

#ifndef _PROGVIDEO_H
#define _PROGVIDEO_H

#include "exec.h"
#include "pixytypes.h"

#define PROG_NAME_VIDEO         "video"
#define PROG_VIDEO_MIN_TYPE     0x70
#define PROG_VIDEO_MAX_TYPE     0x7f

#define TYPE_REQUEST_GETRGB   0x70

#define VIDEO_RGB_SIZE    2

class ProgVideo : public Prog
{
public:
	ProgVideo(uint8_t progIndex);
	virtual ~ProgVideo();
	
	virtual int loop(char *status);

	virtual int packet(uint8_t type, const uint8_t *data, uint8_t len, bool checksum);

private:
	void sendCustom(uint8_t renderFlags=RENDER_FLAG_FLUSH);

};

uint32_t getRGB(uint16_t x, uint16_t y, uint8_t sat);

#endif
