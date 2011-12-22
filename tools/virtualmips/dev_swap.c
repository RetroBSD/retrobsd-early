/*
 * RAM-based disk for swap.
 *
 * Copyright (C) 2011 Igor Mokos
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
#include "mips_memory.h"
#include "device.h"
#include "dev_swap.h"

//#define TRACE       printf
#ifndef TRACE
#define TRACE(...)  /*empty*/
#endif

#define ADDR_MASK   (SWAP_BYTES - 1)

/*
 * Reset swap.
 */
void dev_swap_reset (cpu_mips_t *cpu)
{
    pic32_t *pic32 = (pic32_t*) cpu->vm->hw_data;

    pic32->swap.rd = 0;
    pic32->swap.wr = 0;
    pic32->swap.ldaddr = 0;
    pic32->swap.offset = 0;
}

/*
 * Initialize SD card.
 */
int dev_swap_init (swap_t *d, char *name)
{
    memset (d, 0, sizeof (*d));
    d->name = name;
    return (0);
}

void dev_swap_rd (cpu_mips_t *cpu, int on)
{
    pic32_t *pic32 = (pic32_t*) cpu->vm->hw_data;
    swap_t *d = &pic32->swap;

    if (on && ! d->rd) {
        TRACE ("swap: RD on\n");
        d->rd = 1;
        d->data = d->buf [d->offset];
    } else if (! on && d->rd) {
        TRACE ("swap: RD off\n");
        d->rd = 0;
        d->offset = (d->offset + 1) & ADDR_MASK;
    }
}

void dev_swap_wr (cpu_mips_t *cpu, int on)
{
    pic32_t *pic32 = (pic32_t*) cpu->vm->hw_data;
    swap_t *d = &pic32->swap;

    if (on && ! d->wr) {
        TRACE ("swap: WR on\n");
        d->wr = 1;
        d->buf [d->offset] = d->data;
    } else if (! on && d->wr) {
        TRACE ("swap: WR off\n");
        d->wr = 0;
        d->offset = (d->offset + 1) & ADDR_MASK;
    }
}

void dev_swap_ldaddr (cpu_mips_t *cpu, int on)
{
    pic32_t *pic32 = (pic32_t*) cpu->vm->hw_data;
    swap_t *d = &pic32->swap;

    if (on && ! d->ldaddr) {
        TRACE ("swap: LDADDR on\n");
        d->ldaddr = 1;
        d->offset >>= 8;
        d->offset |= d->data << 16;
    } else if (! on && d->ldaddr) {
        TRACE ("swap: LDADDR off\n");
        d->ldaddr = 0;
    }
}

/*
 * Data i/o: send byte to device.
 * Return received byte.
 */
unsigned dev_swap_io (cpu_mips_t *cpu, unsigned char newval, unsigned char rmask)
{
    pic32_t *pic32 = (pic32_t*) cpu->vm->hw_data;
    swap_t *d = &pic32->swap;

    if (rmask == 0) {
        /* Write mode. */
        TRACE ("swap: send %02x\n", newval);
        d->data = newval;
    } else
        TRACE ("swap: receive %02x\n", d->data);

    /* Read mode. */
    return d->data;
}
