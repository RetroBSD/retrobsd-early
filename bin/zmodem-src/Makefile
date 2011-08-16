#
# Driver makefile for zmodem.  Calls makefile.generic.
#
TOPSRC		= $(shell cd ../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS		+= -Werror -DV7 -DMD=2 -DTXBSIZE=8192 -DNFGVMIN

SRCS		= rz.c sz.c
OBJS		= rz.o sz.o
MAN		= rz.0 sz.0
MANSRC		= rz.1 sz.1

all:            ../rz ../sz ${MAN}

../rz:          rz.o
		${CC} ${LDFLAGS} -o rz.elf $< ${LIBS}
		${OBJDUMP} -S rz.elf > rz.dis
		${SIZE} rz.elf
		${ELF2AOUT} rz.elf $@

../sz:          sz.o
		${CC} ${LDFLAGS} -o sz.elf $< ${LIBS}
		${OBJDUMP} -S sz.elf > sz.dis
		${SIZE} sz.elf
		${ELF2AOUT} sz.elf $@

rz.0:           rz.1
		${MANROFF} $< > $@

sz.0:           sz.1
		${MANROFF} $< > $@

clean:
		rm -f *.o *.elf ${MAN} ../rz ../sz *.elf *.dis tags *~

install: rz sz rz.0 sz.0
		install -c -s -m 755 rz ${DESTBIN}
		install -c -s -m 755 sz ${DESTBIN}
		install -c -m 444 rz.0 ${DESTMAN}
		install -c -m 444 -o bin -g bin sz.0 ${DESTMAN}
		rm -f $(DESTBIN)/sb $(DESTBIN)/sx
		ln $(DESTBIN)/sz $(DESTBIN)/sb
		ln $(DESTBIN)/sz $(DESTBIN)/sx
		rm -f $(DESTBIN)/rb $(DESTBIN)/rx
		ln $(DESTBIN)/rz $(DESTBIN)/rb
		ln $(DESTBIN)/rz $(DESTBIN)/rx

		rm -f $(DESTMAN)/sb.0 $(DESTMAN)/sx.0
		ln $(DESTMAN)/sz.0 $(DESTMAN)/sb.0
		ln $(DESTMAN)/sz.0 $(DESTMAN)/sx.0
		rm -f $(DESTMAN)/rb.0 $(DESTMAN)/rx.0
		ln $(DESTMAN)/rz.0 $(DESTMAN)/rb.0
		ln $(DESTMAN)/rz.0 $(DESTMAN)/rx.0
