/* 
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * Disk routines
 *
 * Copyright 2001, 2005 Pawel Pisarczyk
 *
 * This file is part of Phoenix-RTOS.
 *
 * Phoenix-RTOS is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Phoenix-RTOS kernel is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Phoenix-RTOS kernel; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _DISK_H_
#define _DISK_H_


extern s32 disk_open(u16 dn, char *name, u32 flags);


/* Function reads bytes from file specified by starting block number (handle) */
extern s32 disk_read(u16 dn, s32 handle, u32 *pos, u8 *buff, u32 len);


extern s32 disk_close(u16 dn, s32 handle);


#endif
