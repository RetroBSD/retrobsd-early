#
# Public domain - 1996/10/26 - sms
#
TOPSRC		= $(shell cd ..; pwd)
include $(TOPSRC)/target.mk

SUBDIR		= init-src mkfs-src newfs-src fsck-src reboot-src
# mknod-src mkpasswd-src mount-src shutdown-src swapon-src tunefs-src umount-src

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
