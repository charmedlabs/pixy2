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

#include <string.h>
#include <pixy_init.h>
#include <pixyvals.h>
#include "camera.h"
#include "param.h"
#include "misc.h"
#include "exec.h"
#include "smlink.hpp"

static const ProcModule g_module[] =
{
    {
    "cam_setMode",
    (ProcPtr)cam_setMode, 
    {CRP_INT8, END}, 
    "Set camera mode"
    "@p mode 0=25 FPS, 1280x800; 1=50 FPS, 640x400"
    "@r 0 if success, negative if error"
    },
    {
    "cam_getMode", 
    (ProcPtr)cam_getMode, 
    {END},
    "Get camera mode"
    "@r mode value"
    },
    {
    "cam_setAWB",
    (ProcPtr)cam_setAWB, 
    {CRP_INT8, END}, 
    "Enable/disable Auto White Balance (AWB)" 
    "@p enable (bool) 0=disable, 1=enable"
    "@r 0 if success, negative if error"
    },
    {
    "cam_getAWB", 
    (ProcPtr)cam_getAWB, 
    {END},
    "Get Auto White Balance (AWB) enable"
    "@r (bool) enable value"
    },
    {
    "cam_setWBV", 
    (ProcPtr)cam_setWBV, 
    {CRP_INT32, END},
    "Set White Balance Value (WBV)"
    "@p wbv white balance value"
    "@r 0 if success, negative if error"
    },
    {
    "cam_getWBV", 
    (ProcPtr)cam_getWBV, 
    {END},
    "Get White Balance Value (WBV)"
    "@r white balance value"
    },
    {
    "cam_setAEC", 
    (ProcPtr)cam_setAEC, 
    {CRP_INT8, END},
    "Set Auto Exposure Compensation (AEC)"
    "@p enable (bool) 0=disable, 1=enable"
    "@r 0 if success, negative if error"
    },
    {
    "cam_getAEC", 
    (ProcPtr)cam_getAEC, 
    {END},
    "Get Auto Exposure Compensation (AEC)"
    "@r (bool) enable value"
    },
    {
    "cam_setECV", 
    (ProcPtr)cam_setECV, 
    {CRP_INT32, END},
    "Set Exposure Compensation Value (ECV)"
    "@p exposure compensation value"
    "@r 0 if success, negative if error"
    },
    {
    "cam_getECV", 
    (ProcPtr)cam_getECV, 
    {END},
    "Get Exposure Compensation Value (ECV)"
    "@r exposure compensation value"
    },
    {
    "cam_setBrightness", 
    (ProcPtr)cam_setBrightness, 
    {CRP_INT8, END},
    "Set brightness value to increase or decrease exposure (only applies when AEC is enabled)"
    "@p brightness value between 0 and 255"
    "@r 0 if success, negative if error"
    },
    {
    "cam_getBrightness", 
    (ProcPtr)cam_getBrightness, 
    {END},
    "Get brightness value"
    "@r brightness value"
    },
    {
    "cam_setSaturation", 
    (ProcPtr)cam_setSaturation, 
    {CRP_INT8, END},
    "Set color saturation value"
    "@p saturation value between 0 and 255"
    "@r 0 if success, negative if error"
    },
    {
    "cam_setFlickerAvoidance", 
    (ProcPtr)cam_setFlickerAvoidance, 
    {CRP_INT8, END},
    "Set flicker avoidance"
    "@p enable (0) disabled (1) enabled"
    "@r 0 if success, negative if error"
    },
    {
    "cam_getBrightness", 
    (ProcPtr)cam_getBrightness, 
    {END},
    "Get brightness value"
    "@r brightness value"
    },
    {
    "cam_testPattern", 
    (ProcPtr)cam_testPattern, 
    {CRP_INT8, END},
    "Set test pattern display"
    "@p enable (0) normal mode (1) test pattern"
    "@r always returns 0"
    },
    {
    "cam_getFrame", 
    (ProcPtr)cam_getFrameChirp, 
    {CRP_INT8, CRP_INT16, CRP_INT16, CRP_INT16, CRP_INT16, END},
    "Get a frame from the camera"
    "@p mode one of the following CAM_GRAB_M0R0 (0x00), CAM_GRAB_M1R1 (0x11), CAM_GRAB_M1R2 (0x21)"
    "@p xOffset x offset counting from left"
    "@p yOffset y offset counting from top"
    "@p width width of frame"
    "@p height height of frame"
    "@r 0 if success, negative if error"
    "@r BA81 formatted data"
    },
    {
    "cam_setReg8",
    (ProcPtr)cam_setReg8,
    {CRP_INT16, CRP_INT8, END},
    "Write an 8-bit SCCB register value on the camera chip"
    "@p address register address"
    "@p value register value to set"
    "@r 0 if success, negative if error"
    },  
    {
    "cam_setReg16",
    (ProcPtr)cam_setReg16,
    {CRP_INT16, CRP_INT16, END},
    "Write an 16-bit SCCB register value on the camera chip"
    "@p address register address"
    "@p value register value to set"
    "@r 0 if success, negative if error"
    },  
    {
    "cam_setReg32",
    (ProcPtr)cam_setReg32,
    {CRP_INT16, CRP_INT32, END},
    "Write an 32-bit SCCB register value on the camera chip"
    "@p address register address"
    "@p value register value to set"
    "@r 0 if success, negative if error"
    },  
    {
    "cam_getReg8",
    (ProcPtr)cam_getReg8,
    {CRP_INT16, END},
    "Read an 8-bit SCCB register on the camera chip"
    "@p address register address"
    "@r 0 register value"
    },  
    {
    "cam_getReg16",
    (ProcPtr)cam_getReg16,
    {CRP_INT16, END},
    "Read an 16-bit SCCB register on the camera chip"
    "@p address register address"
    "@r 0 register value"
    },  
    {
    "cam_getReg32",
    (ProcPtr)cam_getReg32,
    {CRP_INT16, END},
    "Read an 32-bit SCCB register on the camera chip"
    "@p address register address"
    "@r 0 register value"
    },  
    {
    "cam_getFramePeriod",
    (ProcPtr)cam_getFramePeriod,
    {END},
    "Return M0 frame period measured and updated after each frame grab"
    "@r frame period in microseconds"
    },  
    {
    "cam_getBlankTime",
    (ProcPtr)cam_getBlankTime,
    {END},
    "Return M0 blanking time measured and updated after each frame grab"
    "@r blank time in microseconds"
    },  
    END
};

    
CSccb *g_sccb = NULL;
Frame8 g_rawFrame;

static uint8_t g_mode = 0;
static uint8_t g_awb = 1;
static uint8_t g_aec = 1;
static uint8_t g_brightness = CAM_BRIGHTNESS_DEFAULT;
static uint8_t g_saturation = CAM_SATURATION_DEFAULT;
static ChirpProc g_getFrameM0 = -1;
static ChirpProc g_getTimingM0 = -1;
static uint32_t g_aecValue = 0;
static uint32_t g_awbValue = 0;

enum CamCommandType
{
    CAM_END, 
    CAM_DELAY,
    CAM_SEND_COMMAND,
    CAM_APPLY_PATCH,
    CAM_REG_WRITE8,
    CAM_REG_WRITE16,
    CAM_REG_WRITE32,
    CAM_SET_RESOLUTION
};
            
struct CamCommand
{
    CamCommandType command;
    uint16_t reg;
    uint32_t val;
};

#define CREG(a, b) (0x8000 | (a<<10) | b)

