/************************************************************************************//**
* \file         xcpmaster.c
* \brief        XCP Master source file.
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
#include <stdint.h>                                   /* assertion module              */
#include "xcpmaster.h"                                /* XCP master protocol module    */


/****************************************************************************************
* Macro definitions
****************************************************************************************/
/* XCP command codes as defined by the protocol currently supported by this module */
#define XCP_MASTER_CMD_CONNECT         (0xFF)
#define XCP_MASTER_CMD_DISCONNECT      (0xFE)
#define XCP_MASTER_CMD_SET_MTA         (0xF6)
#define XCP_MASTER_CMD_UPLOAD          (0xF5)
#define XCP_MASTER_CMD_PROGRAM_START   (0xD2)
#define XCP_MASTER_CMD_PROGRAM_CLEAR   (0xD1)
#define XCP_MASTER_CMD_PROGRAM         (0xD0)
#define XCP_MASTER_CMD_PROGRAM_RESET   (0xCF)
#define XCP_MASTER_CMD_PROGRAM_MAX     (0xC9)

/* XCP response packet IDs as defined by the protocol */
#define XCP_MASTER_CMD_PID_RES         (0xFF) /* positive response */

/* timeout values */
#define XCP_MASTER_CONNECT_TIMEOUT_MS  (20)
#define XCP_MASTER_TIMEOUT_T1_MS       (1000)  /* standard command timeout */
#define XCP_MASTER_TIMEOUT_T2_MS       (2000)  /* build checksum timeout */
#define XCP_MASTER_TIMEOUT_T3_MS       (2000)  /* program start timeout */
#define XCP_MASTER_TIMEOUT_T4_MS       (10000) /* erase timeout */
#define XCP_MASTER_TIMEOUT_T5_MS       (1000)  /* write and reset timeout */
#define XCP_MASTER_TIMEOUT_T6_MS       (1000)  /* user specific connect connect timeout */
#define XCP_MASTER_TIMEOUT_T7_MS       (2000)  /* wait timer timeout */

/** \brief Number of retries to connect to the XCP slave. */
#define XCP_MASTER_CONNECT_RETRIES     (5)


/****************************************************************************************
* Function prototypes
****************************************************************************************/
static uint8_t XcpMasterSendCmdConnect(void);
static uint8_t XcpMasterSendCmdSetMta(uint32_t address);
static uint8_t XcpMasterSendCmdUpload(uint8_t data[], uint8_t length);
static uint8_t XcpMasterSendCmdProgramStart(void);
static uint8_t XcpMasterSendCmdProgramReset(void);
static uint8_t XcpMasterSendCmdProgram(uint8_t length, uint8_t data[]);
static uint8_t XcpMasterSendCmdProgramMax(uint8_t data[]);
static uint8_t XcpMasterSendCmdProgramClear(uint32_t length);
static void     XcpMasterSetOrderedLong(uint32_t value, uint8_t data[]);


/****************************************************************************************
* Local data declarations
****************************************************************************************/
/** \brief Store the byte ordering of the XCP slave. */
static uint8_t xcpSlaveIsIntel = false;

/** \brief The max number of bytes in the command transmit object (master->slave). */
static uint8_t xcpMaxCto;

/** \brief The max number of bytes in the command transmit object (master->slave) during
 *         a programming session.
 */
static uint8_t xcpMaxProgCto = 0;

/** \brief The max number of bytes in the data transmit object (slave->master). */
static uint8_t xcpMaxDto;

/** \brief Internal data buffer for storing the data of the XCP response packet. */
// static tXcpTransportResponsePacket responsePacket;


/************************************************************************************//**
** \brief     Initializes the XCP master protocol layer.
** \param     device Serial communication device name. For example "COM4".
** \param     baudrate Communication speed in bits/sec.
** \return    true is successful, false otherwise.
**
****************************************************************************************/
uint8_t XcpMasterInit(const char *device, uint32_t baudrate)
{
  /* initialize the underlying transport layer that is used for the communication */
  return XcpTransportInit(device, baudrate);
} /*** end of XcpMasterInit ***/


/************************************************************************************//**
** \brief     Uninitializes the XCP master protocol layer.
** \return    none.
**
****************************************************************************************/
void XcpMasterDeinit(void)
{
  XcpTransportClose();
} /*** end of XcpMasterDeinit ***/


