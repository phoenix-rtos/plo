/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * Loader console
 *
 * Copyright 2012, 2017, 2020-2021 Phoenix Systems
 * Copyright 2001, 2005 Pawel Pisarczyk
 * Authors: Pawel Pisarczyk, Lukasz Kosinski, Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "cmd.h"
#include "hal.h"
#include "plostd.h"
#include "console.h"

struct {
	int  ll;
	int  cl;
	char lines[HISTSZ][LINESZ + 1];
} history;



void plo_drawspaces(char attr, unsigned int len)
{
	unsigned int k;

	plostd_setattr(attr);
	for (k = 0; k < len; k++)
		console_putc(' ');
}


/* TODO: clean up and move to cmds */
void plo_cmdloop(void)
{
	char c = 0, sc = 0;
	int pos = 0;
	int k, chgfl = 0, ncl;

	history.ll = 0;
	history.cl = 0;

	for (k = 0; k < HISTSZ; k++)
		history.lines[k][0] = 0;

	plostd_printf(ATTR_LOADER, "%s", PROMPT);
	while (c != '#') {
		while (console_getc(&c) <= 0)
			;

		sc = 0;
		/* Translate backspace */
		if (c == 127) {
			c = 8;
		}
		/* Simple parser for VT100 commands */
		else if (c == 27) {
			while (console_getc(&c) <= 0)
				;

			switch (c) {
			case 91:
				while (console_getc(&c) <= 0)
					;

				switch (c) {
				case 'A':             /* UP */
					sc = 72;
					break;
				case 'B':             /* DOWN */
					sc = 80;
					break;
				}
				break;
			}
			c = 0;
		}

		/* Regular characters */
		if (c) {
			if (c == '\r') {
				if (pos) {
					history.lines[history.ll][pos] = 0;

					cmd_parse(history.lines[history.ll]);

					history.ll = (history.ll + 1) % HISTSZ;
					history.cl = history.ll;
				}
				pos = 0;
				plostd_printf(ATTR_LOADER, "\n%s", PROMPT);
				continue;
			}

			/* If character isn't backspace add it to line buffer */
			if ((c != 8) && (pos < LINESZ)) {
				plostd_setattr(ATTR_USER);
				console_putc(c);
				history.lines[history.ll][pos++] = c;
				history.lines[history.ll][pos] = 0;
			}

			/* Remove character before cursor */
			if ((c == 8) && (pos > 0)) {
				history.lines[history.ll][--pos] = 0;
				plostd_printf(ATTR_USER, "%c %c", 8, 8);
			}
		}
		/* Control characters */
		else {
			switch (sc) {
			case 72:
				ncl = ((history.cl + HISTSZ - 1) % HISTSZ);
				if ((ncl != history.ll) && (history.lines[ncl][0])) {
					history.cl = ncl;
					hal_memcpy(history.lines[history.ll], history.lines[history.cl], plostd_strlen(history.lines[history.cl]) + 1);
					chgfl = 1;
				}
				break;

			case 80:
				ncl = ((history.cl + 1) % HISTSZ);
				if (history.cl != history.ll) {
					history.cl = ncl;
					chgfl = 1;

					if (ncl != history.ll)
						hal_memcpy(history.lines[history.ll], history.lines[history.cl], plostd_strlen(history.lines[history.cl]) + 1);
					else
						history.lines[history.ll][0] = 0;
				}
				break;
			}

			if (chgfl) {
				plostd_printf(ATTR_LOADER, "\r%s", PROMPT);
				plo_drawspaces(ATTR_USER, pos);
				pos = plostd_strlen(history.lines[history.ll]);
				plostd_printf(ATTR_LOADER, "\r%s", PROMPT);
				plostd_printf(ATTR_USER, "%s", history.lines[history.ll]);
				chgfl = 0;
			}
		}
	}
}


void plo_wait(void)
{
	int i, t;
	char c = 0;

	plostd_printf(ATTR_NONE, "\n");
	for (t = hal_getLaunchTimeout(); t; t--) {
		plostd_printf(ATTR_LOADER, "\rWaiting for keyboard, %d seconds", t);
		for (i = 0; i < 1000; i+= CONSOLE_TIMEOUT_MS)
			if (console_getc(&c) > 0)
				break;
	}
	plostd_printf(ATTR_LOADER, "\rWaiting for keyboard, %d seconds", t);

	if (t == 0) {
		/* TODO: run user script */
	}

	plostd_printf(ATTR_INIT, "\n");
}


void plo_init(void)
{
	hal_init();
	console_init();

	plostd_printf(ATTR_LOADER, "\nPhoenix-RTOS loader v. 1.21\n");
	devs_init();
	cmd_init();

	/* Run base script
	 * TODO: change function's name */
	cmd_default();

	/* Wait for interaction with user, otherwise run user script */
	plo_wait();

	/* Enter to interactive mode */
	plo_cmdloop();

	devs_done();
	hal_done();
}
