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

#ifndef _EXEC_H
#define _EXEC_H

#include <new>
#include "chirp.hpp"
#include "debug_frmwrk.h"
#include "cameravals.h"

//#define LEGO

#define FW_MAJOR_VER		      3
#define FW_MINOR_VER		      0
#define FW_BUILD_VER		      18
#ifdef LEGO 
#define FW_TYPE                   "LEGO"
#else
#define FW_TYPE                   "general"
#endif

#define EXEC_MAX_PROGS            7
#define EXEC_DEBUG_MEMORY_CHECK   0x80

#define REGISTER_PROG(progType, name, desc, minType, maxType) \
    Prog *create ## progType(uint8_t progIndex) { \
        return new (std::nothrow) progType(progIndex); } \
    ProgTableUtil g_register ## progType(name, desc, minType, maxType, create ## progType);


class Prog 
{
public:
	virtual ~Prog()
	{
	}
	virtual int loop(char *status) = 0; // if status is null, no gui monitor (PixyMon) is present
	virtual int getView(uint16_t index, const char **name)
	{
		if (index==0)
		{
			*name = "Default";
			return 1;
		}
		return -1;
	};
	virtual int setView(uint16_t index)
	{
		if (index==0) // default implementation is single-view
			return 0;
		return -1;
	}
	virtual int getAction(uint16_t index, const char **name, const char **scriptlet)
	{
		return -1;
	}
	virtual void getResolution(uint16_t *width, uint16_t *height, uint8_t type)
	{
		*width = CAM_RES2_WIDTH;
		*height = CAM_RES2_HEIGHT;
	}
	virtual int packet(uint8_t type, const uint8_t *data, uint8_t len, bool checksum)
	{
		return -1;
	}
	//virtual int loadParams(uint8_t progIndex)
	//{
	//	return 0;
	//}
	static int16_t m_view;
};

struct ProgTableEntry
{
	const char *m_name;
	const char *m_desc;
	uint8_t m_minType;
	uint8_t m_maxType;
	Prog *(*m_create)(uint8_t progIndex);
};
		
struct ProgTableUtil
{
    ProgTableUtil(const char *name, const char *desc, uint8_t minType, uint8_t maxType, Prog *(*create)(uint8_t progIndex))
    {
		ProgTableEntry *entry = m_progTable + m_progTableIndex++;
		entry->m_name = name;
		entry->m_desc = desc;
		entry->m_minType = minType;
		entry->m_maxType = maxType;
		entry->m_create = create;
    }

    static ProgTableEntry m_progTable[EXEC_MAX_PROGS];
    static uint8_t m_progTableIndex;
};


struct ActionScriptlet
{
	const char *action;
	const char *scriptlet;
};


void exec_mainLoop();
int exec_init(Chirp *chirp);
void exec_select();

int exec_progSetup(uint8_t progIndex);
int exec_progLoop(bool gui);
int exec_progExit();
void exec_progPacket(uint8_t type, const uint8_t *data, uint8_t len, bool checksum);
int exec_progResolution(uint8_t type, bool checksum);
int exec_changeProg(uint8_t type);

// Chirp functions
uint32_t exec_running(Chirp *chirp=NULL);
int32_t exec_stop();
int32_t exec_run();
int32_t exec_runProg(const uint8_t &progIndex, Chirp *chirp=NULL);
int32_t exec_runProgName(const char *name, Chirp *chirp=NULL);
int32_t exec_runProgDefault(Chirp *chirp=NULL);
int32_t exec_runProgArg(const uint8_t &progIndex, const int32_t &arg, Chirp *chirp=NULL);
int32_t exec_getProg(const uint8_t &progIndex, Chirp *chirp=NULL);
int32_t exec_getProgIndex(const char *progName, Chirp *chirp=NULL);
int32_t exec_setProgIndex(uint8_t progIndex);
int32_t exec_list();
int32_t exec_version(Chirp *chirp=NULL);
int32_t exec_versionType(Chirp *chirp=NULL);
int32_t exec_getAction(const uint16_t &index, Chirp *chirp=NULL);
uint32_t exec_getUID();
int32_t exec_getView(const uint16_t &index, Chirp *chirp=NULL);
int32_t exec_setView(const uint16_t &index);
int32_t exec_toggleLamp();
int32_t exec_printMC();

int8_t exec_progIndex();
void exec_loadParams();
void exec_sendEvent(Chirp *chirp, uint32_t event);
int exec_runM0(uint8_t prog);
int exec_stopM0();
uint8_t exec_pauseM0();
void exec_resumeM0();
uint8_t exec_runningM0();
void exec_periodic();

int exec_getHardwareVersion(uint16_t *version);
void exec_testMemory();

extern int32_t g_execArg; 

#endif
