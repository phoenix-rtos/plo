/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * ONFI 4.0 definitions
 *
 * Copyright 2026 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */


#ifndef FLASH_ONFI_4_H_
#define FLASH_ONFI_4_H_


#include <types.h>


#define ONFI_TCCS_BASE 500 /* ns, base tCCS value before parameter page read */
#define ONFI_TVDLY     50  /* ns */


typedef struct {
	u32 tADL;           /* Address to Data Loading time (min) */
	u32 tALH;           /* Address Latch Hold time (min) */
	u32 tALS;           /* Address Latch Setup time (min) */
	u32 tAR;            /* ALE to RE# delay (min) */
	u32 tCEA;           /* CE# Access time (max) */
	u32 tCEH;           /* CE# High hold time (min) */
	u32 tCH;            /* CE# Hold time (min) */
	u32 tCHZ;           /* CE# High to Output Hi-Z (max) */
	u32 tCLH;           /* Command Latch Hold time (min) */
	u32 tCLR;           /* CLE to RE# delay (min) */
	u32 tCLS;           /* Command Latch Setup time (min) */
	u32 tCOH;           /* CE# High to Output Hold (min) */
	u32 tCR;            /* CE# Low to RE# Low (min) */
	u32 tCR2;           /* CE# Low to RE# Low 2 (min) */
	u32 tCS;            /* CE# Setup time (min) */
	u32 tCS3;           /* CE# setup time with for data burst after CE# has been high for greater than 1us */
	u32 tDH;            /* Data Hold time (min) */
	u32 tDS;            /* Data Setup time (min) */
	u32 tFEAT;          /* Busy time for Set Features (max) */
	u32 tIR;            /* Output Hi-Z to RE# Low (min) */
	u32 tITC;           /* Interface Transition time (max) */
	u32 tRC;            /* Read Cycle time (min) */
	u32 tREA;           /* RE# Access time (max) */
	u32 tREH;           /* RE# High hold time (min) */
	u32 tRHOH;          /* RE# High to Output Hold (min) */
	u32 tRHW;           /* RE# High to WE# Low (min) */
	u32 tRHZ;           /* RE# High to Output Hi-Z (max) */
	u32 tRLOH;          /* RE# Low to Output Hold (min) */
	u32 tRP;            /* RE# Pulse width (min) */
	u32 tRR;            /* Ready to RE# Low (min) */
	u32 tRST;           /* Device Reset time (max) - raw NAND */
	u32 tRST_EZNAND;    /* Device Reset time (max) - EZ NAND, Reset command */
	u32 tRSTLUN_EZNAND; /* Device Reset time (max) - EZ NAND, Reset LUN command */
	u32 tWB;            /* WE# High to Busy (max) */
	u32 tWC;            /* Write Cycle time (min) */
	u32 tWH;            /* WE# High hold time (min) */
	u32 tWHR;           /* WE# High to RE# Low (min) */
	u32 tWP;            /* WE# Pulse width (min) */
	u32 tWW;            /* WP# High to WE# Low (min) */
} onfi_timingMode_t;


typedef struct {
	/* Revision information and features block */
	u8 signature[4];      /* Parameter page signature */
	u16 revision;         /* Revision number */
	u16 features;         /* Features supported */
	u16 optionalCmds;     /* Optional commands supported */
	u8 primAdvCmdSupport; /* ONFI-JEDEC JTG primary advanced command support */
	u8 res0;
	u16 extParamPageLen; /* Extended parameter page length */
	u8 numOfParamPages;  /* Number of parameter pages */
	u8 res1[17];

	/* Manufacturer information block */
	u8 devManufacturer[12]; /* Device manufacturer */
	u8 devModel[20];        /* Device model */
	u8 jedecId;             /* JEDEC Manufacturer ID */
	u8 dateCode[2];         /* Date code */
	u8 res2[13];

	/* Memory organization block */
	u32 bytesPerPage;             /* Number of data bytes per page */
	u16 spareBytesPerPage;        /* Number of spare bytes per page */
	u32 bytesPerPartialPage;      /* Obsolete: Number of data bytes per partial page */
	u16 spareBytesPerPartialPage; /* Obsolete: Number of spare bytes per partial page */
	u32 pagesPerBlock;            /* Number of pages per block */
	u32 blocksPerLun;             /* Number of blocks per LUN */
	u8 numLuns;                   /* Number of LUN's */
	u8 addrCycles;                /* Number of address cycles */
	u8 bitsPerCell;               /* Number of bits per cell */
	u16 maxBadBlocksPerLun;       /* Bad blocks maximum per LUN */
	u16 blockEndurance;           /* Block endurance */
	u8 guaranteedValidBlock;      /* Guaranteed valid blocks at beginning of target */
	u16 blockEnduranceGvb;        /* Block endurance for guaranteed valid block */
	u8 programsPerPage;           /* Number of programs per page */
	u8 partialProgAttr;           /* Obsolete: Partial programming attributes */
	u8 eccBits;                   /* Number of bits ECC correctability */
	u8 planeAddrBits;             /* Number of plane address bits */
	u8 multiplaneOpAttr;          /* Multi-plane operation attributes */
	u8 ezNandSupport;             /* EZ NAND support parameters */
	u8 res3[12];

	/* Electrical parameters block */
	u8 ioPinCapacitance;     /* I/O pin capacitance */
	u16 timingMode;          /* SDR Timing mode support */
	u16 progCacheTimingMode; /* Obsolete: SDR program cache timing mode support */
	u16 tPageProg;           /* Maximum page program time (us) */
	u16 tBlkErase;           /* Maximum block erase time (us) */
	u16 tPageRead;           /* Maximum page read time (us) */
	u16 tCCS;                /* Maximum change column setup time (ns) */
	u8 nvddrTimingMode;      /* NV-DDR timing mode support */
	u8 nvddr2TimingMode;     /* NV-DDR2 timing mode support */
	u8 nvddrFeat;            /* NV-DDR/NV-DDR2 features */
	u16 clkInputPinCap;      /* CLK input pin capacitance, typical */
	u16 ioPinCap;            /* I/O pin capacitance, typical */
	u16 inputPinCap;         /* Input pin capacitance, typical */
	u8 inputPinCapMax;       /* Input pin capacitance, maximum */
	u8 drvStrength;          /* Driver strength support */
	u16 tMR;                 /* Maximum multi-plane page read time (us) */
	u16 tADL;                /* Program page register clear enhancement tADL value (ns) */
	u16 tPageReadEZ;         /* Typical page read time for EZ NAND (us) */
	u8 nvddr23Feat;          /* NV-DDR2/3 features */
	u8 nvddr23WarmupCyc;     /* NV-DDR2/3 warmup cycles */
	u16 nvddr3TimingMode;    /* NV-DDR3 timing mode support */
	u8 nvddr2TimingMode2;    /* NV-DDR timing mode support (modes 8-10) */
	u8 res4;

	/* Vendor block */
	u16 vendorRevision;    /* Vendor specific revision number */
	u8 vendorSpecific[88]; /* Vendor specific */
	u16 crc;               /* Integrity CRC */
} __attribute__((packed)) onfi_paramPage_t;


const onfi_timingMode_t *onfi_getTimingModeSDR(unsigned int mode);


#endif /* FLASH_ONFI_4_H_ */
