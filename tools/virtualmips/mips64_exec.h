/*
 * Cisco router simulation platform.
 * Copyright (c) 2005,2006 Christophe Fillot (cf@utc.fr)
 */
 /*
  * Copyright (C) yajin 2008 <yajinzhou@gmail.com >
  *     
  * This file is part of the virtualmips distribution. 
  * See LICENSE file for terms of the license. 
  *
  */


#ifndef __MIPS64_EXEC_H__
#define __MIPS64_EXEC_H__

#include "utils.h"
#include "system.h"


 struct mips64_op_desc {
  char       *opname;
  int (*func) (cpu_mips_t *, mips_insn_t);
  m_uint16_t num;
};



extern cpu_mips_t *current_cpu;


/* Run MIPS code in step-by-step mode */
void * mips64_exec_run_cpu(cpu_mips_t * cpu);

#endif
