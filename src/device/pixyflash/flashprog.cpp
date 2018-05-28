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
#include "pixy_init.h"
#include "flash.h"
#include "flashprog.h"

uint8_t g_resetFlag = 0;

uint8_t *g_eraseMap;

static const ProcModule g_module[] =
{
	{
	"flash_sectorSize",
	(ProcPtr)flash_sectorSize, 
	{END}, 
	"Get size of flash sector"
	"@r sector size in bytes"
	},
	{
	"flash_program",
	(ProcPtr)flash_program2, 
	{CRP_INT32, CRP_INTS8, END}, 
	"Program flash (including erasing and verifying)."
	"@p addr destination address"
	"@p data programming data"
	"@p len length of data"
	"@r voltage in millivolts"
	},
	{
	"flash_reset",
	(ProcPtr)flash_reset, 
	{END}, 
	"Reset processor (execute program)."
	"@r always returns 0"
	},
	{
	"getHardwareVersion",
	(ProcPtr)getHardwareVersion, 
	{END}, 
	"Get hardware version"
	"@r returns 0 if successful, <0 otherwise"
	},
	END
};	


void flash_init2()
{
	uint32_t mapsize;
	
	flash_init();

  g_chirpUsb->registerModule(g_module);

	mapsize = FLASH_SIZE/FLASH_SECTOR_SIZE;
	 
	g_eraseMap = new uint8_t[mapsize];
	memset((void *)g_eraseMap, 0, mapsize);
}

int32_t erase(uint32_t addr)
{
	int32_t res;
	
	// if we've already erased, just return 
	printf("erase %x\n", addr);
	if (g_eraseMap[(addr-FLASH_BEGIN)/FLASH_SECTOR_SIZE]==1)
		return 0;

	res = flash_erase(addr, 1);
	printf("erased %d %x %x %x\n", res, *(uint8_t *)(addr+0), *(uint8_t *)(addr+1), *(uint8_t *)(addr+0xfff));
	// update map
	g_eraseMap[(addr-FLASH_BEGIN)/FLASH_SECTOR_SIZE] = 1;

	return res;
}


uint32_t flash_sectorSize()
{
	return FLASH_SECTOR_SIZE; 
}

int32_t flash_program2(const uint32_t &addr, const uint32_t &len, const uint8_t *data)
{
	uint32_t i;

	printf("program %x %x\n", addr, len);

	// check range
	if (addr<FLASH_BEGIN || addr>FLASH_END || addr+len>FLASH_END)
		return -1;

	// erase all sectors spanned by this segment
	for (i=FLASH_SECTOR_MASK(addr); i<addr+len; i+=FLASH_SECTOR_SIZE)
	{
		if (erase(i)<0)
			return -3;
	}
	
	if (flash_program(addr, data, len)<0)
		return -2;

	printf("programmed\n");

	// verify
	for (i=0; i<len; i++)
	{
		if (*(uint8_t *)(addr+i) != data[i])
		{
			printf("error %x %x %x %x\n", addr+i, len, *(uint8_t *)(addr+i), data[i]);
			return -4;
		}
	}

	return 0;
}

#define PRM_ALLOCATED_LEN 			(FLASH_SECTOR_SIZE*8) // 8 sectors
#define PRM_FLASH_LOC	  			(FLASH_BEGIN + FLASH_SIZE - PRM_ALLOCATED_LEN)  // last sectors

int32_t flash_reset()
{
	// erase parameter memory to avoid problems with rogue parameters
	flash_erase(PRM_FLASH_LOC, PRM_ALLOCATED_LEN);
	printf("reset\n");
	g_resetFlag = 1;

	return 0;
}

int32_t getHardwareVersion(Chirp *chirp)
{
	uint16_t version[3];
	
	uint32_t ver = *(uint32_t *)0x40045038;
	// check for tag
	if ((ver&0xffff0000)!=0xc1ab0000)
		return -1;
	
	ver &= 0xffff; // remove tag
	version[0] = ver>>12;
	version[1] = (ver>>8)&0x0f;
	version[2] = ver&0xff;
	
	if (chirp)
		CRP_RETURN(chirp, UINTS16(3, version), END);

	return 0;
	
}


