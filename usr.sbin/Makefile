TOPSRC		= $(shell cd ..; pwd)
include $(TOPSRC)/target.mk

SUBDIR          = chown-src chroot-src update-src
# getty-src pstat-src vipw-src

all:            ${SUBDIR}

${SUBDIR}:      FRC
		cd $@; make ${MFLAGS} SEPFLAG=${SEPFLAG} all

FRC:

install:        FRC
		-for i in ${SUBDIR}; do \
			(cd $$i; make ${MFLAGS} DESTDIR=${DESTDIR} install); done

clean:
		rm -f *~
		for i in ${SUBDIR}; do (cd $$i; make ${MFLAGS} clean); done
