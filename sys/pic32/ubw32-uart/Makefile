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
# UBW32 board
# ===========
#
# Console on UART1
DEFS            += -DCONSOLE_UART1

# SD/MMC card driver on SPI1
# /CS0 at pin A9 (and optional /CS1 at pin A10)
DEFS            += -DSD_PORT=SPI1CON -DSD_MHZ=8
DEFS            += -DSD_CS0_PORT=TRISA -DSD_CS0_PIN=9
DEFS            += -DSD_CS1_PORT=TRISA -DSD_CS1_PIN=10

# LEDs at pins E0, E1, E2, E3
DEFS            += -DLED_AUX_PORT=TRISE    -DLED_AUX_PIN=0    -DLED_AUX_INVERT
DEFS            += -DLED_DISK_PORT=TRISE   -DLED_DISK_PIN=1   -DLED_DISK_INVERT
DEFS            += -DLED_KERNEL_PORT=TRISE -DLED_KERNEL_PIN=2 -DLED_KERNEL_INVERT
DEFS            += -DLED_TTY_PORT=TRISE    -DLED_TTY_PIN=3    -DLED_TTY_INVERT

# Swap on external device, 2 Mbytes
#   data at E0-E7
#     rd at E8
#     wr at E9
# ldaddr at C1
DEFS            += -DSWAPDEV=0x0100 -DSWAPSZ=2048
DEFS            += -DSW_DATA_PORT=TRISE -DSW_DATA_PIN=0
DEFS            += -DSW_RD_PORT=TRISE   -DSW_RD_PIN=8
DEFS            += -DSW_WR_PORT=TRISE   -DSW_WR_PIN=9
DEFS            += -DSW_LDA_PORT=TRISC  -DSW_LDA_PIN=1

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
		  signal.o machdep.o mem.o exception.o
KERNOBJ		+= cons.o swap.o

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
		$(OBJDUMP) -d -S -z $@ > unix.dis
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
