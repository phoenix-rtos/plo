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
#include "devs.h"
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

	dev_handler_t *h;
} phfs_handler_t;


typedef struct {
	char alias[32];
	addr_t addr;
	size_t size;
} phfs_file_t;


struct {
	phfs_handler_t handlers[SIZE_PHFS_HANDLERS];
	unsigned int hCnt;

	phfs_file_t files[SIZE_PHFS_FILES];
	unsigned int fCnt;
} phfs_common;


static const char *phfs_getProtName(unsigned int id)
{
	switch (id)
	{
	case phfs_prot_raw:
		return "raw";

	case phfs_prot_phoenixd:
		return "phoenixd";

	default:
		return "";
	}
}


static int phfs_setProt(phfs_handler_t *ph, const char *prot)
{
	size_t sz = plostd_strlen(prot);

	if (prot == NULL || (sz == 3 && plostd_strncmp(prot, "raw", 3) == 0))
		ph->prot = phfs_prot_raw;
	else if (sz == 8 && plostd_strncmp(prot, "phoenixd", 8) == 0)
		ph->prot = phfs_prot_phoenixd;
	else
		return ERR_ARG;

	return ERR_NONE;
}


static int phfs_getHandlerId(const char *alias)
{
	int i;
	phfs_handler_t *ph;
	size_t sz, aliasSz = plostd_strlen(alias);

	for (i = 0; i < phfs_common.hCnt; ++i) {
		ph = &phfs_common.handlers[i];
		sz = plostd_strlen(ph->alias);
		if ((sz == aliasSz) && (plostd_strncmp(alias, ph->alias, sz) == 0))
			return i;
	}

	return ERR_ARG;
}


int phfs_regDev(const char *alias, unsigned int major, unsigned int minor, const char *prot)
{
	size_t sz;
	dev_handler_t *dh;
	phfs_handler_t *ph;

	if (alias == NULL)
		return ERR_ARG;

	if (phfs_common.hCnt >= SIZE_PHFS_HANDLERS) {
		plostd_printf(ATTR_ERROR, "\nToo many devices");
		return ERR_ARG;
	}

	if (devs_getHandler(major, minor, &dh) < 0) {
		plostd_printf(ATTR_ERROR, "\n%d.%d - device does not exist", major, minor);
		return ERR_ARG;
	}

	/* Check if alias is already in use */
	if (phfs_getHandlerId(alias) >= 0) {
		plostd_printf(ATTR_ERROR, "\n%s - alias is already in use", alias);
		return ERR_ARG;
	}

	ph = &phfs_common.handlers[phfs_common.hCnt];
	if (phfs_setProt(ph, prot) < 0) {
		plostd_printf(ATTR_ERROR, "\n%s - wrong protocol name\n\t use: \"%s\", \"%s\"", prot,
		              phfs_getProtName(phfs_prot_raw), phfs_getProtName(phfs_prot_phoenixd));
		return ERR_ARG;
	}

	sz = plostd_strlen(alias);
	sz = (sz < 7) ? (sz + 1) : 7;
	hal_memcpy(ph->alias, alias, sz);
	ph->alias[sz] = '\0';

	ph->h = dh;
	ph->major = major;
	ph->minor = minor;
	phfs_common.hCnt++;

	return ERR_NONE;
}


int phfs_getFileAddr(handler_t h, addr_t *addr)
{
	if (h.fd >= SIZE_PHFS_FILES)
		return ERR_ARG;

	*addr = phfs_common.files[h.fd].addr;

	return ERR_NONE;
}


int phfs_getFileSize(handler_t h, size_t *size)
{
	if (h.fd >= SIZE_PHFS_FILES)
		return ERR_ARG;

	*size = phfs_common.files[h.fd].size;

	return ERR_NONE;
}


void phfs_showDevs(void)
{
	int i;
	phfs_handler_t *ph;

	if (phfs_common.hCnt == 0) {
		plostd_printf(ATTR_ERROR, "\nNone of the devices have been registered\n");
		return;
	}

	plostd_printf(ATTR_LOADER, "\nALIAS\tID\tPROTOCOL\n");
	for (i = 0; i < phfs_common.hCnt; ++i) {
		ph = &phfs_common.handlers[i];
		plostd_printf(ATTR_NONE, "%s\t%d.%d\t%s\n", ph->alias, ph->major, ph->minor, phfs_getProtName(ph->prot));
	}
}


