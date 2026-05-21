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

#include <lib/lib.h>
#include <syspage.h>


#define ALIGN_ADDR(addr, align) (align ? ((addr + (align - 1)) & ~(align - 1)) : addr)

#define PART_ID_MAX (sizeof(unsigned int) * 8 - 1)

extern char __heap_base[], __heap_limit[];


struct {
	syspage_t *syspage;

	void *heapTop;
	void *heapEnd;
} syspage_common;


/* General functions */

void syspage_init(void)
{
	syspage_part_t *partition;
	syspage_named_port_t *port;

	syspage_common.syspage = (syspage_t *)__heap_base;

	syspage_common.syspage->maps = NULL;
	syspage_common.syspage->partitions = NULL;
	syspage_common.syspage->namedPorts = NULL;
	syspage_common.syspage->progs = NULL;
	syspage_common.syspage->sched = NULL;
	syspage_common.syspage->console = console_default;

	syspage_common.heapTop = (void *)ALIGN_ADDR((addr_t)__heap_base + sizeof(syspage_t), sizeof(long long));
	syspage_common.heapEnd = (char *)__heap_limit;

	syspage_common.syspage->size = (size_t)(syspage_common.heapTop - (void *)syspage_common.syspage);

	hal_syspageSet(&syspage_common.syspage->hs);

	/* Initialize background partition */
	partition = syspage_alloc(sizeof(syspage_part_t) + sizeof(NAME_PART_DEFAULT) + 1U);
	if (partition != NULL) {
		partition->next = partition;
		partition->prev = partition;
		partition->name = (char *)partition + sizeof(syspage_part_t);
		hal_strcpy(partition->name, NAME_PART_DEFAULT);
		partition->availableMem = (size_t)-1;
		partition->schedWindow = 0U;
		partition->hal = NULL;
		partition->id = 0U;
		syspage_common.syspage->partitions = partition;
	}
	else {
		log_error("\nsyspage: Cannot allocate memory for default partition");
	}

	port = syspage_namedPortAdd();
	if (port != NULL) {
		port->name = (char *)syspage_alloc(sizeof(USRV_PORT_NAME) + 1);
		hal_strcpy(port->name, USRV_PORT_NAME);
		port->recvMask = 0U;  /* Only kernel can receive */
		port->sendMask = ~0U; /* All partitions can send */
	}
}


void *syspage_alloc(size_t size)
{
	void *addr, *newTop;

	newTop = (void *)ALIGN_ADDR((addr_t)syspage_common.heapTop + size, sizeof(long long));

	if (newTop >= syspage_common.heapEnd) {
		return NULL;
	}

	addr = syspage_common.heapTop;

	syspage_common.heapTop = newTop;
	syspage_common.syspage->size = (size_t)(newTop - (void *)syspage_common.syspage);

	return addr;
}


void syspage_kernelPAddrAdd(addr_t address)
{
	syspage_common.syspage->pkernel = address;
}


/* Map's functions */

static const syspage_map_t *syspage_mapGet(const char *name)
{
	const syspage_map_t *map = syspage_common.syspage->maps;

	if (map == NULL) {
		return NULL;
	}

	do {
		if (hal_strcmp(name, map->name) == 0) {
			return map;
		}
		map = map->next;
	} while (map != syspage_common.syspage->maps);

	return NULL;
}


static const char *syspage_etype2str(unsigned int type)
{
	switch (type) {
		case hal_entryReserved: return "reserved";
		case hal_entryAllocated: return "allocated";
		case hal_entryTemp: return "temp";
		case hal_entryInvalid: return "invalid";
		default: return NULL;
	}
}


