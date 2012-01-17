/*
 * GPIO driver for PIC32.
 *
 * Copyright (C) 2012 Serge Vakulenko, <serge@vak.ru>
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appear in all
 * copies and that both that the copyright notice and this
 * permission notice and warranty disclaimer appear in supporting
 * documentation, and that the name of the author not be used in
 * advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * The author disclaim all warranties with regard to this
 * software, including all implied warranties of merchantability
 * and fitness.  In no event shall the author be liable for any
 * special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether
 * in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of
 * this software.
 */
#include "param.h"
#include "conf.h"
#include "user.h"
#include "ioctl.h"
#include "systm.h"

/*
 * Devices:
 *      /dev/porta ... /dev/portg
 *      /dev/confa ... /dev/confg
 *
 * Example:
 *      echo ....oiiid....iiii > /dev/confa
 *      echo ....1...0........ > /dev/porta
 *
 * Write to /dev/confX:
 *      'i' - configure the corresponding port pin as an input;
 *      'o' - configure the corresponding port pin as an output;
 *      'd' - configure the corresponding port pin as an open-drain output;
 *      'x' - deconfigure the corresponding port pin;
 *      '.' - no action.
 *
 * Write to /dev/portX:
 *      '0' - set output pin low;
 *      '1' - set output pin high;
 *      '+' - invert the value of output pin;
 *      '.' - no action.
 *
 * Use ioctl() on any of devices to control pins from the user program.
 *      ioctl(fd, GPIO_PORTA |  GPIO_CONFIN, mask)  - configure as input
 *      ioctl(fd, GPIO_PORTB |  GPIO_CONFOUT,mask)  - configure as output
 *      ioctl(fd, GPIO_PORTC |  GPIO_CONFOD, mask)  - configure as open drain
 *      ioctl(fd, GPIO_PORTD |  GPIO_DECONF, mask)  - deconfigure
 *      ioctl(fd, GPIO_PORTE |  GPIO_STORE,  val)   - set values of all pins
 *      ioctl(fd, GPIO_PORTF |  GPIO_SET,    mask)  - set to 1 by mask
 *      ioctl(fd, GPIO_PORTG |  GPIO_CLEAR,  mask)  - set to 0 by mask
 *      ioctl(fd, GPIO_PORT(0)| GPIO_INVERT, mask)  - invert by mask
 * val= ioctl(fd, GPIO_PORT(1)| GPIO_POLL,   0)     - get input values
 *
 * Several operations can be combined in one call.
 * For example, to toggle pin A2 high thew low, and get value
 * of all PORTA pins:
 * val = ioctl(fd, GPIO_PORTA | GPIO_SET | GPIO_CLEAR | GPIO_POLL, 1<<3);
 */
#define NGPIO   7                       /* Ports A, B, C, D, E, F, G */

/*
 * Mask of enabled pins, default empty.
 */
u_int gpio_confmask [NGPIO];

/*
 * PIC32 UART registers.
 */
struct gpioreg {
        volatile unsigned tris;		/* Mask of inputs */
        volatile unsigned trisclr;
        volatile unsigned trisset;
        volatile unsigned trisinv;
        volatile unsigned port;		/* Read inputs, write outputs */
        volatile unsigned portclr;
        volatile unsigned portset;
        volatile unsigned portinv;
        volatile unsigned lat;		/* Read/write outputs */
        volatile unsigned latclr;
        volatile unsigned latset;
        volatile unsigned latinv;
        volatile unsigned odc;		/* Open drain configuration */
        volatile unsigned odcclr;
        volatile unsigned odcset;
        volatile unsigned odcinv;
};

/*
 * If a port address matches a port number,
 * clear a given bit in pin mask.
 */
static u_int
filter_out (mask, portnum, portaddr, pin)
        register u_int mask;
        u_int portnum;
        volatile unsigned *portaddr;
{
	register struct gpioreg *reg = portnum + (struct gpioreg*) TRISA;

        if ((unsigned) reg == (unsigned) portaddr)
                mask &= ~(1 << pin);
        return mask;
}

/*
 * Some pins are used by other drivers.
 * Remove them from the mask.
 */
