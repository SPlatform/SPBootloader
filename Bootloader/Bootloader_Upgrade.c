/*******************************************************************************
 *
 * @file Bootloader_Upgrade.c
 *
 * @author MC
 *
 * @brief Bootloader Upgrade Module
 *
 * @see
 *
 *******************************************************************************
 *
 * GNU GPLv3
 *
 * Copyright (c) 2016 SP
 *
 *  See LICENSE file in Root Directory for license details.
 *
 *******************************************************************************/

/********************************* INCLUDES ***********************************/

#include "Drv_Flash.h"
#include "Drv_UART.h"
#include "Drv_Timer.h"

#include "Bootloader_Internal.h"
#include "Bootloader_Config.h"

#include "IntelHex.h"

#include "postypes.h"

/***************************** MACRO DEFINITIONS ******************************/
/*
 * Compiler switch to request missing parts of image
 */
#define BL_UPGRADE_REQUEST_MISSING_PARTS			(0)

/*
 * Timeout for Bootloader Upgrade
 */
#define BL_UPGRADE_TIMEOUT_IN_MS					(1000)

/* Buffer size for flash writes */
#define BL_UPGRADE_FLASH_WRITE_BUFFER_SIZE          (4 * 1024)

/* Convert Big-Endian Array to Integer Value */
#define CONVERT_BE_ARRAY_TO_INT(arr) \
			((arr)[0] << 24) | ((arr)[1] << 16) | ((arr)[2] << 8) | ((arr)[3])

/***************************** TYPE DEFINITIONS *******************************/
/*
 * Flash upgrade module internal settings
 */
typedef struct
{
	struct
	{
		uint32_t initialized : 1;			/* Upgrade Module initialized */
		uint32_t dataReceived : 1;			/* Data received */
		uint32_t upgradeTimeout : 1;		/* Image Upgrade timeout */
		uint32_t metaDataCompleted : 1;		/* All Meta data received */
	} flags;
	/* Handle for UART which used for data upgrade */
	UartHandle uartHandle;
	/* Upgrade Timeout Handle */
	TimerHandle timeoutTimerHandle;
	/* Currently upgraded segment address */
	uint32_t upgradeSegmentAddress;
	/* Currently upgraded offset */
	uint32_t upgradeBlockOffset;
	/* Received data length from UART */
    uint32_t receivedDataLength;
} FWUpgradeSettings;
/**************************** FUNCTION PROTOTYPES *****************************/

/******************************** VARIABLES ***********************************/
/* Upgrade module internal settings */
PRIVATE FWUpgradeSettings upgradeSettings;

/* Block data buffer for flash writes */
PRIVATE uint8_t blockData[BL_UPGRADE_FLASH_WRITE_BUFFER_SIZE] = { 0 };

/**************************** PRIVATE FUNCTIONS ******************************/
/**
 * Upgrade Timeout Event Handler
 *
 */
PRIVATE void UpgradeTimeoutEventHandler(void)
{
	upgradeSettings.flags.upgradeTimeout = true;
}

/**
 * UART Data Received Event Handler
 *
 */
PRIVATE void DataReceivedEventHandler(void)
{
	upgradeSettings.flags.dataReceived = true;
}

/*
 * Shifts a buffer to left direction
 *
 */
PRIVATE void shiftBufferLeft(uint8_t* buffer, uint32_t bufferLength, uint32_t amount)
{
	uint8_t* alignedBuffer = buffer;
	uint8_t* validDataPtr = &buffer[amount];

	/* Ship amount */
	int remainingDataLen = bufferLength - amount;

	/* Shift buffer */
	while (remainingDataLen-- > 0)
	{
		*alignedBuffer = *validDataPtr;

		alignedBuffer++;
		validDataPtr++;
	}
}

/*
 * Processes an intel hex line executes required jobs
 */
