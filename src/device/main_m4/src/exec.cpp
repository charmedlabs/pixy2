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

#include <stdio.h>
#include <string.h>
#include "pixy_init.h"
#include "misc.h"
#include "exec.h"
#include "button.h"
#include "camera.h"
#include "conncomp.h"
#include "line.h"
#include "serial.h"
#include "rcservo.h"
#include "progpt.h"
#include "progblobs.h"
#include "progchase.h"
#include "progline.h"
#include "param.h"
#include "usblink.h"
#include "led.h"
#include "simplelist.h"


#define LOOP_STATE  1
//#define DEBUG_SERVO

static const ProcModule g_module[] =
{
	{
	"running",
	(ProcPtr)exec_running, 
	{END}, 
	"Is a program running?"
	"@r 1 if a program is running, 2 if running in \"forced\" state, 0 if not, also returns a status string"
	},
	{
	"stop",
	(ProcPtr)exec_stop, 
	{END}, 
	"Stop execution of the current program"
	"@r always returns 0"
	},
	{
	"run",
	(ProcPtr)exec_run, 
	{END}, 
	"Run the current program"
	"@r returns 0 if successful, -1 otherwise"
	},
	{
	"runProg",
	(ProcPtr)exec_runProg, 
	{CRP_UINT8, END}, 
	"Run the specified program"
	"@p program number"
	"@r returns currently running program index if successful, -1 otherwise"
	},
	{
	"runProgName",
	(ProcPtr)exec_runProgName, 
	{CRP_STRING, END}, 
	"Run the specified program name"
	"@p program name of program"
	"@r returns 0 if successful, -1 otherwise"
	},
	{
	"runProgDefault",
	(ProcPtr)exec_runProgDefault, 
	{END}, 
	"Run the default program"
	"@r returns 0 if successful, -1 otherwise"
	},
	{
	"runProgArg",
	(ProcPtr)exec_runProgArg, 
	{CRP_UINT8, CRP_UINT32, END}, 
	"Run the specified program with an argument"
	"@p index program index"
	"@p argument to be passed to program"
	"@r returns 0 if successful, -1 otherwise"
	},
	{
	"getProg",
	(ProcPtr)exec_getProg, 
	{CRP_UINT8, END}, 
	"Get the program name and description assocated with the index argument"
	"@p index program index"
	"@r returns 0 if valid index, 1 if index is the currently running program, -1 if index is out of range"
	},
	{
	"getProgIndex",
	(ProcPtr)exec_getProgIndex, 
	{CRP_STRING, END}, 
	"Get the program index given the program name"
	"@p name program name"
	"@r returns 0 if successful, -1 otherwise"
	},
	{
	"progs",
	(ProcPtr)exec_list, 
	{END}, 
	"List available programs"
	"@r always returns 0"
	},
	{
	"version",
	(ProcPtr)exec_version, 
	{END}, 
	"Get firmware version"
	"@r always returns 0 and an array of 6 uint16 values: the first 3 are major, minor, and build firmware versions, the last 3 are major, minor and build hardware versions"
	},
	{
	"versionType",
	(ProcPtr)exec_versionType, 
	{END}, 
	"Get firmware type"
	"@r always returns 0 and a null-terminated string that describes the type of firmware"
	},
	{
	"getAction",
	(ProcPtr)exec_getAction, 
	{CRP_UINT16, END}, 
	"Get the action scriptlet assocated with the index argument"
	"@p action index"
	"@r returns 0 if valid index, -1 if index is out of range"
	},
	{
	"getUID",
	(ProcPtr)exec_getUID, 
	{END}, 
	"Get the unique ID of this Pixy"
	"@r returns 32-bit unique ID"
	},
	{
	"getView",
	(ProcPtr)exec_getView, 
	{CRP_UINT16, END}, 
	"Get the view of the running program"
	"@p index view index"
	"@r returns 0 if valid index, 1 if index is current view, -1 if index is out of range"
	},
	{
	"setView",
	(ProcPtr)exec_setView, 
	{CRP_UINT16, END}, 
	"Set the view of the running program"
	"@p index view index"
	"@r returns 0 if successful, -1 otherwise, or if index is out of range"
	},	
	{
	"toggleLamp",
	(ProcPtr)exec_toggleLamp, 
	{END}, 
	"Toggle the white LEDs on and off"
	"@r returns 0 if turned off, 1 if turned on"
	},	
	{
	"printMC",
	(ProcPtr)exec_printMC, 
	{END}, 
	"Print manufacturing constants"
	"@r returns 0"
	},	
	END
};

