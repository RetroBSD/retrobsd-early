# This variable sets the board, for whit it should compile
BOARD=mega2560
DISPLAY=nokia3110

CFLAGS=-O3 -std=gnu99 -Wall
NATCFLAGS=-DBOARD_native
AVRCFLAGS=$(CFLAGS) -DAVRCPU -DF_CPU=16000000UL -mmcu=atmega2560
AVRCFLAGS+=-DBOARD_$(BOARD) -DDISPLAY_$(DISPLAY)
AVRCC=avr-gcc
AVROBJCOPY=avr-objcopy
IMAGE_BLOCK_SIZE=256
LIBRARY_PATH=../../../../library
#RETRO_MODULES+=$(LIBRARY_PATH)/dissect.rx
#RETRO_MODULES+=$(LIBRARY_PATH)/eval.rx
RETRO_MODULES+=$(LIBRARY_PATH)/struct.rx
RETRO_MODULES+=$(LIBRARY_PATH)/array.rx
RETRO_MODULES+=$(LIBRARY_PATH)/hash.rx
RETRO_MODULES+=$(LIBRARY_PATH)/stack.rx
RETRO_MODULES+=$(LIBRARY_PATH)/bad.rx
RETRO_MODULES+=$(LIBRARY_PATH)/bstrings.rx
RETRO_MODULES+=$(LIBRARY_PATH)/char.rx
RETRO_MODULES+=$(LIBRARY_PATH)/linkedList.rx
RETRO_MODULES+=$(LIBRARY_PATH)/combinators.rx
RETRO_MODULES+=$(LIBRARY_PATH)/decorator.rx
RETRO_MODULES+=$(LIBRARY_PATH)/math.rx
RETRO_MODULES+=$(LIBRARY_PATH)/variations.rx
#RETRO_MODULES+=$(LIBRARY_PATH)/values.rx
#RETRO_MODULES+=$(LIBRARY_PATH)/locals.rx
RETRO_MODULES+=arduino.rx
RETRO_MODULES+=$(BOARD).rx
RETRO_MODULES+=$(DISPLAY).rx
RETRO_MODULES+=keypad.rx
RETRO_MODULES+=main.rx
RETRO_MODULES+=finish.rx
CC=gcc
CP=cp
CAT=cat
AWK=awk
RM=rm
SED=gsed
HEXDUMP=hexdump

all: retro.orig retro.nat retro.hex

clean:
	$(RM) -f retro.orig retro.hex retro.out retro.nat
	$(RM) -f convert image.data image.hex.h
	$(RM) -f retroImage retroImage16 retroImage64

%.hex: %.out
	$(AVROBJCOPY) -O ihex -R .eeprom $< $@

%.out: %.c
	$(AVRCC) $(AVRCFLAGS) -o $@ $<

%.hex.h: %.data
	$(HEXDUMP) -v -e '"{" ${IMAGE_BLOCK_SIZE}/2 "%d," "},\n"' $< \
	    | $(SED) -e 's/,,* *},/}/' \
	    | $(AWK) ' \
	         BEGIN { curr = 0; } \
	         { print "const static prog_int16_t image_" curr "[${IMAGE_BLOCK_SIZE}] PROGMEM = " $$0 ";"; curr++; } \
		 END { \
		     print "#define IMAGE_CELLS " (${IMAGE_BLOCK_SIZE} * curr); \
		     print "static int16_t image_read(int16_t x) {"; \
		     print "  switch(x / ${IMAGE_BLOCK_SIZE}) {"; \
		     for (i = 0; i < curr; i++) { \
		       print "    case " i ": return pgm_read_word(&(image_" i "[x % ${IMAGE_BLOCK_SIZE}]));" \
		     }; \
		     print "  } return 0; }"; \
		 }' > $@

retro.orig: ../../../complete/retro.c
	$(CC) $(CFLAGS) -o $@ $<

retro.nat: retro.c image.hex.h
	$(CC) $(CFLAGS) $(NATCFLAGS) -o $@ $<

retro.hex: retro.out
retro.out: image.hex.h eeprom_atmel.h console_$(BOARD).h display_$(DISPLAY).h

convert: ../../../../tools/convert.c
	$(CC) -O -DRX16 -o $@ $<

retroImage: ../../../../retroImage retro.orig $(RETRO_MODULES)
	$(CP) $< $@ && $(CAT) $(RETRO_MODULES) | ./retro.orig
image.data: convert retroImage
	./convert && $(CP) retroImage16 image.data
