/*
 * Phoenix-RTOS
 *
 * Operating system loader
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

#include <lib/lib.h>


#define ALIGN_ADDR(addr, align) (align ? ((addr + (align - 1)) & ~(align - 1)) : addr)

extern void _end(void);


struct {
	syspage_t *syspage;

	void *heapTop;
	void *heapEnd;
} syspage_common;


/* General functions */

void syspage_init(void)
{
	syspage_common.syspage = (syspage_t *)ALIGN_ADDR((addr_t)_end, SIZE_PAGE);

	syspage_common.syspage->maps = NULL;
	syspage_common.syspage->progs = NULL;
	syspage_common.syspage->size = sizeof(syspage_t);
	syspage_common.syspage->console = console_default;

	syspage_common.heapTop = (char *)syspage_common.syspage + sizeof(syspage_t);
	syspage_common.heapEnd = (char *)syspage_common.syspage + SIZE_SYSPAGE;

	hal_syspageSet(&syspage_common.syspage->hs);
}


void *syspage_alloc(size_t size)
{
	void *addr;

	if ((void *)((char *)syspage_common.heapTop + size) >= syspage_common.heapEnd)
		return NULL;

	addr = syspage_common.heapTop;

	syspage_common.heapTop = (char *)syspage_common.heapTop + size;
	syspage_common.syspage->size += size;

	return addr;
}


/* Map's functions */

static const syspage_map_t *syspage_mapGet(const char *name)
{
	const syspage_map_t *map = syspage_common.syspage->maps;

	if (map == NULL)
		return NULL;

	do {
		if (hal_strcmp(name, map->name) == 0)
			return map;
	} while ((map = map->next) != syspage_common.syspage->maps);

	return NULL;
}


