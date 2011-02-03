/*
 * SPI emulation for PIC32.
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

#include "vm.h"
#include "mips64_memory.h"
#include "device.h"
#include "dev_sdcard.h"

/*
 * Reset sdcard.
 */
void dev_sdcard_reset (cpu_mips_t *cpu)
{
    pic32_t *pic32 = (pic32_t*) cpu->vm->hw_data;

    pic32->sdcard[0].read_offset = 0;
    pic32->sdcard[0].write_offset = 0;
    pic32->sdcard[0].addr_offset = 0;

    pic32->sdcard[1].read_offset = 0;
    pic32->sdcard[1].write_offset = 0;
    pic32->sdcard[1].addr_offset = 0;
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
    return 0;
}

int dev_sdcard_writable (cpu_mips_t *cpu, int unit)
{
    return 0;
}

void dev_sdcard_select (cpu_mips_t *cpu, int unit, int on)
{
}

unsigned dev_sdcard_read (cpu_mips_t *cpu)
{
    return 0xFF;
}

void dev_sdcard_write (cpu_mips_t *cpu, unsigned data)
{
}
