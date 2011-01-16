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

m_uint32_t pic32_int_table[JZ4740_INT_INDEX_MAX];

int dev_pic32_gpio_init(vm_instance_t * vm, char *name, m_pa_t paddr, m_uint32_t len);
int dev_pic32_uart_init(vm_instance_t * vm, char *name, m_pa_t paddr, m_uint32_t len, u_int irq, vtty_t * vtty);
int dev_pic32_cpm_init(vm_instance_t * vm, char *name, m_pa_t paddr, m_uint32_t len);
int dev_pic32_emc_init(vm_instance_t * vm, char *name, m_pa_t paddr, m_uint32_t len);
int dev_pic32_rtc_init(vm_instance_t * vm, char *name, m_pa_t paddr, m_uint32_t len);
int dev_pic32_wdt_tcu_init(vm_instance_t * vm, char *name, m_pa_t paddr, m_uint32_t len);
int dev_pic32_int_init(vm_instance_t * vm, char *name, m_pa_t paddr, m_uint32_t len);
int dev_pic32_dma_init(vm_instance_t * vm, char *name, m_pa_t paddr, m_uint32_t len);
int dev_pic32_lcd_init(vm_instance_t * vm, char *name, m_pa_t paddr, m_uint32_t len);

void dev_pic32_gpio_setirq(int irq);
void dev_pic32_gpio_clearirq(int irq);



/* Initialize default parameters for  pic32 */
static void pic32_init_defaults(pic32_t * pic32)
{
   vm_instance_t *vm = pic32->vm;

   if (vm->configure_filename == NULL)
      vm->configure_filename = strdup(PIC32_DEFAULT_CONFIG_FILE);
   vm->ram_size = PIC32_DEFAULT_RAM_SIZE;
   vm->boot_method = PIC32_DEFAULT_BOOT_METHOD;
   vm->kernel_filename = strdup(PIC32_DEFAULT_KERNEL_FILENAME);
}

/* Initialize the PIC32 Platform (MIPS) */
static int pic32_init_platform(pic32_t * pic32)
{
   struct vm_instance *vm = pic32->vm;
   cpu_mips_t *cpu0;
   void *(*cpu_run_fn) (void *);

   vm_init_vtty(vm);


   /* Create a CPU group */
   vm->cpu_group = cpu_group_create("System CPU");

   /* Initialize the virtual MIPS processor */
   cpu0 = cpu_create(vm, CPU_TYPE_MIPS32, 0);
   if (! cpu0)
   {
      vm_error(vm, "unable to create CPU0!\n");
      return (-1);
   }
   /* Add this CPU to the system CPU group */
   cpu_group_add(vm->cpu_group, cpu0);
   vm->boot_cpu = cpu0;


   cpu_run_fn = (void *) mips64_exec_run_cpu;
   /* create the CPU thread execution */
   if (pthread_create(&cpu0->cpu_thread, NULL, cpu_run_fn, cpu0) != 0)
   {
      fprintf(stderr, "cpu_create: unable to create thread for CPU%u\n", 0);
      free(cpu0);
      return (-1);
   }
   cpu0->addr_bus_mask = PIC32_ADDR_BUS_MASK;

   /* Initialize RAM */
   vm_ram_init(vm, 0x00000000ULL);

#if 0
   /*create 1GB nand flash */
   if ((vm->flash_size == 0x400) && (vm->flash_type = FLASH_TYPE_NAND_FLASH))
      if (dev_nand_flash_1g_init(vm, "NAND FLASH 1G", NAND_DATAPORT, 0x10004, &(pic32->nand_flash)) == -1)
         return (-1);
   if (dev_pic32_gpio_init(vm, "JZ4740 GPIO", JZ4740_GPIO_BASE, JZ4740_GPIO_SIZE) == -1)
      return (-1);
   if (dev_pic32_uart_init(vm, "JZ4740 UART0", JZ4740_UART0_BASE, JZ4740_UART0_SIZE, 9, vm->vtty_con1) == -1)
      return (-1);
   if (dev_pic32_uart_init(vm, "JZ4740 UART1", JZ4740_UART1_BASE, JZ4740_UART1_SIZE, 8, vm->vtty_con2) == -1)
      return (-1);

   if (dev_pic32_cpm_init(vm, "JZ4740 CPM", JZ4740_CPM_BASE, JZ4740_CPM_SIZE) == -1)
      return (-1);
   if (dev_pic32_emc_init(vm, "JZ4740 EMC", JZ4740_EMC_BASE, JZ4740_EMC_SIZE) == -1)
      return (-1);
   if (dev_pic32_rtc_init(vm, "JZ4740 RTC", JZ4740_RTC_BASE, JZ4740_RTC_SIZE) == -1)
      return (-1);
   if (dev_pic32_wdt_tcu_init(vm, "JZ4740 WDT/TCU", JZ4740_WDT_TCU_BASE, JZ4740_WDT_TCU_SIZE) == -1)
      return (-1);
   if (dev_pic32_int_init(vm, "JZ4740 INT", JZ4740_INT_BASE, JZ4740_INT_SIZE) == -1)
      return (-1);
   if (dev_pic32_dma_init(vm, "JZ4740 DMA", JZ4740_DMA_BASE, JZ4740_DMA_SIZE) == -1)
      return (-1);
#endif
   return (0);
}