static int syspage_strAttr2ui(const char *str, unsigned int *attr)
{
	size_t i;

	*attr = 0;
	for (i = 0; str[i] != '\0'; ++i) {
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
	size_t i, pos = 0;

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
	mapent_t *e = map->entries;

	if (e == NULL) {
		newEntry->next = newEntry;
		newEntry->prev = newEntry;
		map->entries = newEntry;
		return;
	}

	do {
		if (newEntry->start < e->start) {
			newEntry->next = e;
			newEntry->prev = e->prev;

			e->prev->next = newEntry;
			e->prev = newEntry;

			/* Update head of the list */
			if (e == map->entries) {
				map->entries = newEntry;
			}

			break;
		}
		e = e->next;
	} while (e != map->entries);

	/* Add entry at the end of the list */
	if (e == map->entries) {
		newEntry->prev = e->prev;
		e->prev->next = newEntry;
		newEntry->next = e;
		e->prev = newEntry;
	}
}


int syspage_mapAdd(const char *name, addr_t start, addr_t end, const char *attr)
{
	int res;
	size_t len;
	unsigned int iattr;

	mapent_t *entry;
	mapent_t tempEntry;
	syspage_map_t *map = syspage_common.syspage->maps;

	/* Check whether map's name exists or map overlaps with other maps */
	if (map != NULL) {
		do {
			if (((map->start < end) && (map->end > start)) || (hal_strcmp(name, map->name) == 0)) {
				return -EINVAL;
			}
			map = map->next;
		} while (map != syspage_common.syspage->maps);
	}

	len = hal_strlen(name);
	map = syspage_alloc(sizeof(syspage_map_t));
	if (map != NULL) {
		map->name = syspage_alloc(len + 1);
		if (map->name == NULL) {
			return -ENOMEM;
		}
	}
	else {
		return -ENOMEM;
	}
	res = syspage_strAttr2ui(attr, &iattr);
	if (res < 0) {
		return res;
	}

	map->attr = iattr;
	map->entries = NULL;
	map->start = start;
	map->end = end;

	hal_memcpy(map->name, name, len);
	map->name[len] = '\0';


	if (syspage_common.syspage->maps == NULL) {
		map->next = map;
		map->prev = map;
		syspage_common.syspage->maps = map;
	}
	else {
		map->prev = syspage_common.syspage->maps->prev;
		syspage_common.syspage->maps->prev->next = map;
		map->next = syspage_common.syspage->maps;
		syspage_common.syspage->maps->prev = map;
	}

	map->id = (map == syspage_common.syspage->maps) ? 0 : map->prev->id + 1;

	/* Get entries from hal */
	while (hal_memoryGetNextEntry(start, end, &tempEntry) >= 0) {
		entry = syspage_alloc(sizeof(mapent_t));
		if (entry == NULL) {
			return -ENOMEM;
		}

		entry->type = tempEntry.type;
		entry->start = tempEntry.start;
		entry->end = tempEntry.end;

		syspage_sortedInsert(map, entry);
		start = entry->end;
	}

	return EOK;
}


static int syspage_bestFit(const syspage_map_t *map, size_t size, unsigned int align, addr_t *eStart)
{
	const mapent_t *e;

	addr_t bestStart = (addr_t)-1, tempStart;
	size_t bestSz = (size_t)-1, tempSz;

	tempStart = ALIGN_ADDR(map->start, align);
	e = map->entries;
	if (e != NULL) {
		do {
			if (!((e->start < tempStart + size) && (e->end > tempStart))) {
				tempSz = e->start - tempStart;
				if (tempSz < bestSz && tempSz >= size) {
					bestSz = tempSz;
					bestStart = tempStart;
				}
			}
			tempStart = ALIGN_ADDR(e->end, align);
			e = e->next;
		} while (e != map->entries);

		/* Check map's area after the last entry */
		tempStart = ALIGN_ADDR(map->entries->prev->end, align);
		tempSz = map->end - tempStart;
		if (tempSz < bestSz && tempSz >= size) {
			bestSz = tempSz;
			bestStart = tempStart;
		}

		if ((bestSz == (size_t)-1) || (bestStart == (addr_t)-1)) {
			return -EFAULT;
		}

		tempStart = bestStart;
	}
	*eStart = tempStart;

	return EOK;
}


mapent_t *syspage_entryAdd(const char *mapName, addr_t start, size_t size, unsigned int align)
{
	const syspage_map_t *map = NULL, *iterMap;
	mapent_t *entry, *newEntry;

	if (mapName == NULL) {
		iterMap = syspage_common.syspage->maps;

		if (iterMap != NULL) {
			do {
				if (((iterMap->start <= start) && (iterMap->end >= start + size))) {
					map = iterMap;
					break;
				}
				iterMap = iterMap->next;
			} while (iterMap != syspage_common.syspage->maps);
		}
	}
	else {
		map = syspage_mapGet(mapName);
	}

	if (map == NULL) {
		return NULL;
	}

	/* Find the best fit in the defined map */
	if ((mapName != NULL) && (start == (addr_t)-1)) {
		if (syspage_bestFit(map, size, align, &start) < 0) {
			return NULL;
		}
	}
	else {
		/* Add entry to specific area of memory or specific address in the defined map */
		start = ALIGN_ADDR((mapName == NULL) ? start : map->start + start, align);

		/* Check overlapping with existing entries */
		entry = map->entries;
		if (entry != NULL) {
			do {
				if (((entry->start < start + size) && (entry->end > start))) {
					return NULL;
				}
				entry = entry->next;
			} while (entry != map->entries);
		}
	}
	newEntry = syspage_alloc(sizeof(mapent_t));
	if (newEntry == NULL) {
		return NULL;
	}

	newEntry->start = start;
	newEntry->end = start + size;
	newEntry->type = hal_entryAllocated;

	/* Add entry in ascending order to circular list */
	syspage_sortedInsert((syspage_map_t *)map, newEntry);

	return newEntry;
}


int syspage_mapAttrResolve(const char *name, unsigned int *attr)
{
	const syspage_map_t *map = syspage_mapGet(name);

	if (map == NULL) {
		log_error("\nsyspage: %s does not exist", name);
		return -EINVAL;
	}

	*attr = map->attr;

	return EOK;
}


int syspage_mapNameResolve(const char *name, u8 *id)
{
	const syspage_map_t *map = syspage_mapGet(name);

	if (map == NULL) {
		log_error("\nsyspage: %s does not exist", name);
		return -EINVAL;
	}

	*id = map->id;

	return EOK;
}


int syspage_mapRangeResolve(const char *name, addr_t *start, addr_t *end)
{
	const syspage_map_t *map = syspage_mapGet(name);

	if (map == NULL) {
		log_error("\nsyspage: %s does not exist", name);
		return -EINVAL;
	}

	*start = map->start;
	*end = map->end;

	return EOK;
}


unsigned int syspage_mapRangeCheck(addr_t start, addr_t end, unsigned int *attrOut)
{
	const syspage_map_t *map = syspage_common.syspage->maps;

	if (map != NULL) {
		do {
			if ((map->start <= start) && (end < map->end)) {
				if (attrOut != NULL) {
					*attrOut = map->attr;
				}
				return 1;
			}
			map = map->next;
		} while (map != syspage_common.syspage->maps);
	}

	return 0;
}


int syspage_mapAddrResolve(addr_t addr, const char **name)
{
	const syspage_map_t *map = syspage_common.syspage->maps;

	if (map == NULL) {
		log_error("\nsyspage: %x does not match any map", addr);
		return -EINVAL;
	}

	do {
		if ((addr < map->end) && (addr >= map->start)) {
			*name = map->name;
			return EOK;
		}
		map = map->next;
	} while (map != syspage_common.syspage->maps);

	log_error("\nsyspage: %x does not match any map", addr);
	return -EINVAL;
}


const char *syspage_mapName(u8 id)
{
	const syspage_map_t *map = syspage_common.syspage->maps;

	if (map != NULL) {
		do {
			if (map->id == id) {
				return map->name;
			}
			map = map->next;
		} while (map != syspage_common.syspage->maps);
	}

	return NULL;
}


/* Scheduler's functions */

int syspage_schedulerConfigSet(syspage_sched_t *config)
{
	if (syspage_common.syspage->sched != NULL) {
		return -EEXIST;
	}
	syspage_common.syspage->sched = config;
	return EOK;
}


static syspage_sched_t *syspage_schedulerCreateDefault(void)
{
	syspage_sched_t *config = syspage_alloc(sizeof(*config) + sizeof(syspage_sched_cycle_t) * 1);
	if (config == NULL) {
		log_error("\nCannot allocate memory for default scheduler configuration");
		return NULL;
	}
	config->windowCnt = 0; /* Use kernel threads window */
	config->cycleCnt = 1;
	config->flags = sFlagCommonCycle;
	config->cycles[0] = syspage_alloc(sizeof(syspage_sched_cycle_t));
	if (config->cycles[0] == NULL) {
		log_error("\nCannot allocate memory for default scheduler configuration");
		return NULL;
	}
	config->cycles[0]->bgId = 0U;
	config->cycles[0]->len = 0U;

	log_info("\nUsing default scheduler configuration");
	return config;
}


syspage_sched_t *syspage_schedulerConfigGet(void)
{
	if (syspage_common.syspage->sched == NULL) {
		syspage_common.syspage->sched = syspage_schedulerCreateDefault();
	}
	return syspage_common.syspage->sched;
}


/* Partition's functions */

syspage_part_t *syspage_partAdd(void)
{
	syspage_part_t *part;

	part = syspage_alloc(sizeof(syspage_part_t));
	if (part == NULL) {
		return NULL;
	}

	part->prev = syspage_common.syspage->partitions->prev;
	syspage_common.syspage->partitions->prev->next = part;
	part->next = syspage_common.syspage->partitions;
	syspage_common.syspage->partitions->prev = part;
	part->id = part->prev->id + 1;
	if (part->id > PART_ID_MAX) {
		log_error("\nsyspage: Too many partitions!");
		return NULL;
	}

	return part;
}


int syspage_partResolve(const char *name, syspage_part_t **result)
{
	syspage_part_t *part = syspage_common.syspage->partitions;

	*result = NULL;

	if (part == NULL) {
		return -EINVAL;
	}

	do {
		if (hal_strcmp(name, part->name) == 0) {
			*result = part;
			return EOK;
		}
		part = part->next;
	} while (part != syspage_common.syspage->partitions);

	return -EINVAL;
}


syspage_part_t *syspage_partsGet(void)
{
	return syspage_common.syspage->partitions;
}


/* Named Port's functions */

syspage_named_port_t *syspage_namedPortAdd(void)
{
	syspage_named_port_t *port;

	port = syspage_alloc(sizeof(syspage_named_port_t));
	if (port == NULL) {
		return NULL;
	}

	if (syspage_common.syspage->namedPorts == NULL) {
		port->next = port;
		port->prev = port;
		syspage_common.syspage->namedPorts = port;
	}
	else {
		port->prev = syspage_common.syspage->namedPorts->prev;
		syspage_common.syspage->namedPorts->prev->next = port;
		port->next = syspage_common.syspage->namedPorts;
		syspage_common.syspage->namedPorts->prev = port;
	}

	return port;
}


/* Program's functions */

syspage_prog_t *syspage_progAdd(const char *argv, u32 flags)
{
	size_t len, size;
	syspage_prog_t *prog;
	const u32 isExec = (flags & flagSyspageExec) != 0;

	len = hal_strlen(argv);
	size = isExec + len + 1; /* [X] + argv + '\0' */

	prog = syspage_alloc(sizeof(syspage_prog_t));
	if (prog != NULL) {
		prog->argv = syspage_alloc(size);
		if (prog->argv == NULL) {
			return NULL;
		}
	}
	else {
		return NULL;
	}

	if (isExec != 0) {
		prog->argv[0] = 'X';
	}

	hal_memcpy(prog->argv + isExec, argv, len);
	prog->argv[size - 1] = '\0';
	prog->dmaps = NULL;
	prog->imaps = NULL;


	if (syspage_common.syspage->progs == NULL) {
		prog->next = prog;
		prog->prev = prog;
		syspage_common.syspage->progs = prog;
	}
	else {
		prog->prev = syspage_common.syspage->progs->prev;
		syspage_common.syspage->progs->prev->next = prog;
		prog->next = syspage_common.syspage->progs;
		syspage_common.syspage->progs->prev = prog;
	}

	return prog;
}


/* Set console */

void syspage_consoleSet(unsigned int id)
{
	syspage_common.syspage->console = id;
}


/* Information functions */

static void syspage_entriesShow(const syspage_map_t *map)
{
	const char *str;
	const mapent_t *e = map->entries;

	if (e == NULL) {
		return;
	}

	do {
		str = syspage_etype2str(e->type);
		lib_printf("%-13s 0x%08x%4s 0x%08x%2s - %s\n", "", e->start, "", e->end, "", str == NULL ? "null" : str);
		e = e->next;
	} while (e != map->entries);

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
		map = map->next;
	} while (map != syspage_common.syspage->maps);
}


