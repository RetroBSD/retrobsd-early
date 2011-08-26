#!/bin/sh -
#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)newvers.sh	1.6 (2.11BSD GTE) 11/26/94
#
if [ ! -r version ]; then echo 0 > version; fi
touch version
echo `cat version` ${USER-root} `pwd` `date +'%Y-%m-%d'` `hostname` | \
awk ' {
	version = $1 + 1; user = $2; dir = $3; date = $4;
	host = $5;
}
END {
	printf "const char version[] = \"2.11 BSD Unix for PIC32, build #%d:\\n", version;
	printf "     Compiled %s by %s@%s:\\n", date, user, host;
	printf "     %s\\n\";\n", dir;
	printf "%d\n", version > "version";
}' > vers.c
