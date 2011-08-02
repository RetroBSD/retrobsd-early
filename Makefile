#
# Copyright (c) 1986 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
# This makefile is designed to be run as:
#	make build
#	make installsrc
# The `make build' will compile and install the libraries
# before building the rest of the sources. The `make installsrc'
# will then install the remaining binaries.
#
# It can also be run in the more conventional way:
#	make
#	make install
# The `make' will compile everything without installing anything.
# The `make install' will then install everything. Note however
# that all the binaries will have been loaded with the old libraries.
#
# C library options: passed to libc makefile.
# See lib/libc/Makefile for explanation.
#
DEFS		=
LIBCDEFS	= DEFS="${DEFS}"

# global flags
# SRC_MFLAGS are used on makes in command source directories,
# but not in library or compiler directories that will be installed
# for use in compiling everything else.
#
DESTDIR		=
CFLAGS		= -O
SRC_MFLAGS	= -k

# Programs that live in subdirectories, and have makefiles of their own.
#
# 'share' has to be towards the front of the list because programs such as
# lint(1) need their data files, etc installed first.

LIBDIR		= lib
SRCDIR		= tools sys etc share bin sbin

FSUTIL		= tools/fsutil/fsutil
UBW32		= tools/ubw32/ubw32
ROOTDIRS	= sbin/ bin/ dev/ etc/ tmp/
SBIN_FILES	= sbin/init sbin/fsck sbin/mkfs sbin/newfs sbin/reboot
ETC_FILES	= etc/rc etc/rc.local etc/ttys
BIN_FILES	= bin/basename bin/bc bin/cal bin/cat bin/cb bin/chgrp bin/chmod \
                  bin/cmp bin/col bin/comm bin/cp bin/date bin/dc bin/dd bin/df \
                  bin/du bin/echo bin/ed bin/egrep bin/expr bin/false bin/fgrep bin/file \
                  bin/grep bin/hostid bin/join bin/kill bin/ln bin/ls \
                  bin/mesg bin/mkdir bin/mv bin/nice bin/nohup bin/od bin/pagesize \
                  bin/pr bin/ps bin/pwd bin/rev bin/rm bin/rmail bin/rmdir \
                  bin/sh bin/size bin/sleep bin/sort bin/split bin/strip \
                  bin/stty bin/sum bin/sync bin/tar bin/tee bin/time \
                  bin/touch bin/tr bin/tsort bin/true bin/tty bin/uniq bin/who
ROOTFILES	= $(SBIN_FILES) $(ETC_FILES) $(BIN_FILES)
BDEVS           = dev/sd0h!b0:0 dev/sd1h!b0:1
CDEVS           = dev/console!c0:0 \
                  dev/mem!c1:0 dev/kmem!c1:1 dev/null!c1:2 dev/zero!c1:3 \
                  dev/tty!c2:0 \
                  dev/rsd0h!c3:0 dev/rsd1h!c3:1 dev/swap!c3:0 \
                  dev/klog!c4:0 dev/errlog!c4:1 dev/acctlog!c4:2 \
                  dev/stdin!c5:0 dev/stdout!c5:1 dev/stderr!c5:2
FDDEVS          = dev/fd/ dev/fd/0!c5:0 dev/fd/1!c5:1 dev/fd/2!c5:2 \
                  dev/fd/3!c5:3 dev/fd/4!c5:4 dev/fd/5!c5:5 dev/fd/6!c5:6 \
                  dev/fd/7!c5:7 dev/fd/8!c5:8 dev/fd/9!c5:9 dev/fd/10!c5:10 \
                  dev/fd/11!c5:11 dev/fd/12!c5:12 dev/fd/13!c5:13 \
                  dev/fd/14!c5:14 dev/fd/15!c5:15 dev/fd/16!c5:16 \
                  dev/fd/17!c5:17 dev/fd/18!c5:18 dev/fd/19!c5:19 \
                  dev/fd/20!c5:20 dev/fd/21!c5:21 dev/fd/22!c5:22 \
                  dev/fd/23!c5:23 dev/fd/24!c5:24 dev/fd/25!c5:25 \
                  dev/fd/26!c5:26 dev/fd/27!c5:27 dev/fd/28!c5:28 \
                  dev/fd/29!c5:29

