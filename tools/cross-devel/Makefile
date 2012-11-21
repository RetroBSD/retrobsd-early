TOPSRC          = $(shell cd ../..; pwd)
SUBDIR		= startup
PROG		= ar as aout ld nm ranlib size strip

CFLAGS		+= -Werror -Wall -DCROSS -I/usr/include -I$(TOPSRC)/include
LDFLAGS         += -g

AR_OBJS         = ar.o append.o archive.o contents.o delete.o extract.o \
                  misc.o move.o print.o replace.o strmode.o
AOUT_OBJS       = aout.o mips-dis.o
RANLIB_OBJS     = ranlib.o archive.o

all install depend: $(SUBDIR) $(PROG)
		-for i in $(SUBDIR); do $(MAKE) -C $$i $(MFLAGS) DESTDIR=$(DESTDIR) $@; done

clean:
		rm -f *~ *.o $(PROG)
		for i in $(SUBDIR); do $(MAKE) -C $$i $(MFLAGS) clean; done

ar:             $(AR_OBJS)
		$(CC) $(LDFLAGS) -o $@ $(AR_OBJS) $(LIBS)

as:             as.o
		$(CC) $(LDFLAGS) -o $@ $< $(LIBS)

aout:           $(AOUT_OBJS)
		$(CC) $(LDFLAGS) -o $@ $(AOUT_OBJS) $(LIBS)

ld:             ld.o
		$(CC) $(LDFLAGS) -o $@ $< $(LIBS)

nm:             nm.o
		$(CC) $(LDFLAGS) -o $@ $< $(LIBS)

ranlib:         $(RANLIB_OBJS)
		$(CC) $(LDFLAGS) -o $@ $(RANLIB_OBJS) $(LIBS)

size:           size.o
		$(CC) $(LDFLAGS) -o $@ $< $(LIBS)

strip:          strip.o
		$(CC) $(LDFLAGS) -o $@ $< $(LIBS)

crt0.o:		crt0.c
		$(CC) $(CFLAGS) -c $< -o $@
