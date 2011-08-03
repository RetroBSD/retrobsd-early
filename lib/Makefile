#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
TOPSRC		= $(shell cd ..; pwd)
include $(TOPSRC)/target.mk

# Programs that live in subdirectories, and have makefiles of their own.
#
SUBDIR		= startup-$(MACHINE) libc libutil

all:		${SUBDIR}

${SUBDIR}:	FRC
		cd $@; make ${MFLAGS}

FRC:

install:
		-for i in ${SUBDIR}; do \
			(cd $$i; make ${MFLAGS} DESTDIR=${DESTDIR} install); done

tags:
		cd libc; make ${MFLAGS} TAGSFILE=../${TAGSFILE} tags

clean:
		rm -f a.out core *.s *.o *.a *~
		-for i in ${SUBDIR}; do (cd $$i; make ${MFLAGS} clean); done
