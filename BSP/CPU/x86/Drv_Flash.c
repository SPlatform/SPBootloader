/*******************************************************************************
*
* @file Drv_Flash.h
*
* @author MC
*
* @brief Flash Driver Interface
*
* @see
*
******************************************************************************
*
*  Copyright (2016), P-OS
*
*   This software may be modified and distributed under the terms of the
*   'MIT License'.
*
*   See the LICENSE file for details.
*
******************************************************************************/

#ifndef __DRV_FLASH_H
#define __DRV_FLASH_H

/********************************* INCLUDES ***********************************/
#include "Drv_Flash.h"

#include "postypes.h"

/***************************** MACRO DEFINITIONS ******************************/

#define FLASH_LPC17xx_32KPAGES_START_ADDRESS        (0x10000)
#define FLASH_LPC17xx_FLASH_SIZE                    (0x80000)
#define FLASH_LPC17xx_4K_BLOCK_COUNT                (16)
#define FLASH_4K_BLOCK_SIZE                         (4 * 1024)
#define FLASH_32K_BLOCK_SIZE                        (32 * 1024)


/***************************** TYPE DEFINITIONS *******************************/

/**************************** FUNCTION PROTOTYPES *****************************/

/******************************** VARIABLES ***********************************/

/**************************** PRIVATE FUNCTIONS ******************************/

/***************************** PUBLIC FUNCTIONS *******************************/
void Drv_Flash_Init(void)
{

}

int32_t Drv_Flash_PrepareBlock(uint32_t blockNo)
{
	return 0;
}

int32_t Drv_Flash_PrepareBlockRange(uint32_t startBlockNo, uint32_t endBlockNo)
{
	return 0;
}

int32_t Drv_Flash_EraseBlock(uint32_t blockNo)
{
	return 0;
}
int32_t Drv_Flash_EraseBlockRange(uint32_t startBlockNo, uint32_t endBlockNo)
{
	return 0;
}

int32_t Drv_Flash_Write(uint32_t address, uint8_t* data, uint32_t length)
{
	return 0;
}

int32_t Drv_Flash_WriteBlock(uint32_t blockNo, uint8_t* data, uint32_t length)
{
	return 0;
}

int32_t Drv_Flash_GetBlockNoOfAddress(uint32_t address)
{
	return 0;
}

uint32_t Drv_Flash_GetSize(void)
{
	return FLASH_LPC17xx_FLASH_SIZE;
}

#endif	/* __DRV_FLASH_H */