static int phfs_getFileId(const char *alias)
{
	int i;
	phfs_file_t *file;
	size_t sz, aliasSz = plostd_strlen(alias);

	for (i = 0; i < phfs_common.fCnt; ++i) {
		file = &phfs_common.files[i];
		sz = plostd_strlen(file->alias);
		if ((sz == aliasSz) && (plostd_strncmp(alias, file->alias, sz) == 0))
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
	sz = (sz < 31) ? (sz + 1) : 31;
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
	phfs_clbk_t clbk;
	phfs_handler_t *ph;

	if ((res = phfs_getHandlerId(alias)) < 0)
		return ERR_ARG;

	handler->pn = res;
	ph = &phfs_common.handlers[handler->pn];

	switch (ph->prot)
	{
	case phfs_prot_phoenixd:
		clbk.dn = ph->minor;
		clbk.read = ph->h->read;
		clbk.write = ph->h->write;

		if ((res = phoenixd_open(file, flags, &clbk)) < 0)
			return res;

		handler->fd = res;
		break;

	case phfs_prot_raw:
		/* NULL file means that phfs refers to raw device data */
		handler->fd = -1;
		if (file == NULL)
			break;

		if ((res = phfs_getFileId(file)) < 0)
			return res;
		handler->fd = res;
		break;

	default:
		break;
	}

	return ERR_NONE;
}


ssize_t phfs_read(handler_t handler, addr_t offs, u8 *buff, unsigned int len)
{
	phfs_clbk_t clbk;
	phfs_file_t *file;
	phfs_handler_t *ph;

	if (handler.pn >= SIZE_PHFS_HANDLERS)
		return ERR_ARG;

	ph = &phfs_common.handlers[handler.pn];

	switch (ph->prot)
	{
	case phfs_prot_phoenixd:
		clbk.dn = ph->minor;
		clbk.read = ph->h->read;
		clbk.write = ph->h->write;

		return phoenixd_read(handler.fd, offs, buff, len, &clbk);

	case phfs_prot_raw:
		/* Reading raw data from device */
		if (handler.fd == -1)
			return ph->h->read(ph->minor, offs, buff, len);

		/* Reading file defined by alias */
		if (handler.fd >= SIZE_PHFS_FILES)
			return ERR_ARG;

		file = &phfs_common.files[handler.fd];
		if (offs + len < file->size)
			return ph->h->read(ph->minor, file->addr + offs, buff, len);
		else
			return ph->h->read(ph->minor, file->addr + offs, buff, file->size - offs);

	default:
		break;
	}

	return ERR_PHFS_PROTO;
}


ssize_t phfs_write(handler_t handler, addr_t offs, const u8 *buff, unsigned int len)
{
	phfs_clbk_t clbk;
	phfs_handler_t *ph;
	phfs_file_t *file;

	if (handler.pn >= SIZE_PHFS_HANDLERS)
		return ERR_ARG;

	ph = &phfs_common.handlers[handler.pn];

	switch (ph->prot)
	{
	case phfs_prot_phoenixd:
		clbk.dn = ph->minor;
		clbk.read = ph->h->read;
		clbk.write = ph->h->write;

		return phoenixd_write(handler.fd, offs, buff, len, &clbk);

	case phfs_prot_raw:
		/* Writing raw data to device */
		if (handler.fd == -1)
			return ph->h->write(ph->minor, offs, buff, len);

		/* Writing data to file defined by alias */
		if (handler.fd >= SIZE_PHFS_FILES)
			return ERR_ARG;

		file = &phfs_common.files[handler.fd];
		if (offs + len < file->size)
			return ph->h->write(ph->minor, file->addr + offs, buff, len);
		else
			return ph->h->write(ph->minor, file->addr + offs, buff, file->size - offs);

	default:
		break;
	}

	return ERR_PHFS_PROTO;
}


int phfs_close(handler_t handler)
{
	int res;
	phfs_clbk_t clbk;
	phfs_handler_t *ph;

	if (handler.pn >= SIZE_PHFS_HANDLERS)
		return ERR_ARG;

	ph = &phfs_common.handlers[handler.pn];

	switch (ph->prot)
	{
	case phfs_prot_phoenixd:
		clbk.dn = ph->minor;
		clbk.read = ph->h->read;
		clbk.write = ph->h->write;

		if ((res = phoenixd_close(handler.fd, &clbk)) < 0)
			return res;
		break;

	case phfs_prot_raw:
	default:
		break;
	}

	if (ph->h->sync(ph->minor) < 0)
		return ERR_ARG;

	return ERR_NONE;
}
