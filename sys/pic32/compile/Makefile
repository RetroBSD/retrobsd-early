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

# UBW32
# Console on UART1
DEFS            += -DCONSOLE_UART1
# SD/MMC card driver on SPI1
DEFS            += -DSD_PORT=SPI1CON
# /CS0 at pin A9 (and optional /CS1 at pin A10)
DEFS            += -DSD_CS0_PORT=TRISA -DSD_CS0_PIN=9
#DEFS            += -DSD_CS1_PORT=TRISA -DSD_CS1_PIN=10
# LEDs at pins E0, E1, E2, E3
DEFS            += -DLED_AUX_PORT=TRISE    -DLED_AUX_PIN=0
DEFS            += -DLED_DISK_PORT=TRISE   -DLED_DISK_PIN=1
DEFS            += -DLED_KERNEL_PORT=TRISE -DLED_KERNEL_PIN=2
DEFS            += -DLED_TTY_PORT=TRISE    -DLED_TTY_PIN=3

# MAXIMITE
# Console on UART4
#DEFS            += -DCONSOLE_UART4
# SD/MMC card driver on SPI4
#DEFS            += -DSD_PORT=SPI4CON
# /CS0 at pin E0
#DEFS            += -DSD_CS0_PORT=TRISE -DSD_CS0_PIN=0
# LEDs at pins E1, F0 and opposite polarity to UBW32
#DEFS            += -DLED_POLARITY
#DEFS            += -DLED_DISK_PORT=TRISE   -DLED_DISK_PIN=1
#DEFS            += -DLED_KERNEL_PORT=TRISF -DLED_KERNEL_PIN=0

DEPFLAGS	= -MT $@ -MD -MP -MF .deps/$*.dep
CFLAGS		= -O $(DEFS) $(DEPFLAGS)
ASFLAGS		= $(DEFS) $(DEPFLAGS)
CC		= /usr/local/mips461/bin/mips-elf-gcc -EL -g -mips32r2
CC		+= -nostdinc -fno-builtin -Werror -Wall -fno-dwarf2-cfi-asm
SIZE		= /usr/local/mips461/bin/mips-elf-size
OBJDUMP		= /usr/local/mips461/bin/mips-elf-objdump
OBJCOPY		= /usr/local/mips461/bin/mips-elf-objcopy

# Machine-dependent files:
# startup.o MUST be loaded first.
KERNOBJ		= startup.o clock.o devsw.o cons.o sysctl.o \
		  signal.o machdep.o mem.o exception.o

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
		   ufs_bmap.o ufs_disksubr.o ufs_dsort.o ufs_fio.o \
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
		rm -rf .deps *.o *.elf *.bin *.dis *.map *.hex *.srec core mklog assym.h vers.c genassym
		find ../.. -name \*~ | xargs rm -f

.deps:
		mkdir .deps

sys:
		ln -s ../../include $@

machine:
		ln -s .. $@

unix.elf:	$(KERNOBJ) using-bootloader.ld
		$(CC) -nostdlib -T using-bootloader.ld -Wl,-Map=unix.map $(KERNOBJ) -o $@
		chmod -x $@
		$(OBJDUMP) -d -S $@ > unix.dis
		$(OBJCOPY) -O binary $@ unix.bin
		$(OBJCOPY) -O ihex --change-addresses=0x80000000 $@ unix.hex
		chmod -x $@ unix.bin

load:           unix.elf
		ubw32 -write unix.hex -reset

vers.o:		$(H)/*.h $(M)/*.[ch] $(S)/*.c
		sh newvers.sh
		$(CC) -c vers.c

.SUFFIXES:	.i .srec .hex .dis .cpp .cxx .bin .elf

.o.dis:
		$(OBJDUMP) -d -z -S $< > $@

ifeq (.deps, $(wildcard .deps))
-include .deps/*.dep
endif
