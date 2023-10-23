/*
 * Phoenix-RTOS
 *
 * SD memory card driver
 * Compatible with SD Specifications Part A2: SD Host Controller Simplified Specification Version 2.00
 *
 * Copyright 2023 Phoenix Systems
 * Author: Ziemowit Leszczynski, Artur Miller, Jacek Maksymowicz
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "sdcard.h"

#include <lib/lib.h>
#include <hal/armv7a/cache.h>

#include "sdhost_defs.h"
#include "zynq7000-sdio.h"

#define LOG_TAG "sdcard"
/* clang-format off */
#define LOG_ERROR(str, ...) do { if (0) lib_printf(LOG_TAG " error: " str "\n", ##__VA_ARGS__); } while (0)
#define TRACE(str, ...)     do { if (0) lib_printf(LOG_TAG " trace: " str "\n", ##__VA_ARGS__); } while (0)
/* clang-format on */


#define SDHOST_ERROR_REASONS ( \
	SDHOST_INTR_CMD_ERRORS | \
	SDHOST_INTR_DAT_ERRORS | \
	SDHOST_INTR_OVERCURRENT_ERROR | \
	SDHOST_INTR_AUTO_CMD12_ERROR | \
	SDHOST_INTR_ADMA_ERROR | \
	SDHOST_INTR_DMA_ERROR)

#define SDHOST_STATUS_MASK ( \
	SDHOST_INTR_CMD_STATUS | \
	SDHOST_INTR_CARD_IN | \
	SDHOST_INTR_CARD_OUT | \
	SDHOST_ERROR_REASONS)

/* configuration values */
#define THREAD_STACK_SIZE 1024
#define SDHOST_RETRIES    10

/* Number of blocks to erase in a single transaction
 * Number is limited to not trigger timeouts
 */
#define ERASE_N_BLOCKS 128

#define SD_FREQ_INITIAL 400000   /* 400 kHz clock used for card initialization */
#define SD_FREQ_25M     25000000 /* 25 MHz clock usable when card is initialized */
#define SD_FREQ_50M     50000000 /* 50 MHz clock usable when card is initialized and supports high speed */

#define ARRAY_LENGTH(array) (sizeof(array) / sizeof(array[0]))

typedef struct {
	/* Relative Card Address of the card. Needed for selecting the card so it can use the data bus. */
	u32 rca;
	/* Size of SD card in blocks */
	u32 sizeBlocks;
	/* Size (in blocks) of an erase sector */
	u32 eraseSizeBlocks;
	/* Whether the card supports SDHC protocol (requires slightly different handling) */
	u8 highCapacity;
} sdcard_cardMetadata_t;

typedef struct {
	/* Base address (in virtual memory) of SD Host Controller register set */
	volatile u32 *base;
	sdcard_cardMetadata_t card;
	/* Reference clock frequency in Hz */
	u32 refclkFrequency;

	/* Address of DMA buffer in virtual memory (for access by the CPU) */
	void *dmaBuffer;
	/* Address of DMA buffer in physical memory (for access by the SD Host Controller) */
	addr_t dmaBufferPhys;

	u8 sdioInitialized;
	u8 isCDPinSupported;
	u8 isWPPinSupported;
} sdcard_hostData_t;

static sdcard_hostData_t sdio_hosts[PLATFORM_SDIO_N_HOSTS] = { 0 };
static unsigned int initializedHosts = 0;

static inline sdcard_hostData_t *sdcard_getHostForSlot(unsigned int slot)
{
	return (slot >= PLATFORM_SDIO_N_HOSTS) ? NULL : &sdio_hosts[slot];
}


static void sleep_ms(u32 ms)
{

	time_t timeout = hal_timerGet() + ms;
	while (hal_timerGet() < timeout) {
		/* wait */
	}
}

static int sdcard_configClockAndPower(sdcard_hostData_t *host, u32 freq);
static int sdcard_wideAndFast(sdcard_hostData_t *host);


/* Resets parts or all of the host according to the given reset type.
 * Argument must be one of CLOCK_CONTROL_RESET_*
 */
static int sdhost_reset(sdcard_hostData_t *host, u32 resetType)
{
	*(host->base + SDHOST_REG_CLOCK_CONTROL) |= resetType;
	sdio_dataBarrier();
	for (int i = 0; i < SDHOST_RETRIES; ++i) {
		sleep_ms(1);
		if ((*(host->base + SDHOST_REG_CLOCK_CONTROL) & resetType) == 0) {
			return 0;
		}
	}

	return -1;
}


