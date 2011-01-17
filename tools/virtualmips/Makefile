#
# Build VirtualMIPS simulator.
#
CC          = gcc -g
CFLAGS		= -Wall -MT $@ -MD -MP -MF .deps/$*.dep -I/opt/local/include
LIBS		= -L/opt/local/lib -L/usr/local/lib -lconfuse -lintl -lpthread -lelf

# Linux.
#LIBS		+= -lrt

# Optimization.
CFLAGS		+= -O
#CFLAGS		+= -O3 -fomit-frame-pointer


# Microchip PIC32.
CFLAGS		+= -DSIM_PIC32
PROG		= pic32
OBJS		= pic32.o pic32_dev_flash.o

# Ingenic JZ4740.
#CFLAGS		+= -DSIM_PAVO -D_USE_FDD_
#PROG		= pavo
#OBJS		= pavo.o jz4740.o jz4740_dev_cpm.o jz4740_dev_dma.o \
#              jz4740_dev_emc.o jz4740_dev_gpio.o jz4740_dev_int.o \
#              jz4740_dev_rtc.o jz4740_dev_ts.o \
#              jz4740_dev_uart.o jz4740_dev_wdt_tcu.o

# Implement LCD.
#CFLAGS		+= -DSIM_LCD
#LIBS		+= -lSDL
#OBJS		+= jz4740_dev_lcd.o vp_sdl.o

# Use JIT compiler.
#CFLAGS		+= -D_USE_JIT_
#OBJS		+= mips64_jit.o x86_trans.o

# Common files.
OBJS		+= dev_cs8900.o dev_nand_flash_1g.o dev_ram.o \
               dev_vtty.o device.o vm.o cpu.o mips64.o mips64_cp0.o \
               mips64_exec.o mips64_fdd.o crc.o mips64_hostalarm.o \
               mempool.o sbox.o utils.o vp_clock.o vp_timer.o net_io.o \
               mips64_memory.o debug.o gdb_interface.o main.o

all:		.deps $(PROG)

$(PROG):	$(OBJS)
			$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

.deps:
			@mkdir .deps

clean:
			rm -f *.o *~ *_log.txt pavo pic32

include .deps/*.dep