static int pic32_boot(pic32_t * pic32)
{
   vm_instance_t *vm = pic32->vm;

   if (!vm->boot_cpu)
      return (-1);

   return pic32_reset(vm);
}

void pic32_clear_irq(vm_instance_t * vm, u_int irq)
{
   m_uint32_t irq_mask;

   irq_mask = 1 << irq;

   /*clear ISR and IPR */
   pic32_int_table[INTC_ISR / 4] &= ~irq_mask;
   pic32_int_table[INTC_IPR / 4] &= ~irq_mask;


}

/*map irq to soc irq*/
int forced_inline plat_soc_irq(u_int irq)
{
   if ((irq >= 48) && (irq <= 175))
   {
//      dev_pic32_gpio_setirq(irq);
      /*GPIO IRQ */
      if ((irq >= 48) && (irq <= 79))
         irq = IRQ_GPIO0;
      else if ((irq >= 80) && (irq <= 111))
         irq = IRQ_GPIO1;
      else if ((irq >= 112) && (irq <= 143))
         irq = IRQ_GPIO2;
      else if ((irq >= 144) && (irq <= 175))
         irq = IRQ_GPIO3;
   }
   return irq;
}


void pic32_set_irq(vm_instance_t * vm, u_int irq)
{
   m_uint32_t irq_mask;


   irq = plat_soc_irq(irq);

   irq_mask = 1 << irq;
   pic32_int_table[INTC_ISR / 4] |= irq_mask;
   /*first check ICMR. masked interrupt is **invisible** to cpu */
   if (unlikely(pic32_int_table[INTC_IMR / 4] & irq_mask))
   {
      /*the irq is masked. clear IPR */
      pic32_int_table[INTC_IPR / 4] &= ~irq_mask;
   }
   else
   {
      /*the irq is not masked */

      /*set IPR */
      /*
         we set IPR, not *or* . yajin

         JZ Kernel 'plat_irq_dispatch' determine which is the highest priority interrupt
         and handle.
         It uses a function ffs to find first set irq from least bit to highest bit.
         260         irq = ffs(intc_ipr) - 1;

         That means when tcu0 irq and gpio1 irq occurs at the same time ,INTC_IPR=0x8800000
         and irq handler will handle tcu0 irq(bit 23) not gpio1 irq(bit 27).

         In pic32 gpio1->cs8900 int

         TCU0 irq occurs every 10 ms and gpio1 occurs about 10ms (cs8900 has received a packet
         or has txed a packet), jz kernel always handle tcu0 irq. gpio1 irq is hungry. So I just set
         pic32_int_table[INTC_IPR/4]= irq_mask not or(|) irq_mask. TCU0 irq may be lost. However,
         gpio1 irq is not so ofen so it is not a big problem.

         In emulator, irq is not a good method for hardware to tell kernel something has happened.
         Emulator likes polling more than interrupt :) .

       */
      pic32_int_table[INTC_IPR / 4] = irq_mask;


      mips64_set_irq(vm->boot_cpu, JZ4740_INT_TO_MIPS);
      mips64_update_irq_flag(vm->boot_cpu);
   }
}

