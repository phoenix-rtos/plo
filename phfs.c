/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * Phoenix FileSystem
 *
 * Copyright 2020 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "phfs.h"
#include "hal.h"

#include "phoenixd.h"
#include "plostd.h"
#include "errors.h"


#define SIZE_PHFS_HANDLERS    8
#define SIZE_PHFS_FILES       10


enum { phfs_prot_raw = 0, phfs_prot_phoenixd };


typedef struct {
	char alias[8];
	unsigned int major;
	unsigned int minor;
	unsigned int prot;
} phfs_device_t;


typedef struct {
	char alias[32];
	addr_t addr;
	size_t size;
} phfs_file_t;


struct {
	phfs_device_t devices[SIZE_PHFS_HANDLERS];
	unsigned int dCnt;

	phfs_file_t files[SIZE_PHFS_FILES];
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
	if (prot == NULL || (plostd_strcmp(prot, "raw") == 0))
		pd->prot = phfs_prot_raw;
	else if (plostd_strcmp(prot, "phoenixd") == 0)
		pd->prot = phfs_prot_phoenixd;
	else
		return ERR_ARG;

	return ERR_NONE;
}


static int phfs_getHandlerId(const char *alias)
{
	int i;
	phfs_device_t *pd;

	for (i = 0; i < phfs_common.dCnt; ++i) {
		pd = &phfs_common.devices[i];
		if ((plostd_strcmp(alias, pd->alias) == 0))
			return i;
	}

	return ERR_ARG;
}


int phfs_regDev(const char *alias, unsigned int major, unsigned int minor, const char *prot)
{
	size_t sz;
	phfs_device_t *pd;

	if (alias == NULL)
		return ERR_ARG;

	if (phfs_common.dCnt >= SIZE_PHFS_HANDLERS) {
		plostd_printf(ATTR_ERROR, "\nToo many devices");
		return ERR_ARG;
	}

	if (devs_check(major, minor) < 0) {
		plostd_printf(ATTR_ERROR, "\n%d.%d - device does not exist", major, minor);
		return ERR_ARG;
	}

	/* Check if alias is already in use */
	if (phfs_getHandlerId(alias) >= 0) {
		plostd_printf(ATTR_ERROR, "\n%s - alias is already in use", alias);
		return ERR_ARG;
	}

	pd = &phfs_common.devices[phfs_common.dCnt];
	if (phfs_setProt(pd, prot) < 0) {
		plostd_printf(ATTR_ERROR, "\n%s - wrong protocol name\n\t use: \"%s\", \"%s\"", prot,
		              phfs_getProtName(phfs_prot_raw), phfs_getProtName(phfs_prot_phoenixd));
		return ERR_ARG;
	}

	sz = plostd_strlen(alias);
	sz = (sz < (sizeof(pd->alias) - 1)) ? (sz + 1) : (sizeof(pd->alias) - 1);
	hal_memcpy(pd->alias, alias, sz);
	pd->alias[sz] = '\0';

	pd->major = major;
	pd->minor = minor;
	phfs_common.dCnt++;

	return ERR_NONE;
}


int phfs_getFileAddr(handler_t h, addr_t *addr)
{
	if (h.id >= SIZE_PHFS_FILES)
		return ERR_ARG;

	*addr = phfs_common.files[h.id].addr;

	return ERR_NONE;
}


int phfs_getFileSize(handler_t h, size_t *size)
{
	if (h.id >= SIZE_PHFS_FILES)
		return ERR_ARG;

	*size = phfs_common.files[h.id].size;

	return ERR_NONE;
}


void phfs_showDevs(void)
{
	int i;
	phfs_device_t *pd;

	if (phfs_common.dCnt == 0) {
		plostd_printf(ATTR_ERROR, "\nNone of the devices have been registered\n");
		return;
	}

	plostd_printf(ATTR_LOADER, "\nALIAS\tID\tPROTOCOL\n");
	for (i = 0; i < phfs_common.dCnt; ++i) {
		pd = &phfs_common.devices[i];
		plostd_printf(ATTR_NONE, "%s\t%d.%d\t%s\n", pd->alias, pd->major, pd->minor, phfs_getProtName(pd->prot));
	}
}


static int phfs_getFileId(const char *alias)
{
	int i;
	phfs_file_t *file;

	for (i = 0; i < phfs_common.fCnt; ++i) {
		file = &phfs_common.files[i];
		if (plostd_strcmp(alias, file->alias) == 0)
			return i;
	}

	return ERR_ARG;
}


