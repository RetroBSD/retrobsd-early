DESTDIR		= /usr/local/lib/retrobsd
MACHINE		= mips

# MPLABX C32 on Linux - not verified yet
ifneq (,$(wildcard /opt/microchip/mplabc32/v2.00/bin/pic32-gcc))
    GCCPREFIX	= /opt/microchip/mplabc32/v2.00/bin/pic32-
endif 

# MPLABX C32 on Mac OS X - not verified yet
ifneq (,$(wildcard /Applications/microchip/mplabc32/v2.00/bin/pic32-gcc))
    GCCPREFIX   = /Applications/microchip/mplabc32/v2.00/bin/pic32-
endif

# MPLABX C32 on Windows - not verified yet
ifneq (,$(wildcard /c/"Program Files"/Microchip/mplabc32/v2.00/bin/pic32-gcc))
    GCCPREFIX   = /c/"Program Files"/Microchip/mplabc32/v2.00/bin/pic32-
endif

# Plain GCC
ifneq (,$(wildcard /usr/local/mips461/bin/mips-elf-gcc))
    GCCPREFIX	= /usr/local/mips461/bin/mips-elf-
endif

CC		= $(GCCPREFIX)gcc -mips32r2 -EL -nostdinc -I$(TOPSRC)/include \
		  -I/usr/local/mips461/lib/gcc/mips-elf/4.6.1/include
CXX             = $(GCCPREFIX)g++ -mips32r2 -EL -nostdinc -I$(TOPSRC)/include \
		  -I/usr/local/mips461/lib/gcc/mips-elf/4.6.1/include
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

LDFLAGS		= -nostartfiles -fno-dwarf2-cfi-asm -T$(TOPSRC)/lib/elf32-mips.ld \
		  $(TOPSRC)/lib/crt0.o -L$(TOPSRC)/lib
LIBS		= -lc
