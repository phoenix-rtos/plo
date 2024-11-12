/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * ARMv8 cache management
 *
 * Copyright 2022 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _CACHE_H_
#define _CACHE_H_


extern void hal_dcacheInvalAll(void);


extern void hal_dcacheInval(addr_t start, addr_t end);


extern void hal_icacheInval(void);


extern void hal_dcacheClean(addr_t start, addr_t end);


extern void hal_dcacheFlush(addr_t start, addr_t end);


extern void hal_dcacheEnable(unsigned int mode);


extern void hal_icacheEnable(unsigned int mode);

#endif /* _CACHE_H_ */
