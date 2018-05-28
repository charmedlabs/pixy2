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

SPIFI_HANDLE_T *g_spifi = NULL;
uint32_t g_flashSize = 0;
uint32_t *g_tempFlashmem;

int32_t flash_init()
{
	uint32_t memSize;
	
	__disable_irq();
	/* Initialize LPCSPIFILIB library, reset the interface */
	spifiInit(LPC_SPIFI_BASE, true);

	/* register support for the family(s) we may want to work with
	     (only 1 is required) */
	spifiRegisterFamily(SPIFI_REG_FAMILY_SpansionS25FL1);
	
	/* Get required memory for detected device, this may vary per device family */
	memSize = spifiGetHandleMemSize(LPC_SPIFI_BASE);
	if (memSize==0)
		return -1;
	g_tempFlashmem = new uint32_t[memSize/sizeof(uint32_t)];

	/* Initialize and detect a device and get device context */
	g_spifi = spifiInitDevice(g_tempFlashmem, memSize, LPC_SPIFI_BASE, FLASH_BEGIN);
	if (g_spifi==NULL)
		return -2;

	g_flashSize = spifiDevGetInfo(g_spifi, SPIFI_INFO_DEVSIZE);

	spifiDevSetMemMode(g_spifi, true);
	__enable_irq();

	return 0;
}

int32_t flash_erase(uint32_t addr, uint32_t len)
{
	SPIFI_ERR_T res=SPIFI_ERR_NONE;
	uint32_t a; 

	printf("flash erase %x %x\n", addr, len);
	__disable_irq();
	for (a=FLASH_SECTOR_MASK(addr); a<addr+len; a+=FLASH_SECTOR_SIZE)
	{
		res = spifiDevEraseSubBlock(g_spifi, spifiGetSubBlockFromAddr(g_spifi, a));
		if (res!=SPIFI_ERR_NONE)
			goto end;
	}
end:
	spifiDevSetMemMode(g_spifi, true);
	__enable_irq();
	return -res;
}

int32_t flash_program(uint32_t addr, const uint8_t *data, uint32_t len)
{
	SPIFI_ERR_T res;
	uint32_t ac, lc;

	printf("flash program %x %x\n", addr, len);
	__disable_irq();	
	ac = addr&(FLASH_PAGE_SIZE-1);
	if (ac)
	{
		lc = FLASH_PAGE_SIZE-ac;
		res = spifiProgram(g_spifi, addr, (uint32_t *)data, lc);
		if (res!=SPIFI_ERR_NONE)
			goto end;
		res = spifiProgram(g_spifi, addr+lc, (uint32_t *)(data+lc), len-lc);
		if (res!=SPIFI_ERR_NONE)
			goto end;
	}
	else 
		res = spifiProgram(g_spifi, addr, (uint32_t *)data, len);

	end:
	spifiDevSetMemMode(g_spifi, true);
	__enable_irq();
	return -res;

}