const CamCommand camAPGAData[] =
{
#if 0
// [Step4-APGA]
//[APGA Settings M 85% 2015/05/07 22:24:13]
      CAM_REG_WRITE16, 0x3640, 0x0250, // P_G1_P0Q0 - [0:00:08.042]
      CAM_REG_WRITE16, 0x3642, 0x862B, // P_G1_P0Q1 - [0:00:08.045]
      CAM_REG_WRITE16, 0x3644, 0x3E50, // P_G1_P0Q2 - [0:00:08.048]
      CAM_REG_WRITE16, 0x3646, 0x248D, // P_G1_P0Q3 - [0:00:08.051]
      CAM_REG_WRITE16, 0x3648, 0x45EF, // P_G1_P0Q4 - [0:00:08.054]
      CAM_REG_WRITE16, 0x364A, 0x00D0, // P_R_P0Q0 - [0:00:08.057]
      CAM_REG_WRITE16, 0x364C, 0xE6CB, // P_R_P0Q1 - [0:00:08.060]
      CAM_REG_WRITE16, 0x364E, 0x6430, // P_R_P0Q2 - [0:00:08.063]
      CAM_REG_WRITE16, 0x3650, 0x5FCD, // P_R_P0Q3 - [0:00:08.066]
      CAM_REG_WRITE16, 0x3652, 0x092F, // P_R_P0Q4 - [0:00:08.069]
      CAM_REG_WRITE16, 0x3654, 0x0130, // P_B_P0Q0 - [0:00:08.072]
      CAM_REG_WRITE16, 0x3656, 0x590B, // P_B_P0Q1 - [0:00:08.075]
      CAM_REG_WRITE16, 0x3658, 0x568F, // P_B_P0Q2 - [0:00:08.078]
      CAM_REG_WRITE16, 0x365A, 0x5FAC, // P_B_P0Q3 - [0:00:08.081]
      CAM_REG_WRITE16, 0x365C, 0x0430, // P_B_P0Q4 - [0:00:08.084]
      CAM_REG_WRITE16, 0x365E, 0x0110, // P_G2_P0Q0 - [0:00:08.087]
      CAM_REG_WRITE16, 0x3660, 0x978C, // P_G2_P0Q1 - [0:00:08.091]
      CAM_REG_WRITE16, 0x3662, 0x5030, // P_G2_P0Q2 - [0:00:08.093]
      CAM_REG_WRITE16, 0x3664, 0x2DAD, // P_G2_P0Q3 - [0:00:08.096]
      CAM_REG_WRITE16, 0x3666, 0x764E, // P_G2_P0Q4 - [0:00:08.099]
      CAM_REG_WRITE16, 0x3680, 0xB8C7, // P_G1_P1Q0 - [0:00:08.102]
      CAM_REG_WRITE16, 0x3682, 0xC149, // P_G1_P1Q1 - [0:00:08.105]
      CAM_REG_WRITE16, 0x3684, 0x3D6F, // P_G1_P1Q2 - [0:00:08.109]
      CAM_REG_WRITE16, 0x3686, 0x366C, // P_G1_P1Q3 - [0:00:08.111]
      CAM_REG_WRITE16, 0x3688, 0x8511, // P_G1_P1Q4 - [0:00:08.114]
      CAM_REG_WRITE16, 0x368A, 0x95AB, // P_R_P1Q0 - [0:00:08.117]
      CAM_REG_WRITE16, 0x368C, 0xD7EA,// P_R_P1Q1 - [0:00:08.121]
      CAM_REG_WRITE16, 0x368E, 0x1FCF,// P_R_P1Q2 - [0:00:08.124]
      CAM_REG_WRITE16, 0x3690, 0x126C,// P_R_P1Q3 - [0:00:08.126]
      CAM_REG_WRITE16, 0x3692, 0xD9F0,// P_R_P1Q4 - [0:00:08.130]
      CAM_REG_WRITE16, 0x3694, 0x744B,// P_B_P1Q0 - [0:00:08.133]
      CAM_REG_WRITE16, 0x3696, 0xDA2B,// P_B_P1Q1 - [0:00:08.136]
      CAM_REG_WRITE16, 0x3698, 0x44ED,// P_B_P1Q2 - [0:00:08.139]
      CAM_REG_WRITE16, 0x369A, 0xEE0C,// P_B_P1Q3 - [0:00:08.142]
      CAM_REG_WRITE16, 0x369C, 0xA390,// P_B_P1Q4 - [0:00:08.145]
      CAM_REG_WRITE16, 0x369E, 0x222B,// P_G2_P1Q0 - [0:00:08.148]
      CAM_REG_WRITE16, 0x36A0, 0x154C,// P_G2_P1Q1 - [0:00:08.151]
      CAM_REG_WRITE16, 0x36A2, 0x502C,// P_G2_P1Q2 - [0:00:08.154]
      CAM_REG_WRITE16, 0x36A4, 0xBA8C,// P_G2_P1Q3 - [0:00:08.157]
      CAM_REG_WRITE16, 0x36A6, 0xA2F0,// P_G2_P1Q4 - [0:00:08.160]
      CAM_REG_WRITE16, 0x36C0, 0x76D0,// P_G1_P2Q0 - [0:00:08.163]
      CAM_REG_WRITE16, 0x36C2, 0x4B4B,// P_G1_P2Q1 - [0:00:08.166]
      CAM_REG_WRITE16, 0x36C4, 0xB852,// P_G1_P2Q2 - [0:00:08.169]
      CAM_REG_WRITE16, 0x36C6, 0x110B,// P_G1_P2Q3 - [0:00:08.172]
      CAM_REG_WRITE16, 0x36C8, 0x5054,// P_G1_P2Q4 - [0:00:08.175]
      CAM_REG_WRITE16, 0x36CA, 0x7BD0,// P_R_P2Q0 - [0:00:08.178]
      CAM_REG_WRITE16, 0x36CC, 0x0CEE, // P_R_P2Q1 - [0:00:08.181]
      CAM_REG_WRITE16, 0x36CE, 0xB6B1, // P_R_P2Q2 - [0:00:08.184]
      CAM_REG_WRITE16, 0x36D0, 0x7A6E, // P_R_P2Q3 - [0:00:08.187]
      CAM_REG_WRITE16, 0x36D2, 0x1194, // P_R_P2Q4 - [0:00:08.190]
      CAM_REG_WRITE16, 0x36D4, 0x0C50, // P_B_P2Q0 - [0:00:08.193]
      CAM_REG_WRITE16, 0x36D6, 0x090C, // P_B_P2Q1 - [0:00:08.196]
      CAM_REG_WRITE16, 0x36D8, 0xA911, // P_B_P2Q2 - [0:00:08.199]
      CAM_REG_WRITE16, 0x36DA, 0x1710, // P_B_P2Q3 - [0:00:08.202]
      CAM_REG_WRITE16, 0x36DC, 0x1A14, // P_B_P2Q4 - [0:00:08.205]
      CAM_REG_WRITE16, 0x36DE, 0x76B0, // P_G2_P2Q0 - [0:00:08.208]
      CAM_REG_WRITE16, 0x36E0, 0x27AD, // P_G2_P2Q1 - [0:00:08.211]
      CAM_REG_WRITE16, 0x36E2, 0xC1B2, // P_G2_P2Q2 - [0:00:08.214]
      CAM_REG_WRITE16, 0x36E4, 0xF1CE, // P_G2_P2Q3 - [0:00:08.217]
      CAM_REG_WRITE16, 0x36E6, 0x5354, // P_G2_P2Q4 - [0:00:08.220]
      CAM_REG_WRITE16, 0x3700, 0x31AC, // P_G1_P3Q0 - [0:00:08.223]
      CAM_REG_WRITE16, 0x3702, 0x654B, // P_G1_P3Q1 - [0:00:08.226]
      CAM_REG_WRITE16, 0x3704, 0xAC92, // P_G1_P3Q2 - [0:00:08.229]
      CAM_REG_WRITE16, 0x3706, 0xFBCD, // P_G1_P3Q3 - [0:00:08.232]
      CAM_REG_WRITE16, 0x3708, 0x0AB4, // P_G1_P3Q4 - [0:00:08.235]
      CAM_REG_WRITE16, 0x370A, 0x066E, // P_R_P3Q0 - [0:00:08.238]
      CAM_REG_WRITE16, 0x370C, 0x00AE, // P_R_P3Q1 - [0:00:08.241]
      CAM_REG_WRITE16, 0x370E, 0x8A12, // P_R_P3Q2 - [0:00:08.244]
      CAM_REG_WRITE16, 0x3710, 0xF12E, // P_R_P3Q3 - [0:00:08.247]
      CAM_REG_WRITE16, 0x3712, 0x79F3, // P_R_P3Q4 - [0:00:08.250]
      CAM_REG_WRITE16, 0x3714, 0x39EE, // P_B_P3Q0 - [0:00:08.253]
      CAM_REG_WRITE16, 0x3716, 0x7D0E, // P_B_P3Q1 - [0:00:08.256]
      CAM_REG_WRITE16, 0x3718, 0x8492, // P_B_P3Q2 - [0:00:08.259]
      CAM_REG_WRITE16, 0x371A, 0x75AD, // P_B_P3Q3 - [0:00:08.262]
      CAM_REG_WRITE16, 0x371C, 0x58F3, // P_B_P3Q4 - [0:00:08.265]
      CAM_REG_WRITE16, 0x371E, 0x47ED, // P_G2_P3Q0 - [0:00:08.268]
      CAM_REG_WRITE16, 0x3720, 0xA7CA, // P_G2_P3Q1 - [0:00:08.271]
      CAM_REG_WRITE16, 0x3722, 0x9BF2, // P_G2_P3Q2 - [0:00:08.274]
      CAM_REG_WRITE16, 0x3724, 0x5FAC, // P_G2_P3Q3 - [0:00:08.277]
      CAM_REG_WRITE16, 0x3726, 0x0A94, // P_G2_P3Q4 - [0:00:08.280]
      CAM_REG_WRITE16, 0x3740, 0x97B0, // P_G1_P4Q0 - [0:00:08.283]
      CAM_REG_WRITE16, 0x3742, 0x0BF0, // P_G1_P4Q1 - [0:00:08.286]
      CAM_REG_WRITE16, 0x3744, 0x3995, // P_G1_P4Q2 - [0:00:08.289]
      CAM_REG_WRITE16, 0x3746, 0xB5F0, // P_G1_P4Q3 - [0:00:08.292]
      CAM_REG_WRITE16, 0x3748, 0x94D7, // P_G1_P4Q4 - [0:00:08.295]
      CAM_REG_WRITE16, 0x374A, 0xAD2F, // P_R_P4Q0 - [0:00:08.298]
      CAM_REG_WRITE16, 0x374C, 0xC94A, // P_R_P4Q1 - [0:00:08.301]
      CAM_REG_WRITE16, 0x374E, 0x0955, // P_R_P4Q2 - [0:00:08.304]
      CAM_REG_WRITE16, 0x3750, 0xA8B1, // P_R_P4Q3 - [0:00:08.307]
      CAM_REG_WRITE16, 0x3752, 0xFEF6, // P_R_P4Q4 - [0:00:08.310]
      CAM_REG_WRITE16, 0x3754, 0xDCCE, // P_B_P4Q0 - [0:00:08.313]
      CAM_REG_WRITE16, 0x3756, 0x3E0E, // P_B_P4Q1 - [0:00:08.316]
      CAM_REG_WRITE16, 0x3758, 0x09D5, // P_B_P4Q2 - [0:00:08.319]
      CAM_REG_WRITE16, 0x375A, 0x99F2, // P_B_P4Q3 - [0:00:08.322]
      CAM_REG_WRITE16, 0x375C, 0xE816, // P_B_P4Q4 - [0:00:08.325]
      CAM_REG_WRITE16, 0x375E, 0x9310, // P_G2_P4Q0 - [0:00:08.328]
      CAM_REG_WRITE16, 0x3760, 0x6E2F, // P_G2_P4Q1 - [0:00:08.331]
      CAM_REG_WRITE16, 0x3762, 0x3CB5, // P_G2_P4Q2 - [0:00:08.334]
      CAM_REG_WRITE16, 0x3764, 0xC08E, // P_G2_P4Q3 - [0:00:08.337]
      CAM_REG_WRITE16, 0x3766, 0x9737, // P_G2_P4Q4 - [0:00:08.340]
      CAM_REG_WRITE16, 0x3784, 0x0286, // CENTER_COLUMN - [0:00:08.343]
      CAM_REG_WRITE16, 0x3782, 0x01CE, // CENTER_ROW - [0:00:08.346]
      CAM_REG_WRITE16, 0x37C0, 0xA7CA, // P_GR_Q5 - [0:00:08.349]
      CAM_REG_WRITE16, 0x37C2, 0x914A, // P_RD_Q5 - [0:00:08.352]
      CAM_REG_WRITE16, 0x37C4, 0x8C2B, // P_BL_Q5 - [0:00:08.355]
      CAM_REG_WRITE16, 0x37C6, 0xC2AA, // P_GB_Q5 - [0:00:08.358]
      CAM_REG_WRITE16, 0x098E, 0x0000, // LOGICAL_ADDRESS_ACCESS - [0:00:08.361]
      CAM_REG_WRITE16, 0xC960, 0x09C4, // CAM_PGA_L_CONFIG_COLOUR_TEMP - [0:00:08.366]
      CAM_REG_WRITE16, 0xC962, 0x75F8, // CAM_PGA_L_CONFIG_GREEN_RED_Q14 - [0:00:08.371]
      CAM_REG_WRITE16, 0xC964, 0x5000, // CAM_PGA_L_CONFIG_RED_Q14 - [0:00:08.376]
      CAM_REG_WRITE16, 0xC966, 0x7562, // CAM_PGA_L_CONFIG_GREEN_BLUE_Q14 - [0:00:08.381]
      CAM_REG_WRITE16, 0xC968, 0x6FD0, // CAM_PGA_L_CONFIG_BLUE_Q14 - [0:00:08.386]
      CAM_REG_WRITE16, 0xC96A, 0x0FA0, // CAM_PGA_M_CONFIG_COLOUR_TEMP - [0:00:08.391]
      CAM_REG_WRITE16, 0xC96C, 0x7EB1, // CAM_PGA_M_CONFIG_GREEN_RED_Q14 - [0:00:08.396]
      CAM_REG_WRITE16, 0xC96E, 0x7EDE, // CAM_PGA_M_CONFIG_RED_Q14 - [0:00:08.401]
      CAM_REG_WRITE16, 0xC970, 0x7E7B, // CAM_PGA_M_CONFIG_GREEN_BLUE_Q14 - [0:00:08.406]
      CAM_REG_WRITE16, 0xC972, 0x7DD0, // CAM_PGA_M_CONFIG_BLUE_Q14 - [0:00:08.411]
      CAM_REG_WRITE16, 0xC974, 0x1964, // CAM_PGA_R_CONFIG_COLOUR_TEMP - [0:00:08.416]
      CAM_REG_WRITE16, 0xC976, 0x7C07, // CAM_PGA_R_CONFIG_GREEN_RED_Q14 - [0:00:08.421]
      CAM_REG_WRITE16, 0xC978, 0x6F74, // CAM_PGA_R_CONFIG_RED_Q14 - [0:00:08.426]
      CAM_REG_WRITE16, 0xC97A, 0x7E06, // CAM_PGA_R_CONFIG_GREEN_BLUE_Q14 - [0:00:08.431]
      CAM_REG_WRITE16, 0xC97C, 0x7798, // CAM_PGA_R_CONFIG_BLUE_Q14 - [0:00:08.436]
      CAM_REG_WRITE16, 0xC95E, 0x0003, // CAM_PGA_PGA_CONTROL - [0:00:08.441]
#else
      CAM_REG_WRITE16, 0x3640, 0x02D0,  //  P_G1_P0Q0
      CAM_REG_WRITE16, 0x3642, 0x30E8,  //  P_G1_P0Q1
      CAM_REG_WRITE16, 0x3644, 0x0A91,  //  P_G1_P0Q2
      CAM_REG_WRITE16, 0x3646, 0xB06B,  //  P_G1_P0Q3
      CAM_REG_WRITE16, 0x3648, 0x0271,  //  P_G1_P0Q4
      CAM_REG_WRITE16, 0x364A, 0x00B0,  //  P_R_P0Q0
      CAM_REG_WRITE16, 0x364C, 0xEF4A,  //  P_R_P0Q1
      CAM_REG_WRITE16, 0x364E, 0x3651,  //  P_R_P0Q2
      CAM_REG_WRITE16, 0x3650, 0xE48B,  //  P_R_P0Q3
      CAM_REG_WRITE16, 0x3652, 0x1331,  //  P_R_P0Q4
      CAM_REG_WRITE16, 0x3654, 0x0150,  //  P_B_P0Q0
      CAM_REG_WRITE16, 0x3656, 0x202C,  //  P_B_P0Q1
      CAM_REG_WRITE16, 0x3658, 0x02F1,  //  P_B_P0Q2
      CAM_REG_WRITE16, 0x365A, 0x904E,  //  P_B_P0Q3
      CAM_REG_WRITE16, 0x365C, 0x7310,  //  P_B_P0Q4
      CAM_REG_WRITE16, 0x365E, 0x00F0,  //  P_G2_P0Q0
      CAM_REG_WRITE16, 0x3660, 0x966A,  //  P_G2_P0Q1
      CAM_REG_WRITE16, 0x3662, 0x1691,  //  P_G2_P0Q2
      CAM_REG_WRITE16, 0x3664, 0x0FEB,  //  P_G2_P0Q3
      CAM_REG_WRITE16, 0x3666, 0x3DB0,  //  P_G2_P0Q4
      CAM_REG_WRITE16, 0x3680, 0x14CB,  //  P_G1_P1Q0
      CAM_REG_WRITE16, 0x3682, 0xB90D,  //  P_G1_P1Q1
      CAM_REG_WRITE16, 0x3684, 0xE8CF,  //  P_G1_P1Q2
      CAM_REG_WRITE16, 0x3686, 0x0A2F,  //  P_G1_P1Q3
      CAM_REG_WRITE16, 0x3688, 0x7090,  //  P_G1_P1Q4
      CAM_REG_WRITE16, 0x368A, 0x0D2B,  //  P_R_P1Q0
      CAM_REG_WRITE16, 0x368C, 0xE7CC,  //  P_R_P1Q1
      CAM_REG_WRITE16, 0x368E, 0xEC2F,  //  P_R_P1Q2
      CAM_REG_WRITE16, 0x3690, 0x01CF,  //  P_R_P1Q3
      CAM_REG_WRITE16, 0x3692, 0x4B10,  //  P_R_P1Q4
      CAM_REG_WRITE16, 0x3694, 0x590C,  //  P_B_P1Q0
      CAM_REG_WRITE16, 0x3696, 0xDA6C,  //  P_B_P1Q1
      CAM_REG_WRITE16, 0x3698, 0x8470,  //  P_B_P1Q2
      CAM_REG_WRITE16, 0x369A, 0x144B,  //  P_B_P1Q3
      CAM_REG_WRITE16, 0x369C, 0x03D0,  //  P_B_P1Q4
      CAM_REG_WRITE16, 0x369E, 0x4DEC,  //  P_G2_P1Q0
      CAM_REG_WRITE16, 0x36A0, 0xA00D,  //  P_G2_P1Q1
      CAM_REG_WRITE16, 0x36A2, 0xB030,  //  P_G2_P1Q2
      CAM_REG_WRITE16, 0x36A4, 0x262E,  //  P_G2_P1Q3
      CAM_REG_WRITE16, 0x36A6, 0x1A70,  //  P_G2_P1Q4
      CAM_REG_WRITE16, 0x36C0, 0x2A31,  //  P_G1_P2Q0
      CAM_REG_WRITE16, 0x36C2, 0x42C8,  //  P_G1_P2Q1
      CAM_REG_WRITE16, 0x36C4, 0x47B2,  //  P_G1_P2Q2
      CAM_REG_WRITE16, 0x36C6, 0x3011,  //  P_G1_P2Q3
      CAM_REG_WRITE16, 0x36C8, 0xEF12,  //  P_G1_P2Q4
      CAM_REG_WRITE16, 0x36CA, 0x4611,  //  P_R_P2Q0
      CAM_REG_WRITE16, 0x36CC, 0x7B4D,  //  P_R_P2Q1
      CAM_REG_WRITE16, 0x36CE, 0x2973,  //  P_R_P2Q2
      CAM_REG_WRITE16, 0x36D0, 0x73F0,  //  P_R_P2Q3
      CAM_REG_WRITE16, 0x36D2, 0xF1B3,  //  P_R_P2Q4
      CAM_REG_WRITE16, 0x36D4, 0x1951,  //  P_B_P2Q0
      CAM_REG_WRITE16, 0x36D6, 0x1C4E,  //  P_B_P2Q1
      CAM_REG_WRITE16, 0x36D8, 0x51B2,  //  P_B_P2Q2
      CAM_REG_WRITE16, 0x36DA, 0x4250,  //  P_B_P2Q3
      CAM_REG_WRITE16, 0x36DC, 0x93B3,  //  P_B_P2Q4
      CAM_REG_WRITE16, 0x36DE, 0x2711,  //  P_G2_P2Q0
      CAM_REG_WRITE16, 0x36E0, 0x366C,  //  P_G2_P2Q1
      CAM_REG_WRITE16, 0x36E2, 0x2B12,  //  P_G2_P2Q2
      CAM_REG_WRITE16, 0x36E4, 0x1111,  //  P_G2_P2Q3
      CAM_REG_WRITE16, 0x36E6, 0xCD12,  //  P_G2_P2Q4
      CAM_REG_WRITE16, 0x3700, 0xA4EF,  //  P_G1_P3Q0
      CAM_REG_WRITE16, 0x3702, 0xA16E,  //  P_G1_P3Q1
      CAM_REG_WRITE16, 0x3704, 0x1C8C,  //  P_G1_P3Q2
      CAM_REG_WRITE16, 0x3706, 0x03EE,  //  P_G1_P3Q3
      CAM_REG_WRITE16, 0x3708, 0xC472,  //  P_G1_P3Q4
      CAM_REG_WRITE16, 0x370A, 0xFBEE,  //  P_R_P3Q0
      CAM_REG_WRITE16, 0x370C, 0xA62F,  //  P_R_P3Q1
      CAM_REG_WRITE16, 0x370E, 0x82D1,  //  P_R_P3Q2
      CAM_REG_WRITE16, 0x3710, 0x06CE,  //  P_R_P3Q3
      CAM_REG_WRITE16, 0x3712, 0xE32F,  //  P_R_P3Q4
      CAM_REG_WRITE16, 0x3714, 0xC42B,  //  P_B_P3Q0
      CAM_REG_WRITE16, 0x3716, 0x8D2E,  //  P_B_P3Q1
      CAM_REG_WRITE16, 0x3718, 0x8891,  //  P_B_P3Q2
      CAM_REG_WRITE16, 0x371A, 0x5DAF,  //  P_B_P3Q3
      CAM_REG_WRITE16, 0x371C, 0x76EE,  //  P_B_P3Q4
      CAM_REG_WRITE16, 0x371E, 0x82EF,  //  P_G2_P3Q0
      CAM_REG_WRITE16, 0x3720, 0x45ED,  //  P_G2_P3Q1
      CAM_REG_WRITE16, 0x3722, 0xABD1,  //  P_G2_P3Q2
      CAM_REG_WRITE16, 0x3724, 0xFFCF,  //  P_G2_P3Q3
      CAM_REG_WRITE16, 0x3726, 0x836F,  //  P_G2_P3Q4
      CAM_REG_WRITE16, 0x3740, 0x7ED0,  //  P_G1_P4Q0
      CAM_REG_WRITE16, 0x3742, 0x2451,  //  P_G1_P4Q1
      CAM_REG_WRITE16, 0x3744, 0xBCCF,  //  P_G1_P4Q2
      CAM_REG_WRITE16, 0x3746, 0xD171,  //  P_G1_P4Q3
      CAM_REG_WRITE16, 0x3748, 0x8934,  //  P_G1_P4Q4
      CAM_REG_WRITE16, 0x374A, 0x52B1,  //  P_R_P4Q0
      CAM_REG_WRITE16, 0x374C, 0x1D90,  //  P_R_P4Q1
      CAM_REG_WRITE16, 0x374E, 0xFCB2,  //  P_R_P4Q2
      CAM_REG_WRITE16, 0x3750, 0x61EF,  //  P_R_P4Q3
      CAM_REG_WRITE16, 0x3752, 0xEE73,  //  P_R_P4Q4
      CAM_REG_WRITE16, 0x3754, 0x27D1,  //  P_B_P4Q0
      CAM_REG_WRITE16, 0x3756, 0x06AE,  //  P_B_P4Q1
      CAM_REG_WRITE16, 0x3758, 0xF011,  //  P_B_P4Q2
      CAM_REG_WRITE16, 0x375A, 0x55EF,  //  P_B_P4Q3
      CAM_REG_WRITE16, 0x375C, 0xF112,  //  P_B_P4Q4
      CAM_REG_WRITE16, 0x375E, 0x0851,  //  P_G2_P4Q0
      CAM_REG_WRITE16, 0x3760, 0x2251,  //  P_G2_P4Q1
      CAM_REG_WRITE16, 0x3762, 0x13B2,  //  P_G2_P4Q2
      CAM_REG_WRITE16, 0x3764, 0xF410,  //  P_G2_P4Q3
      CAM_REG_WRITE16, 0x3766, 0xBDB4,  //  P_G2_P4Q4
      CAM_REG_WRITE16, 0x3784, 0x0286, // CENTER_COLUMN - [0:00:08.343]
      CAM_REG_WRITE16, 0x3782, 0x01CE, // CENTER_ROW - [0:00:08.346]
      CAM_REG_WRITE16, 0x37C0, 0x7CA6,  //  P_GR_Q5
      CAM_REG_WRITE16, 0x37C2, 0x2EA9,  //  P_RD_Q5
      CAM_REG_WRITE16, 0x37C4, 0x44E8,  //  P_BL_Q5
      CAM_REG_WRITE16, 0x37C6, 0x3CC8,  //  P_GB_Q5
      CAM_REG_WRITE16, 0x098E, 0x0000,  //  LOGICAL addressing
      CAM_REG_WRITE16, 0xC960, 0x0BB8,  //  CAM_PGA_L_CONFIG_COLOUR_TEMP
      CAM_REG_WRITE16, 0xC962, 0x7EE1,  //  CAM_PGA_L_CONFIG_GREEN_RED_Q14
      CAM_REG_WRITE16, 0xC964, 0x7FDC,  //  CAM_PGA_L_CONFIG_RED_Q14
      CAM_REG_WRITE16, 0xC966, 0x7EBC,  //  CAM_PGA_L_CONFIG_GREEN_BLUE_Q14
      CAM_REG_WRITE16, 0xC968, 0x7FE1,  //  CAM_PGA_L_CONFIG_BLUE_Q14
      CAM_REG_WRITE16, 0xC96A, 0x109A,  //  CAM_PGA_M_CONFIG_COLOUR_TEMP
      CAM_REG_WRITE16, 0xC96C, 0x8020,  //  CAM_PGA_M_CONFIG_GREEN_RED_Q14
      CAM_REG_WRITE16, 0xC96E, 0x80AF,  //  CAM_PGA_M_CONFIG_RED_Q14
      CAM_REG_WRITE16, 0xC970, 0x805E,  //  CAM_PGA_M_CONFIG_GREEN_BLUE_Q14
      CAM_REG_WRITE16, 0xC972, 0x8062,  //  CAM_PGA_M_CONFIG_BLUE_Q14
      CAM_REG_WRITE16, 0xC974, 0x1D4C,  //  CAM_PGA_R_CONFIG_COLOUR_TEMP
      CAM_REG_WRITE16, 0xC976, 0x82C9,  //  CAM_PGA_R_CONFIG_GREEN_RED_Q14
      CAM_REG_WRITE16, 0xC978, 0x85B7,  //  CAM_PGA_R_CONFIG_RED_Q14
      CAM_REG_WRITE16, 0xC97A, 0x8340,  //  CAM_PGA_R_CONFIG_GREEN_BLUE_Q14
      CAM_REG_WRITE16, 0xC97C, 0x82FB,  //  CAM_PGA_R_CONFIG_BLUE_Q14
      CAM_REG_WRITE16, 0xC95E, 0x0003,  //  CAM_PGA_PGA_CONTROL#endif
#endif
};

