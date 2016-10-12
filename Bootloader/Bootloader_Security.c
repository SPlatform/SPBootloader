/*******************************************************************************
 *
 * @file Bootloader_Security.c
 *
 * @author MC
 *
 * @brief Security related implementation for Bootloader.
 *		  
 *          RESPONSIBILITIES
 *          1 - Image signature verification
 *          2 - Key Management
 *
 *          IMPLEMENTATION DETAILS
 *          - In that implementation mbedTLS is used for software encryption/
 *          decryption and validation for the first phase. 
 *          - Any HW Crypto support not implemented yet but must be done for 
 *          faster crypto and reduced memory footprint.  
 *          - In first phase, we do not support dynamic memory so we need to 
 *          provide a memory area for mbedTLS. See 'mbedTLSDynamicMemory' 
 *          variable
 *          
 *          ROAD MAP
 *          1 - MBEDTLS_PKCS1_V15 is used but MBEDTLS_PKCS1_V21 should be 
 *          supported also. 
 *          2 - SHA256 is used but SHA512 can be supported also. 
 *          
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

#include "Bootloader_Internal.h"
#include "Bootloader_Config.h"

#include "mbedtls/config.h"
#include "mbedtls/platform.h"
#include "mbedtls/rsa.h"
#include "mbedtls/md.h"
#include "mbedtls/memory_buffer_alloc.h"

#include "postypes.h"

/***************************** MACRO DEFINITIONS ******************************/

#if defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C)
/*
 * Dynamic Memory Size for mbedTLS
 *  TODO This size is enough for RSA 2048 + SHA256.
 *  Shouls be evaluated when a feature is added or used.
 */
#define BL_SECURITY_MBEDTLS_DYN_MEM_SIZE            (8 * 1024)
#endif /* #if defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C) */

/***************************** TYPE DEFINITIONS *******************************/

/*
 * RSA Public Keys
 */
typedef struct
{
	char* publicKeyN;   /* N Part of RSA Key */
	char* publicKeyE;   /* E Part of RSA Key */
} RSAPublicKey;

/**************************** FUNCTION PROTOTYPES *****************************/

/******************************** VARIABLES ***********************************/
/*
 * If Dynamic Memory (heap) is not supported, a memory area must be provided to
 * mbedTLS library. 
 * mbedTLS library uses
 */
#if defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C)
PRIVATE uint8_t mbedTLSDynamicMemory[BL_SECURITY_MBEDTLS_DYN_MEM_SIZE];
#endif  /* #if defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C) */

/**************************** PRIVATE FUNCTIONS ******************************/

/*
 * Reads RSA Keys and Returns
 *
 */
PRIVATE ALWAYS_INLINE void GetRSAKey(RSAPublicKey* key)
{
    /* TODO Remove Test Mode */
#if BL_TEST_MODE
	key->publicKeyN = (char*)TEST_PUBLIC_KEY_N;
	key->publicKeyE = (char*)TEST_PUBLIC_KEY_E;
#else   /* #if BL_TEST_MODE */
#error "Not defined yet!"
#endif  /* #if BL_TEST_MODE */
}

/***************************** PUBLIC FUNCTIONS *******************************/
/*
 * Initializes Security Module
 */
INTERNAL void BL_SecurityInit(void)
{
#if defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C)
    mbedtls_memory_buffer_alloc_init(mbedTLSDynamicMemory, sizeof(mbedTLSDynamicMemory));
#else   /* #if defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C)*/
    #error "You need to initialize Heap for dynamic memory allocations (e.g. calloc, free)"
#endif  /* #if defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C) */
}

/*
 * Validates Image using its Signature with RSA Keys
 * 
 *	Uses RSA2048 and SHA256 to verify and validate images. 
 *
 */
INTERNAL BLStatusCode BL_ValidateImage(FirmwareInfo* fwMetaData)
{
    RSAPublicKey rsaPublicKey;
	BLStatusCode status = BL_Status_Success;
	int32_t retVal = false;
	mbedtls_rsa_context rsa;
	unsigned char hash[32];
    
    /* Get RSA Keys first */
    GetRSAKey(&rsaPublicKey);

	/* Initialize RSA object */
	mbedtls_rsa_init(&rsa, MBEDTLS_RSA_PKCS_V15, 0);

	/* Read public key */
	if ((retVal = mbedtls_mpi_read_string(&rsa.N, 16, rsaPublicKey.publicKeyN)) != 0 ||
		(retVal = mbedtls_mpi_read_string(&rsa.E, 16, rsaPublicKey.publicKeyE)) != 0)
	{
		status = BL_StatusSecurity_BadInput;
		
		goto exit;
	}

	/* TODO What do '+7' and '>>3 mean?'*/
	rsa.len = (mbedtls_mpi_bitlen(&rsa.N) + 7) >> 3;
	
	if (rsa.len != FIRMWARE_SIGNATURE_LENGTH)
	{
		status = BL_StatusSecurity_InvalidRSASignFormat;

		goto exit;
	}

    /* Check Data Integrity according to SHA */
	retVal = mbedtls_md(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256),
						(unsigned char*)fwMetaData->image, fwMetaData->header.imageSize, hash);

	if (retVal != 0)
	{
		status = BL_StatusSecurity_MDVerFail;

		goto exit;
	}

    /* Check RSA Signature */
	retVal = mbedtls_rsa_pkcs1_verify(&rsa, NULL, NULL, MBEDTLS_RSA_PUBLIC,
									  MBEDTLS_MD_SHA256, 20, hash, 
									  fwMetaData->imageSignature);
	if (retVal != 0)
	{
		status = BL_StatusSecurity_RSAVerFail;

		goto exit;
	}

	status = BL_Status_Success;	

exit:
	/* Free RSA Resources */
	mbedtls_rsa_free(&rsa);

	return status;
}
