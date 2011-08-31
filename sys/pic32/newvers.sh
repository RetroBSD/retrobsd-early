#!/bin/sh -
#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
echo `svnversion` ${USER-root} `pwd` `date +'%Y-%m-%d'` `hostname` | \
awk ' {
	version = $1;
        user = $2;
        dir = $3;
        date = $4;
	host = $5;
	printf "const char version[] = \"2.11 BSD Unix for PIC32, revision #%d:\\n", version;
	printf "     Compiled %s by %s@%s:\\n", date, user, host;
	printf "     %s\\n\";\n", dir;
}'