static u_int
gpio_filter (mask, portnum)
        u_int mask;
        u_int portnum;
{
#ifdef LED_AUX_PORT
        mask = filter_out (mask, portnum, &LED_AUX_PORT, LED_AUX_PIN);
#endif
#ifdef LED_DISK_PORT
        mask = filter_out (mask, portnum, &LED_DISK_PORT, LED_DISK_PIN);
#endif
#ifdef LED_KERNEL_PORT
        mask = filter_out (mask, portnum, &LED_KERNEL_PORT, LED_KERNEL_PIN);
#endif
#ifdef LED_TTY_PORT
        mask = filter_out (mask, portnum, &LED_TTY_PORT, LED_TTY_PIN);
#endif
#ifdef SD_CS0_PORT
        mask = filter_out (mask, portnum, &SD_CS0_PORT, SD_CS0_PIN);
#endif
#ifdef SD_CS1_PORT
        mask = filter_out (mask, portnum, &SD_CS1_PORT, SD_CS1_PIN);
#endif
#ifdef SD_ENA_PORT
        mask = filter_out (mask, portnum, &SD_ENA_PORT, SD_ENA_PIN);
#endif
#ifdef SW_DATA_PORT
        mask = filter_out (mask, portnum, &SW_DATA_PORT, SW_DATA_PIN);
#endif
#ifdef SW_LDA_PORT
        mask = filter_out (mask, portnum, &SW_LDA_PORT, SW_LDA_PIN);
#endif
#ifdef SW_RD_PORT
        mask = filter_out (mask, portnum, &SW_RD_PORT, SW_RD_PIN);
#endif
#ifdef SW_WR_PORT
        mask = filter_out (mask, portnum, &SW_WR_PORT, SW_WR_PIN);
#endif
        return mask;
}

int
gpioopen (dev, flag, mode)
	dev_t dev;
{
	int unit = minor(dev);

	if (unit >= NGPIO)
		return ENXIO;
	if (u.u_uid != 0)
		return EPERM;
	return 0;
}

int
gpioclose (dev, flag, mode)
	dev_t dev;
{
	return 0;
}

int
gpioread (dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
        // TODO
	return ENODEV;
}

int
gpiowrite (dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
        // TODO
	return ENODEV;
}

/*
 * Commands:
 * GPIO_CONFIN  - configure as input
 * GPIO_CONFOUT - configure as output
 * GPIO_CONFOD  - configure as open drain
 * GPIO_DECONF  - deconfigure
 * GPIO_STORE   - store all outputs
 * GPIO_SET     - set to 1 by mask
 * GPIO_CLEAR   - set to 0 by mask
 * GPIO_INVERT  - invert by mask
 * GPIO_POLL    - poll
 *
 * Use GPIO_PORT(n) to set port number.
 */
int
gpioioctl (dev, cmd, addr, flag)
	dev_t dev;
	register u_int cmd;
	caddr_t addr;
{
	register u_int portnum, op, mask;
	register struct gpioreg *reg;

	if ((cmd & (IOC_INOUT | IOC_VOID)) != IOC_VOID ||
            ((cmd >> 8) & 0xff) != 'g')
		return EINVAL;
	portnum = cmd & 0xff;
	if (portnum >= NGPIO)
		return ENXIO;

        op = cmd & ~(IOC_INOUT | IOC_VOID | 0xffff);
        mask = (u_int) addr & 0xffff;
        reg = portnum + (struct gpioreg*) TRISA;
printf ("gpioioctl (cmd=%08x, addr=%08x, flag=%d)\n", cmd, addr, flag);

	if (op & GPIO_CONFIN) {
                /* configure as input */
                mask = gpio_filter (mask, portnum);
                reg->trisset = mask;
                gpio_confmask [portnum] |= mask;
        }
        if (op & GPIO_CONFOUT) {
                /* configure as output */
                mask = gpio_filter (mask, portnum);
                reg->odcclr = mask;
                reg->trisclr = mask;
                gpio_confmask [portnum] |= mask;
        }
        if (op & GPIO_CONFOD) {
                /* configure as open drain */
                mask = gpio_filter (mask, portnum);
                reg->odcset = mask;
                reg->trisclr = mask;
                gpio_confmask [portnum] |= mask;
        }
        if (op & GPIO_DECONF) {
                /* deconfigure */
                gpio_confmask [portnum] &= ~mask;
        }
        if (op & GPIO_STORE) {
                /* store all outputs */
                reg->lat = (reg->lat & ~gpio_confmask [portnum]) |
                        (mask & gpio_confmask [portnum]);
        }
        if (op & GPIO_SET) {
                /* set to 1 by mask */
                reg->latset = mask & gpio_confmask [portnum];
        }
        if (op & GPIO_CLEAR) {
                /* set to 0 by mask */
                reg->latclr = mask & gpio_confmask [portnum];
        }
        if (op & GPIO_INVERT) {
                /* invert by mask */
                reg->latinv = mask & gpio_confmask [portnum];
        }
        if (op & GPIO_POLL) {
                /* send current pin values to user */
		u.u_rval = reg->port;
        }
	return 0;
}
