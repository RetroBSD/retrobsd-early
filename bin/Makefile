#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
TOPSRC		= $(shell cd ..; pwd)
include $(TOPSRC)/target.mk

CFLAGS          += -Werror

# Programs that live in subdirectories, and have makefiles of their own.
#
SUBDIR		= ar-src as-src awk-src cc-src chflags-src chpass-src \
                  cpp-src dc-src diff-src find-src fstat-src hostname-src \
                  id-src la-src ld-src ls-src login-src make-src man-src \
                  more-src nm-src passwd-src portio-src printf-src ranlib-src \
                  re-src renice-src sed-src sh-src stty-src sysctl-src \
                  test-src uname-src xargs-src zmodem-src
# TODO: ccom-src lex-src m4-src yacc-src

# Shell scripts that need only be installed and are never removed.
#
SCRIPT		= false nohup true #lorder mkdep

# C programs that live in the current directory and do not need
# explicit make lines.
#
STD		= basename cal cat cb chgrp chmod cmp col comm cp date dd du \
		  echo ed fgrep file grep head hostid hostname join kill last ln \
                  mesg mkdir mv nice od pagesize pr pwd rev rm rmail rmdir \
                  size sleep sort split sum sync tail tar tee time touch tr \
                  tsort tty uniq w wc whereis who

# C programs that live in the current directory and need explicit make lines.
#
NSTD		= bc expr egrep

# Programs that must run setuid to root
#
SETUID		= mail su

# Programs that run set-group-id operator
#
OPERATOR	= df

# Programs that must run set-group-id kmem.
#
KMEM		= ps iostat vmstat

# Programs that must run set-group-id tty.
#
TTY		= wall write

# 'strip' is handled specially because 'install -s' now uses 'strip' and
# thus we can't do a 'install -s strip /bin/strip' without an error.
#
BINS		= ${STD} ${NSTD} ${SETUID} ${OPERATOR} ${KMEM} ${TTY} ${SCRIPT} strip

all:		${SUBDIR} ${BINS}

${SUBDIR}:	FRC
		cd $@; make ${MFLAGS}

FRC:

# ${STD} ${SETUID} ${KMEM} ${OPERATOR} ${TTY} strip
%: %.c
		${CC} ${CFLAGS} ${LDFLAGS} -o $@.elf $< ${LIBS}
		${OBJDUMP} -S $@.elf > $@.dis
		${SIZE} $@.elf
		${ELF2AOUT} $@.elf $@

# Files listed in ${NSTD} have explicit make lines given below.

# bc egrep expr
%.c: %.y
		${YACC} ${YFLAGS} $<
		-/bin/mv y.tab.c $@

${SCRIPT}:
		install -c $@.sh $@

install:	${BINS}
		install -s -m 751 -g staff ${STD} ${NSTD} ${DESTDIR}/bin
		-for i in ${SUBDIR}; do \
			(cd $$i; make ${MFLAGS} DESTDIR=${DESTDIR} install); done
		-for i in ${SCRIPT}; do (install -c $$i.sh ${DESTDIR}/bin/$$i); done
		-for i in ${SETUID}; do \
			(install -o root -m 4751 -s $$i ${DESTDIR}/bin/$$i); done
		-for i in ${OPERATOR}; do \
			(install -g operator -m 2751 -s $$i ${DESTDIR}/bin/$$i); done
		-for i in ${KMEM}; do \
			(install -g kmem -m 2751 -s $$i ${DESTDIR}/bin/$$i); done
		-for i in ${TTY}; do \
			(install -g tty -m 2751 -s $$i ${DESTDIR}/bin/$$i); done
		-install -s -m 751 -g staff strip ${DESTDIR}/bin/xstrip
		-/bin/mv ${DESTDIR}/bin/xstrip ${DESTDIR}/bin/strip
		/bin/rm -f ${DESTDIR}/bin/e; ln ${DESTDIR}/bin/ed ${DESTDIR}/bin/e

clean:
		/bin/rm -f ${BINS} expr.c a.out core *.s *.o *.dis *.elf *~ y.tab.c errs
		for i in ${SUBDIR}; do (cd $$i; make ${MFLAGS} clean); done

depend:		expr.c
		for i in ${BINS}; do \
		    cc -M ${INCPATH} $$i.c | sed 's/\.o//' | \
		    awk ' { if ($$1 != prev) \
			{ if (rec != "") print rec; rec = $$0; prev = $$1; } \
			else { if (length(rec $$2) > 78) { print rec; rec = $$0; } \
			else rec = rec " " $$2 } } \
			END { print rec } ' >> makedep; done
		echo '/^# DO NOT DELETE THIS LINE/+2,$$d' >eddep
		echo '$$r makedep' >>eddep
		echo 'w' >>eddep
		cp Makefile Makefile.bak
		ed - Makefile < eddep
		/bin/rm eddep makedep
		echo '# DEPENDENCIES MUST END AT END OF FILE' >> Makefile
		echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> Makefile
		echo '# see make depend above' >> Makefile

# DO NOT DELETE THIS LINE -- make depend uses it
