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

#ifndef _FLEXSPI_ROM_API_H_
#define _FLEXSPI_ROM_API_H_

#include "../types.h"

typedef struct {
    volatile u32 option0;
    volatile u32 option1;
} serial_norConfigOption_t;


typedef enum {
    kFlexSpiOperation_Command,                 /* FlexSPI operation: Only command, both TX and RX buffer are ignored. */
    kFlexSpiOperation_Config,                  /* FlexSPI operation: Configure device mode, the TX FIFO size is fixed in LUT. */
    kFlexSpiOperation_Write,                   /* FlexSPI operation: Write, only TX buffer is effective */
    kFlexSpiOperation_Read,                    /* FlexSPI operation: Read, only Rx Buffer is effective. */
    kFlexSpiOperation_End = kFlexSpiOperation_Read,
} flexspi_operation_t;


typedef struct {
    flexspi_operation_t operation;             /* FlexSPI operation */
    u32 baseAddress;                      /* FlexSPI operation base address */
    u32 seqId;                            /* Sequence Id */
    u32 seqNum;                           /* Sequence Number */
    u8 isParallelModeEnable;              /* Is a parallel transfer */
    u32 *txBuffer;                        /* Tx buffer */
    u32 txSize;                           /* Tx size in bytes */
    u32 *rxBuffer;                        /* Rx buffer */
    u32 rxSize;                           /* Rx size in bytes */
} flexspi_xfer_t;


typedef struct {
    u8 seqNum;                             /* Sequence Number, valid number: 1-16 */
    u8 seqId;                              /* Sequence Index, valid number: 0-15 */
    u16 reserved;
} flexspi_lutSeq_t;


typedef struct {
    u32 tag;                               /* Tag, fixed value 0x42464346UL */
    u32 version;                           /* Version,[31:24] -'V', [23:16] - Major, [15:8] - Minor, [7:0] - bugfix */
    u32 reserved0;                         /* Reserved for future use */
    u8 readSampleClkSrc;                   /* Read Sample Clock Source, valid value: 0/1/3 */
    u8 csHoldTime;                         /* CS hold time, default value: 3 */
    u8 csSetupTime;                        /* CS setup time, default value: 3 */
    u8 columnAddressWidth;                 /* Column Address with, for HyperBus protocol, it is fixed to 3, For */
    u8 deviceModeCfgEnable;                /* Device Mode Configure enable flag, 1 - Enable, 0 - Disable */
    u8 deviceModeType;                     /* Specify the configuration command type:Quad Enable, DPI/QPI/OPI switch, */
    u16 waitTimeCfgCommands;               /* Wait time for all configuration commands, unit: 100us, Used for */
    flexspi_lutSeq_t deviceModeSeq;             /* Device mode sequence info, [7:0] - LUT sequence id, [15:8] - LUt */
    u32 deviceModeArg;                     /* Argument/Parameter for device configuration */
    u8 configCmdEnable;                    /* Configure command Enable Flag, 1 - Enable, 0 - Disable */
    u8 configModeType[3];                  /* Configure Mode Type, similar as deviceModeTpe */
    flexspi_lutSeq_t configCmdSeqs[3];          /* Sequence info for Device Configuration command, similar as deviceModeSeq */
    u32 reserved1;                         /* Reserved for future use */
    u32 configCmdArgs[3];                  /* Arguments/Parameters for device Configuration commands */
    u32 reserved2;                         /* Reserved for future use */
    u32 controllerMiscOption;              /* Controller Misc Options, see Misc feature bit definitions for more */
    u8 deviceType;                         /* Device Type:  See Flash Type Definition for more details */
    u8 sflashPadType;                      /* Serial Flash Pad Type: 1 - Single, 2 - Dual, 4 - Quad, 8 - Octal */
    u8 serialClkFreq;                      /* Serial Flash Frequencey, device specific definitions, See System Boot */
    u8 lutCustomSeqEnable;                 /* LUT customization Enable, it is required if the program/erase cannot */
    u32 reserved3[2];                      /* Reserved for future use */
    u32 sflashA1Size;                      /* Size of Flash connected to A1 */
    u32 sflashA2Size;                      /* Size of Flash connected to A2 */
    u32 sflashB1Size;                      /* Size of Flash connected to B1 */
    u32 sflashB2Size;                      /* Size of Flash connected to B2 */
    u32 csPadSettingOverride;              /* CS pad setting override value */
    u32 sclkPadSettingOverride;            /* SCK pad setting override value */
    u32 dataPadSettingOverride;            /* data pad setting override value */
    u32 dqsPadSettingOverride;             /* DQS pad setting override value */
    u32 timeoutInMs;                       /* Timeout threshold for read status command */
    u32 commandInterval;                   /* CS deselect interval between two commands */
    u16 dataValidTime[2];                  /* CLK edge to data valid time for PORT A and PORT B, in terms of 0.1ns */
    u16 busyOffset;                        /* Busy offset, valid value: 0-31 */
    u16 busyBitPolarity;                   /* Busy flag polarity, 0 - busy flag is 1 when flash device is busy, 1 - */
    u32 lut[64];                           /* Lookup table holds Flash command sequences */
    flexspi_lutSeq_t lutCustomSeq[12];          /* Customizable LUT Sequences */
    u32 reserved4[4];                      /* Reserved for future use */
} flexspi_memConfig_t;


typedef struct {
    volatile flexspi_memConfig_t mem;           /* Common memory configuration info via FlexSPI */
    u32 pageSize;                          /* Page size of Serial NOR */
    u32 sectorSize;                        /* Sector size of Serial NOR */
    u8 ipcmdSerialClkFreq;                 /* Clock frequency for IP command */
    u8 isUniformBlockSize;                 /* Sector/Block size is the same */
    u8 reserved0[2];                       /* Reserved for future use */
    u8 serialNorType;                      /* Serial NOR Flash type: 0/1/2/3 */
    u8 needExitNoCmdMode;                  /* Need to exit NoCmd mode before other IP command */
    u8 halfClkForNonReadCmd;               /* Half the Serial Clock for non-read command: true/false */
    u8 needRestoreNoCmdMode;               /* Need to Restore NoCmd mode after IP commmand execution */
    u32 blockSize;                         /* Block size */
    u32 reserved2[11];                     /* Reserved for future use */
} flexspi_norConfig_t;


int flexspi_norFlashInit(u32 instance, flexspi_norConfig_t *config);


int flexspi_norFlashPageProgram(u32 instance, flexspi_norConfig_t *config, u32 dstAddr, const u32 *src);


int flexspi_norFlashEraseAll(u32 instance, flexspi_norConfig_t *config);


int flexspi_norGetConfig(u32 instance, flexspi_norConfig_t *config, serial_norConfigOption_t *option);


int flexspi_norFlashErase(u32 instance, flexspi_norConfig_t *config, u32 start, u32 length);


int flexspi_norFlashRead(u32 instance, flexspi_norConfig_t *config, char *dst, u32 start, u32 bytes);


int flexspi_norFlashExecuteSeq(u32 instance, flexspi_xfer_t *xfer);


int flexspi_norFlashUpdateLUT(u32 instance, u32 seqIndex, const u32 *lutBase, u32 seqNumber);


#endif
