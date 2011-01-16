 /*
  * Copyright (C) yajin 2008 <yajinzhou@gmail.com >
  *     
  * This file is part of the virtualmips distribution. 
  * See LICENSE file for terms of the license. 
  *
  */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <pthread.h>

#include "cpu.h"
#include "mips64_memory.h"
#include "device.h"
#include "mips64.h"
#include "mips64_cp0.h"
#include "mips64_exec.h"
#include "vm.h"



#define LOG_ENABLE 0
/* Log a message for a CPU */
void cpu_log(cpu_mips_t * cpu, char *module, char *format, ...)
{
#if LOG_ENABLE
   char buffer[256];
   va_list ap;

   va_start(ap, format);
   snprintf(buffer, sizeof(buffer), "CPU%u: %s", cpu->id, module);
   vm_flog(cpu->vm, buffer, format, ap);
   va_end(ap);
#endif
}

/* Start a CPU */
void cpu_start(cpu_mips_t * cpu)
{
   if (cpu)
   {
      cpu->state = CPU_STATE_RUNNING;
   }
}

/* Stop a CPU */
void cpu_stop(cpu_mips_t * cpu)
{
   if (cpu)
   {
      cpu_log(cpu, "CPU_STATE", "Halting CPU (old state=%u)...\n", cpu->state);
      cpu->state = CPU_STATE_HALTED;
   }
}
void cpu_restart(cpu_mips_t * cpu)
{
   if (cpu)
   {
      cpu_log(cpu, "CPU_STATE", "Restartting CPU (old state=%u)...\n", cpu->state);
      cpu->state = CPU_STATE_RESTARTING;
   }
}

/* Create a new CPU */
cpu_mips_t *cpu_create(vm_instance_t * vm, u_int type, u_int id)
{
   cpu_mips_t *cpu;

   if (!(cpu = malloc(sizeof(*cpu))))
      return NULL;

   memset(cpu, 0, sizeof(*cpu));
   cpu->vm = vm;
   cpu->id = id;
   cpu->type = type;
   cpu->state = CPU_STATE_SUSPENDED;
   cpu->vm = vm;
   return cpu;

}

int cpu_register (cpu_mips_t *cpu, const mips_def_t *def)
{
	ASSERT(def!=NULL,"def can not be NULL\n");
	memcpy(&cpu->def,def,sizeof(*def));
	return (0);
}
int cpu_init (cpu_mips_t *cpu)
{
	mips64_init( cpu);
	return (0);
}

/* Delete a CPU */
void cpu_delete(cpu_mips_t * cpu)
{
   if (cpu)
   {
      /* Stop activity of this CPU */
      cpu_stop(cpu);
      pthread_join(cpu->cpu_thread, NULL);
      mips64_delete(cpu);
      free(cpu);
   }
}


