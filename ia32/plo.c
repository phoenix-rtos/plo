/* 
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * Loader console
 *
 * Copyright 2012 Phoenix Systems
 * Copyright 2001, 2005 Pawel Pisarczyk
 * Author: Pawel Pisarczyk
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "config.h"
#include "low.h"
#include "plostd.h"
#include "timer.h"
#include "serial.h"

extern cmds;

struct {
	int  ll;
	int  cl;
	char lines[HISTSZ][LINESZ + 1];
} history;


void plo_drawspaces(char attr, unsigned int len)
{
	unsigned int k;

	for (k = 0; k < len; k++)
		low_putc(attr, ' ');
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
		low_getc(&c, &sc);

		/* Regular characters */
		if (c) {
			if (c == '\r') {
				if (pos) {
					history.lines[history.ll][pos] = 0;

					cmd_parse(history.lines[history.ll]);

					history.ll = (++history.ll % HISTSZ);
					history.cl = history.ll;
				}
				pos = 0;
				plostd_printf(ATTR_LOADER, "\n%s", PROMPT);
				continue;
			}

			/* If character isn't backspace add it to line buffer */
			if ((c != 8) && (pos < LINESZ)) {
				low_putc(ATTR_USER, c);
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
					low_memcpy(history.lines[history.ll], history.lines[history.cl], plostd_strlen(history.lines[history.cl]) + 1);
					chgfl = 1;
				}
				break;

			case 80:
				ncl = ((history.cl + 1) % HISTSZ);
				if (history.cl != history.ll) { 
					history.cl = ncl;
					chgfl = 1;
					
					if (ncl != history.ll)
						low_memcpy(history.lines[history.ll], history.lines[history.cl], plostd_strlen(history.lines[history.cl]) + 1);
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
	u16 t;

	low_init();	
	plostd_printf(ATTR_LOADER, "%s\n", WELCOME);
	
	timer_init();
	serial_init(BPS_115200);
	phfs_init();

	/* Execute loader command */
	for (t = _plo_timeout; t; t--) {
		plostd_printf(ATTR_INIT, "\r%d seconds to automatic boot      ", t);		
		if (timer_wait(1000, TIMER_KEYB, NULL, 0))
			break;
	}
	
	if (t == 0) {
		plostd_printf(ATTR_INIT, "\n%s%s", PROMPT, _plo_command);
		cmd_parse(_plo_command);
	}	
	plostd_printf(ATTR_INIT, "\n");
		
	/* Enter to interactive mode */
	plo_cmdloop();
	
	serial_done();
	timer_done();
	
	low_done();
	return;
}
