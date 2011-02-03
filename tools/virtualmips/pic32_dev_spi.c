/*
 * SPI emulation for PIC32.
 * Two SD/MMC disks attached to SPI2.
 *
 * Copyright (C) 2011 Serge Vakulenko <serge@vak.ru>
 *
 * This file is part of the virtualmips distribution.
 * See LICENSE file for terms of the license.
 */
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#include "device.h"
#include "mips64_memory.h"
#include "pic32.h"
#include "cpu.h"
#include "vp_timer.h"
#include "dev_sdcard.h"

#define SPI_REG_SIZE    0x40

#define PINC_CS0        1
#define PINC_CD0        2
#define PINC_WE0        3
#define PINE_CS1        5
#define PINE_CD1        6
#define PINE_WE1        7

struct pic32_spi_data {
    struct vdevice  *dev;
    vtty_t          *vtty;
    vm_instance_t   *vm;
    pic32_t         *pic32;

    u_int           irq;            /* base irq number */
#define IRQ_FAULT   0               /* error interrupt */
#define IRQ_TX      1               /* transmitter interrupt */
#define IRQ_RX      2               /* receiver interrupt */

    unsigned        con;            /* 0x00 - Control */
    unsigned        stat;           /* 0x10 - Status */
    unsigned        buf;            /* 0x20 - Transmit and receive buffer */
    unsigned        brg;            /* 0x40 - Baud rate generator */
};

extern cpu_mips_t *current_cpu;

/*
 * Perform an assign/clear/set/invert operation.
 */
static inline unsigned write_op (a, b, op)
{
    switch (op & 0xc) {
    case 0x0:           /* Assign */
        a = b;
        break;
    case 0x4:           /* Clear */
        a &= ~b;
        break;
    case 0x8:           /* Set */
        a |= b;
        break;
    case 0xc:           /* Invert */
        a ^= b;
        break;
    }
    return a;
}

void *dev_pic32_spi_access (cpu_mips_t * cpu, struct vdevice *dev,
    m_uint32_t offset, u_int op_size, u_int op_type,
    m_reg_t * data, m_uint8_t * has_set_value)
{
    struct pic32_spi_data *d = dev->priv_data;
    unsigned newval;

    if (offset >= SPI_REG_SIZE) {
        *data = 0;
        return NULL;
    }
    if (op_type == MTS_READ)
        *data = 0;
    switch (offset & 0x1f0) {
    case PIC32_SPI1CON & 0x1f0:         /* SPIx Control */
        if (op_type == MTS_READ) {
            *data = d->con;
        } else {
            d->con = write_op (d->con, *data, offset);
            if (! (d->con & PIC32_SPICON_ON)) {
                d->vm->clear_irq (d->vm, d->irq + IRQ_FAULT);
                d->vm->clear_irq (d->vm, d->irq + IRQ_RX);
                d->vm->clear_irq (d->vm, d->irq + IRQ_TX);
                d->stat = PIC32_SPISTAT_SPITBE;
            }
        }
        break;

    case PIC32_SPI1STAT & 0x1f0:        /* SPIx Status */
        if (op_type == MTS_READ) {
            *data = d->stat;
        } else {
            /* Only ROV bit is writable. */
            newval = write_op (d->stat, *data, offset);
            d->stat = (d->stat & ~PIC32_SPISTAT_SPIROV) |
                (newval & PIC32_SPISTAT_SPIROV);
        }
        break;

    case PIC32_SPI1BUF & 0x1ff:         /* SPIx SPIx Buffer */
        if (op_type == MTS_READ) {
            if (dev->phys_addr == PIC32_SPI2CON)
                d->buf = dev_sdcard_read (cpu);
            *data = d->buf;
            if (d->stat & PIC32_SPISTAT_SPIRBF) {
                d->stat &= ~PIC32_SPISTAT_SPIRBF;
                //d->vm->clear_irq (d->vm, d->irq + IRQ_RX);
            }
        } else {
            d->buf = *data;
            if (dev->phys_addr == PIC32_SPI2CON)
                dev_sdcard_write (cpu, d->buf);
            if (d->stat & PIC32_SPISTAT_SPIRBF) {
                d->stat |= PIC32_SPISTAT_SPIROV;
                //d->vm->set_irq (d->vm, d->irq + IRQ_FAULT);
            } else {
                d->stat |= PIC32_SPISTAT_SPIRBF;
                //d->vm->set_irq (d->vm, d->irq + IRQ_RX);
            }
        }
        break;

    case PIC32_SPI1BRG & 0x1f0:         /* SPIx Baud rate */
        if (op_type == MTS_READ) {
            *data = d->brg;
        } else {
            d->brg = write_op (d->brg, *data, offset);
        }
        break;

    default:
        ASSERT (0, "unknown spi offset %x\n", offset);
    }
    *has_set_value = TRUE;
    return NULL;
}

void dev_pic32_spi_reset (cpu_mips_t *cpu, struct vdevice *dev)
{
    struct pic32_spi_data *d = dev->priv_data;

    d->con = 0;
    d->stat = PIC32_SPISTAT_SPITBE;             /* Transmit buffer is empty */
    d->buf = 0;
    d->brg = 0;
}

int dev_pic32_spi_init (vm_instance_t *vm, char *name, unsigned paddr,
    unsigned irq)
{
    struct pic32_spi_data *d;
    pic32_t *pic32 = (pic32_t *) vm->hw_data;

    /* allocate the private data structure */
    d = malloc (sizeof (*d));
    if (!d) {
        fprintf (stderr, "PIC32 SPI: unable to create device.\n");
        return (-1);
    }
    memset (d, 0, sizeof (*d));
    d->dev = dev_create (name);
    if (! d->dev) {
        free (d);
        return (-1);
    }
    d->dev->priv_data = d;
    d->dev->phys_addr = paddr;
    d->dev->phys_len = SPI_REG_SIZE;
    d->dev->flags = VDEVICE_FLAG_NO_MTS_MMAP;
    d->vm = vm;
    d->irq = irq;
    d->pic32 = pic32;
    d->dev->handler = dev_pic32_spi_access;
    d->dev->reset_handler = dev_pic32_spi_reset;

    vm_bind_device (vm, d->dev);
    return (0);
}
