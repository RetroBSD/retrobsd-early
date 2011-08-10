/*
 * SD/MMC card emulation.
 *
 * Copyright (C) 2011 Serge Vakulenko <serge@vak.ru>
 *
 * This file is part of the virtualmips distribution.
 * See LICENSE file for terms of the license.
 *
 */
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "vm.h"
#include "mips64_memory.h"
#include "device.h"
#include "dev_sdcard.h"

//#define TRACE       printf
#ifndef TRACE
#define TRACE(...)  /*empty*/
#endif

/*
 * Definitions for MMC/SDC commands.
 */
#define CMD_GO_IDLE         (0x40+0)    /* CMD0 */
#define	CMD_SEND_OP_SDC		(0x40+41)   /* ACMD41 (SDC) */
#define CMD_SET_BLEN		(0x40+16)
#define CMD_SEND_CSD		(0x40+9)
#define CMD_READ_SINGLE		(0x40+17)
#define CMD_WRITE_SINGLE	(0x40+24)
#define CMD_APP             (0x40+55)   /* CMD55 */

static void sdcard_read_data (int fd, unsigned offset,
    unsigned char *buf, unsigned blen)
{
    /* Fill uninitialized blocks by FF: simulate real flash media. */
    memset (buf, 0xFF, blen);

    if (lseek (fd, offset, 0) != offset) {
        printf ("sdcard: seek failed, offset %u\n", offset);
        return;
    }
    if (read (fd, buf, blen) < 0) {
        printf ("sdcard: read failed, offset %u\n", offset);
        return;
    }
}

static void sdcard_write_data (int fd, unsigned offset,
    unsigned char *buf, unsigned blen)
{
    if (lseek (fd, offset, 0) != offset) {
        printf ("sdcard: seek failed, offset %u\n", offset);
        return;
    }
    if (write (fd, buf, blen) != blen) {
        printf ("sdcard: write failed, offset %u\n", offset);
        return;
    }
}

static void sdcard_reset (sdcard_t *d)
{
    d->select = 0;
    d->blen = 512;
    d->count = 0;
}

/*
 * Reset sdcard.
 */
void dev_sdcard_reset (cpu_mips_t *cpu)
{
    pic32_t *pic32 = (pic32_t*) cpu->vm->hw_data;

    sdcard_reset (&pic32->sdcard[0]);
    sdcard_reset (&pic32->sdcard[1]);
}

/*
 * Initialize SD card.
 */
int dev_sdcard_init (sdcard_t *d, char *name, unsigned mbytes, char *filename)
{
    memset (d, 0, sizeof (*d));
    d->name = name;
    if (mbytes == 0 || ! filename) {
        /* No SD card installed. */
        return (0);
    }
    d->kbytes = mbytes * 1024;
    d->fd = open (filename, O_RDWR);
    if (d->fd < 0) {
        /* Fatal: no image available. */
        perror (filename);
        return (-1);
    }
    return (0);
}

int dev_sdcard_detect (cpu_mips_t *cpu, int unit)
{
    pic32_t *pic32 = (pic32_t*) cpu->vm->hw_data;

    return (pic32->sdcard[unit].fd > 0);
}

int dev_sdcard_writable (cpu_mips_t *cpu, int unit)
{
    pic32_t *pic32 = (pic32_t*) cpu->vm->hw_data;

    return (pic32->sdcard[unit].fd > 0);
}

void dev_sdcard_select (cpu_mips_t *cpu, int unit, int on)
{
    pic32_t *pic32 = (pic32_t*) cpu->vm->hw_data;
    sdcard_t *d = &pic32->sdcard[unit];

    if (on) {
        TRACE ("sdcard%d: (((\n", unit);
        d->select = 1;
        d->count = 0;
    } else {
        TRACE ("sdcard%d: )))\n", unit);
        d->select = 0;
    }
}

/*
 * Data i/o: send byte to device.
 * Return received byte.
 */
