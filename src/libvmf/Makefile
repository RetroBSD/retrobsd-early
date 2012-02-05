# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.2.1 (2.11BSD) 1999/10/24
#
# vmf package
#
TOPSRC          = $(shell cd ../..; pwd)
include $(TOPSRC)/target.mk

CFILES=	vmf.c
OBJS=	vmf.o
POBJS=	../vmf.o

CTAGS=	ctags
#DEFS=	-DNOSCCS
#CFLAGS=	-O $(DEFS)
TAGSFILE=tags

#.c.o:
#	${CC} -c -p ${CFLAGS} $*.c
#	ld -x -r $*.o
#	mv a.out profiled/$*.o
#	${CC} -c ${CFLAGS} $*.c
#	ld -x -r $*.o
#	mv a.out $*.o

../libvmf.a ../libvmf_p.a: ${OBJS}
	@echo building profiled ../libvmf.a
	@cd profiled; $(AR) ru ../../libvmf_p.a ${POBJS}
	$(RANLIB) ../libvmf_p.a
	@echo building normal ../libvmf.a
	@$(AR) ru ../libvmf.a ${OBJS}
	$(RANLIB) ../libvmf.a

install: ../libvmf.a ../libvmf_p.a
	install -m 644 ../libvmf ${DESTDIR}/usr/lib/libvmf.a
	ranlib ${DESTDIR}/usr/lib/libvmf.a
	install -m 644 ../libvmf_p ${DESTDIR}/usr/lib/libvmf_p.a
	ranlib ${DESTDIR}/usr/lib/libvmf_p.a

tags:
	cwd=`pwd`; \
	for i in ${CFILES}; do \
		ctags -a -f ${TAGSFILE} $$cwd/$$i; \
	done

clean:
	rm -f ${OBJS} profiled/*.o a.out core test errs \
	    ../libvmf.a ../libvmf_p.a tags