COMMON_CONFIG_INFO_ARRAY;

static void printf_configure(pic32_t * pic32)
{

   vm_instance_t *vm = pic32->vm;
   PRINT_COMMON_COFING_OPTION;

   /*print other configure information here */
}
static void pic32_parse_configure(pic32_t * pic32)
{
   vm_instance_t *vm = pic32->vm;
   cfg_opt_t opts[] = {
      COMMON_CONFIG_OPTION
          /*add other configure information here */
/*          CFG_SIMPLE_INT("cs8900_enable", &(pic32->cs8900_enable)),*/
/*      CFG_SIMPLE_STR("cs8900_iotype", &(pic32->cs8900_iotype)),*/
      CFG_SIMPLE_INT("jit_use", &(vm->jit_use)),

      CFG_END()
   };
   cfg_t *cfg;

   cfg = cfg_init(opts, 0);
   cfg_parse(cfg, vm->configure_filename);
   cfg_free(cfg);

   VALID_COMMON_CONFIG_OPTION;

   /*add other configure information validation here */
   if (vm->boot_method == BOOT_BINARY)
   {
      ASSERT(vm->boot_from == 2, "boot_from must be 2(NAND Flash)\n pic32 only can boot from NAND Flash.\n");
   }
   if (vm->jit_use==1)
   	{
   		ASSERT(JIT_SUPPORT==1, "You must compile with JIT Support to use jit. \n");
   	}

   /*Print the configure information */
   printf_configure(pic32);

}

/* Create a router instance */
vm_instance_t *create_instance(char *configure_filename)
{
   pic32_t *pic32;
   char *name;
   if (!(pic32 = malloc(sizeof(*pic32))))
   {
      fprintf(stderr, "ADM5120': Unable to create new instance!\n");
      return NULL;
   }

   memset(pic32, 0, sizeof(*pic32));
   name = strdup("pic32");

   if (!(pic32->vm = vm_create(name, VM_TYPE_PIC32)))
   {
      fprintf(stderr, "PIC32 : unable to create VM instance!\n");
      goto err_vm;
   }
   free(name);

   if (configure_filename != NULL)
      pic32->vm->configure_filename = strdup(configure_filename);
   pic32_init_defaults(pic32);
   pic32_parse_configure(pic32);
   /*init gdb debug */
   vm_debug_init(pic32->vm);


   pic32->vm->hw_data = pic32;

   return (pic32->vm);


 err_vm:
   free(name);
   free(pic32);
   return NULL;
}

int init_instance(vm_instance_t * vm)
{
   pic32_t *pic32 = VM_PIC32(vm);

   if (pic32_init_platform(pic32) == -1)
   {
      vm_error(vm, "unable to initialize the platform hardware.\n");
      return (-1);
   }
   /* IRQ routing */
   vm->set_irq = pic32_set_irq;
   vm->clear_irq = pic32_clear_irq;

   return (pic32_boot(pic32));
}

