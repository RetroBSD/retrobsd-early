#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	4.19.2 (2.11BSD) 1997/8/28
#
# Machine dependent routines are located in a subtree which parallels
# the top directories.  This subtree is identified by the machine name.
#
# Compatibility routines are kept in directories with a prefixing
# ``compat'' (so they all sort together).
#
# The C run-time startup code is always machine dependent and expected
# to be located in ${MACHINE}/csu
#
# All files contain sccsid strings, but these are not compiled into
# library objects by default, as a space-saving measure.  To produce
# a library that contains these strings in every object except
# system call stubs, add -DLIBC_SCCS to DEFS below; to put these
# strings into system call stubs, use -DSYSLIBC_SCCS.
#
# To compile a non-floating point versions of some standard library
# routines add -DNONFP.  This will speed up some operations if you don't
# have hardware floating point.  To compile a non-separate I&D version add
# -DNONSEPARATE.
#
DESTDIR		=
INSTALL		= install -m 644
MACHINE		= arch-mips
DEFS		=
ALL		= gen stdio stdlib string ${MACHINE}
TAGSFILE	= tags

libc.a:		${MACHINE}/csu ${ALL}
		rm -rf tmp
		mkdir tmp
		cd tmp; \
			for i in ${ALL}; do ar x ../$$i/$${i}lib; done; \
			ar cr ../libc.a `lorder *.o | tsort`
		ar ma findiop.o libc.a exit.o
		ar ma exit.o libc.a fakcu.o
		ar ma fltpr.o libc.a doprnt.o
		ar ma doprnt.o libc.a ffltpr.o
		rm -rf tmp

${MACHINE}/csu ${ALL}: FRC
		cd $@; make ${MFLAGS} DEFS=${DEFS}

FRC:

install:	libc.a
		${INSTALL} libc.a ${DESTDIR}/lib/libc.a
		ranlib ${DESTDIR}/lib/libc.a
		cd ${MACHINE}/csu; make DESTDIR=${DESTDIR} install

tags:
		for i in ${ALL}; do \
			(cd $$i; make ${MFLAGS} TAGSFILE=../${TAGSFILE} tags); \
		done

clean:
		for i in ${MACHINE}/csu ${ALL}; \
			do (cd $$i; make ${MFLAGS} clean); done
		rm -rf tmp

depend:
		for i in ${MACHINE}/csu ${ALL}; \
			do (cd $$i; make ${MFLAGS} DEFS=${DEFS} depend); done
