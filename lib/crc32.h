/*
 * Phoenix-RTOS
 *
 * crc32
 *
 * Copyright 2023 Phoenix Systems
 * Author: Hubert Badocha
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


/* TODO: provide alternate version if this tool should work on big-endian host */

#include <hal/hal.h>


u32 lib_crc32(const u8 *buf, u32 len, u32 base);
