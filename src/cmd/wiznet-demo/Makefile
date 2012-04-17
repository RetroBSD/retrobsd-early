TOPSRC          = $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS          += -O -Wall -Werror
LIBS            += -lwiznet

EXAMPLES        = telnet-client \
                  udp-ntp-client \
                  chat-server \
                  web-client \
                  web-server

all:            $(EXAMPLES)

web-client:     web-client.o
		${CC} ${LDFLAGS} -o $@.elf $< ${LIBS}
		${OBJDUMP} -S $@.elf > $@.dis
		${SIZE} $@.elf
		${ELF2AOUT} $@.elf $@

web-server:     web-server.o
		${CC} ${LDFLAGS} -o $@.elf $< ${LIBS}
		${OBJDUMP} -S $@.elf > $@.dis
		${SIZE} $@.elf
		${ELF2AOUT} $@.elf $@

telnet-client:  telnet-client.o
		${CC} ${LDFLAGS} -o $@.elf $< ${LIBS}
		${OBJDUMP} -S $@.elf > $@.dis
		${SIZE} $@.elf
		${ELF2AOUT} $@.elf $@

udp-ntp-client: udp-ntp-client.o
		${CC} ${LDFLAGS} -o $@.elf $< ${LIBS}
		${OBJDUMP} -S $@.elf > $@.dis
		${SIZE} $@.elf
		${ELF2AOUT} $@.elf $@

chat-server:    chat-server.o
		${CC} ${LDFLAGS} -o $@.elf $< ${LIBS}
		${OBJDUMP} -S $@.elf > $@.dis
		${SIZE} $@.elf
		${ELF2AOUT} $@.elf $@

clean:
		rm -f *.o *.elf *.dis $(EXAMPLES)

install:        all
		install $(EXAMPLES) $(DESTDIR)/bin/
