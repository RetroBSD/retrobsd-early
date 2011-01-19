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

#define DATA_WIDTH          32
#define LL

/* TODO */
#define JZ4740_INT_INDEX_MAX 0x7        /*0x1C/4 */
#define JZ4740_INT_TO_MIPS   0x2        /*jz4740 intc will issue int 2 to mips cpu */
#define INTC_ISR	( 0x00)
#define INTC_IMR	( 0x04)
#define INTC_IMSR	( 0x08)
#define INTC_IMCR	( 0x0c)
#define INTC_IPR	( 0x10)
#define IRQ_I2C		1
#define IRQ_UHC		3
#define IRQ_UART0	9
#define IRQ_SADC	12
#define IRQ_MSC		14
#define IRQ_RTC		15
#define IRQ_SSI		16
#define IRQ_CIM		17
#define IRQ_AIC		18
#define IRQ_ETH		19
#define IRQ_DMAC	20
#define IRQ_TCU2	21
#define IRQ_TCU1	22
#define IRQ_TCU0	23
#define IRQ_UDC 	24
#define IRQ_GPIO3	25
#define IRQ_GPIO2	26
#define IRQ_GPIO1	27
#define IRQ_GPIO0	28
#define IRQ_IPU		29
#define IRQ_LCD		30

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

    unsigned start_address;     /* jump here on reset */
    unsigned boot_flash_size;   /* size of boot flash in kbytes */
    unsigned boot_flash_address;        /* phys.address of boot flash */
    char *boot_file_name;       /* image of boot flash */
};

typedef struct pic32_system pic32_t;

vm_instance_t *create_instance (char *conf);
int init_instance (vm_instance_t * vm);
int pic32_reset (vm_instance_t * vm);
int dev_pic32_flash_init (vm_instance_t * vm, char *name,
    unsigned flash_kbytes, unsigned flash_address, char *filename);
#endif