static inline u8 sdcard_isWriteProtected(sdcard_hostData_t *host)
{
	if (host->isWPPinSupported != 0) {
		if ((*(host->base + SDHOST_REG_PRES_STATE) & PRES_STATE_WRITE_PROT_PIN) == 0) {
			return 1;
		}
	}

	return 0;
}


static int _sdio_cmdExecutionWait(sdcard_hostData_t *host, u32 flags, time_t deadline)
{
	int ret = -ETIME;
	int doResetCmd = 0, doResetDat = 0;

	do {
		u32 val = *(host->base + SDHOST_REG_INTR_STATUS);
		if ((val & SDHOST_ERROR_REASONS) != 0) {
			doResetCmd = ((val & SDHOST_INTR_CMD_ERRORS) != 0) ? 1 : 0;
			doResetDat = ((val & SDHOST_INTR_DAT_ERRORS) != 0) ? 1 : 0;
			*(host->base + SDHOST_REG_INTR_STATUS) = SDHOST_ERROR_REASONS;
			ret = -EIO;
			break;
		}
		else if ((val & flags) == flags) {
			*(host->base + SDHOST_REG_INTR_STATUS) = flags;
			ret = EOK;
			break;
		}

		if ((val & SDHOST_INTR_BLOCK_GAP) != 0) {
			/* Not strictly an error, but should not happen in the current implementation */
			*(host->base + SDHOST_REG_INTR_STATUS) = SDHOST_INTR_BLOCK_GAP;
			doResetDat = 1;
			ret = -EIO;
			break;
		}
	} while (hal_timerGet() < deadline);

	if ((doResetCmd != 0) || (ret == -ETIME)) {
		sdhost_reset(host, CLOCK_CONTROL_RESET_CMD);
	}

	if ((doResetDat != 0) || (ret == -ETIME)) {
		sdhost_reset(host, CLOCK_CONTROL_RESET_DAT);
	}

	return ret;
}


/* res is single u32 if isLongResponse == 0, array of 4 u32 otherwise */
static int _sdio_cmdSend(sdcard_hostData_t *host, u8 cmd, u32 arg, u32 *res, u16 blockCount, int isLongResponse, time_t deadline)
{
	sdhost_command_reg_t cmdFrame;
	u32 val;

	if (cmd >= MAX_SD_COMMANDS) {
		return -EINVAL;
	}

	sdhost_command_data_t dataType = sdCmdMetadata[cmd].dataType;
	if (dataType == CMD_INVALID) {
		return -EINVAL;
	}

	val = *(host->base + SDHOST_REG_PRES_STATE) & PRES_STATE_BUSY_FLAGS;
	if (val != 0) {
		TRACE("busy %x", val);
		return -EBUSY;
	}

	val = *(host->base + SDHOST_REG_INTR_STATUS) & (SDHOST_INTR_TRANSFER_DONE | SDHOST_INTR_CMD_DONE);
	if (val != 0) {
		TRACE("intr bits %x", val);
		return -EBUSY;
	}

	cmdFrame.commandIdx = cmd;
	cmdFrame.commandMeta = sdCmdMetadata[cmd].bitsWhenSending;

	if ((dataType == CMD_NO_DATA) || (dataType == CMD_NO_DATA_WAIT)) {
		*(host->base + SDHOST_REG_TRANSFER_BLOCK) = 0;
	}
	else {
		if (blockCount != 0) {
			u32 blockLength;
			switch (dataType) {
				case CMD_READ8:
					blockLength = 8;
					break;

				case CMD_READ64:
					blockLength = 64;
					break;

				default:
					blockLength = SDCARD_BLOCKLEN;
					break;
			}

			*(host->base + SDHOST_REG_TRANSFER_BLOCK) =
				((u32)blockCount << 16) |
				TRANSFER_BLOCK_SDMA_BOUNDARY_4K |
				blockLength;
			*(host->base + SDHOST_REG_SDMA_ADDRESS) = host->dmaBufferPhys;
		}

		cmdFrame.dataPresent = 1;
		cmdFrame.dmaEnable = 1;
		cmdFrame.blockCountEnable = 1;
		if ((dataType == CMD_READ) || (dataType == CMD_READ_MULTI) || (dataType == CMD_READ8) || (dataType == CMD_READ64)) {
			cmdFrame.directionRead = 1;
		}

		if ((dataType == CMD_READ_MULTI) || (dataType == CMD_WRITE_MULTI)) {
			cmdFrame.multiBlock = 1;
			cmdFrame.autoCmd12Enable = 1;
		}
	}

	*(host->base + SDHOST_REG_CMD_ARGUMENT) = arg;
	sdio_dataBarrier();
	/* This register write starts command execution and must be done last */
	*(host->base + SDHOST_REG_CMD) = cmdFrame.raw;

	int ret = _sdio_cmdExecutionWait(host, SDHOST_INTR_CMD_DONE, deadline);
	if (ret < 0) {
		TRACE("error %d on cmd %d", ret, cmd);
		return ret;
	}

	if (dataType != CMD_NO_DATA) {
		ret = _sdio_cmdExecutionWait(host, SDHOST_INTR_TRANSFER_DONE, deadline);
		if (ret < 0) {
			TRACE("error %d on cmd %d", ret, cmd);
			return ret;
		}
	}

	if (res != NULL) {
		int responseLen = isLongResponse ? 4 : 1;
		for (int i = 0; i < responseLen; i++) {
			res[i] = *(host->base + SDHOST_REG_RESPONSE_0 + i);
		}
	}

	return 0;
}