static const ActionScriptlet g_actions[]=
{
	{
	"Toggle lamp", 
	"toggleLamp\n" 
	"run\n"
	}
};

uint8_t g_running = false;
uint8_t g_run = false;
uint8_t g_progIndex;
uint8_t g_runningProgIndex;

Prog *g_prog = NULL;
uint8_t g_defaultProgram;
uint8_t g_override = false;
char g_statusString[128];

int32_t g_execArg = 0;  // this arg mechanism is lame... should introduce an argv type mechanism 
uint8_t g_debug = 0;

static ChirpProc g_runM0 = -1;
static ChirpProc g_runningM0 = -1;
static ChirpProc g_stopM0 = -1;
static uint8_t g_progM0 = 0;
static uint8_t g_state = 0;

int16_t Prog::m_view;


ProgTableEntry ProgTableUtil::m_progTable[EXEC_MAX_PROGS];
uint8_t ProgTableUtil::m_progTableIndex = 0;

static void loadParams();

#define SDELAY 500000

#ifdef DEBUG_SERVO
void exec_servo()
{
	static uint8_t state = 0;
	static uint32_t timer;
	switch(state)
	{
		case 0:
		setTimer(&timer);
		rcs_setPos(0, 0);
		rcs_setPos(1, 0);
		state = 2;
		break;
			
		case 1:
		if (getTimer(timer)>SDELAY)
		{
			setTimer(&timer);
			rcs_setPos(0, 0);
			rcs_setPos(1, 0);
			state = 2;
		}
		break;
		
		
		case 2:
		if (getTimer(timer)>SDELAY)
		{
			setTimer(&timer);
			rcs_setPos(0, 1000);
			rcs_setPos(1, 1000);
			state = 3;
		}
		break;
		
		case 3:
		if (getTimer(timer)>SDELAY)
		{
			setTimer(&timer);
			rcs_setPos(0, 0);
			rcs_setPos(1, 0);
			state = 4;
		}
		break;

		case 4:
		if (getTimer(timer)>SDELAY)
		{
			setTimer(&timer);
			rcs_setPos(0, 500);
			rcs_setPos(1, 500);
			state = 5;
		}
		break;

		case 5:
		if (getTimer(timer)>SDELAY)
		{
			setTimer(&timer);
			rcs_setPos(0, 1000);
			rcs_setPos(1, 1000);
			state = 1;
		}
		break;
	}
}
#endif

int compProgNames(const void *a, const void *b)
{
	const ProgTableEntry *ba=(const ProgTableEntry *)a, *bb=(const ProgTableEntry *)b;
	
	return strcmp(ba->m_name, bb->m_name);
}

int exec_init(Chirp *chirp)
{
	// sort program table by program name so the results are predictable
	qsort(ProgTableUtil::m_progTable, ProgTableUtil::m_progTableIndex, sizeof(ProgTableEntry), compProgNames);
	
	chirp->registerModule(g_module);

	g_runM0 = g_chirpM0->getProc("run", NULL);
	g_runningM0 = g_chirpM0->getProc("running", NULL);
	g_stopM0 = g_chirpM0->getProc("stop", NULL);	

	loadParams();		

	return 0;	
}


