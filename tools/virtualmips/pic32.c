/*
 * PIC32 emulation.
 *
 * Copyright (C) 2011 Serge Vakulenko <serge@vak.ru>
 *
 * This file is part of the virtualmips distribution.
 * See LICENSE file for terms of the license.
 */
#define _GNU_SOURCE
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <confuse.h>

#include "vp_lock.h"
#include "utils.h"
#include "mips64.h"
#include "vm.h"
#include "cpu.h"
#include "mips64_exec.h"
#include "debug.h"

#include "pic32.h"
#include "device.h"
#include "mips64_jit.h"

/*
 * Translate IRQ number to interrupt vector.
 */
static const int irq_to_vector[] = {
    PIC32_VECT_CT,      /* 0  - Core Timer Interrupt */
    PIC32_VECT_CS0,     /* 1  - Core Software Interrupt 0 */
    PIC32_VECT_CS1,     /* 2  - Core Software Interrupt 1 */
    PIC32_VECT_INT0,    /* 3  - External Interrupt 0 */
    PIC32_VECT_T1,      /* 4  - Timer1 */
    PIC32_VECT_IC1,     /* 5  - Input Capture 1 */
    PIC32_VECT_OC1,     /* 6  - Output Compare 1 */
    PIC32_VECT_INT1,    /* 7  - External Interrupt 1 */
    PIC32_VECT_T2,      /* 8  - Timer2 */
    PIC32_VECT_IC2,     /* 9  - Input Capture 2 */
    PIC32_VECT_OC2,     /* 10 - Output Compare 2 */
    PIC32_VECT_INT2,    /* 11 - External Interrupt 2 */
    PIC32_VECT_T3,      /* 12 - Timer3 */
    PIC32_VECT_IC3,     /* 13 - Input Capture 3 */
    PIC32_VECT_OC3,     /* 14 - Output Compare 3 */
    PIC32_VECT_INT3,    /* 15 - External Interrupt 3 */
    PIC32_VECT_T4,      /* 16 - Timer4 */
    PIC32_VECT_IC4,     /* 17 - Input Capture 4 */
    PIC32_VECT_OC4,     /* 18 - Output Compare 4 */
    PIC32_VECT_INT4,    /* 19 - External Interrupt 4 */
    PIC32_VECT_T5,      /* 20 - Timer5 */
    PIC32_VECT_IC5,     /* 21 - Input Capture 5 */
    PIC32_VECT_OC5,     /* 22 - Output Compare 5 */
    PIC32_VECT_SPI1,    /* 23 - SPI1 Fault */
    PIC32_VECT_SPI1,    /* 24 - SPI1 Transfer Done */
    PIC32_VECT_SPI1,    /* 25 - SPI1 Receive Done */
    PIC32_VECT_U1,      /* 26 - UART1 Error */
    PIC32_VECT_U1,      /* 27 - UART1 Receiver */
    PIC32_VECT_U1,      /* 28 - UART1 Transmitter */
    PIC32_VECT_I2C1,    /* 29 - I2C1 Bus Collision Event */
    PIC32_VECT_I2C1,    /* 30 - I2C1 Slave Event */
    PIC32_VECT_I2C1,    /* 31 - I2C1 Master Event */
    PIC32_VECT_CN,      /* 32 - Input Change Interrupt */
    PIC32_VECT_AD1,     /* 33 - ADC1 Convert Done */
    PIC32_VECT_PMP,     /* 34 - Parallel Master Port */
    PIC32_VECT_CMP1,    /* 35 - Comparator Interrupt */
    PIC32_VECT_CMP2,    /* 36 - Comparator Interrupt */
    PIC32_VECT_SPI2,    /* 37 - SPI2 Fault */
    PIC32_VECT_SPI2,    /* 38 - SPI2 Transfer Done */
    PIC32_VECT_SPI2,    /* 39 - SPI2 Receive Done */
    PIC32_VECT_U2,      /* 40 - UART2 Error */
    PIC32_VECT_U2,      /* 41 - UART2 Receiver */
    PIC32_VECT_U2,      /* 42 - UART2 Transmitter */
    PIC32_VECT_I2C2,    /* 43 - I2C2 Bus Collision Event */
    PIC32_VECT_I2C2,    /* 44 - I2C2 Slave Event */
    PIC32_VECT_I2C2,    /* 45 - I2C2 Master Event */
    PIC32_VECT_FSCM,    /* 46 - Fail-Safe Clock Monitor */
    PIC32_VECT_RTCC,    /* 47 - Real-Time Clock and Calendar */
    PIC32_VECT_DMA0,    /* 48 - DMA Channel 0 */
    PIC32_VECT_DMA1,    /* 49 - DMA Channel 1 */
    PIC32_VECT_DMA2,    /* 50 - DMA Channel 2 */
    PIC32_VECT_DMA3,    /* 51 - DMA Channel 3 */
    -1,                 /* 52 */
    -1,                 /* 53 */
    -1,                 /* 54 */
    -1,                 /* 55 */
    PIC32_VECT_FCE,     /* 56 - Flash Control Event */
    PIC32_VECT_USB,     /* 57 - USB */
};

