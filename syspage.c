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


#include "syspage.h"

#include <hal/hal.h>
#include <lib/log.h>
#include <lib/console.h>
#include <lib/errno.h>


#define MAX_PROGRAMS_NB   32
#define MAX_MAPS_NB       16
#define MAX_MAP_NAME_SIZE 8
#define MAX_ARGS_SIZE     256
#define MAX_ENTRIES_NB    6 /* 3 of kernel's sections, 2 of plo's sections and syspage */

#define MAX_SYSPAGE_SIZE (sizeof(syspage_t) + MAX_ARGS_SIZE + MAX_PROGRAMS_NB * sizeof(syspage_program_t) + MAX_MAPS_NB * sizeof(syspage_map_t))


#pragma pack(push, 1)

typedef struct {
	addr_t start;
	addr_t end;
	u32 attr;

	u8 id;
	char name[SIZE_MAP_NAME + 1];
} syspage_map_t;


typedef struct {
	addr_t start;
	addr_t end;

	u8 dmap;
	u8 imap;

	char name[SIZE_APP_NAME + 1];
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

	addr_t kernel_entry;

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
			return EOK;
		}
	}

	return -EINVAL;
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
			return -EINVAL;
		}
	}

	return EOK;
}


static int syspage_strAttr2ui(const char *str, unsigned int *attr)
{
	int i;

	*attr = 0;
	for (i = 0; str[i]; ++i) {
		switch (str[i]) {
			case 'r':
				*attr |= mAttrRead;
				break;

			case 'w':
				*attr |= mAttrWrite;
				break;

			case 'x':
				*attr |= mAttrExec;
				break;

			case 's':
				*attr |= mAttrShareable;
				break;

			case 'c':
				*attr |= mAttrCacheable;
				break;

			case 'b':
				*attr |= mAttrBufferable;
				break;

			default:
				log_error("\nsyspage: Wrong attribute - '%c'", str[i]);
				return -EINVAL;
		}
	}

	return EOK;
}


