#
# la Makefile.  Revised 1996/1/28
#
TOPSRC		= $(shell cd ../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS		+= -Werror

SRCS		= la.c
OBJS		= la.o
MAN		= la.0
MANSRC		= la.1

all:            ../la la.0

../la:          ${OBJS}
		${CC} ${LDFLAGS} -o la.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S la.elf > la.dis
		${SIZE} la.elf
		${ELF2AOUT} la.elf $@

la.0:           ${MANSRC}
		${MANROFF} ${MANSRC} > ${MAN}

clean:
		rm -f *.o *.elf ${MAN} ../la *.elf *.dis tags *~
