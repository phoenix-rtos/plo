/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * Syspage
 *
 * Copyright 2020 Phoenix Systems
 * Authors: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "imxrt.h"

#include "../low.h"
#include "../plostd.h"
#include "../syspage.h"

#define MAX_PROGRAMS_NB         32
#define MAX_MAPS_NB             16
#define MAX_ARGS_SIZE           256
#define MAX_ENTRIES_NB          6     /* 3 of kernel's sections, 2 of plo's sections and syspage */

#define MAX_SYSPAGE_SIZE     (sizeof(syspage_t) + MAX_ARGS_SIZE * sizeof(char) + MAX_PROGRAMS_NB * sizeof(syspage_program_t) + MAX_MAPS_NB * sizeof(syspage_map_t))


#pragma pack(push, 1)

typedef struct _syspage_map_t {
	u32 start;
	u32 end;
	u32 attr;

	u8 id;
	char name[8];
} syspage_map_t;


typedef struct syspage_program_t {
	u32 start;
	u32 end;

	u8 dmap;
	u8 imap;

	char cmdline[16];
} syspage_program_t;


typedef struct _syspage_t {
	struct {
		void *text;
		u32 textsz;

		void *data;
		u32 datasz;

		void *bss;
		u32 bsssz;
	} kernel;

	u32 syspagesz;

	char *arg;

	u32 progssz;
	syspage_program_t *progs;

	u32 mapssz;
	syspage_map_t *maps;
} syspage_t;

#pragma pack(pop)



typedef	struct {
	u32 start;
	u32 end;
} mapt_entry_t;


struct {
	syspage_t *syspage;

	u16 argCnt;
	char args[MAX_ARGS_SIZE];

	u32 progsCnt;
	syspage_program_t progs[MAX_PROGRAMS_NB];

	u32 mapsCnt;
	struct {
		mapt_entry_t entry[MAX_ENTRIES_NB];

		u32 top;
		syspage_map_t map;
	} maps[MAX_MAPS_NB];

	mapt_entry_t entries[MAX_ENTRIES_NB];     /* General entries like syspage, kernel's elf sections and plo's elf sections */
} syspage_common;



/* Auxiliary functions */

static int syspage_getMapID(const char *map, u8 *id)
{
	int i;

	for (i = 0; i < syspage_common.mapsCnt; ++i) {
		if (plostd_strncmp(map, syspage_common.maps[i].map.name, plostd_strlen(syspage_common.maps[i].map.name)) == 0) {
			*id = syspage_common.maps[i].map.id;
			return 0;
		}
	}

	return -1;
}


static void syspage_addEntries2Map(u32 id, u32 start, u32 end)
{
	int j;
	u32 enStart, enEnd;

	if ((syspage_common.maps[id].map.start < end) && (syspage_common.maps[id].map.end > start)) {
		if (syspage_common.maps[id].map.start > start)
			enStart = syspage_common.maps[id].map.start;
		else
			enStart = start;

		if (syspage_common.maps[id].map.end < end)
			enEnd = syspage_common.maps[id].map.end;
		else
			enEnd = end;

		/* Put entry into map */
		for (j = 0; j < MAX_ENTRIES_NB; ++j) {
			if (syspage_common.maps[id].entry[j].start == 0 && syspage_common.maps[id].entry[j].end == 0) {
				syspage_common.maps[id].entry[j].start = enStart;
				syspage_common.maps[id].entry[j].end = enEnd;
			}
		}

		/* Check whether entry is from begining of the map and increase top */
		if (enStart == syspage_common.maps[id].map.start)
			syspage_common.maps[id].top = enEnd;
	}

	return;
}


