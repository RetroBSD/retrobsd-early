#	$Id: Makefile.in,v 1.22 2009/11/14 16:27:23 ragge Exp $
#
# Makefile.in for the cc part of pcc.
#

srcdir=.
top_srcdir=../..
top_builddir=../..
prefix = /usr/local
exec_prefix = /usr/local
bindir = ${exec_prefix}/bin
libdir = ${exec_prefix}/lib
libexecdir = ${exec_prefix}/libexec
includedir = ${prefix}/include
datarootdir = ${prefix}/share
mandir = ${datarootdir}/man
strip = yes
CC = gcc
EXEEXT = 
TARGOS = bsd
TARGMACH = mips
TARGET = mips-unknown-bsd
VERSION = 0.9.9
PCCLIBDIR = $(libdir)/pcc/$(TARGET)/$(VERSION)/lib
PCCINCDIR = $(libdir)/pcc/$(TARGET)/$(VERSION)/include
CFLAGS = -g -O2  -Wall -Wmissing-prototypes -Wstrict-prototypes -Wshadow -Wsign-compare
CPPFLAGS =  -DLIBEXECDIR=\"${libexecdir}/\" \
	 -DGCC_COMPAT -DINCLUDEDIR=\"${includedir}/\" \
	-DPCCINCDIR=\"${PCCINCDIR}/\" -DPCCLIBDIR=\"${PCCLIBDIR}/\" \
	-Dos_${TARGOS} -DTARGMACH=${TARGMACH} -Dmach_${TARGMACH} \
	-I${top_builddir} -I${top_srcdir}/os/${TARGOS} -I${MIPDIR} -I${MDIR}
LIBS = 
LDFLAGS = 
INSTALL = /usr/bin/install -c
INSTALL_PROGRAM = ${INSTALL}

OBJS=cc.o compat.o
DEST=mips-bsd-pcc$(EXEEXT)

MIPDIR=${top_srcdir}/mip
MDIR=${top_srcdir}/arch/$(TARGMACH)

all: ${DEST}

${DEST}: $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $@

.c.o:
	$(CC) $(DEFS) $(CPPFLAGS) $(CFLAGS) -c $<

compat.o: $(MIPDIR)/compat.c
	$(CC) $(DEFS) $(CPPFLAGS) $(CFLAGS) -c -o $@ $(MIPDIR)/compat.c

install:
	test -z "${DESTDIR}$(bindir)" || mkdir -p "${DESTDIR}$(bindir)"
	${INSTALL_PROGRAM} ${DEST} ${DESTDIR}${bindir}
	@if [ ${strip} = yes ]; then		\
		strip ${DESTDIR}${bindir}/${DEST} ;	\
		echo strip ${DESTDIR}${bindir}/${DEST} ;	\
	fi
	test -z "${DESTDIR}$(mandir)/man1" || mkdir -p "${DESTDIR}$(mandir)/man1"
	${INSTALL} $(srcdir)/cc.1 ${DESTDIR}${mandir}/man1/${DEST}.1
	test -z "${DESTDIR}$(PCCINCDIR)" || mkdir -p "${DESTDIR}$(PCCINCDIR)"
	test -z "${DESTDIR}$(PCCLIBDIR)" || mkdir -p "${DESTDIR}$(PCCLIBDIR)"

clean:
	/bin/rm -f  $(OBJS) ${DEST}

distclean: clean
	/bin/rm -f  Makefile
