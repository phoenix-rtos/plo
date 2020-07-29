/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * i.MX RT ROM API driver for FlexSPI
 *
 * Copyright 2019, 2020 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#include "rom_api.h"


#define FLEXSPI_DRIVER_API_ADDRESS 0x00201a60

typedef struct {
    u32 version;
    s32 (*init)(u32 instance, flexspi_norConfig_t *config);
    s32 (*program)(u32 instance, flexspi_norConfig_t *config, u32 dstAddr, const u32 *src);
    s32 (*erase_all)(u32 instance, flexspi_norConfig_t *config);
    s32 (*erase)(u32 instance, flexspi_norConfig_t *config, u32 start, u32 lengthInBytes);
    s32 (*read)(u32 instance, flexspi_norConfig_t *config, u32 *dst, u32 addr, u32 lengthInBytes);
    void (*clear_cache)(u32 instance);
    s32 (*xfer)(u32 instance, flexspi_xfer_t *xfer);
    s32 (*update_lut)(u32 instance, u32 seqIndex, const u32 *lutBase, u32 seqNumber);
    s32 (*get_config)(u32 instance, flexspi_norConfig_t *config, serial_norConfigOption_t *option);
} flexspi_norDriverInterface_t;


static volatile flexspi_norDriverInterface_t *flexspi_norApi = (void *)FLEXSPI_DRIVER_API_ADDRESS;


int flexspi_norFlashInit(u32 instance, flexspi_norConfig_t *config)
{
    return flexspi_norApi->init(instance, config);
}


int flexspi_norFlashPageProgram(u32 instance, flexspi_norConfig_t *config, u32 dstAddr, const u32 *src)
{
    return flexspi_norApi->program(instance, config, dstAddr, src);
}


int flexspi_norFlashEraseAll(u32 instance, flexspi_norConfig_t *config)
{
    return flexspi_norApi->erase_all(instance, config);
}


int flexspi_norGetConfig(u32 instance, flexspi_norConfig_t *config, serial_norConfigOption_t *option)
{
    return flexspi_norApi->get_config(instance, config, option);
}


int flexspi_norFlashErase(u32 instance, flexspi_norConfig_t *config, u32 start, u32 length)
{
    return flexspi_norApi->erase(instance, config, start, length);
}


int flexspi_norFlashRead(u32 instance, flexspi_norConfig_t *config, char *dst, u32 start, u32 bytes)
{
    return flexspi_norApi->read(instance, config, (u32 *)dst, start, bytes);
}


int flexspi_norFlashUpdateLUT(u32 instance, u32 seqIndex, const u32 *lutBase, u32 seqNumber)
{
    return flexspi_norApi->update_lut(instance, seqIndex, lutBase, seqNumber);
}


int flexspi_norFlashExecuteSeq(u32 instance, flexspi_xfer_t *xfer)
{
    return flexspi_norApi->xfer(instance, xfer);
}
