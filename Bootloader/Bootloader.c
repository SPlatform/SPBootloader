/*******************************************************************************
 *
 * @file Bootloader.c
 *
 * @author MC
 *
 * @brief Bootloader application implementation.
 *          - Listens for upgrade attempt
 *              - Upgrades Firmware
 *          - Validates Firmware Image
 *              - Jump to Firmware if Firmware has a valid signature
 *
 *          ROAD MAP
 *          1 - Firmware Upgrade
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

#include "DRV_CPUCore.h"

#include "Bootloader_Internal.h"
#include "Bootloader_Config.h"
#include "BSPConfig.h"

#include "postypes.h"

/***************************** MACRO DEFINITIONS ******************************/

/***************************** TYPE DEFINITIONS *******************************/

/*
 * Bootlader Internal Settings
 */
typedef struct
{
	/* Meta Data of Firmware */
	FirmwareMetaData firmwareMetaData;
} BootloaderSettings;

/**************************** FUNCTION PROTOTYPES *****************************/

/******************************** VARIABLES ***********************************/
/* Bootloader internal settings */
PRIVATE BootloaderSettings settings = { { 0 } };

/**************************** PRIVATE FUNCTIONS ******************************/
/*
 * Reads Firmware Area and returns Meta Data of Firmware
 * 
 */
PRIVATE ALWAYS_INLINE void GetMetaData(FirmwareMetaData* metaData)
{
    /* TODO Remove Test Mode */
#if BL_TEST_MODE
	metaData->imageAddress = (uint8_t*)TEST_HEX;
	metaData->imageSignatureAddress = (uint8_t*)TEST_HEX_SIGN;
	metaData->imageLength = strlen(TEST_HEX);
	metaData->imageSignatureLength = sizeof(TEST_HEX_SIGN) / sizeof(TEST_HEX_SIGN[0]);
#else
#error "Not defined yet!"
#endif
}

/*
 * Checks user interaction for new FW Upgrade Request. 
 *
 */
PRIVATE ALWAYS_INLINE bool CheckAndWaitForUpgradeAttemmp(void)
{
	/* TODO If there is no image return true */

	/* TODO Check external attemp like a pin */

	return (BOOL_TRUE);
}

/*
 * Checks whether image (firmware) is valid. 
 *  Valid image is an image which signed with valid signature. 
 *
 */
PRIVATE ALWAYS_INLINE bool IsValidImage(void)
{
	BLStatusCode statusCode;

	/* Get Meta Data */
	GetMetaData(&settings.firmwareMetaData);

	/* Check whether image is valid */
	statusCode = BL_ValidateImage(&settings.firmwareMetaData);

	if (BL_Status_Success != statusCode)
	{
        DEBUG_PRINT("\nBL Err:%d", statusCode);
        return BOOL_FALSE;
	}

	return BOOL_TRUE;
}

/*
 * Initializes HW
 *
 */
PRIVATE ALWAYS_INLINE int32_t InitializeHW(void)
{
    extern void SystemInit(void);
    extern void SystemCoreClockUpdate(void);
    
    /* 
     * SystemInit function was already called in startup.s file and it is 
     * moved here to call it explicitly
     */
    SystemInit();
    
    return RESULT_SUCCESS;
}
/***************************** PUBLIC FUNCTIONS *******************************/
/*
 * Bootloader application entry point
 *
 */
int main(void)
{
	bool upgradeFW = BOOL_FALSE;
	bool validImage = BOOL_FALSE;
    
    /* Initialize HW First */
    InitializeHW();
    
    /* Initialize Bootloader Security */
    BL_SecurityInit();
    
    do
    {
        /* Check new image attempt */
        upgradeFW = CheckAndWaitForUpgradeAttemmp();
        if (BOOL_TRUE == upgradeFW)
        {
            /*
             * TODO Download Image Here
             */
        }
        
        /* Check Whether Firmware is valid (signed) */
        validImage = IsValidImage();
        
        /* Try until have a valid image */
    } while (BOOL_FALSE == validImage);
    
    /*
     * Firmware is a validated image so just jump to firmware. 
     */
    BL_JumpToFirmware((uint32_t)settings.firmwareMetaData.imageAddress);
    

    return 0;
}
