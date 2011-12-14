include ../gcc-config.mk

CC              = $(GCCPREFIX)gcc -g -EL -mips32r2 -fno-dwarf2-cfi-asm
OBJCOPY         = $(GCCPREFIX)objcopy
OBJDUMP         = $(GCCPREFIX)objdump
SIZE            = $(GCCPREFIX)size
RM              = rm -f
CFLAGS          = -g -Wall -Os -I../../../include \
                  -DCPU_KHZ=80000 \
                  -DUSB_NUM_STRING_DESCRIPTORS=3 -DUSB_MAX_EP_NUMBER=1 \
                  -DUSB_EP0_BUFF_SIZE=8 \
                  -DHID_INPUT_REPORT_BYTES=2 \
                  -DHID_OUTPUT_REPORT_BYTES=2 \
                  -DHID_FEATURE_REPORT_BYTES=2
                  -DHID_INT_OUT_EP_SIZE=64 \
                  -DHID_INT_IN_EP_SIZE=64
                  -DHID_RPT01_SIZE=29

LDFLAGS         = -nostdlib -Tboot.ld -Wl,-Map=usbboot.map
OBJS            = usb_boot.o usb_device.o usb_function_hid.o
I               = ..

# UBW32
#CFLAGS          += -DUBW32 -DFLASH_USER=0x1d005000

# Maximite
#CFLAGS         += -DMAXIMITE

# eflightworks DIP
CFLAGS         += -DDIP

all:            usbboot.hex usbboot.elf
		$(SIZE) usbboot.elf

usbboot.hex:    usbboot.elf
		$(OBJCOPY) -O ihex --change-addresses=0x80000000 $< $@

usbboot.elf:    $(OBJS)
		$(CC) $(LDFLAGS) -o $@ $(OBJS)
		$(OBJDUMP) -S $@ > usbboot.dis

usb_boot.o:     $(I)/usb_boot.c $(I)/usb_ch9.h $(I)/usb_device.h $(I)/usb_hal_pic32.h $(I)/usb_function_hid.h
		$(CC) $(CFLAGS) -o $@ -c $(I)/usb_boot.c

usb_device.o:   $(I)/usb_device.c $(I)/usb_device.h $(I)/usb_ch9.h $(I)/usb_hal_pic32.h
		$(CC) $(CFLAGS) -o $@ -c $(I)/usb_device.c

usb_function_hid.o: $(I)/usb_function_hid.c $(I)/usb_device.h $(I)/usb_ch9.h $(I)/usb_hal_pic32.h $(I)/usb_function_hid.h
		$(CC) $(CFLAGS) -o $@ -c $(I)/usb_function_hid.c

startup.o:      startup.S
		$(CC) $(CFLAGS) -o $@ -c $<

clean:
		$(RM) *.o usbboot.elf usbboot.hex usbboot.map usbboot.dis

load:           usbboot.hex
		pic32prog $<
