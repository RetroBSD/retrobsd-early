CFLAGS          = -O -DDEBUG -g

# For Linux of Mac OS X
CFLAGS          += -DTERMIOS

all:            re

# Вместо r.termc.o можно использовать и -ltermcap

OBJ             = r.cmd.o r.edit.o r.file.o r.gettc.o r.hlam.o r.init.o \
                  r.mac.o r.main.o r.tele.o r.termc.o r.ttyio.o r.wind.o

re:             $(OBJ)
		cc $(OBJ) -o $@

$(OBJ):         r.defs.h

install:        re
		chmod a-t ${DSTDIR}/bin/re
		-${DSTDIR}/bin/re </dev/null
		strip re
		cp re ${DSTDIR}/bin/re
		chmod 7111 ${DSTDIR}/bin/re

clean:
		rm -f *.o *~ re