PRIVATE BLStatusCode processIntelHexLine(IntelHexLine* intelHexLine)
{
	BLStatusCode retVal = BL_Status_Success;
	int32_t flashStatus;

    switch(intelHexLine->recordType)
    {
		case INTELHEX_RECORDTYPE_EOF:
			{
				/*
				 * We have reached to end of file. Write all buffered data into flash
				 */
				/* Calculate flash offset to write remaining data */
				uint32_t address = upgradeSettings.upgradeSegmentAddress + upgradeSettings.upgradeBlockOffset;
				uint32_t blockNo;

				/* We need write 4K buffer, so fill remaining area with 0xFF */
				memset(&blockData[upgradeSettings.receivedDataLength], 0xFF, BL_UPGRADE_FLASH_WRITE_BUFFER_SIZE - upgradeSettings.receivedDataLength);

				blockNo = Drv_Flash_GetBlockNoOfAddress(address);

				do
				{
					/* Prepare flash block for write operation */
					flashStatus = Drv_Flash_PrepareBlock(blockNo);

					/* Try until it is ready */
				} while (flashStatus == FLASH_STATUS_BUSY);

				/* Write last portion now */
				Drv_Flash_Write(address, blockData, BL_UPGRADE_FLASH_WRITE_BUFFER_SIZE);
			}
			break;
		case INTELHEX_RECORDTYPE_EXTENDED_LINEAR_ADDRESS:
			/* Previous segment is completed, get next segment to upgrade */
			upgradeSettings.upgradeSegmentAddress = intelHexLine->data[0] << 8 | intelHexLine->data[1];
			upgradeSettings.upgradeSegmentAddress *= INTELHEX_SEGMENT_SIZE;
			break;
        case INTELHEX_RECORDTYPE_DATA:
			/* Collect received data into block buffer */
            memcpy(&blockData[intelHexLine->address], intelHexLine->data, intelHexLine->lenght);

			upgradeSettings.receivedDataLength += intelHexLine->lenght;

			/* We received all metadata of firmware, we can prepare flash upgrade area now */
			if ((upgradeSettings.flags.metaDataCompleted == 0) &&
				(upgradeSettings.receivedDataLength == FIRMWARE_METADATA_LENGTH))
			{

				/* Firmware object including header, metadata and image */
				PRIVATE FirmwareInfo* firmware = (FirmwareInfo*)blockData;
				uint32_t firstBlockAddress;
				uint32_t startBlockNo;
				uint32_t endBlockNo;

				firstBlockAddress = firmware->header.imageOffset - FIRMWARE_METADATA_LENGTH;

				if (firstBlockAddress != FIRMWARE_START_ADDRESS)
				{
					/* UPS FW offset is not compatible with current version of bootloader */
					retVal = BL_StatusUpgrade_InCompatibleFWOffset;
					goto intelhex_process_err;
				}

				if (firmware->header.imageOffset + firmware->header.imageSize > Drv_Flash_GetSize())
				{
					/* UPS Firmware exceeds flash size */
					retVal = BL_StatusUpgrade_FWExceedsFlash;
					goto intelhex_process_err;
				}

				/* Find range of FW Image blocks */
				startBlockNo = Drv_Flash_GetBlockNoOfAddress(firstBlockAddress);
				endBlockNo = Drv_Flash_GetBlockNoOfAddress(firmware->header.imageOffset + firmware->header.imageSize - 1);

				do
				{
					flashStatus = Drv_Flash_PrepareBlockRange(startBlockNo, endBlockNo);

				} while (flashStatus == FLASH_STATUS_BUSY);

				flashStatus = Drv_Flash_EraseBlockRange(startBlockNo, startBlockNo);

				upgradeSettings.flags.metaDataCompleted = 1;
			}
			else if (upgradeSettings.receivedDataLength == BL_UPGRADE_FLASH_WRITE_BUFFER_SIZE)
			{
				/*
				 * We collected a page data, we can write to flash now
				 */

				uint32_t address = upgradeSettings.upgradeSegmentAddress + upgradeSettings.upgradeBlockOffset;

				Drv_Flash_WriteBlock(address, blockData, BL_UPGRADE_FLASH_WRITE_BUFFER_SIZE);

				upgradeSettings.upgradeBlockOffset += BL_UPGRADE_FLASH_WRITE_BUFFER_SIZE;

				upgradeSettings.receivedDataLength = 0;
			}

            break;
    }

	return BL_Status_Success;

intelhex_process_err:

	return retVal;
}