/* Initialize the PIC32 Platform (MIPS) */
static int pic32_init_platform (pic32_t *pic32)
{
    struct vm_instance *vm = pic32->vm;
    cpu_mips_t *cpu0;
    void *(*cpu_run_fn) (void *);

    vm_init_vtty (vm);

    /* Create a CPU group */
    vm->cpu_group = cpu_group_create ("System CPU");

    /* Initialize the virtual MIPS processor */
    cpu0 = cpu_create (vm, CPU_TYPE_MIPS32, 0);
    if (! cpu0) {
        vm_error (vm, "unable to create CPU0!\n");
        return (-1);
    }
    /* Add this CPU to the system CPU group */
    cpu_group_add (vm->cpu_group, cpu0);
    vm->boot_cpu = cpu0;

    /* create the CPU thread execution */
    cpu_run_fn = (void *) mips64_exec_run_cpu;
    if (pthread_create (&cpu0->cpu_thread, NULL, cpu_run_fn, cpu0) != 0) {
        fprintf (stderr, "cpu_create: unable to create thread for CPU%u\n",
            0);
        free (cpu0);
        return (-1);
    }
    /* 32-bit address */
    cpu0->addr_bus_mask = 0xffffffff;

    /* Initialize RAM */
    vm_ram_init (vm, 0x00000000ULL);

    /* Initialize two flash areas */
    if (vm->flash_size != 0)
        if (dev_pic32_flash_init (vm, "Program flash", vm->flash_size,
                vm->flash_address, vm->flash_filename) == -1)
            return (-1);
    if (pic32->boot_flash_size != 0)
        if (dev_pic32_flash_init (vm, "Boot flash", pic32->boot_flash_size,
                pic32->boot_flash_address, pic32->boot_file_name) == -1)
            return (-1);
    if (dev_pic32_uart_init (vm, "PIC32 UART1", PIC32_U1MODE,
            PIC32_IRQ_U1E, vm->vtty_con1) == -1)
        return (-1);
    if (dev_pic32_uart_init (vm, "PIC32 UART2", PIC32_U2MODE,
            PIC32_IRQ_U2E, vm->vtty_con2) == -1)
        return (-1);
    if (dev_pic32_intcon_init (vm, "PIC32 INTCON", PIC32_INTCON) == -1)
        return (-1);
    if (dev_pic32_spi_init (vm, "PIC32 SPI1", PIC32_SPI1CON,
            PIC32_IRQ_SPI1E) == -1)
        return (-1);
    if (dev_pic32_spi_init (vm, "PIC32 SPI2", PIC32_SPI2CON,
            PIC32_IRQ_SPI2E) == -1)
        return (-1);
    if (dev_pic32_gpio_init (vm, "PIC32 GPIO", PIC32_TRISA) == -1)
        return (-1);
    if (dev_sdcard_init (&pic32->sdcard[0], "SD Card 0", pic32->sdcard0_size,
            pic32->sdcard0_file_name) < 0)
        return (-1);
    if (dev_sdcard_init (&pic32->sdcard[1], "SD Card 1", pic32->sdcard1_size,
            pic32->sdcard1_file_name) < 0)
        return (-1);
    return (0);
}

/*
 * Find pending interrupt with the biggest priority.
 * Setup INTSTAT and cause registers.
 * Update irq_pending flag for CPU.
 */