static int syspage_isMapFree(u8 mapID, u32 sz)
{
	int i;
	u32 top = syspage_common.maps[mapID].top;

	for (i = 0; i < MAX_ENTRIES_NB; ++i) {
		if (syspage_common.maps[mapID].entry[i].start < (top + sz) && syspage_common.maps[mapID].entry[i].end > (top)) {
			/* Move top to the end of entries */
			syspage_common.maps[mapID].top = syspage_common.maps[mapID].entry[i].end;
			return -1;
		}
	}

	return 0;
}



/* Initialization function */

void syspage_init(void)
{
	int i, j;

	syspage_common.argCnt = 0;
	syspage_common.mapsCnt = 0;
	syspage_common.progsCnt = 0;
	syspage_common.syspage = NULL;

	for (i = 0; i < MAX_ENTRIES_NB; ++i) {
		syspage_common.entries[i].start = 0;
		syspage_common.entries[i].end = 0;
	}

	for (i = 0; i < MAX_MAPS_NB; ++i) {
		for (j = 0; j < MAX_ENTRIES_NB; ++j) {
			syspage_common.maps[i].entry[j].start = 0;
			syspage_common.maps[i].entry[j].end = 0;
		}
	}
}



/* Syspage's location functions */

int syspage_setAddress(void *addr)
{
	u32 sz;

	/* Assign syspage to the map */
	syspage_common.syspage = (void *)addr;
	sz = (u32)(MAX_SYSPAGE_SIZE / 0x200) * 0x200 + (MAX_SYSPAGE_SIZE % 0x200 ? 0x200 : 0); /* allign to PAGE_SIZE */

	syspage_addEntries((u32)addr, sz);

	syspage_common.syspage->arg = NULL;
	syspage_common.syspage->maps = NULL;
	syspage_common.syspage->progs = NULL;

	syspage_common.syspage->mapssz = 0;
	syspage_common.syspage->progssz = 0;
	syspage_common.syspage->syspagesz = sizeof(syspage_t);

	syspage_common.syspage->kernel.data = NULL;
	syspage_common.syspage->kernel.datasz = 0;

	syspage_common.syspage->kernel.bss = NULL;
	syspage_common.syspage->kernel.bsssz = 0;

	syspage_common.syspage->kernel.text = NULL;
	syspage_common.syspage->kernel.textsz = 0;

	return 0;
}


void *syspage_getAddress(void)
{
	return (void *)syspage_common.syspage;
}


/* General functions */

void syspage_save(void)
{
	int i;

	syspage_common.argCnt++; /* The last char is '\0' */
	syspage_common.syspage->arg = (char *)((void *)syspage_common.syspage + syspage_common.syspage->syspagesz);
	syspage_common.syspage->syspagesz += syspage_common.argCnt;
	low_memcpy((void *)(syspage_common.syspage->arg), syspage_common.args, syspage_common.argCnt);

	syspage_common.syspage->progs = (syspage_program_t *)((void *)syspage_common.syspage + syspage_common.syspage->syspagesz);
	syspage_common.syspage->progssz = syspage_common.progsCnt;
	syspage_common.syspage->syspagesz += (syspage_common.progsCnt * sizeof(syspage_program_t));
	low_memcpy((void *)(syspage_common.syspage->progs), syspage_common.progs, syspage_common.progsCnt * sizeof(syspage_program_t));

	syspage_common.syspage->maps = (syspage_map_t *)((void *)syspage_common.syspage + syspage_common.syspage->syspagesz);
	for (i = 0; i < syspage_common.mapsCnt; ++i)
		low_memcpy((void *)((void *)syspage_common.syspage->maps + i * sizeof(syspage_map_t)), &syspage_common.maps[i].map, sizeof(syspage_map_t));

	syspage_common.syspage->syspagesz += (syspage_common.mapsCnt * sizeof(syspage_map_t));
	syspage_common.syspage->mapssz = syspage_common.mapsCnt;
}