/**
 * Processes Image Upload State.
 *
 * Handles UART messages to upgrade Firmware
 *
 */
PRIVATE BLStatusCode ProcessMessageImageUpload(void)
{
	BLStatusCode status;
	uint8_t recvBuffer[256];
	IntelHexLine intelHexLine;
	int32_t recvDataLen = 0;
	int32_t dataLength = 0;
	IntelHexStatusCode ihRetVal;
	uint32_t parsedLineLength;
	int32_t offset = 0;
	bool eof = false;

	/* Initialize flags at the beginning of upgrade transaction */
    upgradeSettings.flags.metaDataCompleted = 0;
    upgradeSettings.receivedDataLength = 0;
    upgradeSettings.upgradeSegmentAddress = 0;
	upgradeSettings.upgradeBlockOffset = 0;

	do
	{
		/* Data received from UART */
		if (upgradeSettings.flags.dataReceived)
		{
			/* Clear flag */
			upgradeSettings.flags.dataReceived = false;

			/* Reset Timeout timer first */
			Drv_Timer_Start(upgradeSettings.timeoutTimerHandle, BL_UPGRADE_TIMEOUT_IN_MS);

			/*
			 * Get UART Data
			 * Concatanate received data using offset to continue incomplete
			 * intel HEX data.
			 */
			recvDataLen = Drv_UART_Receive(upgradeSettings.uartHandle, &recvBuffer[offset], sizeof(recvBuffer) - offset);
			if (recvDataLen < 0) continue;

			/* Increase total dta size */
			dataLength += recvDataLen;
		}

		/*
		 * This block aligns intel hex string to start of buffer.
		 * If there exist meaningless data at the beginning, shifts
		 * buffer until align Intel HEX Prefix (':') to start of buffer.
		 *
		 */
		if (dataLength > 0)
		{
			char* prefixPtr;

			/* Just add terminator char to guarantee string boundary */
			recvBuffer[dataLength] = '\0';

			/* Find Intel HEX Prefix character */
			prefixPtr = strchr((char*)recvBuffer, INTELHEX_PREFIX);

			if (prefixPtr == NULL)
			{
				/* There is no IntelHex Prefix, Discard All Data */
				dataLength = 0;
			}
			else
			{
				/* Offset of Intel HEX prefix in buffer */
				int offsetOfPrefix = prefixPtr - (char*)recvBuffer;

				if (offsetOfPrefix > 0)
				{
					/*
					 * If Intel HEX prefix is not first character, shift it
					 * until done.
					 */
					shiftBufferLeft(recvBuffer, dataLength, offsetOfPrefix);
				}

				/* Update data length after shift */
				dataLength -= offsetOfPrefix;
			}
		}

		/*
		 * This block parses aligned intel HEX string.
		 * It contains a loop to parse intel HEX strings if exist more than one
		 */
		{
			/*
			 * Buffer could be shifted for each valid intel hex line string
			 * but instead, a parse offset is used to walk on buffer for better
			 * performance.
			 */
			int parseOffset = 0;

			while (dataLength > 0)
			{
				/* Parse aligned string first */
				ihRetVal = IntelHex_Parse(&recvBuffer[parseOffset], dataLength, &intelHexLine, &parsedLineLength);

				if (ihRetVal == IntelHex_Success)
				{
					/* In case of success parse, process intel hex item */
					processIntelHexLine(&intelHexLine);

#if (BL_UPGRADE_REQUEST_MISSING_PARTS == 0)
					if (intelHexLine.recordType == INTELHEX_RECORDTYPE_EOF)
					{
						eof = true;
						break;
					}
#endif
				}
				else if (ihRetVal == IntelHex_Err_MissingLine)
				{
					/* Exit loop if line is missing to wait remaining part of line */
					break;
				}

				/* Increase parsing offset to avoid shifting */
				parseOffset += parsedLineLength;

				/* Decrease data length as parsed line */
				dataLength -= parsedLineLength;
			}
		}

		/*
		 * Set offset using data length to continue to receive new Intel HEX data
		 * in Append Mode
		 */
		offset = dataLength;

		/* TODO handle if intel hex is corrupted. Request it from host again */
		/*
		 * TODO decide when will exit. We should not exit when EOF received because
		 * some parts may still missing and should wait them also.
		 */

#if (BL_UPGRADE_REQUEST_MISSING_PARTS == 0)
		if (eof == true)
		{
			status = BL_Status_Success;
			break;
		}
#endif

        /* TODO Open here */
//		if (upgradeSettings.flags.upgradeTimeout)
//		{
//			/*
//			 * Timmeout occured during upgrade break execution
//			 */
//			status = BL_StatusComm_UpgradeTimeout;
//			break;
//		}
	} while (true);

	return status;
}

