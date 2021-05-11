/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * Syspage
 *
 * Copyright 2020-2021 Phoenix Systems
 * Authors: Hubert Buczynski, Gerard Swiderski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#include "hal.h"
#include "lib.h"
#include "syspage.h"
#include "errors.h"


#define MAX_PROGRAMS_NB         32
#define MAX_MAPS_NB             16
#define MAX_MAP_NAME_SIZE       8
#define MAX_ARGS_SIZE           256
#define MAX_ENTRIES_NB          6     /* 3 of kernel's sections, 2 of plo's sections and syspage */
#define MAX_CMDLINE_SIZE        16

#define MAX_SYSPAGE_SIZE        (sizeof(syspage_t) + MAX_ARGS_SIZE + MAX_PROGRAMS_NB * sizeof(syspage_program_t) \
                                 + MAX_MAPS_NB * sizeof(syspage_map_t))


#pragma pack(push, 1)

typedef struct {
	addr_t start;
	addr_t end;
	u32 attr;

	u8 id;
	char name[MAX_MAP_NAME_SIZE];
} syspage_map_t;


typedef struct {
	addr_t start;
	addr_t end;

	u8 dmap;
	u8 imap;

	char cmdline[MAX_CMDLINE_SIZE];
} syspage_program_t;


typedef struct {
	struct {
		addr_t text;
		size_t textsz;

		addr_t data;
		size_t datasz;

		addr_t bss;
		size_t bsssz;
	} kernel;

	size_t syspagesz;

	char *arg;

	size_t progssz;
	syspage_program_t *progs;

	size_t mapssz;
	syspage_map_t *maps;

	syspage_hal_t hal;
} syspage_t;

#pragma pack(pop)



typedef struct {
	addr_t start;
	addr_t end;
} map_entry_t;


/* Structure contains entries which belong to syspage map and occupied its space */
typedef struct {
	map_entry_t entry[MAX_ENTRIES_NB];

	addr_t top;
	syspage_map_t map;
} plo_map_t;


/* The syspage_common contains fields which are copied into to the kernel's memory.
 * The *syspage points to memory which is set in run time and is shared with plo and kernel. */
struct {
	syspage_t *syspage;

	size_t argCnt;
	char args[MAX_ARGS_SIZE];

	size_t progsCnt;
	syspage_program_t progs[MAX_PROGRAMS_NB];

	size_t mapsCnt;
	plo_map_t maps[MAX_MAPS_NB];

	/* General entries: syspage, kernel's elf sections and plo's elf sections */
	map_entry_t entries[MAX_ENTRIES_NB];

	syspage_hal_t hal;
} syspage_common;



/* Auxiliary functions */

static int syspage_getMapID(const char *name, u8 *id)
{
	int i;
	syspage_map_t *map;

	for (i = 0; i < syspage_common.mapsCnt; ++i) {
		map = &syspage_common.maps[i].map;
		if (hal_strncmp(name, map->name, hal_strlen(map->name)) == 0) {
			*id = map->id;
			return ERR_NONE;
		}
	}

	return ERR_ARG;
}


static void syspage_addEntries2Map(u32 id, addr_t start, addr_t end)
{
	int i;
	map_entry_t *entry;
	syspage_map_t *map;
	addr_t enStart, enEnd;

	map = &syspage_common.maps[id].map;
	if ((map->start < end) && (map->end > start)) {
		if (map->start > start)
			enStart = map->start;
		else
			enStart = start;

		if (map->end < end)
			enEnd = map->end;
		else
			enEnd = end;

		/* Put entry into map */
		for (i = 0; i < MAX_ENTRIES_NB; ++i) {
			entry = &syspage_common.maps[id].entry[i];
			if (entry->start == 0 && entry->end == 0) {
				entry->start = enStart;
				entry->end = enEnd;
			}
		}

		/* Check whether entry is from begining of the map and increase top */
		if (enStart == map->start)
			syspage_common.maps[id].top = enEnd;
	}
}