void syspage_show(void)
{
	int i = 0;
	plostd_printf(ATTR_LOADER, "\nSyspage addres: 0x%p\n", syspage_getAddress());
	plostd_printf(ATTR_NONE, "--------------------------\n");

	plostd_printf(ATTR_LOADER, "Kernel sections: \n");
	plostd_printf(ATTR_USER, ".text: %p \tsize: %p\n", syspage_common.syspage->kernel.text, syspage_common.syspage->kernel.textsz);
	plostd_printf(ATTR_USER, ".data: %p \tsize: %p\n", syspage_common.syspage->kernel.data, syspage_common.syspage->kernel.datasz);
	plostd_printf(ATTR_USER, ".bss:  %p \tsize: %p\n", syspage_common.syspage->kernel.bss, syspage_common.syspage->kernel.bsssz);

	plostd_printf(ATTR_LOADER, "\nPrograms number: %d\n", syspage_common.progsCnt);
	if (syspage_common.progsCnt) {
		plostd_printf(ATTR_LOADER, "NAME\t\tSTART\t\tEND\t\tIMAP\tDMAP\n");
		for (i = 0; i < syspage_common.progsCnt; ++i)
			plostd_printf(ATTR_USER, "%s\t\t%p\t%p\t%d\t%d\n", syspage_common.progs[i].cmdline, syspage_common.progs[i].start, syspage_common.progs[i].end, syspage_common.progs[i].imap, syspage_common.progs[i].dmap);
	}

	plostd_printf(ATTR_LOADER, "\n");
	plostd_printf(ATTR_LOADER, "Mulimaps number: %d\n", syspage_common.mapsCnt);
	if (syspage_common.mapsCnt) {
		plostd_printf(ATTR_LOADER, "ID\tNAME\tSTART\t\tEND\tTOP\t\tFREESZ\tATTR\n");
		for (i = 0; i < syspage_common.mapsCnt; ++i)
			plostd_printf(ATTR_USER, "%d\t%s\t%p\t%p\t%p\t%d\t%d\n",  syspage_common.maps[i].map.id,  syspage_common.maps[i].map.name, syspage_common.maps[i].map.start,
						  syspage_common.maps[i].map.end, syspage_common.maps[i].top, syspage_common.maps[i].map.end - syspage_common.maps[i].top, syspage_common.maps[i].map.attr);
	}
}



/* Map's functions */

int syspage_addmap(const char *name, void *start, void *end, u32 attr)
{
	int i;
	u32 mapID, size;
	mapID = syspage_common.mapsCnt;

	/* Check whether map exists and overlaps with other maps */
	for (i = 0; i < syspage_common.mapsCnt; ++i) {
		if (((syspage_common.maps[i].map.start < (u32)end) && (syspage_common.maps[i].map.end > (u32)start)) ||
			(plostd_strncmp(name, syspage_common.maps[i].map.name, plostd_strlen(syspage_common.maps[i].map.name)) == 0))
			return -1;
	}

	syspage_common.maps[mapID].map.start = (u32)start;
	syspage_common.maps[mapID].map.end = (u32)end;
	syspage_common.maps[mapID].map.attr = attr;
	syspage_common.maps[mapID].map.id = mapID;

	size = plostd_strlen(name) < 7 ? plostd_strlen(name) : 7;
	low_memcpy(syspage_common.maps[mapID].map.name, name, size);
	syspage_common.maps[mapID].map.name[size] = '\0';

	syspage_common.maps[mapID].top = (u32)start;

	for (i = 0; i < MAX_ENTRIES_NB; ++i) {
		if (syspage_common.entries[i].end != 0)
			syspage_addEntries2Map(mapID, syspage_common.entries[i].start, syspage_common.entries[i].end);
	}

	syspage_common.mapsCnt++;

	return 0;
}


int syspage_getMapTop(const char *map, void **addr)
{
	u8 id;

	if (syspage_getMapID(map, &id) < 0) {
		plostd_printf(ATTR_ERROR, "\nMAPS for %s doesn not exist!\n", map);
		return -1;
	}

	*addr = (void *)syspage_common.maps[id].top;

	return 0;
}


