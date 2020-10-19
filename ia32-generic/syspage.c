/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * Syspage
 *
 * Copyright 2020 Phoenix Systems
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#include "../low.h"
#include "../plostd.h"
#include "../syspage.h"




/* Initialization function */

void syspage_init(void)
{

}



/* Syspage's location functions */

int syspage_setAddress(void *addr)
{

}


void *syspage_getAddress(void)
{
	return (void *)0x0;
}


/* General functions */

void syspage_save(void)
{

}


void syspage_show(void)
{

}



/* Map's functions */

int syspage_addmap(const char *name, void *start, void *end, u32 attr)
{
	return 0;
}


int syspage_getMapTop(const char *map, void **addr)
{
	return 0;
}


int syspage_getFreeSize(const char *map, u32 *sz)
{
	return 0;
}


int syspage_write2Map(const char *map, const u8 *buff, u32 len)
{
	return 0;
}


void syspage_addEntries(u32 start, u32 sz)
{

}



/* Program's functions */

int syspage_addProg(void *start, void *end, const char *imap, const char *dmap, const char *name)
{
	return 0;
}



/* Setting kernel's data */

void syspage_setKernelText(void *addr, u32 size)
{

}


void syspage_setKernelBss(void *addr, u32 size)
{

}


void syspage_setKernelData(void *addr, u32 size)
{

}
