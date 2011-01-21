/*
 * Copyright (C) 2011 Serge Vakulenko <serge@vak.ru>
 *
 * This file is part of the virtualmips distribution.
 * See LICENSE file for terms of the license.
 */
#ifndef __PIC32_H__
#define __PIC32_H__

#include "types.h"
#include "pic32mx.h"

#define DATA_WIDTH          32          /* MIPS32 architecture */
#define LL                              /* empty - printf format for machine word */

/*
 * Data types
 */
typedef m_uint32_t m_va_t;
typedef m_uint32_t m_pa_t;
typedef m_uint32_t m_reg_t;
typedef m_int32_t m_ireg_t;
typedef m_uint32_t m_cp0_reg_t;

/*Guest endian*/
#define GUEST_BYTE_ORDER  ARCH_LITTLE_ENDIAN

/* Host to VM conversion functions */
#if HOST_BYTE_ORDER == GUEST_BYTE_ORDER
#define htovm16(x) (x)
#define htovm32(x) (x)
#define htovm64(x) (x)

#define vmtoh16(x) (x)
#define vmtoh32(x) (x)
#define vmtoh64(x) (x)
#else //host:big guest:little

#define htovm16(x) (ntohs(x))
#define htovm32(x) (ntohl(x))
#define htovm64(x) (swap64(x))

#define vmtoh16(x) (htons(x))
#define vmtoh32(x) (htonl(x))
#define vmtoh64(x) (swap64(x))
#endif

struct pic32_system {
    /* Associated VM instance */
    vm_instance_t *vm;

    unsigned start_address;         /* jump here on reset */
    unsigned boot_flash_size;       /* size of boot flash in kbytes */
    unsigned boot_flash_address;    /* phys.address of boot flash */
    char *boot_file_name;           /* image of boot flash */

    struct vdevice *intdev;         /* interrupt controller */
    unsigned intcon;                /* interrupt control */
    unsigned intstat;               /* interrupt status */
    unsigned iptmr;                 /* temporal proximity */
    unsigned ifs[3];                /* interrupt flag status */
    unsigned iec[3];                /* interrupt enable control */
    unsigned ipc[12];               /* interrupt priority control */
    unsigned ivprio[64];            /* priority of interrupt vectors */
};

typedef struct pic32_system pic32_t;
struct virtual_tty;

vm_instance_t *create_instance (char *conf);
int init_instance (vm_instance_t *vm);
int pic32_reset (vm_instance_t *vm);
void pic32_update_irq_flag (pic32_t *pic32);
int dev_pic32_flash_init (vm_instance_t *vm, char *name,
    unsigned flash_kbytes, unsigned flash_address, char *filename);
int dev_pic32_uart_init (vm_instance_t *vm, char *name, unsigned paddr,
    unsigned irq, struct virtual_tty *vtty);
int dev_pic32_intcon_init (vm_instance_t *vm, char *name, unsigned paddr);

#endif