const CamCommand camInitData[] =
{
#if 0

    CAM_REG_WRITE16, 0x98E, 0x1000,
    CAM_REG_WRITE8, 0xC97E, 0x01,           //cam_sysctl_pll_enable = 1
    CAM_REG_WRITE16, 0xC980, 0x0230,        //cam_sysctl_pll_divider_m_n = 560
    CAM_REG_WRITE16, 0xC982, 0x0700,        //cam_sysctl_pll_divider_p = 1792
    CAM_DELAY, 5000, 0,                     //Delay for 5ms for PLL Lock
    CAM_REG_WRITE16, 0xC800, 0x0000,        //cam_sensor_cfg_y_addr_start = 0
    CAM_REG_WRITE16, 0xC802, 0x0000,        //cam_sensor_cfg_x_addr_start = 0
    CAM_REG_WRITE16, 0xC804, 0x03CD,        //cam_sensor_cfg_y_addr_end = 973
    CAM_REG_WRITE16, 0xC806, 0x050D,        //cam_sensor_cfg_x_addr_end = 1293
    CAM_REG_WRITE32, 0xC808, 0x206CC80,     //cam_sensor_cfg_pixclk = 34000000
    CAM_REG_WRITE16, 0xC80C, 0x0001,        //cam_sensor_cfg_row_speed = 1
    CAM_REG_WRITE16, 0xC80E, 0x01C3,        //cam_sensor_cfg_fine_integ_time_min = 451
    CAM_REG_WRITE16, 0xC810, 0x03B3,        //cam_sensor_cfg_fine_integ_time_max = 947
    CAM_REG_WRITE16, 0xC812, 0x020E,        //cam_sensor_cfg_frame_length_lines = 526
    CAM_REG_WRITE16, 0xC814, 0x049E,        //cam_sensor_cfg_line_length_pck = 1182
    CAM_REG_WRITE16, 0xC816, 0x00E0,        //cam_sensor_cfg_fine_correction = 224
    CAM_REG_WRITE16, 0xC818, 0x01E3,        //cam_sensor_cfg_cpipe_last_row = 483
    CAM_REG_WRITE16, 0xC826, 0x0020,        //cam_sensor_cfg_reg_0_data = 32
    CAM_REG_WRITE16, 0xC834, 0x0330,        //cam_sensor_control_read_mode = 816
    CAM_REG_WRITE16, 0xC854, 0x0000,        //cam_crop_window_xoffset = 0
    CAM_REG_WRITE16, 0xC856, 0x0000,        //cam_crop_window_yoffset = 0
    CAM_REG_WRITE16, 0xC858, 0x0280,        //cam_crop_window_width = 640
    CAM_REG_WRITE16, 0xC85A, 0x01E0,        //cam_crop_window_height = 480
    CAM_REG_WRITE8, 0xC85C, 0x03,           //cam_crop_cropmode = 3
    CAM_REG_WRITE16, 0xC868, 0x0280,        //cam_output_width = 640
    CAM_REG_WRITE16, 0xC86A, 0x01E0,        //cam_output_height = 480
    CAM_REG_WRITE16, 0xC86C, 0x0E10,        //cam_output_format Processed Bayer
    CAM_REG_WRITE8, 0xC878, 0x00,           //cam_aet_aemode = 0
    //CAM_REG_WRITE16, 0xC88C, 0x1B58,      //cam_aet_max_frame_rate = 7000
    //CAM_REG_WRITE16, 0xC88E, 0x1B58,      //cam_aet_min_frame_rate = 7000
    CAM_REG_WRITE16, 0xC88C, 0x36b3,        //cam_aet_max_frame_rate = 7000
    CAM_REG_WRITE16, 0xC88E, 0x36b3,        //cam_aet_min_frame_rate = 7000
    CAM_REG_WRITE16, 0xC914, 0x0000,        //cam_stat_awb_clip_window_xstart = 0
    CAM_REG_WRITE16, 0xC916, 0x0000,        //cam_stat_awb_clip_window_ystart = 0
    CAM_REG_WRITE16, 0xC918, 0x027F,        //cam_stat_awb_clip_window_xend = 639
    CAM_REG_WRITE16, 0xC91A, 0x01DF,        //cam_stat_awb_clip_window_yend = 479
    CAM_REG_WRITE16, 0xC91C, 0x0000,        //cam_stat_ae_initial_window_xstart = 0
    CAM_REG_WRITE16, 0xC91E, 0x0000,        //cam_stat_ae_initial_window_ystart = 0
    CAM_REG_WRITE16, 0xC920, 0x007F,        //cam_stat_ae_initial_window_xend = 127
    CAM_REG_WRITE16, 0xC922, 0x005F,        //cam_stat_ae_initial_window_yend = 95
    CAM_REG_WRITE16, 0x001E, 0x0755,        // set slew rate on pixclk, sync signals, dout  
#else
// [Timing_settings]
    CAM_REG_WRITE16, 0x098E, 0x1000, // LOGICAL_ADDRESS_ACCESS [CAM_SYSCTL_PLL_ENABLE]
    CAM_REG_WRITE8, 0xC97E, 0x01,// CAM_SYSCTL_PLL_ENABLE - [0:00:07.515]
    CAM_REG_WRITE16, 0xC980, 0x0230,// CAM_SYSCTL_PLL_DIVIDER_M_N - [0:00:07.520]
    CAM_REG_WRITE16, 0xC982, 0x0700,// CAM_SYSCTL_PLL_DIVIDER_P - [0:00:07.525]
    CAM_DELAY, 5000, 0,                     //Delay for 5ms for PLL Lock

#if 1
    CAM_SET_RESOLUTION, CAM_RES2_WIDTH, CAM_RES2_HEIGHT, 
#endif
#if 0
      CAM_REG_WRITE16, 0xC800, 0x0000,// CAM_SENSOR_CFG_Y_ADDR_START - [0:00:07.530]
      CAM_REG_WRITE16, 0xC802, 0x0000,// CAM_SENSOR_CFG_X_ADDR_START - [0:00:07.535]
      CAM_REG_WRITE16, 0xC804, 0x03CD,// CAM_SENSOR_CFG_Y_ADDR_END - [0:00:07.540]
      CAM_REG_WRITE16, 0xC806, 0x050D,// CAM_SENSOR_CFG_X_ADDR_END - [0:00:07.545]
      CAM_REG_WRITE32, 0xC808, 0x0206CC80,// CAM_SENSOR_CFG_PIXCLK - [0:00:07.550]
      CAM_REG_WRITE16, 0xC80C, 0x0001,// CAM_SENSOR_CFG_ROW_SPEED - [0:00:07.555]
      CAM_REG_WRITE16, 0xC80E, 0x01C3,// CAM_SENSOR_CFG_FINE_INTEG_TIME_MIN - [0:00:07.560]
      CAM_REG_WRITE16, 0xC810, 0x03B3,// CAM_SENSOR_CFG_FINE_INTEG_TIME_MAX - [0:00:07.565]
      CAM_REG_WRITE16, 0xC812, 0x020E,// CAM_SENSOR_CFG_FRAME_LENGTH_LINES - [0:00:07.570]
      CAM_REG_WRITE16, 0xC814, 0x049E,// CAM_SENSOR_CFG_LINE_LENGTH_PCK - [0:00:07.575]
      CAM_REG_WRITE16, 0xC816, 0x00E0,// CAM_SENSOR_CFG_FINE_CORRECTION - [0:00:07.580]
      CAM_REG_WRITE16, 0xC818, 0x01E3,// CAM_SENSOR_CFG_CPIPE_LAST_ROW - [0:00:07.585]
      CAM_REG_WRITE16, 0xC826, 0x0020,// CAM_SENSOR_CFG_REG_0_DATA - [0:00:07.590]
      CAM_REG_WRITE16, 0xC834, 0x0330,// CAM_SENSOR_CONTROL_READ_MODE - [0:00:07.595]
      CAM_REG_WRITE16, 0xC854, 0x0000,// CAM_CROP_WINDOW_XOFFSET - [0:00:07.600]
      CAM_REG_WRITE16, 0xC856, 0x0000,// CAM_CROP_WINDOW_YOFFSET - [0:00:07.605]
      CAM_REG_WRITE16, 0xC858, 0x0280,// CAM_CROP_WINDOW_WIDTH - [0:00:07.610]
      CAM_REG_WRITE16, 0xC85A, 0x01E0,// CAM_CROP_WINDOW_HEIGHT - [0:00:07.615]
      CAM_REG_WRITE8, 0xC85C, 0x03,// CAM_CROP_CROPMODE - [0:00:07.620]
      CAM_REG_WRITE16, 0xC868, 0x0280,// CAM_OUTPUT_WIDTH - [0:00:07.625]
      CAM_REG_WRITE16, 0xC86A, 0x01E0,// CAM_OUTPUT_HEIGHT - [0:00:07.630]
      CAM_REG_WRITE16, 0xC86C, 0x0E10,// cam_output_format Processed Bayer
      CAM_REG_WRITE8, 0xC878, 0x00,// CAM_AET_AEMODE - [0:00:07.635]
      CAM_REG_WRITE16, 0xC88C, 0x1B58,// CAM_AET_MAX_FRAME_RATE - [0:00:07.640]
      CAM_REG_WRITE16, 0xC88E, 0x1B58,// CAM_AET_MIN_FRAME_RATE - [0:00:07.645]
      CAM_REG_WRITE16, 0xC914, 0x0000,// CAM_STAT_AWB_CLIP_WINDOW_XSTART - [0:00:07.650]
      CAM_REG_WRITE16, 0xC916, 0x0000,// CAM_STAT_AWB_CLIP_WINDOW_YSTART - [0:00:07.655]
      CAM_REG_WRITE16, 0xC918, 0x027F,// CAM_STAT_AWB_CLIP_WINDOW_XEND - [0:00:07.660]
      CAM_REG_WRITE16, 0xC91A, 0x01DF,// CAM_STAT_AWB_CLIP_WINDOW_YEND - [0:00:07.665]
      CAM_REG_WRITE16, 0xC91C, 0x0000,// CAM_STAT_AE_INITIAL_WINDOW_XSTART - [0:00:07.670]
      CAM_REG_WRITE16, 0xC91E, 0x0000,// CAM_STAT_AE_INITIAL_WINDOW_YSTART - [0:00:07.675]
      CAM_REG_WRITE16, 0xC920, 0x007f,// CAM_STAT_AE_INITIAL_WINDOW_XEND - [0:00:07.680]
      CAM_REG_WRITE16, 0xC922, 0x005f,// CAM_STAT_AE_INITIAL_WINDOW_YEND - [0:00:07.685]
// Change Config
      CAM_SEND_COMMAND, 0, 0,
#endif
#if 0 // new config
    CAM_REG_WRITE16, 0xC800, 0x0000,        //cam_sensor_cfg_y_addr_start = 0
    CAM_REG_WRITE16, 0xC802, 0x0000,        //cam_sensor_cfg_x_addr_start = 0
    CAM_REG_WRITE16, 0xC804, 0x03CD,        //cam_sensor_cfg_y_addr_end = 973
    CAM_REG_WRITE16, 0xC806, 0x050D,        //cam_sensor_cfg_x_addr_end = 1293
    CAM_REG_WRITE32, 0xC808, 0x206CC80,     //cam_sensor_cfg_pixclk = 34000000
    CAM_REG_WRITE16, 0xC80C, 0x0001,        //cam_sensor_cfg_row_speed = 1
    CAM_REG_WRITE16, 0xC80E, 0x01C3,        //cam_sensor_cfg_fine_integ_time_min = 451
    CAM_REG_WRITE16, 0xC810, 0x03B3,        //cam_sensor_cfg_fine_integ_time_max = 947
    CAM_REG_WRITE16, 0xC812, 0x020E,        //cam_sensor_cfg_frame_length_lines = 526
    CAM_REG_WRITE16, 0xC814, 0x049E,        //cam_sensor_cfg_line_length_pck = 1182
    CAM_REG_WRITE16, 0xC816, 0x00E0,        //cam_sensor_cfg_fine_correction = 224
    CAM_REG_WRITE16, 0xC818, 0x01E3,        //cam_sensor_cfg_cpipe_last_row = 483
    CAM_REG_WRITE16, 0xC826, 0x0020,        //cam_sensor_cfg_reg_0_data = 32
    CAM_REG_WRITE16, 0xC834, 0x0330,        //cam_sensor_control_read_mode = 816
    CAM_REG_WRITE16, 0xC854, 0x0000,        //cam_crop_window_xoffset = 0
    CAM_REG_WRITE16, 0xC856, 0x0000,        //cam_crop_window_yoffset = 0
    CAM_REG_WRITE16, 0xC858, 0x0280,        //cam_crop_window_width = 640
    CAM_REG_WRITE16, 0xC85A, 0x01E0,        //cam_crop_window_height = 480
    CAM_REG_WRITE8, 0xC85C, 0x03,           //cam_crop_cropmode = 3
    CAM_REG_WRITE16, 0xC868, 0x0280,        //cam_output_width = 640
    CAM_REG_WRITE16, 0xC86A, 0x01E0,        //cam_output_height = 480
    CAM_REG_WRITE16, 0xC86C, 0x0E10,        //cam_output_format Processed Bayer
    CAM_REG_WRITE8, 0xC878, 0x00,           //cam_aet_aemode = 0
    //CAM_REG_WRITE16, 0xC88C, 0x1B58,      //cam_aet_max_frame_rate = 7000
    //CAM_REG_WRITE16, 0xC88E, 0x1B58,      //cam_aet_min_frame_rate = 7000
    CAM_REG_WRITE16, 0xC88C, 0x36b3,        //cam_aet_max_frame_rate = 7000
    CAM_REG_WRITE16, 0xC88E, 0x36b3,        //cam_aet_min_frame_rate = 7000
    CAM_REG_WRITE16, 0xC914, 0x0000,        //cam_stat_awb_clip_window_xstart = 0
    CAM_REG_WRITE16, 0xC916, 0x0000,        //cam_stat_awb_clip_window_ystart = 0
    CAM_REG_WRITE16, 0xC918, 0x027F,        //cam_stat_awb_clip_window_xend = 639
    CAM_REG_WRITE16, 0xC91A, 0x01DF,        //cam_stat_awb_clip_window_yend = 479
    CAM_REG_WRITE16, 0xC91C, 0x0000,        //cam_stat_ae_initial_window_xstart = 0
    CAM_REG_WRITE16, 0xC91E, 0x0000,        //cam_stat_ae_initial_window_ystart = 0
    CAM_REG_WRITE16, 0xC920, 0x007F,        //cam_stat_ae_initial_window_xend = 127
    CAM_REG_WRITE16, 0xC922, 0x005F,        //cam_stat_ae_initial_window_yend = 95
    CAM_REG_WRITE16, 0x001E, 0x0755,        // set slew rate on pixclk, sync signals, dout  
// Change Config
      CAM_SEND_COMMAND, 0, 0,
#endif

#if 1
// [Step3-Recommended]
// [Sensor optimization]
      CAM_REG_WRITE16, 0x316A, 0x8270,// RESERVED_CORE_316A - [0:00:07.769]
      CAM_REG_WRITE16, 0x316C, 0x8270,// RESERVED_CORE_316C - [0:00:07.772]
      CAM_REG_WRITE16, 0x3ED0, 0x2305,// RESERVED_CORE_3ED0 - [0:00:07.776]
      CAM_REG_WRITE16, 0x3ED2, 0x77CF,// RESERVED_CORE_3ED2 - [0:00:07.779]
      CAM_REG_WRITE16, 0x316E, 0x8202,// RESERVED_CORE_316E - [0:00:07.781]
      CAM_REG_WRITE16, 0x3180, 0x87FF,// RESERVED_CORE_3180 - [0:00:07.785]
      CAM_REG_WRITE16, 0x30D4, 0x6080,// RESERVED_CORE_30D4 - [0:00:07.788]
      CAM_REG_WRITE16, 0xA802, 0x0008,// RESERVED_AE_TRACK_02 - [0:00:07.793]

// [Errata item 1]
      CAM_REG_WRITE16, 0x3E14, 0xFF39,// RESERVED_CORE_3E14 - [0:00:07.796]
#endif

#if 1
// [Load Patch 0202]
      CAM_REG_WRITE16, 0x0982, 0x0001,// ACCESS_CTL_STAT - [0:00:07.799]
      CAM_REG_WRITE16, 0x098A, 0x5000,// PHYSICAL_ADDRESS_ACCESS - [0:00:07.801]
      CAM_REG_WRITE16, 0xD000, 0x70CF,
      CAM_REG_WRITE16, 0xD002, 0xFFFF,
      CAM_REG_WRITE16, 0xD004, 0xC5D4,
      CAM_REG_WRITE16, 0xD006, 0x903A,
      CAM_REG_WRITE16, 0xD008, 0x2144,
      CAM_REG_WRITE16, 0xD00A, 0x0C00,
      CAM_REG_WRITE16, 0xD00C, 0x2186,
      CAM_REG_WRITE16, 0xD00E, 0x0FF3,
      CAM_REG_WRITE16, 0xD010, 0xB844,
      CAM_REG_WRITE16, 0xD012, 0xB948,
      CAM_REG_WRITE16, 0xD014, 0xE082,
      CAM_REG_WRITE16, 0xD016, 0x20CC,
      CAM_REG_WRITE16, 0xD018, 0x80E2,
      CAM_REG_WRITE16, 0xD01A, 0x21CC,
      CAM_REG_WRITE16, 0xD01C, 0x80A2,
      CAM_REG_WRITE16, 0xD01E, 0x21CC,
      CAM_REG_WRITE16, 0xD020, 0x80E2,
      CAM_REG_WRITE16, 0xD022, 0xF404,
      CAM_REG_WRITE16, 0xD024, 0xD801,
      CAM_REG_WRITE16, 0xD026, 0xF003,
      CAM_REG_WRITE16, 0xD028, 0xD800,
      CAM_REG_WRITE16, 0xD02A, 0x7EE0,
      CAM_REG_WRITE16, 0xD02C, 0xC0F1,
      CAM_REG_WRITE16, 0xD02E, 0x08BA,  // [0:00:07.804]
      CAM_REG_WRITE16, 0xD030, 0x0600,
      CAM_REG_WRITE16, 0xD032, 0xC1A1,
      CAM_REG_WRITE16, 0xD034, 0x76CF,
      CAM_REG_WRITE16, 0xD036, 0xFFFF,
      CAM_REG_WRITE16, 0xD038, 0xC130,
      CAM_REG_WRITE16, 0xD03A, 0x6E04,
      CAM_REG_WRITE16, 0xD03C, 0xC040,
      CAM_REG_WRITE16, 0xD03E, 0x71CF,
      CAM_REG_WRITE16, 0xD040, 0xFFFF,
      CAM_REG_WRITE16, 0xD042, 0xC790,
      CAM_REG_WRITE16, 0xD044, 0x8103,
      CAM_REG_WRITE16, 0xD046, 0x77CF,
      CAM_REG_WRITE16, 0xD048, 0xFFFF,
      CAM_REG_WRITE16, 0xD04A, 0xC7C0,
      CAM_REG_WRITE16, 0xD04C, 0xE001,
      CAM_REG_WRITE16, 0xD04E, 0xA103,
      CAM_REG_WRITE16, 0xD050, 0xD800,
      CAM_REG_WRITE16, 0xD052, 0x0C6A,
      CAM_REG_WRITE16, 0xD054, 0x04E0,
      CAM_REG_WRITE16, 0xD056, 0xB89E,
      CAM_REG_WRITE16, 0xD058, 0x7508,
      CAM_REG_WRITE16, 0xD05A, 0x8E1C,
      CAM_REG_WRITE16, 0xD05C, 0x0809,
      CAM_REG_WRITE16, 0xD05E, 0x0191,  // [0:00:07.808]
      CAM_REG_WRITE16, 0xD060, 0xD801,
      CAM_REG_WRITE16, 0xD062, 0xAE1D,
      CAM_REG_WRITE16, 0xD064, 0xE580,
      CAM_REG_WRITE16, 0xD066, 0x20CA,
      CAM_REG_WRITE16, 0xD068, 0x0022,
      CAM_REG_WRITE16, 0xD06A, 0x20CF,
      CAM_REG_WRITE16, 0xD06C, 0x0522,
      CAM_REG_WRITE16, 0xD06E, 0x0C5C,
      CAM_REG_WRITE16, 0xD070, 0x04E2,
      CAM_REG_WRITE16, 0xD072, 0x21CA,
      CAM_REG_WRITE16, 0xD074, 0x0062,
      CAM_REG_WRITE16, 0xD076, 0xE580,
      CAM_REG_WRITE16, 0xD078, 0xD901,
      CAM_REG_WRITE16, 0xD07A, 0x79C0,
      CAM_REG_WRITE16, 0xD07C, 0xD800,
      CAM_REG_WRITE16, 0xD07E, 0x0BE6,
      CAM_REG_WRITE16, 0xD080, 0x04E0,
      CAM_REG_WRITE16, 0xD082, 0xB89E,
      CAM_REG_WRITE16, 0xD084, 0x70CF,
      CAM_REG_WRITE16, 0xD086, 0xFFFF,
      CAM_REG_WRITE16, 0xD088, 0xC8D4,
      CAM_REG_WRITE16, 0xD08A, 0x9002,
      CAM_REG_WRITE16, 0xD08C, 0x0857,
      CAM_REG_WRITE16, 0xD08E, 0x025E,  // [0:00:07.811]
      CAM_REG_WRITE16, 0xD090, 0xFFDC,
      CAM_REG_WRITE16, 0xD092, 0xE080,
      CAM_REG_WRITE16, 0xD094, 0x25CC,
      CAM_REG_WRITE16, 0xD096, 0x9022,
      CAM_REG_WRITE16, 0xD098, 0xF225,
      CAM_REG_WRITE16, 0xD09A, 0x1700,
      CAM_REG_WRITE16, 0xD09C, 0x108A,
      CAM_REG_WRITE16, 0xD09E, 0x73CF,
      CAM_REG_WRITE16, 0xD0A0, 0xFF00,
      CAM_REG_WRITE16, 0xD0A2, 0x3174,
      CAM_REG_WRITE16, 0xD0A4, 0x9307,
      CAM_REG_WRITE16, 0xD0A6, 0x2A04,
      CAM_REG_WRITE16, 0xD0A8, 0x103E,
      CAM_REG_WRITE16, 0xD0AA, 0x9328,
      CAM_REG_WRITE16, 0xD0AC, 0x2942,
      CAM_REG_WRITE16, 0xD0AE, 0x7140,
      CAM_REG_WRITE16, 0xD0B0, 0x2A04,
      CAM_REG_WRITE16, 0xD0B2, 0x107E,
      CAM_REG_WRITE16, 0xD0B4, 0x9349,
      CAM_REG_WRITE16, 0xD0B6, 0x2942,
      CAM_REG_WRITE16, 0xD0B8, 0x7141,
      CAM_REG_WRITE16, 0xD0BA, 0x2A04,
      CAM_REG_WRITE16, 0xD0BC, 0x10BE,
      CAM_REG_WRITE16, 0xD0BE, 0x934A,  // [0:00:07.814]
      CAM_REG_WRITE16, 0xD0C0, 0x2942,
      CAM_REG_WRITE16, 0xD0C2, 0x714B,
      CAM_REG_WRITE16, 0xD0C4, 0x2A04,
      CAM_REG_WRITE16, 0xD0C6, 0x10BE,
      CAM_REG_WRITE16, 0xD0C8, 0x130C,
      CAM_REG_WRITE16, 0xD0CA, 0x010A,
      CAM_REG_WRITE16, 0xD0CC, 0x2942,
      CAM_REG_WRITE16, 0xD0CE, 0x7142,
      CAM_REG_WRITE16, 0xD0D0, 0x2250,
      CAM_REG_WRITE16, 0xD0D2, 0x13CA,
      CAM_REG_WRITE16, 0xD0D4, 0x1B0C,
      CAM_REG_WRITE16, 0xD0D6, 0x0284,
      CAM_REG_WRITE16, 0xD0D8, 0xB307,
      CAM_REG_WRITE16, 0xD0DA, 0xB328,
      CAM_REG_WRITE16, 0xD0DC, 0x1B12,
      CAM_REG_WRITE16, 0xD0DE, 0x02C4,
      CAM_REG_WRITE16, 0xD0E0, 0xB34A,
      CAM_REG_WRITE16, 0xD0E2, 0xED88,
      CAM_REG_WRITE16, 0xD0E4, 0x71CF,
      CAM_REG_WRITE16, 0xD0E6, 0xFF00,
      CAM_REG_WRITE16, 0xD0E8, 0x3174,
      CAM_REG_WRITE16, 0xD0EA, 0x9106,
      CAM_REG_WRITE16, 0xD0EC, 0xB88F,
      CAM_REG_WRITE16, 0xD0EE, 0xB106,  // [0:00:07.817]
      CAM_REG_WRITE16, 0xD0F0, 0x210A,
      CAM_REG_WRITE16, 0xD0F2, 0x8340,
      CAM_REG_WRITE16, 0xD0F4, 0xC000,
      CAM_REG_WRITE16, 0xD0F6, 0x21CA,
      CAM_REG_WRITE16, 0xD0F8, 0x0062,
      CAM_REG_WRITE16, 0xD0FA, 0x20F0,
      CAM_REG_WRITE16, 0xD0FC, 0x0040,
      CAM_REG_WRITE16, 0xD0FE, 0x0B02,
      CAM_REG_WRITE16, 0xD100, 0x0320,
      CAM_REG_WRITE16, 0xD102, 0xD901,
      CAM_REG_WRITE16, 0xD104, 0x07F1,
      CAM_REG_WRITE16, 0xD106, 0x05E0,
      CAM_REG_WRITE16, 0xD108, 0xC0A1,
      CAM_REG_WRITE16, 0xD10A, 0x78E0,
      CAM_REG_WRITE16, 0xD10C, 0xC0F1,
      CAM_REG_WRITE16, 0xD10E, 0x71CF,
      CAM_REG_WRITE16, 0xD110, 0xFFFF,
      CAM_REG_WRITE16, 0xD112, 0xC7C0,
      CAM_REG_WRITE16, 0xD114, 0xD840,
      CAM_REG_WRITE16, 0xD116, 0xA900,
      CAM_REG_WRITE16, 0xD118, 0x71CF,
      CAM_REG_WRITE16, 0xD11A, 0xFFFF,
      CAM_REG_WRITE16, 0xD11C, 0xD02C,
      CAM_REG_WRITE16, 0xD11E, 0xD81E,  // [0:00:07.820]
      CAM_REG_WRITE16, 0xD120, 0x0A5A,
      CAM_REG_WRITE16, 0xD122, 0x04E0,
      CAM_REG_WRITE16, 0xD124, 0xDA00,
      CAM_REG_WRITE16, 0xD126, 0xD800,
      CAM_REG_WRITE16, 0xD128, 0xC0D1,
      CAM_REG_WRITE16, 0xD12A, 0x7EE0,  // [0:00:07.822]
      CAM_REG_WRITE16, 0x098E, 0x0000, // LOGICAL_ADDRESS_ACCESS - [0:00:07.825]

// [Apply Patch 0202]
      CAM_REG_WRITE16, 0xE000, 0x010C, // PATCHLDR_LOADER_ADDRESS - [0:00:07.830]
      CAM_REG_WRITE16, 0xE002, 0x0202, // PATCHLDR_PATCH_ID - [0:00:07.835]
      CAM_REG_WRITE32, 0xE004, 0x41030202, // PATCHLDR_FIRMWARE_ID - [0:00:07.840]
      CAM_APPLY_PATCH, 0, 0,

// [Load Patch 0302]
      CAM_REG_WRITE16, 0x0982, 0x0001, // ACCESS_CTL_STAT - [0:00:07.898]
      CAM_REG_WRITE16, 0x098A, 0x512C, // PHYSICAL_ADDRESS_ACCESS - [0:00:07.901]
      CAM_REG_WRITE16, 0xD12C, 0x70CF,
      CAM_REG_WRITE16, 0xD12E, 0xFFFF,
      CAM_REG_WRITE16, 0xD130, 0xC5D4,
      CAM_REG_WRITE16, 0xD132, 0x903A,
      CAM_REG_WRITE16, 0xD134, 0x2144,
      CAM_REG_WRITE16, 0xD136, 0x0C00,
      CAM_REG_WRITE16, 0xD138, 0x2186,
      CAM_REG_WRITE16, 0xD13A, 0x0FF3,
      CAM_REG_WRITE16, 0xD13C, 0xB844,
      CAM_REG_WRITE16, 0xD13E, 0x262F,
      CAM_REG_WRITE16, 0xD140, 0xF008,
      CAM_REG_WRITE16, 0xD142, 0xB948,
      CAM_REG_WRITE16, 0xD144, 0x21CC,
      CAM_REG_WRITE16, 0xD146, 0x8021,
      CAM_REG_WRITE16, 0xD148, 0xD801,
      CAM_REG_WRITE16, 0xD14A, 0xF203,
      CAM_REG_WRITE16, 0xD14C, 0xD800,
      CAM_REG_WRITE16, 0xD14E, 0x7EE0,
      CAM_REG_WRITE16, 0xD150, 0xC0F1,
      CAM_REG_WRITE16, 0xD152, 0x71CF,
      CAM_REG_WRITE16, 0xD154, 0xFFFF,
      CAM_REG_WRITE16, 0xD156, 0xC610,
      CAM_REG_WRITE16, 0xD158, 0x910E,
      CAM_REG_WRITE16, 0xD15A, 0x208C,  // [0:00:07.904]
      CAM_REG_WRITE16, 0xD15C, 0x8014,
      CAM_REG_WRITE16, 0xD15E, 0xF418,
      CAM_REG_WRITE16, 0xD160, 0x910F,
      CAM_REG_WRITE16, 0xD162, 0x208C,
      CAM_REG_WRITE16, 0xD164, 0x800F,
      CAM_REG_WRITE16, 0xD166, 0xF414,
      CAM_REG_WRITE16, 0xD168, 0x9116,
      CAM_REG_WRITE16, 0xD16A, 0x208C,
      CAM_REG_WRITE16, 0xD16C, 0x800A,
      CAM_REG_WRITE16, 0xD16E, 0xF410,
      CAM_REG_WRITE16, 0xD170, 0x9117,
      CAM_REG_WRITE16, 0xD172, 0x208C,
      CAM_REG_WRITE16, 0xD174, 0x8807,
      CAM_REG_WRITE16, 0xD176, 0xF40C,
      CAM_REG_WRITE16, 0xD178, 0x9118,
      CAM_REG_WRITE16, 0xD17A, 0x2086,
      CAM_REG_WRITE16, 0xD17C, 0x0FF3,
      CAM_REG_WRITE16, 0xD17E, 0xB848,
      CAM_REG_WRITE16, 0xD180, 0x080D,
      CAM_REG_WRITE16, 0xD182, 0x0090,
      CAM_REG_WRITE16, 0xD184, 0xFFEA,
      CAM_REG_WRITE16, 0xD186, 0xE081,
      CAM_REG_WRITE16, 0xD188, 0xD801,
      CAM_REG_WRITE16, 0xD18A, 0xF203,  // [0:00:07.907]
      CAM_REG_WRITE16, 0xD18C, 0xD800,
      CAM_REG_WRITE16, 0xD18E, 0xC0D1,
      CAM_REG_WRITE16, 0xD190, 0x7EE0,
      CAM_REG_WRITE16, 0xD192, 0x78E0,
      CAM_REG_WRITE16, 0xD194, 0xC0F1,
      CAM_REG_WRITE16, 0xD196, 0x71CF,
      CAM_REG_WRITE16, 0xD198, 0xFFFF,
      CAM_REG_WRITE16, 0xD19A, 0xC610,
      CAM_REG_WRITE16, 0xD19C, 0x910E,
      CAM_REG_WRITE16, 0xD19E, 0x208C,
      CAM_REG_WRITE16, 0xD1A0, 0x800A,
      CAM_REG_WRITE16, 0xD1A2, 0xF418,
      CAM_REG_WRITE16, 0xD1A4, 0x910F,
      CAM_REG_WRITE16, 0xD1A6, 0x208C,
      CAM_REG_WRITE16, 0xD1A8, 0x8807,
      CAM_REG_WRITE16, 0xD1AA, 0xF414,
      CAM_REG_WRITE16, 0xD1AC, 0x9116,
      CAM_REG_WRITE16, 0xD1AE, 0x208C,
      CAM_REG_WRITE16, 0xD1B0, 0x800A,
      CAM_REG_WRITE16, 0xD1B2, 0xF410,
      CAM_REG_WRITE16, 0xD1B4, 0x9117,
      CAM_REG_WRITE16, 0xD1B6, 0x208C,
      CAM_REG_WRITE16, 0xD1B8, 0x8807,
      CAM_REG_WRITE16, 0xD1BA, 0xF40C,  // [0:00:07.910]
      CAM_REG_WRITE16, 0xD1BC, 0x9118,
      CAM_REG_WRITE16, 0xD1BE, 0x2086,
      CAM_REG_WRITE16, 0xD1C0, 0x0FF3,
      CAM_REG_WRITE16, 0xD1C2, 0xB848,
      CAM_REG_WRITE16, 0xD1C4, 0x080D,
      CAM_REG_WRITE16, 0xD1C6, 0x0090,
      CAM_REG_WRITE16, 0xD1C8, 0xFFD9,
      CAM_REG_WRITE16, 0xD1CA, 0xE080,
      CAM_REG_WRITE16, 0xD1CC, 0xD801,
      CAM_REG_WRITE16, 0xD1CE, 0xF203,
      CAM_REG_WRITE16, 0xD1D0, 0xD800,
      CAM_REG_WRITE16, 0xD1D2, 0xF1DF,
      CAM_REG_WRITE16, 0xD1D4, 0x9040,
      CAM_REG_WRITE16, 0xD1D6, 0x71CF,
      CAM_REG_WRITE16, 0xD1D8, 0xFFFF,
      CAM_REG_WRITE16, 0xD1DA, 0xC5D4,
      CAM_REG_WRITE16, 0xD1DC, 0xB15A,
      CAM_REG_WRITE16, 0xD1DE, 0x9041,
      CAM_REG_WRITE16, 0xD1E0, 0x73CF,
      CAM_REG_WRITE16, 0xD1E2, 0xFFFF,
      CAM_REG_WRITE16, 0xD1E4, 0xC7D0,
      CAM_REG_WRITE16, 0xD1E6, 0xB140,
      CAM_REG_WRITE16, 0xD1E8, 0x9042,
      CAM_REG_WRITE16, 0xD1EA, 0xB141,  // [0:00:07.913]
      CAM_REG_WRITE16, 0xD1EC, 0x9043,
      CAM_REG_WRITE16, 0xD1EE, 0xB142,
      CAM_REG_WRITE16, 0xD1F0, 0x9044,
      CAM_REG_WRITE16, 0xD1F2, 0xB143,
      CAM_REG_WRITE16, 0xD1F4, 0x9045,
      CAM_REG_WRITE16, 0xD1F6, 0xB147,
      CAM_REG_WRITE16, 0xD1F8, 0x9046,
      CAM_REG_WRITE16, 0xD1FA, 0xB148,
      CAM_REG_WRITE16, 0xD1FC, 0x9047,
      CAM_REG_WRITE16, 0xD1FE, 0xB14B,
      CAM_REG_WRITE16, 0xD200, 0x9048,
      CAM_REG_WRITE16, 0xD202, 0xB14C,
      CAM_REG_WRITE16, 0xD204, 0x9049,
      CAM_REG_WRITE16, 0xD206, 0x1958,
      CAM_REG_WRITE16, 0xD208, 0x0084,
      CAM_REG_WRITE16, 0xD20A, 0x904A,
      CAM_REG_WRITE16, 0xD20C, 0x195A,
      CAM_REG_WRITE16, 0xD20E, 0x0084,
      CAM_REG_WRITE16, 0xD210, 0x8856,
      CAM_REG_WRITE16, 0xD212, 0x1B36,
      CAM_REG_WRITE16, 0xD214, 0x8082,
      CAM_REG_WRITE16, 0xD216, 0x8857,
      CAM_REG_WRITE16, 0xD218, 0x1B37,
      CAM_REG_WRITE16, 0xD21A, 0x8082,  // [0:00:07.916]
      CAM_REG_WRITE16, 0xD21C, 0x904C,
      CAM_REG_WRITE16, 0xD21E, 0x19A7,
      CAM_REG_WRITE16, 0xD220, 0x009C,
      CAM_REG_WRITE16, 0xD222, 0x881A,
      CAM_REG_WRITE16, 0xD224, 0x7FE0,
      CAM_REG_WRITE16, 0xD226, 0x1B54,
      CAM_REG_WRITE16, 0xD228, 0x8002,
      CAM_REG_WRITE16, 0xD22A, 0x78E0,
      CAM_REG_WRITE16, 0xD22C, 0x71CF,
      CAM_REG_WRITE16, 0xD22E, 0xFFFF,
      CAM_REG_WRITE16, 0xD230, 0xC350,
      CAM_REG_WRITE16, 0xD232, 0xD828,
      CAM_REG_WRITE16, 0xD234, 0xA90B,
      CAM_REG_WRITE16, 0xD236, 0x8100,
      CAM_REG_WRITE16, 0xD238, 0x01C5,
      CAM_REG_WRITE16, 0xD23A, 0x0320,
      CAM_REG_WRITE16, 0xD23C, 0xD900,
      CAM_REG_WRITE16, 0xD23E, 0x78E0,
      CAM_REG_WRITE16, 0xD240, 0x220A,
      CAM_REG_WRITE16, 0xD242, 0x1F80,
      CAM_REG_WRITE16, 0xD244, 0xFFFF,
      CAM_REG_WRITE16, 0xD246, 0xD4E0,
      CAM_REG_WRITE16, 0xD248, 0xC0F1,
      CAM_REG_WRITE16, 0xD24A, 0x0811,  // [0:00:07.919]
      CAM_REG_WRITE16, 0xD24C, 0x0051,
      CAM_REG_WRITE16, 0xD24E, 0x2240,
      CAM_REG_WRITE16, 0xD250, 0x1200,
      CAM_REG_WRITE16, 0xD252, 0xFFE1,
      CAM_REG_WRITE16, 0xD254, 0xD801,
      CAM_REG_WRITE16, 0xD256, 0xF006,
      CAM_REG_WRITE16, 0xD258, 0x2240,
      CAM_REG_WRITE16, 0xD25A, 0x1900,
      CAM_REG_WRITE16, 0xD25C, 0xFFDE,
      CAM_REG_WRITE16, 0xD25E, 0xD802,
      CAM_REG_WRITE16, 0xD260, 0x1A05,
      CAM_REG_WRITE16, 0xD262, 0x1002,
      CAM_REG_WRITE16, 0xD264, 0xFFF2,
      CAM_REG_WRITE16, 0xD266, 0xF195,
      CAM_REG_WRITE16, 0xD268, 0xC0F1,
      CAM_REG_WRITE16, 0xD26A, 0x0E7E,
      CAM_REG_WRITE16, 0xD26C, 0x05C0,
      CAM_REG_WRITE16, 0xD26E, 0x75CF,
      CAM_REG_WRITE16, 0xD270, 0xFFFF,
      CAM_REG_WRITE16, 0xD272, 0xC84C,
      CAM_REG_WRITE16, 0xD274, 0x9502,
      CAM_REG_WRITE16, 0xD276, 0x77CF,
      CAM_REG_WRITE16, 0xD278, 0xFFFF,
      CAM_REG_WRITE16, 0xD27A, 0xC344,  // [0:00:07.922]
      CAM_REG_WRITE16, 0xD27C, 0x2044,
      CAM_REG_WRITE16, 0xD27E, 0x008E,
      CAM_REG_WRITE16, 0xD280, 0xB8A1,
      CAM_REG_WRITE16, 0xD282, 0x0926,
      CAM_REG_WRITE16, 0xD284, 0x03E0,
      CAM_REG_WRITE16, 0xD286, 0xB502,
      CAM_REG_WRITE16, 0xD288, 0x9502,
      CAM_REG_WRITE16, 0xD28A, 0x952E,
      CAM_REG_WRITE16, 0xD28C, 0x7E05,
      CAM_REG_WRITE16, 0xD28E, 0xB5C2,
      CAM_REG_WRITE16, 0xD290, 0x70CF,
      CAM_REG_WRITE16, 0xD292, 0xFFFF,
      CAM_REG_WRITE16, 0xD294, 0xC610,
      CAM_REG_WRITE16, 0xD296, 0x099A,
      CAM_REG_WRITE16, 0xD298, 0x04A0,
      CAM_REG_WRITE16, 0xD29A, 0xB026,
      CAM_REG_WRITE16, 0xD29C, 0x0E02,
      CAM_REG_WRITE16, 0xD29E, 0x0560,
      CAM_REG_WRITE16, 0xD2A0, 0xDE00,
      CAM_REG_WRITE16, 0xD2A2, 0x0A12,
      CAM_REG_WRITE16, 0xD2A4, 0x0320,
      CAM_REG_WRITE16, 0xD2A6, 0xB7C4,
      CAM_REG_WRITE16, 0xD2A8, 0x0B36,
      CAM_REG_WRITE16, 0xD2AA, 0x03A0,  // [0:00:07.925]
      CAM_REG_WRITE16, 0xD2AC, 0x70C9,
      CAM_REG_WRITE16, 0xD2AE, 0x9502,
      CAM_REG_WRITE16, 0xD2B0, 0x7608,
      CAM_REG_WRITE16, 0xD2B2, 0xB8A8,
      CAM_REG_WRITE16, 0xD2B4, 0xB502,
      CAM_REG_WRITE16, 0xD2B6, 0x70CF,
      CAM_REG_WRITE16, 0xD2B8, 0x0000,
      CAM_REG_WRITE16, 0xD2BA, 0x5536,
      CAM_REG_WRITE16, 0xD2BC, 0x7860,
      CAM_REG_WRITE16, 0xD2BE, 0x2686,
      CAM_REG_WRITE16, 0xD2C0, 0x1FFB,
      CAM_REG_WRITE16, 0xD2C2, 0x9502,
      CAM_REG_WRITE16, 0xD2C4, 0x78C5,
      CAM_REG_WRITE16, 0xD2C6, 0x0631,
      CAM_REG_WRITE16, 0xD2C8, 0x05E0,
      CAM_REG_WRITE16, 0xD2CA, 0xB502,
      CAM_REG_WRITE16, 0xD2CC, 0x72CF,
      CAM_REG_WRITE16, 0xD2CE, 0xFFFF,
      CAM_REG_WRITE16, 0xD2D0, 0xC5D4,
      CAM_REG_WRITE16, 0xD2D2, 0x923A,
      CAM_REG_WRITE16, 0xD2D4, 0x73CF,
      CAM_REG_WRITE16, 0xD2D6, 0xFFFF,
      CAM_REG_WRITE16, 0xD2D8, 0xC7D0,
      CAM_REG_WRITE16, 0xD2DA, 0xB020,  // [0:00:07.928]
      CAM_REG_WRITE16, 0xD2DC, 0x9220,
      CAM_REG_WRITE16, 0xD2DE, 0xB021,
      CAM_REG_WRITE16, 0xD2E0, 0x9221,
      CAM_REG_WRITE16, 0xD2E2, 0xB022,
      CAM_REG_WRITE16, 0xD2E4, 0x9222,
      CAM_REG_WRITE16, 0xD2E6, 0xB023,
      CAM_REG_WRITE16, 0xD2E8, 0x9223,
      CAM_REG_WRITE16, 0xD2EA, 0xB024,
      CAM_REG_WRITE16, 0xD2EC, 0x9227,
      CAM_REG_WRITE16, 0xD2EE, 0xB025,
      CAM_REG_WRITE16, 0xD2F0, 0x9228,
      CAM_REG_WRITE16, 0xD2F2, 0xB026,
      CAM_REG_WRITE16, 0xD2F4, 0x922B,
      CAM_REG_WRITE16, 0xD2F6, 0xB027,
      CAM_REG_WRITE16, 0xD2F8, 0x922C,
      CAM_REG_WRITE16, 0xD2FA, 0xB028,
      CAM_REG_WRITE16, 0xD2FC, 0x1258,
      CAM_REG_WRITE16, 0xD2FE, 0x0101,
      CAM_REG_WRITE16, 0xD300, 0xB029,
      CAM_REG_WRITE16, 0xD302, 0x125A,
      CAM_REG_WRITE16, 0xD304, 0x0101,
      CAM_REG_WRITE16, 0xD306, 0xB02A,
      CAM_REG_WRITE16, 0xD308, 0x1336,
      CAM_REG_WRITE16, 0xD30A, 0x8081,  // [0:00:07.931]
      CAM_REG_WRITE16, 0xD30C, 0xA836,
      CAM_REG_WRITE16, 0xD30E, 0x1337,
      CAM_REG_WRITE16, 0xD310, 0x8081,
      CAM_REG_WRITE16, 0xD312, 0xA837,
      CAM_REG_WRITE16, 0xD314, 0x12A7,
      CAM_REG_WRITE16, 0xD316, 0x0701,
      CAM_REG_WRITE16, 0xD318, 0xB02C,
      CAM_REG_WRITE16, 0xD31A, 0x1354,
      CAM_REG_WRITE16, 0xD31C, 0x8081,
      CAM_REG_WRITE16, 0xD31E, 0x7FE0,
      CAM_REG_WRITE16, 0xD320, 0xA83A,
      CAM_REG_WRITE16, 0xD322, 0x78E0,
      CAM_REG_WRITE16, 0xD324, 0xC0F1,
      CAM_REG_WRITE16, 0xD326, 0x0DC2,
      CAM_REG_WRITE16, 0xD328, 0x05C0,
      CAM_REG_WRITE16, 0xD32A, 0x7608,
      CAM_REG_WRITE16, 0xD32C, 0x09BB,
      CAM_REG_WRITE16, 0xD32E, 0x0010,
      CAM_REG_WRITE16, 0xD330, 0x75CF,
      CAM_REG_WRITE16, 0xD332, 0xFFFF,
      CAM_REG_WRITE16, 0xD334, 0xD4E0,
      CAM_REG_WRITE16, 0xD336, 0x8D21,
      CAM_REG_WRITE16, 0xD338, 0x8D00,
      CAM_REG_WRITE16, 0xD33A, 0x2153,  // [0:00:07.934]
      CAM_REG_WRITE16, 0xD33C, 0x0003,
      CAM_REG_WRITE16, 0xD33E, 0xB8C0,
      CAM_REG_WRITE16, 0xD340, 0x8D45,
      CAM_REG_WRITE16, 0xD342, 0x0B23,
      CAM_REG_WRITE16, 0xD344, 0x0000,
      CAM_REG_WRITE16, 0xD346, 0xEA8F,
      CAM_REG_WRITE16, 0xD348, 0x0915,
      CAM_REG_WRITE16, 0xD34A, 0x001E,
      CAM_REG_WRITE16, 0xD34C, 0xFF81,
      CAM_REG_WRITE16, 0xD34E, 0xE808,
      CAM_REG_WRITE16, 0xD350, 0x2540,
      CAM_REG_WRITE16, 0xD352, 0x1900,
      CAM_REG_WRITE16, 0xD354, 0xFFDE,
      CAM_REG_WRITE16, 0xD356, 0x8D00,
      CAM_REG_WRITE16, 0xD358, 0xB880,
      CAM_REG_WRITE16, 0xD35A, 0xF004,
      CAM_REG_WRITE16, 0xD35C, 0x8D00,
      CAM_REG_WRITE16, 0xD35E, 0xB8A0,
      CAM_REG_WRITE16, 0xD360, 0xAD00,
      CAM_REG_WRITE16, 0xD362, 0x8D05,
      CAM_REG_WRITE16, 0xD364, 0xE081,
      CAM_REG_WRITE16, 0xD366, 0x20CC,
      CAM_REG_WRITE16, 0xD368, 0x80A2,
      CAM_REG_WRITE16, 0xD36A, 0xDF00,  // [0:00:07.937]
      CAM_REG_WRITE16, 0xD36C, 0xF40A,
      CAM_REG_WRITE16, 0xD36E, 0x71CF,
      CAM_REG_WRITE16, 0xD370, 0xFFFF,
      CAM_REG_WRITE16, 0xD372, 0xC84C,
      CAM_REG_WRITE16, 0xD374, 0x9102,
      CAM_REG_WRITE16, 0xD376, 0x7708,
      CAM_REG_WRITE16, 0xD378, 0xB8A6,
      CAM_REG_WRITE16, 0xD37A, 0x2786,
      CAM_REG_WRITE16, 0xD37C, 0x1FFE,
      CAM_REG_WRITE16, 0xD37E, 0xB102,
      CAM_REG_WRITE16, 0xD380, 0x0B42,
      CAM_REG_WRITE16, 0xD382, 0x0180,
      CAM_REG_WRITE16, 0xD384, 0x0E3E,
      CAM_REG_WRITE16, 0xD386, 0x0180,
      CAM_REG_WRITE16, 0xD388, 0x0F4A,
      CAM_REG_WRITE16, 0xD38A, 0x0160,
      CAM_REG_WRITE16, 0xD38C, 0x70C9,
      CAM_REG_WRITE16, 0xD38E, 0x8D05,
      CAM_REG_WRITE16, 0xD390, 0xE081,
      CAM_REG_WRITE16, 0xD392, 0x20CC,
      CAM_REG_WRITE16, 0xD394, 0x80A2,
      CAM_REG_WRITE16, 0xD396, 0xF429,
      CAM_REG_WRITE16, 0xD398, 0x76CF,
      CAM_REG_WRITE16, 0xD39A, 0xFFFF,  // [0:00:07.940]
      CAM_REG_WRITE16, 0xD39C, 0xC84C,
      CAM_REG_WRITE16, 0xD39E, 0x082D,
      CAM_REG_WRITE16, 0xD3A0, 0x0051,
      CAM_REG_WRITE16, 0xD3A2, 0x70CF,
      CAM_REG_WRITE16, 0xD3A4, 0xFFFF,
      CAM_REG_WRITE16, 0xD3A6, 0xC90C,
      CAM_REG_WRITE16, 0xD3A8, 0x8805,
      CAM_REG_WRITE16, 0xD3AA, 0x09B6,
      CAM_REG_WRITE16, 0xD3AC, 0x0360,
      CAM_REG_WRITE16, 0xD3AE, 0xD908,
      CAM_REG_WRITE16, 0xD3B0, 0x2099,
      CAM_REG_WRITE16, 0xD3B2, 0x0802,
      CAM_REG_WRITE16, 0xD3B4, 0x9634,
      CAM_REG_WRITE16, 0xD3B6, 0xB503,
      CAM_REG_WRITE16, 0xD3B8, 0x7902,
      CAM_REG_WRITE16, 0xD3BA, 0x1523,
      CAM_REG_WRITE16, 0xD3BC, 0x1080,
      CAM_REG_WRITE16, 0xD3BE, 0xB634,
      CAM_REG_WRITE16, 0xD3C0, 0xE001,
      CAM_REG_WRITE16, 0xD3C2, 0x1D23,
      CAM_REG_WRITE16, 0xD3C4, 0x1002,
      CAM_REG_WRITE16, 0xD3C6, 0xF00B,
      CAM_REG_WRITE16, 0xD3C8, 0x9634,
      CAM_REG_WRITE16, 0xD3CA, 0x9503,  // [0:00:07.943]
      CAM_REG_WRITE16, 0xD3CC, 0x6038,
      CAM_REG_WRITE16, 0xD3CE, 0xB614,
      CAM_REG_WRITE16, 0xD3D0, 0x153F,
      CAM_REG_WRITE16, 0xD3D2, 0x1080,
      CAM_REG_WRITE16, 0xD3D4, 0xE001,
      CAM_REG_WRITE16, 0xD3D6, 0x1D3F,
      CAM_REG_WRITE16, 0xD3D8, 0x1002,
      CAM_REG_WRITE16, 0xD3DA, 0xFFA4,
      CAM_REG_WRITE16, 0xD3DC, 0x9602,
      CAM_REG_WRITE16, 0xD3DE, 0x7F05,
      CAM_REG_WRITE16, 0xD3E0, 0xD800,
      CAM_REG_WRITE16, 0xD3E2, 0xB6E2,
      CAM_REG_WRITE16, 0xD3E4, 0xAD05,
      CAM_REG_WRITE16, 0xD3E6, 0x0511,
      CAM_REG_WRITE16, 0xD3E8, 0x05E0,
      CAM_REG_WRITE16, 0xD3EA, 0xD800,
      CAM_REG_WRITE16, 0xD3EC, 0xC0F1,
      CAM_REG_WRITE16, 0xD3EE, 0x0CFE,
      CAM_REG_WRITE16, 0xD3F0, 0x05C0,
      CAM_REG_WRITE16, 0xD3F2, 0x0A96,
      CAM_REG_WRITE16, 0xD3F4, 0x05A0,
      CAM_REG_WRITE16, 0xD3F6, 0x7608,
      CAM_REG_WRITE16, 0xD3F8, 0x0C22,
      CAM_REG_WRITE16, 0xD3FA, 0x0240,  // [0:00:07.946]
      CAM_REG_WRITE16, 0xD3FC, 0xE080,
      CAM_REG_WRITE16, 0xD3FE, 0x20CA,
      CAM_REG_WRITE16, 0xD400, 0x0F82,
      CAM_REG_WRITE16, 0xD402, 0x0000,
      CAM_REG_WRITE16, 0xD404, 0x190B,
      CAM_REG_WRITE16, 0xD406, 0x0C60,
      CAM_REG_WRITE16, 0xD408, 0x05A2,
      CAM_REG_WRITE16, 0xD40A, 0x21CA,
      CAM_REG_WRITE16, 0xD40C, 0x0022,
      CAM_REG_WRITE16, 0xD40E, 0x0C56,
      CAM_REG_WRITE16, 0xD410, 0x0240,
      CAM_REG_WRITE16, 0xD412, 0xE806,
      CAM_REG_WRITE16, 0xD414, 0x0E0E,
      CAM_REG_WRITE16, 0xD416, 0x0220,
      CAM_REG_WRITE16, 0xD418, 0x70C9,
      CAM_REG_WRITE16, 0xD41A, 0xF048,
      CAM_REG_WRITE16, 0xD41C, 0x0896,
      CAM_REG_WRITE16, 0xD41E, 0x0440,
      CAM_REG_WRITE16, 0xD420, 0x0E96,
      CAM_REG_WRITE16, 0xD422, 0x0400,
      CAM_REG_WRITE16, 0xD424, 0x0966,
      CAM_REG_WRITE16, 0xD426, 0x0380,
      CAM_REG_WRITE16, 0xD428, 0x75CF,
      CAM_REG_WRITE16, 0xD42A, 0xFFFF,  // [0:00:07.949]
      CAM_REG_WRITE16, 0xD42C, 0xD4E0,
      CAM_REG_WRITE16, 0xD42E, 0x8D00,
      CAM_REG_WRITE16, 0xD430, 0x084D,
      CAM_REG_WRITE16, 0xD432, 0x001E,
      CAM_REG_WRITE16, 0xD434, 0xFF47,
      CAM_REG_WRITE16, 0xD436, 0x080D,
      CAM_REG_WRITE16, 0xD438, 0x0050,
      CAM_REG_WRITE16, 0xD43A, 0xFF57,
      CAM_REG_WRITE16, 0xD43C, 0x0841,
      CAM_REG_WRITE16, 0xD43E, 0x0051,
      CAM_REG_WRITE16, 0xD440, 0x8D04,
      CAM_REG_WRITE16, 0xD442, 0x9521,
      CAM_REG_WRITE16, 0xD444, 0xE064,
      CAM_REG_WRITE16, 0xD446, 0x790C,
      CAM_REG_WRITE16, 0xD448, 0x702F,
      CAM_REG_WRITE16, 0xD44A, 0x0CE2,
      CAM_REG_WRITE16, 0xD44C, 0x05E0,
      CAM_REG_WRITE16, 0xD44E, 0xD964,
      CAM_REG_WRITE16, 0xD450, 0x72CF,
      CAM_REG_WRITE16, 0xD452, 0xFFFF,
      CAM_REG_WRITE16, 0xD454, 0xC700,
      CAM_REG_WRITE16, 0xD456, 0x9235,
      CAM_REG_WRITE16, 0xD458, 0x0811,
      CAM_REG_WRITE16, 0xD45A, 0x0043,  // [0:00:07.952]
      CAM_REG_WRITE16, 0xD45C, 0xFF3D,
      CAM_REG_WRITE16, 0xD45E, 0x080D,
      CAM_REG_WRITE16, 0xD460, 0x0051,
      CAM_REG_WRITE16, 0xD462, 0xD801,
      CAM_REG_WRITE16, 0xD464, 0xFF77,
      CAM_REG_WRITE16, 0xD466, 0xF025,
      CAM_REG_WRITE16, 0xD468, 0x9501,
      CAM_REG_WRITE16, 0xD46A, 0x9235,
      CAM_REG_WRITE16, 0xD46C, 0x0911,
      CAM_REG_WRITE16, 0xD46E, 0x0003,
      CAM_REG_WRITE16, 0xD470, 0xFF49,
      CAM_REG_WRITE16, 0xD472, 0x080D,
      CAM_REG_WRITE16, 0xD474, 0x0051,
      CAM_REG_WRITE16, 0xD476, 0xD800,
      CAM_REG_WRITE16, 0xD478, 0xFF72,
      CAM_REG_WRITE16, 0xD47A, 0xF01B,
      CAM_REG_WRITE16, 0xD47C, 0x0886,
      CAM_REG_WRITE16, 0xD47E, 0x03E0,
      CAM_REG_WRITE16, 0xD480, 0xD801,
      CAM_REG_WRITE16, 0xD482, 0x0EF6,
      CAM_REG_WRITE16, 0xD484, 0x03C0,
      CAM_REG_WRITE16, 0xD486, 0x0F52,
      CAM_REG_WRITE16, 0xD488, 0x0340,
      CAM_REG_WRITE16, 0xD48A, 0x0DBA,  // [0:00:07.955]
      CAM_REG_WRITE16, 0xD48C, 0x0200,
      CAM_REG_WRITE16, 0xD48E, 0x0AF6,
      CAM_REG_WRITE16, 0xD490, 0x0440,
      CAM_REG_WRITE16, 0xD492, 0x0C22,
      CAM_REG_WRITE16, 0xD494, 0x0400,
      CAM_REG_WRITE16, 0xD496, 0x0D72,
      CAM_REG_WRITE16, 0xD498, 0x0440,
      CAM_REG_WRITE16, 0xD49A, 0x0DC2,
      CAM_REG_WRITE16, 0xD49C, 0x0200,
      CAM_REG_WRITE16, 0xD49E, 0x0972,
      CAM_REG_WRITE16, 0xD4A0, 0x0440,
      CAM_REG_WRITE16, 0xD4A2, 0x0D3A,
      CAM_REG_WRITE16, 0xD4A4, 0x0220,
      CAM_REG_WRITE16, 0xD4A6, 0xD820,
      CAM_REG_WRITE16, 0xD4A8, 0x0BFA,
      CAM_REG_WRITE16, 0xD4AA, 0x0260,
      CAM_REG_WRITE16, 0xD4AC, 0x70C9,
      CAM_REG_WRITE16, 0xD4AE, 0x0451,
      CAM_REG_WRITE16, 0xD4B0, 0x05C0,
      CAM_REG_WRITE16, 0xD4B2, 0x78E0,
      CAM_REG_WRITE16, 0xD4B4, 0xD900,
      CAM_REG_WRITE16, 0xD4B6, 0xF00A,
      CAM_REG_WRITE16, 0xD4B8, 0x70CF,
      CAM_REG_WRITE16, 0xD4BA, 0xFFFF,  // [0:00:07.958]
      CAM_REG_WRITE16, 0xD4BC, 0xD520,
      CAM_REG_WRITE16, 0xD4BE, 0x7835,
      CAM_REG_WRITE16, 0xD4C0, 0x8041,
      CAM_REG_WRITE16, 0xD4C2, 0x8000,
      CAM_REG_WRITE16, 0xD4C4, 0xE102,
      CAM_REG_WRITE16, 0xD4C6, 0xA040,
      CAM_REG_WRITE16, 0xD4C8, 0x09F1,
      CAM_REG_WRITE16, 0xD4CA, 0x8114,
      CAM_REG_WRITE16, 0xD4CC, 0x71CF,
      CAM_REG_WRITE16, 0xD4CE, 0xFFFF,
      CAM_REG_WRITE16, 0xD4D0, 0xD4E0,
      CAM_REG_WRITE16, 0xD4D2, 0x70CF,
      CAM_REG_WRITE16, 0xD4D4, 0xFFFF,
      CAM_REG_WRITE16, 0xD4D6, 0xC594,
      CAM_REG_WRITE16, 0xD4D8, 0xB03A,
      CAM_REG_WRITE16, 0xD4DA, 0x7FE0,
      CAM_REG_WRITE16, 0xD4DC, 0xD800,
      CAM_REG_WRITE16, 0xD4DE, 0x0000,
      CAM_REG_WRITE16, 0xD4E0, 0x0000,
      CAM_REG_WRITE16, 0xD4E2, 0x0500,
      CAM_REG_WRITE16, 0xD4E4, 0x0500,
      CAM_REG_WRITE16, 0xD4E6, 0x0200,
      CAM_REG_WRITE16, 0xD4E8, 0x0330,
      CAM_REG_WRITE16, 0xD4EA, 0x0000,  // [0:00:07.961]
      CAM_REG_WRITE16, 0xD4EC, 0x0000,
      CAM_REG_WRITE16, 0xD4EE, 0x03CD,
      CAM_REG_WRITE16, 0xD4F0, 0x050D,
      CAM_REG_WRITE16, 0xD4F2, 0x01C5,
      CAM_REG_WRITE16, 0xD4F4, 0x03B3,
      CAM_REG_WRITE16, 0xD4F6, 0x00E0,
      CAM_REG_WRITE16, 0xD4F8, 0x01E3,
      CAM_REG_WRITE16, 0xD4FA, 0x0280,
      CAM_REG_WRITE16, 0xD4FC, 0x01E0,
      CAM_REG_WRITE16, 0xD4FE, 0x0109,
      CAM_REG_WRITE16, 0xD500, 0x0080,
      CAM_REG_WRITE16, 0xD502, 0x0500,
      CAM_REG_WRITE16, 0xD504, 0x0000,
      CAM_REG_WRITE16, 0xD506, 0x0000,
      CAM_REG_WRITE16, 0xD508, 0x0000,
      CAM_REG_WRITE16, 0xD50A, 0x0000,
      CAM_REG_WRITE16, 0xD50C, 0x0000,
      CAM_REG_WRITE16, 0xD50E, 0x0000,
      CAM_REG_WRITE16, 0xD510, 0x0000,
      CAM_REG_WRITE16, 0xD512, 0x0000,
      CAM_REG_WRITE16, 0xD514, 0x0000,
      CAM_REG_WRITE16, 0xD516, 0x0000,
      CAM_REG_WRITE16, 0xD518, 0x0000,
      CAM_REG_WRITE16, 0xD51A, 0x0000,  // [0:00:07.964]
      CAM_REG_WRITE16, 0xD51C, 0x0000,
      CAM_REG_WRITE16, 0xD51E, 0x0000,
      CAM_REG_WRITE16, 0xD520, 0xFFFF,
      CAM_REG_WRITE16, 0xD522, 0xC9B4,
      CAM_REG_WRITE16, 0xD524, 0xFFFF,
      CAM_REG_WRITE16, 0xD526, 0xD324,
      CAM_REG_WRITE16, 0xD528, 0xFFFF,
      CAM_REG_WRITE16, 0xD52A, 0xCA34,
      CAM_REG_WRITE16, 0xD52C, 0xFFFF,
      CAM_REG_WRITE16, 0xD52E, 0xD3EC,  // [0:00:07.966]
      CAM_REG_WRITE16, 0x098E, 0x0000, // LOGICAL_ADDRESS_ACCESS - [0:00:07.969]

// [Apply Patch 0302]
      CAM_REG_WRITE16, 0xE000, 0x04B4, // PATCHLDR_LOADER_ADDRESS - [0:00:07.974]
      CAM_REG_WRITE16, 0xE002, 0x0302, // PATCHLDR_PATCH_ID - [0:00:07.979]
      CAM_REG_WRITE32, 0xE004, 0x41030202, // PATCHLDR_FIRMWARE_ID - [0:00:07.984]
      CAM_APPLY_PATCH, 0, 0, 
#endif


#if 1
//[Step5-AWB_CCM]
//[Color Correction Matrices 04/13/15 10:31:59 - A-1040SOC - REV2 ]
      CAM_REG_WRITE16, 0x098E, 0x0000, // LOGICAL_ADDRESS_ACCESS - [0:00:08.443]
      CAM_REG_WRITE16, 0xC892, 0x01F5, // CAM_AWB_CCM_L_0 - [0:00:08.448]
      CAM_REG_WRITE16, 0xC894, 0xFF85, // CAM_AWB_CCM_L_1 - [0:00:08.453]
      CAM_REG_WRITE16, 0xC896, 0xFF85, // CAM_AWB_CCM_L_2 - [0:00:08.458]
      CAM_REG_WRITE16, 0xC898, 0xFF85, // CAM_AWB_CCM_L_3 - [0:00:08.463]
      CAM_REG_WRITE16, 0xC89A, 0x012A, // CAM_AWB_CCM_L_4 - [0:00:08.468]
      CAM_REG_WRITE16, 0xC89C, 0x0051, // CAM_AWB_CCM_L_5 - [0:00:08.473]
      CAM_REG_WRITE16, 0xC89E, 0xFF98, // CAM_AWB_CCM_L_6 - [0:00:08.478]
      CAM_REG_WRITE16, 0xC8A0, 0xFEB4, // CAM_AWB_CCM_L_7 - [0:00:08.483]
      CAM_REG_WRITE16, 0xC8A2, 0x02B4, // CAM_AWB_CCM_L_8 - [0:00:08.488]
      CAM_REG_WRITE16, 0xC8C8, 0x0074, // CAM_AWB_CCM_L_RG_GAIN - [0:00:08.493]
      CAM_REG_WRITE16, 0xC8CA, 0x011E, // CAM_AWB_CCM_L_BG_GAIN - [0:00:08.498]
      CAM_REG_WRITE16, 0xC8A4, 0x0226, // CAM_AWB_CCM_M_0 - [0:00:08.503]
      CAM_REG_WRITE16, 0xC8A6, 0xFF38, // CAM_AWB_CCM_M_1 - [0:00:08.508]
      CAM_REG_WRITE16, 0xC8A8, 0xFFA2, // CAM_AWB_CCM_M_2 - [0:00:08.513]
      CAM_REG_WRITE16, 0xC8AA, 0xFF8D, // CAM_AWB_CCM_M_3 - [0:00:08.518]
      CAM_REG_WRITE16, 0xC8AC, 0x0145, // CAM_AWB_CCM_M_4 - [0:00:08.523]
      CAM_REG_WRITE16, 0xC8AE, 0x002E, // CAM_AWB_CCM_M_5 - [0:00:08.528]
      CAM_REG_WRITE16, 0xC8B0, 0xFFCB, // CAM_AWB_CCM_M_6 - [0:00:08.533]
      CAM_REG_WRITE16, 0xC8B2, 0xFF17, // CAM_AWB_CCM_M_7 - [0:00:08.538]
      CAM_REG_WRITE16, 0xC8B4, 0x021E, // CAM_AWB_CCM_M_8 - [0:00:08.543]
      CAM_REG_WRITE16, 0xC8CC, 0x0095, // CAM_AWB_CCM_M_RG_GAIN - [0:00:08.548]
      CAM_REG_WRITE16, 0xC8CE, 0x0100, // CAM_AWB_CCM_M_BG_GAIN - [0:00:08.553]
      CAM_REG_WRITE16, 0xC8B6, 0x021E, // CAM_AWB_CCM_R_0 - [0:00:08.558]
      CAM_REG_WRITE16, 0xC8B8, 0xFF2D, // CAM_AWB_CCM_R_1 - [0:00:08.563]
      CAM_REG_WRITE16, 0xC8BA, 0xFFB5, // CAM_AWB_CCM_R_2 - [0:00:08.568]
      CAM_REG_WRITE16, 0xC8BC, 0xFF9C, // CAM_AWB_CCM_R_3 - [0:00:08.573]
      CAM_REG_WRITE16, 0xC8BE, 0x0183, // CAM_AWB_CCM_R_4 - [0:00:08.578]
      CAM_REG_WRITE16, 0xC8C0, 0xFFE0, // CAM_AWB_CCM_R_5 - [0:00:08.583]
      CAM_REG_WRITE16, 0xC8C2, 0xFFF4, // CAM_AWB_CCM_R_6 - [0:00:08.588]
      CAM_REG_WRITE16, 0xC8C4, 0xFF78, // CAM_AWB_CCM_R_7 - [0:00:08.593]
      CAM_REG_WRITE16, 0xC8C6, 0x0194, // CAM_AWB_CCM_R_8 - [0:00:08.598]
      CAM_REG_WRITE16, 0xC8D0, 0x00B3, // CAM_AWB_CCM_R_RG_GAIN - [0:00:08.603]
      CAM_REG_WRITE16, 0xC8D2, 0x008D, // CAM_AWB_CCM_R_BG_GAIN - [0:00:08.608]
      CAM_REG_WRITE16, 0xC8D4, 0x09C4, // CAM_AWB_CCM_L_CTEMP - [0:00:08.613]
      CAM_REG_WRITE16, 0xC8D6, 0x0D67, // CAM_AWB_CCM_M_CTEMP - [0:00:08.618]
      CAM_REG_WRITE16, 0xC8D8, 0x1964, // CAM_AWB_CCM_R_CTEMP - [0:00:08.623]
      CAM_REG_WRITE16, 0xC8EC, 0x09C4, // CAM_AWB_COLOR_TEMPERATURE_MIN - [0:00:08.628]
      CAM_REG_WRITE16, 0xC8EE, 0x1964, // CAM_AWB_COLOR_TEMPERATURE_MAX - [0:00:08.633]
      CAM_REG_WRITE8, 0xC8F2, 0x03, // CAM_AWB_AWB_XSCALE - [0:00:08.638]
      CAM_REG_WRITE8, 0xC8F3, 0x02, // CAM_AWB_AWB_YSCALE - [0:00:08.643]
      CAM_REG_WRITE16, 0xC8F4, 0xF800, // CAM_AWB_AWB_WEIGHTS_0 - [0:00:08.648]
      CAM_REG_WRITE16, 0xC8F6, 0xA800, // CAM_AWB_AWB_WEIGHTS_1 - [0:00:08.653]
      CAM_REG_WRITE16, 0xC8F8, 0x5400, // CAM_AWB_AWB_WEIGHTS_2 - [0:00:08.658]
      CAM_REG_WRITE16, 0xC8FA, 0x1B80, // CAM_AWB_AWB_WEIGHTS_3 - [0:00:08.663]
      CAM_REG_WRITE16, 0xC8FC, 0x0DC0, // CAM_AWB_AWB_WEIGHTS_4 - [0:00:08.668]
      CAM_REG_WRITE16, 0xC8FE, 0x0000, // CAM_AWB_AWB_WEIGHTS_5 - [0:00:08.673]
      CAM_REG_WRITE16, 0xC900, 0x0000, // CAM_AWB_AWB_WEIGHTS_6 - [0:00:08.678]
      CAM_REG_WRITE16, 0xC902, 0x0000, // CAM_AWB_AWB_WEIGHTS_7 - [0:00:08.683]
      CAM_REG_WRITE16, 0xC904, 0x0030, // CAM_AWB_AWB_XSHIFT_PRE_ADJ - [0:00:08.688]
      CAM_REG_WRITE16, 0xC906, 0x002A, // CAM_AWB_AWB_YSHIFT_PRE_ADJ - [0:00:08.693]
#endif

#if 1
// [Step7-CPIPE_Preference]
      CAM_REG_WRITE16, 0xC926, 0x0020, // CAM_LL_START_BRIGHTNESS - [0:00:08.698]
      CAM_REG_WRITE16, 0xC928, 0x009A, // CAM_LL_STOP_BRIGHTNESS - [0:00:08.703]
      CAM_REG_WRITE16, 0xC946, 0x0070, // CAM_LL_START_GAIN_METRIC - [0:00:08.708]
      CAM_REG_WRITE16, 0xC948, 0x00F3, // CAM_LL_STOP_GAIN_METRIC - [0:00:08.713]
      CAM_REG_WRITE16, 0xC952, 0x0020, // CAM_LL_START_TARGET_LUMA_BM - [0:00:08.718]
      CAM_REG_WRITE16, 0xC954, 0x009A, // CAM_LL_STOP_TARGET_LUMA_BM - [0:00:08.723]
#endif
#if 1
      CAM_REG_WRITE8, 0xC92A, 0x80, // CAM_LL_START_SATURATION - [0:00:08.728]
      CAM_REG_WRITE8, 0xC92B, 0x4B, // CAM_LL_END_SATURATION - [0:00:08.733]
      CAM_REG_WRITE8, 0xC92C, 0x00, // CAM_LL_START_DESATURATION - [0:00:08.738]
      CAM_REG_WRITE8, 0xC92D, 0xFF, // CAM_LL_END_DESATURATION - [0:00:08.743]
#endif
#if 1
      CAM_REG_WRITE8, 0xC92E, 0x3C, // CAM_LL_START_DEMOSAIC - [0:00:08.748]
      CAM_REG_WRITE8, 0xC92F, 0x02, // CAM_LL_START_AP_GAIN - [0:00:08.753]
      CAM_REG_WRITE8, 0xC930, 0x06, // CAM_LL_START_AP_THRESH - [0:00:08.758]
      CAM_REG_WRITE8, 0xC931, 0x64, // CAM_LL_STOP_DEMOSAIC - [0:00:08.763]

      CAM_REG_WRITE8, 0xC932, 0x01, // CAM_LL_STOP_AP_GAIN - [0:00:08.768]
      CAM_REG_WRITE8, 0xC933, 0x0C, // CAM_LL_STOP_AP_THRESH - [0:00:08.773]
      CAM_REG_WRITE8, 0xC934, 0x3C, // CAM_LL_START_NR_RED - [0:00:08.778]
      CAM_REG_WRITE8, 0xC935, 0x3C, // CAM_LL_START_NR_GREEN - [0:00:08.783]
      CAM_REG_WRITE8, 0xC936, 0x3C, // CAM_LL_START_NR_BLUE - [0:00:08.788]
      CAM_REG_WRITE8, 0xC937, 0x0F, // CAM_LL_START_NR_THRESH - [0:00:08.793]
      CAM_REG_WRITE8, 0xC938, 0x64, // CAM_LL_STOP_NR_RED - [0:00:08.798]
      CAM_REG_WRITE8, 0xC939, 0x64, // CAM_LL_STOP_NR_GREEN - [0:00:08.803]
      CAM_REG_WRITE8, 0xC93A, 0x64, // CAM_LL_STOP_NR_BLUE - [0:00:08.808]
      CAM_REG_WRITE8, 0xC93B, 0x32, // CAM_LL_STOP_NR_THRESH - [0:00:08.813]
#endif
#if 1
      CAM_REG_WRITE16, 0xC93C, 0x0020, // CAM_LL_START_CONTRAST_BM - [0:00:08.818]
      CAM_REG_WRITE16, 0xC93E, 0x009A, // CAM_LL_STOP_CONTRAST_BM - [0:00:08.823]
      //CAM_REG_WRITE16, 0xC940, 0x00DC, // CAM_LL_GAMMA - [0:00:08.828]
      // reduce gamma by a bit to not distort hues but allow darker colors 
      CAM_REG_WRITE16, 0xC940, 150, // CAM_LL_GAMMA - [0:00:08.828]
      CAM_REG_WRITE8, 0xC942, 0x38, // CAM_LL_START_CONTRAST_GRADIENT - [0:00:08.833]
      CAM_REG_WRITE8, 0xC943, 0x30, // CAM_LL_STOP_CONTRAST_GRADIENT - [0:00:08.838]
      CAM_REG_WRITE8, 0xC944, 0x50, // CAM_LL_START_CONTRAST_LUMA_PERCENTAGE - [0:00:08.843]
      CAM_REG_WRITE8, 0xC945, 0x19, // CAM_LL_STOP_CONTRAST_LUMA_PERCENTAGE - [0:00:08.848]
      CAM_REG_WRITE16, 0xC94A, 0x0230, // CAM_LL_START_FADE_TO_BLACK_LUMA - [0:00:08.853]
      CAM_REG_WRITE16, 0xC94C, 0x0010, // CAM_LL_STOP_FADE_TO_BLACK_LUMA - [0:00:08.858]
      CAM_REG_WRITE16, 0xC94E, 0x01CD, // CAM_LL_CLUSTER_DC_TH_BM - [0:00:08.863]
      CAM_REG_WRITE8, 0xC950, 0x05, // CAM_LL_CLUSTER_DC_GATE_PERCENTAGE - [0:00:08.868]
      CAM_REG_WRITE8, 0xC951, 0x40, // CAM_LL_SUMMING_SENSITIVITY_FACTOR - [0:00:08.873]
      CAM_REG_WRITE8, 0xC87B, 0x1B, // CAM_AET_TARGET_AVERAGE_LUMA_DARK - [0:00:08.878]
#endif
#if 1
      CAM_REG_WRITE16, 0xC890, 0x0080, // CAM_AET_TARGET_GAIN - [0:00:08.888]
      //CAM_REG_WRITE16, 0xC886, 0x0100, // CAM_AET_AE_MAX_VIRT_AGAIN - [0:00:08.893]
      CAM_REG_WRITE16, 0xC87C, 0x005A, // CAM_AET_BLACK_CLIPPING_TARGET - [0:00:08.898]
      CAM_REG_WRITE8, 0xB42A, 0x05, // CCM_DELTA_GAIN - [0:00:08.903]
      CAM_REG_WRITE8, 0xA80A, 0x20, // AE_TRACK_AE_TRACKING_DAMPENING_SPEED - [0:00:08.908]
#endif


#if 1
 // [Speed up AE/AWB]
      CAM_REG_WRITE16, 0xA802, 0x0008, // RESERVED_AE_TRACK_02 - [0:00:08.921]
      CAM_REG_WRITE8, 0xC908, 0x01, // RESERVED_CAM_108 - [0:00:08.926]
      CAM_REG_WRITE8, 0xC879, 0x01, // CAM_AET_SKIP_FRAMES - [0:00:08.931]
      CAM_REG_WRITE8, 0xC909, 0x02, // CAM_AWB_AWBMODE - [0:00:08.936]
      CAM_REG_WRITE8, 0xA80A, 0x18, // AE_TRACK_AE_TRACKING_DAMPENING_SPEED - [0:00:08.938],
      CAM_REG_WRITE8, 0xA80B, 0x18, // AE_TRACK_AE_DAMPENING_SPEED - [0:00:08.943]
      CAM_REG_WRITE8, 0xAC16, 0x18, // AWB_PRE_AWB_RATIOS_TRACKING_SPEED - [0:00:08.948]
#endif

// [Step8-Features]
// [Parallel setting for SOC1040]
      //CAM_REG_WRITE16, 0x3210, 0x0018, // disable gamma, CCM for hue-based algorithms  
      CAM_REG_WRITE8, 0xBC07, 0x02, // disable auto gamma curve, set for noise reduction only 
      //CAM_REG_WRITE16, 0xC87C, 0x0010, // CAM_AET_BLACK_CLIPPING_TARGET - [0:00:08.898]
      //CAM_REG_WRITE16, 0xAC04, 0x0208,
      //CAM_REG_WRITE16, 0xC88C, 0x36B3,
      //CAM_REG_WRITE16, 0xC88E, 0x36B3,

      CAM_REG_WRITE16, 0x301a, 0x00234,         // enable skip bit
      CAM_REG_WRITE16, 0xC87C, 0x005A, // CAM_AET_BLACK_CLIPPING_TARGET - [0:00:08.898]
      CAM_REG_WRITE8, 0xCC00, 0x08,  // change AE mode, 
      CAM_REG_WRITE16, 0xC984, 0x8000, // CAM_PORT_OUTPUT_CONTROL - [0:00:08.913]
      CAM_REG_WRITE16, 0x001E, 0x0655, // PAD_SLEW - [0:00:08.916]

      //CAM_REG_WRITE16, 0xc834, 0x0332, // invert image

#endif
    // Change Config
    CAM_SEND_COMMAND, 0, 0, 

    CAM_END

};

