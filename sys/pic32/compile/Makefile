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
D		= ../../drivers

vpath %.c ${M}:${S}:${D}
vpath %.S ${M}:${S}:${D}

# Kernel options.
DEFS		+= -I. -I${H} -DKERNEL -DPIC32MX -DUCB_METER

# kl console driver
DEFS		+= -DNKL=1

# rk disk driver
DEFS            += -DNRK=2

DEPFLAGS	= -MT $@ -MD -MP -MF .deps/$*.dep
CFLAGS		= -O ${DEFS} ${DEPFLAGS}
ASFLAGS		= ${DEFS} ${DEPFLAGS}
CC		= /usr/local/mipsel441/bin/mipsel-elf32-gcc -mips32r2 -nostdinc -fno-builtin
CC		+= -Werror #-Wall
SIZE		= /usr/local/mipsel441/bin/mipsel-elf32-size

# Machine-dependent files:
# startuo.o MUST be loaded first, param.o MUST be loaded last.
KERNOBJ		= startup.o clock.o conf.o cons.o sysctl.o \
		  machdep.o machdep2.o mem.o trap.o

# Kernel.
KERNOBJ		+= init_main.o init_sysent.o kern_clock.o \
		   kern_descrip.o kern_exec.o kern_exit.o kern_fork.o \
		   kern_mman.o kern_proc.o kern_prot.o \
		   kern_prot2.o kern_resource.o kern_sig.o kern_sig2.o \
		   kern_subr.o kern_synch.o kern_sysctl.o kern_time.o \
		   kern_xxx.o subr_log.o subr_prf.o subr_rmap.o \
		   syscalls.o sys_generic.o sys_inode.o \
		   sys_pipe.o sys_process.o tty.o tty_conf.o \
		   tty_subr.o tty_tty.o ufs_alloc.o ufs_bio.o \
		   ufs_bmap.o ufs_disksubr.o ufs_dsort.o ufs_fio.o \
		   ufs_inode.o ufs_mount.o ufs_namei.o ufs_subr.o \
		   ufs_syscalls.o ufs_syscalls2.o vfs_vnops.o vm_proc.o \
		   vm_sched.o vm_swap.o vm_swp.o vm_text.o

# Drivers.
KERNOBJ		+= rk.o

# Configuration-dependent files: param.o MUST be loaded last.
KERNOBJ		+= vers.o param.o

all:		.deps sys machine unix

clean:
		rm -rf unix core mklog *.o assym.h vers.c genassym .deps
		find ../.. -name \*~ | xargs rm -f

.deps:
		mkdir .deps

sys:
		ln -s ../../include $@

machine:
		ln -s .. $@

unix:		${KERNOBJ}
		$(CC) -nostdlib -T using-bootloader.ld ${KERNOBJ} -r -o unix
		$(SIZE) unix

vers.o:		${H}/*.h ${M}/*.[ch] ${S}/*.c ${D}/*.[ch]
		sh newvers.sh
		${CC} -c vers.c

ifeq (.deps, $(wildcard .deps))
-include .deps/*.dep
endif
