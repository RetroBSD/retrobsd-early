# Copyright (c) 1986 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
# This makefile is designed to be run as:
#	make
#
# The `make' will compile everything, including a kernel, utilities
# and a root filesystem image.

#
# Supported boards
#
MAX32           = sys/pic32/max32
UBW32           = sys/pic32/ubw32
UBW32UART       = sys/pic32/ubw32-uart
MAXIMITE        = sys/pic32/maximite
EXPLORER16      = sys/pic32/explorer16
STARTERKIT      = sys/pic32/starter-kit
DUINOMITE       = sys/pic32/duinomite
PINGUINO        = sys/pic32/pinguino-micro
DIP             = sys/pic32/dip
BAREMETAL       = sys/pic32/baremetal

# Select target board
TARGET          ?= $(MAX32)

# Filesystem and swap sizes.
FS_KBYTES       = 16384
SWAP_KBYTES     = 2048

# Set this to the device name for your SD card.  With this
# enabled you can use "make installfs" to copy the filesys.img
# to the SD card.

# SDCARD          = /dev/sdb

#
# C library options: passed to libc makefile.
# See lib/libc/Makefile for explanation.
#
DEFS		=

# global flags
# SRC_MFLAGS are used on makes in command source directories,
# but not in library or compiler directories that will be installed
# for use in compiling everything else.
#
SRC_MFLAGS	= -k

# Programs that live in subdirectories, and have makefiles of their own.
#
# 'share' has to be towards the front of the list because programs such as
# lint(1) need their data files, etc installed first.

LIBDIR		= lib
SRCDIR		= tools etc share bin sbin

CLEANDIR	= $(LIBDIR) $(SRCDIR) $(UBW32) $(UBW32UART) \
                  $(MAXIMITE) $(MAX32) $(EXPLORER16) $(STARTERKIT)

FSUTIL		= tools/fsutil/fsutil

#
# Filesystem contents.
#
SBIN_FILES	= sbin/chown sbin/chroot sbin/disktool sbin/fsck sbin/halt sbin/init \
                  sbin/mkfs sbin/mknod sbin/mkpasswd sbin/mount \
                  sbin/pstat sbin/reboot sbin/shutdown sbin/umount \
                  sbin/update sbin/vipw sbin/poweroff
ETC_FILES	= etc/rc etc/rc.local etc/ttys etc/gettytab etc/group \
                  etc/passwd etc/shadow etc/fstab etc/motd etc/shells \
                  etc/termcap
BIN_FILES	= bin/apropos bin/aout bin/ar bin/as bin/awk bin/basename \
                  bin/basic bin/bc bin/cal bin/cat bin/cb bin/cc bin/chflags bin/chgrp \
                  bin/chmod bin/chpass bin/cmp bin/col bin/comm bin/cp \
                  bin/cpp bin/date bin/dc bin/dd bin/df bin/diff bin/du \
                  bin/echo bin/ed bin/egrep bin/expr bin/false bin/fgrep \
                  bin/file bin/find bin/forth bin/fstat bin/grep bin/groups bin/head \
                  bin/hostid bin/hostname bin/id bin/iostat bin/join \
                  bin/kill bin/la bin/ld bin/last bin/ln bin/ls bin/login bin/lol \
                  bin/mail bin/make bin/man bin/mesg bin/mkdir bin/more bin/mv \
                  bin/nice bin/nm bin/nohup bin/od bin/pagesize bin/passwd \
                  bin/portio bin/pr bin/printf bin/ps bin/pwd bin/re bin/ranlib \
                  bin/renice bin/renumber bin/rev bin/rm bin/rmail bin/rmdir bin/rz \
                  bin/scm bin/sed bin/sh bin/size bin/sl bin/sleep bin/sort bin/split \
                  bin/strip bin/stty bin/su bin/sum bin/sync bin/sysctl \
                  bin/sz bin/tail bin/tar bin/tee bin/test \
                  bin/time bin/touch bin/tr bin/tsort bin/true bin/tty \
                  bin/uname bin/uniq bin/vmstat bin/w bin/wall bin/wc \
                  bin/whereis bin/who bin/whoami bin/write bin/xargs
LIB_FILES	=
LIBEXEC_FILES	= libexec/bigram libexec/code libexec/diffh libexec/getty
INC_FILES	= include/stdio.h include/syscall.h include/sys/types.h \
                  include/sys/select.h
