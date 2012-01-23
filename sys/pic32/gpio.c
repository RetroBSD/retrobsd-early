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
#include "uio.h"

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
#define NGPIO           7               /* Ports A, B, C, D, E, F, G */
#define NPINS           16              /* Number of pins per port */

#define MINOR_CONF      0x40            /* Minor mask: /dev/confX */
#define MINOR_UNIT      0x07            /* Minor mask: unit number */

/*
 * Mask of configured pins, default empty.
 */
u_int gpio_confmask [NGPIO];

/*
 * To enable debug output, comment out the first line,
 * and uncomment the second line.
 */
#define PRINTDBG(...) /*empty*/
//#define PRINTDBG printf

/*
 * PIC32 port i/o registers.
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
	register struct gpioreg *reg = portnum + (struct gpioreg*) &TRISA;

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

static void
gpio_print (dev, buf)
	dev_t dev;
        char *buf;
{
	u_int unit = minor(dev) & MINOR_UNIT;
	register struct gpioreg *reg = unit + (struct gpioreg*) &TRISA;
        register u_int mask, conf, tris;
        register char c;

        conf = gpio_confmask [unit];
        tris = reg->tris;
        if (minor(dev) & MINOR_CONF) {
                /* /dev/confX device: port configuration mask */
                u_int odc = reg->odc;
                for (mask=1<<(NPINS-1); mask; mask>>=1) {
                        if (! (conf & mask))
                                c = '-';
                        else if (tris & mask)
                                c = 'i';
                        else
                                c = (odc & mask) ? 'd' : 'o';
                        *buf++ = c;
                }
        } else {
                /* /dev/portX device: port value mask */
                u_int lat = reg->lat;
                u_int port = reg->port;
                for (mask=1<<(NPINS-1); mask; mask>>=1) {
                        if (! (conf & mask))
                                c = '-';
                        else if (tris & mask)
                                c = (port & mask) ? '1' : '0';
                        else
                                c = (lat & mask) ? '1' : '0';
                        *buf++ = c;
                }
        }
        *buf++ = '\n';
        *buf = 0;
}

static void
gpio_parse (dev, buf)
	dev_t dev;
        char *buf;
{
	u_int unit = minor(dev) & MINOR_UNIT;
	register struct gpioreg *reg = unit + (struct gpioreg*) &TRISA;
        register u_int mask;
        register char c;

        if (minor(dev) & MINOR_CONF) {
                /* /dev/confX device: port configuration mask */
                for (mask=1<<(NPINS-1); mask; mask>>=1) {
                        c = *buf++;
                        if (c <= ' ' || c > '~')
                                break;
                        if (c == 'x' || c == 'X')
                                gpio_confmask [unit] &= ~mask;
                        else if (c == 'i' || c == 'I')
                                reg->trisset = mask;
                        else if (c == 'o' || c == 'O') {
                                reg->odcclr = mask;
                                reg->trisclr = mask;
                        } else if (c == 'd' || c == 'D') {
                                reg->odcset = mask;
                                reg->trisclr = mask;
                        }
                }
        } else {
                /* /dev/portX device: port value mask */
                u_int conf = gpio_confmask [unit];
                u_int tris = reg->tris;
                for (mask=1<<(NPINS-1); mask; mask>>=1) {
                        c = *buf++;
                        if (c <= ' ' || c > '~')
                                break;
                        if (! (conf & mask) || (tris & mask))
                                continue;
                        if (c == '0')
                                reg->latclr = mask;
                        else
                                reg->latset = mask;
                }
        }
}

int
gpioopen (dev, flag, mode)
	dev_t dev;
{
	register u_int unit = minor(dev) & MINOR_UNIT;

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
	register struct iovec *iov;
	register u_int cnt = NPINS + 1;
        char buf [20];

        /* I/o size should be large enough. */
        iov = uio->uio_iov;
	if (iov->iov_len < cnt)
	        return EIO;

        /* Read only cnt bytes. */
        if (uio->uio_offset >= cnt)
                return 0;
        cnt -= uio->uio_offset;

        /* Print port status to buffer. */
        gpio_print (dev, buf);
        //PRINTDBG ("gpioread -> %s", buf);

        bcopy (buf + uio->uio_offset, iov->iov_base, cnt);
        iov->iov_base += cnt;
        iov->iov_len -= cnt;
        uio->uio_resid -= cnt;
        uio->uio_offset += cnt;
	return 0;
}

