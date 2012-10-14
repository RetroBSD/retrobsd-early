
.SUFFIXES: .c .s .o

.c.o:
	smallc $<
	as -o $@ $*.s

.s.o:
	as -o $@ $<

all:	primelist primesum test1

primelist:	primelist.o lib.o sys.o
	ld -o primelist -e main primelist.o lib.o sys.o

primesum:	primesum.o lib.o sys.o
	ld -o primesum -e main primesum.o lib.o sys.o

test1:	test1.o lib.o sys.o
	ld -o $@ -e main test1.o lib.o sys.o

