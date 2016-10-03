/*******************************************************************************
 *
 * @file IntelHex.h
 *
 * @author MC
 *
 * @brief Intel Hex Parser Library
 *
 * @see
 *
 ******************************************************************************
 *
 * GNU GPLv3
 *
 * Copyright (c) 2016 SP
 *
 *  See LICENSE file in Root Directory for license details.
 *
 ******************************************************************************/

#ifndef __INTEL_HEX_H
#define __INTEL_HEX_H

/********************************* INCLUDES ***********************************/

#include "postypes.h"

/***************************** MACRO DEFINITIONS ******************************/

/*
 * Intel HEX lines starts with ':' prefix
 */
#define INTELHEX_PREFIX									':'

/* While dynamic memory is not safe, data length of intel hex is limited*/
#define INTELHEX_ALLOWED_MAX_DATA_LENGTH				(32)

/*
 * Intel HEX record(line) types
 */
#define INTELHEX_RECORDTYPE_DATA						(0)
#define INTELHEX_RECORDTYPE_EOF							(1)
#define INTELHEX_RECORDTYPE_EXTENDED_SEGMENT_ADDRESS	(2)
#define INTELHEX_RECORDTYPE_EXTENDED_LINEAR_ADDRESS		(4)
#define INTELHEX_RECORDTYPE_START_LINEAR_ADDRESS		(5)

/* Intel Hex Segment Size is 64K */
#define INTELHEX_SEGMENT_SIZE							(64 * 1024)
/***************************** TYPE DEFINITIONS *******************************/
/*
 * Intel HEX Library Spesific Status Codes
*/
typedef enum
{
	/* Intel HEX process is success */
	IntelHex_Success = 0,
	/*
	 * Intel HEX Line includes more data than allowed.
	 * Please see INTELHEX_ALLOWED_MAX_DATA_LENGTH
	 */
	IntelHex_Err_DataLengthExceedsAllowed,
	/*
	 * Intel HEX line missing and can be retry once completed.
	 */
	IntelHex_Err_MissingLine,
	/*
	 * Intel HEX line imcomplete and no way to recover.
	 * Should be ignored
	 */
	IntelHex_Err_IncompleteLine,
	/*
	 * Intel HEX line corrupted.
	 */
	IntelHex_Err_CRCError
} IntelHexStatusCode;

/*
 * Intel HEX Line Parsed Details
 */
typedef struct
{
	/* Length of Intel HEX Data */
	uint32_t lenght;
	/* Record (Line) Type. INTELHEX_RECORDTYPE_ defines */
	uint32_t recordType;
	/* Address of data */
	uint32_t address;
	/* Intel HEX Data */
	uint8_t data[INTELHEX_ALLOWED_MAX_DATA_LENGTH];
	/* CRC of Intel HEX */
	uint8_t crc;
} IntelHexLine;

/*************************** FUNCTION DEFINITIONS *****************************/
/*
 * Parses an Intel HEX string and returns IntelHexLine object as parsed data.
 *
 * IMP :
 * - Caller is responsible to send a intel hex string which starts with ':'
 * and ends with terminator char ('\0')
 * - Check 'intelHexLine' variable if function returns success
 *
 * @param intelHexStr Intel HEX String to be parsed
 * @param intelHexStrLen Length of Intel HEX String
 * @param intelHexLine [out] Parsed Intel HEX Info. Function fills client
 *		  object with parsed info.
 * @param parsedLineLength [out] Parsed Line Length. It can be used to
 *		  calculate remaining data length to parse also remaining data later.
 *
 * @retval IntelHex_Success Intel HEX string is parsed successfully.
 * @retval IntelHex_Err_CRCError Intel HEX string is corrupted.
 *		   parsedLineLength returns corrupted but parsed data length.
 * @retval IntelHex_Err_IncompleteLine Intel HEX is incomplete and
 *		   unrecoverable. parsedLineLength returns incomplete line length.
 * @retval IntelHex_Err_MissingLine Intel HEX has missing part and once it is
 *		   completed string can be retried to parse again. parsedLineLength
 *		   returns all length of intel HEX string.
 * @retval IntelHex_Err_DataLengthExceedsAllowed this function has a data
 *		   length limitation. Please see INTELHEX_ALLOWED_MAX_DATA_LENGTH
 */
IntelHexStatusCode IntelHex_Parse(uint8_t* intelHexStr, uint32_t intelHexStrLen, IntelHexLine* intelHexLine, uint32_t* parsedLineLength);

#endif	/* __INTEL_HEX_H */
