# MPLABX C32 on Linux
ifneq (,$(wildcard /opt/microchip/mplabc32/v2.00/bin/pic32-gcc))
    GCCPREFIX   = /opt/microchip/mplabc32/v2.00/bin/pic32-
    LDFLAGS     = -Wl,--oformat=elf32-tradlittlemips
endif

# MPLABX C32 on Mac OS X
ifneq (,$(wildcard /Applications/microchip/mplabc32/v2.00/bin/pic32-gcc))
    GCCPREFIX   = /Applications/microchip/mplabc32/v2.00/bin/pic32-
    LDFLAGS     = -Wl,--oformat=elf32-tradlittlemips
endif

# MPLABX C32 on Windows - not verified yet
ifneq (,$(wildcard /c/"Program Files"/Microchip/mplabc32/v2.00/bin/pic32-gcc))
    GCCPREFIX   = /c/"Program Files"/Microchip/mplabc32/v2.00/bin/pic32-
    LDFLAGS     = -Wl,--oformat=elf32-tradlittlemips
    DEFS        += -D__MPLABX__
endif

# Digilent MPIDE on Linux
#ifneq (,$(wildcard /opt/mpide-0022-linux32-20110822/hardware/pic32/compiler/pic32-tools/bin/pic32-gcc))
#    GCCPREFIX   = /opt/mpide-0022-linux32-20110822/hardware/pic32/compiler/pic32-tools/bin/pic32-
#    LDFLAGS     = -Wl,--oformat=elf32-tradlittlemips
#endif

# chipKIT PIC32 compiler on Linux
ifneq (,$(wildcard /usr/local/pic32-tools/bin/pic32-gcc))
    GCCPREFIX   = /usr/local/pic32-tools/bin/pic32-
    LDFLAGS     = -Wl,--oformat=elf32-tradlittlemips
endif

# Plain GCC
ifneq (,$(wildcard /usr/local/mips461/bin/mips-elf-gcc))
ifndef GCCPREFIX
    GCCPREFIX   = /usr/local/mips461/bin/mips-elf-
    LDFLAGS     =
endif
endif

# chipKIT MPIDE on Mac OS X
ifneq (,$(wildcard /Applications/Mpide.app/Contents/Resources/Java/hardware/tools/avr))
    AVRDUDE     = /Applications/Mpide.app/Contents/Resources/Java/hardware/tools/avr/bin/avrdude \
                  -C /Applications/Mpide.app/Contents/Resources/Java/hardware/tools/avr/etc/avrdude.conf \
                  -P /dev/tty.usbserial-*
endif

# chipKIT MPIDE on Linux
ifneq (,$(wildcard /opt/mpide-0022-linux32-20110822/hardware/tools/avrdude))
    AVRDUDE     = /opt/mpide-0022-linux32-20110822/hardware/tools/avrdude \
                  -C /opt/mpide-0022-linux32-20110822/hardware/tools/avrdude.conf \
                  -P /dev/ttyUSB0
endif