uint32_t exec_running(Chirp *chirp)
{
	// if there is no status string, we'll make a relevant one based on the state and the program name
	if (g_statusString[0]=='\0')
	{
		char statusString[128];
		
		strcpy(statusString, ProgTableUtil::m_progTable[g_progIndex].m_name);
		if (g_running)
		{
			// add frames per second to status message, but only if we're running (and period isn't zero)
			float fps = cam_getFPS();
				sprintf(statusString + strlen(statusString), " running %.2f fps", fps);
		}
		else
			strcat(statusString, " stopped");
	
		if (chirp)
			CRP_RETURN(chirp, STRING(statusString), END);
	}
	else if (chirp)
		CRP_RETURN(chirp, STRING(g_statusString), END);
	if (g_running)
		return g_running + g_override; 
	else
		return 0;
}

int32_t exec_stop()
{
	g_run = 0;
	return 0;
}

int32_t exec_run()
{
	g_run = true;
	g_running = true;		
	return 0;
}

int32_t exec_runProg(const uint8_t &progIndex, Chirp *chirp)
{		
	exec_run();
	return exec_setProgIndex(progIndex);
}

int32_t exec_runProgName(const char *name, Chirp *chirp)
{
	int32_t res;
	
	res = exec_getProgIndex(name);
	
	if (res<0)
		return res;
	else
		return (exec_runProg(res)==res);	
}

int32_t exec_runProgDefault(Chirp *chirp)
{
	return exec_runProg(g_defaultProgram);
}

int32_t exec_runProgArg(const uint8_t &progIndex, const int32_t &arg, Chirp *chirp)
{
	int32_t res = exec_runProg(progIndex, chirp);
	if (res<0)
		return res;

	g_execArg = arg;
	return 0;
}

int32_t exec_getProg(const uint8_t &progIndex, Chirp *chirp)
{	
	if (progIndex>=ProgTableUtil::m_progTableIndex)
		return -1;
	
	if (chirp)
		CRP_RETURN(chirp, STRING(ProgTableUtil::m_progTable[progIndex].m_name), STRING(ProgTableUtil::m_progTable[progIndex].m_desc), END);
	
	return progIndex==g_progIndex; // return 1 if they match,0 otherwise
}

int32_t exec_getProgIndex(const char *progName, Chirp *chirp)
{
	uint8_t i;
	for (i=0; i<ProgTableUtil::m_progTableIndex; i++)
	{
		if (strncmp(ProgTableUtil::m_progTable[i].m_name, progName, strlen(progName))==0)
			return i;
	}
	return -1;
}

int32_t exec_setProgIndex(uint8_t progIndex)
{
	if (progIndex>=ProgTableUtil::m_progTableIndex)
		return -1;

	g_progIndex = progIndex;

	g_execArg = 0;
	
	return g_runningProgIndex;
}

int32_t exec_list()
{
	uint8_t i;
	for (i=0; i<ProgTableUtil::m_progTableIndex; i++)
		cprintf(0, "%d: %s, %s\n", i, ProgTableUtil::m_progTable[i].m_name, ProgTableUtil::m_progTable[i].m_desc);

 	return 0;
}

int32_t exec_version(Chirp *chirp)
{
	uint32_t res;
	
	uint16_t ver[] = {FW_MAJOR_VER, FW_MINOR_VER, FW_BUILD_VER, 0, 0, 0};
	
	res = exec_getHardwareVersion(ver+3);

	if (chirp)
		CRP_RETURN(chirp, UINTS16(sizeof(ver)/sizeof(uint16_t), ver), END);

	return res;
}

int32_t exec_versionType(Chirp *chirp)
{
	if (chirp)
		CRP_RETURN(chirp, STRING(FW_TYPE), END);

	return 0;
}


int32_t exec_getAction(const uint16_t &index, Chirp *chirp)
{
	int res=-1;
	uint16_t n;
	const char *name=NULL, *scriptlet=NULL;
	uint16_t index2 = index; //index-n;
	
	if (g_prog)
	{
			
		res = g_prog->getAction(index2, &name, &scriptlet);
		
		// if it fails, find how many actions there are
		if (res<0)
		{		
			for (index2=0; true; index2++)
			{		
				if (g_prog->getAction(index2, &name, &scriptlet)<0)
					break;
			}
			// calc index into global index
			index2 -= index;
		}
	}
	if (res<0)
	{
		n = sizeof(g_actions)/sizeof(ActionScriptlet);

		// check exec-based actions first
		if (index2<n)
		{
			name = g_actions[index2].action;
			scriptlet = g_actions[index2].scriptlet;
			res = 0;
		}
	}
	// then add program-based actions

	if (chirp)
		CRP_RETURN(chirp, STRING(name), STRING(scriptlet), END);
	return res;
}


