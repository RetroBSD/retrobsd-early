/****************************************************************************
 File        : main.c
 Description : Main source file for 'ubw32,' a simple command-line tool for
               communicating with the UBW32 bootloader and downloading new
               firmware.  The UBW32 (32 bit USB Bit Whacker) info page is at:

               http://www.schmalzhaus.com/UBW32/

               Though not yet tested for such, the program will likely work
               with any device based on the Microchip HID Bootloader; the
               UBW32 was simply the first such device I'd been exposed to,
               so that name was adopted here (please see notes below).
 History     : 2/19/2009  Initial implementation.
 License     : Copyright 2009 Phillip Burgess - pburgess@dslextreme.com

               This file is part of 'ubw32' program.

               'ubw32' is free software: you can redistribute it and/or
               modify it under the terms of the GNU General Public License
               as published by the Free Software Foundation, either version
               3 of the License, or (at your option) any later version.

               'ubw32' is distributed in the hope that it will be useful,
               but WITHOUT ANY WARRANTY; without even the implied warranty
               of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
               See the GNU General Public License for more details.

               You should have received a copy of the GNU General Public
               License along with 'ubw32' source code.  If not,
               see <http://www.gnu.org/licenses/>.

               This license applies specifically to the 'ubw32' software,
               which does not originate from nor is directly affiliated with
               UBW32 hardware developer Brian Schmalz, manufacturer/
               distributor SparkFun Electronics, nor component supplier
               Microchip Technology.  The UBW32 hardware, design files and
               documents, bootloader and firmware code are the property of
               their respective rights holders and may or may not be
               distributed under different terms than this software.
 ****************************************************************************/

#include <stdio.h>
#include <string.h>
#include "ubw32.h"

extern unsigned char usbBuf[];  /* In usb code */

/* Program's actions aren't necessarily performed in command-line order.
   Bit flags keep track of options set or cleared during input parsing,
   then are singularly checked as actions are performed.  Some actions
   (such as writing) don't have corresponding bits here; certain non-NULL
   string values indicate such actions should occur. */
#define ACTION_UNLOCK (1 << 0)
#define ACTION_ERASE  (1 << 1)
#define ACTION_VERIFY (1 << 2)
#define ACTION_RESET  (1 << 3)

/****************************************************************************
 Function    : main
 Description : ubw32 program startup; parse command-line input and issue
               commands as needed; return program status.
 Returns     : int  0 on success, else various numeric error codes.
 ****************************************************************************/