/************************************************************************************//**
** \brief     Connect to the XCP slave.
** \return    true is successfull, false otherwise.
**
****************************************************************************************/
uint8_t XcpMasterConnect(void)
{
  uint8_t cnt;
  
  /* try to connect with a finite amount of retries */
  for (cnt=0; cnt<XCP_MASTER_CONNECT_RETRIES; cnt++)
  {
    /* send the connect command */
    if (XcpMasterSendCmdConnect() == true)
    {
      /* connected so no need to retry */
      return true;
    }
  }
  /* still here so could not connect */
  return false;
} /*** end of XcpMasterConnect ***/


/************************************************************************************//**
** \brief     Disconnect the slave.
** \return    true is successfull, false otherwise.
**
****************************************************************************************/
uint8_t XcpMasterDisconnect(void)
{
  /* send reset command instead of the disconnect. this causes the user program on the
   * slave to automatically start again if present.
   */
  return XcpMasterSendCmdProgramReset();
} /*** end of XcpMasterDisconnect ***/

/************************************************************************************//**
** \brief     Puts a connected slave in programming session.
** \return    true is successfull, false otherwise.
**
****************************************************************************************/
uint8_t XcpMasterStartProgrammingSession(void)
{
  /* place the slave in programming mode */
  return XcpMasterSendCmdProgramStart();
} /*** end of XcpMasterStartProgrammingSession ***/


/************************************************************************************//**
** \brief     Stops the programming session by sending a program command with size 0 and
**            then resetting the slave.
** \return    true is successfull, false otherwise.
**
****************************************************************************************/
uint8_t XcpMasterStopProgrammingSession(void)
{
  /* stop programming by sending the program command with size 0 */
  if (XcpMasterSendCmdProgram(0, 0) == false)
  {
    return false;
  }
  /* request a reset of the slave */
  return XcpMasterSendCmdProgramReset();
} /*** end of XcpMasterStopProgrammingSession ***/


/************************************************************************************//**
** \brief     Erases non volatile memory on the slave.
** \param     addr Base memory address for the erase operation.
** \param     len Number of bytes to erase.
** \return    true is successfull, false otherwise.
**
****************************************************************************************/
uint8_t XcpMasterClearMemory(uint32_t addr, uint32_t len)
{
  /* first set the MTA pointer */
  if (XcpMasterSendCmdSetMta(addr) == false)
  {
    return false;
  }
  /* now perform the erase operation */
  return XcpMasterSendCmdProgramClear(len);
} /*** end of XcpMasterClearMemory ***/


/************************************************************************************//**
** \brief     Reads data from the slave's memory.
** \param     addr Base memory address for the read operation
** \param     len Number of bytes to read.
** \param     data Destination buffer for storing the read data bytes.
** \return    true is successfull, false otherwise.
**
****************************************************************************************/
uint8_t XcpMasterReadData(uint32_t addr, uint32_t len, uint8_t data[])
{
  uint8_t currentReadCnt;
  uint32_t bufferOffset = 0;

  /* first set the MTA pointer */
  if (XcpMasterSendCmdSetMta(addr) == false)
  {
    return false;
  }
  /* perform segmented upload of the data */
  while (len > 0)
  {
    /* set the current read length to make optimal use of the available packet data. */
    currentReadCnt = len % (xcpMaxDto - 1);
    if (currentReadCnt == 0)
    {
      currentReadCnt = (xcpMaxDto - 1);
    }
    /* upload some data */
    if (XcpMasterSendCmdUpload(&data[bufferOffset], currentReadCnt) == false)
    {
      return false;
    }
    /* update loop variables */
    len -= currentReadCnt;
    bufferOffset += currentReadCnt;
  }
  /* still here so all data successfully read from the slave */
  return true;
} /*** end of XcpMasterReadData ***/


/************************************************************************************//**
** \brief     Programs data to the slave's non volatile memory. Note that it must be
**            erased first.
** \param     addr Base memory address for the program operation
** \param     len Number of bytes to program.
** \param     data Source buffer with the to be programmed bytes.
** \return    true is successfull, false otherwise.
**
****************************************************************************************/
uint8_t XcpMasterProgramData(uint32_t addr, uint32_t len, uint8_t data[])
{
  uint8_t currentWriteCnt;
  uint32_t bufferOffset = 0;

  /* first set the MTA pointer */
  if (XcpMasterSendCmdSetMta(addr) == false)
  {
    return false;
  }
  /* perform segmented programming of the data */
  while (len > 0)
  {
    /* set the current read length to make optimal use of the available packet data. */
    currentWriteCnt = len % (xcpMaxProgCto - 1);
    if (currentWriteCnt == 0)
    {
      currentWriteCnt = (xcpMaxProgCto - 1);
    }
    /* prepare the packed data for the program command */
    if (currentWriteCnt < (xcpMaxProgCto - 1))
    {
      /* program data */
      if (XcpMasterSendCmdProgram(currentWriteCnt, &data[bufferOffset]) == false)
      {
        return false;
      }
    }
    else
    {
      /* program max data */
      if (XcpMasterSendCmdProgramMax(&data[bufferOffset]) == false)
      {
        return false;
      }
    }
    /* update loop variables */
    len -= currentWriteCnt;
    bufferOffset += currentWriteCnt;
  }
  /* still here so all data successfully programmed */
  return true;
} /*** end of XcpMasterProgramData ***/


