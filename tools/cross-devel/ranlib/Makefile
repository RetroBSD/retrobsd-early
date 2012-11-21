TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/cross.mk

CFLAGS		+= -Werror -Wall -g -I. -I../ar
LDFLAGS		+= -g

SRCS            = ../ar/archive.c ranlib.c
OBJS            = archive.o ranlib.o

all:            mips-ranlib

mips-ranlib:    ${OBJS}
		${CC} ${LDFLAGS} -o $@ ${OBJS} ${LIBS}

archive.o:      ../ar/archive.c
		${CC} ${CFLAGS} -c -o $@ $<

clean:
		rm -f *.o mips-ranlib a.out tags *~
