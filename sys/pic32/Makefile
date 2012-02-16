
# Programs that live in subdirectories, and have makefiles of their own.
#
SUBDIR		= baremetal dip duinomite explorer16 max32 maximite meb \
                  pinguino-micro starter-kit ubw32 ubw32-uart usbboot

default:

all:
		-for i in $(SUBDIR); do make -C $$i all; done

install:

clean:
		-for i in $(SUBDIR); do make -C $$i clean; done
		find .. -name \*~ | xargs rm -f
