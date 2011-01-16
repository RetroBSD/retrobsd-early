 /*
  * Copyright (C) yajin 2008 <yajinzhou@gmail.com >
  *     
  * This file is part of the virtualmips distribution. 
  * See LICENSE file for terms of the license. 
  *
  */


#include<string.h>
#include <assert.h>
#include<stdlib.h>
#include <confuse.h>

#include "utils.h"
#include "mips64.h"
#include "vm.h"
#include "cpu.h"
#include "mips64_exec.h"
#include "debug.h"

#include "jz4740.h"
#include "pavo.h"
#include "device.h"


int jz4740_boot_from_nandflash(vm_instance_t * vm)
{
    struct vdevice *dev;
    unsigned char *page_addr;
    int i;

    pavo_t *pavo;
    if (vm->type == VM_TYPE_PAVO)
    {
        pavo = VM_PAVO(vm);
    }
    else
        ASSERT(0, "Error vm type\n");
    /*get ram device */
    dev = dev_lookup(vm, 0x0);
    assert(dev != NULL);
    assert(dev->host_addr != 0);
    /*copy 8K nand flash data to 8K RAM */
    for (i = 0; i < (0x2000 / NAND_FLASH_1G_PAGE_DATA_SIZE); i++)
    {
        page_addr = get_nand_flash_page_ptr(i, pavo->nand_flash->flash_map[0]);
        memcpy((unsigned char *) dev->host_addr + NAND_FLASH_1G_PAGE_DATA_SIZE * i, page_addr,
               NAND_FLASH_1G_PAGE_DATA_SIZE);
    }
    return (0);
}

int jz4740_reset(vm_instance_t * vm)
{
    cpu_mips_t *cpu;
    vm_suspend(vm);

    /* Reset the boot CPU */
    cpu = vm->cpu;
    mips64_reset(cpu);

    /*reset all devices */
    dev_reset_all(vm);

    if (jz4740_boot_from_nandflash(vm) == -1)
        return (-1);

/* Launch the simulation */
    printf("VM '%s': starting simulation (CPU0 PC=0x%" LL "x)\n", vm->name, cpu->pc);
    vm->status = VM_STATUS_RUNNING;
    cpu_start(vm->cpu);
    return (0);

}