uint32_t exec_getUID()
{
	uint32_t val;
	volatile uint32_t *mem;

	for (val=0, mem=(volatile uint32_t *)0x40045000; mem<(volatile uint32_t *)0x40045010; mem++)
		val += *mem;

	return val;
}

int32_t exec_getView(const uint16_t &index, Chirp *chirp)
{
	int32_t res;
	const char *string;
	
	if (g_prog)
	{
		res = g_prog->getView(index, &string);
		if (res<0)
			return res; // return early
		if (chirp)
			CRP_RETURN(chirp, STRING(string), END);
		return res;
	}
	return -1;
}

int32_t exec_setView(const uint16_t &index)
{
	int32_t res=0;
	
	if (g_prog)
		res = g_prog->setView(index);			
	
	if (res>=0)
		Prog::m_view = index; 

	return res;
}

int32_t exec_toggleLamp()
{
	int state = led_toggleLamp();
	cc_setLEDOverride(state);
	return state;
}

int32_t exec_printMC()
{
	cprintf(0, "0x%x 0x%x\n", *(uint32_t *)0x40045034, *(uint32_t *)0x40045038);
	return 0;
}

int exec_runM0(uint8_t prog)
{
	int responseInt;

	g_chirpM0->callSync(g_runM0, UINT8(prog), END_OUT_ARGS,
		&responseInt, END_IN_ARGS);

	g_progM0 = prog;

	return responseInt;
}


int exec_stopM0()
{
	int responseInt;

	g_chirpM0->callSync(g_stopM0, END_OUT_ARGS,
		&responseInt, END_IN_ARGS);

	while(exec_runningM0());
	
	return responseInt;
}

uint8_t exec_runningM0()
{
	uint32_t responseInt;

	g_chirpM0->callSync(g_runningM0, END_OUT_ARGS,
		&responseInt, END_IN_ARGS);

	return responseInt;
}


void exec_periodic()
{	
	periodic();
#ifdef DEBUG_SERVO
	exec_servo();
#endif
	if (prm_dirty())
		exec_loadParams();
}

void exec_select()
{
	uint8_t select;
	
	prm_get("Program select on power-up", &select, END);
	g_progIndex = g_defaultProgram;

	// select using button state machine
	if (select)
	{
		selectProgram(ProgTableUtil::m_progTableIndex, &g_progIndex);
		g_defaultProgram = g_progIndex;
	}

	exec_runProg(g_progIndex);
}

static void loadParams()
{
	int i;
	char buf[256], buf2[64];

	// create program menu
	strcpy(buf, "Selects the program number that's run upon power-up. @c Expert");
	for (i=0; i<ProgTableUtil::m_progTableIndex; i++)
	{
		sprintf(buf2, " @s %d=%s", i, ProgTableUtil::m_progTable[i].m_name);
		strcat(buf, buf2);
	} 

	// exec's params added here
	prm_add("Default program", 0, PRM_PRIORITY_4-10, buf, UINT8(0), END);
	prm_add("Program select on power-up", PRM_FLAG_CHECKBOX, PRM_PRIORITY_4-10,
		"@c Expert Allows you to choose program other than default program upon power-up by button press sequence (default disabled)" , UINT8(0), END);
	prm_add("Debug", 0, PRM_PRIORITY_4-11, 
		"@c Expert Sets the debug level for the firmware. (default 0)", UINT8(0), END);
	
	prm_get("Debug", &g_debug, END);
	prm_get("Default program", &g_defaultProgram, END);
	
}

void exec_loadParams()
{
 	cc_loadParams();
	ser_loadParams();
	cam_loadParams();
#ifndef LEGO
	rcs_loadParams();
#endif
	line_loadParams(exec_getProgIndex(PROG_NAME_LINE));
	loadParams(); // local
}

