DESTDIR		= /usr/local
MACHINE		= mips

CC		= gcc

AS		= $(CC) -x assembler-with-cpp
LD		= ld
AR		= ar
RANLIB		= ranlib
SIZE		= size
OBJDUMP		= objdump
INSTALL		= install -m 644
INSTALLDIR	= install -m 755 -d
TAGSFILE	= tags
MANROFF		= nroff -man -h
ELF2AOUT	= cp

CFLAGS		= -O -DCROSS -I/usr/include #-I$(TOPSRC)/include

LDFLAGS		=

LIBS		=
