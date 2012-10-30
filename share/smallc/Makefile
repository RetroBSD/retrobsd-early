
.SUFFIXES: .c .s .o

.c.o:
	smallc $<
	as -o $@ $*.s

.s.o:
	as -o $@ $<

all:	primelist primesum test1 test2

primelist:	primelist.o lib.o sys.o
	ld -o primelist -e _main primelist.o lib.o sys.o

primesum:	primesum.o lib.o sys.o
	ld -o primesum -e _main primesum.o lib.o sys.o

test1:	test1.o lib.o sys.o
	ld -o $@ -e _main test1.o lib.o sys.o

test2:	test2.o lib.o sys.o
	ld -o $@ -e _main test2.o lib.o sys.o

clean:
	rm *.o primelist primesum test1 test2 primelist.s primesum.s test1.s test2.s
	
