/****************************************************************************
 File        : usb-osx.c
 Description : Encapsulates all nonportable, Mac-specific USB I/O code within
               the ubw32 program.  Each supported operating system has its
               own source file, providing a common calling syntax to the
               portable sections of the code.
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
#include <IOKit/hid/IOHIDDevicePlugIn.h>
#include "ubw32.h"

static IOHIDDeviceDeviceInterface **device = NULL;
unsigned char                       usbBuf[64];

/****************************************************************************
 Function    : usbCallback
 Description : Callback function as required of OS X asynchronous HID reports
               (synchronous mode didn't seem to work as expected).  All this
               does is stop the current Run Loop.  None of the callback
               parameters are actually used at present.
 ****************************************************************************/
static void usbCallback(
  void            *context,
  IOReturn         result,
  void            *sender,
  IOHIDReportType  type,
  uint32_t         reportID,
  uint8_t         *report,
  CFIndex          reportLength)
{
	DEBUGMSG("In callback");
	CFRunLoopStop(CFRunLoopGetCurrent());
	DEBUGMSG("Exiting callback");
}

/****************************************************************************
 Function    : usbOpen
 Description : Searches for and opens the first available UBW32 device.
 Parameters  : unsigned short         Vendor ID to search for.
               unsigned short         Product ID to search for.
 Returns     : Status code:
                 ERR_NONE             Success; device open and ready for I/O.
                 ERR_UBW32_NOT_FOUND  UBW32 not detected on any USB bus
                                      (might be connected but not in
                                       Bootloader mode).
                 ERR_USB_INIT1        Initialization error in code prior to
                                      device open call (device query).
                 ERR_USB_OPEN         Error opening device (probably in use
                                      by another program).
                 ERR_USB_INIT2        Initialization error in code after the
                                      device open call (async report setup).
 Notes       : If multiple UBW32 devices are connected, only the first device
               found (and not in use by another application) is returned.
               This code sets no particular preference or sequence in the
               search ordering; whatever IOServiceGetMatchingService decides.
 ****************************************************************************/
ErrorCode usbOpen(
  const unsigned short vendorID,
  const unsigned short productID)
{
  CFMutableDictionaryRef dict;
  ErrorCode              status = ERR_UBW32_NOT_FOUND;

  if((dict = IOServiceMatching(kIOHIDDeviceKey))) {

    io_service_t service;

    /* Set up matching dictionary for vendor & product */
    CFDictionarySetValue(dict,CFSTR(kIOHIDVendorIDKey),
      CFNumberCreate(kCFAllocatorDefault,kCFNumberShortType,&vendorID));
    CFDictionarySetValue(dict,CFSTR(kIOHIDProductIDKey),
      CFNumberCreate(kCFAllocatorDefault,kCFNumberShortType,&productID));

    /* Get service for first (or only) device in dict.  Note that dict is
       never explicitly released in this code; that already occurs within
       IOServiceGetMatchingService() */
    if((service = IOServiceGetMatchingService(kIOMasterPortDefault,dict))) {

      IOCFPlugInInterface **plugInInterface = NULL;
      SInt32                dum;

      status = ERR_USB_INIT1; /* UBW32 found, but not yet initialized */

      /* Get intermediate plug-in interface */
      (void)IOCreatePlugInInterfaceForService(service,
        kIOHIDDeviceTypeID,kIOCFPlugInInterfaceID,&plugInInterface,&dum);

      (void)IOObjectRelease(service);

      if(plugInInterface) {
        /* Get specific device interface */
        (*plugInInterface)->QueryInterface(plugInInterface,
          CFUUIDGetUUIDBytes(kIOHIDDeviceDeviceInterfaceID),(LPVOID)&device);

	(*plugInInterface)->Release(plugInInterface);

        if(device) {
          status = ERR_USB_OPEN; /* Init phase 1 OK, but not yet open */

          if((kIOReturnSuccess == (*device)->open(device,0))) {

            CFTypeRef eventSource;

            status = ERR_USB_INIT2; /* Open OK, phase 2 init awaits */

            /* Set up asynchronous input report shenanigans */
            if((kIOReturnSuccess ==
                (*device)->getAsyncEventSource(device,&eventSource)) &&
               (kIOReturnSuccess == (*device)->setInputReportCallback(device,
                (unsigned char *)usbBuf,sizeof(usbBuf),usbCallback,NULL,0))) {

              CFRunLoopAddSource(CFRunLoopGetCurrent(),
                (CFRunLoopSourceRef)eventSource,kCFRunLoopDefaultMode);

              return ERR_NONE;

            } /* else cleanup and return error code */
            (*device)->close(device,0);
          }
          (*device)->Release(device);
          device = NULL;
        }
      }
    }
  }

  return status;
}

/****************************************************************************
 Function    : usbWrite
 Description : Write data packet to currently-open UBW32 device, optionally
               followed by a packet read operation.  Data source is always
               global array usbBuf[].  For read operation, destination is
               always usbBuf[] also, overwriting contents there.
 Parameters  : char       Size of source data in bytes (max 64).
               char       If set, read response packet.
 Returns     : ErrorCode  ERR_NONE on success, ERR_USB_WRITE on error.
 Notes       : Device is assumed to have already been successfully opened
               by the time this function is called; no checks performed here.
 ****************************************************************************/
ErrorCode usbWrite(
  const char len,
  const char read)
{
#ifdef DEBUG
	int i;
	(void)puts("Sending:");
	for(i=0;i<8;i++) (void)printf("%02x ",((unsigned char *)usbBuf)[i]);
	(void)printf(": ");
	for(;i<64;i++) (void)printf("%02x ",((unsigned char *)usbBuf)[i]);
	(void)putchar('\n'); fflush(stdout);
	DEBUGMSG("\nAbout to write");
#endif

	if(kIOReturnSuccess != (*device)->setReport(device,
	  kIOHIDReportTypeOutput,0,usbBuf,len,500,NULL,NULL,0))
		return ERR_USB_WRITE;

	DEBUGMSG("Done w/write");

	if(read) {
		DEBUGMSG("About to read");
		CFRunLoopRun(); /* Read invokes callback when done */
#ifdef DEBUG
		(void)puts("Done reading\nReceived:");
		for(i=0;i<8;i++) (void)printf("%02x ",usbBuf[i]);
		(void)printf(": ");
		for(;i<64;i++) (void)printf("%02x ",usbBuf[i]);
		(void)putchar('\n'); fflush(stdout);
#endif
	}

	return ERR_NONE;
}

/****************************************************************************
 Function    : usbClose
 Description : Closes previously-opened USB device.
 Parameters  : None (void)
 Returns     : Nothing (void)
 Notes       : Device is assumed to have already been successfully opened
               by the time this function is called; no checks performed here.
 ****************************************************************************/
void usbClose(void)
{
	(*device)->close(device,0);
	(*device)->Release(device);
	device = NULL;
}
