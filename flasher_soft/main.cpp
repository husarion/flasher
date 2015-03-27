/************************************************************************************//**
* \file         main.c
* \brief        flasher_soft command line demonstration program for OpenBLT.
* \ingroup      flasher_soft
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
#include <sb_types.h>                                 /* C types                       */
#include <stdio.h>                                    /* standard I/O library          */
#include <string.h>                                   /* string library                */
#include "xcpmaster.h"                                /* XCP master protocol module    */
#include "timeutil.h"                                 /* time utility module           */

#include "ihex.h"

/****************************************************************************************
* Function prototypes
****************************************************************************************/
static void     DisplayProgramInfo(void);
static void     DisplayProgramUsage(void);
static sb_uint8 ParseCommandLine(int argc, const char *argv[]);


/****************************************************************************************
* Macro definitions
****************************************************************************************/
/** \brief Program return code if all went ok. */
#define PROG_RESULT_OK    (0)

/** \brief Program return code if an error occurred. */
#define PROG_RESULT_ERROR (1)


/****************************************************************************************
* Local data declarations
****************************************************************************************/
/** \brief Name of the serial device, such as COM4 or /dev/ttyUSB0. */
static char serialDeviceName[32];

/** \brief Serial communication speed in bits per second. */
static sb_uint32 serialBaudrate;

/** \brief Name of the S-record file. */
static char hexFileName[128];


/************************************************************************************//**
** \brief     Program entry point.
** \param     argc Number of program parameters.
** \param     argv array to program parameter strings.
** \return    0 on success, > 0 on error.
**
****************************************************************************************/
sb_int32 main(int argc, const char *argv[])
{
	IHexFile f;
	
	/* disable buffering for the standard output to make sure printf does not wait until
	 * a newline character is detected before outputting text on the console.
	 */
	setbuf(stdout, 0);
	
	/* inform user about the program */
	// DisplayProgramInfo();
	
	/* start out by making sure program was started with the correct parameters */
	if (ParseCommandLine(argc, argv) == SB_FALSE)
	{
		/* parameters invalid. inform user about how this program works */
		DisplayProgramUsage();
		return PROG_RESULT_ERROR;
	}
	
	f.load(hexFileName);

	serialBaudrate = 921600;
	/* -------------------- start the firmware update procedure ------------------------ */
	printf("Starting firmware update for \"%s\" using %s @ %u bits/s\n", hexFileName, serialDeviceName, serialBaudrate);
	
	/* -------------------- Open the serial port --------------------------------------- */
	printf("Opening serial port %s...", serialDeviceName);
	if (XcpMasterInit(serialDeviceName, serialBaudrate) == SB_FALSE)
	{
		printf("ERROR\n");
		// SrecordClose(hSrecord);
		return PROG_RESULT_ERROR;
	}
	printf("OK\n");
	
	/* -------------------- Connect to XCP slave --------------------------------------- */
	printf("Connecting to bootloader...");
	if (XcpMasterConnect() == SB_FALSE)
	{
		/* no response. prompt the user to reset the system */
		printf("TIMEOUT\nReset your microcontroller...");
	}
	/* now keep retrying until we get a response */
	while (XcpMasterConnect() == SB_FALSE)
	{
		/* delay a bit to not pump up the CPU load */
		TimeUtilDelayMs(20);
	}
	printf("OK\n");
	
	/* -------------------- Prepare the programming session ---------------------------- */
	printf("Initializing programming session...");
	if (XcpMasterStartProgrammingSession() == SB_FALSE)
	{
		printf("ERROR\n");
		XcpMasterDisconnect();
		XcpMasterDeinit();
		// SrecordClose(hSrecord);
		return PROG_RESULT_ERROR;
	}
	printf("OK\n");
	
	/* -------------------- Erase memory ----------------------------------------------- */
	// printf("Erasing %u bytes starting at 0x%08x...", fileParseResults.data_bytes_total, fileParseResults.address_low);
	for (unsigned int i = 0; i < f.parts.size(); i++)
	{
		TPart* part = f.parts[i];
		printf("erasing 0x%08x len: %d\r\n", part->getStartAddr(), part->getLen());
		if (XcpMasterClearMemory(part->getStartAddr(), part->getLen()) == SB_FALSE)
		{
			printf("ERROR\n");
			XcpMasterDisconnect();
			XcpMasterDeinit();
			// SrecordClose(hSrecord);
			return PROG_RESULT_ERROR;
		}
	}
	printf("OK\n");
	
	/* -------------------- Program data ----------------------------------------------- */
	printf("Programming data. Please wait...");
	/* loop through all S-records with program data */
	for (unsigned int i = 0; i < f.parts.size(); i++)
	{
		TPart* part = f.parts[i];
		uint32_t curAddr = part->getStartAddr();
		uint8_t* data = part->data.data();
		
		while (curAddr < part->getEndAddr())
		{
			int len = part->getEndAddr() - curAddr;
			if (len > 255) len = 255;
			
			printf("writing 0x%08x len: %d\r\n", curAddr, len);
			if (XcpMasterProgramData(curAddr, len, data) == SB_FALSE)
			{
				printf("ERROR\n");
				XcpMasterDisconnect();
				XcpMasterDeinit();
				return PROG_RESULT_ERROR;
			}
			
			data += len;
			curAddr += len;
		}
		
	}
	printf("OK\n");
	
	/* -------------------- Stop the programming session ------------------------------- */
	printf("Finishing programming session...");
	if (XcpMasterStopProgrammingSession() == SB_FALSE)
	{
		printf("ERROR\n");
		XcpMasterDisconnect();
		XcpMasterDeinit();
		return PROG_RESULT_ERROR;
	}
	printf("OK\n");
	
	/* -------------------- Disconnect from XCP slave and perform software reset ------- */
	printf("Performing software reset...");
	if (XcpMasterDisconnect() == SB_FALSE)
	{
		printf("ERROR\n");
		XcpMasterDeinit();
		return PROG_RESULT_ERROR;
	}
	printf("OK\n");
	
	/* -------------------- close the serial port -------------------------------------- */
	XcpMasterDeinit();
	printf("Closed serial port %s\n", serialDeviceName);
	
	/* -------------------- close the S-record file ------------------------------------ */
	printf("Closed S-record file \"%s\"\n", hexFileName);
	
	/* all done */
	printf("Firmware successfully updated!\n");
	return PROG_RESULT_OK;
} /*** end of main ***/


