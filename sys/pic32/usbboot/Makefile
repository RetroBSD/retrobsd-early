# MPLABX C32 on Linux
ifneq (,$(wildcard /opt/microchip/mplabc32/v2.00/bin/pic32-gcc))
    GCCPREFIX	= /opt/microchip/mplabc32/v2.00/bin/pic32-
    INCLUDES    = -I/opt/microchip/mplabc32/v2.00/lib/gcc/pic32mx/4.5.1/include
endif

# MPLABX C32 on Mac OS X
ifneq (,$(wildcard /Applications/microchip/mplabc32/v2.00/bin/pic32-gcc))
    GCCPREFIX   = /Applications/microchip/mplabc32/v2.00/bin/pic32-
    INCLUDES    = -I/Applications/microchip/mplabc32/v2.00/lib/gcc/pic32mx/4.5.1/include
endif

# MPLABX C32 on Windows
ifneq (,$(wildcard /c/"Program Files"/Microchip/mplabc32/v2.00/bin/pic32-gcc))
    GCCPREFIX   = /c/"Program Files"/Microchip/mplabc32/v2.00/bin/pic32-
    INCLUDES    = -I/c/"Program Files"/microchip/mplabc32/v2.00/lib/gcc/pic32mx/4.5.1/include
endif

CC              = $(GCCPREFIX)gcc
OBJCOPY         = $(GCCPREFIX)objcopy
SIZE            = $(GCCPREFIX)size
RM              = rm -f
CFLAGS          = -g -Wall -O1 -I../../../include \
                  -DUSB_NUM_STRING_DESCRIPTORS=3 -DUSB_MAX_EP_NUMBER=1 \
                  -DUSB_EP0_BUFF_SIZE=8 \
                  -DHID_INPUT_REPORT_BYTES=2 \
                  -DHID_OUTPUT_REPORT_BYTES=2 \
                  -DHID_FEATURE_REPORT_BYTES=2
# HID
#define HID_INTF_ID             0x00
#define HID_EP 			1
#define HID_INT_OUT_EP_SIZE     64
#define HID_INT_IN_EP_SIZE      64
#define HID_NUM_OF_DSC          1
#define HID_RPT01_SIZE          29

LDFLAGS         = -Wl,--defsym=__MPLAB_BUILD=1,-Map=usbboot.map
OBJS            = usb_device.o usb_function_hid.o usb_boot.o
I               = ..

# UBW32
CC              += -mprocessor=32MX795F512L
CFLAGS          += -DUBW32

# Maximite
#CC             += -mprocessor=32MX695F512H
#CFLAGS         += -DMAXIMITE

# eflightworks DIP
#CC             += -mprocessor=32MX795F512H
#CFLAGS         += -DDIP

all:            usbboot.hex usbboot.elf
		$(SIZE) usbboot.elf

usbboot.hex:    usbboot.elf
		$(OBJCOPY) -O ihex --change-addresses=0x80000000 $< $@

usbboot.elf:    $(OBJS)
		$(CC) $(LDFLAGS) -o $@ $(OBJS)

usb_boot.o:     $(I)/usb_boot.c $(I)/usb_ch9.h $(I)/usb_device.h $(I)/usb_hal_pic32.h $(I)/usb_function_hid.h
		$(CC) $(CFLAGS) -o $@ -c $(I)/usb_boot.c

usb_device.o:   $(I)/usb_device.c $(I)/usb_device.h $(I)/usb_ch9.h $(I)/usb_hal_pic32.h
		$(CC) $(CFLAGS) -o $@ -c $(I)/usb_device.c

usb_function_hid.o: $(I)/usb_function_hid.c $(I)/usb_device.h $(I)/usb_ch9.h $(I)/usb_hal_pic32.h $(I)/usb_function_hid.h
		$(CC) $(CFLAGS) -o $@ -c $(I)/usb_function_hid.c

clean:
		$(RM) *.o usbboot.elf usbboot.hex usbboot.map
