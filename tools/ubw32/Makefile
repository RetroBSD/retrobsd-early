CC              = gcc
EXECS           = ubw32
OBJS            = main.o hex.o
CFLAGS          = -O

ifeq ($(shell uname -s),Darwin)
# Rules for Mac OS X
OBJS            += usb-osx.o
LDFLAGS         = -Wl,-framework,IOKit,-framework,CoreFoundation
else
# Rules for Linux, etc.
OBJS            += usb-linux.o
LDFLAGS         = -lusb
endif

all:            $(EXECS)

ubw32:          $(OBJS)
		$(CC) $(OBJS) $(LDFLAGS) -o ubw32
		strip ubw32

# Must install as root; e.g. 'sudo make install'
install:        ubw32
		cp ubw32 /usr/local/bin

clean:
		rm -f $(EXECS) *.o core *~
###
*.o:            ubw32.h
