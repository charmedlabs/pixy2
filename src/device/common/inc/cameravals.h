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

#ifndef CAMERAVALS_H
#define CAMERAVALS_H

#define CAM_MODE0               0x00
#define CAM_MODE1               0x01

#define CAM_LIGHT_NORMAL        0
#define CAM_LIGHT_LOW           1
#define CAM_LIGHT_HIGH          2 // not sure if combining high light and low light exposure is possible, or good

#define CAM_BRIGHTNESS_DEFAULT  50
#define CAM_BRIGHTNESS_SCALE    2.1
#define CAM_SATURATION_DEFAULT	90 
#define CAM_FRAMERATE_DEFAULT   61


#define CAM_RES0                0x00 
#define CAM_RES1                0x01
#define CAM_RES2                0x02
#define CAM_RES0_WIDTH          1280
#define CAM_RES0_HEIGHT         800
#define CAM_RES1_WIDTH          640
#define CAM_RES1_HEIGHT         400
#define CAM_RES2_WIDTH          316
#define CAM_RES2_HEIGHT         208
#define CAM_RES3_WIDTH          (CAM_RES2_WIDTH+320)
#define CAM_RES3_HEIGHT         (CAM_RES2_HEIGHT/2)

#define CAM_PIXEL_SKIP 			    8 
#define CAM_LINE_SKIP 			    12
#define CAM_PREBUF_LEN			    64
#define CAM_FRAME_HEADER_LEN    36

#define CAM_GRAB_M0R0           (CAM_RES0<<4 | CAM_MODE0)
#define CAM_GRAB_M1R1           (CAM_RES1<<4 | CAM_MODE1)
#define CAM_GRAB_M1R2           (CAM_RES2<<4 | CAM_MODE1)

#endif
