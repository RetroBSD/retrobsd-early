BUILDPATH = ../../../tools/configsys/../../sys/pic32
H         = ../../../tools/configsys/../../sys/include
M         = ../../../tools/configsys/../../sys/pic32
S         = ../../../tools/configsys/../../sys/kernel

vpath %.c $(M):$(S)
vpath %.S $(M):$(S)

KERNOBJ += _startup.o adc.o clock.o cons.o devsw.o exception.o glcd.o gpio.o init_main.o init_sysent.o kern_clock.o kern_descrip.o kern_exec.o kern_exit.o kern_fork.o kern_glob.o kern_mman.o kern_proc.o kern_prot.o kern_prot2.o kern_resource.o kern_sig.o kern_sig2.o kern_subr.o kern_synch.o kern_sysctl.o kern_time.o machdep.o mem.o oc.o rd_sd.o rdisk.o signal.o spi_bus.o subr_log.o subr_prf.o subr_rmap.o swap.o sys_generic.o sys_inode.o sys_pipe.o sys_process.o syscalls.o sysctl.o tty.o tty_conf.o tty_subr.o tty_tty.o uart.o ufs_alloc.o ufs_bio.o ufs_bmap.o ufs_dsort.o ufs_fio.o ufs_inode.o ufs_mount.o ufs_namei.o ufs_subr.o ufs_syscalls.o ufs_syscalls2.o usb_device.o usb_function_cdc.o usb_uart.o vers.o vfs_vnops.o vm_sched.o vm_swap.o vm_swp.o 

DEFS += -DADC_ENABLED=YES
DEFS += -DBUS_KHZ=80000
DEFS += -DCONSOLE_UART2=YES
DEFS += -DCPU_KHZ=80000
DEFS += -DGLCD_ENABLED=YES
DEFS += -DGPIO_ENABLED=YES
DEFS += -DKERNEL
DEFS += -DLED_AUX_PIN=11
DEFS += -DLED_AUX_PORT=TRISB
DEFS += -DLED_DISK_PIN=9
DEFS += -DLED_DISK_PORT=TRISB
DEFS += -DLED_KERNEL_PIN=8
DEFS += -DLED_KERNEL_PORT=TRISB
DEFS += -DLED_TTY_PIN=10
DEFS += -DLED_TTY_PORT=TRISB
DEFS += -DNBUF=8
DEFS += -DOC_ENABLED=YES
DEFS += -DPIC32MX7
DEFS += -DSD0_CS_PIN=9
DEFS += -DSD0_CS_PORT=TRISG
DEFS += -DSD0_PORT=2
DEFS += -DUART2_BAUD=2400
DEFS += -DUART2_ENABLED=YES
DEFS += -DUARTUSB_BAUD=1000000
DEFS += -DUARTUSB_ENABLED=YES
DEFS += -DUCB_METER
DEFS += -DUSB_MAX_EP_NUMBER=3
DEFS += -DUSB_NUM_STRING_DESCRIPTORS=3

BLREBOOT = -p

LDSCRIPT = ../../../tools/configsys/../../sys/pic32/lds/bootloader.ld

include ../../../tools/configsys/../../sys/pic32/kernel-post.mk
