 /*
  * Copyright (C) yajin 2008 <yajinzhou@gmail.com >
  *     
  * This file is part of the virtualmips distribution. 
  * See LICENSE file for terms of the license. 
  *
  */



#ifndef __CPU_H__
#define __CPU_H__

#include <pthread.h>

#include "utils.h"
#include "mips64.h"
#include "system.h"


/* Possible CPU types */
enum
{
   CPU_TYPE_MIPS64 = 1,
   CPU_TYPE_MIPS32,
};

/* Virtual CPU states */
enum
{
   CPU_STATE_RUNNING = 0,       /*cpu is running */
   CPU_STATE_HALTED,
   CPU_STATE_SUSPENDED,         /*CPU is SUSPENDED */
   CPU_STATE_RESTARTING,        /*cpu is restarting */
   CPU_STATE_PAUSING,           /*cpu is pausing for timer */
};




void cpu_log(cpu_mips_t * cpu, char *module, char *format, ...);
void cpu_start(cpu_mips_t * cpu);
void cpu_stop(cpu_mips_t * cpu);
void cpu_restart(cpu_mips_t * cpu);
cpu_mips_t *cpu_create(vm_instance_t * vm, u_int type, u_int id);
int cpu_register (cpu_mips_t *cpu, const mips_def_t *def);
int cpu_init (cpu_mips_t *cpu);
void cpu_delete(cpu_mips_t * cpu);




#endif