/* res is single u32 if isLongResponse == 0, array of 4 u32 otherwise */
static int sdio_cmdSendEx(sdcard_hostData_t *host, u8 cmd, u32 arg, u32 *res, int isLongResponse, u8 *data)
{
	int ret;

	/* 2 ms is enough for commands that don't do writes */
	time_t deadline = hal_timerGet() + 2;
	if ((cmd & SDIO_ACMD_BIT) != 0) {
		u32 resAcmd;
		ret = _sdio_cmdSend(host, SDIO_CMD55_APP_CMD, host->card.rca, &resAcmd, 0, 0, deadline);
		if (ret < 0) {
			return ret;
		}

		if ((resAcmd & CARD_STATUS_APP_CMD) == 0) {
			LOG_ERROR("app cmd not accepted");
			return -EOPNOTSUPP;
		}
	}

	if (cmd >= MAX_SD_COMMANDS) {
		return -EINVAL;
	}

	size_t dataSize;
	switch (sdCmdMetadata[cmd].dataType) {
		case CMD_READ8:
			dataSize = 8;
			break;

		case CMD_READ64:
			dataSize = 64;
			break;

		default:
			dataSize = 0;
			break;
	}

	u16 blockCount = (dataSize > 0) ? 1 : 0;
	ret = _sdio_cmdSend(host, cmd, arg, res, blockCount, isLongResponse, deadline);
	if (data != NULL) {
		hal_dcacheInval(host->dmaBufferPhys, host->dmaBufferPhys + dataSize);
		hal_memcpy(data, host->dmaBuffer, dataSize);
	}

	return ret;
}

/* Send common command with no data transfer */
static inline int sdio_cmdSend(sdcard_hostData_t *host, u8 cmd, u32 arg, u32 *res)
{
	return sdio_cmdSendEx(host, cmd, arg, res, 0, NULL);
}

/* Send common command with no data transfer and a long response */
static inline int sdio_cmdSendWithLongResponse(sdcard_hostData_t *host, u8 cmd, u32 arg, u32 res[4])
{
	return sdio_cmdSendEx(host, cmd, arg, res, 1, NULL);
}


static int sdcard_getCardSize(sdcard_hostData_t *host)
{
	u32 resp[4];
	u32 blockNr;
	u32 eraseSectorSize;
	sdcard_cardMetadata_t *card = &host->card;
	if (sdio_cmdSendWithLongResponse(host, SDIO_CMD9_SEND_CSD, card->rca, resp) < 0) {
		return -EIO;
	}

	u32 csdVersion = CSD_VERSION(resp);
	if (csdVersion == 0) {
		u32 cSize = CSDV1_C_SIZE(resp);
		u32 cSizeMultiplier = CSDV1_C_SIZE_MULT(resp);
		u32 mult = 4 << cSizeMultiplier;
		u32 readBlLen = CSDV1_READ_BL_LEN(resp);
		u32 writeBlLen = CSDV1_WRITE_BL_LEN(resp);
		blockNr = (cSize + 1) * mult;
		if (readBlLen >= 9) {
			blockNr <<= readBlLen - 9;
		}
		else {
			blockNr >>= 9 - readBlLen;
		}

		/* In cards with CSD v1 if ERASE_BLK_EN == 0 the card may erase more blocks
		 * than we selected - rounding on both sides of the range up to eraseSectorSize.
		 */
		eraseSectorSize = CSDV1_ERASE_SECTOR_SIZE(resp);
		if (writeBlLen >= 9) {
			eraseSectorSize <<= writeBlLen - 9;
		}
		else {
			u32 divider = 1 << (9 - writeBlLen);
			/* Round up - it's better to erase more than necessary than try to erase less
			 * and end up erasing more than intended.
			 */
			eraseSectorSize = (eraseSectorSize + divider - 1) / divider;
		}
	}
	else if (csdVersion == 1) {
		u32 cSize = CSDV2_C_SIZE(resp);
		blockNr = cSize << 10;
		/* In CSD v2 this field is not used and card can always erase blocks one by one */
		eraseSectorSize = 1;
	}
	else {
		/* Unknown CSD structure version; cannot compute size. */
		return -EOPNOTSUPP;
	}

	TRACE("Memory card size: %u blocks", blockNr);
	card->sizeBlocks = blockNr;
	card->eraseSizeBlocks = eraseSectorSize;
	return 0;
}