all:		${LIBDIR} ${SRCDIR} root.bin unix.hex

unix.hex:       sys/pic32/compile/unix.hex
		cp -p $? $@

load:           unix.hex $(UBW32)
		$(UBW32) -write unix.hex -reset

lib:		FRC
		cd lib/startup-mips; make ${MFLAGS}
		cd lib/libc; make ${MFLAGS} ${LIBCDEFS}

${SRCDIR}: FRC
		cd $@; make ${MFLAGS} ${SRC_MFLAGS}

root.bin:	$(FSUTIL) sys/pic32/compile/unix.elf $(ROOTFILES)
		rm -f $@
		$(FSUTIL) -n16384 -s2048 $@
		$(FSUTIL) -a $@ $(ROOTDIRS) $(ROOTFILES)
		$(FSUTIL) -a $@ $(CDEVS)
		$(FSUTIL) -a $@ $(BDEVS)
#		$(FSUTIL) -a $@ $(FDDEVS)

sdcard:         root.bin
		sudo dd if=root.bin of=/dev/sdb

$(FSUTIL):
		cd tools/fsutil; make ${MFLAGS}

build:		buildlib ${SRCDIR}

buildlib: FRC
		@echo installing /usr/include
		# cd include; make ${MFLAGS} DESTDIR=${DESTDIR} install
		@echo
		@echo compiling libc.a
		cd lib/libc; make ${MFLAGS} ${LIBCDEFS}
		@echo installing /lib/libc.a
		cd lib/libc; make ${MFLAGS} DESTDIR=${DESTDIR} install
		@echo
		@echo compiling C compiler
		cd lib; make ${MFLAGS} ccom cpp c2
		@echo installing C compiler
		cd lib/ccom; make ${MFLAGS} DESTDIR=${DESTDIR} install
		cd lib/cpp; make ${MFLAGS} DESTDIR=${DESTDIR} install
		cd lib/c2; make ${MFLAGS} DESTDIR=${DESTDIR} install
		cd lib; make ${MFLAGS} clean
		@echo
		@echo re-compiling libc.a
		cd lib/libc; make ${MFLAGS} ${LIBCDEFS}
		@echo re-installing /lib/libc.a
		cd lib/libc; make ${MFLAGS} DESTDIR=${DESTDIR} install
		@echo
		@echo re-compiling C compiler
		cd lib; make ${MFLAGS} ccom cpp c2
		@echo re-installing C compiler
		cd lib/ccom; make ${MFLAGS} DESTDIR=${DESTDIR} install
		cd lib/cpp; make ${MFLAGS} DESTDIR=${DESTDIR} install
		cd lib/c2; make ${MFLAGS} DESTDIR=${DESTDIR} install
		@echo

FRC:

install:
		-for i in ${LIBDIR} ${SRCDIR}; do \
			(cd $$i; \
			make ${MFLAGS} ${SRC_MFLAGS} DESTDIR=${DESTDIR} install); \
		done

installsrc:
		-for i in ${SRCDIR}; do \
			(cd $$i; \
			make ${MFLAGS} ${SRC_MFLAGS} DESTDIR=${DESTDIR} install); \
		done

tags:
		for i in include lib; do \
			(cd $$i; make ${MFLAGS} TAGSFILE=../tags tags); \
		done
		sort -u +0 -1 -o tags tags

clean:
		rm -f a.out core root.bin swap.bin *.s *.o *~
		for i in ${LIBDIR} ${SRCDIR}; do (cd $$i; make -k ${MFLAGS} clean); done
