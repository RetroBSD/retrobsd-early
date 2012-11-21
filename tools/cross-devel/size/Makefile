TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/cross.mk

CFLAGS		+= -Werror -Wall -g
LDFLAGS		+= -g

SRCS		= size.c
OBJS            = size.o

all:            mips-size

mips-size:      ${OBJS}
		${CC} ${LDFLAGS} -o $@ ${OBJS} ${LIBS}

clean:
		rm -f *.o mips-size a.out tags *~