static void syspage_progMapsShow(const syspage_prog_t *prog)
{
	size_t i;

	for (i = 0; (i < prog->imapSz) || (i < prog->dmapSz); ++i) {
		if (i != 0) {
			lib_printf("\n%42s", "");
		}
		if (i < prog->imapSz) {
			lib_printf("%4s %-14s", "", syspage_mapName(prog->imaps[i]));
		}
		else {
			lib_printf("%4s ", "");
		}

		if (i < prog->dmapSz) {
			lib_printf(" %-14s", syspage_mapName(prog->dmaps[i]));
		}
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
		prog = prog->next;
	} while (prog != syspage_common.syspage->progs);
}


void *syspage_progAllocateAndAdd(const char *map, size_t size, const char *argv, u32 flags, int allowOverwrite)
{
	syspage_prog_t *prog;
	const mapent_t *entry;
	const syspage_prog_t *iter;
	const char *iterArgv;

	if (allowOverwrite == 0) {
		iter = syspage_common.syspage->progs;
		if (iter != NULL) {
			do {
				iterArgv = (iter->argv[0] == 'X') ? (iter->argv + 1) : iter->argv;
				if (hal_strcmp(argv, iterArgv) == 0) {
					return NULL;
				}

				iter = iter->next;
			} while (iter != syspage_common.syspage->progs);
		}
	}

	entry = syspage_entryAdd(map, (addr_t)-1, size, SIZE_PAGE);
	if (entry == NULL) {
		log_error("\nCannot allocate memory for %s", argv);
		return NULL;
	}

	prog = syspage_progAdd(argv, flags);
	if (prog == NULL) {
		log_error("\nCannot add syspage program for %s", argv);
		return NULL;
	}

	prog->imaps = NULL;
	prog->imapSz = 0;
	prog->dmaps = NULL;
	prog->dmapSz = 0;
	prog->start = entry->start;
	prog->end = entry->end;

	return (void *)entry->start;
}


/* TODO: function get value from hal-specific struct, consider move to target-specific code */
#if defined(HAS_GRAPHICS) && HAS_GRAPHICS != 0
void syspage_graphmodeSet(graphmode_t graphmode)
{
	syspage_common.syspage->hs.graphmode.width = graphmode.width;
	syspage_common.syspage->hs.graphmode.height = graphmode.height;
	syspage_common.syspage->hs.graphmode.bpp = graphmode.bpp;
	syspage_common.syspage->hs.graphmode.pitch = graphmode.pitch;
	syspage_common.syspage->hs.graphmode.framebuffer = graphmode.framebuffer;
}
#endif
