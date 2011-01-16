 /*
  * Copyright (C) yajin 2008 <yajinzhou@gmail.com >
  *     
  * This file is part of the virtualmips distribution. 
  * See LICENSE file for terms of the license. 
  *
  */

#ifndef __PAVO_H__
#define __PAVO_H__

#include "jz4740.h"
#include "dev_nand_flash_1g.h"

#ifdef SIM_LCD
#include "vp_sdl.h"
#endif


#define PAVO_DEFAULT_CONFIG_FILE     "pavo.conf"



struct pavo_system
{
   /* Associated VM instance */
   vm_instance_t *vm;
   nand_flash_1g_data_t *nand_flash;

   m_uint32_t cs8900_enable;
   char *cs8900_iotype;

};


typedef struct pavo_system pavo_t;


#define VM_PAVO(vm) ((pavo_t *)vm->hw_data)


vm_instance_t *create_instance(char *conf);
int init_instance(vm_instance_t * vm);
int pavo_reset(vm_instance_t * vm);

#endif
