/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Disk driver (based on BIOS interrupt calls)
 *
 * Copyright 2012, 2021 Phoenix Systems
 * Copyright 2001, 2005, 2006 Pawel Pisarczyk
 *
 * Author: Pawel Pisarczyk, Lukasz Kosinski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <devices/devs.h>
#include <hal/hal.h>
#include <lib/lib.h>


/* Disk block size */
#define SIZE_BLOCK 0x200

/* Disk caches size in blocks */
#define BLOCKS_RCACHE (SIZE_RCACHE / SIZE_BLOCK)
#define BLOCKS_WCACHE (SIZE_WCACHE / SIZE_BLOCK)

/* Disk access type (BIOS int 0x13 extension) */
#define DISK_READ  0x42
#define DISK_WRITE 0x43


typedef struct {
	/* Disk geometry packet */
	struct {
		unsigned short len;      /* Packet length */
		unsigned short flags;    /* Packet flags */
		unsigned int cyls;       /* Cylinders */
		unsigned int heads;      /* Heads */
		unsigned int secs;       /* Sectors per track */
		unsigned long long size; /* Total sectors */
		unsigned short secsz;    /* Sector size */
		void *edd;               /* EDD pointer */
	} __attribute__((packed, aligned(16))) geo;
	unsigned char dn; /* Disk number */
} diskbios_t;


struct {
	/* Disks info */
	diskbios_t disks[DISKBIOS_MAX_CNT];

	/* Read cache handling */
	unsigned char lrdn; /* Last read disk */
	unsigned int lrc;   /* Last read cylinder */
	unsigned int lrh;   /* Last read head */
	unsigned int lrp;   /* Last read rcache position */

	/* Write cache handling */
	unsigned char lwdn; /* Last written disk */
	unsigned int lwc;   /* Last written cylinder */
	unsigned int lwh;   /* Last written head */
	unsigned int lwp;   /* Last written wcache position */
	unsigned int lwb;   /* Last wcache segment begin position */
	unsigned int lwe;   /* Last wcache segment end position */

	/* Disk caches */
	char *const rcache; /* Read cache */
	char *const wcache; /* Write cache */
} diskbios_common = {
	.lrdn = -1,
	.lwdn = -1,
	.rcache = (char *const)ADDR_RCACHE,
	.wcache = (char *const)ADDR_WCACHE
};


/* Retrieves disk geometry */
static int diskbios_geometry(diskbios_t *disk)
{
	int ret = ((unsigned int)&disk->geo & 0xffff0000) >> 4;

	disk->geo.len = sizeof(disk->geo);
	disk->geo.secsz = SIZE_BLOCK;

	__asm__ volatile(
		/* Extended read disk parameters */
		"pushl $0x13; "
		"pushl %%eax; "
		"pushl $0x0; "
		"movb $0x48, %%ah; "
		"call _interrupts_bios; "
		"jnc 0f; "
		/* Fallback to CHS read disk parameters */
		"xorw %%di, %%di; "
		"movb $0x8, %%ah; "
		"call _interrupts_bios; "
		"jc 1f; "
		/* Store cylinders */
		"xorl %%eax, %%eax; "
		"movb %%ch, %%al; "
		"movb %%cl, %%ah; "
		"andb $0xc0, %%ah; "
		"rolb $0x2, %%ah; "
		"incl %%eax; "
		"movl %%eax, 4(%%esi); "
		/* Store heads */
		"xorl %%eax, %%eax; "
		"movb %%dh, %%al; "
		"incl %%eax; "
		"movl %%eax, 8(%%esi); "
		/* Store sectors */
		"xorl %%eax, %%eax; "
		"movb %%cl, %%al; "
		"andb $0x3f, %%al; "
		"movl %%eax, 12(%%esi); "
		"0: "
		"xorl %%eax, %%eax; "
		"1: "
		"addl $0xc, %%esp; "
	: "+a" (ret)
	: "d" (disk->dn), "S" (&disk->geo)
	: "ebx", "ecx", "edi", "memory", "cc");

	return ret;
}


