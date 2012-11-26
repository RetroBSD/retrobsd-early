TOPSRC          = $(shell cd ..; pwd)
SUBDIR		= startup libc
PROG		= ar as aout ld nm ranlib size strip

SYSINC = `cpp -v </dev/null 2>&1 | grep '^ ' | grep include | awk '{print "-I" $$1 " "}'`

CFLAGS		+= -nostdinc -g -Werror -Wall -DCROSS -I.  $(SYSINC) -I$(TOPSRC)/include \
                   -I$(TOPSRC)/src/cmd/ar -I$(TOPSRC)/src/cmd/as
LDFLAGS         += -g

AR_OBJS         = ar.o append.o archive.o contents.o delete.o extract.o \
                  misc.o move.o print.o replace.o strmode.o
AOUT_OBJS       = aout.o mips-dis.o
RANLIB_OBJS     = ranlib.o archive.o

vpath %.c $(TOPSRC)/src/cmd/ar $(TOPSRC)/src/cmd/as $(TOPSRC)/src/cmd/ld \
    $(TOPSRC)/src/cmd/nm $(TOPSRC)/src/cmd/ranlib $(TOPSRC)/src/cmd

all install depend: headers $(SUBDIR) $(PROG)
		-for i in $(SUBDIR); do $(MAKE) -C $$i $(MFLAGS) DESTDIR=$(DESTDIR) $@; done

clean:
		rm -f *~ *.o *.a $(PROG) retroImage
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

.PHONY: headers

headers:
	-ln -s ../include/a.out.h .
	-ln -s ../include/ar.h .
	-ln -s ../include/nlist.h .