int pic32_reset(vm_instance_t * vm)
{
   cpu_mips_t *cpu;
   m_va_t kernel_entry_point;
   vm_suspend(vm);

   /* Check that CPU activity is really suspended */
   if (cpu_group_sync_state(vm->cpu_group) == -1)
   {
      vm_error(vm, "unable to sync with system CPUs.\n");
      return (-1);
   }

   /* Reset the boot CPU */
   cpu = (vm->boot_cpu);
   mips64_reset(cpu);

   /*set configure register */
   cpu->cp0.config_usable = 0x83;       /* configure sel 0 1 7 is valid */
   cpu->cp0.config_reg[0] = JZ4740_CONFIG0;
   cpu->cp0.config_reg[1] = JZ4740_CONFIG1;
   cpu->cp0.config_reg[7] = JZ4740_CONFIG7;

   /*set PC and PRID */
   cpu->cp0.reg[MIPS_CP0_PRID] = JZ4740_PRID;
   cpu->cp0.tlb_entries = JZ4740_DEFAULT_TLB_ENTRYNO;
   cpu->pc = JZ4740_ROM_PC;
   /*reset all devices */
   dev_reset_all(vm);

#ifdef _USE_JIT_
   /*if jit is used. flush all jit buffer*/
   if (vm->jit_use)
   		mips64_jit_flush(cpu,0);
#endif
   /*If we boot from elf kernel image, load the image and set pc to elf entry */
   if (vm->boot_method == BOOT_ELF)
   {
      if (mips64_load_elf_image(cpu, vm->kernel_filename, &kernel_entry_point) == -1)
         return (-1);
      cpu->pc = kernel_entry_point;
   }
#if 0
   else if (vm->boot_method == BOOT_BINARY)
   {
      if (jz4740_boot_from_nandflash(vm) == -1)
         return (-1);
   }
#endif
/* Launch the simulation */
   printf("VM '%s': starting simulation (CPU0 PC=0x%" LL "x), JIT %sabled.\n",
          vm->name, cpu->pc, vm->jit_use ? "en" : "dis");
   vm->status = VM_STATUS_RUNNING;
   cpu_start(vm->boot_cpu);
   return (0);
}




#if 0
m_uint32_t pic32_int_table [JZ4740_INT_INDEX_MAX];

int dev_pic32_gpio_init(vm_instance_t * vm, char *name, m_pa_t paddr, m_uint32_t len);
int dev_pic32_uart_init(vm_instance_t * vm, char *name, m_pa_t paddr, m_uint32_t len, u_int irq, vtty_t * vtty);
int dev_pic32_cpm_init(vm_instance_t * vm, char *name, m_pa_t paddr, m_uint32_t len);
int dev_pic32_emc_init(vm_instance_t * vm, char *name, m_pa_t paddr, m_uint32_t len);
int dev_pic32_rtc_init(vm_instance_t * vm, char *name, m_pa_t paddr, m_uint32_t len);
int dev_pic32_wdt_tcu_init(vm_instance_t * vm, char *name, m_pa_t paddr, m_uint32_t len);
int dev_pic32_int_init(vm_instance_t * vm, char *name, m_pa_t paddr, m_uint32_t len);
int dev_pic32_dma_init(vm_instance_t * vm, char *name, m_pa_t paddr, m_uint32_t len);
int dev_pic32_lcd_init(vm_instance_t * vm, char *name, m_pa_t paddr, m_uint32_t len);

void dev_pic32_gpio_setirq(int irq);
void dev_pic32_gpio_clearirq(int irq);

#if 0
static mips_def_t pic32_def = {
        .name = "pic32",
        .CP0_PRid = 0x0ad0024f,
        .config_usable = 0x83,
        .CP0_Config0 = 0x80000082,
        .CP0_Config1 = 0x3E613080 ,
        .CP0_Config7 = 0,
        .SEGBITS = 32,
        .PABITS = 32,
        .pc=0x80000004,
        .tlb_entries =32,
        .address_model = 32,
};
#endif

/*
 * Initialize the pic32 Platform (MIPS)
 */
