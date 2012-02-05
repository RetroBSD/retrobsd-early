TOPSRC		= $(shell cd ../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS		+= -Werror

SRCS		= diff.c diffdir.c diffreg.c
OBJS		= diff.o diffdir.o diffreg.o

DIFF            = /bin/diff
DIFFH           = /libexec/diffh
PR              = /bin/pr
CFLAGS          += -DDIFF='"${DIFF}"' -DDIFFH='"${DIFFH}"' -DPR='"${PR}"'

all:            ../diff ../../libexec/diffh

../diff:        ${OBJS}
		${CC} ${LDFLAGS} -o diff.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S diff.elf > diff.dis
		${SIZE} diff.elf
		${ELF2AOUT} diff.elf $@

../../libexec/diffh: diffh.o
		${CC} ${LDFLAGS} -o diffh.elf $< ${LIBS}
		${OBJDUMP} -S diffh.elf > diffh.dis
		${SIZE} diffh.elf
		${ELF2AOUT} diffh.elf $@

clean:
		rm -f *.o *.elf ${MAN} ../diff ../../libexec/diffh *.elf *.dis tags *~

install:        all
		install -s diff ${DESTDIR}/bin/diff
		install -s ../../libexec/diffh ${DESTDIR}/usr/libexec/diffh
