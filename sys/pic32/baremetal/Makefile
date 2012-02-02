#
# Build RetroBSD kernel for PIC32 platform.
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
# Basic pic32mx795f512l chip with internal oscillator
# ===================================================
#
# Console on UART1 with hardware handshaking
DEFS            += -DCONSOLE_UART1 -DCONSOLE_RTSCTS
# SD/MMC card driver on SPI1
DEFS            += -DSD_PORT=SPI1CON -DSD_MHZ=10
# /CS0 at pin B1
DEFS            += -DSD_CS0_PORT=TRISC -DSD_CS0_PIN=14
# LEDs at pins A0, A1, A2, A3
DEFS            += -DLED_DISK_PORT=TRISE   -DLED_DISK_PIN=4
DEFS            += -DLED_KERNEL_PORT=TRISE -DLED_KERNEL_PIN=3
DEFS            += -DLED_TTY_PORT=TRISE    -DLED_TTY_PIN=2
DEFS            += -DLED_AUX_PORT=TRISG    -DLED_AUX_PIN=13

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
LDFLAGS         += -nostdlib -T bare-metal.ld -Wl,-Map=unix.map
SIZE		= $(GCCPREFIX)size
OBJDUMP		= $(GCCPREFIX)objdump
OBJCOPY		= $(GCCPREFIX)objcopy
MPIDE           = /Applications/Mpide.app/Contents/Resources/Java/

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
		   ufs_bmap.o ufs_dsort.o ufs_fio.o \
		   ufs_inode.o ufs_mount.o ufs_namei.o ufs_subr.o \
		   ufs_syscalls.o ufs_syscalls2.o vfs_vnops.o \
		   vm_sched.o vm_swap.o vm_swp.o

# Drivers.
KERNOBJ		+= sd.o 

# Configuration-dependent files.
KERNOBJ		+= vers.o devcfg.o

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

unix.elf:	$(KERNOBJ) bare-metal.ld
		$(CC) $(LDFLAGS) $(KERNOBJ) -o $@
		chmod -x $@
		$(OBJDUMP) -d -S $@ > unix.dis
		$(OBJCOPY) -O ihex --change-addresses=0x80000000 $@ unix.hex

load:           unix.elf
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
