/*
 * ADC driver for PIC32.
 *
 * Copyright (C) 2012 Majenko Technologies <matt@majenko.co.uk>
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
#include "adc.h"
#include "debug.h"

extern int uwritec(struct uio *);


int nactive = 0;

struct adc_info {
    int last_reading;
    unsigned char active;
};

struct adc_info adc[ADCMAX+1];

int adc_open(dev_t dev, int flag, int mode)
{
    int channel;

    channel = minor(dev);
    if(channel>ADCMAX)
        return ENODEV;

    DEBUG("adc%2: opened\n",channel);

    AD1PCFG &= ~(1<<channel);
    adc[channel].active = 1;
    if(nactive==0)
    {
        // Enable and configure the ADC here
        AD1CSSL = 0xFFFF;
        AD1CON2 = 0b0000010000111100;
        AD1CON3 = 0b0000011100000111;
        AD1CON1 = 0b1000000011100110;
	IPC(6) = 0b00000100000000000000000000000000;
        IECSET(1) = 1<<(PIC32_IRQ_AD1-32);
    }
    nactive++;
    return 0;
}

int adc_close(dev_t dev, int flag, int mode)
{
    int channel;

    channel = minor(dev);
    if(channel>ADCMAX)
        return ENODEV;

    AD1PCFG |= (1<<channel);
    adc[channel].active = 0;
    nactive--;
    if(nactive==0)
    {
        // Switch off the ADC here.
        AD1CSSL = 0x0000;
        AD1CON1 = 0x0000;
        asm volatile("NOP");
        IECCLR(1) = 1<<(PIC32_IRQ_AD1-32);
    }
    return 0;
}

// Return the most recent ADC value
int adc_read(dev_t dev, struct uio *uio, int flag)
{
    int channel;
    char temp[10];
    int c;
    int lr;
    int tv;

    channel = minor(dev);
    if(channel>ADCMAX)
        return ENODEV;

    lr = adc[channel].last_reading;
    c=0;
    if(lr >= 1000)
    {
        tv = lr/1000;
        temp[c++] = '0' + tv;
        lr = lr - (tv*1000);
    }
    if((lr >= 100) || (c>0))
    {
        tv = lr/100;
        temp[c++] = '0' + tv;
        lr = lr - (tv*100);
    }
    if((lr >= 10) || (c>0))
    {
        tv = lr/10;
        temp[c++] = '0' + tv;
        lr = lr - (tv*10);
    }
    temp[c++] = '0' + lr;
    temp[c++] = '\n';
    temp[c] = 0;

    uiomove(temp,strlen(temp), uio);

    return 0;
}

// Trigger an ADC conversion
int adc_write(dev_t dev, struct uio *uio, int flag)
{
    int channel;
    char c;

    channel = minor(dev);
    if(channel>ADCMAX)
        return ENODEV;

    c = uwritec(uio);
    while(uio->uio_iov->iov_len>0)
    {
        c = uwritec(uio);
    }
    return 0;
}

int adc_ioctl(dev_t dev, register u_int cmd, caddr_t addr, int flag)
{
    int unit;
    unit = minor(dev);

    switch(cmd)
    {
        default:
            return EINVAL;
    }

    return 0;
}

void adc_intr()
{
//    AD1CON1 &= ~(1<<1);

    adc[0].last_reading = ADC1BUF0;
    adc[1].last_reading = ADC1BUF1;
    adc[2].last_reading = ADC1BUF2;
    adc[3].last_reading = ADC1BUF3;
    adc[4].last_reading = ADC1BUF4;
    adc[5].last_reading = ADC1BUF5;
    adc[6].last_reading = ADC1BUF6;
    adc[7].last_reading = ADC1BUF7;
    adc[8].last_reading = ADC1BUF8;
    adc[9].last_reading = ADC1BUF9;
    adc[10].last_reading = ADC1BUFA;
    adc[11].last_reading = ADC1BUFB;
    adc[12].last_reading = ADC1BUFC;
    adc[13].last_reading = ADC1BUFD;
    adc[14].last_reading = ADC1BUFE;
    adc[15].last_reading = ADC1BUFF;
    IFSCLR(1) = 1<<(PIC32_IRQ_AD1-32);
}
