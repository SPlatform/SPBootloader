/*******************************************************************************
 *
 * @file Bootloader.h
 *
 * @author MC
 *
 * @brief Bootloader Configurations
 *
 * @see https://github.com/SPlatform/SPBootloader/wiki
 *
 ******************************************************************************
 *
 * GNU GPLv3
 *
 * Copyright (c) 2016 SP
 *
 *  See LICENSE file in Root Directory for license details.
 *
 *******************************************************************************/
#ifndef __BOOTLOADER_CONFIG_H
#define __BOOTLOADER_CONFIG_H

/********************************* INCLUDES ***********************************/
#include "postypes.h"
/***************************** MACRO DEFINITIONS ******************************/

/***************************** TYPE DEFINITIONS *******************************/

/*
 * This define should be moved to suitable area which is accessible by
 * build system.
 * Firmware code should be linked according to this address.
 */
#define FIRMWARE_START_ADDRESS               	(0x10000)

/* TODO This value should be common for all images (BL, FW, User Apps)*/
#define FIRMWARE_SIGNATURE_LENGTH              	(256)

#define FIRMWARE_METADATA_LENGTH               	(256 + FIRMWARE_SIGNATURE_LENGTH)


/* Timer Number of FW Upgrade Timeout */
#define BL_FW_UPGRADE_TIMEOUT_TIMER_NO			(0)

/* UART */
#define BL_FW_UPGRADE_UART_NO					(0)
/* */
#define BL_FW_UPGRADE_UART_BAUD_RATE			(115200)

/* TODO Remove Test Mode */
#define BL_TEST_MODE							(1)

#ifdef _WIN32
#define SIMULATION_MODE							(1)
#else
#define SIMULATION_MODE							(0)
#endif

#if BL_TEST_MODE

/* TODO Remove */
#include "TestData.h"

#endif

#endif	/* __BOOTLOADER_CONFIG_H */
