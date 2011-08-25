DESTDIR		= /usr/local/lib/retrobsd
MACHINE		= mips

# MPLABX C32 on Linux - tested
# Fails due to missing stdarg.h and no linker support for elf32-littlemips target
ifneq (,$(wildcard /opt/microchip/mplabc32/v2.00/bin/pic32-gcc))
    GCCPREFIX	= /opt/microchip/mplabc32/v2.00/bin/pic32-
    LDFLAGS	= -Wl,--no-data-init
    INCLUDES    = -I/opt/microchip/mplabc32/v2.00/lib/gcc/pic32mx/4.5.1/include
endif

# MPLABX C32 on Mac OS X - not verified yet
ifneq (,$(wildcard /Applications/microchip/mplabc32/v2.00/bin/pic32-gcc))
    GCCPREFIX   = /Applications/microchip/mplabc32/v2.00/bin/pic32-
    LDFLAGS	= -Wl,--no-data-init
    INCLUDES    = -I/Applications/microchip/mplabc32/v2.00/lib/gcc/pic32mx/4.5.1/include
endif

# MPLABX C32 on Windows - not verified yet
ifneq (,$(wildcard /c/"Program Files"/Microchip/mplabc32/v2.00/bin/pic32-gcc))
    GCCPREFIX   = /c/"Program Files"/Microchip/mplabc32/v2.00/bin/pic32-
    LDFLAGS	= -Wl,--no-data-init
    INCLUDES    = -I/c/"Program Files"/microchip/mplabc32/v2.00/lib/gcc/pic32mx/4.5.1/include
endif

# Plain GCC
ifneq (,$(wildcard /usr/local/mips461/bin/mips-elf-gcc))
# Linux MPLABX C32 compiler doesn't support some functionality
# we require, so allow plain GCC to rule here for now
#ifndef GCCPREFIX
    GCCPREFIX	= /usr/local/mips461/bin/mips-elf-
    LDFLAGS	=
    INCLUDES    = -I/usr/local/mips461/lib/gcc/mips-elf/4.6.1/include
endif

CC		= $(GCCPREFIX)gcc -mips32r2 -EL -nostdinc -I$(TOPSRC)/include $(INCLUDES)
CXX             = $(GCCPREFIX)g++ -mips32r2 -EL -nostdinc -I$(TOPSRC)/include $(INCLUDES)
LD		= $(GCCPREFIX)ld
AR		= $(GCCPREFIX)ar
RANLIB          = $(GCCPREFIX)ranlib
SIZE            = $(GCCPREFIX)size
OBJDUMP         = $(GCCPREFIX)objdump -mmips:isa32r2
AS		= $(CC) -x assembler-with-cpp
YACC            = byacc
INSTALL		= install -m 644
INSTALLDIR	= install -m 755 -d
TAGSFILE	= tags
MANROFF		= nroff -man -h
ELF2AOUT	= $(TOPSRC)/tools/elf2aout/elf2aout

CFLAGS		= -O -g

LDFLAGS		+= -nostartfiles -fno-dwarf2-cfi-asm -T$(TOPSRC)/lib/elf32-mips.ld \
		   $(TOPSRC)/lib/crt0.o -L$(TOPSRC)/lib
LIBS		= -lc
