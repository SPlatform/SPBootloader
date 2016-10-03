/*******************************************************************************
 *
 * @file Drv_CPUCore.h
 *
 * @author MC
 *
 * @brief UART Driver Interface
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

#ifndef __DRV_UART_H
#define __DRV_UART_H

/********************************* INCLUDES ***********************************/
#include "postypes.h"

/***************************** MACRO DEFINITIONS ******************************/
#define DRV_UART_INVALID_HANDLER			(-1)
/***************************** TYPE DEFINITIONS *******************************/
typedef int32_t UartHandle;

typedef void(*UARTDataReceivedEventHandler)(void);
/*************************** FUNCTION DEFINITIONS *****************************/
void Drv_UART_Init(void);

UartHandle Drv_UART_Get(uint32_t uartNo, uint32_t baudRate, UARTDataReceivedEventHandler dataReceivedEventHandler);
void Drv_UART_Release(UartHandle uart);

int32_t Drv_UART_Send(UartHandle uart, uint8_t* sendBuffer, uint32_t sendLength);
/*
 * @return Number of read bytes. Returns zero if there is no unread bytes
 * @return -1 In case of error
 *
 */
int32_t Drv_UART_Receive(UartHandle uart, uint8_t* receiveBuffer, uint32_t receiveLength);

#endif	/* __DRV_UART_H */