void pic32_update_irq_flag (pic32_t *pic32)
{
    cpu_mips_t *cpu = pic32->vm->boot_cpu;
    int vector, level, irq, n, v;

    /* Assume no interrupts pending. */
    cpu->irq_cause = 0;
    cpu->irq_pending = 0;
    pic32->intstat = 0;

    if ((pic32->ifs[0] & pic32->iec[0]) ||
        (pic32->ifs[1] & pic32->iec[1]) ||
        (pic32->ifs[2] & pic32->iec[2]))
    {
        /* Find the most prioritive pending interrupt,
         * it's vector and level. */
        vector = 0;
        level = 0;
        for (irq=0; irq<sizeof(irq_to_vector)/sizeof(int); irq++) {
            n = irq >> 5;
            if ((pic32->ifs[n] & pic32->iec[n]) >> (irq & 31) & 1) {
                /* Interrupt is pending. */
                v = irq_to_vector [irq];
                if (v < 0)
                    continue;
                if (pic32->ivprio[v] > level) {
                    vector = v;
                    level = pic32->ivprio[v];
                }
            }
        }
        pic32->intstat = vector | (level << 8);

        cpu->irq_cause = level << 10;
/*printf ("-- vector = %d, level = %d\n", vector, level);*/
    }
/*else printf ("-- no irq pending\n");*/

    mips64_update_irq_flag (cpu);
}

void pic32_clear_irq (vm_instance_t *vm, u_int irq)
{
    pic32_t *pic32 = (pic32_t*) vm->hw_data;

    /* Clear interrupt flag status */
    pic32->ifs [irq >> 5] &= ~(1 << (irq & 31));

    pic32_update_irq_flag (pic32);
}

void pic32_set_irq (vm_instance_t *vm, u_int irq)
{
    pic32_t *pic32 = (pic32_t*) vm->hw_data;

    /* Set interrupt flag status */
    pic32->ifs [irq >> 5] |= 1 << (irq & 31);

    pic32_update_irq_flag (pic32);
}

/*
 * Activate core timer interrupt
 */
void set_timer_irq (cpu_mips_t *cpu)
{
    pic32_set_irq (cpu->vm, PIC32_VECT_CT);
}

/*
 * Clear core timer interrupt
 */
void clear_timer_irq (cpu_mips_t *cpu)
{
    pic32_clear_irq (cpu->vm, PIC32_VECT_CT);
}

COMMON_CONFIG_INFO_ARRAY;

static void pic32_parse_configure (pic32_t *pic32)
{
    vm_instance_t *vm = pic32->vm;
    char *start_address = 0;
    cfg_opt_t opts[] = {
        COMMON_CONFIG_OPTION CFG_SIMPLE_INT ("jit_use", &(vm->jit_use)),
        CFG_SIMPLE_INT ("boot_flash_size", &pic32->boot_flash_size),
        CFG_SIMPLE_INT ("boot_flash_address", &pic32->boot_flash_address),
        CFG_SIMPLE_STR ("boot_file_name", &pic32->boot_file_name),
        CFG_SIMPLE_STR ("start_address", &start_address),
        CFG_SIMPLE_INT ("sdcard0_size", &pic32->sdcard0_size),
        CFG_SIMPLE_INT ("sdcard1_size", &pic32->sdcard1_size),
        CFG_SIMPLE_STR ("sdcard0_file_name", &pic32->sdcard0_file_name),
        CFG_SIMPLE_STR ("sdcard1_file_name", &pic32->sdcard1_file_name),

        /*CFG_SIMPLE_STR ("cs8900_iotype", &(pic32->cs8900_iotype)), */
        /* add other configure information here */
        CFG_END ()
    };
    cfg_t *cfg;

    cfg = cfg_init (opts, 0);
    cfg_parse (cfg, vm->configure_filename);
    cfg_free (cfg);
    if (start_address)
        pic32->start_address = strtoul (start_address, 0, 0);

    VALID_COMMON_CONFIG_OPTION;

    /*add other configure information validation here */
    if (vm->jit_use) {
        ASSERT (JIT_SUPPORT == 1,
            "You must compile with JIT Support to use jit.\n");
    }

    /* Print the configure information */
    PRINT_COMMON_CONFIG_OPTION;

    /* print other configure information here */
    if (pic32->boot_flash_size > 0) {
        printf ("boot_flash_size: %dk bytes\n", pic32->boot_flash_size);
        printf ("boot_flash_address: 0x%x\n", pic32->boot_flash_address);
        printf ("boot_file_name: %s\n", pic32->boot_file_name);
    }
    if (pic32->sdcard0_size > 0) {
        printf ("sdcard0_size: %dM bytes\n", pic32->sdcard0_size);
        printf ("sdcard0_file_name: %s\n", pic32->sdcard0_file_name);
    }
    if (pic32->sdcard1_size > 0) {
        printf ("sdcard1_size: %dM bytes\n", pic32->sdcard1_size);
        printf ("sdcard1_file_name: %s\n", pic32->sdcard1_file_name);
    }
    printf ("start_address: 0x%x\n", pic32->start_address);
}

