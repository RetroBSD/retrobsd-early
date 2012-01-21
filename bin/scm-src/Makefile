CFLAGS          = -Wall -Wno-unused-function -DDEBUG -O
CC              = gcc -g

OBJS            = scm.o func.o
TESTS           = tests/*.scm

all:            scm

scm:            $(OBJS)
		$(CC) $(LDFLAGS) $(OBJS) $(LIBS) -o scm

tests:          scm $(TESTS) ALWAYS
		for i in $(TESTS); do \
                        base=`basename $$i .scm`;\
                        ./scm -v $$i > tests/$$base.out;\
                        diff tests/$$base.expected tests/$$base.out; done

expected:       scm $(TESTS)
		for i in $(TESTS); do \
                        base=`basename $$i .scm`;\
                        ./scm -v $$i > tests/$$base.expected; done

clean:;         rm -f *.o *.b .,* core.* core a.out scm tests/*.out

ALWAYS:

###
$(OBJS): scm.h
