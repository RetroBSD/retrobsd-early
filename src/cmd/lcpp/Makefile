TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk
#include $(TOPSRC)/cross.mk

OBJS            = cpp.o lex.o nlist.o tokens.o macro.o eval.o \
                  include.o hideset.o getopt.o unix.o

LDFLAGS         += -g

CFLAGS		+= -Werror -Wall -Os
CFLAGS          += -DCPP_DEBUG -DGCC_COMPAT -DHAVE_CPP_VARARG_MACRO_GCC

all:            cpp

cpp:            $(OBJS)
		${CC} ${LDFLAGS} -o cpp.elf $(OBJS) ${LIBS}
		${OBJDUMP} -S cpp.elf > cpp.dis
		${SIZE} cpp.elf
		${ELF2AOUT} cpp.elf $@ && rm cpp.elf

clean:
		rm -f *.o *.0 *.elf cpp *.elf *.dis tags *~ lex.yy.c y.tab.[ch] tests/run*

install:        all
		install cpp $(DESTDIR)/bin/lcpp

$(OBJS):	cpp.h

test:
		./cpp.elf < tests/test1 > tests/run1
		cmp tests/run1 tests/res1
		./cpp.elf < tests/test2 > tests/run2
		cmp tests/run2 tests/res2
		./cpp.elf < tests/test3 > tests/run3
		cmp tests/run3 tests/res3
		./cpp.elf < tests/test4 > tests/run4
		cmp tests/run4 tests/res4
		./cpp.elf < tests/test5 > tests/run5
		cmp tests/run5 tests/res5
		./cpp.elf < tests/test6 > tests/run6
		cmp tests/run6 tests/res6
		./cpp.elf < tests/test7 > tests/run7
		cmp tests/run7 tests/res7
		./cpp.elf < tests/test8 > tests/run8
		cmp tests/run8 tests/res8
		./cpp.elf < tests/test9 > tests/run9
		cmp tests/run9 tests/res9
		./cpp.elf < tests/test10 > tests/run10
		cmp tests/run10 tests/res10
