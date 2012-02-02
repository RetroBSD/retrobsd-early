#
# Copyright (c) 1988 Regents of the University of California.
# All rights reserved.
#
# Redistribution and use in source and binary forms are permitted
# provided that this notice is preserved and that due credit is given
# to the University of California at Berkeley. The name of the University
# may not be used to endorse or promote products derived from this
# software without specific prior written permission. This software
# is provided ``as is'' without express or implied warranty.
#
H		= ../../include
M		= ..
S		= ../../kernel

vpath %.c $(M):$(S)
vpath %.S $(M):$(S)

# Kernel options.
DEFS		+= -I. -I$(H) -DKERNEL -DUCB_METER -DPIC32MX7

# CPU frequency 80 MHz.
DEFS		+= -DCPU_KHZ=80000
DEFS		+= -DBUS_KHZ=80000

#
# Pinguino-micro board
# ====================
#
# Console on USB
# For Windows, use the driver: http://www.schmalzhaus.com/UBW32/FW/UBW32inf.zip
DEFS            += -DCONSOLE_USB
DEFS  		+= -DUSB_NUM_STRING_DESCRIPTORS=3 -DUSB_MAX_EP_NUMBER=3

# SD/MMC card driver on SPI2
# /CS0 at pin B13
DEFS            += -DSD_PORT=SPI2CON
DEFS            += -DSD_CS0_PORT=TRISB -DSD_CS0_PIN=13

# LED at pin D1
DEFS            += -DLED_KERNEL_PORT=TRISD -DLED_KERNEL_PIN=1 -DLED_KERNEL_INVERT

# Include or exclude drivers

# General Purpose I/O
DRIVER_GPIO         = yes

# Basic ADC interface
DRIVER_ADC          = no

# Power control (power LED, and soft power-off by button)
# requires supported PSU (ATX)
DRIVER_POWER        = no

POWER_LED_PORT      = TRISG
POWER_LED_PIN       = 12
POWER_SWITCH_PORT   = TRISG
POWER_SWITCH_PIN    = 0
POWER_CONTROL_PORT  = TRISE
POWER_CONTROL_PIN   = 9


DEPFLAGS	= -MT $@ -MD -MP -MF .deps/$*.dep
CFLAGS		= -O $(DEFS) $(DEPFLAGS)
ASFLAGS		= $(DEFS) $(DEPFLAGS)

include ../gcc-config.mk

CC		= $(GCCPREFIX)gcc -EL -g -mips32r2
CC		+= -nostdinc -fno-builtin -Werror -Wall -fno-dwarf2-cfi-asm
LDFLAGS         += -nostdlib -T using-bootloader.ld -Wl,-Map=unix.map
SIZE		= $(GCCPREFIX)size
OBJDUMP		= $(GCCPREFIX)objdump
OBJCOPY		= $(GCCPREFIX)objcopy

# Machine-dependent files:
# startup.o MUST be loaded first.
KERNOBJ		= startup.o clock.o devsw.o sysctl.o \
		  signal.o machdep.o mem.o exception.o \
                  usb_device.o usb_function_cdc.o
KERNOBJ		+= usb_console.o

# Kernel.
KERNOBJ		+= init_main.o init_sysent.o kern_clock.o \
		   kern_descrip.o kern_exec.o kern_exit.o kern_fork.o \
		   kern_mman.o kern_proc.o kern_prot.o \
		   kern_prot2.o kern_resource.o kern_sig.o kern_sig2.o \
		   kern_subr.o kern_synch.o kern_sysctl.o kern_time.o \
		   subr_log.o subr_prf.o subr_rmap.o \
		   sys_generic.o sys_inode.o syscalls.o \
		   sys_pipe.o sys_process.o tty.o tty_conf.o \
		   tty_subr.o tty_tty.o ufs_alloc.o ufs_bio.o \
		   ufs_bmap.o ufs_dsort.o ufs_fio.o \
		   ufs_inode.o ufs_mount.o ufs_namei.o ufs_subr.o \
		   ufs_syscalls.o ufs_syscalls2.o vfs_vnops.o \
		   vm_sched.o vm_swap.o vm_swp.o

# Drivers.
KERNOBJ		+= sd.o 

# Configuration-dependent files.
KERNOBJ		+= vers.o

# This makefile does the work including the right files and options for the drivers
include ../common/Makefile.internalDrivers

all:		.deps sys machine unix.elf
		$(SIZE) unix.elf

clean:
		rm -rf .deps *.o *.elf *.bin *.dis *.map *.srec core \
		mklog assym.h vers.c genassym sys machine
		find ../.. -name \*~ | xargs rm -f

.deps:
		mkdir .deps

sys:
		ln -s ../../include $@

machine:
		ln -s .. $@

unix.elf:	$(KERNOBJ) using-bootloader.ld
		$(CC) $(LDFLAGS) $(KERNOBJ) -o $@
		chmod -x $@
		$(OBJDUMP) -d -S $@ > unix.dis
		$(OBJCOPY) -O binary $@ unix.bin
		$(OBJCOPY) -O ihex --change-addresses=0x80000000 $@ unix.hex
		chmod -x $@ unix.bin

load:           unix.elf
		pic32prog -p unix.hex

vers.o:		../newvers.sh $(H)/*.h $(M)/*.[ch] $(S)/*.c
		sh ../newvers.sh > vers.c
		$(CC) -c vers.c

.SUFFIXES:	.i .srec .hex .dis .cpp .cxx .bin .elf

.o.dis:
		$(OBJDUMP) -d -z -S $< > $@

ifeq (.deps, $(wildcard .deps))
-include .deps/*.dep
endif