int sdcard_initCard(unsigned int slot, int fallbackMode)
{
	sdcard_hostData_t *host = sdcard_getHostForSlot(slot);
	if (host == NULL) {
		return -ENOENT;
	}

	/* Switch off 4-bit mode, because card will be in 1-bit mode after CMD0 */
	*(host->base + SDHOST_REG_HOST_CONTROL) &= ~HOST_CONTROL_4_BIT_MODE;
	sdcard_configClockAndPower(host, SD_FREQ_INITIAL);
	/* Before we know the card's RCA, set to 0 to send to all cards */
	host->card.rca = 0;

	sdhost_reset(host, CLOCK_CONTROL_RESET_CMD);
	sdhost_reset(host, CLOCK_CONTROL_RESET_DAT);
	if (sdio_cmdSend(host, SDIO_CMD0_GO_IDLE_STATE, 0, NULL) < 0) {
		/* This will only fail if something is wrong with host, succeeds even with no card inserted */
		LOG_ERROR("go idle fail");
		return -EIO;
	}

	u8 trySdhc = 1;
	u32 cmd8Response;
	if (sdio_cmdSend(host, SDIO_CMD8_SEND_IF_COND, (IF_COND_ECHO_PATTERN | IF_COND_3V3_SUPPORTED), &cmd8Response) < 0) {
		/* This command is not supported on SD v1 cards so it must be treated as Standard Capacity*/
		trySdhc = 0;
	}
	else {
		if ((cmd8Response & 0xff) != IF_COND_ECHO_PATTERN) {
			LOG_ERROR("connection error");
			return -EIO;
		}

		if (((cmd8Response >> 8) & 0xf) != 0b0001) {
			LOG_ERROR("incompatible voltage");
			return -EOPNOTSUPP;
		}
	}

	u32 acmd41Response;
	u32 acmd41Arg = (trySdhc == 1) ? (1 << 30) : 0;
	if (sdio_cmdSend(host, SDIO_ACMD41_SD_SEND_OP_COND, acmd41Arg, &acmd41Response) < 0) {
		LOG_ERROR("op cond fail");
		return -EIO;
	}

	/* NOTE: on some hosts the host voltage could be changed at this point */
	if ((acmd41Response & ((1 << 20) | (1 << 21))) == 0) {
		LOG_ERROR("3.3V not supported");
		return -EOPNOTSUPP;
	}

	acmd41Arg |= acmd41Response & 0xffffff;
	/* According to docs timeout value for initialization process == 1 sec */
	for (int i = 0; i < 1000; i++) {
		if (sdio_cmdSend(host, SDIO_ACMD41_SD_SEND_OP_COND, acmd41Arg, &acmd41Response) < 0) {
			LOG_ERROR("waiting for card init failed");
			return -EIO;
		}

		if ((acmd41Response & IF_COND_READY) != 0) {
			break;
		}

		/* We can wait up to 50 ms before reissuing ACMD41 */
		sleep_ms(1);
	}

	if ((acmd41Response & IF_COND_READY) == 0) {
		return -ETIME;
	}

	host->card.highCapacity = trySdhc && (((acmd41Response >> 30) & 1) != 0);

	/* Not sure what that is for, but it's in the documentation that we should do this */
	if (sdio_cmdSend(host, SDIO_CMD2_ALL_SEND_CID, 0, NULL) < 0) {
		return -EIO;
	}

	u32 cardRCA;
	if (sdio_cmdSend(host, SDIO_CMD3_RELATIVE_ADDR, 0, &cardRCA) < 0) {
		return -EIO;
	}

	host->card.rca = cardRCA & 0xffff0000;
	if (sdcard_getCardSize(host) < 0) {
		LOG_ERROR("cannot determine size");
		host->card.sizeBlocks = 0;
		host->card.eraseSizeBlocks = 1;
	}

	if (sdio_cmdSend(host, SDIO_CMD7_SELECT_CARD, host->card.rca, NULL) < 0) {
		return -EIO;
	}

	if (host->card.highCapacity == 0) {
		if (sdio_cmdSend(host, SDIO_CMD16_SET_BLOCKLEN, SDCARD_BLOCKLEN, NULL) < 0) {
			TRACE("set blocklen fail");
			return -EIO;
		}
	}

	u32 finalStatus;
	if (sdio_cmdSend(host, SDIO_CMD13_SEND_STATUS, host->card.rca, &finalStatus) < 0) {
		return -EIO;
	}

	if ((finalStatus & CARD_STATUS_ERRORS) != 0) {
		return -EIO;
	}

	if (CARD_STATUS_CURRENT_STATE(finalStatus) != CARD_STATUS_CURRENT_STATE_TRAN) {
		/* Something unexpected must have happened because card is not in a state to transfer data */
		return -EIO;
	}

	return fallbackMode ? 0 : sdcard_wideAndFast(host);
}