static int cam_runCommands(const CamCommand *commands);

int cam_sendCommand()
{
    uint16_t success;

    g_sccb->Write8(0xdc00, 0x28); 
    g_sccb->Write16(0x0080, 0x8002);
    while(g_sccb->Read16(0x0080)&0x02);
    delayus(1000);

    success = g_sccb->Read16(0x0080);
    if ((success&0x8000)==0)
        return -1;

    return 0;       
}

int cam_applyPatch()
{
    uint16_t success;

    g_sccb->Write16(0x0080, 0x8001);
    while(g_sccb->Read16(0x0080)&0x01);
    delayus(1000);
    success = g_sccb->Read16(0x0080);
    if ((success&0x8000)==0)
        return -1;

    return 0;       
}
 

int cam_runCommands(const CamCommand *commands)
{
    int i;

    for (i=0; commands[i].command!=CAM_END; i++)
    {
        switch(commands[i].command)
        {
        case CAM_DELAY:
            delayus(commands[i].reg);
            break;

        case CAM_SEND_COMMAND:
            cam_sendCommand();
            break;

        case CAM_APPLY_PATCH:
            cam_applyPatch();
            break;

        case CAM_REG_WRITE8:
            g_sccb->Write8(commands[i].reg, commands[i].val);
#ifdef VERIFY
            if (g_sccb->Read8(commands[i].reg)!=commands[i].val)
            {
                printf("error %x %x %x\n", commands[i].reg, commands[i].val, g_sccb->Read8(commands[i].reg));
                return -1;
            }
#endif
            break;

        case CAM_REG_WRITE16:
            g_sccb->Write16(commands[i].reg, commands[i].val);
#ifdef VERIFY
            if (g_sccb->Read16(commands[i].reg)!=commands[i].val)
            {
                printf("error %x %x %x\n", commands[i].reg, commands[i].val, g_sccb->Read16(commands[i].reg));
                return -1;
            }
#endif
            break;

        case CAM_REG_WRITE32:
            g_sccb->Write32(commands[i].reg, commands[i].val);
#ifdef VERIFY
            if (g_sccb->Read32(commands[i].reg)!=commands[i].val)
            {
                printf("error %x %x %x\n", commands[i].reg, commands[i].val, g_sccb->Read32(commands[i].reg));
                return -1;
            }
#endif
            break;

        case CAM_SET_RESOLUTION:
            cam_setResolution(0, 0, commands[i].reg, commands[i].val);
            break;
        }
    }


    return 0;
}


