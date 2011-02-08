GCCBIN		= /usr/local/mipsel441/bin
CC		= $(GCCBIN)/mipsel-elf32-gcc -nostdinc -I$(TOPSRC)/include \
		  -I/usr/local/mipsel441/lib/gcc/mipsel-elf32/4.4.1/include
AS		= $(CC) -x assembler-with-cpp
LD		= $(GCCBIN)/mipsel-elf32-ld -nostdlib
AR		= $(GCCBIN)/mipsel-elf32-ar