/* Extract information about supported functions in a given function group from the function register */
static inline u16 sdcard_extractFunctionGroupInfo(u8 fnRegister[64], u8 fnGroup)
{
	if ((fnGroup > 6) || (fnGroup < 1)) {
		return 0;
	}

	u8 fnGroupIndex = (6 - fnGroup) * 2 + 2;
	return ((u16)fnRegister[fnGroupIndex] << 8) | fnRegister[fnGroupIndex + 1];
}


/* Extract status of function from a given function group from the function register */
static inline u8 sdcard_extractFunctionSwitchResult(u8 fnRegister[64], u8 fnGroup)
{
	if ((fnGroup > 6) || (fnGroup < 1)) {
		return 0;
	}

	fnGroup = (6 - fnGroup);
	u8 fnGroupIndex = fnGroup / 2 + 14;
	u8 fnGroupBits = ((fnGroup % 2) == 0) ? 4 : 0;
	return (fnRegister[fnGroupIndex] >> fnGroupBits) & 0xf;
}


static int sdcard_hasHighSpeedFunction(sdcard_hostData_t *host, u8 tmpReg[64])
{
	u32 getFunctionArg = SDIO_SWITCH_FUNC_GET | SDIO_SWITCH_FUNC_HIGH_SPEED;
	if (sdio_cmdSendEx(host, SDIO_CMD6_SWITCH_FUNC, getFunctionArg, NULL, 0, tmpReg) < 0) {
		return 0;
	}

	u16 accessModeFunctions = sdcard_extractFunctionGroupInfo(tmpReg, SDCARD_FUNCTION_GROUP_ACCESS_MODE);
	return (accessModeFunctions & SDCARD_FUNCTION_GROUP_ACCESS_MODE_HIGH_SPEED) != 0;
}


/* Switches card to maximum width and speed supported */
static int sdcard_wideAndFast(sdcard_hostData_t *host)
{
	u8 bigRegs[64];
	/* Get SD card configuration register with some useful info about the card */
	if (sdio_cmdSendEx(host, SDIO_ACMD51_SEND_SCR, 0, NULL, 0, bigRegs) < 0) {
		return -EIO;
	}

	int cmd6Supported = (SCR_SD_SPEC(bigRegs) >= SCR_SD_SPEC_V1_10) ? 1 : 0;
	/* In theory all SD cards should support 4-bit, but make sure */
	if ((SCR_BUS_WIDTHS(bigRegs) & SCR_BUS_WIDTHS_4_BIT) != 0) {
		if (sdio_cmdSend(host, SDIO_ACMD6_SET_BUS_WIDTH, 2, NULL) < 0) {
			LOG_ERROR("bus widening failed");
			return -EIO;
		}

		*(host->base + SDHOST_REG_HOST_CONTROL) |= HOST_CONTROL_4_BIT_MODE;
		sleep_ms(1);
		if (sdio_cmdSendEx(host, SDIO_ACMD13_SD_STATUS, 0, NULL, 0, bigRegs) < 0) {
			LOG_ERROR("bus widening failed");
			return -EIO;
		}

		if (SD_STATUS_DAT_BUS_WIDTH(bigRegs) != SD_STATUS_DAT_BUS_WIDTH_4_BIT) {
			LOG_ERROR("bus widening failed");
			return -EIO;
		}
	}

	int isHighSpeedSupported = 0;
	if ((cmd6Supported != 0) && (sdcard_hasHighSpeedFunction(host, bigRegs) != 0)) {
		u32 switchFunctionArg = SDIO_SWITCH_FUNC_SET | SDIO_SWITCH_FUNC_HIGH_SPEED;
		if (sdio_cmdSendEx(host, SDIO_CMD6_SWITCH_FUNC, switchFunctionArg, NULL, 0, bigRegs) == 0) {
			if (sdcard_extractFunctionSwitchResult(bigRegs, SDCARD_FUNCTION_GROUP_ACCESS_MODE) == 1) {
				isHighSpeedSupported = 1;
			}
		}
	}

	if (isHighSpeedSupported != 0) {
		TRACE("using HS mode");
		if (sdcard_configClockAndPower(host, SD_FREQ_50M) < 0) {
			return -EIO;
		}
	}
	else {
		TRACE("HS mode not supported");
		if (sdcard_configClockAndPower(host, SD_FREQ_25M) < 0) {
			return -EIO;
		}
	}

	sleep_ms(1);

	/* Perform a final transaction to check if data transfer is working (we only care about retcode this time) */
	if (sdio_cmdSendEx(host, SDIO_ACMD13_SD_STATUS, 0, NULL, 0, NULL) < 0) {
		LOG_ERROR("bus speed change failed");
		return -EIO;
	}

	return 0;
}


