# MAXIMITE
# Console on UART4
DEFS            += -DCONSOLE_UART4
# SD/MMC card driver on SPI4
DEFS            += -DSD_PORT=SPI4CON
# /CS0 at pin E0
DEFS            += -DSD_CS0_PORT=TRISE -DSD_CS0_PIN=0
# LEDs at pins E1, F0 and opposite polarity to UBW32
DEFS            += -DLED_POLARITY
DEFS            += -DLED_DISK_PORT=TRISE   -DLED_DISK_PIN=1
DEFS            += -DLED_KERNEL_PORT=TRISF -DLED_KERNEL_PIN=0