/* Performs read/write access to disk */
static int diskbios_access(diskbios_t *disk, unsigned char mode, unsigned int c, unsigned int h, unsigned int s, unsigned char n, char *buff)
{
	/* Disk Address Packet */
	struct {
		unsigned char len;      /* Packet length */
		unsigned char res;      /* Unused byte */
		unsigned short secs;    /* Sectors */
		unsigned short offs;    /* Buffer offset */
		unsigned short seg;     /* Buffer segment */
		unsigned long long sec; /* Sector (LBA) */
	} __attribute__((packed, aligned(16))) dap;
	int ret = ((unsigned int)&dap & 0xffff0000) >> 4;

	/* Initialize DAP */
	dap.len = sizeof(dap);
	dap.res = 0;
	dap.secs = n;
	dap.offs = (unsigned int)buff;
	dap.seg = ((unsigned int)buff & 0xffff0000) >> 4;
	dap.sec = ((unsigned long long)c * disk->geo.heads + h) * disk->geo.secs + (s - 1);

	__asm__ volatile(
		/* Extended read/write sectors */
		"pushl $0x13; "
		"pushl %%eax; "
		"pushl $0x0; "
		"movw %%di, %%ax; "
		"call _interrupts_bios; "
		"jnc 0f; "
		/* Fallback to CHS read/write sectors */
		"addl $0xc, %%esp; "
		"movb %5, %%dh; "
		"movw %4, %%cx; "
		"xchgb %%cl, %%ch; "
		"rorb $0x2, %%cl; "
		"movb %6, %%al; "
		"orb %%al, %%cl; "
		"movw 4(%%esi), %%bx; "
		"movw %%di, %%ax; "
		"orw 2(%%esi), %%ax; "
		"andb $0x3, %%ah; "
		"pushl $0x13; "
		"pushl $0x0; "
		"pushl 6(%%esi); "
		"call _interrupts_bios; "
		"jc 1f; "
		"0: "
		"xorl %%eax, %%eax; "
		"1: "
		"addl $0xc, %%esp; "
	: "+a" (ret)
	: "d" (disk->dn), "S" (&dap), "D" ((unsigned int)mode << 8), "m" (c), "m" (h), "m" (s)
	: "ebx", "ecx", "memory", "cc");

	return ret;
}


/* Returns disk instance */
static diskbios_t *diskbios_get(unsigned int minor)
{
	diskbios_t *disk;

	if (minor >= DISKBIOS_MAX_CNT)
		return NULL;
	disk = &diskbios_common.disks[minor];

	return (disk->dn == -1) ? NULL : disk;
}


static ssize_t diskbios_read(unsigned int minor, addr_t offs, void *buff, size_t len, time_t timeout)
{
	diskbios_t *disk;
	unsigned long long sb, eb;
	unsigned int c, h, s, p;
	size_t size, l = 0;

	if ((disk = diskbios_get(minor)) == NULL)
		return -EINVAL;

	if (!len)
		return 0;

	/* Calculate start and end blocks */
	sb = offs / SIZE_BLOCK;
	eb = (offs + len - 1) / SIZE_BLOCK;

	for (; sb <= eb; sb++) {
		c = sb / disk->geo.secs / disk->geo.heads;
		h = sb / disk->geo.secs % disk->geo.heads;
		s = sb % disk->geo.secs;
		p = s / BLOCKS_RCACHE * BLOCKS_RCACHE;

		/* Read compact track segment from disk to cache */
		if ((disk->dn != diskbios_common.lrdn) || (c != diskbios_common.lrc) || (h != diskbios_common.lrh) || (p != diskbios_common.lrp)) {
			if (diskbios_access(disk, DISK_READ, c, h, p + 1, min(BLOCKS_RCACHE, disk->geo.secs - p), diskbios_common.rcache))
				return -EIO;

			diskbios_common.lrdn = disk->dn;
			diskbios_common.lrc = c;
			diskbios_common.lrh = h;
			diskbios_common.lrp = p;
		}

		/* Read data from cache */
		size = (sb == eb) ? len - l : SIZE_BLOCK - offs % SIZE_BLOCK;
		hal_memcpy(buff, diskbios_common.rcache + s % BLOCKS_RCACHE * SIZE_BLOCK + offs % SIZE_BLOCK, size);
		buff += size;
		offs += size;
		l += size;
	}

	return l;
}