unsigned dev_sdcard_io (cpu_mips_t *cpu, unsigned data)
{
    pic32_t *pic32 = (pic32_t*) cpu->vm->hw_data;
    sdcard_t *d = pic32->sdcard[0].select ? &pic32->sdcard[0] :
                  pic32->sdcard[1].select ? &pic32->sdcard[1] : 0;
    unsigned reply, offset;

    if (! d) {
        //printf ("sdcard: unselected i/o\n");
        return 0xFF;
    }
    data = (unsigned char) data;
    reply = 0xFF;
    if (d->count == 0) {
        d->buf[0] = data;
        d->count++;
    } else {
        switch (d->buf[0]) {
        case CMD_GO_IDLE:               /* CMD0: reset */
            if (d->count >= 7)
                break;
            d->buf [d->count++] = data;
            if (d->count == 7)
                reply = 0x01;
            break;
        case CMD_APP:                   /* CMD55: application prefix */
            if (d->count >= 7)
                break;
            d->buf [d->count++] = data;
            if (d->count == 7) {
                reply = 0;
                d->count = 0;
            }
            break;
        case CMD_SEND_OP_SDC:           /* ACMD41: initialization */
            if (d->count >= 7)
                break;
            d->buf [d->count++] = data;
            if (d->count == 7)
                reply = 0;
            break;
        case CMD_SET_BLEN:              /* Set block length */
            if (d->count >= 7)
                break;
            d->buf [d->count++] = data;
            if (d->count == 7) {
                d->blen = d->buf[1] << 24 | d->buf[2] << 16 |
                    d->buf[3] << 8 | d->buf[4];
                TRACE ("sdcard%d: set block length %u bytes\n", d->unit, d->blen);
                reply = (d->blen > 0 && d->blen <= 1024) ? 0 : 4;
            }
            break;
        case CMD_SEND_CSD:              /* Get card data */
            if (d->count >= 7)
                break;
            d->buf [d->count++] = data;
            if (d->count == 7) {
                /* Send reply */
                TRACE ("sdcard%d: send media size %u sectors\n",
                    d->unit, d->kbytes * 2);
                reply = 0;
                d->limit = 16 + 3;
                d->count = 1;
                d->buf[0] = 0;
                d->buf[1] = 0xFE;
                d->buf[2+0] = 1 << 6;     /* SDC ver 2.00 */
                d->buf[2+1] = 0;
                d->buf[2+2] = 0;
                d->buf[2+3] = 0;
                d->buf[2+4] = 0;
                d->buf[2+5] = 0;
                d->buf[2+6] = 0;
                d->buf[2+7] = 0;
                d->buf[2+8] = (d->kbytes / 512 - 1) >> 8;
                d->buf[2+9] = d->kbytes / 512 - 1;
                d->buf[2+10] = 0;
                d->buf[2+11] = 0;
                d->buf[2+12] = 0;
                d->buf[2+13] = 0;
                d->buf[2+14] = 0;
                d->buf[2+15] = 0;
                d->buf[d->limit - 1] = 0xFF;
                d->buf[d->limit] = 0xFF;
            }
            break;
        case CMD_READ_SINGLE:           /* Read block */
            if (d->count >= 7)
                break;
            d->buf [d->count++] = data;
            if (d->count == 7) {
                /* Send reply */
                reply = 0;
                offset = d->buf[1] << 24 | d->buf[2] << 16 |
                    d->buf[3] << 8 | d->buf[4];
                TRACE ("sdcard%d: read offset %u, length %u kbytes\n", d->unit, offset, d->blen);
                d->limit = d->blen + 3;
                d->count = 1;
                d->buf[0] = 0;
                d->buf[1] = 0xFE;
                sdcard_read_data (d->fd, offset, &d->buf[2], d->blen);
                d->buf[d->limit - 1] = 0xFF;
                d->buf[d->limit] = 0xFF;
            }
            break;
        case CMD_WRITE_SINGLE:          /* Write block */
            if (d->count >= sizeof (d->buf))
                break;
            d->buf [d->count++] = data;
            if (d->count == 7) {
                /* Accept command */
                reply = 0;
                offset = d->buf[1] << 24 | d->buf[2] << 16 |
                    d->buf[3] << 8 | d->buf[4];
                TRACE ("sdcard%d: write offset %u\n", d->unit, offset);
            } else if (d->count == 7 + d->blen + 2 + 2) {
                if (d->buf[7] == 0xFE) {
                    /* Accept data */
                    reply = 0x05;
                    offset = d->buf[1] << 24 | d->buf[2] << 16 |
                        d->buf[3] << 8 | d->buf[4];
                    sdcard_write_data (d->fd, offset, &d->buf[8], d->blen);
                    TRACE ("sdcard%d: write data, length %u kbytes\n", d->unit, d->blen);
                } else {
                    /* Reject data */
                    reply = 4;
                    TRACE ("sdcard%d: reject write data, tag=%02x\n", d->unit, d->buf[7]);
                }
            }
            break;
        case 0:                         /* Reply */
            if (d->count > d->limit)
                break;
            reply = d->buf [d->count++];
            break;
        default:                        /* Ignore */
            break;
        }
    }
    TRACE ("sdcard%d: send %02x, reply %02x\n", d->unit, data, reply);
    return reply;
}
