H		= ../../include
M		= ..
S		= ../../kernel

vpath %.c $(M):$(S)
vpath %.S $(M):$(S)

DEFS        += -I. -I$(H) -DKERNEL -DUCB_METER -DPIC32MX7

# Include any local specific configuration overrides
-include Makefile.local


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
PROGTOOL        = $(AVRDUDE) -c stk500v2 -p pic32 -b 115200

# Machine-dependent files:
# startup.o MUST be loaded first.
KERNOBJ		= startup.o clock.o devsw.o sysctl.o \
		  signal.o machdep.o mem.o exception.o \
                
KERNOBJ		+= cons.o rd_sramc.o

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
		   vm_sched.o vm_swap.o vm_swp.o kern_glob.o swap.o spi_bus.o

# Drivers.
KERNOBJ		+= rdisk.o rd_sd.o

# Configuration-dependent files.
KERNOBJ		+= vers.o

# This makefile does the work including the right files and options for the drivers
include ../drivers.mk

all:		.deps sys machine unix.elf
		$(SIZE) unix.elf

clean:
		rm -rf .deps *.o *.elf *.bin *.dis *.map *.srec core \
		mklog assym.h vers.c genassym sys machine

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

load:           unix.hex
		pic32prog unix.hex

vers.o:		../newvers.sh $(H)/*.h $(M)/*.[ch] $(S)/*.c
		sh ../newvers.sh > vers.c
		$(CC) -c vers.c

.SUFFIXES:	.i .srec .hex .dis .cpp .cxx .bin .elf

.o.dis:
		$(OBJDUMP) -d -z -S $< > $@

ifeq (.deps, $(wildcard .deps))
-include .deps/*.dep
endif