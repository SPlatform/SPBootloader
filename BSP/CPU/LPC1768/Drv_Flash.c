/*******************************************************************************
*
* @file Drv_Flash.c
*
* @author MC
*
* @brief Flash Driver for LPC17xx
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
#include "Drv_CPUCore.h"

/***************************** MACRO DEFINITIONS ******************************/
/*
 * LPC17xx allows flash programming using IAP interface.
 *  IAP interface located in a specific address.
 */
#define IAP_INTERFACE_ADDRESS                       (0x1fff1ff1)

/* LPC1768 has 512K Flash */
#define FLASH_LPC17xx_FLASH_SIZE                    (0x80000)
/* 32K Block starts from 0x10000 address */
#define FLASH_LPC17xx_32KPAGES_START_ADDRESS        (0x10000)
/* LPC1768 has 16 4K blocks*/
#define FLASH_LPC17xx_4K_BLOCK_COUNT                (16)
/* 4K Block size */
#define FLASH_4K_BLOCK_SIZE                         (4 * 1024)
/* 32K Block size */
#define FLASH_32K_BLOCK_SIZE                        (32 * 1024)

/* IAP interface requires actual CPU Core in KHz for flash writes */
#define CPU_CLOCK_IN_KHZ()                          (Drv_CPUCore_GetCPUFrequency() / 1000)

/***************************** TYPE DEFINITIONS *******************************/
/*
 * IAP API to program flash during execution
 */
typedef void (*IAPInterface)(unsigned long *, unsigned long *);

/*
 * IAP commands
 */
typedef enum
{
	IAP_CMD_PREPARE_SECTOR = 50,
	IAP_CMD_COPY_RAM_TO_FLASH,
	IAP_CMD_ERASE_SECTORS,
	IAP_CMD_BLANK_CHECK,
	IAP_CMD_READ_PART_ID,
	IAP_CMD_READ_BOOTCODE_VERSION,
	IAP_CMD_COMPARE,
	IAP_CMD_REINVOKE_ISP
} IAPCommands;

/*
 * IAP Status Codes
 */
typedef enum
{
	IAP_STATUS_SUCCESS,
	IAP_STATUS_INVALID_CMD,
	IAP_STATUS_SRC_ADDR_ERROR,
	IAP_STATUS_DST_ADDR_ERROR,
	IAP_STATUS_SRC_ADDR_NOT_MAPPED,
	IAP_STATUS_DST_ADDR_NOT_MAPPED,
	IAP_STATUS_COUNT_ERROR,
	IAP_STATUS_INVALID_SECTOR,
	IAP_STATUS_SECTOR_NOT_BLANK,
	IAP_STATUS_SECTOR_NOT_PREPARED,
	IAP_STATUS_COMPARE_ERROR,
	IAP_STATUS_BUSY
} IAPStatus;

/*
 * IAP Parameters to check block status
 */
typedef struct
{
    unsigned long iapCommand;
    unsigned long startSector;
    unsigned long endSector;
    unsigned long __reserved;
    unsigned long __reserved2;
} IAPFlashBlockStatusParams;

/*
 * IAP Parameters for flash write
 */
typedef struct
{
    unsigned long iapCommand;
    unsigned long destination;
    unsigned long source;
    unsigned long byteCount;
    unsigned long cpuClockInKHZ;
} IAPFlashWriteParams;

/*
 * IAP Parameters to erase block
 */
typedef struct
{
    unsigned long iapCommand;
    unsigned long startSector;
    unsigned long endSector;
    unsigned long cpuClockInKHZ;
    unsigned long __reserved;
} IAPFlashEraseParams;

/* IAP Result structure */
typedef struct
{
    unsigned long result;
    unsigned long __reserved;
    unsigned long __reserved2;
} IAPResult;

/**************************** FUNCTION PROTOTYPES *****************************/

/******************************** VARIABLES ***********************************/
/* IAP command runner interface */
PRIVATE IAPInterface runIAPCommand = (IAPInterface)IAP_INTERFACE_ADDRESS;

/**************************** PRIVATE FUNCTIONS ******************************/
/**
 * Returns start address of specified block
 */
PRIVATE ALWAYS_INLINE uint32_t getBlockAddress(uint32_t blockNo)
{
    uint32_t blockAddress;
    /*
     * TODO LPC17xx family has different sizes of flash. Add runtime size check
     */
    if (blockNo < FLASH_LPC17xx_4K_BLOCK_COUNT)
    {
        blockAddress = blockNo * FLASH_4K_BLOCK_SIZE;
    }
    else
    {
        blockNo -= FLASH_LPC17xx_4K_BLOCK_COUNT;

        blockAddress = FLASH_LPC17xx_32KPAGES_START_ADDRESS;
        blockAddress += (blockNo * FLASH_32K_BLOCK_SIZE);
    }

    return blockAddress;
}

/***************************** PUBLIC FUNCTIONS *******************************/
/**
 * Initializes Flash Driver
 */
PUBLIC void Drv_Flash_Init(void)
{
}

/**
 * Erases range of blocks.
 *  Drv_Flash_PrepareBlock must be called before
 */