static void syspage_uiAttr2str(unsigned int attr, char *str)
{
	unsigned int i = 0;
	unsigned int pos = 0;

	for (i = 0; i < 32; ++i) {
		switch (attr & (1 << i)) {
			case mAttrRead:
				str[pos++] = 'r';
				break;

			case mAttrWrite:
				str[pos++] = 'w';
				break;

			case mAttrExec:
				str[pos++] = 'x';
				break;

			case mAttrShareable:
				str[pos++] = 's';
				break;

			case mAttrCacheable:
				str[pos++] = 'c';
				break;

			case mAttrBufferable:
				str[pos++] = 'b';
				break;
		}
	}

	str[pos] = '\0';
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

void syspage_setAddress(addr_t addr)
{
	size_t sz;

	/* Assign syspage to the map */
	syspage_common.syspage = (void *)addr;
	sz = (size_t)(MAX_SYSPAGE_SIZE / PAGE_SIZE) * PAGE_SIZE + (MAX_SYSPAGE_SIZE % PAGE_SIZE ? PAGE_SIZE : 0); /* allign to PAGE_SIZE */

	syspage_addEntries(addr, sz);

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


addr_t syspage_getAddress(void)
{
	return (addr_t)syspage_common.syspage;
}


/* General functions */

int syspage_save(void)
{
	int i;

	if (syspage_common.syspage == NULL)
		return -ENOMEM;

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

	return EOK;
}


void syspage_showMaps(void)
{
	int i;
	char attr[33];
	plo_map_t *pmap;

	if (syspage_common.mapsCnt == 0) {
		lib_printf("\nMaps number: 0");
		return;
	}

	lib_printf(CONSOLE_BOLD "\n%-4s %-8s %-14s %-14s %-14s %-14s %s\n", "ID", "NAME", "START", "END", "TOP", "FREESZ", "ATTR");
	lib_printf(CONSOLE_NORMAL);
	for (i = 0; i < syspage_common.mapsCnt; ++i) {
		pmap = &syspage_common.maps[i];
		syspage_uiAttr2str(pmap->map.attr, attr);
		lib_printf("%d%-3s %-8s 0x%08x%4s 0x%08x%4s 0x%08x%4s 0x%08x%4s %s\n", pmap->map.id, "", pmap->map.name,
			pmap->map.start, "", pmap->map.end, "", pmap->top, "", pmap->map.end - pmap->top, "", attr);
	}
}


void syspage_showApps(void)
{
	int i;
	syspage_program_t *prog;
	if (syspage_common.progsCnt == 0) {
		lib_printf("\nApps number: 0");
		return;
	}

	lib_printf(CONSOLE_BOLD "\n%-16s %-14s %-14s %-14s %-14s\n", "NAME", "START", "END", "IMAP ID", "DMAP ID");
	lib_printf(CONSOLE_NORMAL);
	for (i = 0; i < syspage_common.progsCnt; ++i) {
		prog = &syspage_common.progs[i];
		lib_printf("%-16s 0x%08x%4s 0x%08x%4s %d%13s %d\n", prog->name, prog->start, "", prog->end, "", prog->imap, "", prog->dmap);
	}
}


void syspage_showKernel(void)
{
	if (syspage_common.syspage->kernel.textsz == 0) {
		lib_printf("\nKernel is not loaded");
		return;
	}
	lib_printf(CONSOLE_BOLD "\nKernel sections: \n" CONSOLE_NORMAL);
	lib_printf("%-8s 0x%08x \tsize: 0x%08x\n", ".text:", syspage_common.syspage->kernel.text, syspage_common.syspage->kernel.textsz);
	lib_printf("%-8s 0x%08x \tsize: 0x%08x\n", ".data:", syspage_common.syspage->kernel.data, syspage_common.syspage->kernel.datasz);
	lib_printf("%-8s 0x%08x \tsize: 0x%08x\n", ".bss:", syspage_common.syspage->kernel.bss, syspage_common.syspage->kernel.bsssz);
}


void syspage_showAddr(void)
{
	lib_printf("\nSyspage address: 0x%x\n", syspage_getAddress());
}


/* Validation */

int syspage_validateAddrMap(unsigned int opt, addr_t addr, u8 id, unsigned int attr)
{
	int i;
	plo_map_t *pmap;

	for (i = 0; i < syspage_common.mapsCnt; ++i) {
		pmap = &syspage_common.maps[i];

		if (addr >= pmap->map.start && addr < pmap->map.end) {
			/* optional check if addr is below map top */
			if ((opt & flagValidateTop) != 0 && addr >= pmap->top)
				break;

			/* optional check of map id */
			if ((opt & flagValidateMap) != 0 && id != i)
				break;

			/* optional check of map attributes */
			if ((opt & flagValidateAttr) != 0 && (pmap->map.attr & attr) != attr)
				break;

			return EOK;
		}
	}

	return -EINVAL;
}


int syspage_validateKernel(addr_t *addr)
{
	if (syspage_common.syspage->kernel.textsz == 0)
		return -EINVAL;

	if (syspage_common.syspage->kernel.bsssz == 0)
		return -EINVAL;

	if (syspage_common.syspage->kernel.datasz > 0) {
		if (syspage_validateAddrMap(flagValidateAttr, syspage_common.syspage->kernel.data, 0, mAttrRead | mAttrWrite) != EOK)
			return -EINVAL;
	}

	if (syspage_validateAddrMap(flagValidateAttr | flagValidateTop, syspage_common.syspage->kernel.text, 0, mAttrExec) != EOK)
		return -EINVAL;

	if (syspage_validateAddrMap(flagValidateAttr | flagValidateTop, syspage_common.kernel_entry, 0, mAttrExec) != EOK)
		return -EINVAL;

	if (syspage_validateAddrMap(flagValidateAttr, syspage_common.syspage->kernel.bss, 0, mAttrRead | mAttrWrite) != EOK)
		return -EINVAL;

	*addr = syspage_common.kernel_entry;

	return EOK;
}


/* Map's functions */

int syspage_addmap(const char *name, addr_t start, addr_t end, const char *attr)
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
		if (((map->start < end) && (map->end > start)) ||
			(hal_strncmp(name, map->name, hal_strlen(map->name)) == 0))
			return -EINVAL;
	}

	ploMap = &syspage_common.maps[mapID];
	map = &ploMap->map;

	map->start = start;
	map->end = end;
	map->id = mapID;
	if (syspage_strAttr2ui(attr, &map->attr) < 0)
		return -EINVAL;

	len = hal_strlen(name);
	size = min(len, SIZE_MAP_NAME - 1);

	hal_memcpy(map->name, name, size);
	map->name[size] = '\0';

	ploMap->top = start;

	for (i = 0; i < MAX_ENTRIES_NB; ++i) {
		entry = &syspage_common.entries[i];
		if (entry->end != 0)
			syspage_addEntries2Map(mapID, entry->start, entry->end);
	}

	syspage_common.mapsCnt++;

	return EOK;
}


int syspage_getMapTop(const char *map, addr_t *addr)
{
	u8 id;

	if (syspage_getMapID(map, &id) < 0) {
		log_error("\nsyspage: %s does not exist", map);
		return -EINVAL;
	}

	*addr = syspage_common.maps[id].top;

	return EOK;
}


int syspage_setMapTop(const char *map, addr_t addr)
{
	u8 id;

	if (syspage_getMapID(map, &id) < 0) {
		log_error("\nsyspage: %s does not exist", map);
		return -EINVAL;
	}

	syspage_common.maps[id].top = addr;

	return EOK;
}


