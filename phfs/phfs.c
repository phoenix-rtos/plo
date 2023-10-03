/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Phoenix FileSystem
 *
 * Copyright 2020-2021 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "phfs.h"
#include "phoenixd.h"

#include <lib/lib.h>

#define SIZE_PHFS_HANDLERS 8
#define SIZE_PHFS_ALIASES  20

#define PHFS_TIMEOUT_MS 500


enum { phfs_prot_raw = 0, phfs_prot_phoenixd };


typedef struct {
	char alias[8];
	unsigned int major;
	unsigned int minor;
	unsigned int prot;
} phfs_device_t;


typedef struct {
	char alias[34];
	addr_t addr;
	size_t size;
} phfs_file_t;


struct {
	phfs_device_t devices[SIZE_PHFS_HANDLERS];
	unsigned int dCnt;

	phfs_file_t files[SIZE_PHFS_ALIASES];
	unsigned int fCnt;
} phfs_common;


static const char *phfs_getProtName(unsigned int id)
{
	switch (id) {
		case phfs_prot_raw:
			return "raw";

		case phfs_prot_phoenixd:
			return "phoenixd";

		default:
			return "";
	}
}


static int phfs_setProt(phfs_device_t *pd, const char *prot)
{
	if (prot == NULL || (hal_strcmp(prot, "raw") == 0))
		pd->prot = phfs_prot_raw;
	else if (hal_strcmp(prot, "phoenixd") == 0)
		pd->prot = phfs_prot_phoenixd;
	else
		return -EINVAL;

	return EOK;
}


static int phfs_getHandlerId(const char *alias)
{
	int i;
	phfs_device_t *pd;

	for (i = 0; i < phfs_common.dCnt; ++i) {
		pd = &phfs_common.devices[i];
		if ((hal_strcmp(alias, pd->alias) == 0))
			return i;
	}

	return -EINVAL;
}


int phfs_devReg(const char *alias, unsigned int major, unsigned int minor, const char *prot)
{
	size_t sz;
	phfs_device_t *pd;

	if (alias == NULL)
		return -EINVAL;

	if (phfs_common.dCnt >= SIZE_PHFS_HANDLERS) {
		log_error("\nphfs: Too many devices");
		return -EINVAL;
	}

	if (devs_check(major, minor) < 0) {
		log_error("\nphfs: %d.%d - device does not exist", major, minor);
		return -EINVAL;
	}

	/* Check if alias is already in use */
	if (phfs_getHandlerId(alias) >= 0) {
		log_error("\nphfs: %s - alias is already in use", alias);
		return -EINVAL;
	}

	pd = &phfs_common.devices[phfs_common.dCnt];
	if (phfs_setProt(pd, prot) < 0) {
		log_error("\nphfs: %s - wrong protocol name\n\t use: \"%s\", \"%s\"", prot,
			phfs_getProtName(phfs_prot_raw), phfs_getProtName(phfs_prot_phoenixd));
		return -EINVAL;
	}

	sz = hal_strlen(alias);
	sz = (sz < (sizeof(pd->alias) - 1)) ? (sz + 1) : (sizeof(pd->alias) - 1);
	hal_memcpy(pd->alias, alias, sz);
	pd->alias[sz] = '\0';

	pd->major = major;
	pd->minor = minor;
	phfs_common.dCnt++;

	return EOK;
}


int phfs_aliasAddrResolve(handler_t h, addr_t *addr)
{
	if (h.id >= SIZE_PHFS_ALIASES || h.id >= phfs_common.fCnt)
		return -EINVAL;

	*addr = phfs_common.files[h.id].addr;

	return EOK;
}


void phfs_devsShow(void)
{
	int i;
	phfs_device_t *pd;

	if (phfs_common.dCnt == 0) {
		log_error("\nphfs: None of the devices have been registered\n");
		return;
	}

	lib_printf(CONSOLE_BOLD "\nALIAS\tID\tPROTOCOL\n" CONSOLE_NORMAL);
	for (i = 0; i < phfs_common.dCnt; ++i) {
		pd = &phfs_common.devices[i];
		lib_printf("%s\t%d.%d\t%s\n", pd->alias, pd->major, pd->minor, phfs_getProtName(pd->prot));
	}
}


void phfs_aliasesShow(void)
{
	int i;
	phfs_file_t *f;

	if (phfs_common.fCnt == 0) {
		log_error("\nphfs: None of the files have been registered\n");
		return;
	}

	lib_printf(CONSOLE_BOLD "\n%-32s %-10s %-10s\n" CONSOLE_NORMAL, "ALIAS", "OFFSET", "SIZE");
	for (i = 0; i < phfs_common.fCnt; ++i) {
		f = &phfs_common.files[i];
		lib_printf("%-32s 0x%08x 0x%08x\n", f->alias, f->addr, f->size);
	}
}


static int phfs_getAliasId(const char *alias)
{
	int i;
	phfs_file_t *file;

	for (i = 0; i < phfs_common.fCnt; ++i) {
		file = &phfs_common.files[i];
		if (hal_strcmp(alias, file->alias) == 0)
			return i;
	}

	return -EINVAL;
}


int phfs_aliasReg(const char *alias, addr_t addr, size_t size)
{
	size_t sz;
	phfs_file_t *file;

	if (alias == NULL)
		return -EINVAL;

	if (phfs_common.fCnt >= SIZE_PHFS_ALIASES) {
		log_error("\nphfs: Exceeded max number of files");
		return -EINVAL;
	}

	/* Check if alias is already in use */
	if (phfs_getAliasId(alias) >= 0) {
		log_error("\nphfs: %s - alias is already in use", alias);
		return -EINVAL;
	}

	file = &phfs_common.files[phfs_common.fCnt];

	sz = hal_strlen(alias);
	sz = (sz < (sizeof(file->alias) - 1)) ? (sz + 1) : (sizeof(file->alias) - 1);
	hal_memcpy(file->alias, alias, sz);
	file->alias[sz] = '\0';

	file->addr = addr;
	file->size = size;

	++phfs_common.fCnt;

	return EOK;
}


