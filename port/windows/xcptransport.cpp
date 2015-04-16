/************************************************************************************//**
* \file         port\win32\xcptransport.c
* \brief        XCP transport layer interface source file.
* \ingroup      SerialBoot
* \internal
*----------------------------------------------------------------------------------------
*                          C O P Y R I G H T
*----------------------------------------------------------------------------------------
*   Copyright (c) 2014  by Feaser    http://www.feaser.com    All rights reserved
*
*----------------------------------------------------------------------------------------
*                            L I C E N S E
*----------------------------------------------------------------------------------------
* This file is part of OpenBLT. OpenBLT is free software: you can redistribute it and/or
* modify it under the terms of the GNU General Public License as published by the Free
* Software Foundation, either version 3 of the License, or (at your option) any later
* version.
*
* OpenBLT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
* without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
* PURPOSE. See the GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along with OpenBLT.
* If not, see <http://www.gnu.org/licenses/>.
*
* A special exception to the GPL is included to allow you to distribute a combined work
* that includes OpenBLT without being obliged to provide the source code for any
* proprietary components. The exception text is included at the bottom of the license
* file <license.html>.
*
* \endinternal
****************************************************************************************/

/****************************************************************************************
* Include files
****************************************************************************************/
#include <assert.h>                                   /* assertion module              */
#include <windows.h>                                  /* for WIN32 library             */
#include <string.h>                                   /* string library                */
#include "xcpmaster.h"                                /* XCP master protocol module    */
#include "timeutil.h"                                 /* time utility module           */


/****************************************************************************************
* Macro definitions
****************************************************************************************/
#define UART_TX_BUFFER_SIZE      (1024)               /**< transmission buffer size    */
#define UART_RX_BUFFER_SIZE      (1024)               /**< reception buffer size       */

/** \brief maximum number of bytes in a transmit/receive XCP packet in UART. */
#define XCP_MASTER_UART_MAX_DATA ((XCP_MASTER_TX_MAX_DATA>XCP_MASTER_RX_MAX_DATA) ? \
                                   (XCP_MASTER_TX_MAX_DATA+1) : (XCP_MASTER_RX_MAX_DATA+1))

/** \brief The smallest time in millisecond that the UART is configured for. */
#define UART_RX_TIMEOUT_MIN_MS   (100)


/****************************************************************************************
* Local data declarations
****************************************************************************************/
static tXcpTransportResponsePacket responsePacket;
static HANDLE hUart = INVALID_HANDLE_VALUE;

string error;
void setError()
{
	DWORD dwLastError = ::GetLastError();
	TCHAR lpBuffer[256] = TEXT("?");
	if (dwLastError != 0)   // Don't want to see a "operation done successfully" error ;-)
		::FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,                 // It´s a system error
		                NULL,                                      // No string to be formatted needed
		                dwLastError,                               // Hey Windows: Please explain this error!
		                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Do it in the standard language
		                lpBuffer,              // Put the message here
		                sizeof(lpBuffer) - 1,                   // Number of bytes to store the message
		                NULL);
	error = lpBuffer;
}
const string& XcpTransportGetLastError()
{
	return error;
}
/************************************************************************************//**
** \brief     Initializes the communication interface used by this transport layer.
** \param     device Serial communication device name. For example "COM4".
** \param     baudrate Communication speed in bits/sec.
** \return    true if successful, false otherwise.
**
****************************************************************************************/
uint8_t XcpTransportInit(const char *device, uint32_t baudrate)
{
	COMMTIMEOUTS timeouts = { 0 };
	DCB dcbSerialParams = { 0 };
	char portStr[64] = "\\\\.\\\0";
	
	/* construct the COM port name as a string */
	strncat(portStr, device, 59);
	
	/* obtain access to the COM port */
	hUart = CreateFile(portStr, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING,
	                   FILE_ATTRIBUTE_NORMAL, 0);
	                   
	/* validate COM port handle */
	if (hUart == INVALID_HANDLE_VALUE)
	{
		setError();
		return false;
	}
	
	/* get current COM port configuration */
	dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
	if (!GetCommState(hUart, &dcbSerialParams))
	{
		XcpTransportClose();
		return false;
	}
	
	/* configure the baudrate and 8,n,1 */
	dcbSerialParams.BaudRate = baudrate;
	dcbSerialParams.ByteSize = 8;
	dcbSerialParams.StopBits = ONESTOPBIT;
	dcbSerialParams.Parity = NOPARITY;
	if (!SetCommState(hUart, &dcbSerialParams))
	{
		XcpTransportClose();
		return false;
	}
	
	/* set communication timeout parameters */
	timeouts.ReadIntervalTimeout = UART_RX_TIMEOUT_MIN_MS;
	timeouts.ReadTotalTimeoutConstant = UART_RX_TIMEOUT_MIN_MS;
	timeouts.ReadTotalTimeoutMultiplier = 1;
	timeouts.WriteTotalTimeoutConstant = UART_RX_TIMEOUT_MIN_MS;
	timeouts.WriteTotalTimeoutMultiplier = 1;
	if (!SetCommTimeouts(hUart, &timeouts))
	{
		XcpTransportClose();
		return false;
	}
	
	/* set transmit and receive buffer sizes */
	if (!SetupComm(hUart, UART_RX_BUFFER_SIZE, UART_TX_BUFFER_SIZE))
	{
		XcpTransportClose();
		return false;
	}
	
	/* empty the transmit and receive buffers */
	if (!FlushFileBuffers(hUart))
	{
		XcpTransportClose();
		return false;
	}
	/* successfully connected to the serial device */
	return true;
} /*** end of XcpTransportInit ***/


