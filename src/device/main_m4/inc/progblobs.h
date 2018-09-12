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

#ifndef _PROGBLOBS_H
#define _PROGBLOBS_H

#include "exec.h"

#define SYNC_SERVO                 0x00ff
#define SYNC_CAM_BRIGHTNESS        0x00fe
#define SYNC_SET_LED               0x00fd

#define PROG_BLOBS_MIN_TYPE        0x20
#define PROG_BLOBS_MAX_TYPE        0x2f

#define TYPE_REQUEST_GETBLOBS      0x20
#define TYPE_RESPONSE_GETBLOBS     0x21


#define PROG_NAME_BLOBS            "color_connected_components"

// All of ProgBlobs is static because we expect ProgPt is going to be calling directly into it.
// We're only using the vtable...
class ProgBlobs : public Prog
{
public:
	ProgBlobs(uint8_t progIndex)
	{
		staticInit(progIndex);
	}
	virtual ~ProgBlobs()
	{
		staticExit();
	}
	
	virtual int loop(char *status)
	{
		return staticLoop(status);
	}
	virtual int getView(uint16_t index, const char **name)
	{
		return staticGetView(index, name);
	}
	virtual int setView(uint16_t index)
	{
		return staticSetView(index);
	}
	virtual int getAction(uint16_t index, const char **name, const char **scriptlet)
	{
		return staticGetAction(index, name, scriptlet);
	}
	virtual int packet(uint8_t type, const uint8_t *data, uint8_t len, bool checksum)
	{
		return staticPacket(type, data, len, checksum);
	}

	static void staticInit(uint8_t progIndex);
	static void staticExit();
	
	static int staticLoop(char *status);
	static int staticGetView(uint16_t index, const char **name);
	static int staticSetView(uint16_t index);
	static int staticGetAction(uint16_t index, const char **name, const char **scriptlet);
	static int staticPacket(uint8_t type, const uint8_t *data, uint8_t len, bool checksum);

	static bool handleButton(char *status);

private:
	static void handleButtonStatus(char *str);
	static void resetButtonState();
	static void scaleLED(uint32_t r, uint32_t g, uint32_t b, uint32_t n);
	static void ledPipe();
	static void setSignature();


	static uint8_t m_state;
	static void handleRecv();
	static void blobsAssemble(uint8_t sigmap, uint8_t n, bool checksum);
	static const char *m_views[];
	static const ActionScriptlet m_actions[];

	static uint32_t m_timer;
	static uint8_t m_buttonState;
	static uint8_t m_index;
	static bool m_ledPipe;
	static uint8_t renderState;
};

#endif