/*
 * Create an instance of virtual machine.
 */
vm_instance_t *create_instance (char *configure_filename)
{
    vm_instance_t *vm;
    pic32_t *pic32;
    const char *name = "pic32";

    pic32 = malloc (sizeof (*pic32));
    if (! pic32) {
        fprintf (stderr, "PIC32: unable to create new instance!\n");
        return NULL;
    }
    memset (pic32, 0, sizeof (*pic32));

    vm = vm_create (name, VM_TYPE_PIC32);
    if (! vm) {
        fprintf (stderr, "PIC32: unable to create VM instance!\n");
        free (pic32);
        return NULL;
    }
    vm->hw_data = pic32;
    pic32->vm = vm;

    /* Initialize default parameters for  pic32 */
    if (configure_filename == NULL)
        configure_filename = "pic32.conf";
    vm->configure_filename = strdup (configure_filename);
    vm->ram_size = 128;         /* kilobytes */
    vm->boot_method = BOOT_BINARY;
    vm->boot_from = BOOT_FROM_NOR_FLASH;
    vm->flash_type = FLASH_TYPE_NOR_FLASH;

    pic32_parse_configure (pic32);

    /* init gdb debug */
    vm_debug_init (pic32->vm);

    return pic32->vm;
}

int init_instance (vm_instance_t * vm)
{
    pic32_t *pic32 = (pic32_t *) vm->hw_data;
    cpu_mips_t *cpu;

    if (pic32_init_platform (pic32) == -1) {
        vm_error (vm, "unable to initialize the platform hardware.\n");
        return (-1);
    }
    if (! vm->boot_cpu)
        return (-1);

    /* IRQ routing */
    vm->set_irq = pic32_set_irq;
    vm->clear_irq = pic32_clear_irq;

    vm_suspend (vm);

    /* Check that CPU activity is really suspended */
    if (cpu_group_sync_state (vm->cpu_group) == -1) {
        vm_error (vm, "unable to sync with system CPUs.\n");
        return (-1);
    }

    /* Reset the boot CPU */
    cpu = vm->boot_cpu;
    mips64_reset (cpu);

    /* Set config0-config3 registers. */
    cpu->cp0.config_usable = 0x0f;
    cpu->cp0.config_reg[0] = 0xa4010582;
    cpu->cp0.config_reg[1] = 0x80000004;
    cpu->cp0.config_reg[2] = 0x80000000;
    cpu->cp0.config_reg[3] = 0x00000060;

    /* set PC and PRID */
    cpu->cp0.reg[MIPS_CP0_PRID] = 0x00ff8700;   /* TODO */
    cpu->cp0.tlb_entries = 0;
    cpu->pc = pic32->start_address;

    /* reset all devices */
    dev_reset_all (vm);
    dev_sdcard_reset (cpu);

#ifdef _USE_JIT_
    /* if jit is used. flush all jit buffer */
    if (vm->jit_use)
        mips64_jit_flush (cpu, 0);
#endif

    /* Launch the simulation */
    printf ("--- Start simulation: PC=0x%" LL "x, JIT %sabled\n",
            cpu->pc, vm->jit_use ? "en" : "dis");
    vm->status = VM_STATUS_RUNNING;
    cpu_start (vm->boot_cpu);
    return (0);
}
