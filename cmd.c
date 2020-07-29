/*
 * Phoenix-RTOS
 *
 * plo - perating system loader
 *
 * Loader commands
 *
 * Copyright 2012, 2017, 2020 Phoenix Systems
 * Copyright 2001, 2005 Pawel Pisarczyk
 * Author: Pawel Pisarczyk, Pawel Kolodziej, Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "errors.h"
#include "low.h"
#include "plostd.h"
#include "phfs.h"
#include "elf.h"
#include "cmd.h"


struct {
    void (*f)(char *);
    char *cmd;
    char *help;
} cmds[] = {
    { cmd_dump,    "dump", "    - dumps memory, usage: dump <segment>:<offset>" },
    { cmd_go,      "go!", "     - starts Phoenix-RTOS loaded into memory" },
    { cmd_help,    "help", "    - prints this message" },
    { cmd_copy,    "copy", "    - copies data between devices, usage:\n           copy <src device> <src file/LBA> <dst device> <dst file/LBA> [<len>]" },
    { cmd_load,    "load", "    - loads Phoenix-RTOS, usage: load [<boot device>] [<kernel args>]" },
    { cmd_memmap,  "mem", "     - prints physical memory map" },
    { cmd_cmd,     "cmd", "     - boot command, usage: cmd [<command>]" },
    { cmd_timeout, "timeout", " - boot timeout, usage: timeout [<timeout>]" },
    { cmd_save,    "save", "    - saves configuration" },
    { cmd_lspci,   "lspci", "   - enumerates PCI buses" },
    { NULL, NULL, NULL }
};



/* Function parses loader commands */
void cmd_parse(char *line)
{
    int k;
    char word[LINESZ + 1], cmd[LINESZ + 1];
    unsigned int p = 0, wp;

    for (;;) {
        if (cmd_getnext(line, &p, ";", DEFAULT_CITES, word, sizeof(word)) == NULL) {
            plostd_printf(ATTR_ERROR, "\nSyntax error!\n");
            return;
        }
        if (*word == 0)
            break;

        wp = 0;
        if (cmd_getnext(word, &wp, DEFAULT_BLANKS, DEFAULT_CITES, cmd, sizeof(cmd)) == NULL) {
            plostd_printf(ATTR_ERROR, "\nSyntax error!\n");
            return;
        }

        /* Find command and launch associated function */
        for (k = 0; cmds[k].cmd != NULL; k++) {

            if (!plostd_strcmp(cmd, cmds[k].cmd)) {
                cmds[k].f(word + wp);
                break;
            }
        }
        if (!cmds[k].cmd)
            plostd_printf(ATTR_ERROR, "\n'%s' - unknown command!\n", cmd);
    }

    return;
}


/* Generic command handlers */

void cmd_help(char *s)
{
    int k;

    plostd_printf(ATTR_LOADER, "\n");
    plostd_printf(ATTR_LOADER, "Loader commands:\n");
    plostd_printf(ATTR_LOADER, "----------------\n");

    for (k = 0; cmds[k].cmd; k++)
        plostd_printf(ATTR_LOADER, "%s %s\n", cmds[k].cmd, cmds[k].help);
    return;
}


/* Function setups boot timeout */
void cmd_timeout(char *s)
{
    char word[LINESZ + 1];
    unsigned int p = 0;

    plostd_printf(ATTR_LOADER, "\n");

    if (cmd_getnext(s, &p, DEFAULT_BLANKS, DEFAULT_CITES, word, sizeof(word)) == NULL) {
        plostd_printf(ATTR_ERROR, "Syntax error!\n");
        return;
    }
    if (*word)
        _plo_timeout = plostd_ahtoi(word);

    plostd_printf(ATTR_LOADER, "timeout=0x%x\n", _plo_timeout);
    return;
}


void cmd_go(char *s)
{
    plostd_printf(ATTR_LOADER, "\nStarting Phoenix-RTOS...\n");
    low_launch();

    return;
}


/* Function setups boot command */
void cmd_cmd(char *s)
{
    unsigned int p = 0;
    int l;

    plostd_printf(ATTR_LOADER, "\n");
    cmd_skipblanks(s, &p, DEFAULT_BLANKS);
    s += p;

    if (*s) {
        low_memcpy(_plo_command, s, l = min(plostd_strlen(s), CMD_SIZE - 1));
        *((char *)_plo_command + l) = 0;
    }

    plostd_printf(ATTR_LOADER, "cmd=%s\n", (char *)_plo_command);

    return;
}



/* Auxiliary functions */

/* Function prints progress indicator */
void cmd_showprogress(u32 p)
{
    char *states = "-\\|/";

    plostd_printf(ATTR_LOADER, "%c%c", 8, states[p % plostd_strlen(states)]);
    return;
}


/* Function skips blank characters */
void cmd_skipblanks(char *line, unsigned int *pos, char *blanks)
{
    char c, blfl;
    unsigned int i;

    while ((c = *((char *)(line + *pos))) != 0) {
        blfl = 0;
        for (i = 0; i < plostd_strlen(blanks); i++) {
            if (c == *(char *)(blanks + i)) {
                blfl = 1;
                break;
            }
        }
        if (!blfl)
            break;
        (*pos)++;
    }

    return;
}


/* Function retrieves next symbol from line */
char *cmd_getnext(char *line, unsigned int *pos, char *blanks, char *cites, char *word, unsigned int len)
{
    char citefl = 0, c;
    unsigned int i, wp = 0;

    /* Skip leading blank characters */
    cmd_skipblanks(line, pos, blanks);

    wp = 0;
    while ((c = *(char *)(line + *pos)) != 0) {

        /* Test cite characters */
        if (cites) {
            for (i = 0; cites[i]; i++) {
                if (c != cites[i])
                    continue;
                citefl ^= 1;
                break;
            }

            /* Go to next iteration if cite character found */
            if (cites[i]) {
                (*pos)++;
                continue;
            }
        }

        /* Test separators */
        for (i = 0; blanks[i]; i++) {
            if (c != blanks[i])
                continue;
            break;
        }
        if (!citefl && blanks[i])
            break;

        word[wp++] = c;
        if (wp == len)
            return NULL;

        (*pos)++;
    }

    if (citefl)
        return NULL;

    word[wp] = 0;
    return word;
}
