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

OPTS		= -DGENERIC -DPIC32MX
DEFS		= -DKERNEL ${OPTS} -I. -I${H}
CFLAGS		= -O ${DEFS}
ASFLAGS		= ${DEFS}
CC		= /usr/local/mipsel441/bin/mipsel-elf32-gcc -mips32r2 -nostdinc -fno-builtin
SIZE		= /usr/local/mipsel441/bin/mipsel-elf32-size

# Machine-dependent files:
# mch_start.o MUST be loaded first, param.o MUST be loaded last.
KERNOBJ		= mch_start.o clock.o conf.o cons.o machdep.o machdep2.o \
		  mem.o syscalls.o trap.o

# Kernel.
KERNOBJ		+= init_main.o init_sysent.o kern_acct.o kern_clock.o \
		   kern_descrip.o kern_exec.o kern_exit.o kern_fork.o \
		   kern_mman.o kern_pdp.o kern_proc.o kern_prot.o \
		   kern_prot2.o kern_resource.o kern_sig.o kern_sig2.o \
		   kern_subr.o kern_synch.o kern_sysctl.o kern_time.o \
		   kern_xxx.o subr_log.o subr_prf.o \
		   subr_rmap.o subr_xxx.o sys_generic.o sys_inode.o \
		   sys_pipe.o sys_process.o tty.o tty_conf.o \
		   tty_subr.o tty_tty.o ufs_alloc.o ufs_bio.o \
		   ufs_bmap.o ufs_disksubr.o ufs_dsort.o ufs_fio.o \
		   ufs_inode.o ufs_mount.o ufs_namei.o ufs_subr.o \
		   ufs_syscalls.o ufs_syscalls2.o vfs_vnops.o vm_proc.o \
		   vm_sched.o vm_swap.o vm_swp.o vm_text.o

# Drivers.
KERNOBJ		+= hk.o ht.o ra.o rk.o rx.o xp.o

# Configuration-dependent files: param.o MUST be loaded last.
KERNOBJ		+= ioconf.o vers.o param.o

BASE		= br.o dh.o dhu.o dhv.o dkbad.o dr.o dz.o init_sysent.o kern_clock.o \
		  kern_descrip.o kern_mman.o kern_proc.o kern_sig.o kern_subr.o \
		  kern_prot2.o kern_synch.o lp.o machdep.o ra.o ram.o si.o \
		  subr_rmap.o subr_xxx.o sys_inode.o trap.o tty.o \
		  tty_conf.o tty_subr.o tty_tb.o tty_tty.o ufs_alloc.o ufs_bio.o \
		  ufs_bmap.o ufs_dsort.o ufs_fio.o ufs_inode.o ufs_namei.o \
		  xp.o
OV1		= sys_generic.o ufs_syscalls.o vfs_vnops.o
OV2		= kern_acct.o kern_exec.o kern_exit.o kern_fork.o kern_resource.o
OV3		= clock.o cons.o kern_time.o \
		  machdep2.o quota_sys.o subr_prf.o sys_process.o \
		  syscalls.o ufs_mount.o
OV4		= hk.o init_main.o kern_prot.o tty_pty.o quota_kern.o quota_subr.o \
		  quota_ufs.o vm_swp.o vm_swap.o vm_proc.o
OV5		= ht.o tm.o ts.o
OV6		= tmscp.o tmscpdump.o mem.o
OV7		= rl.o ingreslock.o ufs_disksubr.o
OV8		= rx.o kern_sysctl.o vm_sched.o vm_text.o
OV9		= kern_pdp.o kern_xxx.o ufs_syscalls2.o ufs_subr.o rk.o sys_pipe.o \
		  kern_sig2.o subr_log.o

all:		sys machine unix

clean:
		rm -f unix core mklog *.o assym.h genassym ../*~ ../*/*~

sys:
		ln -s ../../include $@

machine:
		ln -s .. $@

unix:		${KERNOBJ}
		$(CC) -nostdlib -T using-bootloader.ld ${KERNOBJ} -r -o unix
		$(SIZE) unix

vers.o:		FRC
		sh newvers.sh
		${CC} -c vers.c

FRC:

SYSOBJS		= init_main.o init_sysent.o kern_acct.o kern_clock.o		\
		  kern_descrip.o kern_exec.o kern_exit.o kern_fork.o kern_mman.o	\
		  kern_proc.o kern_prot.o kern_resource.o kern_sig.o kern_sysctl.o \
		  kern_prot2.o kern_sig2.o					\
		  kern_subr.o kern_synch.o kern_time.o kern_xxx.o quota_sys.o	\
		  quota_kern.o quota_ufs.o quota_subr.o subr_log.o subr_prf.o subr_rmap.o \
		  ingreslock.o							\
		  subr_xxx.o sys_generic.o sys_inode.o sys_kern.o			\
		  sys_pipe.o sys_process.o syscalls.o tty.o tty_conf.o tty_pty.o	\
		  tty_subr.o tty_tb.o tty_tty.o ufs_alloc.o ufs_bio.o ufs_bmap.o	\
		  ufs_dsort.o ufs_fio.o ufs_inode.o ufs_mount.o ufs_namei.o	\
		  ufs_subr.o ufs_syscalls.o uipc_syscalls.o vm_proc.o vm_sched.o	\
		  ufs_disksubr.o ufs_syscalls2.o					\
		  vfs_vnops.o \
		  vm_swap.o vm_swp.o vm_text.o

MACHOBJS	= mch_start.o

DEVOBJS		= br.o dh.o dhu.o dhv.o dkbad.o dr.o dz.o hk.o ht.o lp.o ra.o rk.o \
		  rl.o rx.o si.o tm.o tmscp.o tmscpdump.o ts.o xp.o

###