static const char *syspage_etype2str(unsigned int type)
{
	switch (type) {
		case hal_entryReserved: return "reserved";
		case hal_entryAllocated: return "allocated";
		case hal_entryTemp: return "temp";
		default: return NULL;
	}
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


static void syspage_sortedInsert(syspage_map_t *map, mapent_t *newEntry)
{
	mapent_t *e;

	if ((e = map->entries) == NULL) {
		LIST_ADD(&map->entries, newEntry);
		return;
	}

	do {
		if (newEntry->start < e->start) {
			newEntry->next = e;
			newEntry->prev = e->prev;

			e->prev->next = newEntry;
			e->prev = newEntry;

			/* Update head of the list */
			if (e == map->entries)
				map->entries = newEntry;

			break;
		}
	} while ((e = e->next) != map->entries);

	/* Add entry at the end of the list */
	if (e == map->entries)
		LIST_ADD(&map->entries, newEntry);
}


int syspage_mapAdd(const char *name, addr_t start, addr_t end, const char *attr)
{
	int res;
	size_t len;

	mapent_t *entry;
	mapent_t tempEntry;
	syspage_map_t *map = syspage_common.syspage->maps;

	/* Check whether map's name exists or map overlaps with other maps */
	if (map != NULL) {
		do {
			if (((map->start < end) && (map->end > start)) ||
					(hal_strcmp(name, map->name) == 0))
				return -EINVAL;
		} while ((map = map->next) != syspage_common.syspage->maps);
	}

	len = hal_strlen(name);
	if ((map = syspage_alloc(sizeof(syspage_map_t))) == NULL ||
			(map->name = syspage_alloc(len + 1)) == NULL)
		return -ENOMEM;

	if ((res = syspage_strAttr2ui(attr, &map->attr)) < 0)
		return res;

	map->entries = NULL;
	map->start = start;
	map->end = end;

	hal_memcpy(map->name, name, len);
	map->name[len] = '\0';

	LIST_ADD(&syspage_common.syspage->maps, map);
	map->id = (map == syspage_common.syspage->maps) ? 0 : map->prev->id + 1;

	/* Get entries from hal */
	while (hal_memoryGetNextEntry(start, end, &tempEntry) >= 0) {
		if ((entry = syspage_alloc(sizeof(mapent_t))) == NULL)
			return NULL;

		entry->type = tempEntry.type;
		entry->start = tempEntry.start;
		entry->end = tempEntry.end;

		syspage_sortedInsert(map, entry);
		start = entry->end;
	}

	if ((res = hal_memoryAddMap(map->start, map->end, map->attr, map->id)) < 0)
		return res;

	return EOK;
}


static int syspage_bestFit(const syspage_map_t *map, size_t size, unsigned int align, addr_t *eStart)
{
	const mapent_t *e;

	addr_t bestStart = (addr_t)-1, tempStart;
	size_t bestSz = (size_t)-1, tempSz;

	tempStart = ALIGN_ADDR(map->start, align);
	if ((e = map->entries) != NULL) {
		do {
			if (!((e->start < tempStart + size) && (e->end > tempStart))) {
				tempSz = e->start - tempStart;
				if (tempSz < bestSz && tempSz >= size) {
					bestSz = tempSz;
					bestStart = tempStart;
				}
			}
			tempStart = ALIGN_ADDR(e->end, align);
		} while ((e = e->next) != map->entries);

		/* Check map's area after the last entry */
		tempStart = ALIGN_ADDR(map->entries->prev->end, align);
		tempSz = map->end - tempStart;
		if (tempSz < bestSz && tempSz >= size) {
			bestSz = tempSz;
			bestStart = tempStart;
		}

		if (bestSz == (size_t)-1 || bestStart == (addr_t)-1)
			return -EFAULT;

		tempStart = bestStart;
	}
	*eStart = tempStart;

	return EOK;
}


mapent_t *syspage_entryAdd(const char *mapName, addr_t start, size_t size, unsigned int align)
{
	const syspage_map_t *map;
	mapent_t *entry, *newEntry;

	if (mapName == NULL) {
		for (map = syspage_common.syspage->maps; map != NULL; map = map->next) {
			if (((map->start < start + size) && (map->end > start)))
				break;
		}
	}
	else {
		map = syspage_mapGet(mapName);
	}

	if (map == NULL)
		return NULL;

	/* Find the best fit in the defined map */
	if (mapName != NULL && start == (addr_t)-1) {
		if (syspage_bestFit(map, size, align, &start) < 0)
			return NULL;
	}
	else {
		/* Add entry to specific area of memory or specific address in the defined map */
		start = ALIGN_ADDR((mapName == NULL) ? start : map->start + start, align);

		/* Check overlapping with existing entries */
		if ((entry = map->entries) != NULL) {
			do {
				if (((entry->start < start + size) && (entry->end > start)))
					return NULL;
			} while ((entry = entry->next) != map->entries);
		}
	}

	if ((newEntry = syspage_alloc(sizeof(mapent_t))) == NULL)
		return NULL;

	newEntry->start = start;
	newEntry->end = start + size;
	newEntry->type = hal_entryAllocated;

	/* Add entry in ascending order to circular list */
	syspage_sortedInsert((syspage_map_t *)map, newEntry);

	return newEntry;
}


int syspage_mapAttrResolve(const char *name, unsigned int *attr)
{
	const syspage_map_t *map;

	if ((map = syspage_mapGet(name)) == NULL) {
		log_error("\nsyspage: %s does not exist", name);
		return -EINVAL;
	}

	*attr = map->attr;

	return EOK;
}


int syspage_mapNameResolve(const char *name, u8 *id)
{
	const syspage_map_t *map;

	if ((map = syspage_mapGet(name)) == NULL) {
		log_error("\nsyspage: %s does not exist", name);
		return -EINVAL;
	}

	*id = map->id;

	return EOK;
}


int syspage_mapRangeResolve(const char *name, addr_t *start, addr_t *end)
{
	const syspage_map_t *map;

	if ((map = syspage_mapGet(name)) == NULL) {
		log_error("\nsyspage: %s does not exist", name);
		return -EINVAL;
	}

	*start = map->start;
	*end = map->end;

	return EOK;
}


const char *syspage_mapName(u8 id)
{
	const syspage_map_t *map;

	if ((map = syspage_common.syspage->maps) != NULL) {
		do {
			if (map->id == id)
				return map->name;
		} while ((map = map->next) != syspage_common.syspage->maps);
	}

	return NULL;
}


/* Program's functions */

syspage_prog_t *syspage_progAdd(const char *argv, u32 flags)
{
	size_t len, size;
	syspage_prog_t *prog;
	const u32 isExec = (flags & flagSyspageExec) != 0;

	len = hal_strlen(argv);
	size = isExec + len + 1; /* [X] + argv + '\0' */

	if ((prog = syspage_alloc(sizeof(syspage_prog_t))) == NULL ||
		(prog->argv = syspage_alloc(size)) == NULL)
		return NULL;

	if (isExec)
		prog->argv[0] = 'X';

	hal_memcpy(prog->argv + isExec, argv, len);
	prog->argv[size - 1] = '\0';
	prog->dmaps = NULL;
	prog->imaps = NULL;

	LIST_ADD(&syspage_common.syspage->progs, prog);

	return prog;
}



/* Set console */

void syspage_consoleSet(unsigned int id)
{

}


/* Information functions */

static void syspage_entriesShow(const syspage_map_t *map)
{
	const char *str;
	const mapent_t *e;

	if ((e = map->entries) == NULL)
		return;

	do {
		str = syspage_etype2str(e->type);
		lib_printf("%-13s 0x%08x%4s 0x%08x%2s - %s\n", "", e->start, "", e->end, "", str == NULL ? "null" : str);
	} while ((e = e->next) != map->entries);

	lib_printf("%-13s -------------------------\n", "");
}


void syspage_mapShow(void)
{
	char attr[33];
	const syspage_map_t *map = syspage_common.syspage->maps;

	if (map == NULL) {
		lib_printf("\nMaps number: 0");
		return;
	}

	lib_printf(CONSOLE_BOLD "\n%-4s %-8s %-14s %-14s %s\n", "ID", "NAME", "START", "END", "ATTR");
	lib_printf(CONSOLE_NORMAL);

	do {
		syspage_uiAttr2str(map->attr, attr);
		lib_printf("%d%-3s %-8s 0x%08x%4s 0x%08x%4s %s\n", map->id, "", map->name,
			map->start, "", map->end, "", attr);
		syspage_entriesShow(map);
	} while ((map = map->next) != syspage_common.syspage->maps);
}


static void syspage_progMapsShow(const syspage_prog_t *prog)
{
	unsigned int i;

	for (i = 0; i < prog->imapSz || i < prog->dmapSz; ++i) {
		if (i)
			lib_printf("\n%42s", "");
		if (i < prog->imapSz)
			lib_printf("%4s %-14s", "", syspage_mapName(prog->imaps[i]));
		else
			lib_printf("%4s ", "");

		if (i < prog->dmapSz)
			lib_printf(" %-14s", syspage_mapName(prog->dmaps[i]));
	}
}


void syspage_progShow(void)
{
	const char *name;
	const syspage_prog_t *prog = syspage_common.syspage->progs;

	if (prog == NULL) {
		lib_printf("\nApps number: 0");
		return;
	}

	lib_printf(CONSOLE_BOLD "\n%-16s %-14s %-14s %-14s %-14s\n", "NAME", "START", "END", "IMAPs", "DMAPs");
	lib_printf(CONSOLE_NORMAL);
	do {
		name = (prog->argv[0] == 'X') ? prog->argv + 1 : prog->argv;
		lib_printf("%-16s 0x%08x%4s 0x%08x", name, prog->start, "", prog->end);
		syspage_progMapsShow(prog);
		lib_printf("\n");
	} while ((prog = prog->next) != syspage_common.syspage->progs);
}
