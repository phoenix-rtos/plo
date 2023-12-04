/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * RTT pipes: communication through debug probe (driver)
 *
 * Copyright 2023-2024 Phoenix Systems
 * Author: Gerard Swiderski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _RTT_H_
#define _RTT_H_


int rtt_check(int chan);


ssize_t rtt_read(int chan, void *buf, size_t count);


ssize_t rtt_write(int chan, const void *buf, size_t count);


#endif /* end of _RTT_H_ */
