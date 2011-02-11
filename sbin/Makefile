#
# Public domain - 1996/10/26 - sms
#
TOPSRC		= $(shell cd ..; pwd)
include $(TOPSRC)/target.mk

SUBDIR		= init mkfs newfs fsck #mknod mkpasswd mount \
		  reboot shutdown swapon tunefs umount

all:		${SUBDIR}

${SUBDIR}:	FRC
		cd $@; make ${MFLAGS} all

FRC:

install:	FRC
		-for i in ${SUBDIR}; do \
			(cd $$i; make ${MFLAGS} DESTDIR=${DESTDIR} install); done

clean:
		for i in ${SUBDIR}; do (cd $$i; make ${MFLAGS} clean); done
		rm -f *~