int phfs_regFile(const char *alias, addr_t addr, size_t size)
{
	size_t sz;
	phfs_file_t *file;

	if (alias == NULL)
		return ERR_ARG;

	if (phfs_common.fCnt >= SIZE_PHFS_FILES) {
		plostd_printf(ATTR_ERROR, "\nExceeded max number of files");
		return ERR_ARG;
	}

	/* Check if alias is already in use */
	if (phfs_getFileId(alias) >= 0) {
		plostd_printf(ATTR_ERROR, "\n%s - alias is already in use", alias);
		return ERR_ARG;
	}

	file = &phfs_common.files[phfs_common.fCnt];

	sz = plostd_strlen(alias);
	sz = (sz < (sizeof(file->alias) - 1)) ? (sz + 1) : (sizeof(file->alias) - 1);
	hal_memcpy(file->alias, alias, sz);
	file->alias[sz] = '\0';

	file->addr = addr;
	file->size = size;

	++phfs_common.fCnt;

	return ERR_NONE;
}


int phfs_open(const char *alias, const char *file, unsigned int flags, handler_t *handler)
{
	int res;
	phfs_device_t *pd;

	if ((res = phfs_getHandlerId(alias)) < 0)
		return ERR_ARG;

	handler->pd = res;
	pd = &phfs_common.devices[handler->pd];

	switch (pd->prot) {
	case phfs_prot_phoenixd:
		if ((res = phoenixd_open(file, pd->major, pd->minor, flags)) < 0)
			return res;

		handler->id = res;
		break;

	case phfs_prot_raw:
		/* NULL file means that phfs refers to raw device data */
		handler->id = -1;
		if (file == NULL)
			break;

		if ((res = phfs_getFileId(file)) < 0)
			return res;
		handler->id = res;
		break;

	default:
		break;
	}

	return ERR_NONE;
}


ssize_t phfs_read(handler_t handler, addr_t offs, u8 *buff, unsigned int len)
{
	phfs_file_t *file;
	phfs_device_t *pd;

	if (handler.pd >= SIZE_PHFS_HANDLERS)
		return ERR_ARG;

	pd = &phfs_common.devices[handler.pd];

	switch (pd->prot) {
	case phfs_prot_phoenixd:
		return phoenixd_read(handler.id, pd->major, pd->minor, offs, buff, len);

	case phfs_prot_raw:
		/* Reading raw data from device */
		if (handler.id == -1)
			return devs_read(pd->major, pd->minor, offs, buff, len);

		/* Reading file defined by alias */
		if (handler.id >= SIZE_PHFS_FILES)
			return ERR_ARG;

		file = &phfs_common.files[handler.id];
		if (offs + len < file->size)
			return devs_read(pd->major, pd->minor, file->addr + offs, buff, len);
		else
			return devs_read(pd->major, pd->minor, file->addr + offs, buff, file->size - offs);

	default:
		break;
	}

	return ERR_PHFS_PROTO;
}


ssize_t phfs_write(handler_t handler, addr_t offs, const u8 *buff, unsigned int len)
{
	phfs_device_t *pd;
	phfs_file_t *file;

	if (handler.pd >= SIZE_PHFS_HANDLERS)
		return ERR_ARG;

	pd = &phfs_common.devices[handler.pd];

	switch (pd->prot) {
	case phfs_prot_phoenixd:
		return phoenixd_write(handler.id, pd->major, pd->minor, offs, buff, len);

	case phfs_prot_raw:
		/* Writing raw data to device */
		if (handler.id == -1)
			return devs_write(pd->major, pd->minor, offs, buff, len);

		/* Writing data to file defined by alias */
		if (handler.id >= SIZE_PHFS_FILES)
			return ERR_ARG;

		file = &phfs_common.files[handler.id];
		if (offs + len < file->size)
			return devs_write(pd->major, pd->minor, file->addr + offs, buff, len);
		else
			return devs_write(pd->major, pd->minor, file->addr + offs, buff, file->size - offs);

	default:
		break;
	}

	return ERR_PHFS_PROTO;
}


int phfs_close(handler_t handler)
{
	int res;
	phfs_device_t *pd;

	if (handler.pd >= SIZE_PHFS_HANDLERS)
		return ERR_ARG;

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
		return ERR_ARG;

	return ERR_NONE;
}


int phfs_isMappable(handler_t handler, addr_t addr, size_t sz, int mode, addr_t memaddr, size_t memsz, int memmode, addr_t *devOffs)
{
	phfs_device_t *pd;

	if (handler.pd >= SIZE_PHFS_HANDLERS)
		return ERR_ARG;

	pd = &phfs_common.devices[handler.pd];

	return devs_isMappable(pd->major, pd->minor, addr, sz, mode, memaddr, memsz, memmode, devOffs);
}
