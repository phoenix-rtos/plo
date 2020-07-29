/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * Loader commands
 *
 * Copyright 2020 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "../errors.h"
#include "../plostd.h"
#include "../phfs.h"
#include "../low.h"
#include "../cmd.h"
#include "../elf.h"

#include "config.h"


struct {
    char *name;
    unsigned int pdn;
} devices[] = {
    { "flash0", PDN_FLASH0 },
    { "flash1", PDN_FLASH1 },
    { "com1", PDN_COM1 },
    { "com2", PDN_COM2 },
    { "usb", PDN_USB },
    { NULL, NULL }
};


void cmd_dump(char *s)
{
    /* TODO */
    plostd_printf(ATTR_LOADER, "\nDUMP in progress...\n");
}


void cmd_loadkernel(unsigned int pdn, char *arg, u16 *po)
{
    int err;
    u32 offs;
    Elf32_Word k;
    Elf32_Ehdr hdr;
    Elf32_Phdr phdr;

    if ((err = phfs_open(pdn, NULL, 0)) < 0) {
        plostd_printf(ATTR_ERROR, "Cannot initialize flash memory!\n");
        return;
    }

    /* Read ELF header */
    /* TODO: get kernel adress from partition table */
    offs = KERNEL_BASE;
    if (phfs_read(pdn, 0, &offs, (u8 *)&hdr, (u32)sizeof(Elf32_Ehdr)) < 0) {
        plostd_printf(ATTR_ERROR, "Can't read ELF header!\n");
        return;
    }

    if ((hdr.e_ident[0] != 0x7f) && (hdr.e_ident[1] != 'E') && (hdr.e_ident[2] != 'L') && (hdr.e_ident[3] != 'F')) {
        plostd_printf(ATTR_ERROR, "File isn't ELF object!\n");
        return;
    }

    /* Read program segments */
    for (k = 0; k < hdr.e_phnum; k++) {
        offs = KERNEL_BASE + hdr.e_phoff + k * sizeof(Elf32_Phdr);
        if (phfs_read(pdn, 0, &offs , (u8 *)&phdr, (u32)sizeof(Elf32_Phdr)) < 0) {
            plostd_printf(ATTR_ERROR, "Can't read Elf32_Phdr, k=%d!\n", k);
            return;
        }

        if ((phdr.p_type == PT_LOAD) && (phdr.p_paddr != 0)) {
            /* TODO */
        }
    }
    plostd_printf(ATTR_LOADER, "\nLoad kernel command is in progress...\n");

    return;
}


void cmd_load(char *s)
{
    char word[LINESZ + 1];
    unsigned int p = 0, dn;
    u16 po;


    cmd_skipblanks(s, &p, DEFAULT_BLANKS);
    if (cmd_getnext(s, &p, DEFAULT_BLANKS, NULL, word, sizeof(word)) == NULL) {
        plostd_printf(ATTR_ERROR, "\nSize error!\n");
        return;
    }

    /* Show boot devices if parameter is empty */
    if (*word == 0) {
        plostd_printf(ATTR_LOADER, "\nBoot devices: ");
        for (dn = 0; devices[dn].name; dn++)
            plostd_printf(ATTR_LOADER, "%s ", devices[dn].name);
        plostd_printf(ATTR_LOADER, "\n");
        return;
    }

    for (dn = 0; devices[dn].name; dn++)  {
        if (!plostd_strcmp(word, devices[dn].name))
            break;
    }

    if (!devices[dn].name) {
        plostd_printf(ATTR_ERROR, "\n'%s' - unknown boot device!\n", word);
        return;
    }

    /* Load kernel */
    plostd_printf(ATTR_LOADER, "\nLoading kernel\n");

    cmd_loadkernel(devices[dn].pdn, NULL, &po);

    /* TODO */

    return;
}


void cmd_copy(char *s)
{
    /* TODO */
    plostd_printf(ATTR_LOADER, "\nCOPY in progress...\n");
}


void cmd_memmap(char *s)
{
    /* TODO */
    plostd_printf(ATTR_LOADER, "\nMEMMAP in progress...\n");
}



/* Function saves boot configuration */
void cmd_save(char *s)
{
    /* TODO */
    plostd_printf(ATTR_LOADER, "\nSAVE in progress...\n");
}


void cmd_lspci(char *s)
{
    plostd_printf(ATTR_LOADER, "\nlspci is not supported on this board\n");
}

