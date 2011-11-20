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

CC = $(GCCPREFIX)gcc
OBJCOPY = $(GCCPREFIX)objcopy
RM = rm -f
CFLAGS = -g -O1 -I.
LDFLAGS = -Wl,--defsym=__MPLAB_BUILD=1,-Map=Output/HIDBoot.map
OBJS = Output/usb_descriptors.o Output/main.o Output/usb_device.o Output/usb_function_hid.o

# UBW32
#CC += -mprocessor=32MX795F512L

# Maximite
#CC += -mprocessor=32MX695F512H
#CFLAGS += -DMAXIMITE

# eflightworks DIP
CC += -mprocessor=32MX795F512H
CFLAGS += -DEFLIGHTWORKS_DIP

Output/HIDBoot.hex: Output/HIDBoot.elf
	$(OBJCOPY) -O ihex --change-addresses=0x80000000 $< $@

Output/HIDBoot.elf: $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -oOutput/HIDBoot.elf

Output/usb_descriptors.o: usb_descriptors.c USB/usb.h USB/GenericTypeDefs.h USB/Compiler.h usb_config.h USB/usb_common.h USB/usb_ch9.h USB/usb_device.h USB/usb_hal.h USB/usb_hal_pic32.h USB/usb_function_hid.h
	$(CC) $(CFLAGS) -c usb_descriptors.c -oOutput/usb_descriptors.o

Output/main.o: main.c USB/usb.h USB/GenericTypeDefs.h USB/Compiler.h usb_config.h USB/usb_common.h USB/usb_ch9.h USB/usb_device.h USB/usb_hal.h USB/usb_hal_pic32.h HardwareProfile.h HardwareProfile\ -\ MAXIMITE.h USB/usb_function_hid.h
	$(CC) $(CFLAGS) -c main.c -oOutput/main.o

Output/usb_device.o: USB/usb_device.c USB/usb.h USB/GenericTypeDefs.h USB/Compiler.h usb_config.h USB/usb_common.h USB/usb_ch9.h USB/usb_device.h USB/usb_hal.h USB/usb_hal_pic32.h USB/Compiler.h HardwareProfile.h HardwareProfile\ -\ MAXIMITE.h USB/usb_device_local.h
	$(CC) $(CFLAGS) -c USB/usb_device.c -oOutput/usb_device.o

Output/usb_function_hid.o: USB/usb_function_hid.c USB/GenericTypeDefs.h USB/Compiler.h USB/usb.h USB/GenericTypeDefs.h USB/Compiler.h usb_config.h USB/usb_common.h USB/usb_ch9.h USB/usb_device.h USB/usb_hal.h USB/usb_hal_pic32.h USB/Compiler.h USB/usb_function_hid.h
	$(CC) $(CFLAGS) -c USB/usb_function_hid.c -oOutput/usb_function_hid.o

clean:
	$(RM) Output/*.o Output/HIDBoot.elf Output/HIDBoot.hex Output/HIDBoot.map
