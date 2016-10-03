/*******************************************************************************
*
* @file IntelHex.c
*
* @author MC
*
* @brief Intel Hex Parser Implementation
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

/********************************* INCLUDES ***********************************/

#include "IntelHex.h"

/***************************** MACRO DEFINITIONS ******************************/

/*
 * Intel Hex header
 * :LLAAAART
 *  LL			: Length
 *    AAAA		: Address
 *        RT	: Record Type
 */
#define INTELHEX_HEADER_FORMAT				":%2x%4x%2x"

/* Length of Intel HEX Header (INTELHEX_HEADER_FORMAT) */
#define INTELHEX_HEADER_LENGTH				(9)

/* Intel HEX  CRC String Length */
#define INTELHEX_CRC_LENGTH					(2)

/* Offset of Data in Intel HEX string */
#define INTELHEX_DATA_OFFSET				(INTELHEX_HEADER_LENGTH)

/* String length of a regular Intel HEX */
#define INTEL_HEX_LINE_LENGTH(dataLength)	(INTELHEX_HEADER_LENGTH + (dataLength * 2) + INTELHEX_CRC_LENGTH)

/***************************** TYPE DEFINITIONS *******************************/

/*************************** FUNCTION DEFINITIONS *****************************/
/**
 * Parses Intel HEX String
 */
IntelHexStatusCode IntelHex_Parse(uint8_t* intelHexStr, uint32_t intelHexStrLength, IntelHexLine* intelHexLine, uint32_t* parsedLineLength)
{
	uint32_t index;
	uint8_t* dataPtr;
	uint8_t crcSum = 0;
	uint32_t intelHexLineLength = 0;
	char* secondPrefixPtr;
	uint32_t secondPrefixOffset;

	*parsedLineLength = 0;

	/* Intel HEX String should be longer than Intel HEX Header Length */
	if (intelHexStrLength < INTELHEX_HEADER_LENGTH)
	{
		*parsedLineLength = intelHexStrLength;
		return IntelHex_Err_MissingLine;
	}

	/* Parse Intel HEX Header first */
	sscanf((char*)intelHexStr, INTELHEX_HEADER_FORMAT,
		&intelHexLine->lenght,
		&intelHexLine->address,
		&intelHexLine->recordType);

	/* Check allowed data length */
	if (intelHexLine->lenght > INTELHEX_ALLOWED_MAX_DATA_LENGTH)
	{
		return IntelHex_Err_DataLengthExceedsAllowed;
	}

	intelHexLineLength = INTEL_HEX_LINE_LENGTH(intelHexLine->lenght);
	/* Intel HEX String should not be shorter than regular length */
	if (intelHexStrLength < intelHexLineLength)
	{
		*parsedLineLength = intelHexStrLength;
		return IntelHex_Err_MissingLine;
	}

	secondPrefixPtr = strchr((char*)(&intelHexStr[1]), INTELHEX_PREFIX);
	if (secondPrefixPtr != NULL)
	{
		/*
		 * If there exists a second Intel HEX Prefix, we can check
		 * whether first intel HEX part is valid
		 */
		secondPrefixOffset = secondPrefixPtr - (char*)intelHexStr;
		if (secondPrefixOffset < intelHexLineLength)
		{
			*parsedLineLength = secondPrefixOffset;
			return IntelHex_Err_IncompleteLine;
		}
	}

	/* Start to calculate crc of intel hex line */
	crcSum = intelHexLine->lenght + (intelHexLine->address >> 8) + (intelHexLine->address & 0xFF) + intelHexLine->recordType;

	/* Get offset of Data */
	dataPtr = &intelHexStr[INTELHEX_DATA_OFFSET];

	for (index = 0; index < intelHexLine->lenght; index++)
	{
		/* Parse all data byte by byte */
		sscanf((char*)dataPtr, "%2x", (uint32_t*)&intelHexLine->data[index]);

		dataPtr += 2;

		/* Add also parsed byte to crc calculation */
		crcSum += intelHexLine->data[index];
	}

	/* parse crc */
	sscanf((char*)dataPtr, "%2x", (uint32_t*)&intelHexLine->crc);

	/* need to two complimentary to finalize crc calculation ss*/
	crcSum = (~crcSum) + 1;

	/* Content CRC Check */
	if (crcSum != intelHexLine->crc)
	{
		*parsedLineLength = intelHexLineLength;
		return IntelHex_Err_CRCError;
	}

	/* return parsed line length */
	*parsedLineLength = intelHexLineLength;

	return IntelHex_Success;
}
