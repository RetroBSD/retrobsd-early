CC = gcc
CFLAGS = -Wall -O3 -fomit-frame-pointer -DUSE_SDL -I/usr/local/include/SDL
LIBS = -lSDL -Wl,-framework,Cocoa

default: retro
.PHONY: default

OFILES = loader.o ngaro.o devices.o vm.o
$(OFILES): %.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

retro: $(OFILES)
	$(CC) $(LIBS) $^ -o $@

clean:
	@rm -f retro *.o