PUBLIC int32_t Drv_Flash_EraseBlockRange(uint32_t startBlockNo, uint32_t endBlockNo)
{
	int32_t retVal = RESULT_SUCCESS;
    IAPFlashEraseParams eraseParams;
    IAPResult iapResult;

    iapResult.result = 0;

	/* Prepare Erase Command */
    eraseParams.iapCommand = IAP_CMD_ERASE_SECTORS;
    eraseParams.startSector = (unsigned long)startBlockNo;
    eraseParams.endSector = (unsigned long)endBlockNo;
    eraseParams.cpuClockInKHZ = (unsigned long)CPU_CLOCK_IN_KHZ();

    /*
     * Enter critical section.
     * Need to disable interupts first
     */
    Drv_CPUCore_DisableInterrupts();

	/* Run erase command */
    runIAPCommand((unsigned long *)&eraseParams, (unsigned long *)&iapResult);

	/* Exit critical section */
    Drv_CPUCore_EnableInterrupts();

    if (iapResult.result != IAP_STATUS_SUCCESS)
    {
    	retVal = RESULT_FAIL;
    }

	return retVal;
}

/**
 * Erases single block
 *  Drv_Flash_PrepareBlock must be called before
 */
int32_t Drv_Flash_EraseBlock(uint32_t blockNo)
{
    return Drv_Flash_EraseBlockRange(blockNo, blockNo);
}

/**
 * Writes data to flash address
 *  Drv_Flash_PrepareBlock must be called before
 *
 */
int32_t Drv_Flash_Write(uint32_t address, uint8_t* data, uint32_t length)
{
	int32_t retVal = RESULT_SUCCESS;
	IAPFlashWriteParams writeParams;
    IAPResult iapResult;

    iapResult.result = 0;

	/* Prepare Flash Write Command */
    writeParams.iapCommand = IAP_CMD_COPY_RAM_TO_FLASH;
    writeParams.destination = (unsigned long)address;
    writeParams.source = (unsigned long)data;
    writeParams.byteCount = (unsigned long)length;
    writeParams.cpuClockInKHZ = (unsigned long)CPU_CLOCK_IN_KHZ();

    /*
     * Enter critical section.
     * Need to disable interupts first
     */
    Drv_CPUCore_DisableInterrupts();

	/* Run Flash Write Command */
    runIAPCommand((unsigned long *)&writeParams, (unsigned long *)&iapResult);

	/* Exit from Critical Section */
    Drv_CPUCore_EnableInterrupts();

    if (iapResult.result != IAP_STATUS_SUCCESS)
    {
    	retVal = RESULT_FAIL;
    }

    return retVal;
}

/**
 * Writes data to block (from start address of block)
 *  Drv_Flash_PrepareBlock must be called before
 *
 */
int32_t Drv_Flash_WriteBlock(uint32_t blockNo, uint8_t* data, uint32_t length)
{
	uint32_t blockAddress;

    blockAddress = getBlockAddress(blockNo);

	return Drv_Flash_Write(blockAddress, data, length);
}

/**
 * Prepares Block for Write/Erase operations
 *
 */
int32_t Drv_Flash_PrepareBlockRange(uint32_t startBlockNo, uint32_t endBlockNo)
{
    IAPFlashBlockStatusParams statusParams;
    IAPResult iapResult;

    iapResult.result = 0;

    statusParams.iapCommand = IAP_CMD_PREPARE_SECTOR;
	statusParams.startSector = (unsigned long)startBlockNo;
    statusParams.endSector = (unsigned long)endBlockNo;

    /*
     * Enter critical section.
     * Need to disable interupts first
     */
    Drv_CPUCore_DisableInterrupts();

    runIAPCommand((unsigned long *)&statusParams, (unsigned long *)&iapResult);

    Drv_CPUCore_EnableInterrupts();

    if (iapResult.result == IAP_STATUS_SUCCESS)
    {
        return FLASH_STATUS_SUCCESS;
    }
    else if (iapResult.result == IAP_STATUS_BUSY)
    {
        return FLASH_STATUS_BUSY;
    }

    return FLASH_STATUS_FAILURE;
}

int32_t Drv_Flash_PrepareBlock(uint32_t blockNo)
{
    return Drv_Flash_PrepareBlockRange(blockNo, blockNo);
}

int32_t Drv_Flash_GetBlockNoOfAddress(uint32_t address)
{
    int32_t blockNo;

    if (address >= FLASH_LPC17xx_FLASH_SIZE)
    {
        return -1;
    }

    if (address < FLASH_LPC17xx_32KPAGES_START_ADDRESS)
    {
        blockNo = (int32_t)address / FLASH_4K_BLOCK_SIZE;
    }
    else
    {
        address -= FLASH_LPC17xx_32KPAGES_START_ADDRESS;

        blockNo = FLASH_LPC17xx_4K_BLOCK_COUNT;
        blockNo += address / FLASH_32K_BLOCK_SIZE;
    }

    return blockNo;
}

uint32_t Drv_Flash_GetSize(void)
{
	return FLASH_LPC17xx_FLASH_SIZE;
}