/************************************************************************************//**
** \brief     Sends the XCP Connect command.
** \return    true is successfull, false otherwise.
**
****************************************************************************************/
static uint8_t XcpMasterSendCmdConnect(void)
{
  uint8_t packetData[2];
  tXcpTransportResponsePacket *responsePacketPtr;
  
  /* prepare the command packet */
  packetData[0] = XCP_MASTER_CMD_CONNECT;
  packetData[1] = 0; /* normal mode */
  
  /* send the packet */
  if (XcpTransportSendPacket(packetData, 2, XCP_MASTER_CONNECT_TIMEOUT_MS) == false)
  {
    /* cound not set packet or receive response within the specified timeout */
    return false;
  }
  /* still here so a response was received */
  responsePacketPtr = XcpTransportReadResponsePacket();
  
  /* check if the reponse was valid */
  if ( (responsePacketPtr->len == 0) || (responsePacketPtr->data[0] != XCP_MASTER_CMD_PID_RES) )
  {
    /* not a valid or positive response */
    return false;
  }
  
  /* process response data */
  if ((responsePacketPtr->data[2] & 0x01) == 0)
  {
    /* store slave's byte ordering information */
    xcpSlaveIsIntel = true;
  }
  /* store max number of bytes the slave allows for master->slave packets. */
  xcpMaxCto = responsePacketPtr->data[3];
  xcpMaxProgCto = xcpMaxCto;
  /* store max number of bytes the slave allows for slave->master packets. */
  if (xcpSlaveIsIntel == true)
  {
    xcpMaxDto = responsePacketPtr->data[4] + (responsePacketPtr->data[5] << 8);
  }
  else
  {
    xcpMaxDto = responsePacketPtr->data[5] + (responsePacketPtr->data[4] << 8);
  }
  
  /* double check size configuration of the master */
  assert(XCP_MASTER_TX_MAX_DATA >= xcpMaxCto);
  assert(XCP_MASTER_RX_MAX_DATA >= xcpMaxDto);
  
  /* still here so all went well */  
  return true;
} /*** end of XcpMasterSendCmdConnect ***/


/************************************************************************************//**
** \brief     Sends the XCP Set MTA command.
** \param     address New MTA address for the slave.
** \return    true is successfull, false otherwise.
**
****************************************************************************************/
static uint8_t XcpMasterSendCmdSetMta(uint32_t address)
{
  uint8_t packetData[8];
  tXcpTransportResponsePacket *responsePacketPtr;
  
  /* prepare the command packet */
  packetData[0] = XCP_MASTER_CMD_SET_MTA;
  packetData[1] = 0; /* reserved */
  packetData[2] = 0; /* reserved */
  packetData[3] = 0; /* address extension not supported */
  
  /* set the address taking into account byte ordering */
  XcpMasterSetOrderedLong(address, &packetData[4]);
  
  /* send the packet */
  if (XcpTransportSendPacket(packetData, 8, XCP_MASTER_TIMEOUT_T1_MS) == false)
  {
    /* cound not set packet or receive response within the specified timeout */
    return false;
  }
  /* still here so a response was received */
  responsePacketPtr = XcpTransportReadResponsePacket();
  
  /* check if the reponse was valid */
  if ( (responsePacketPtr->len == 0) || (responsePacketPtr->data[0] != XCP_MASTER_CMD_PID_RES) )
  {
    /* not a valid or positive response */
    return false;
  }
  
  /* still here so all went well */  
  return true;
} /*** end of XcpMasterSendCmdSetMta ***/