static void _sdcard_free(sdcard_hostData_t *host)
{
	TRACE("freeing resources");
	sdhost_reset(host, CLOCK_CONTROL_RESET_ALL);
	host->sdioInitialized = 0;
	*(host->base + SDHOST_REG_INTR_STATUS_ENABLE) = 0;
	*(host->base + SDHOST_REG_CLOCK_CONTROL) = 0;
}


void sdcard_free(unsigned int slot)
{
	sdcard_hostData_t *host = sdcard_getHostForSlot(slot);
	if (host == NULL) {
		return;
	}

	if (host->sdioInitialized == 0) {
		return;
	}

	_sdcard_free(host);
}


int sdcard_initHost(unsigned int slot, char *dataBuffer)
{
	sdcard_hostData_t *host = sdcard_getHostForSlot(slot);
	if (host == NULL) {
		return -ENOENT;
	}

	if (host->sdioInitialized == 1) {
		return (initializedHosts < PLATFORM_SDIO_N_HOSTS) ?
			(PLATFORM_SDIO_N_HOSTS - initializedHosts) :
			0;
	}

	/* Perform platform-specific configuration */
	sdio_platformInfo_t info;
	if (sdio_platformConfigure(slot, &info) < 0) {
		return -EIO;
	}

	host->refclkFrequency = info.refclkFrequency;
	host->isCDPinSupported = info.isCDPinSupported;
	host->isWPPinSupported = info.isWPPinSupported;
	host->base = (volatile u32 *)info.regBankPhys;
	host->dmaBuffer = dataBuffer;
	host->dmaBufferPhys = (addr_t)host->dmaBuffer;

	if (sdhost_reset(host, CLOCK_CONTROL_RESET_ALL) < 0) {
		_sdcard_free(host);
		return -EIO;
	}

	if (sdcard_configClockAndPower(host, SD_FREQ_INITIAL) < 0) {
		_sdcard_free(host);
		return -EIO;
	}

	if (host->isCDPinSupported == 0) {
		/* Spoof card state so that it always appears inserted */
		*(host->base + SDHOST_REG_HOST_CONTROL) |= HOST_CONTROL_CARD_DET_TEST | HOST_CONTROL_CARD_DET_TEST_ENABLE;
	}

	*(host->base + SDHOST_REG_INTR_STATUS_ENABLE) = SDHOST_STATUS_MASK;
	host->sdioInitialized = 1;
	initializedHosts++;
	return (initializedHosts < PLATFORM_SDIO_N_HOSTS) ?
		(PLATFORM_SDIO_N_HOSTS - initializedHosts) :
		0;
}


/* Calculate the divisor value so the output frequency is no larger than `freq` */
static int sdcard_calculateDivisor(u32 refclk, u32 freq, u32 *divisor)
{
	if (freq >= refclk) {
		*divisor = CLOCK_CONTROL_DIV_1;
		return 0;
	}

	for (u32 val = CLOCK_CONTROL_DIV_2; val <= CLOCK_CONTROL_DIV_256; val <<= 1) {
		refclk /= 2;
		if (freq >= refclk) {
			*divisor = val;
			return 0;
		}
	}

	return -1;
}