/**
 * Initialize Bootloader Upgrade Module
 */
PRIVATE ALWAYS_INLINE BLStatusCode InitializeBLUpgradeModule(void)
{
#if BL_DEBUG_MODE
	BLStatusCode status;
#endif /* #if BL_DEBUG_MODE */

	/* Clear flags first */
	upgradeSettings.flags.dataReceived = false;
	upgradeSettings.flags.upgradeTimeout = false;

	/* Create a timer to handle upgrade timeouts */
	upgradeSettings.timeoutTimerHandle = Drv_Timer_Create(BL_FW_UPGRADE_TIMEOUT_TIMER_NO, DRV_TIMER_PRI_LOW, UpgradeTimeoutEventHandler);

#if BL_DEBUG_MODE
	if (DRV_TIMER_INVALID_HANDLE == upgradeSettings.timerHandle)
	{
		status = BL_StatusComm_TimerCannotBeCreated;
		goto upgrade_init_fail;
	}
#endif /* #if BL_DEBUG_MODE */

	/* TODO Get configuration from project config */
	upgradeSettings.uartHandle = Drv_UART_Get(BL_FW_UPGRADE_UART_NO, 115200, DataReceivedEventHandler);

#if BL_DEBUG_MODE
	if (DRV_UART_INVALID_HANDLER == upgradeSettings.uartHandle)
	{
		status = BL_StatusComm_UartPortCannotBeOpened;
		goto upgrade_init_fail;
	}
#endif /* #if BL_DEBUG_MODE */

	return BL_Status_Success;

#if BL_DEBUG_MODE

upgrade_init_fail:
	if (DRV_UART_INVALID_HANDLER != upgradeSettings.uartHandle)
	{
		Drv_UART_Release(upgradeSettings.uartHandle);
	}

	if (DRV_TIMER_INVALID_HANDLE != upgradeSettings.timerHandle)
	{
		Drv_Timer_Release(upgradeSettings.timerHandle);
	}

	return status;
#endif /* #if BL_DEBUG_MODE */
}

/*
 * Releases all resources used during fw upgrade
 */
PRIVATE ALWAYS_INLINE void DeInitializeBLUpgradeModule(void)
{
	Drv_UART_Release(upgradeSettings.uartHandle);
	Drv_Timer_Release(upgradeSettings.timeoutTimerHandle);
}

/***************************** PUBLIC FUNCTIONS *******************************/
/*
 * Upgrades Firmware
 */
BLStatusCode BL_UpgradeFirmware(void)
{
	BLStatusCode status = BL_Status_Success;

	/* Initialize Module First */
	status = InitializeBLUpgradeModule();

#if BL_DEBUG_MODE
	if (status != BL_Status_Success;)
	{
		return status;
	}
#endif

	/* Start Timeout Timer First */
	Drv_Timer_Start(upgradeSettings.timeoutTimerHandle, BL_UPGRADE_TIMEOUT_IN_MS);

	/* TODO Move to suitable area */
	status = ProcessMessageImageUpload();

    DeInitializeBLUpgradeModule();

	return status;

}
