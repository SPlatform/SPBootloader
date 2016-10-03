/********************************* INCLUDES ***********************************/

#include "Bootloader_Config.h"

#include "Debug.h"
#include "postypes.h"

/***************************** MACRO DEFINITIONS ******************************/

#define BL_JumpToFirmware                   Drv_CPUCore_JumpToImage

/***************************** TYPE DEFINITIONS *******************************/
/*
 * Bootlaoder Status Codes
 */
typedef enum
{
	BL_Status_Success = 1,
	/* Security Status Codes */
	BL_StatusSecurity_BadInput = 10,
	BL_StatusSecurity_InvalidRSASignFormat = 11,
	BL_StatusSecurity_MDVerFail = 12,
	BL_StatusSecurity_RSAVerFail = 13,

	BL_StatusDev_UartPortCannotBeOpened = 30,
	BL_StatusDev_TimerCannotBeCreated,

	BL_StatusUpgrade_InCompatibleFWOffset = 50,
	BL_StatusUpgrade_FWExceedsFlash,
	BL_StatusUpgrade_Timeout,



} BLStatusCode;


typedef struct
{
	uint32_t imageSize;
	uint32_t imageOffset;
} FirmwareMetaDataHeader;

typedef struct
{
    FirmwareMetaDataHeader header;
    uint32_t padding[(FIRMWARE_SIGNATURE_LENGTH - sizeof(FirmwareMetaDataHeader)) / sizeof(uint32_t)];
    uint8_t imageSignature[FIRMWARE_SIGNATURE_LENGTH];
    uint32_t image[1];
} FirmwareInfo;

/**************************** FUNCTION PROTOTYPES *****************************/

/******************************** VARIABLES ***********************************/

/**************************** PRIVATE FUNCTIONS ******************************/

//static FirmwareMetaData2 fwMetaData = { 0 };

/***************************** PUBLIC FUNCTIONS *******************************/
/*
 * Initializes Botloader Security
 *  Must be called before use security module. 
 *
 * @param none
 * @return none
 */
void BL_SecurityInit(void);

/*
 * Validates firmware Image. 
 * 
 *  Caller must provide image and its valid signature. 
 *  Must be called before use security module. 
 *  Also public keys which used to sign image must be provided. 
 *  
 *
 * @param fwMetaData Meta Data of Firmware. Includes image and signature info
 * @param rsaPublicKey RSA keys to validate image
 *
 * @retval BL_Status_Success Image has a valid signature and validation is OK
 * @retval BL_StatusSecurity_BadInput Invalid parameters
 * @retval BL_StatusSecuirty_InvalidRSASignFormat Invalid signature format
 * @retval BL_StatusSecurity_MDVerFail MD (Integrity) verification failure
 * @retval BL_StatusSecurity_RSAVerFail RSA validation failure
 *
 */
BLStatusCode BL_ValidateImage(FirmwareInfo* fwMetaData);

BLStatusCode BL_UpgradeFirmware(void);
