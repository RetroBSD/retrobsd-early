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
UBW32UARTSDRAM  = pic32/ubw32-uart-sdram
MAXIMITE        = pic32/maximite
EXPLORER16      = pic32/explorer16
STARTERKIT      = pic32/starter-kit
DUINOMITE       = pic32/duinomite
PINGUINO        = pic32/pinguino-micro
DIP             = pic32/dip
BAREMETAL       = pic32/baremetal
RETROONE	= pic32/retroone
FUBARINO	= pic32/fubarino
FUBARINOUART	= pic32/fubarino-uart

# Select target board
TARGET          ?= $(MAX32)

# Filesystem and swap sizes.
FS_KBYTES       = 163840
U_KBYTES        = 163840
SWAP_KBYTES     = 20480

# The ROOTSWAP is a bit of a pain.  It's required for fsck to operate.
# That's such a waste.  Never mind, it will go once the swap
# system is rewritten so that /dev/tmp can be used instead.
#
ROOTSWAP	= `expr $(FS_KBYTES) / 4000 + 520`

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

-include Makefile.user

#
# Filesystem contents.
#
BIN_FILES	:= $(wildcard bin/*)
SBIN_FILES	:= $(wildcard sbin/*)
GAMES_FILES	:= $(shell find games -type f ! -path '*/.*')
LIBEXEC_FILES	:= $(wildcard libexec/*)
LIB_FILES	:= lib/crt0.o lib/retroImage $(wildcard lib/*.a)
ETC_FILES	= etc/rc etc/rc.local etc/ttys etc/gettytab etc/group \
                  etc/passwd etc/shadow etc/fstab etc/motd etc/shells \
                  etc/termcap etc/MAKEDEV
INC_FILES	= $(wildcard include/*.h) \
                  $(wildcard include/sys/*.h) \
                  $(wildcard include/machine/*.h) \
                  $(wildcard include/smallc/*.h) \
                  $(wildcard include/smallc/sys/*.h) \
                  $(wildcard include/arpa/*.h)
SHARE_FILES	= share/re.help share/example/Makefile \
                  share/example/ashello.S share/example/chello.c \
                  share/example/blkjack.bas share/example/hilow.bas \
                  share/example/stars.bas share/example/prime.scm \
                  share/example/fact.fth share/example/echo.S \
                  $(wildcard share/smallc/*)
MANFILES	= share/man/ share/man/cat1/ share/man/cat2/ share/man/cat3/ \
		  share/man/cat4/ share/man/cat5/ share/man/cat6/ share/man/cat7/ \
		  share/man/cat8/ $(wildcard share/man/cat?/*)
ALLFILES	= $(SBIN_FILES) $(ETC_FILES) $(BIN_FILES) $(LIB_FILES) $(LIBEXEC_FILES) \
                  $(INC_FILES) $(SHARE_FILES) $(GAMES_FILES) \
                  var/log/messages var/log/wtmp .profile
ALLDIRS         = sbin/ bin/ dev/ etc/ tmp/ lib/ libexec/ share/ share/example/ \
                  share/misc/ share/smallc/ var/ var/run/ var/log/ u/ \
                  games/ games/lib/ include/ include/sys/ include/machine/ \
                  include/arpa/ include/smallc/ include/smallc/sys/ \
                  share/misc/ share/smallc/ var/ var/run/ var/log/ u/ include/ include/sys/ \
                  games/ games/lib/
BDEVS           = dev/rd0!b0:0 dev/rd0a!b0:1 dev/rd0b!b0:2 dev/rd0c!b0:3 dev/rd0d!b0:4
BDEVS           += dev/rd1!b1:0 dev/rd1a!b1:1 dev/rd1b!b1:2 dev/rd1c!b1:3 dev/rd1d!b1:4
BDEVS           += dev/rd2!b2:0 dev/rd2a!b2:1 dev/rd2b!b2:2 dev/rd2c!b2:3 dev/rd2d!b2:4
BDEVS           += dev/rd3!b3:0 dev/rd3a!b3:1 dev/rd3b!b3:2 dev/rd3c!b3:3 dev/rd3d!b3:4
BDEVS		+= dev/swap!b4:0

D_CONSOLE       += dev/console!c0:0
D_MEM		+= dev/mem!c1:0 dev/kmem!c1:1 dev/null!c1:2 dev/zero!c1:3
D_TTY		+= dev/tty!c2:0
D_LOG		+= dev/klog!c3:0
D_FD		+= dev/stdin!c4:0 dev/stdout!c4:1 dev/stderr!c4:2
D_GPIO		+= dev/porta!c5:0  dev/portb!c5:1  dev/portc!c5:2 \
                   dev/portd!c5:3  dev/porte!c5:4  dev/portf!c5:5 \
		   dev/portg!c5:6  dev/confa!c5:64 dev/confb!c5:65 \
		   dev/confc!c5:66 dev/confd!c5:67 dev/confe!c5:68 \
		   dev/conff!c5:69 dev/confg!c5:70
D_ADC            = dev/adc0!c6:0 dev/adc1!c6:1 dev/adc2!c6:2 dev/adc3!c6:3 \
                   dev/adc4!c6:4 dev/adc5!c6:5 dev/adc6!c6:6 dev/adc7!c6:7 \
                   dev/adc8!c6:8 dev/adc9!c6:9 dev/adc10!c6:10 dev/adc11!c6:11 \
                   dev/adc12!c6:12 dev/adc13!c6:13 dev/adc14!c6:14 dev/adc15!c6:15
D_SPI           += dev/spi1!c7:0 dev/spi2!c7:1 dev/spi3!c7:2 dev/spi4!c7:3
D_GLCD		+= dev/glcd0!c8:0
D_PWM		 = dev/oc1!c9:0 dev/oc2!c9:1 dev/oc3!c9:2 dev/oc4!c9:3 dev/oc5!c9:4

CDEVS		= $(D_CONSOLE) $(D_MEM) $(D_TTY) $(D_LOG) $(D_FD) $(D_GPIO) \
		  $(D_ADC) $(D_SPI) $(D_GLCD) $(G_PWM)

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
U_DIRS          = $(addsuffix /,$(shell find u -type d ! -path '*/.svn*'))
U_FILES         = $(shell find u -type f ! -path '*/.svn/*')
U_ALL           = $(patsubst u/%,%,$(U_DIRS) $(U_FILES))

all:            build kernel
		$(MAKE) fs

fs:             sdcard.rd

kernel:
		$(MAKE) -C sys/$(TARGET)

build:
		$(MAKE) -C tools
		$(MAKE) -C lib
		$(MAKE) -C src install

filesys.img:	$(FSUTIL) $(ALLFILES)
		rm -f $@
		$(FSUTIL) -n$(FS_KBYTES) -s$(ROOTSWAP) $@
		$(FSUTIL) -a $@ $(ALLDIRS) $(ALLFILES)
		$(FSUTIL) -a $@ $(MANFILES)
		$(FSUTIL) -a $@ $(CDEVS)
		$(FSUTIL) -a $@ $(BDEVS)
#		$(FSUTIL) -a $@ $(FDDEVS)

swap.img:
		dd if=/dev/zero of=$@ bs=1k count=$(SWAP_KBYTES)

user.img:	$(FSUTIL)
ifneq ($(U_KBYTES), 0)
		rm -f $@
		$(FSUTIL) -n$(U_KBYTES) $@
		(cd u; ../$(FSUTIL) -a ../$@ $(U_ALL))
endif

sdcard.rd:	filesys.img swap.img user.img
ifneq ($(U_KBYTES), 0)
		tools/mkrd/mkrd -out $@ -boot filesys.img -swap swap.img -fs user.img
else
		tools/mkrd/mkrd -out $@ -boot filesys.img -swap swap.img
endif

$(FSUTIL):
		cd tools/fsutil; $(MAKE)

clean:
		rm -f *~
		for dir in tools lib src sys/pic32; do $(MAKE) -C $$dir -k clean; done

cleanall:       clean
		$(MAKE) -C lib clean
		rm -f sys/pic32/*/unix.hex bin/* sbin/* games/[a-k]* games/[m-z]* libexec/* share/man/cat*/*
		rm -f games/lib/adventure.dat
		rm -f games/lib/cfscores
		rm -f share/re.help
		rm -f share/misc/more.help
		rm -f etc/termcap
		rm -rf share/unixbench

installfs: filesys.img
ifdef SDCARD
	sudo dd bs=10M if=sdcard.rd of=$(SDCARD)
else
	@echo "Error: No SDCARD defined."
endif
