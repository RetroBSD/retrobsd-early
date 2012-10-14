all:	primelist primesum

primelist:	primelist.o lib.o sys.o
	ld -o primelist -e main primelist.o lib.o sys.o

primesum:	primesum.o lib.o sys.o
	ld -o primesum -e main primesum.o lib.o sys.o

primelist.s:	primelist.c
	smallc primelist.c

primelist.o:	primelist.s
	as -o primelist.o primelist.s

primesum.s:	primesum.c
	smallc primesum.c

primesum.o:	primesum.s
	as -o primesum.o primesum.s

lib.s:	lib.c
	smallc lib.c

lib.o:	lib.s
	as -o lib.o lib.s

sys.o:	sys.s
	as -o sys.o sys.s