int
gpiowrite (dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	register struct iovec *iov = uio->uio_iov;
	register u_int cnt = NPINS;
        char buf [20];

        /* I/o size should be large enough. */
	if (iov->iov_len < cnt)
	        return EIO;

        bcopy (iov->iov_base, buf, cnt);
        iov->iov_base += cnt;
        iov->iov_len -= cnt;
        uio->uio_resid -= cnt;
        uio->uio_offset += cnt;

        PRINTDBG ("gpiowrite ('%s')\n", buf);
        gpio_parse (dev, buf);
	return 0;
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
	register u_int unit, mask, value;
	register struct gpioreg *reg;

	if ((cmd & (IOC_INOUT | IOC_VOID)) != IOC_VOID ||
            ((cmd >> 8) & 0xff) != 'g')
		return EINVAL;
	unit = cmd & 0xff;
	if (unit >= NGPIO)
		return ENXIO;

        mask = gpio_filter ((u_int) addr & 0xffff, unit);
        reg = unit + (struct gpioreg*) &TRISA;
        PRINTDBG ("gpioioctl (cmd=%08x, addr=%08x, flag=%d)\n", cmd, addr, flag);

	if (cmd & GPIO_COMMAND & GPIO_CONFIN) {
                /* configure as input */
                PRINTDBG ("TRIS%cSET %p := %04x\n", unit+'A', &reg->trisset, mask);
                reg->trisset = mask;
                gpio_confmask [unit] |= mask;
        }
        if (cmd & GPIO_COMMAND & GPIO_CONFOUT) {
                /* configure as output */
                PRINTDBG ("ODC%cCLR %p := %04x\n", unit+'A', &reg->odcclr, mask);
                reg->odcclr = mask;
                PRINTDBG ("TRIS%cCLR %p := %04x\n", unit+'A', &reg->trisclr, mask);
                reg->trisclr = mask;
                gpio_confmask [unit] |= mask;
        }
        if (cmd & GPIO_COMMAND & GPIO_CONFOD) {
                /* configure as open drain */
                PRINTDBG ("ODC%cSET %p := %04x\n", unit+'A', &reg->odcset, mask);
                reg->odcset = mask;
                PRINTDBG ("TRIS%cCLR %p := %04x\n", unit+'A', &reg->trisclr, mask);
                reg->trisclr = mask;
                gpio_confmask [unit] |= mask;
        }
        if (cmd & GPIO_COMMAND & GPIO_DECONF) {
                /* deconfigure */
                gpio_confmask [unit] &= ~mask;
        }
        if (cmd & GPIO_COMMAND & GPIO_STORE) {
                /* store all outputs */
                value = reg->lat;
                PRINTDBG ("LAT%c %p -> %04x\n", unit+'A', &reg->lat, value);
                value &= ~gpio_confmask [unit];
                value |= mask & gpio_confmask [unit];
                PRINTDBG ("LAT%c %p := %04x\n", unit+'A', &reg->lat, value);
                reg->lat = value;
        }
        if (cmd & GPIO_COMMAND & GPIO_SET) {
                /* set to 1 by mask */
                mask &= gpio_confmask [unit];
                PRINTDBG ("LAT%cSET %p := %04x\n", unit+'A', &reg->latset, mask);
                reg->latset = mask;
        }
        if (cmd & GPIO_COMMAND & GPIO_CLEAR) {
                /* set to 0 by mask */
                mask &= gpio_confmask [unit];
                PRINTDBG ("LAT%cCLR %p := %04x\n", unit+'A', &reg->latclr, mask);
                reg->latclr = mask;
        }
        if (cmd & GPIO_COMMAND & GPIO_INVERT) {
                /* invert by mask */
                mask &= gpio_confmask [unit];
                PRINTDBG ("LAT%cINV %p := %04x\n", unit+'A', &reg->latinv, mask);
                reg->latinv = mask;
        }
        if (cmd & GPIO_COMMAND & GPIO_POLL) {
                /* send current pin values to user */
		value = reg->port;
                PRINTDBG ("PORT%c %p -> %04x\n", unit+'A', &reg->port, value);
		u.u_rval = value;
        }
	return 0;
}
