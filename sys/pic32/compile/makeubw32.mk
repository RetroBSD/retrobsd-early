# UBW32
# Console on UART1
DEFS            += -DCONSOLE_UART1
# SD/MMC card driver on SPI1
DEFS            += -DSD_PORT=SPI1CON
# /CS0 at pin A9 (and optional /CS1 at pin A10)
DEFS            += -DSD_CS0_PORT=TRISA -DSD_CS0_PIN=9
#DEFS            += -DSD_CS1_PORT=TRISA -DSD_CS1_PIN=10
# LEDs at pins E0, E1, E2, E3
DEFS            += -DLED_AUX_PORT=TRISE    -DLED_AUX_PIN=0
DEFS            += -DLED_DISK_PORT=TRISE   -DLED_DISK_PIN=1
DEFS            += -DLED_KERNEL_PORT=TRISE -DLED_KERNEL_PIN=2
DEFS            += -DLED_TTY_PORT=TRISE    -DLED_TTY_PIN=3