int cam_init(uint16_t *hwVer)
{   
    g_sccb = new CSccb(0x90);

    cam_runCommands(camInitData);
	if (hwVer[1]==2)
		cam_runCommands(camAPGAData);
	//else
	//	cam_runCommands(camAPGAData2);
    
    g_chirpUsb->registerModule(g_module);
    
    g_getFrameM0 = g_chirpM0->getProc("getFrame", NULL);
    g_getTimingM0 = g_chirpM0->getProc("getTiming", NULL);
    
    if (g_getFrameM0<0 || g_getTimingM0<0)
        return -1;

    cam_loadParams();
    
    // get timings
    uint16_t hblank, hactive, vblank, vactive;
    cam_getTiming(&hblank, &hactive, &vblank, &vactive);
    
    return 0;
}

int32_t cam_setMode(const uint8_t &mode)
{
    if (mode==0) // raw video
    {
        uint8_t saturation;
        g_mode = 0;
        //g_sccb->Write16(0x3210, 0x00a8); // enable gamma
        //g_sccb->Write16(0xc87c, 0x005a); // increase black level clipping
        prm_get("Color saturation", &saturation, END);
        cam_setSaturation(saturation);
        //cam_setBrightness(g_brightness);
    }
    else if (mode!=0) // CCC, processing
    {
        g_mode = 1;
        //g_sccb->Write16(0x3210, 0x0018); // disable gamma
        //g_sccb->Write16(0xc87c, 0x000a); // decrease black-level clipping 
        cam_setSaturation(CAM_SATURATION_DEFAULT);
        //cam_setBrightness(g_brightness);
    }
    return 0;
}

