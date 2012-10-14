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
MAX32           = pic32/max32
UBW32           = pic32/ubw32
UBW32UART       = pic32/ubw32-uart
MAXIMITE        = pic32/maximite
EXPLORER16      = pic32/explorer16
STARTERKIT      = pic32/starter-kit
DUINOMITE       = pic32/duinomite
PINGUINO        = pic32/pinguino-micro
DIP             = pic32/dip
BAREMETAL       = pic32/baremetal
RETROONE	= pic32/retroone

# Select target board
TARGET          ?= $(MAX32)

# Filesystem and swap sizes.
FS_KBYTES       = 16384
SWAP_KBYTES     = 2048

# Set this to the device name for your SD card.  With this
# enabled you can use "make installfs" to copy the filesys.img
# to the SD card.

#SDCARD          = /dev/sdb

#
# C library options: passed to libc makefile.
# See lib/libc/Makefile for explanation.
#
DEFS		=

FSUTIL		= tools/fsutil/fsutil

#
# Filesystem contents.
#
BIN_FILES	:= $(wildcard bin/*)
SBIN_FILES	:= $(wildcard sbin/*)
GAMES_FILES	:= $(shell find games -type f ! -path '*/.*')
LIB_FILES	:= $(wildcard lib/*)
LIBEXEC_FILES	:= $(wildcard libexec/*)
ETC_FILES	= etc/rc etc/rc.local etc/ttys etc/gettytab etc/group \
                  etc/passwd etc/shadow etc/fstab etc/motd etc/shells \
                  etc/termcap
INC_FILES	= include/stdio.h include/syscall.h include/sys/types.h \
                  include/sys/select.h
SHARE_FILES	= share/re.help share/example/Makefile \
                  share/example/ashello.S share/example/chello.c \
                  share/example/blkjack.bas share/example/hilow.bas \
                  share/example/stars.bas share/example/prime.scm \
                  share/example/fact.fth share/example/echo.S \
                  share/smallc/lib.c share/smallc/Makefile share/smallc/primelist.c \
                  share/smallc/primesum.c share/smallc/sys.s
ALLFILES	= $(SBIN_FILES) $(ETC_FILES) $(BIN_FILES) $(LIB_FILES) $(LIBEXEC_FILES) \
                  $(INC_FILES) $(SHARE_FILES) $(GAMES_FILES) \
                  var/log/messages var/log/wtmp .profile
ALLDIRS         = sbin/ bin/ dev/ etc/ tmp/ lib/ libexec/ share/ share/example/ \
                  share/misc/ share/smallc/ var/ var/run/ var/log/ u/ include/ include/sys/ \
                  games/ games/lib/ 
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
                  dev/confd!c7:67 dev/confe!c7:68 dev/conff!c7:69 dev/confg!c7:70 \
                  dev/spi1!c9:0 dev/spi2!c9:1 dev/spi3!c9:2 dev/spi4!c9:3 \
		  dev/glcd0!c10:0
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
ADCDEVS         = dev/adc0!c8:0 dev/adc1!c8:1 dev/adc2!c8:2 dev/adc3!c8:3 \
                  dev/adc4!c8:4 dev/adc5!c8:5 dev/adc6!c8:6 dev/adc7!c8:7 \
                  dev/adc8!c8:8 dev/adc9!c8:9 dev/adc10!c8:10 dev/adc11!c8:11 \
                  dev/adc12!c8:12 dev/adc13!c8:13 dev/adc14!c8:14 dev/adc15!c8:15
OCDEVS		= dev/oc0!c11:0 dev/oc1!c11:1 dev/oc2!c11:2 dev/oc3!c11:3 dev/oc4!c11:4

all:            build kernel
		$(MAKE) fs

fs:             filesys.img user.img

kernel:
		$(MAKE) -C sys/$(TARGET)

build:
		$(MAKE) -C tools
		$(MAKE) -C src install

filesys.img:	$(FSUTIL) $(ALLFILES)
		rm -f $@
		$(FSUTIL) -n$(FS_KBYTES) -s$(SWAP_KBYTES) $@
		$(FSUTIL) -a $@ $(ALLDIRS) $(ALLFILES)
		$(FSUTIL) -a $@ $(CDEVS)
		$(FSUTIL) -a $@ $(BDEVS)
		$(FSUTIL) -a $@ $(ADCDEVS)
		$(FSUTIL) -a $@ $(OCDEVS)
#		$(FSUTIL) -a $@ $(FDDEVS)

user.img:	$(FSUTIL)
		rm -f $@
		$(FSUTIL) -n$(FS_KBYTES) $@

$(FSUTIL):
		cd tools/fsutil; $(MAKE)

clean:
		rm -f *~
		for dir in tools src sys/pic32; do $(MAKE) -C $$dir -k clean; done

cleanall:       clean
		rm -f sys/pic32/*/unix.hex bin/* sbin/* lib/* games/[a-k]* games/[m-z]* libexec/* share/man/cat*/*
		rm -f games/lib/adventure.dat
		rm -f games/lib/cfscores
		rm -f share/re.help
		rm -f share/misc/more.help
		rm -f etc/termcap


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
