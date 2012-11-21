#
# Version 2.2 January 24, 1998
#
TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/cross.mk

CFLAGS		+= -Werror -Wall -g
LDFLAGS		+= -g

SRCS		= ld.c
OBJS            = ld.o
MAN		= ld.0
MANSRC		= ld.1

all:            retrobsd-ld

retrobsd-ld:    ${OBJS}
		${CC} ${LDFLAGS} -o $@ ${OBJS} ${LIBS}

clean:
		rm -f *.o retrobsd-ld a.out tags *~