int main(
  int   argc,
  char *argv[])
{
	char        *hexFile   = NULL,
	             actions   = ACTION_VERIFY,
	             eol;        /* 1 = last command-line arg */
	ErrorCode    status    = ERR_NONE;
	int          i;
	unsigned int vendorID  = 0x04d8,
	             productID = 0x003c;

	const char * const errorString[ERR_EOL] = {
		"Missing or malformed command-line argument",
		"Command not recognized",
		"UBW32 not found (is device attached and in Bootloader mode?)",
		"USB initialization failed (phase 1)",
		"USB initialization failed (phase 2)",
		"UBW32 could not be opened for I/O",
		"USB write error",
		"USB read error",
		"Could not open hex file for input",
		"Could not query hex file size",
		"Could not map hex file to memory",
		"Unrecognized or invalid hex file syntax",
		"Bad end-of-line checksum in hex file",
		"Unsupported record type in hex file",
		"Verify failed"
	};

	/* To create a sensible sequence of operations, all command-line
	   input is processed prior to taking any actions.  The sequence
	   of actions performed may not always directly correspond to the
	   order or quantity of input; commands follow precedence, not
	   input order.  For example, the action corresponding to the "-u"
	   (unlock) command must take place early on, before any erase or
	   write operations, even if specified late on the command line;
	   conversely, "-r" (reset) should always be performed last
	   regardless of input order.  In the case of duplicitous
	   commands (e.g. if multiple "-w" (write) commands are present),
	   only the last one will take effect.

	   The precedence of commands (first to last) is:

	   -v and -p <hex>  USB vendor and/or product IDs
	   -u               Unlock configuration memory
	   -e               Erase program memory
	   -n               No verify after write
	   -w <file>        Write program memory
	   -r               Reset */

	for(i=1;(i < argc) && (ERR_NONE == status);i++) {
		eol = (i >= (argc - 1));
		if(!strncasecmp(argv[i],"-v",2)) {
			if(eol || (1 != sscanf(argv[++i],"%x",&vendorID)))
				status = ERR_CMD_ARG;
		} else if(!strncasecmp(argv[i],"-p",2)) {
			if(eol || (1 != sscanf(argv[++i],"%x",&productID)))
				status = ERR_CMD_ARG;
		} else if(!strncasecmp(argv[i],"-u",2)) {
			actions |= ACTION_UNLOCK;
		} else if(!strncasecmp(argv[i],"-e",2)) {
			actions |= ACTION_ERASE;
		} else if(!strncasecmp(argv[i],"-n",2)) {
			actions &= ~ACTION_VERIFY;
		} else if(!strncasecmp(argv[i],"-w",2)) {
			if(eol) {
				status   = ERR_CMD_ARG;
			} else {
				hexFile  = argv[++i];
				actions |= ACTION_ERASE;
			}
		} else if(!strncasecmp(argv[i],"-r",2)) {
			actions |= ACTION_RESET;
		} else if(!strncasecmp(argv[i],"-h",2) ||
		          !strncasecmp(argv[i],"-?",2)) {
			(void)puts(
"ubw32 : a Microchip HID Bootloader utility\n"
"Option     Description                                      Default\n"
"-------------------------------------------------------------------------\n"
"-w <file>  Write hex file to UBW32 (will erase first)       None\n"
"-e         Erase UBW32 code space (implicit if -w)          No erase\n"
"-r         Reset device on program exit                     No reset\n"
"-n         No verify after write                            Verify on\n"
"-u         Unlock configuration memory before erase/write   Config locked\n"
"-v <hex>   USB device vendor ID                             04d8\n"
"-p <hex>   USB device product ID                            003c\n"
"-h or -?   Help");
			return 0;
		} else {
			status = ERR_CMD_UNKNOWN;
		}
	}

	/* After successful command-line parsage, find/open USB device. */

	if((ERR_NONE == status) &&
	   (ERR_NONE == (status = usbOpen(vendorID,productID)))) {

		/* And start doing stuff... */

		(void)printf("UBW32 found");
		usbBuf[0] = QUERY_DEVICE;
		if(ERR_NONE == (status = usbWrite(1,1))) {
			int j;
			for(i=1,j=3;usbBuf[j]!=TypeEndOfTypeList;j+=9,i++) {
			  if(usbBuf[j] == TypeProgramMemory)
			    (void)printf(": %d bytes free",bufRead32(j + 5));
			}
		}
		(void)putchar('\n');

		if((ERR_NONE == status) && (actions & ACTION_UNLOCK)) {
			(void)puts("Unlocking configuration memory...");
			usbBuf[0] = UNLOCK_CONFIG;
			usbBuf[1] = UNLOCKCONFIG;
			status    = usbWrite(2,0);
		}

		/* Although the next actual operation is ACTION_ERASE,
		   if we anticipate hex-writing in a subsequent step,
                   attempt opening file now so we can display any error
		   message quickly rather than waiting through the whole
		   erase operation (it's usually a simple filename typo). */
		if((ERR_NONE == status) && hexFile &&
		   (ERR_NONE != (status = hexOpen(hexFile))))
			hexFile = NULL;  /* Open or mmap error */

		if((ERR_NONE == status) && (actions & ACTION_ERASE)) {
			(void)puts("Erasing...");
			usbBuf[0] = ERASE_DEVICE;
			status    = usbWrite(1,0);
			/* The query here isn't needed for any technical
			   reason, just makes the presentation better.
			   The ERASE_DEVICE command above returns
			   immediately...subsequent commands can be made
			   but will pause until the erase cycle completes.
			   So this query just keeps the "Writing" message
			   or others from being displayed prematurely. */
			usbBuf[0] = QUERY_DEVICE;
			status    = usbWrite(1,1);
		}

		if(hexFile) {
			if(ERR_NONE == status) {
			  (void)printf("Writing hex file '%s':",hexFile);
			  status = hexWrite((actions & ACTION_VERIFY) != 0);
			  (void)putchar('\n');
			}
			hexClose();
		}

		if((ERR_NONE == status) && (actions & ACTION_RESET)) {
			(void)puts("Resetting device...");
			usbBuf[0] = RESET_DEVICE;
			status = usbWrite(1,0);
		}

		usbClose();
	}

	if(ERR_NONE != status) {
		(void)printf("%s Error",argv[0]);
		if(status <= ERR_EOL)
			(void)printf(": %s\n",errorString[status - 1]);
		else
			(void)puts(" of indeterminate type.");
	}

	return (int)status;
}
