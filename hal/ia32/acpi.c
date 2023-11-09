/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * ACPI detection and configuration
 *
 * Copyright 2023 Phoenix Systems
 * Author: Andrzej Stalke
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */
#include "acpi.h"

#define MAX_CPU_COUNT 64

typedef struct {
	char magic[4];
	u32 length;
	u8 revision;
	u8 checksum;
	char oem[6];
	char oemID[8];
	u32 oemRevision;
	u32 creatorId;
	u32 creatorRevision;
} __attribute__((packed)) sdt_header_t;


typedef struct {
	char magic[8];
	u8 checksum;
	char oemID[6];
	u8 revision;
	struct {
		sdt_header_t header;
		sdt_header_t *sdt[];
	} __attribute__((packed)) * rsdt;
} __attribute__((packed)) rsdp_t;


typedef struct {
	rsdp_t rsdp;

	u32 length;
	u64 xsdtAddr;
	u8 extChecksum;
	u8 _reserved[3];
} __attribute__((packed)) xsdp_t;


static inline int _hal_checksumVerify(const void *data, const size_t size)
{
	const u8 *ptr;
	u8 sum = 0;
	for (ptr = (const u8 *)data; ptr < (const u8 *)data + size; ++ptr) {
		sum += *ptr;
	}
	return (sum == 0) ? 1 : 0;
}


static void _hal_acpiMadtHandler(hal_syspage_t *hs, sdt_header_t *header)
{
	if (_hal_checksumVerify(header, header->length) != 0) {
		hs->madt = (unsigned long)header; /* FIXME - should be addr_t */
		hs->madtLength = header->length;
	}
}


static void _hal_acpiFadtHandler(hal_syspage_t *hs, sdt_header_t *header)
{
	if (_hal_checksumVerify(header, header->length) != 0) {
		hs->fadt = (unsigned long)header; /* FIXME - should be addr_t */
		hs->fadtLength = header->length;
	}
}


static void _hal_acpiHpetHandler(hal_syspage_t *hs, sdt_header_t *header)
{
	if (_hal_checksumVerify(header, header->length) != 0) {
		hs->hpet = (unsigned long)header; /* FIXME - should be addr_t */
		hs->hpetLength = header->length;
	}
}


static const struct {
	const char magic[4];
	void (*handler)(hal_syspage_t *hs, sdt_header_t *);
} acpi_knownTables[] = {
	{ .magic = "APIC", .handler = _hal_acpiMadtHandler },
	{ .magic = "FACP", .handler = _hal_acpiFadtHandler },
	{ .magic = "HPET", .handler = _hal_acpiHpetHandler }
};


static rsdp_t *_hal_acpiLookForRsdp(hal_syspage_t *hs, void *start, void *end)
{
	static const char rsdpMagic[] = "RSD PTR ";
	void *ptr;
	rsdp_t *entry;
	for (ptr = start; ptr < end; ptr += 16) {
		if (hal_strncmp(rsdpMagic, ptr, 8) == 0) {
			if (_hal_checksumVerify(ptr, sizeof(rsdp_t)) != 0) {
				entry = ptr;
				if ((entry->revision == 2) && (_hal_checksumVerify(entry, sizeof(xsdp_t)) != 0)) {
					hs->acpi_version = ACPI_XSDP;
				}
				else {
					hs->acpi_version = ACPI_RSDP;
				}
				return entry;
			}
		}
	}
	return NULL;
}


static rsdp_t *_hal_acpiFindRsdp(hal_syspage_t *hs)
{
	/* Search EBDA */
	unsigned int ebda = ((*(u16 *)0x40eu) << 4) & ~(SIZE_PAGE - 1);
	rsdp_t *result;
	if ((ebda < 0x00080000u) || (ebda > 0x0009ffffu)) {
		ebda = 0x00080000u;
	}
	hs->ebda = ebda;

	result = _hal_acpiLookForRsdp(hs, (void *)ebda, (void *)(ebda + 1024));
	if (result == NULL) {
		result = _hal_acpiLookForRsdp(hs, (void *)0x000e0000u, (void *)0x000fffffu);
	}
	return result;
}


static void _hal_acpiHandleRsdt(hal_syspage_t *hs, rsdp_t *rsdp)
{
	size_t i, j;
	if (_hal_checksumVerify(rsdp->rsdt, rsdp->rsdt->header.length) != 0) {
		for (i = 0; i < (rsdp->rsdt->header.length - sizeof(sdt_header_t)) / 4; ++i) {
			for (j = 0; j < sizeof(acpi_knownTables) / sizeof(*acpi_knownTables); ++j) {
				if (hal_strncmp(rsdp->rsdt->sdt[i]->magic, acpi_knownTables[j].magic, 4) == 0) {
					acpi_knownTables[j].handler(hs, rsdp->rsdt->sdt[i]);
					break;
				}
			}
		}
	}
	else {
		/* Invalid RSDT table */
		hs->acpi_version = ACPI_NONE;
	}
}


void hal_acpiInit(hal_syspage_t *hs)
{
	hs->ebda = 0;
	hs->acpi_version = ACPI_NONE;
	hs->localApicAddr = 0;
	hs->madt = 0;
	hs->madtLength = 0;
	hs->fadt = 0;
	hs->fadtLength = 0;
	hs->hpet = 0;
	hs->hpetLength = 0;

	rsdp_t *rsdp = _hal_acpiFindRsdp(hs);
	if (rsdp != NULL) {
		switch (hs->acpi_version) {
			case ACPI_XSDP:
				/* TODO: Don't fallback to RSDP */
				hs->acpi_version = ACPI_RSDP;
				/* fall-through */
			case ACPI_RSDP:
				_hal_acpiHandleRsdt(hs, rsdp);
				break;
			default:
				/* RSDP not found */
				break;
		}
	}
	else {
		hs->acpi_version = ACPI_NONE;
	}
}
