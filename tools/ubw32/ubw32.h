/****************************************************************************
 File        : ubw32.h
 Description : Common header file for all ubw32 sources.
 History     : 2/19/2009  Initial implementation.
               4/16/2009  Bug fix for non-Intel and 64-bit platforms.
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

#ifndef _UBW32_H_
#define _UBW32_H_

#ifdef DEBUG
#define DEBUGMSG(str) (void)puts(str); fflush(stdout);
#else
#define DEBUGMSG(str)
#endif /* DEBUG */

/* On Intel architectures, can make some crass endianism optimizations */

#if defined(i386) || defined(__x86_64__)
#define bufWrite32(src,pos,val) *(unsigned int *)&src[pos] = val
#define bufRead32(pos)          *(unsigned int *)&usbBuf[pos]
#else
#define bufWrite32(src,pos,val) src[pos    ] =  val        & 0xff; \
                                src[pos + 1] = (val >>  8) & 0xff; \
                                src[pos + 2] = (val >> 16) & 0xff; \
                                src[pos + 3] = (val >> 24)
#define bufRead32(pos)         ( usbBuf[pos    ]        | \
                                (usbBuf[pos + 1] <<  8) | \
                                (usbBuf[pos + 2] << 16) | \
                                (usbBuf[pos + 3] << 24) )
#endif /* i386 || __x86_64__ */

/* Values derived from Microchip HID Bootloader source */

/* Bootloader commands */
#define	QUERY_DEVICE      0x02
#define	UNLOCK_CONFIG     0x03
#define ERASE_DEVICE      0x04
#define PROGRAM_DEVICE    0x05
#define	PROGRAM_COMPLETE  0x06
#define GET_DATA          0x07
#define	RESET_DEVICE      0x08

/* Sub-commands for the ERASE_DEVICE command */
#define UNLOCKCONFIG      0x00
#define LOCKCONFIG        0x01

/* Response types for QUERY_DEVICE command */
#define	TypeProgramMemory 0x01
#define TypeEEPROM        0x02
#define TypeConfigWords   0x03
#define	TypeEndOfTypeList 0xFF

/* Error codes returned by various functions */

typedef enum
{
	ERR_NONE = 0,        /* Success (non-error) */
	ERR_CMD_ARG,
	ERR_CMD_UNKNOWN,
	ERR_UBW32_NOT_FOUND,
	ERR_USB_INIT1,
	ERR_USB_INIT2,
	ERR_USB_OPEN,
	ERR_USB_WRITE,
	ERR_USB_READ,
	ERR_HEX_OPEN,
	ERR_HEX_STAT,
	ERR_HEX_MMAP,
	ERR_HEX_SYNTAX,
	ERR_HEX_CHECKSUM,
	ERR_HEX_RECORD,
	ERR_VERIFY,
	ERR_EOL              /* End-of-list, not actual error code */
} ErrorCode;

/* Function prototypes */

extern ErrorCode
	hexOpen(char *),
	hexWrite(char),
	usbOpen(unsigned short,unsigned short),
	usbWrite(char,char);
extern void
	hexClose(void),
	usbClose(void);

#endif /* _UBW32_H_ */
