/********************************************************************
 FileName:     	HardwareProfile.h
 Dependencies:	See INCLUDES section
 Processor:		PIC18 or PIC24 USB Microcontrollers
 Hardware:		The code is natively intended to be used on the following
 				hardware platforms: PICDEM™ FS USB Demo Board, 
 				PIC18F87J50 FS USB Plug-In Module, or
 				Explorer 16 + PIC24 USB PIM.  The firmware may be
 				modified for use on other USB platforms by editing this
 				file (HardwareProfile.h).
 Complier:  	Microchip C18 (for PIC18) or C30 (for PIC24)
 Company:		Microchip Technology, Inc.

 Software License Agreement:

 The software supplied herewith by Microchip Technology Incorporated
 (the “Company”) for its PIC® Microcontroller is intended and
 supplied to you, the Company’s customer, for use solely and
 exclusively on Microchip PIC Microcontroller products. The
 software is owned by the Company and/or its supplier, and is
 protected under applicable copyright laws. All rights are reserved.
 Any use in violation of the foregoing restrictions may subject the
 user to criminal sanctions under applicable laws, as well as to
 civil liability for the breach of the terms and conditions of this
 license.

 THIS SOFTWARE IS PROVIDED IN AN “AS IS” CONDITION. NO WARRANTIES,
 WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED
 TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE COMPANY SHALL NOT,
 IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
 CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.

********************************************************************
 File Description:

 Change History:
  Rev   Date         Description
  1.0   11/19/2004   Initial release
  2.1   02/26/2007   Updated for simplicity and to use common
                     coding style

********************************************************************/

#ifndef HARDWARE_PROFILE_H
#define HARDWARE_PROFILE_H

#if !defined(DEMO_BOARD)
    #if defined(__C32__)
        #if defined(__32MX440F512H__) || defined(__32MX460F512L__) \
       || defined(__32MX695F512L__) || defined(__32MX795F512L__) \
       || defined(__32MX695F512H__) || defined(__32MX795F512H__)
			#ifdef MAXIMITE
                #include "HardwareProfile - MAXIMITE.h"
			#else
            #if defined (UBW32_MAXXWAVE)
				#include "HardwareProfile - UBW32_MaxxWave.h"
			#else
				#include "HardwareProfile - UBW32.h"
			#endif
            #endif
        #endif
    #endif

    #if defined(__C30__)
        #if defined(__PIC24FJ256GB110__)
            #define DEMO_BOARD PIC24FJ256GB110_PIM
            #define EXPLORER_16
			#define PIC24FJ256GB110_PIM
            #define CLOCK_FREQ 32000000
			#include "HardwareProfile - PIC24FJ256GB110 PIM.h"
        #elif defined(__PIC24FJ256GB106__)
            #define DEMO_BOARD PIC24F_STARTER_KIT
            #define PIC24F_STARTER_KIT
            #define CLOCK_FREQ 32000000
			#include "HardwareProfile - PIC24F Starter Kit.h"
        #endif
    #endif

    #if defined(__18CXX)
        #if defined(__18F4550)
            #define DEMO_BOARD PICDEM_FS_USB
            #define PICDEM_FS_USB
            #define CLOCK_FREQ 48000000
			#include "HardwareProfile - PICDEM FSUSB.h"
        #elif defined(__18F87J50)
            #define DEMO_BOARD PIC18F87J50_PIM
            #define PIC18F87J50_PIM
            #define CLOCK_FREQ 48000000
			#include "HardwareProfile - PIC18F87J50 PIM.h"
        #elif defined(__18F14K50)
            #define DEMO_BOARD LOW_PIN_COUNT_USB_DEVELOPMENT_KIT
            #define LOW_PIN_COUNT_USB_DEVELOPMENT_KIT
            #define CLOCK_FREQ 48000000
			#include "HardwareProfile - Low Pin Count USB Development Kit.h"
        #endif
    #endif
#endif

#if !defined DEMO_BOARD
    #error "Demo board not defined.  Either define DEMO_BOARD for a custom board or select the correct processor for the demo board."
#endif

#endif  //HARDWARE_PROFILE_H