static int syspage_isMapFree(u8 id, size_t sz)
{
	int i;
	map_entry_t *entry;
	addr_t top = syspage_common.maps[id].top;

	for (i = 0; i < MAX_ENTRIES_NB; ++i) {
		entry = &syspage_common.maps[id].entry[i];
		if (entry->start < (top + sz) && entry->end > top) {
			/* Move top to the end of entries */
			syspage_common.maps[id].top = entry->end;
			return ERR_ARG;
		}
	}

	return ERR_NONE;
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

void syspage_setAddress(void *addr)
{
	size_t sz;

	/* Assign syspage to the map */
	syspage_common.syspage = (void *)addr;
	sz = (size_t)(MAX_SYSPAGE_SIZE / PAGE_SIZE) * PAGE_SIZE + (MAX_SYSPAGE_SIZE % PAGE_SIZE ? PAGE_SIZE : 0); /* allign to PAGE_SIZE */

	syspage_addEntries((addr_t)addr, sz);

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
}


void *syspage_getAddress(void)
{
	return (void *)syspage_common.syspage;
}


/* General functions */

int syspage_save(void)
{
	int i;

	if (syspage_common.syspage == NULL)
		return ERR_MEM;

	/* Save syspage arguments */
	syspage_common.argCnt++; /* The last char is '\0' */
	syspage_common.syspage->arg = (char *)((void *)syspage_common.syspage + syspage_common.syspage->syspagesz);
	syspage_common.syspage->syspagesz += syspage_common.argCnt;
	hal_memcpy((void *)(syspage_common.syspage->arg), syspage_common.args, syspage_common.argCnt);

	/* Save programs */
	syspage_common.syspage->progs = (syspage_program_t *)((void *)syspage_common.syspage + syspage_common.syspage->syspagesz);
	syspage_common.syspage->progssz = syspage_common.progsCnt;
	syspage_common.syspage->syspagesz += (syspage_common.progsCnt * sizeof(syspage_program_t));
	hal_memcpy((void *)(syspage_common.syspage->progs), syspage_common.progs, syspage_common.progsCnt * sizeof(syspage_program_t));

	/* Save maps */
	syspage_common.syspage->maps = (syspage_map_t *)((void *)syspage_common.syspage + syspage_common.syspage->syspagesz);
	for (i = 0; i < syspage_common.mapsCnt; ++i)
		hal_memcpy((void *)((void *)syspage_common.syspage->maps + i * sizeof(syspage_map_t)), &syspage_common.maps[i].map, sizeof(syspage_map_t));

	syspage_common.syspage->syspagesz += (syspage_common.mapsCnt * sizeof(syspage_map_t));
	syspage_common.syspage->mapssz = syspage_common.mapsCnt;

	/* Save architecture dependent structure */
	if (sizeof(syspage_hal_t) != 0) {
		hal_memcpy((void *)&syspage_common.syspage->hal, (void *)&syspage_common.hal, sizeof(syspage_hal_t));
		syspage_common.syspage->syspagesz += sizeof(syspage_hal_t);
	}

	return ERR_NONE;
}


/* TODO: align text and show map's attributes as string based on integer */
void syspage_show(void)
{
	int i;
	plo_map_t *pmap;
	syspage_program_t *prog;

	lib_printf("\nSyspage addres: 0x%p\n", syspage_getAddress());
	lib_printf("--------------------------\n");

	lib_printf("Kernel sections: \n");
	lib_printf(".text: %p \tsize: %p\n", syspage_common.syspage->kernel.text, syspage_common.syspage->kernel.textsz);
	lib_printf(".data: %p \tsize: %p\n", syspage_common.syspage->kernel.data, syspage_common.syspage->kernel.datasz);
	lib_printf(".bss:  %p \tsize: %p\n", syspage_common.syspage->kernel.bss, syspage_common.syspage->kernel.bsssz);

	lib_printf("\nPrograms number: %d\n", syspage_common.progsCnt);
	if (syspage_common.progsCnt) {
		lib_printf("NAME\t\tSTART\t\tEND\t\tIMAP\tDMAP\n");
		for (i = 0; i < syspage_common.progsCnt; ++i) {
			prog = &syspage_common.progs[i];
			lib_printf("%s\t%p\t%p\t%d\t%d\n", prog->cmdline, prog->start, prog->end, prog->imap, prog->dmap);
		}
	}

	lib_printf("\n");
	lib_printf("Mulimaps number: %d\n", syspage_common.mapsCnt);
	if (syspage_common.mapsCnt) {
		lib_printf("ID\tNAME\tSTART\t\tEND\tTOP\t\tFREESZ\tATTR\n");
		for (i = 0; i < syspage_common.mapsCnt; ++i) {
			pmap = &syspage_common.maps[i];
			lib_printf("%d\t%s\t%p\t%p\t%p\t%p\t%d\n",  pmap->map.id,  pmap->map.name, pmap->map.start, pmap->map.end,
			              pmap->top, pmap->map.end - pmap->top, pmap->map.attr);
		}
	}
}



/* Map's functions */

int syspage_addmap(const char *name, void *start, void *end, u32 attr)
{
	int i;
	size_t size;
	map_entry_t *entry;
	syspage_map_t *map;
	plo_map_t *ploMap;
	u32 len, mapID = syspage_common.mapsCnt;

	/* Check whether map exists and overlaps with other maps */
	for (i = 0; i < syspage_common.mapsCnt; ++i) {
		map = &syspage_common.maps[i].map;
		if (((map->start < (addr_t)end) && (map->end > (addr_t)start)) ||
			(hal_strncmp(name, map->name, hal_strlen(map->name)) == 0))
			return ERR_ARG;
	}

	ploMap = &syspage_common.maps[mapID];
	map = &ploMap->map;

	map->start = (addr_t)start;
	map->end = (addr_t)end;
	map->attr = attr;
	map->id = mapID;

	len =  hal_strlen(name) ;
	size = min(len, MAX_MAP_NAME_SIZE - 1);

	hal_memcpy(map->name, name, size);
	map->name[size] = '\0';

	ploMap->top = (addr_t)start;

	for (i = 0; i < MAX_ENTRIES_NB; ++i) {
		entry = &syspage_common.entries[i];
		if (entry->end != 0)
			syspage_addEntries2Map(mapID, entry->start, entry->end);
	}

	syspage_common.mapsCnt++;

	return ERR_NONE;
}


int syspage_getMapTop(const char *map, void **addr)
{
	u8 id;

	if (syspage_getMapID(map, &id) < 0) {
		lib_printf("\nMAPS for %s does not exist!\n", map);
		return ERR_ARG;
	}

	*addr = (void *)syspage_common.maps[id].top;

	return ERR_NONE;
}


int syspage_alignMapTop(const char *map)
{
	u8 id;
	addr_t newTop;
	plo_map_t *ploMap;

	if (syspage_getMapID(map, &id) < 0) {
		lib_printf("\nMAPS for %s does not exist!\n", map);
		return ERR_ARG;
	}

	ploMap = &syspage_common.maps[id];
	newTop = (ploMap->top + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

	if (newTop > ploMap->map.end) {
		lib_printf("\nMap %s is full!\n", map);
		return ERR_MEM;
	}

	ploMap->top = newTop;

	return ERR_NONE;
}


int syspage_getFreeSize(const char *map, size_t *sz)
{
	u8 id;

	if (syspage_getMapID(map, &id) < 0) {
		lib_printf("\nMAPS for %s does not exist!\n", map);
		return ERR_ARG;
	}

	*sz = syspage_common.maps[id].map.end - syspage_common.maps[id].top;

	return ERR_NONE;
}


int syspage_write2Map(const char *map, const u8 *buff, size_t len)
{
	u8 id;
	size_t freesz;
	plo_map_t *ploMap;

	if (syspage_getMapID(map, &id) < 0) {
		lib_printf("\nMAPS for %s does not exist!\n", map);
		return ERR_ARG;
	}

	ploMap = &syspage_common.maps[id];
	freesz = ploMap->map.end - ploMap->top;

	if (freesz < len) {
		lib_printf("\nThere isn't any free space in %s !\n", map);
		return ERR_ARG;
	}

	if (syspage_isMapFree(id, len) < 0) {
		lib_printf("\nUps!! You encountered on some data. Top has been moved. Please try again!!\n");
		return ERR_MEM;
	}

	hal_memcpy((void *)ploMap->top, buff, len);

	ploMap->top += len;

	return ERR_NONE;
}


void syspage_addEntries(addr_t start, size_t sz)
{
	int i;
	map_entry_t *entry;

	for (i = 0; i < MAX_ENTRIES_NB; ++i) {
		entry = &syspage_common.entries[i];
		if (entry->start == 0 && entry->end == 0) {
			entry->start = start;
			entry->end = start + sz;
			break;
		}
	}

	/* Check whether entry overlapped with existing maps */
	for (i = 0; i < syspage_common.mapsCnt; ++i)
		syspage_addEntries2Map(i, start, start + sz);
}


int syspage_getMapAttr(const char *map, unsigned int *attr)
{
	u8 id;

	if (syspage_getMapID(map, &id) < 0) {
		lib_printf("\nMAPS for %s does not exist!\n", map);
		return ERR_ARG;
	}

	*attr = syspage_common.maps[id].map.attr;

	return ERR_NONE;
}


/* Program's functions */

int syspage_addProg(void *start, void *end, const char *imap, const char *dmap, const char *name, u32 flags)
{
	u8 imapID, dmapID;
	unsigned int pos, len;
	syspage_program_t *prog;
	u32 progID = syspage_common.progsCnt;
	const u32 isExec = (flags & flagSyspageExec) != 0;

	if ((syspage_getMapID(imap, &imapID) < 0) || (syspage_getMapID(dmap, &dmapID) < 0)) {
		lib_printf("\nMAPS for %s does not exist!\n", name);
		return ERR_ARG;
	}

	len = hal_strlen(name);

	if (syspage_common.argCnt + isExec + len + 1 + 1 > MAX_ARGS_SIZE) {
		lib_printf("\nMAX_ARGS_SIZE for %s exceeded!\n", name);
		return ERR_ARG;
	}

	for (pos = 0; pos < len; pos++) {
		if (name[pos] == ';')
			break;
	}

	if (pos >= MAX_CMDLINE_SIZE) {
		lib_printf("\nSyspage program %s, name is too long!\n", name);
		return ERR_ARG;
	}

	prog = &syspage_common.progs[progID];
	prog->start = (addr_t)start;
	prog->end = (addr_t)end;
	prog->dmap = dmapID;
	prog->imap = imapID;

	if (isExec)
		syspage_common.args[syspage_common.argCnt++] = 'X';

	hal_memcpy((void *)&syspage_common.args[syspage_common.argCnt], name, len);

	syspage_common.argCnt += len;
	syspage_common.args[syspage_common.argCnt++] = ' ';
	syspage_common.args[syspage_common.argCnt] = '\0';

	/* copy only program name, without (;) args) */
	hal_memcpy(prog->cmdline, name, pos);

	while (pos < MAX_CMDLINE_SIZE)
		prog->cmdline[pos++] = '\0';

	syspage_common.progsCnt++;

	return ERR_NONE;
}



/* Setting kernel's data */

void syspage_setKernelText(void *addr, size_t size)
{
	syspage_common.syspage->kernel.text = (addr_t)addr;
	syspage_common.syspage->kernel.textsz = size;

	syspage_addEntries((addr_t)addr, size);
}


void syspage_setKernelBss(void *addr, size_t size)
{
	syspage_common.syspage->kernel.bss = (addr_t)addr;
	syspage_common.syspage->kernel.bsssz = size;

	syspage_addEntries((addr_t)addr, size);
}


void syspage_setKernelData(void *addr, size_t size)
{
	syspage_common.syspage->kernel.data = (addr_t)addr;
	syspage_common.syspage->kernel.datasz = size;

	syspage_addEntries((addr_t)addr, size);
}


/* Add specific hal data */

void syspage_setHalData(const syspage_hal_t *hal)
{
	if (sizeof(syspage_hal_t) != 0)
		hal_memcpy((void *)&syspage_common.hal, (void *)hal, sizeof(syspage_hal_t));
}

