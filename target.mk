DESTDIR		= /usr/local/lib/retrobsd
MACHINE		= mips
GCCBIN		= /usr/local/mips461/bin
CC		= $(GCCBIN)/mips-elf-gcc -EL -nostdinc -I$(TOPSRC)/include \
		  -I/usr/local/mips461/lib/gcc/mips-elf/4.6.1/include
CXX            	= $(GCCBIN)/mips-elf-g++ -EL -nostdinc -I$(TOPSRC)/include \
		  -I/usr/local/mips461/lib/gcc/mips-elf/4.6.1/include
AS		= $(CC) -x assembler-with-cpp
LD		= $(GCCBIN)/mips-elf-ld
AR		= $(GCCBIN)/mips-elf-ar
RANLIB		= $(GCCBIN)/mips-elf-ranlib
SIZE		= $(GCCBIN)/mips-elf-size
OBJDUMP		= $(GCCBIN)/mips-elf-objdump
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
