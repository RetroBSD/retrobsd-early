/*
 * SecureDigital flash card.
 *
 * Copyright (C) 2011 Serge Vakulenko <serge@vak.ru>
 *
 * This file is part of the virtualmips distribution.
 * See LICENSE file for terms of the license.
 */
#ifndef __DEV_SD__
#define __DEV_SD__

/* SD card private data */
struct sdcard {
    char *name;                         /* Device name */
    unsigned kbytes;                    /* Disk size */
    int fd;                             /* Image file */

    unsigned char wbuf [1024];
    unsigned read_offset;
    unsigned write_offset;
    unsigned addr_offset;

    /* for read */
    unsigned char *data_port_ipr;
};
typedef struct sdcard sdcard_t;

int dev_sdcard_init (sdcard_t *d, char *devname, unsigned mbytes, char *filename);
void dev_sdcard_reset (cpu_mips_t *cpu);
int dev_sdcard_detect (cpu_mips_t *cpu, int unit);
int dev_sdcard_writable (cpu_mips_t *cpu, int unit);
void dev_sdcard_select (cpu_mips_t *cpu, int unit, int on);
unsigned dev_sdcard_read (cpu_mips_t *cpu);
void dev_sdcard_write (cpu_mips_t *cpu, unsigned data);

#endif