int phfs_open(const char *alias, const char *file, unsigned int flags, handler_t *handler)
{
	int res;
	phfs_device_t *pd;

	res = phfs_getHandlerId(alias);
	if (res < 0) {
		return res;
	}

	handler->pd = res;
	pd = &phfs_common.devices[handler->pd];

	if ((flags & PHFS_OPEN_RAWONLY) != 0 && pd->prot != phfs_prot_raw) {
		return -ENXIO;
	}

	switch (pd->prot) {
		case phfs_prot_phoenixd:
			res = phoenixd_open(file, pd->major, pd->minor, flags);
			if (res < 0) {
				return res;
			}

			handler->id = res;
			break;

		case phfs_prot_raw:
			/* NULL file means that phfs refers to raw device data */
			handler->id = -1;
			if (file == NULL) {
				break;
			}

			res = phfs_getAliasId(file);
			if (res < 0) {
				return res;
			}

			handler->id = res;
			break;

		default:
			break;
	}

	return EOK;
}


ssize_t phfs_read(handler_t handler, addr_t offs, void *buff, size_t len)
{
	phfs_file_t *file;
	phfs_device_t *pd;

	if (handler.pd >= SIZE_PHFS_HANDLERS)
		return -EINVAL;

	pd = &phfs_common.devices[handler.pd];

	switch (pd->prot) {
		case phfs_prot_phoenixd:
			return phoenixd_read(handler.id, pd->major, pd->minor, offs, buff, len);

		case phfs_prot_raw:
			/* Reading raw data from device */
			if (handler.id == -1)
				return devs_read(pd->major, pd->minor, offs, buff, len, PHFS_TIMEOUT_MS);

			/* Reading file defined by alias */
			if (handler.id >= SIZE_PHFS_ALIASES)
				return -EINVAL;
			file = &phfs_common.files[handler.id];

			return devs_read(pd->major, pd->minor, file->addr + offs, buff, min(len, file->size - offs), PHFS_TIMEOUT_MS);

		default:
			break;
	}

	return -EIO;
}


ssize_t phfs_write(handler_t handler, addr_t offs, const void *buff, size_t len)
{
	phfs_device_t *pd;
	phfs_file_t *file;

	if (handler.pd >= SIZE_PHFS_HANDLERS)
		return -EINVAL;

	pd = &phfs_common.devices[handler.pd];

	switch (pd->prot) {
		case phfs_prot_phoenixd:
			return phoenixd_write(handler.id, pd->major, pd->minor, offs, buff, len);

		case phfs_prot_raw:
			/* Writing raw data to device */
			if (handler.id == -1)
				return devs_write(pd->major, pd->minor, offs, buff, len);

			/* Writing data to file defined by alias */
			if (handler.id >= SIZE_PHFS_ALIASES)
				return -EINVAL;
			file = &phfs_common.files[handler.id];

			return devs_write(pd->major, pd->minor, file->addr + offs, buff, min(len, file->size - offs));

		default:
			break;
	}

	return -EIO;
}


ssize_t phfs_erase(handler_t handler, addr_t offs, size_t len, unsigned int flags)
{
	phfs_device_t *pd;

	if (handler.pd >= SIZE_PHFS_HANDLERS) {
		return -EINVAL;
	}

	pd = &phfs_common.devices[handler.pd];

	if (pd->minor != DEV_STORAGE && pd->prot != phfs_prot_raw) {
		return -EINVAL;
	}

	return devs_erase(pd->major, pd->minor, offs, len, flags);
}


int phfs_close(handler_t handler)
{
	int res;
	phfs_device_t *pd;

	if (handler.pd >= SIZE_PHFS_HANDLERS)
		return -EINVAL;

	pd = &phfs_common.devices[handler.pd];

	switch (pd->prot) {
		case phfs_prot_phoenixd:
			if ((res = phoenixd_close(handler.id, pd->major, pd->minor)) < 0)
				return res;
			break;

		case phfs_prot_raw:
		default:
			break;
	}

	if (devs_sync(pd->major, pd->minor) < 0)
		return -EINVAL;

	return EOK;
}


int phfs_map(handler_t handler, addr_t addr, size_t sz, int mode, addr_t memaddr, size_t memsz, int memmode, addr_t *a)
{
	phfs_device_t *pd;

	if (handler.pd >= SIZE_PHFS_HANDLERS)
		return -EINVAL;

	pd = &phfs_common.devices[handler.pd];

	return devs_map(pd->major, pd->minor, addr, sz, mode, memaddr, memsz, memmode, a);
}


int phfs_stat(handler_t handler, phfs_stat_t *stat)
{
	int res;
	phfs_device_t *pd;
	phfs_file_t *file;

	if (handler.pd >= SIZE_PHFS_HANDLERS)
		return -EINVAL;

	pd = &phfs_common.devices[handler.pd];

	switch (pd->prot) {
		case phfs_prot_phoenixd:
			if ((res = phoenixd_stat(handler.id, pd->major, pd->minor, stat)) < 0)
				return res;
			break;

		case phfs_prot_raw:
			if (handler.id == -1 || handler.id >= SIZE_PHFS_ALIASES)
				return -EINVAL;

			file = &phfs_common.files[handler.id];
			stat->size = file->size;
		default:
			break;
	}

	return EOK;
}