int8_t exec_progIndex()
{
	return g_progIndex;
}


//#define FOO

void exec_mainLoop()
{
	bool prevConnected = false;
	bool connected, gui;
	uint8_t saveIndex;
	
#ifdef FOO
	g_state = 4;
#else
	g_state = 0;
#endif

#ifndef FOO
	exec_select();
#endif

	// enable USB *after* we've initialized chirp, all modules and selected the program
	USBLink *usbLink = new USBLink;
	g_chirpUsb->setLink(usbLink);

	g_runningProgIndex = EXEC_MAX_PROGS;
	
	while(1)
	{
		connected = g_chirpUsb->connected();
		gui = connected && USB_Configuration && g_chirpUsb->hinformer();
		
		exec_periodic();

		switch (g_state)
		{
		case 0:	// setup state
			led_set(0);  // turn off any stray led 
			saveIndex = g_progIndex; // save off program index to eliminate any race conditions with ISRs
		
			if (g_runningProgIndex!=saveIndex || g_prog==NULL) // new program
			{
				// if we're running a program already, exit
				exec_progExit(); 
				// reset shadow parameters, invalidate current view, ledOverride but only if we're switching programs
				if (g_runningProgIndex!=saveIndex)
				{
					// reset shadows
					prm_resetShadows();
					Prog::m_view = -1; 
					cc_setLEDOverride(false);
				}
				if (exec_progSetup(saveIndex)<0) // then run				
					g_state = 3; // stop state
				else 
				{
					if (g_runningProgIndex!=saveIndex)
					{
						exec_sendEvent(g_chirpUsb, EVT_PROG_CHANGE);
						g_runningProgIndex = saveIndex; // update g_runningProgIndex and arg -- we're now transitioned
					}
					g_state = LOOP_STATE; // loop state
				}
			}
			else
				g_state = LOOP_STATE;
			break;

		case LOOP_STATE:  // loop state, program is running
			ser_setReady(); // we're ready
			ser_update(); // update serial while we're running
			if (!g_run)
				g_state = 3; // stop state
			// if g_program has been changed out from under us...
			else if (g_progIndex!=g_runningProgIndex)
				g_state = 0; // transition out of this program and arg, to the next
			else if (exec_progLoop(gui)<0)
				g_state = 3; // stop state
			else if (prevConnected && !connected) // if we disconnect from pixymon, revert back to default program
			{
				exec_runProg(g_defaultProgram); // run default program
				g_state = 0; // setup state
			}
			break;

		case 3:	// stop state
			led_set(0);  // turn off any stray led 
			// set variable to indicate we've stopped
			exec_progExit(); // we should exit because we may need to free up memory to write parameters
			g_run = false;
			g_running = false;
			g_state = 4; // wait for run state
			break;

		case 4:	// wait for run state
#ifndef FOO
			if (g_run) 
			{
				exec_run();
				g_state = 0; // back to setup state
			}
			else if (!connected || !USB_Configuration) // if we disconnect from pixy or unplug cable, revert back to default program
			{
				exec_runProg(g_defaultProgram); // run default program
				g_state = 0;	// back to setup state
			}
#endif
			break;
				
		default:
			g_state = 3; // stop state				
		}

		prevConnected = connected;
	}
}

int exec_progSetup(uint8_t progIndex)
{
	if (progIndex<=ProgTableUtil::m_progTableIndex && g_prog==NULL)
	{
		if (g_debug&EXEC_DEBUG_MEMORY_CHECK)
		{	
			cprintf(0, "Setup:\n");	
			exec_testMemory();
		}
		
		g_prog = (*ProgTableUtil::m_progTable[progIndex].m_create)(progIndex);
		if (g_prog==NULL) // out of memory!
			return -2;

		return 0;
	}
	return -1;
}

