#!/bin/sh -
#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
# Device "make" file.  Valid arguments:
#	std	standard devices
#	local	configuration specific devices
#	fd	file descriptor driver
# Disks:
#	rk*	unibus rk05
# Pseudo terminals:
#	pty*	set of 16 master and slave pseudo terminals

PATH=/etc:/sbin:/usr/sbin:/bin:/usr/bin
umask 77
for i
do
case $i in

std)
	mknod console	c 0 0
	mknod mem	c 1 0	; chmod 640 mem ; chgrp kmem mem
	mknod kmem	c 1 1	; chmod 640 kmem ; chgrp kmem kmem
	mknod null	c 1 2	; chmod 666 null
	mknod zero	c 1 3	; chmod 444 zero
	mknod tty	c 2 0	; chmod 666 tty
 	mknod klog	c 4 0	; chmod 600 klog
	mknod errlog	c 4 1	; chmod 600 errlog
	mknod acctlog	c 4 2	; chmod 600 acctlog
	;;

fd)
	umask 0
	rm -rf fd
	rm -f stdin stdout stderr
	mkdir fd
	chmod 755 fd
	mknod stdin c 5 0
	mknod stdout c 5 1
	mknod stderr c 5 2
	eval `echo "" | awk '{ for (i = 0; i < 32; i++)
			printf("mknod fd/%d c 5 %d; ",i,i); }'`
	;;

rk*)
	# The 2.11BSD rk driver doesn't support partitions.  We create
	# a single block and charater inode pair for each unit and
	# call it rkNh.
	umask 2 ; unit=`expr $i : '..\(.*\)'`
	case $i in
	rk*) name=rk; blk=0; chr=3;;
	esac
	mknod ${name}${unit}h b ${blk} ${unit}
	mknod r${name}${unit}h c ${chr} ${unit}
	chgrp operator ${name}${unit}h r${name}${unit}h
	chmod 640 ${name}${unit}h r${name}${unit}h
	;;

pty*)
	class=`expr $i : 'pty\(.*\)'`
	case $class in
	0) offset=0 name=p;;
	1) offset=16 name=q;;
	2) offset=32 name=r;;
	3) offset=48 name=s;;
	4) offset=64 name=t;;
	5) offset=80 name=u;;
	*) echo bad unit for pty in: $i;;
	esac
	case $class in
	0|1|2|3|4|5)
		umask 0
		eval `echo $offset $name | awk ' { b=$1; n=$2 } END {
			for (i = 0; i < 16; i++)
				printf("mknod tty%s%x c 11 %d; \
					mknod pty%s%x c 10 %d; ", \
					n, i, b+i, n, i, b+i); }'`
		umask 77
		;;
	esac
	;;

local)
	sh MAKEDEV.local
	;;
esac
done