uint32_t cam_getMode()
{
    return g_mode;
}

int32_t cam_setAWB(const uint8_t &awb)
{
    if (awb!=g_awb)
    {
        if (awb==0)
        {
            g_sccb->Write8(0xcc01, 0);
            g_awb = 0;
        }
        else
        {
            g_sccb->Write8(0xcc01, 1);
            g_awb = 1;
        }
        // wait for this to propagate before setting other registers, for example, setting white-balance value
        delayms(20);
    }
    return 0;
}

uint32_t cam_getAWB()
{
    return g_awb;
}
                               
int32_t cam_setWBV(const uint32_t &wbv)
{
    g_sccb->Write16(0xcc18, wbv);
    return 0;           
}

int32_t cam_getWBV()
{
    uint16_t wbv;

    // can't get WBV while AWB is enabled
    if (g_awb)
        return -1;
    wbv = g_sccb->Read16(0xcc18);

    return wbv;
}

int32_t cam_setAEC(const uint8_t &aec)
{
    if (aec!=g_aec)
    {
        if (aec==0)
        {
            g_sccb->Write8(0xcc00, 1); // turn off AEC, AGC
            g_aec = 0;
        }
        else
        {
            g_sccb->Write8(0xcc00, 8); // turn on AEC, AGC
            g_aec = 1;
        }
    }
    return 0;
}