static int sdcard_configClockAndPower(sdcard_hostData_t *host, u32 freq)
{
	if ((freq != SD_FREQ_INITIAL) && (freq != SD_FREQ_25M) && (freq != SD_FREQ_50M)) {
		return -EINVAL;
	}

	/* NOTE: In theory, the Capabilities register can hold the reference clock frequency,
	 * but this doesn't have to be implemented. For this reason, we get the frequency
	 * from sdio_platformConfigure().
	 */
	u32 divRegValue;
	if (sdcard_calculateDivisor(host->refclkFrequency, freq, &divRegValue) < 0) {
		return -EINVAL;
	}

	*(host->base + SDHOST_REG_CLOCK_CONTROL) &= ~CLOCK_CONTROL_START_SD_CLOCK;

	if (freq == SD_FREQ_50M) {
		*(host->base + SDHOST_REG_HOST_CONTROL) |= HOST_CONTROL_HIGH_SPEED;
	}
	else {
		*(host->base + SDHOST_REG_HOST_CONTROL) &= ~HOST_CONTROL_HIGH_SPEED;
	}

	/* This looks weird because we may set the "divisor" to 0, but this is intended */
	*(host->base + SDHOST_REG_CLOCK_CONTROL) = divRegValue | CLOCK_CONTROL_START_INTERNAL_CLOCK;
	sdio_dataBarrier();
	for (int i = 0; i < SDHOST_RETRIES; i++) {
		u32 val = *(host->base + SDHOST_REG_CLOCK_CONTROL) & CLOCK_CONTROL_INTERNAL_CLOCK_STABLE;
		if (val != 0) {
			*(host->base + SDHOST_REG_CLOCK_CONTROL) |= CLOCK_CONTROL_START_SD_CLOCK | CLOCK_CONTROL_DATA_TIMEOUT_VALUE(0b1110UL);
			*(host->base + SDHOST_REG_HOST_CONTROL) |= HOST_CONTROL_BUS_VOLTAGE_3V3 | HOST_CONTROL_BUS_POWER;
			return 0;
		}

		sleep_ms(1);
	}

	return -ETIME;
}


static int _sdcard_transferBlocks(sdcard_hostData_t *host, sdio_dir_t dir, u32 blockOffset, u16 blockCount, time_t deadline)
{
	u8 cmd;

	if (dir == sdio_read) {
		if (blockCount > 1) {
			cmd = SDIO_CMD18_READ_MULTIPLE_BLOCK;
		}
		else {
			cmd = SDIO_CMD17_READ_SINGLE_BLOCK;
		}
	}
	else {
		if (blockCount > 1) {
			cmd = SDIO_CMD25_WRITE_MULTIPLE_BLOCK;
		}
		else {
			cmd = SDIO_CMD24_WRITE_SINGLE_BLOCK;
		}
	}

	/* The unit of “data address” in argument is byte for Standard Capacity SD Memory Card
	 * and block (512 bytes) for High Capacity SD Memory Card.
	 */
	u32 arg = (host->card.highCapacity != 0) ? blockOffset : (blockOffset * SDCARD_BLOCKLEN);
	u32 resp;
	int ret = _sdio_cmdSend(host, cmd, arg, &resp, blockCount, 0, deadline);

	if (ret < 0) {
		return ret;
	}

	if ((resp & CARD_STATUS_ERRORS) != 0) {
		LOG_ERROR("transfer error %08x", resp);
		return -EIO;
	}

	return 0;
}


int sdcard_transferBlocks(unsigned int slot, sdio_dir_t dir, u32 blockOffset, u32 blocks, time_t deadline)
{
	sdcard_hostData_t *host = sdcard_getHostForSlot(slot);
	if (host == NULL) {
		return -ENOENT;
	}

	if (blocks > (SDCARD_MAX_TRANSFER / SDCARD_BLOCKLEN)) {
		return -EINVAL;
	}

	if ((dir == sdio_write) && (sdcard_isWriteProtected(host) != 0)) {
		return -EPERM;
	}

	if (dir == sdio_write) {
		hal_dcacheClean(host->dmaBufferPhys, host->dmaBufferPhys + blocks * SDCARD_BLOCKLEN);
	}

	int ret = _sdcard_transferBlocks(host, dir, blockOffset, blocks, deadline);
	if (dir == sdio_read) {
		hal_dcacheInval(host->dmaBufferPhys, host->dmaBufferPhys + blocks * SDCARD_BLOCKLEN);
	}

	return ret;
}


static int _sdcard_eraseBlocks(sdcard_hostData_t *host, u32 start, u32 end)
{
	u32 resp;
	int ret;
	time_t deadline = hal_timerGet() + 3000;
	ret = _sdio_cmdSend(host, SDIO_CMD32_ERASE_WR_BLK_START, start, &resp, 0, 0, deadline);
	if ((ret < 0) || ((resp & CARD_STATUS_ERRORS) != 0)) {
		LOG_ERROR("erase start %d %08x", ret, resp);
		return -EIO;
	}

	ret = _sdio_cmdSend(host, SDIO_CMD33_ERASE_WR_BLK_END, end, &resp, 0, 0, deadline);
	if ((ret < 0) || ((resp & CARD_STATUS_ERRORS) != 0)) {
		LOG_ERROR("erase end %d %08x", ret, resp);
		return -EIO;
	}

	ret = _sdio_cmdSend(host, SDIO_CMD38_ERASE, 0, &resp, 0, 0, deadline);
	if ((ret < 0) || ((resp & CARD_STATUS_ERRORS) != 0)) {
		LOG_ERROR("do erase %d %08x", ret, resp);
		return -EIO;
	}

	return ret;
}


