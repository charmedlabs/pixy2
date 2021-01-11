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

#ifndef CAMERA_H
#define CAMERA_H

#include "chirp.hpp"
#include "sccb.h"
#include "pixytypes.h"
#include <cameravals.h>


int cam_init(uint16_t *hwVer);

int32_t cam_setMode(const uint8_t &mode);
uint32_t cam_getMode();

int32_t cam_setAWB(const uint8_t &awb);
uint32_t cam_getAWB();
							   
int32_t cam_setWBV(const uint32_t &wbv);
int32_t cam_getWBV();

int32_t cam_setAEC(const uint8_t &aec);
uint32_t cam_getAEC();

int32_t cam_setECV(const uint32_t &ecv);
uint32_t cam_getECV();

int32_t cam_setBrightness(const uint8_t &brightness);
uint32_t cam_getBrightness();

int32_t cam_setSaturation(const uint8_t &saturation);
uint32_t cam_getSaturation();

int32_t cam_setFlickerAvoidance(const uint8_t &fa);
int cam_testPattern(const uint8_t &enable);

int32_t cam_getFrameChirp(const uint8_t &type, const uint16_t &xOffset, const uint16_t &yOffset, const uint16_t &xWidth, const uint16_t &yWidth, Chirp *chirp);
int32_t cam_getFrameChirpFlags(const uint8_t &type, const uint16_t &xOffset, const uint16_t &yOffset, const uint16_t &xWidth, const uint16_t &yWidth, Chirp *chirp, uint8_t renderFlags=RENDER_FLAG_FLUSH, bool remote=false);
int32_t cam_getFrame(uint8_t *memory, uint32_t memSize, uint8_t type, uint16_t xOffset, uint16_t yOffset, uint16_t xWidth, uint16_t yWidth);
int32_t cam_getTiming(uint16_t *hblank, uint16_t *hactive, uint16_t *vblank, uint16_t *vactive);
int32_t cam_setReg8(const uint16_t &reg, const uint8_t &value);
int32_t cam_setReg16(const uint16_t &reg, const uint16_t &value);
int32_t cam_setReg32(const uint16_t &reg, const uint32_t &value);
int32_t cam_getReg8(const uint16_t &reg);
int32_t cam_getReg16(const uint16_t &reg);
int32_t cam_getReg32(const uint16_t &reg);
int32_t cam_getFramePeriod();
float cam_getFPS();
int32_t cam_getBlankTime();

int32_t cam_setFramerate(const uint8_t &framerate);
int32_t cam_setResolution(const uint16_t &xoffset, const uint16_t &yoffset, const uint16_t &width, const uint16_t &height);

void cam_loadParams();
int32_t cam_sendFrame(Chirp *chirp, uint16_t xWidth, uint16_t yWidth, uint8_t renderFlags=RENDER_FLAG_FLUSH, uint32_t fourcc=FOURCC('B','A','8','1'));

extern CSccb *g_sccb;
extern Frame8 g_rawFrame;


 #endif