int syspage_getFreeSize(const char *map, u32 *sz)
{
	u8 id;

	if (syspage_getMapID(map, &id) < 0) {
		plostd_printf(ATTR_ERROR, "\nMAPS for %s doesn not exist!\n", map);
		return -1;
	}

	*sz = syspage_common.maps[id].map.end - syspage_common.maps[id].top;

	return 0;
}


int syspage_write2Map(const char *map, const u8 *buff, u32 len)
{
	u8 id;
	u32 freesz;

	if (syspage_getMapID(map, &id) < 0) {
		plostd_printf(ATTR_ERROR, "\nMAPS for %s doesn not exist!\n", map);
		return -1;
	}

	freesz = syspage_common.maps[id].map.end - syspage_common.maps[id].top;

	if (freesz < len ){
		plostd_printf(ATTR_ERROR, "\nThere isn't any free space in %s !\n", map);
		return -1;
	}

	if (syspage_isMapFree(id, len) < 0 ) {
		plostd_printf(ATTR_ERROR, "\nUps!! You encountered on some data. Top has been moved. Please try again!!\n");
		return -1;
	}

	low_memcpy((void *)syspage_common.maps[id].top, buff, len);

	syspage_common.maps[id].top += len;

	return 0;
}


void syspage_addEntries(u32 start, u32 sz)
{
	int i;

	for (i = 0; i < MAX_ENTRIES_NB; ++i) {
		if (syspage_common.entries[i].start == 0 && syspage_common.entries[i].end == 0) {
			syspage_common.entries[i].start = start;
			syspage_common.entries[i].end = start + sz;
			break;
		}
	}

	/* Check whether entry overlapped with existing maps */
	for (i = 0; i < syspage_common.mapsCnt; ++i)
		syspage_addEntries2Map(i, start, start + sz);
}



/* Program's functions */

int syspage_addProg(void *start, void *end, const char *imap, const char *dmap, const char *name)
{
	u8 imapID, dmapID;
	unsigned int pos = 0;
	u32 progID = syspage_common.progsCnt;

	if ((syspage_getMapID(imap, &imapID) < 0) || (syspage_getMapID(dmap, &dmapID) < 0)) {
		plostd_printf(ATTR_ERROR, "\nMAPS for %s doesn not exist!\n", name);
		return -1;
	}

	syspage_common.progs[progID].start = (u32)start;
	syspage_common.progs[progID].end = (u32)end;
	syspage_common.progs[progID].dmap = dmapID;
	syspage_common.progs[progID].imap = imapID;

	syspage_common.args[syspage_common.argCnt++] = 'X';
	low_memcpy((void *)&syspage_common.args[syspage_common.argCnt], name, plostd_strlen(name));

	syspage_common.argCnt += plostd_strlen(name);
	syspage_common.args[syspage_common.argCnt++] = ' ';
	syspage_common.args[syspage_common.argCnt] = '\0';

	low_memcpy(syspage_common.progs[progID].cmdline, name, plostd_strlen(name));

	for (pos = plostd_strlen(name); pos < 16; ++pos)
		syspage_common.progs[progID].cmdline[pos] = 0;

	syspage_common.progsCnt++;

	return 0;
}



/* Setting kernel's data */

void syspage_setKernelText(void *addr, u32 size)
{
	syspage_common.syspage->kernel.text = addr;
	syspage_common.syspage->kernel.textsz = size;

	syspage_addEntries((u32)addr, size);
}


void syspage_setKernelBss(void *addr, u32 size)
{
	syspage_common.syspage->kernel.bss = addr;
	syspage_common.syspage->kernel.bsssz = size;

	syspage_addEntries((u32)addr, size);
}


void syspage_setKernelData(void *addr, u32 size)
{
	syspage_common.syspage->kernel.data = addr;
	syspage_common.syspage->kernel.datasz = size;

	syspage_addEntries((u32)addr, size);
}
