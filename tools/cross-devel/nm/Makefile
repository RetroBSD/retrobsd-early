TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/cross.mk

CFLAGS		+= -Werror -Wall -I../ar

SRCS		= nm.c
OBJS		= nm.o

all:            mips-nm

mips-nm:        ${OBJS}
		${CC} ${LDFLAGS} -o $@ ${OBJS} ${LIBS}

clean:
		rm -f *.o mips-nm tags *~