int exec_progLoop(bool gui)
{
	int res; 
	
	if (g_prog==NULL)
		return -1;

	// reset status string
	g_statusString[0] = '\0';
	// loop
	if (gui)
		res = g_prog->loop(g_statusString);
	else
		res = g_prog->loop(NULL); // indicate that there is no gui to return data to

	if (g_debug&EXEC_DEBUG_MEMORY_CHECK)
		exec_testMemory();
	
	// override if result is nonzero -- the override will cause pixymon to lock out play/stop buttons, etc.
	if (res>0)
		g_override = true;
	else
		g_override = false;
	
	return res;
}

int exec_progExit()
{
	if (g_prog)
	{
		delete g_prog;
		g_prog = NULL;
		if (g_debug&EXEC_DEBUG_MEMORY_CHECK)
		{	
			cprintf(0, "Exit:\n");	
			exec_testMemory();
		}
		return 0;
	}
	return -1;
}

int exec_changeProg(uint8_t type)
{
	int i;
	
	// If the current program is capable of handling, return success
	if (ProgTableUtil::m_progTable[g_progIndex].m_minType<=type && type<=ProgTableUtil::m_progTable[g_progIndex].m_maxType)
		return 0;
	
	// If not, find the correct program
	for (i=0; i<ProgTableUtil::m_progTableIndex; i++)
	{
		if (ProgTableUtil::m_progTable[i].m_minType<=type && type<=ProgTableUtil::m_progTable[i].m_maxType)
		{
			g_progIndex = i;
			return 0;
		}
	}
	
	return -1;
}

void exec_progPacket(uint8_t type, const uint8_t *data, uint8_t len, bool checksum)
{
	// only valid when we're in the loop state, ie after we've already called
	// setup for that program, otherwise there might be a race
	// condition between initialization of program and calling of progPacket, which 
	// is interrupt-driven.
	
	if (g_state==LOOP_STATE)
	{
		if (g_override)
		{
			ser_sendError(SER_ERROR_BUTTON_OVERRIDE, checksum);
			return;
		}
		else if (g_prog)
		{
			if (exec_changeProg(type)<0)
			{
				ser_sendError(SER_ERROR_TYPE_UNSUPPORTED, checksum);
				return;
			}
			if (g_progIndex!=g_runningProgIndex)
			{
				ser_sendError(SER_ERROR_PROG_CHANGING, checksum);
				return;
			}						
			if (g_prog->packet(type, data, len, checksum)<0)
				ser_sendError(SER_ERROR_TYPE_UNSUPPORTED, checksum);
			return;
		}
	}
	else
		ser_sendError(SER_ERROR_PROG_CHANGING, checksum);
}


int exec_progResolution(uint8_t type, bool checksum)
{
	if (g_prog)
	{
		uint8_t *txData;
		ser_getTx(&txData);
		g_prog->getResolution((uint16_t *)txData, (uint16_t *)(txData + 2), type);
		ser_setTx(SER_TYPE_RESPONSE_RESOLUTION, 2*sizeof(uint16_t), checksum);	
		
		return 0;
	}
	return -1;
}


uint8_t exec_pauseM0()
{
	uint8_t running;
	running = exec_runningM0();
	if (running)
		exec_stopM0();
	return running;
}

void exec_resumeM0()
{
	g_qqueue->flush();
	exec_runM0(g_progM0);
}

void exec_sendEvent(Chirp *chirp, uint32_t event)
{
	if (chirp)
		CRP_SEND_XDATA(chirp, HTYPE(FOURCC('E','V','T','1')), INT32(event));
}

int exec_getHardwareVersion(uint16_t *version)
{
	uint32_t ver = *(uint32_t *)0x40045038;
	// check for tag
	if ((ver&0xffff0000)!=0xc1ab0000)
		return -1;
	
	ver &= 0xffff; // remove tag
	version[0] = ver>>12;
	version[1] = (ver>>8)&0x0f;
	version[2] = ver&0xff;
	return 0;
}

void exec_testMemory()
{
	SimpleList<uint32_t> list;	
	int n=0;
	
	while(1)
	{
		if (list.add(n)==NULL)
			break;
		n++;
	}
	cprintf(0, "mem %d\n", n);
}