/* Writes character to terminal output */
static ssize_t diskbios_write(unsigned int minor, addr_t offs, const void *buff, size_t len)
{
	diskbios_t *disk;
	unsigned long long sb, eb;
	unsigned int c, h, s, b, p;
	size_t size, l = 0;

	if ((disk = diskbios_get(minor)) == NULL)
		return -EINVAL;

	if (!len)
		return 0;

	/* Calculate start and end blocks */
	sb = offs / SIZE_BLOCK;
	eb = (offs + len - 1) / SIZE_BLOCK;

	for (; sb <= eb; sb++) {
		c = sb / disk->geo.secs / disk->geo.heads;
		h = sb / disk->geo.secs % disk->geo.heads;
		s = sb % disk->geo.secs;
		b = s % BLOCKS_WCACHE;
		p = s / BLOCKS_WCACHE * BLOCKS_WCACHE;

		/* Write compact track segment from cache to disk */
		if ((diskbios_common.lwb != diskbios_common.lwe) &&
			((disk->dn != diskbios_common.lwdn) || (c != diskbios_common.lwc) || (h != diskbios_common.lwh) || (p != diskbios_common.lwp) || (b + 1 < diskbios_common.lwb) || (b > diskbios_common.lwe))) {
			if (diskbios_access(disk, DISK_WRITE,
				diskbios_common.lwc, diskbios_common.lwh, diskbios_common.lwp + diskbios_common.lwb + 1,
				diskbios_common.lwe - diskbios_common.lwb, diskbios_common.wcache + diskbios_common.lwb * SIZE_BLOCK))
				return -EIO;

			/* Mark cache empty */
			diskbios_common.lwb = diskbios_common.lwe = 0;
		}

		/* Write data to cache */
		size = (sb == eb) ? len - l : SIZE_BLOCK - offs % SIZE_BLOCK;
		hal_memcpy(diskbios_common.wcache + b * SIZE_BLOCK + offs % SIZE_BLOCK, buff, size);
		buff += size;
		offs += size;
		l += size;

		/* Update cache info */
		diskbios_common.lwdn = disk->dn;
		diskbios_common.lwc = c;
		diskbios_common.lwh = h;
		diskbios_common.lwp = p;

		if (diskbios_common.lwb == diskbios_common.lwe) {
			diskbios_common.lwb = b;
			diskbios_common.lwe = b + 1;
		}
		else if (b + 1 == diskbios_common.lwb) {
			diskbios_common.lwb--;
		}
		else if (b == diskbios_common.lwe) {
			diskbios_common.lwe++;
		}
	}

	return l;
}


static int diskbios_sync(unsigned int minor)
{
	diskbios_t *disk;

	if ((disk = diskbios_get(minor)) == NULL)
		return -EINVAL;

	if ((diskbios_common.lwb != diskbios_common.lwe) && (disk->dn == diskbios_common.lwdn)) {
		if (diskbios_access(disk, DISK_WRITE,
			diskbios_common.lwc, diskbios_common.lwh, diskbios_common.lwp + diskbios_common.lwb + 1,
			diskbios_common.lwe - diskbios_common.lwb, diskbios_common.wcache + diskbios_common.lwb * SIZE_BLOCK))
			return -EIO;

		/* Mark cache empty */
		diskbios_common.lwb = diskbios_common.lwe = 0;
	}

	return EOK;
}


static int diskbios_map(unsigned int minor, addr_t addr, size_t sz, int mode, addr_t memaddr, size_t memsz, int memmode, addr_t *a)
{
	diskbios_t *disk;

	if ((disk = diskbios_get(minor)) == NULL)
		return -EINVAL;

	/* Device mode cannot be higher than map mode to copy data */
	if ((mode & memmode) != mode)
		return -EINVAL;

	return dev_isNotMappable;
}


static int diskbios_done(unsigned int minor)
{
	return diskbios_sync(minor);
}


static int diskbios_init(unsigned int minor)
{
	diskbios_t *disk;

	if ((disk = diskbios_get(minor)) == NULL)
		return -EINVAL;

	disk->dn = minor;

	/* Correct hard drive disk number */
	if (minor >= DISKBIOS_FLOPPY_CNT)
		disk->dn += 0x80 - DISKBIOS_FLOPPY_CNT;

	/* Get disk geometry */
	if (diskbios_geometry(disk)) {
		/* Mark disk not available */
		disk->dn = -1;
		return -ENOENT;
	}

	return EOK;
}


__attribute__((constructor)) static void ttydisk_register(void)
{
	static const dev_handler_t h = {
		.read = diskbios_read,
		.write = diskbios_write,
		.sync = diskbios_sync,
		.map = diskbios_map,
		.done = diskbios_done,
		.init = diskbios_init,
	};

	devs_register(DEV_DISK, DISKBIOS_MAX_CNT, &h);
}
