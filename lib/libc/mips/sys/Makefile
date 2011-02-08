TOPSRC		= $(shell cd ../../../..; pwd)
include $(TOPSRC)/target.mk

ASFLAGS		= ${DEFS}

# modules which can not use SYSCALL and must be assembled from sources.  The
# rest of the system calls are generated with printf(1) and do not have
# source files associated with them.

PDPSRCS		= _exit.S brk.S execl.S execle.S execv.S fork.S pipe.S ptrace.S sbrk.S \
		  sigaction.S sigreturn.S vfork.S

PDPOBJS		= _exit.o brk.o execl.o execle.o execv.o fork.o pipe.o ptrace.o sbrk.o \
		  sigaction.o sigreturn.o vfork.o

# these are generated with a second argument to SYSCALL of 'norm'.  Have to
# split into more than one list because 'make' only permits 80 "lefts".

NORM		= __sysctl.o accept.o access.o adjtime.o bind.o chdir.o chflags.o \
		  chmod.o chown.o chroot.o close.o connect.o dup.o dup2.o \
		  execve.o fchdir.o fchflags.o fchmod.o fchown.o fcntl.o flock.o fstat.o \
		  fsync.o ftruncate.o getdtablesize.o getgroups.o getitimer.o \
		  getsockname.o \
		  getpeername.o getpriority.o getrlimit.o getrusage.o getsockopt.o \
		  gettimeofday.o ioctl.o kill.o killpg.o link.o listen.o lstat.o mkdir.o \
		  mknod.o mount.o open.o pselect.o quota.o read.o readlink.o readv.o \
		  reboot.o \
		  recv.o recvfrom.o recvmsg.o rename.o rmdir.o select.o send.o sendmsg.o \
		  sendto.o setgroups.o setitimer.o setpgrp.o setpriority.o setquota.o \
		  setuid.o seteuid.o setgid.o setegid.o \
		  setrlimit.o setsockopt.o settimeofday.o \
		  shutdown.o sigaltstack.o socket.o socketpair.o stat.o symlink.o \
		  sigprocmask.o sigstack.o sigwait.o \
		  statfs.o fstatfs.o getfsstat.o \
		  truncate.o umount.o unlink.o utimes.o wait4.o write.o writev.o \
		  lseek.o

# these are generated with a second argument to SYSCALL of 'error'

ERROR		= sigsuspend.o

# these are generated with a second argument to SYSCALL of 'noerror'

NOERROR		= getgid.o getegid.o getpgrp.o getpid.o \
		  getppid.o getuid.o geteuid.o profil.o sigpending.o sync.o \
		  umask.o vhangup.o

OBJS		= ${NORM} ${LONG} ${ERROR} ${NOERROR}

TAGSFILE	= tags

sys.a:		${OBJS}
		@echo "building sys.a"
		@$(AR) cru sys.a ${OBJS}

${NORM}:	SYS.h #/usr/include/syscall.h
		@echo creating $*.o
		@printf '#include "SYS.h"\nSYSCALL_norm($*)\n' | \
			$(AS) ${ASFLAGS} - -c -o $*.o

${ERROR}:	SYS.h #/usr/include/syscall.h
		@echo creating $*.o
		@printf '#include "SYS.h"\nSYSCALL_error($*)\n' | \
			$(AS) ${ASFLAGS} - -c -o $*.o

${NOERROR}:	SYS.h #/usr/include/syscall.h
		@echo creating $*.o
		@printf '#include "SYS.h"\nSYSCALL_noerror($*)\n' | \
			$(AS) ${ASFLAGS} - -c -o $*.o

clean:
		rm -f *~ *.o a.out sys.a