/************************************************************************************//**
** \brief     Transmits an XCP packet on the transport layer and attemps to receive the
**            response within the given timeout. The data in the response packet is
**            stored in an internal data buffer that can be obtained through function
**            XcpTransportReadResponsePacket().
** \return    true is the response packet was successfully received and stored,
**            false otherwise.
**
****************************************************************************************/
uint8_t XcpTransportSendPacket(uint8_t *data, uint8_t len, uint16_t timeOutMs)
{
	DWORD dwWritten = 0;
	DWORD dwRead = 0;
	uint32_t dwToRead;
	uint16_t cnt;
	static unsigned char xcpUartBuffer[XCP_MASTER_UART_MAX_DATA]; /* static to lower stack load */
	uint16_t xcpUartLen;
	uint8_t *uartReadDataPtr;
	uint32_t timeoutTime;
	
	/* ------------------------ XCP packet transmission -------------------------------- */
	/* prepare the XCP packet for transmission on UART. this is basically the same as the
	 * xcp packet data but just the length of the packet is added to the first byte.
	 */
	xcpUartLen = len + 1;
	xcpUartBuffer[0] = len;
	for (cnt = 0; cnt < len; cnt++)
	{
		xcpUartBuffer[cnt + 1] = data[cnt];
	}
	
	/* first submit the XCP packet for transmission */
	if (!WriteFile(hUart, xcpUartBuffer, xcpUartLen, &dwWritten, 0))
	{
		return false;
	}
	/* double check that all bytes were actually transmitted */
	if (dwWritten != xcpUartLen)
	{
		return false;
	}
	
	/* ------------------------ XCP packet reception ----------------------------------- */
	/* determine timeout time */
	timeoutTime = TimeUtilGetSystemTimeMs() + timeOutMs + UART_RX_TIMEOUT_MIN_MS;
	
	/* read the first byte, which contains the length of the xcp packet that follows */
	dwToRead = 1;
	uartReadDataPtr = &responsePacket.len;
	while (dwToRead > 0)
	{
		dwRead = 0;
		if (ReadFile(hUart, uartReadDataPtr, dwToRead, &dwRead, NULL))
		{
			/* update the bytes that were already read */
			uartReadDataPtr += dwRead;
			dwToRead -= dwRead;
		}
		/* check for timeout if not yet done */
		if ((dwToRead > 0) && (TimeUtilGetSystemTimeMs() >= timeoutTime))
		{
			/* timeout occurred */
			return false;
		}
	}
	
	/* read the rest of the packet */
	dwToRead = responsePacket.len;
	uartReadDataPtr = &responsePacket.data[0];
	while (dwToRead > 0)
	{
		dwRead = 0;
		if (ReadFile(hUart, uartReadDataPtr, dwToRead, &dwRead, NULL))
		{
			/* update the bytes that were already read */
			uartReadDataPtr += dwRead;
			dwToRead -= dwRead;
		}
		/* check for timeout if not yet done */
		if ((dwToRead > 0) && (TimeUtilGetSystemTimeMs() >= timeoutTime))
		{
			/* timeout occurred */
			return false;
		}
	}
	/* still here so the complete packet was received */
	return true;
} /*** end of XcpMasterTpSendPacket ***/


/************************************************************************************//**
** \brief     Reads the data from the response packet. Make sure to not call this
**            function while XcpTransportSendPacket() is active, because the data won't be
**            valid then.
** \return    Pointer to the response packet data.
**
****************************************************************************************/
tXcpTransportResponsePacket *XcpTransportReadResponsePacket(void)
{
	return &responsePacket;
} /*** end of XcpTransportReadResponsePacket ***/


/************************************************************************************//**
** \brief     Closes the communication channel.
** \return    none.
**
****************************************************************************************/
void XcpTransportClose(void)
{
	/* close the COM port handle if valid */
	if (hUart != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hUart);
	}
	
	/* set handles to invalid */
	hUart = INVALID_HANDLE_VALUE;
} /*** end of XcpTransportClose ***/


/*********************************** end of xcptransport.c *****************************/