/************************************************************************************//**
** \brief     Sends the XCP UPLOAD command.
** \param     data Destination data buffer.
** \param     length Number of bytes to upload.
** \return    true is successfull, false otherwise.
**
****************************************************************************************/
static uint8_t XcpMasterSendCmdUpload(uint8_t data[], uint8_t length)
{
  uint8_t packetData[2];
  tXcpTransportResponsePacket *responsePacketPtr;
  uint8_t data_index;
  
  /* cannot request more data then the max rx data - 1 */
  assert(length < XCP_MASTER_RX_MAX_DATA);
  
  /* prepare the command packet */
  packetData[0] = XCP_MASTER_CMD_UPLOAD;
  packetData[1] = length;

  /* send the packet */
  if (XcpTransportSendPacket(packetData, 2, XCP_MASTER_TIMEOUT_T1_MS) == false)
  {
    /* cound not set packet or receive response within the specified timeout */
    return false;
  }
  /* still here so a response was received */
  responsePacketPtr = XcpTransportReadResponsePacket();
  
  /* check if the reponse was valid */
  if ( (responsePacketPtr->len == 0) || (responsePacketPtr->data[0] != XCP_MASTER_CMD_PID_RES) )
  {
    /* not a valid or positive response */
    return false;
  }
  
  /* now store the uploaded data */
  for (data_index=0; data_index<length; data_index++)
  {
    data[data_index] = responsePacketPtr->data[data_index+1];
  }
  
  /* still here so all went well */  
  return true;
} /*** end of XcpMasterSendCmdUpload ***/


/************************************************************************************//**
** \brief     Sends the XCP PROGRAM START command.
** \return    true is successfull, false otherwise.
**
****************************************************************************************/
static uint8_t XcpMasterSendCmdProgramStart(void)
{
  uint8_t packetData[1];
  tXcpTransportResponsePacket *responsePacketPtr;

  /* prepare the command packet */
  packetData[0] = XCP_MASTER_CMD_PROGRAM_START;

  /* send the packet */
  if (XcpTransportSendPacket(packetData, 1, XCP_MASTER_TIMEOUT_T3_MS) == false)
  {
    /* cound not set packet or receive response within the specified timeout */
    return false;
  }
  /* still here so a response was received */
  responsePacketPtr = XcpTransportReadResponsePacket();
  
  /* check if the reponse was valid */
  if ( (responsePacketPtr->len == 0) || (responsePacketPtr->data[0] != XCP_MASTER_CMD_PID_RES) )
  {
    /* not a valid or positive response */
    return false;
  }
  
  /* store max number of bytes the slave allows for master->slave packets during the
   * programming session
   */
  xcpMaxProgCto = responsePacketPtr->data[3];
  
  /* still here so all went well */  
  return true;
} /*** end of XcpMasterSendCmdProgramStart ***/


/************************************************************************************//**
** \brief     Sends the XCP PROGRAM RESET command. Note that this command is a bit 
**            different as in it does not require a response.
** \return    true is successfull, false otherwise.
**
****************************************************************************************/
static uint8_t XcpMasterSendCmdProgramReset(void)
{
  uint8_t packetData[1];
  tXcpTransportResponsePacket *responsePacketPtr;

  /* prepare the command packet */
  packetData[0] = XCP_MASTER_CMD_PROGRAM_RESET;

  /* send the packet, assume the sending itself is ok and check if a response was
   * received.
   */
  if (XcpTransportSendPacket(packetData, 1, XCP_MASTER_TIMEOUT_T5_MS) == false)
  {
    /* probably no response received within the specified timeout, but that is allowed
     * for the reset command.
     */
    return true;
  }
  /* still here so a response was received */
  responsePacketPtr = XcpTransportReadResponsePacket();
  
  /* check if the reponse was valid */
  if ( (responsePacketPtr->len == 0) || (responsePacketPtr->data[0] != XCP_MASTER_CMD_PID_RES) )
  {
    /* not a valid or positive response */
    return false;
  }
  
  /* still here so all went well */  
  return true;
} /*** end of XcpMasterSendCmdProgramReset ***/


/************************************************************************************//**
** \brief     Sends the XCP PROGRAM command.
** \param     length Number of bytes in the data array to program.
** \param     data Array with data bytes to program.
** \return    true is successfull, false otherwise.
**
****************************************************************************************/
static uint8_t XcpMasterSendCmdProgram(uint8_t length, uint8_t data[])
{
  uint8_t packetData[XCP_MASTER_TX_MAX_DATA];
  tXcpTransportResponsePacket *responsePacketPtr;
  uint8_t cnt;
  
  /* verify that this number of bytes actually first in this command */
  assert(length <= (xcpMaxProgCto-2) && (xcpMaxProgCto <= XCP_MASTER_TX_MAX_DATA));
  
  /* prepare the command packet */
  packetData[0] = XCP_MASTER_CMD_PROGRAM;
  packetData[1] = length;
  for (cnt=0; cnt<length; cnt++)
  {
    packetData[cnt+2] = data[cnt];
  }

  /* send the packet */
  if (XcpTransportSendPacket(packetData, length+2, XCP_MASTER_TIMEOUT_T5_MS) == false)
  {
    /* cound not set packet or receive response within the specified timeout */
    return false;
  }
  /* still here so a response was received */
  responsePacketPtr = XcpTransportReadResponsePacket();
  
  /* check if the reponse was valid */
  if ( (responsePacketPtr->len == 0) || (responsePacketPtr->data[0] != XCP_MASTER_CMD_PID_RES) )
  {
    /* not a valid or positive response */
    return false;
  }
  
  /* still here so all went well */  
  return true;
} /*** end of XcpMasterSendCmdProgram ***/


