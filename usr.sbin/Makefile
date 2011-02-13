#
# Public domain - 1996/10/24 - sms
#
DESTDIR=
CFLAGS=	-O
SEPFLAG= -i

SUBDIR=	chown-src chroot-src getty-src pstat-src update-src vipw-src

# This is broken and doesn't compile - thus it is not included in SUBDIR above.
BROKEN=	trsp

all:	${SUBDIR}

${SUBDIR}: FRC
	cd $@; make ${MFLAGS} SEPFLAG=${SEPFLAG} all

FRC:

install: FRC
	-for i in ${SUBDIR}; do \
		(cd $$i; make ${MFLAGS} DESTDIR=${DESTDIR} install); done

clean:
	for i in ${SUBDIR}; do (cd $$i; make ${MFLAGS} clean); done