static int pic32_init_platform(pic32_t *pic32)
{
    struct vm_instance *vm = pic32->vm;
    cpu_mips_t *cpu;
    void *(*cpu_run_fn) (void *);

    vm_init_vtty(vm);

    /* Initialize the virtual MIPS processor */
    cpu = cpu_create(vm, CPU_TYPE_MIPS32, 0);
    if (! cpu)
    {
        vm_error(vm, "unable to create CPU0!\n");
        return (-1);
    }
    cpu_register(cpu, &pic32_def);
    cpu_init(cpu);

    vm->cpu = cpu;

    cpu_run_fn = (void *) mips64_exec_run_cpu;
    /* create the CPU thread execution */
    if (pthread_create(&cpu->cpu_thread, NULL, cpu_run_fn, cpu) != 0)
    {
        fprintf(stderr, "cpu_create: unable to create thread for CPU%u\n", 0);
        free(cpu);
        return (-1);
    }

    /* Initialize RAM */
    vm_ram_init(vm, 0x00000000ULL);

#if 0
    /*create 1GB nand flash */
    if ((vm->configure->flash_size == 0x400) && (vm->configure->flash_type = FLASH_TYPE_NAND_FLASH))
        if (dev_nand_flash_1g_init(vm, "NAND FLASH 1G", NAND_DATAPORT, 0x10004, &(pic32->nand_flash)) == -1)
            return (-1);
    if (dev_pic32_gpio_init(vm, "JZ4740 GPIO", JZ4740_GPIO_BASE, JZ4740_GPIO_SIZE) == -1)
        return (-1);
    if (dev_pic32_uart_init(vm, "JZ4740 UART0", JZ4740_UART0_BASE, JZ4740_UART0_SIZE, 9, vm->vtty_con1) == -1)
        return (-1);
    if (dev_pic32_uart_init(vm, "JZ4740 UART1", JZ4740_UART1_BASE, JZ4740_UART1_SIZE, 8, vm->vtty_con2) == -1)
        return (-1);
    if (dev_pic32_cpm_init(vm, "JZ4740 CPM", JZ4740_CPM_BASE, JZ4740_CPM_SIZE) == -1)
        return (-1);
    if (dev_pic32_emc_init(vm, "JZ4740 EMC", JZ4740_EMC_BASE, JZ4740_EMC_SIZE) == -1)
        return (-1);
    if (dev_pic32_rtc_init(vm, "JZ4740 RTC", JZ4740_RTC_BASE, JZ4740_RTC_SIZE) == -1)
        return (-1);
    if (dev_pic32_wdt_tcu_init(vm, "JZ4740 WDT/TCU", JZ4740_WDT_TCU_BASE, JZ4740_WDT_TCU_SIZE) == -1)
        return (-1);
    if (dev_pic32_int_init(vm, "JZ4740 INT", JZ4740_INT_BASE, JZ4740_INT_SIZE) == -1)
        return (-1);
    if (dev_pic32_dma_init(vm, "JZ4740 DMA", JZ4740_DMA_BASE, JZ4740_DMA_SIZE) == -1)
        return (-1);
#endif
    return (0);
}

static int pic32_boot(pic32_t *pic32)
{
    vm_instance_t *vm = pic32->vm;

    if (!vm->cpu)
        return (-1);

    return pic32_reset(vm);
}

void pic32_clear_irq(vm_instance_t * vm, u_int irq)
{
    m_uint32_t irq_mask;

    irq_mask = 1 << irq;

    /*clear ISR and IPR */
    pic32_int_table[INTC_ISR / 4] &= ~irq_mask;
    pic32_int_table[INTC_IPR / 4] &= ~irq_mask;
}

/*map irq to soc irq*/
int forced_inline plat_soc_irq(u_int irq)
{
    if ((irq >= 48) && (irq <= 175))
    {
//        dev_pic32_gpio_setirq(irq);
        /*GPIO IRQ */
        if ((irq >= 48) && (irq <= 79))
            irq = IRQ_GPIO0;
        else if ((irq >= 80) && (irq <= 111))
            irq = IRQ_GPIO1;
        else if ((irq >= 112) && (irq <= 143))
            irq = IRQ_GPIO2;
        else if ((irq >= 144) && (irq <= 175))
            irq = IRQ_GPIO3;
    }
    return irq;
}


