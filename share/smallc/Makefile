CC      = scc

all:	hello primelist primesum test1 test2 test3

hello:	hello.c
	$(CC) $(CFLAGS) -o $@ $(LDFLAGS) hello.c

primelist: primelist.c
	$(CC) $(CFLAGS) -o $@ $(LDFLAGS) primelist.c

primesum: primesum.c
	$(CC) $(CFLAGS) -o $@ $(LDFLAGS) primesum.c

test1:	test1.c
	$(CC) $(CFLAGS) -o $@ $(LDFLAGS) test1.c

test2:	test2.c
	$(CC) $(CFLAGS) -o $@ $(LDFLAGS) test2.c

test3:	test3.c
	$(CC) $(CFLAGS) -o $@ $(LDFLAGS) test3.c

clean:
	rm *.o primelist primesum test1 test2 primelist.s primesum.s test1.s test2.s