int syspage_alignMapTop(const char *map)
{
	u8 id;
	addr_t newTop;
	plo_map_t *ploMap;

	if (syspage_getMapID(map, &id) < 0) {
		log_error("\nsyspage: %s does not exist", map);
		return -EINVAL;
	}

	ploMap = &syspage_common.maps[id];
	newTop = (ploMap->top + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

	if (newTop > ploMap->map.end) {
		log_error("\nsyspage:  %s is full!\n", map);
		return -ENOMEM;
	}

	ploMap->top = newTop;

	return EOK;
}


int syspage_getFreeSize(const char *map, size_t *sz)
{
	u8 id;

	if (syspage_getMapID(map, &id) < 0) {
		log_error("\nsyspage: %s does not exist", map);
		return -EINVAL;
	}

	*sz = syspage_common.maps[id].map.end - syspage_common.maps[id].top;

	return EOK;
}


int syspage_write2Map(const char *map, const u8 *buff, size_t len)
{
	u8 id;
	size_t freesz;
	plo_map_t *ploMap;

	if (syspage_getMapID(map, &id) < 0) {
		log_error("\nsyspage: %s does not exist", map);
		return -EINVAL;
	}

	ploMap = &syspage_common.maps[id];
	freesz = ploMap->map.end - ploMap->top;

	if (freesz < len) {
		log_error("\nsyspage: There isn't any free space in %s", map);
		return -EINVAL;
	}

	if (syspage_isMapFree(id, len) < 0) {
		log_error("\nsyspage: You encountered on some data. Top has been moved. Please try again!");
		return -ENOMEM;
	}

	hal_memcpy((void *)ploMap->top, buff, len);

	ploMap->top += len;

	return EOK;
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
		log_error("\nsyspage: %s does not exist", map);
		return -EINVAL;
	}

	*attr = syspage_common.maps[id].map.attr;

	return EOK;
}


/* Program's functions */

int syspage_addProg(addr_t start, addr_t end, const char *imap, const char *dmap, const char *cmdline, u32 flags)
{
	u8 imapID, dmapID;
	unsigned int pos, len;
	syspage_program_t *prog;
	u32 progID = syspage_common.progsCnt;
	const u32 isExec = (flags & flagSyspageExec) != 0;

	if ((syspage_getMapID(imap, &imapID) < 0) || (syspage_getMapID(dmap, &dmapID) < 0)) {
		log_error("\nsyspage: %s or %s does not exist!\n", imap, dmap);
		return -EINVAL;
	}

	len = hal_strlen(cmdline);

	if (syspage_common.argCnt + isExec + len + 1 + 1 > MAX_ARGS_SIZE) {
		log_error("\nsyspage: MAX_ARGS_SIZE for %s exceeded!\n", cmdline);
		return -EINVAL;
	}

	for (pos = 0; pos < len; pos++) {
		if (cmdline[pos] == ';')
			break;
	}

	if (pos > SIZE_APP_NAME) {
		log_error("\nsyspage: %s, name is too long!\n", cmdline);
		return -EINVAL;
	}

	prog = &syspage_common.progs[progID];
	prog->start = start;
	prog->end = end;
	prog->dmap = dmapID;
	prog->imap = imapID;

	if (isExec)
		syspage_common.args[syspage_common.argCnt++] = 'X';

	hal_memcpy((void *)&syspage_common.args[syspage_common.argCnt], cmdline, len);

	syspage_common.argCnt += len;
	syspage_common.args[syspage_common.argCnt++] = ' ';
	syspage_common.args[syspage_common.argCnt] = '\0';

	/* copy only program name, without (;) args) */
	hal_memcpy(prog->name, cmdline, pos);

	while (pos <= SIZE_APP_NAME)
		prog->name[pos++] = '\0';

	syspage_common.progsCnt++;

	return EOK;
}


/* Setting kernel's data */

void syspage_setKernelEntry(addr_t addr)
{
	syspage_common.kernel_entry = addr;
}


void syspage_setKernelText(addr_t addr, size_t size)
{
	syspage_common.syspage->kernel.text = addr;
	syspage_common.syspage->kernel.textsz = size;

	syspage_addEntries(addr, size);
}


void syspage_setKernelBss(addr_t addr, size_t size)
{
	syspage_common.syspage->kernel.bss = addr;
	syspage_common.syspage->kernel.bsssz = size;

	syspage_addEntries(addr, size);
}


void syspage_setKernelData(addr_t addr, size_t size)
{
	syspage_common.syspage->kernel.data = addr;
	syspage_common.syspage->kernel.datasz = size;

	syspage_addEntries(addr, size);
}


/* Add specific hal data */

void syspage_setHalData(const syspage_hal_t *hal)
{
	if (sizeof(syspage_hal_t) != 0)
		hal_memcpy((void *)&syspage_common.hal, (void *)hal, sizeof(syspage_hal_t));
}