void pic32_set_irq(vm_instance_t * vm, u_int irq)
{
    m_uint32_t irq_mask;

    irq = plat_soc_irq(irq);

    irq_mask = 1 << irq;
    pic32_int_table[INTC_ISR / 4] |= irq_mask;

    /*first check ICMR. masked interrupt is **invisible** to cpu */
    if (unlikely(pic32_int_table[INTC_IMR / 4] & irq_mask))
    {
        /*the irq is masked. clear IPR */
        pic32_int_table[INTC_IPR / 4] &= ~irq_mask;
    }
    else
    {
        /*the irq is not masked */

        /*set IPR */
        /*
           we set IPR, not *or* . yajin

           JZ Kernel 'plat_irq_dispatch' determine which is the highest priority interrupt
           and handle.
           It uses a function ffs to find first set irq from least bit to highest bit.
           260         irq = ffs(intc_ipr) - 1;

           That means when tcu0 irq and gpio1 irq occurs at the same time ,INTC_IPR=0x8800000
           and irq handler will handle tcu0 irq(bit 23) not gpio1 irq(bit 27).

           In emulator, irq is not a good method for hardware to tell kernel something has happened.
           Emulator likes polling more than interrupt :) .
         */
        pic32_int_table[INTC_IPR / 4] = irq_mask;

        mips64_set_irq(vm->cpu, JZ4740_INT_TO_MIPS);
        mips64_update_irq_flag(vm->cpu);
    }
}

COMMON_CONFIG_INFO_ARRAY;

static void printf_configure(pic32_t *pic32)
{

    vm_instance_t *vm = pic32->vm;
    PRINT_COMMON_COFING_OPTION;

    /*print other configure information here */
}

static void pic32_parse_configure(pic32_t *pic32)
{
    vm_instance_t *vm = pic32->vm;
    cfg_opt_t opts[] = {
        COMMON_CONFIG_OPTION
        /*add other configure information here */
//      CFG_SIMPLE_INT("cs8900_enable", &(pic32->cs8900_enable)),
//      CFG_SIMPLE_STR("cs8900_iotype", &(pic32->cs8900_iotype)),

        CFG_END()
    };
    cfg_t *cfg;

    cfg = cfg_init(opts, 0);
    cfg_parse(cfg, vm->configure_filename);
    cfg_free(cfg);

    VALID_COMMON_CONFIG_OPTION;

    /*Print the configure information */
    printf_configure(pic32);

}

/* Create a router instance */
vm_instance_t *create_instance(char *configure_filename)
{
    pic32_t *pic32;
    char *name;
    pic32 = malloc(sizeof(*pic32));
    if (! pic32)
    {
        fprintf(stderr, "PIC32: Unable to create new instance!\n");
        return NULL;
    }

    memset(pic32, 0, sizeof(*pic32));
    name = strdup("pic32");

    pic32->vm = vm_create(name, VM_TYPE_PIC32);
    if (! pic32->vm)
    {
        fprintf(stderr, "PIC32: unable to create VM instance!\n");
        goto err_vm;
    }
    free(name);

    if (configure_filename == NULL)
        pic32->vm->configure_filename = strdup(PIC32_DEFAULT_CONFIG_FILE);
    else
        pic32->vm->configure_filename = strdup(configure_filename);
    pic32_parse_configure(pic32);
    /*init gdb debug */
    vm_debug_init(pic32->vm);

    pic32->vm->hw_data = pic32;

    return (pic32->vm);

err_vm:
    free(name);
    free(pic32);
    return NULL;

}

int init_instance(vm_instance_t * vm)
{
    pic32_t *pic32 = VM_PIC32(vm);

    if (pic32_init_platform(pic32) == -1)
    {
        vm_error(vm, "unable to initialize the platform hardware.\n");
        return (-1);
    }
    /* IRQ routing */
    vm->set_irq = pic32_set_irq;
    vm->clear_irq = pic32_clear_irq;

    return (pic32_boot(pic32));
}
#endif