SHARE_FILES	= share/re.help share/example/Makefile \
                  share/example/ashello.S share/example/chello.c \
                  share/example/blkjack.bas share/example/hilow.bas \
                  share/example/stars.bas share/example/prime.scm \
                  share/example/fact.fth
ALLFILES	= $(SBIN_FILES) $(ETC_FILES) $(BIN_FILES) $(LIB_FILES) $(LIBEXEC_FILES) \
                  $(INC_FILES) $(SHARE_FILES) var/log/messages var/log/wtmp .profile
ALLDIRS         = sbin/ bin/ dev/ etc/ tmp/ lib/ libexec/ share/ share/example/ \
                  share/misc/ var/ var/run/ var/log/ u/ include/ include/sys/

BDEVS           = dev/sd0!b0:0 dev/sd1!b0:1 dev/sw0!b1:0
CDEVS           = dev/console!c0:0 \
                  dev/mem!c1:0 dev/kmem!c1:1 dev/null!c1:2 dev/zero!c1:3 \
                  dev/tty!c2:0 \
                  dev/rsd0!c3:0 dev/rsd1!c3:1 dev/swap!c3:0 \
                  dev/klog!c4:0 \
                  dev/stdin!c5:0 dev/stdout!c5:1 dev/stderr!c5:2 \
                  dev/rsw0!c6:0 \
                  dev/porta!c7:0 dev/portb!c7:1 dev/portc!c7:2 \
                  dev/portd!c7:3 dev/porte!c7:4 dev/portf!c7:5 dev/portg!c7:6 \
                  dev/confa!c7:64 dev/confb!c7:65 dev/confc!c7:66 \
                  dev/confd!c7:67 dev/confe!c7:68 dev/conff!c7:69 dev/confg!c7:70
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

all:
		$(MAKE) -C $(LIBDIR) DEFS="$(DEFS)"
		$(MAKE) -C $(TARGET)
		for dir in $(SRCDIR); do $(MAKE) -C $$dir $(SRC_MFLAGS); done
		$(MAKE) filesys.img user.img

filesys.img:	$(FSUTIL) $(TARGET)/unix.elf $(ALLFILES)
		rm -f $@
		$(FSUTIL) -n$(FS_KBYTES) -s$(SWAP_KBYTES) $@
		$(FSUTIL) -a $@ $(ALLDIRS) $(ALLFILES)
		$(FSUTIL) -a $@ $(CDEVS)
		$(FSUTIL) -a $@ $(BDEVS)
#		$(FSUTIL) -a $@ $(FDDEVS)

user.img:	$(FSUTIL)
		rm -f $@
		$(FSUTIL) -n$(FS_KBYTES) $@

$(FSUTIL):
		cd tools/fsutil; $(MAKE)

clean:
		rm -f *~
		for dir in $(CLEANDIR); do $(MAKE) -C $$dir -k clean; done

realclean: clean
		rm -f sys/pic32/*/unix.hex

# TODO
buildlib:
		@echo installing /usr/include
		# cd include; $(MAKE) DESTDIR=$(DESTDIR) install
		@echo
		@echo compiling libc.a
		cd lib/libc; $(MAKE) $(LIBCDEFS)
		@echo installing /lib/libc.a
		cd lib/libc; $(MAKE) DESTDIR=$(DESTDIR) install
		@echo
		@echo compiling C compiler
		cd lib; $(MAKE) ccom cpp c2
		@echo installing C compiler
		cd lib/ccom; $(MAKE) DESTDIR=$(DESTDIR) install
		cd lib/cpp; $(MAKE) DESTDIR=$(DESTDIR) install
		cd lib/c2; $(MAKE) DESTDIR=$(DESTDIR) install
		cd lib; $(MAKE) clean
		@echo
		@echo re-compiling libc.a
		cd lib/libc; $(MAKE) $(LIBCDEFS)
		@echo re-installing /lib/libc.a
		cd lib/libc; $(MAKE) DESTDIR=$(DESTDIR) install
		@echo
		@echo re-compiling C compiler
		cd lib; $(MAKE) ccom cpp c2
		@echo re-installing C compiler
		cd lib/ccom; $(MAKE) DESTDIR=$(DESTDIR) install
		cd lib/cpp; $(MAKE) DESTDIR=$(DESTDIR) install
		cd lib/c2; $(MAKE) DESTDIR=$(DESTDIR) install
		@echo

installfs: filesys.img
ifdef SDCARD
	sudo dd bs=16k if=filesys.img of=$(SDCARD)
else
	@echo "Error: No SDCARD defined."
endif
