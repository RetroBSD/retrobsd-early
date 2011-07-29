/****************************************************************************
 File        : usb-linux.c
 Description : Encapsulates all nonportable, Linux-specific USB I/O code
               within the ubw32 program.  Each supported operating system has
               its own source file, providing a common calling syntax to the
               portable sections of the code.
 History     : 3/16/2009  Initial Linux support for ubw32 program.
	       12/15/1010 Get rid of libhid; using libusb directly.
 License     : Copyright 2009 Phillip Burgess - pburgess@dslextreme.com
               Portions Copyright 2010 Serge Vakulenko - serge@vak.ru

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
#include <usb.h>
#include "ubw32.h"
#include <errno.h>

static usb_dev_handle *usbdev;
unsigned char        usbBuf[64];

/****************************************************************************
 Function    : usbOpen
 Description : Searches for and opens the first available UBW32 device.
 Parameters  : unsigned short         Vendor ID to search for.
               unsigned short         Product ID to search for.
 Returns     : Status code:
                 ERR_NONE             Success; device open and ready for I/O.
                 ERR_USB_INIT1        Initialization error in HID init code.
                 ERR_USB_INIT2        New HID alloc failed.
                 ERR_UBW32_NOT_FOUND  UBW32 not detected on any USB bus
                                      (might be connected but not in
                                       Bootloader mode).
 Notes       : If multiple UBW32 devices are connected, only the first device
               found (and not in use by another application) is returned.
               This code sets no particular preference or sequence in the
               search ordering; whatever the default libhid 'matching
               function' decides.
 ****************************************************************************/
ErrorCode usbOpen(
  const unsigned short vendorID,
  const unsigned short productID)
{
	struct usb_bus *bus;
	struct usb_device *dev;

	usb_init();
	usb_find_busses();
	usb_find_devices();
	for (bus = usb_get_busses(); bus; bus = bus->next) {
		for (dev = bus->devices; dev; dev = dev->next) {
			if (dev->descriptor.idVendor == vendorID &&
			    dev->descriptor.idProduct == productID) {
				usbdev = usb_open (dev);
				if (! usbdev) {
					return ERR_USB_INIT2;
				}
				if (usb_claim_interface (usbdev, 0) != 0) {
					if (usb_detach_kernel_driver_np (usbdev, 0) < 0) {
						return ERR_USB_INIT2;
					}
					if (usb_claim_interface (usbdev, 0) != 0) {
						return ERR_USB_INIT2;
					}
				}
				return ERR_NONE;
			}
		}
	}
	return ERR_UBW32_NOT_FOUND;
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

	int bytes_written = usb_interrupt_write (usbdev, 0x01, usbBuf, len, 0);
	if (bytes_written != len) {
		return ERR_USB_WRITE;
	}
	DEBUGMSG("Done w/write");

	if(read) {
		DEBUGMSG("About to read");

		int len = usb_interrupt_read (usbdev, 0x81, usbBuf, 64, 0);
		if (len != 64) {
			return ERR_USB_READ;
		}
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
	usb_release_interface (usbdev, 0);
	usb_close (usbdev);
	usbdev = 0;
}