uint32_t cam_getAEC()
{
    return g_aec;
}

int32_t cam_setECV(const uint32_t &ecv)
{
    uint32_t val = ecv;
    g_sccb->Write16(0xcc0e, val&0xffff);
    val >>= 16;
    g_sccb->Write32(0xcc04, val);

    return 0;       
}

uint32_t cam_getECV()
{
    uint32_t ecv;

    ecv = g_sccb->Read32(0xcc04);
    ecv <<= 16;
    ecv |= g_sccb->Read16(0xcc0e);

    return ecv;
}

int32_t cam_setBrightness(const uint8_t &brightness)
{
    uint16_t b;

#if 0
    if (g_mode==1)
        b = (uint16_t)(brightness*CAM_BRIGHTNESS_SCALE);
    else
#endif
        b = brightness;

    if (b>0xff)
        b = 0xff;

    g_sccb->Write16(0xcc0a, b);
    g_brightness = brightness;

    return 0;
}

int32_t cam_setSaturation(const uint8_t &saturation)
{
    g_sccb->Write16(0xcc12, saturation);
    g_saturation = saturation;

    return 0;
}

uint32_t cam_getSaturation()
{
    return g_saturation;
}

uint32_t cam_getBrightness()
{
    return g_brightness;
}

int32_t cam_setFlickerAvoidance(const uint8_t &fa)
{
    if (fa)
        g_sccb->Write8(0xcc21, 1);
    else
        g_sccb->Write8(0xcc21, 0);

    return 0;
}
        

int cam_testPattern(const uint8_t &enable)
{
    if (enable)
    {
        g_sccb->Write8(0xc84c, 2);
        g_sccb->Write8(0xc84d, 4);
    }
    else
        g_sccb->Write8(0xc84c, 0);

    cam_sendCommand();
    return 0;
}

int32_t cam_getFrame(uint8_t *memory, uint32_t memSize, uint8_t type, uint16_t xOffset, uint16_t yOffset, uint16_t xWidth, uint16_t yWidth)
{
    int32_t res;
    int32_t responseInt = -1;

    if (xWidth*yWidth>memSize)
        return -2;

    // check resolutions
    res = type >> 4;
    if (res==0)
    {
        if (xOffset+xWidth>CAM_RES0_WIDTH || yOffset+yWidth>CAM_RES0_HEIGHT)
            return -1;
    }
    else if (res==1) 
    {
        if (xOffset+xWidth>CAM_RES1_WIDTH || yOffset+yWidth>CAM_RES1_HEIGHT)
            return -1;
    }
    else if (res==2)
    {
        if (xOffset+xWidth>CAM_RES2_WIDTH || yOffset+yWidth>CAM_RES2_HEIGHT)
            return -1;
    }
    else
        return -3;

    // forward call to M0, get frame
    g_chirpM0->callSync(g_getFrameM0, 
        UINT8(type), UINT32((uint32_t)memory), UINT16(xOffset), UINT16(yOffset), UINT16(xWidth), UINT16(yWidth), END_OUT_ARGS,
        &responseInt, END_IN_ARGS);

    if (responseInt==0)
    {
        g_rawFrame.m_pixels = memory;
        g_rawFrame.m_width = xWidth;
        g_rawFrame.m_height = yWidth;
    }

    //printf("%x %x %x %x %x\n", g_sccb->Read16(0xc909), g_sccb->Read16(0xac00)&0x1f, g_sccb->Read16(0xc8f0), g_sccb->Read16(0xc8ec), g_sccb->Read16(0xc8ee));

    return responseInt;
}

int32_t cam_getTiming(uint16_t *hblank, uint16_t *hactive, uint16_t *vblank, uint16_t *vactive)
{
    int32_t responseInt = -1;
    g_chirpM0->callSync(g_getTimingM0, END_OUT_ARGS, &responseInt, hblank, hactive, vblank, vactive, END_IN_ARGS);
    printf("timing: %d %d %d %d\n", *hblank, *hactive, *vblank, *vactive);

    return responseInt;
}

int32_t cam_setFramerate(const uint8_t &framerate)
{
    uint16_t val = framerate*0x100;
    g_sccb->Write16(0xC88C, val);       //cam_aet_max_frame_rate = 128*34000000/((46+height)*(542+width))
    g_sccb->Write16(0xC88E, val);       //cam_aet_min_frame_rate = 128*34000000/((46+height)*(542+width))
    cam_sendCommand();
    // wait a bit because this takes some time to propagate through the imager
    delayms(50);
    return 0;

}