int sdcard_eraseBlocks(unsigned int slot, u32 blockOffset, u32 nBlocks)
{
	int ret = EOK;

	sdcard_hostData_t *host = sdcard_getHostForSlot(slot);
	if (host == NULL) {
		return -ENOENT;
	}

	if (sdcard_isWriteProtected(host)) {
		return -EPERM;
	}

	if ((blockOffset % host->card.eraseSizeBlocks != 0) || (nBlocks % host->card.eraseSizeBlocks != 0)) {
		return -EINVAL;
	}

	u32 erasePerIteration = (host->card.eraseSizeBlocks > ERASE_N_BLOCKS) ?
		host->card.eraseSizeBlocks :
		ERASE_N_BLOCKS;
	while (nBlocks > 0) {
		if (nBlocks < erasePerIteration) {
			erasePerIteration = nBlocks;
		}

		u32 start = blockOffset;
		u32 end = blockOffset + erasePerIteration - 1;
		if (host->card.highCapacity == 0) {
			start *= SDCARD_BLOCKLEN;
			end *= SDCARD_BLOCKLEN;
		}

		ret = _sdcard_eraseBlocks(host, start, end);
		if (ret < 0) {
			break;
		}

		blockOffset += erasePerIteration;
		nBlocks -= erasePerIteration;
	}

	return ret;
}


int sdcard_writeFF(unsigned int slot, u32 blockOffset, u32 nBlocks)
{
	int ret = EOK;

	sdcard_hostData_t *host = sdcard_getHostForSlot(slot);
	if (host == NULL) {
		return -ENOENT;
	}

	if (sdcard_isWriteProtected(host)) {
		return -EPERM;
	}

	u32 erasePerIteration = SDCARD_MAX_TRANSFER / SDCARD_BLOCKLEN;
	hal_memset(host->dmaBuffer, 0xff, SDCARD_MAX_TRANSFER);
	hal_dcacheClean(host->dmaBufferPhys, host->dmaBufferPhys + SDCARD_MAX_TRANSFER);
	while (nBlocks > 0) {
		if (nBlocks < erasePerIteration) {
			erasePerIteration = nBlocks;
		}

		u8 cmd = (erasePerIteration > 1) ? SDIO_CMD25_WRITE_MULTIPLE_BLOCK : SDIO_CMD24_WRITE_SINGLE_BLOCK;
		u32 arg = host->card.highCapacity ? blockOffset : (blockOffset * SDCARD_BLOCKLEN);
		time_t deadline = hal_timerGet() + 1000;
		u32 resp;
		ret = _sdio_cmdSend(host, cmd, arg, &resp, erasePerIteration, 0, deadline);
		if (ret < 0) {
			break;
		}

		if ((resp & CARD_STATUS_ERRORS) != 0) {
			LOG_ERROR("transfer error %08x", resp);
			ret = -EIO;
			break;
		}

		blockOffset += erasePerIteration;
		nBlocks -= erasePerIteration;
	}

	return ret;
}


u32 sdcard_getSizeBlocks(unsigned int slot)
{
	sdcard_hostData_t *host = sdcard_getHostForSlot(slot);
	if (host == NULL) {
		return 0;
	}

	return host->card.sizeBlocks;
}


u32 sdcard_getEraseSizeBlocks(unsigned int slot)
{
	sdcard_hostData_t *host = sdcard_getHostForSlot(slot);
	if (host == NULL) {
		return 0;
	}

	return host->card.eraseSizeBlocks;
}


sdcard_insertion_t sdcard_isInserted(unsigned int slot)
{
	sdcard_hostData_t *host = sdcard_getHostForSlot(slot);
	if (host == NULL) {
		return 0;
	}

	u32 val = *(host->base + SDHOST_REG_PRES_STATE);
	if ((val & PRES_STATE_CARD_DET_STABLE) == 0) {
		return SDCARD_INSERTION_UNSTABLE;
	}

	return ((val & PRES_STATE_CARD_INSERTED) != 0) ? SDCARD_INSERTION_IN : SDCARD_INSERTION_OUT;
}
