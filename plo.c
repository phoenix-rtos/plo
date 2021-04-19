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
#include "devs.h"
#include "phfs.h"
#include "timer.h"
#include "plostd.h"
#include "uart.h"

#include "config.h"


struct {
	int  ll;
	int  cl;
	char lines[HISTSZ][LINESZ + 1];
} history;



void plo_drawspaces(char attr, unsigned int len)
{
	unsigned int k;

	hal_setattr(attr);
	for (k = 0; k < len; k++)
		hal_putc(' ');
	return;
}


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
		hal_getc(&c, &sc);

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
				hal_setattr(ATTR_USER);
				hal_putc(c);
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
	return;
}


void plo_init(void)
{
	u16 t = 0;

	hal_init();
	devs_init();
	// phfs_init();
	cmd_init();

	plostd_printf(ATTR_LOADER, "%s \n", PLO_WELCOME);
	plostd_printf(ATTR_INIT, "Detected UART, setting to maximal speed %d Bps\n", uart_getBaudrate());

	/* Wait and execute saved loader command */
	for (t = hal_getLaunchTimeout(); t; t--) {
		plostd_printf(ATTR_INIT, "\r%d seconds to automatic boot      ", t);

		if (timer_wait(1000, TIMER_KEYB, NULL, 0))
			break;
	}

	if (t == 0) {
		plostd_printf(ATTR_INIT, "\n%s\n", PROMPT);
		cmd_default();
	}
	plostd_printf(ATTR_INIT, "\n");

	/* Enter to interactive mode */
	plo_cmdloop();

	hal_done();

	return;
}