int32_t cam_setResolution(const uint16_t &xoffset, const uint16_t &yoffset, const uint16_t &width, const uint16_t &height)
{
#if 1
    uint16_t xoffset2 = (xoffset&~0x03)*2;
    uint16_t yoffset2 = 0; //yoffset*2 + CAM_LINE_SKIP;
    uint16_t width2 = (width + CAM_PIXEL_SKIP)*2;
    uint16_t height2 = (height+1)*2;

    float frate = (float)17000000.0/((46+(float)height2)*(542+(float)width2))*512 + (float)0.5;
    uint16_t rate = (uint16_t)frate;

    g_sccb->Write16(0xC800, yoffset2);      //cam_sensor_cfg_y_addr_start = yoffset
    g_sccb->Write16(0xC802, xoffset2);      //cam_sensor_cfg_x_addr_start = xoffset
    g_sccb->Write16(0xC804, 493+height2);   //cam_sensor_cfg_y_addr_end = 493+height
    g_sccb->Write16(0xC806, 653+width2);        //cam_sensor_cfg_x_addr_end = 653+width

    g_sccb->Write32(0xC808, 0x206CC80);     //cam_sensor_cfg_pixclk = 34000000
    g_sccb->Write16(0xC80C, 0x0001);        //cam_sensor_cfg_row_speed = 1
    g_sccb->Write16(0xC80E, 0x01C3);        //cam_sensor_cfg_fine_integ_time_min = 451

    g_sccb->Write16(0xC810, 307+width2);        //cam_sensor_cfg_fine_integ_time_max = 307+width
    g_sccb->Write16(0xC812, 46+height2);        //cam_sensor_cfg_frame_length_lines = 46+height
    g_sccb->Write16(0xC814, 542+width2);        //cam_sensor_cfg_line_length_pck = 542+width

    g_sccb->Write16(0xC816, 0x00E0);        //cam_sensor_cfg_fine_correction = 224

    g_sccb->Write16(0xC818, 3+height2);     //cam_sensor_cfg_cpipe_last_row = 3+height

    g_sccb->Write16(0xC826, 0x0020);        //cam_sensor_cfg_reg_0_data = 32
    g_sccb->Write16(0xC834, 0x0330);        //cam_sensor_control_read_mode = 816
    g_sccb->Write16(0xC854, 0x0000);        //cam_crop_window_xoffset = 0
    g_sccb->Write16(0xC856, 0x0000);        //cam_crop_window_yoffset = 0   

    g_sccb->Write16(0xC858, width2);        //cam_crop_window_width = width
    g_sccb->Write16(0xC85A, height2);       //cam_crop_window_height = height

    g_sccb->Write8(0xC85C, 0x03);       //cam_crop_cropmode = 3

    g_sccb->Write16(0xC868, width2);        //cam_output_width = width
    g_sccb->Write16(0xC86A, height2);       //cam_output_height = height

    g_sccb->Write16(0xC86C, 0x0E10);        //cam_output_format Processed Bayer
    g_sccb->Write8(0xC878, 0x00);       //cam_aet_aemode = 0

    g_sccb->Write16(0xC88C, rate);      //cam_aet_max_frame_rate = 128*34000000/((46+height)*(542+width))
    g_sccb->Write16(0xC88E, rate);      //cam_aet_min_frame_rate = 128*34000000/((46+height)*(542+width))

    g_sccb->Write16(0xC914, 0x0000);        //cam_stat_awb_clip_window_xstart = 0
    g_sccb->Write16(0xC916, 0x0000);        //cam_stat_awb_clip_window_ystart = 0

    g_sccb->Write16(0xC918, width2-1);      //cam_stat_awb_clip_window_xend = width-1
    g_sccb->Write16(0xC91A, height2-1);     //cam_stat_awb_clip_window_yend = height-1

    g_sccb->Write16(0xC91C, 0x0000);        //cam_stat_ae_initial_window_xstart = 0
    g_sccb->Write16(0xC91E, 0x0000);        //cam_stat_ae_initial_window_ystart = 0

    g_sccb->Write16(0xC920, width2/5-1);        //cam_stat_ae_initial_window_xend = width/5 - 1
    g_sccb->Write16(0xC922, height2/5-1);       //cam_stat_ae_initial_window_yend = height/5  - 1
#else
      g_sccb->Write16(0xC800, 0x00f0);// CAM_SENSOR_CFG_Y_ADDR_START - [0:00:07.530]
      g_sccb->Write16(0xC802, 0x0140);// CAM_SENSOR_CFG_X_ADDR_START - [0:00:07.535]
      g_sccb->Write16(0xC804, 0x02dd);// CAM_SENSOR_CFG_Y_ADDR_END - [0:00:07.540]
      g_sccb->Write16(0xC806, 0x03cd);// CAM_SENSOR_CFG_X_ADDR_END - [0:00:07.545]
      g_sccb->Write32(0xC808, 0x0206CC80);// CAM_SENSOR_CFG_PIXCLK - [0:00:07.550]
      g_sccb->Write16(0xC80C, 0x0001);// CAM_SENSOR_CFG_ROW_SPEED - [0:00:07.555]
      g_sccb->Write16(0xC80E, 0x01C3);// CAM_SENSOR_CFG_FINE_INTEG_TIME_MIN - [0:00:07.560]
      g_sccb->Write16(0xC810, 0x0273);// CAM_SENSOR_CFG_FINE_INTEG_TIME_MAX - [0:00:07.565]
      g_sccb->Write16(0xC812, 0x011e);// CAM_SENSOR_CFG_FRAME_LENGTH_LINES - [0:00:07.570]
      g_sccb->Write16(0xC814, 0x035e);// CAM_SENSOR_CFG_LINE_LENGTH_PCK - [0:00:07.575]
      g_sccb->Write16(0xC816, 0x00E0);// CAM_SENSOR_CFG_FINE_CORRECTION - [0:00:07.580]
      g_sccb->Write16(0xC818, 0x00f3);// CAM_SENSOR_CFG_CPIPE_LAST_ROW - [0:00:07.585]
      g_sccb->Write16(0xC826, 0x0020);// CAM_SENSOR_CFG_REG_0_DATA - [0:00:07.590]
      g_sccb->Write16(0xC834, 0x0330);// CAM_SENSOR_CONTROL_READ_MODE - [0:00:07.595]
      g_sccb->Write16(0xC854, 0x0000);// CAM_CROP_WINDOW_XOFFSET - [0:00:07.600]
      g_sccb->Write16(0xC856, 0x0000);// CAM_CROP_WINDOW_YOFFSET - [0:00:07.605]
      g_sccb->Write16(0xC858, 0x0140);// CAM_CROP_WINDOW_WIDTH - [0:00:07.610]
      g_sccb->Write16(0xC85A, 0x00f0);// CAM_CROP_WINDOW_HEIGHT - [0:00:07.615]
      g_sccb->Write8(0xC85C, 0x03);// CAM_CROP_CROPMODE - [0:00:07.620]
      g_sccb->Write16(0xC868, 0x0140);// CAM_OUTPUT_WIDTH - [0:00:07.625]
      g_sccb->Write16(0xC86A, 0x00f0);// CAM_OUTPUT_HEIGHT - [0:00:07.630]
      g_sccb->Write16(0xC86C, 0x0E10);// cam_output_format Processed Bayer
      g_sccb->Write8(0xC878, 0x00);// CAM_AET_AEMODE - [0:00:07.635]
      g_sccb->Write16(0xC88C, 0x44f5);// CAM_AET_MAX_FRAME_RATE - [0:00:07.640]
      g_sccb->Write16(0xC88E, 0x44f5);// CAM_AET_MIN_FRAME_RATE - [0:00:07.645]
      g_sccb->Write16(0xC914, 0x0000);// CAM_STAT_AWB_CLIP_WINDOW_XSTART - [0:00:07.650]
      g_sccb->Write16(0xC916, 0x0000);// CAM_STAT_AWB_CLIP_WINDOW_YSTART - [0:00:07.655]
      g_sccb->Write16(0xC918, 0x013f);// CAM_STAT_AWB_CLIP_WINDOW_XEND - [0:00:07.660]
      g_sccb->Write16(0xC91A, 0x00ef);// CAM_STAT_AWB_CLIP_WINDOW_YEND - [0:00:07.665]
      g_sccb->Write16(0xC91C, 0x0000);// CAM_STAT_AE_INITIAL_WINDOW_XSTART - [0:00:07.670]
      g_sccb->Write16(0xC91E, 0x0000);// CAM_STAT_AE_INITIAL_WINDOW_YSTART - [0:00:07.675]
      g_sccb->Write16(0xC920, 0x003f);// CAM_STAT_AE_INITIAL_WINDOW_XEND - [0:00:07.680]
      g_sccb->Write16(0xC922, 0x002f);// CAM_STAT_AE_INITIAL_WINDOW_YEND - [0:00:07.685]

#endif
    cam_sendCommand();

    return 0;
}


int32_t cam_getFrameChirp(const uint8_t &type, const uint16_t &xOffset, const uint16_t &yOffset, const uint16_t &xWidth, const uint16_t &yWidth, Chirp *chirp)
{
    return cam_getFrameChirpFlags(type, xOffset, yOffset, xWidth, yWidth, chirp, RENDER_FLAG_FLUSH, true);
}

int32_t cam_getFrameChirpFlags(const uint8_t &type, const uint16_t &xOffset, const uint16_t &yOffset, const uint16_t &xWidth, const uint16_t &yWidth, Chirp *chirp, uint8_t renderFlags, bool remote)
{
    int32_t result;
    int32_t len;
    uint8_t *frame = (uint8_t *)SRAM1_LOC + CAM_PREBUF_LEN;

    result = cam_getFrame(frame, SRAM1_SIZE-CAM_PREBUF_LEN, type, xOffset, yOffset, xWidth, yWidth);
    if (result<0)
        return result;
    
    if (remote) // remote calls have 4 more bytes...
    {
        frame -= CAM_FRAME_HEADER_LEN + 4; // +4 because of responseInt
        // fill buffer contents manually for return data
        len = Chirp::serialize(chirp, frame, SRAM1_SIZE, HTYPE(FOURCC('B','A','8','1')), HINT8(renderFlags), UINT16(xWidth), UINT16(yWidth), UINTS8_NO_COPY(xWidth*yWidth), END);
        if (len!=CAM_FRAME_HEADER_LEN+4)
            return -1;

        // tell chirp to use this buffer
        chirp->useBuffer(frame, CAM_FRAME_HEADER_LEN+4+xWidth*yWidth);
        return 0;
    }
    else
        return cam_sendFrame(chirp, xWidth, yWidth, renderFlags);
}


int32_t cam_sendFrame(Chirp *chirp, uint16_t xWidth, uint16_t yWidth, uint8_t renderFlags, uint32_t fourcc)
{
    int32_t len;

    uint8_t *frame = (uint8_t *)SRAM1_LOC + CAM_PREBUF_LEN - CAM_FRAME_HEADER_LEN;
    // fill buffer contents manually for return data 
    len = Chirp::serialize(chirp, frame, SRAM1_SIZE, HTYPE(fourcc), HINT8(renderFlags), UINT16(xWidth), UINT16(yWidth), UINTS8_NO_COPY(xWidth*yWidth), END);
    if (len!=CAM_FRAME_HEADER_LEN)
        return -1;
    
    // tell chirp to use this buffer
    chirp->useBuffer(frame, CAM_FRAME_HEADER_LEN+xWidth*yWidth); 

    return 0;
}

int32_t cam_setReg8(const uint16_t &reg, const uint8_t &value)
{
    g_sccb->Write8(reg, value);
    return 0;
}


int32_t cam_setReg16(const uint16_t &reg, const uint16_t &value)
{
    g_sccb->Write16(reg, value);
    return 0;
}


int32_t cam_setReg32(const uint16_t &reg, const uint32_t &value)
{
    g_sccb->Write32(reg, value);
    return 0;
}


int32_t cam_getReg8(const uint16_t &reg)
{
    return g_sccb->Read8(reg);
}


int32_t cam_getReg16(const uint16_t &reg)
{
    return g_sccb->Read16(reg);
}


int32_t cam_getReg32(const uint16_t &reg)
{
    return g_sccb->Read32(reg);
}

int32_t cam_getFramePeriod()
{
    // get frame period from M0, scale by 16 to get useconds
    return SM_OBJECT->frameTime<<4;
}


float cam_getFPS()
{
    float fps = 0.0f;
    
    uint32_t fp = cam_getFramePeriod();
    if (fp>0)
        fps = 1000000.0f/cam_getFramePeriod();
    return fps;
}


int32_t cam_getBlankTime()
{
    return SM_OBJECT->blankTime;
}



void cam_shadowCallback(const char *id, const uint8_t &val)
{
    if (strcmp(id, "Camera brightness")==0)
        cam_setBrightness(val);
    else if (strcmp(id, "Color saturation")==0)
        cam_setSaturation(val);
    else if (strcmp(id, "Min frames per second")==0)
        cam_setFramerate(val);
    else if (strcmp(id, "Flicker avoidance")==0)
        cam_setFlickerAvoidance(val);
    else if (strcmp(id, "Auto Exposure Correction")==0)
    {
        cam_setAEC(val);
        if (val==0)
        {
            delayus(50000); // wait a few frames so we can be sure the value is valid 
            g_aecValue = cam_getECV();
        }
    }
    else if (strcmp(id, "Auto White Balance")==0)
    {
        cam_setAWB(val);
        if (val==0)
            g_awbValue = cam_getWBV();
    } 
}

void cam_loadParams()
{
    int8_t progVideo = exec_getProgIndex("video");

    prm_add("Camera brightness", PRM_FLAG_SLIDER, PRM_PRIORITY_5-10, 
        "@c Tuning @m 0 @M 255 Sets the average brightness of the camera, can be between 0 and 255 (default " STRINGIFY(CAM_BRIGHTNESS_DEFAULT) ")", UINT8(CAM_BRIGHTNESS_DEFAULT), END);
    prm_setShadowCallback("Camera brightness", (ShadowCallback)cam_shadowCallback);

    prm_add("Auto Exposure Correction", PRM_FLAG_ADVANCED | PRM_FLAG_CHECKBOX, PRM_PRIORITY_1, 
        "@c Camera Enables/disables Auto Exposure Correction. (default enabled)", UINT8(1), END);
    prm_setShadowCallback("Auto Exposure Correction", (ShadowCallback)cam_shadowCallback);

    prm_add("AEC Value", PRM_FLAG_INTERNAL, PRM_PRIORITY_1,
        "", UINT32(0), END);

    if (progVideo>=0)
    {
        prm_add("Color saturation", PROG_FLAGS(progVideo) | PRM_FLAG_ADVANCED | PRM_FLAG_SLIDER, PRM_PRIORITY_5,  
            "@c Tuning @m 0 @M 255 Sets the color saturation, can be between 0 and 255 (default " STRINGIFY(CAM_SATURATION_DEFAULT+40) ")", UINT8(CAM_SATURATION_DEFAULT+40), END);
        prm_setShadowCallback("Color saturation", (ShadowCallback)cam_shadowCallback);
    }
                                                                           
    prm_add("Auto White Balance", PRM_FLAG_ADVANCED | PRM_FLAG_CHECKBOX, PRM_PRIORITY_1, 
        "@c Camera Enables/disables Auto White Balance. When this is set, AWB is enabled continuously. (default disabled)", UINT8(0), END);
    prm_setShadowCallback("Auto White Balance", (ShadowCallback)cam_shadowCallback);

    prm_add("Auto White Balance on power-up", PRM_FLAG_ADVANCED | PRM_FLAG_CHECKBOX, PRM_PRIORITY_1,
        "@c Camera Enables/disables Auto White Balance on power-up. When this is set, AWB is enabled only upon power-up. (default enabled)", UINT8(1), END);
    prm_setShadowCallback("Auto White Balance on power-up", (ShadowCallback)cam_shadowCallback);

    prm_add("Flicker avoidance", PRM_FLAG_ADVANCED | PRM_FLAG_CHECKBOX, PRM_PRIORITY_1,
        "@c Camera Enables/disables flicker avoidance with indoor lighting that flickers at 50 or 60Hz.  Note, may cause overexposure in outdoor setting when enabled (default disabled)", UINT8(0), END);
    prm_setShadowCallback("Flicker avoidance", (ShadowCallback)cam_shadowCallback);

    prm_add("AWB Value", PRM_FLAG_INTERNAL, PRM_PRIORITY_DEFAULT,
        "", UINT32(0), END);

    prm_add("Min frames per second", PRM_FLAG_SLIDER, PRM_PRIORITY_1, 
        "@c Camera @m 2 @M 61 Sets the minimum frames per second (framerate).  Note, adjusting this value lower will allow Pixy to capture frames with less noise and in less light. (default " STRINGIFY(CAM_FRAMERATE_DEFAULT) ")", UINT8(CAM_FRAMERATE_DEFAULT), END);
    prm_setShadowCallback("Min frames per second", (ShadowCallback)cam_shadowCallback);
    
    uint8_t brightness, aec, awb, awbp, fa, fps;
    uint32_t ecv, wbv;
    prm_get("Camera brightness", &brightness, END);
    prm_get("Auto Exposure Correction", &aec, END);
    prm_get("Auto White Balance", &awb, END);
    prm_get("Auto White Balance on power-up", &awbp, END);
    prm_get("Flicker avoidance", &fa);
    prm_get("Min frames per second", &fps);
    
    cam_setBrightness(brightness); 
    cam_setFlickerAvoidance(fa);
    cam_setFramerate(fps);
    
    delayus(50000); // wait a few frames 
    cam_setAEC(aec);
    if (!aec)
    {
        if (g_aecValue) // save queried ECV 
        {
            prm_set("AEC Value", UINT32(g_aecValue), END);
            g_aecValue = 0;
        }
        else
        {
            prm_get("AEC Value", &ecv, END); // grab saved ECV and set it
            cam_setECV(ecv);
        }
    }
    cam_setAWB(awb);
    if (!awb && !awbp)
    {
        if (g_awbValue) // save queried ECV 
        {
            prm_set("AWB Value", UINT32(g_awbValue), END);
            g_awbValue = 0;
        }
        else
        {
            prm_get("AWB Value", &wbv, END); // grab saved WBV and set it
            cam_setWBV(wbv);
        }
    }

}