/************************************************************************************//**
** \brief     Sends the XCP PROGRAM MAX command.
** \param     data Array with data bytes to program.
** \return    true is successfull, false otherwise.
**
****************************************************************************************/
static uint8_t XcpMasterSendCmdProgramMax(uint8_t data[])
{
  uint8_t packetData[XCP_MASTER_TX_MAX_DATA];
  tXcpTransportResponsePacket *responsePacketPtr;
  uint8_t cnt;
  
  /* verify that this number of bytes actually first in this command */
  assert(xcpMaxProgCto <= XCP_MASTER_TX_MAX_DATA);
  
  /* prepare the command packet */
  packetData[0] = XCP_MASTER_CMD_PROGRAM_MAX;
  for (cnt=0; cnt<(xcpMaxProgCto-1); cnt++)
  {
    packetData[cnt+1] = data[cnt];
  }

  /* send the packet */
  if (XcpTransportSendPacket(packetData, xcpMaxProgCto, XCP_MASTER_TIMEOUT_T5_MS) == false)
  {
    /* cound not set packet or receive response within the specified timeout */
    return false;
  }
  /* still here so a response was received */
  responsePacketPtr = XcpTransportReadResponsePacket();
  
  /* check if the reponse was valid */
  if ( (responsePacketPtr->len == 0) || (responsePacketPtr->data[0] != XCP_MASTER_CMD_PID_RES) )
  {
    /* not a valid or positive response */
    return false;
  }
  
  /* still here so all went well */  
  return true;
} /*** end of XcpMasterSendCmdProgramMax ***/


/************************************************************************************//**
** \brief     Sends the XCP PROGRAM CLEAR command.
** \return    true is successfull, false otherwise.
**
****************************************************************************************/
static uint8_t XcpMasterSendCmdProgramClear(uint32_t length)
{
  uint8_t packetData[8];
  tXcpTransportResponsePacket *responsePacketPtr;

  /* prepare the command packet */
  packetData[0] = XCP_MASTER_CMD_PROGRAM_CLEAR;
  packetData[1] = 0; /* use absolute mode */
  packetData[2] = 0; /* reserved */
  packetData[3] = 0; /* reserved */

  /* set the erase length taking into account byte ordering */
  XcpMasterSetOrderedLong(length, &packetData[4]);


  /* send the packet */
  if (XcpTransportSendPacket(packetData, 8, XCP_MASTER_TIMEOUT_T4_MS) == false)
  {
    /* cound not set packet or receive response within the specified timeout */
    return false;
  }
  /* still here so a response was received */
  responsePacketPtr = XcpTransportReadResponsePacket();
  
  /* check if the reponse was valid */
  if ( (responsePacketPtr->len == 0) || (responsePacketPtr->data[0] != XCP_MASTER_CMD_PID_RES) )
  {
    /* not a valid or positive response */
    return false;
  }
  
  /* still here so all went well */  
  return true;
} /*** end of XcpMasterSendCmdProgramClear ***/


/************************************************************************************//**
** \brief     Stores a 32-bit value into a byte buffer taking into account Intel
**            or Motorola byte ordering.
** \param     value The 32-bit value to store in the buffer.
** \param     data Array to the buffer for storage.
** \return    none.
**
****************************************************************************************/
static void XcpMasterSetOrderedLong(uint32_t value, uint8_t data[])
{
  if (xcpSlaveIsIntel == true)
  {
    data[3] = (uint8_t)(value >> 24);
    data[2] = (uint8_t)(value >> 16);
    data[1] = (uint8_t)(value >>  8);
    data[0] = (uint8_t)value;
  }
  else
  {
    data[0] = (uint8_t)(value >> 24);
    data[1] = (uint8_t)(value >> 16);
    data[2] = (uint8_t)(value >>  8);
    data[3] = (uint8_t)value;
  }
} /*** end of XcpMasterSetOrderedLong ***/


/*********************************** end of xcpmaster.c ********************************/
