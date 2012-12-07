/*
 * UART driver for PIC32.
 *
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include "param.h"
#include "conf.h"
#include "user.h"
#include "ioctl.h"
#include "tty.h"
#include "systm.h"

#include "uart.h"
#include "usb_uart.h"

#define CONCAT(x,y) x ## y
#define BBAUD(x) CONCAT(B,x)

#ifndef CONSOLE_BAUD
#define CONSOLE_BAUD 115200
#endif

#define NKL     1                       /* Only one console device */

struct tty cnttys [1];

void cnstart (struct tty *tp);

static char *
port_name ()
{
#ifdef CONSOLE_UART1
        return "UART1";
#elif defined (CONSOLE_UART2)
        return "UART2";
#elif defined (CONSOLE_UART3)
        return "UART3";
#elif defined (CONSOLE_UART4)
        return "UART4";
#elif defined (CONSOLE_UART5)
        return "UART5";
#elif defined (CONSOLE_UART6)
        return "UART6";
#elif defined (CONSOLE_USB)
        return "USB";
#endif
        /* Cannot happen */
        return "???";
}

void cnidentify()
{
        printf ("console: port %s\n", port_name());
#if 0
	printf ("Config  = %08x\n", mips_read_c0_register (16, 0));
	printf ("Config1 = %08x\n", mips_read_c0_register (16, 1));
	printf ("Config2 = %08x\n", mips_read_c0_register (16, 2));
	printf ("Config3 = %08x\n", mips_read_c0_register (16, 3));
        printf ("IntCtl  = %08x\n", mips_read_c0_register (12, 1));
	printf ("SRSCtl  = %08x\n", mips_read_c0_register (12, 2));
	printf ("PRId    = %08x\n", mips_read_c0_register (15, 0));
#endif
}

/*ARGSUSED*/
int
cnopen (dev, flag, mode)
	dev_t dev;
{
#ifdef CONSOLE_UART1
	return uartopen(makedev(UART_MAJOR,0),flag,mode);
#endif
#ifdef CONSOLE_UART2
	return uartopen(makedev(UART_MAJOR,1),flag,mode);
#endif
#ifdef CONSOLE_UART3
	return uartopen(makedev(UART_MAJOR,2),flag,mode);
#endif
#ifdef CONSOLE_UART4
	return uartopen(makedev(UART_MAJOR,3),flag,mode);
#endif
#ifdef CONSOLE_UART5
	return uartopen(makedev(UART_MAJOR,4),flag,mode);
#endif
#ifdef CONSOLE_UART6
	return uartopen(makedev(UART_MAJOR,5),flag,mode);
#endif
#ifdef CONSOLE_USB
	return usbopen(makedev(USB_MAJOR,0),flag,mode);
#endif
	return(ENODEV);
}

/*ARGSUSED*/
int
cnclose (dev, flag, mode)
	dev_t dev;
{
#ifdef CONSOLE_UART1
	return uartclose(makedev(UART_MAJOR,0),flag,mode);
#endif
#ifdef CONSOLE_UART2
	return uartclose(makedev(UART_MAJOR,1),flag,mode);
#endif
#ifdef CONSOLE_UART3
	return uartclose(makedev(UART_MAJOR,2),flag,mode);
#endif
#ifdef CONSOLE_UART4
	return uartclose(makedev(UART_MAJOR,3),flag,mode);
#endif
#ifdef CONSOLE_UART5
	return uartclose(makedev(UART_MAJOR,4),flag,mode);
#endif
#ifdef CONSOLE_UART6
	return uartclose(makedev(UART_MAJOR,5),flag,mode);
#endif
#ifdef CONSOLE_USB
	return usbclose(makedev(USB_MAJOR,0),flag,mode);
#endif
	return(ENODEV);
}

/*ARGSUSED*/
int
cnread (dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
#ifdef CONSOLE_UART1
	return uartread(makedev(UART_MAJOR,0),uio,flag);
#endif
#ifdef CONSOLE_UART2
	return uartread(makedev(UART_MAJOR,1),uio,flag);
#endif
#ifdef CONSOLE_UART3
	return uartread(makedev(UART_MAJOR,2),uio,flag);
#endif
#ifdef CONSOLE_UART4
	return uartread(makedev(UART_MAJOR,3),uio,flag);
#endif
#ifdef CONSOLE_UART5
	return uartread(makedev(UART_MAJOR,4),uio,flag);
#endif
#ifdef CONSOLE_UART6
	return uartread(makedev(UART_MAJOR,5),uio,flag);
#endif
#ifdef CONSOLE_USB
	return usbread(makedev(USB_MAJOR,0),uio,flag);
#endif
	return(ENODEV);
}