/************************************************************************************//**
** \brief     Outputs information to the user about this program.
** \return    none.
**
****************************************************************************************/
static void DisplayProgramInfo(void)
{
	printf("-------------------------------------------------------------------------\n");
	printf("flasher_soft version 1.00. Performs firmware updates via the serial port\n");
	printf("for a microcontroller based system that runs the OpenBLT bootloader.\n\n");
	printf("Copyright (c) by Feaser  http://www.feaser.com\n");
	printf("-------------------------------------------------------------------------\n");
} /*** end of DisplayProgramInfo ***/


/************************************************************************************//**
** \brief     Outputs information to the user about how to use this program.
** \return    none.
**
****************************************************************************************/
static void DisplayProgramUsage(void)
{
	printf("Usage:    flasher_soft -d[device] [hex file]\n\n");
#ifdef PLATFORM_WIN32
	printf("Example:  flasher_soft -dCOM4 myfirmware.hex\n");
	printf("          -> Connects to COM4\n");
#else
	printf("Example:  flasher_soft -d/dev/ttyUSB0 myfirmware.hex\n");
	printf("          -> Connects to ttyUSB0\n");
#endif
	printf("             bits/second and programs the myfirmware.hex file in non-\n");
	printf("             volatile memory of the microcontroller using OpenBLT.\n");
	// printf("-------------------------------------------------------------------------\n");
} /*** end of DisplayProgramUsage ***/


/************************************************************************************//**
** \brief     Parses the command line arguments. A fixed amount of arguments is expected.
**            The program should be called as:
**              flasher_soft -d[device] -b[baudrate] [s-record file]
** \param     argc Number of program parameters.
** \param     argv array to program parameter strings.
** \return    SB_TRUE on success, SB_FALSE otherwise.
**
****************************************************************************************/
static sb_uint8 ParseCommandLine(int argc, const char *argv[])
{
	sb_uint8 paramIdx;
	sb_uint8 paramDfound = SB_FALSE;
	sb_uint8 paramBfound = SB_FALSE;
	sb_uint8 srecordfound = SB_FALSE;
	
	/* make sure the right amount of arguments are given */
	if (argc != 3)
	{
		return SB_FALSE;
	}
	
	/* loop through all the command lina parameters, just skip the 1st one because this
	 * is the name of the program, which we are not interested in.
	 */
	for (paramIdx = 1; paramIdx < argc; paramIdx++)
	{
		/* is this the device name? */
		if ((argv[paramIdx][0] == '-') && (argv[paramIdx][1] == 'd') && (paramDfound == SB_FALSE))
		{
			/* copy the device name and set flag that this parameter was found */
			strcpy(serialDeviceName, &argv[paramIdx][2]);
			paramDfound = SB_TRUE;
		}

		/* still here so it must be the filename */
		else if (srecordfound == SB_FALSE)
		{
			/* copy the file name and set flag that this parameter was found */
			strcpy(hexFileName, &argv[paramIdx][0]);
			srecordfound = SB_TRUE;
		}
	}
	
	/* verify if all parameters were found */
	if ((paramDfound == SB_FALSE) || (srecordfound == SB_FALSE))
	{
		return SB_FALSE;
	}
	
	/* still here so the parsing was successful */
	return SB_TRUE;
} /*** end of ParseCommandLine ***/


/*********************************** end of main.c *************************************/
