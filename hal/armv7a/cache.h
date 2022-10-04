/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * ARMv7 Cortex-A cache management
 *
 * Copyright 2022 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


extern void hal_dcacheEnable(unsigned int mode);


extern void hal_dcacheInvalAll(void);


extern void hal_dcacheInval(addr_t start, addr_t end);


extern void hal_dcacheClean(addr_t start, addr_t end);


extern void hal_dcacheFlush(addr_t start, addr_t end);


extern void hal_icacheEnable(unsigned int mode);


extern void hal_icacheInval(void);