/*ARGSUSED*/
int
cnwrite (dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
#ifdef CONSOLE_UART1
	return uartwrite(makedev(UART_MAJOR,0),uio,flag);
#endif
#ifdef CONSOLE_UART2
	return uartwrite(makedev(UART_MAJOR,1),uio,flag);
#endif
#ifdef CONSOLE_UART3
	return uartwrite(makedev(UART_MAJOR,2),uio,flag);
#endif
#ifdef CONSOLE_UART4
	return uartwrite(makedev(UART_MAJOR,3),uio,flag);
#endif
#ifdef CONSOLE_UART5
	return uartwrite(makedev(UART_MAJOR,4),uio,flag);
#endif
#ifdef CONSOLE_UART6
	return uartwrite(makedev(UART_MAJOR,5),uio,flag);
#endif
#ifdef CONSOLE_USB
	return usbwrite(makedev(USB_MAJOR,0),uio,flag);
#endif
	return(ENODEV);
}

int
cnselect (dev, rw)
	register dev_t dev;
	int rw;
{
#ifdef CONSOLE_UART1
	return uartselect(makedev(UART_MAJOR,0),rw);
#endif
#ifdef CONSOLE_UART2
	return uartselect(makedev(UART_MAJOR,1),rw);
#endif
#ifdef CONSOLE_UART3
	return uartselect(makedev(UART_MAJOR,2),rw);
#endif
#ifdef CONSOLE_UART4
	return uartselect(makedev(UART_MAJOR,3),rw);
#endif
#ifdef CONSOLE_UART5
	return uartselect(makedev(UART_MAJOR,4),rw);
#endif
#ifdef CONSOLE_UART6
	return uartselect(makedev(UART_MAJOR,5),rw);
#endif
#ifdef CONSOLE_USB
	return usbselect(makedev(USB_MAJOR,0),rw);
#endif
	return(ENODEV);
}

/*ARGSUSED*/
int
cnioctl (dev, cmd, addr, flag)
	dev_t dev;
	register u_int cmd;
	caddr_t addr;
{
#ifdef CONSOLE_UART1
	return uartioctl(makedev(UART_MAJOR,0),cmd,addr,flag);
#endif
#ifdef CONSOLE_UART2
	return uartioctl(makedev(UART_MAJOR,1),cmd,addr,flag);
#endif
#ifdef CONSOLE_UART3
	return uartioctl(makedev(UART_MAJOR,2),cmd,addr,flag);
#endif
#ifdef CONSOLE_UART4
	return uartioctl(makedev(UART_MAJOR,3),cmd,addr,flag);
#endif
#ifdef CONSOLE_UART5
	return uartioctl(makedev(UART_MAJOR,4),cmd,addr,flag);
#endif
#ifdef CONSOLE_UART6
	return uartioctl(makedev(UART_MAJOR,5),cmd,addr,flag);
#endif
#ifdef CONSOLE_USB
	return usbioctl(makedev(USB_MAJOR,0),cmd,addr,flag);
#endif
	return(ENODEV);
}

/*
 * Put a symbol on console terminal.
 */
void
cnputc (c)
	char c;
{
#ifdef CONSOLE_UART1
	uartputc(0,c);
#endif 
#ifdef CONSOLE_UART2
	uartputc(1,c);
#endif 
#ifdef CONSOLE_UART3
	uartputc(2,c);
#endif 
#ifdef CONSOLE_UART4
	uartputc(3,c);
#endif 
#ifdef CONSOLE_UART5
	uartputc(4,c);
#endif 
#ifdef CONSOLE_UART6
	uartputc(5,c);
#endif 
#ifdef CONSOLE_USB
	usbputc(c);
#endif 
}

/*
 * Receive a symbol from console terminal.
 */
int
cngetc ()
{
#ifdef CONSOLE_UART1
	return uartgetc(0);
#endif 
#ifdef CONSOLE_UART2
	return uartgetc(1);
#endif 
#ifdef CONSOLE_UART3
	return uartgetc(2);
#endif 
#ifdef CONSOLE_UART4
	return uartgetc(3);
#endif 
#ifdef CONSOLE_UART5
	return uartgetc(4);
#endif 
#ifdef CONSOLE_UART6
	return uartgetc(5);
#endif 
#ifdef CONSOLE_USB
	return usbgetc(0);
#endif 
}
