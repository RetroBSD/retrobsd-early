# makefile for building retro image and some vm implementations

CFLAGS = -Wall

all: clean retro

retro:
	$(CC) $(CFLAGS) vm/complete/retro.c -o retro

python:
	cp vm/complete/retro.py retro
	chmod +x retro

ruby:
	cp vm/complete/retro.rb retro
	chmod +x retro

sbcl:
	( cd vm/complete/lisp && sbcl --no-sysinit --no-userinit --noprint --load sbcl.lisp )

.PHONY: image

image: retro
	cd image && cat meta.rx kernel.rx >../core.rx
	./retro --shrink --image retroImage --with core.rx
	./retro --with vm/web/html5/dumpImage.rx
	mv retroImage.js vm/web/html5
	rm core.rx

clean:
	rm -f retro
	rm -f retroImage16 retroImage64
	rm -f retroImage16BE retroImageBE retroImage64BE
	rm -f convert

