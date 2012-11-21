TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/cross.mk

CFLAGS		+= -Werror -Wall

SRCS		= append.c ar.c archive.c contents.c delete.c extract.c misc.c \
                  move.c print.c replace.c strmode.c
OBJS            = append.o ar.o archive.o contents.o delete.o extract.o \
                  misc.o move.o print.o replace.o strmode.o

all:            mips-ar

mips-ar:        ${OBJS}
		${CC} ${LDFLAGS} -o $@ ${OBJS} ${LIBS}

clean:
		rm -f *.o mips-ar tags *~
