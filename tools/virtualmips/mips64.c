/*
 * Cisco router simulation platform.
 * Copyright (c) 2005,2006 Christophe Fillot (cf@utc.fr)
 *
 */

 /*
  * Copyright (C) yajin 2008 <yajinzhou@gmail.com >
  *     
  * This file is part of the virtualmips distribution. 
  * See LICENSE file for terms of the license. 
  *
  */

#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <libelf.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>

#include "mips64_memory.h"
#include "mips64_exec.h"
#include "mips64.h"
#include "vm.h"
#include "utils.h"
#include "system.h"
#include "mips64_cp0.h"


#define GDB_SR            32
#define GDB_LO            33
#define GDB_HI            34
#define GDB_BAD         35
#define GDB_CAUSE    36
#define GDB_PC           37


/* Set a register */
void mips64_reg_set(cpu_mips_t * cpu, u_int reg, m_reg_t val)
{
    if (reg < MIPS64_GPR_NR)
        cpu->gpr[reg] = val;
}

/*get register value giving index. For GDB*/
int mips64_reg_get(cpu_mips_t * cpu, u_int reg, m_reg_t * val)
{
    if (reg < MIPS64_GPR_NR)
    {
        *val = cpu->gpr[reg];
        return SUCCESS;
    }
    else
    {
        switch (reg)
        {
        case GDB_SR:
            *val = cpu->cp0.reg[MIPS_CP0_STATUS];
            break;
        case GDB_LO:
            *val = cpu->lo;
            break;
        case GDB_HI:
            *val = cpu->hi;
            break;
        case GDB_BAD:
            *val = cpu->cp0.reg[MIPS_CP0_BADVADDR];
            break;
        case GDB_CAUSE:
            *val = cpu->cp0.reg[MIPS_CP0_CAUSE];
            break;
        case GDB_PC:
            *val = cpu->pc;
            break;
        default:
            return FAILURE;
        }

    }

    return SUCCESS;

}



/* Delete a MIPS64 processor */
void mips64_delete(cpu_mips_t * cpu)
{
    if (cpu)
    {
        mips_mem_shutdown(cpu);
    }
}

/* Reset a MIPS64 CPU */
int mips64_reset(cpu_mips_t * cpu)
{
    cpu->cp0.reg[MIPS_CP0_STATUS] = MIPS_CP0_STATUS_BEV;
    cpu->cp0.reg[MIPS_CP0_CAUSE] = 0;

        /*set configure register */
    cpu->cp0.config_usable = cpu->def.config_usable;      /* configure sel 0 1 7 is valid */
    cpu->cp0.config_reg[0] = cpu->def.CP0_Config0;
    cpu->cp0.config_reg[1] = cpu->def.CP0_Config1;
    cpu->cp0.config_reg[2] = cpu->def.CP0_Config2;
    cpu->cp0.config_reg[3] = cpu->def.CP0_Config3;
    cpu->cp0.config_reg[4] = cpu->def.CP0_Config4;
    cpu->cp0.config_reg[5] = cpu->def.CP0_Config5;
    cpu->cp0.config_reg[6] = cpu->def.CP0_Config6;
    cpu->cp0.config_reg[7] = cpu->def.CP0_Config7;

    /*set PC and PRID */
    cpu->pc = cpu->def.pc;
    cpu->cp0.reg[MIPS_CP0_PRID] =  cpu->def.CP0_PRid;
    cpu->cp0.tlb_entries = cpu->def.tlb_entries;
    

    /* Clear the complete TLB */
    memset(&cpu->cp0.tlb, 0, MIPS64_TLB_MAX_ENTRIES * sizeof(tlb_entry_t));

    /* Restart the MTS subsystem */
    mips_set_addr_mode(cpu, cpu->def.address_model);       /* zzz */
    cpu->mts_rebuild(cpu);

    return (0);
}

/* Initialize a MIPS64 processor */
int mips64_init(cpu_mips_t * cpu)
{
	
    /* Set the CPU methods */
    cpu->reg_get = (void *) mips64_reg_get;
    cpu->reg_set = (void *) mips64_reg_set;

    cpu->addr_bus_mask =(m_uint32_t)((1ULL<<cpu->def.PABITS) -1);

    /* Set the startup parameters */
    mips64_reset(cpu);
    return (0);
}


/* Update the IRQ flag */
int  mips64_update_irq_flag(cpu_mips_t * cpu)
{
    mips_cp0_t *cp0 = &cpu->cp0;
    m_uint32_t imask, sreg_mask;
    m_uint32_t cause;
    cpu->irq_pending = FALSE;

    cause = cp0->reg[MIPS_CP0_CAUSE] & ~MIPS_CP0_CAUSE_IMASK;
    cp0->reg[MIPS_CP0_CAUSE] = cause | cpu->irq_cause;
    sreg_mask = MIPS_CP0_STATUS_IE | MIPS_CP0_STATUS_EXL | MIPS_CP0_STATUS_ERL;

    if ((cp0->reg[MIPS_CP0_STATUS] & sreg_mask) == MIPS_CP0_STATUS_IE)
    {

        imask = cp0->reg[MIPS_CP0_STATUS] & MIPS_CP0_STATUS_IMASK;
        if (unlikely(cp0->reg[MIPS_CP0_CAUSE] & imask))
        {
            cpu->irq_pending = TRUE;

            return (TRUE);
        }
    }

    return (FALSE);

}

/* Generate an exception */
void mips64_trigger_exception(cpu_mips_t * cpu, u_int exc_code, int bd_slot)
{
    mips_cp0_t *cp0 = &cpu->cp0;
    m_uint64_t new_pc;
    m_reg_t cause;


    /* keep IM, set exception code and bd slot */
    cause = cp0->reg[MIPS_CP0_CAUSE];

    /* we don't set EPC if EXL is set */
    if (!(cp0->reg[MIPS_CP0_STATUS] & MIPS_CP0_STATUS_EXL))
    {
        cp0->reg[MIPS_CP0_EPC] = cpu->pc;
        /*Cause BD is not update. MIPS VOLUME  V3 P65 */
        cause &= ~MIPS_CP0_CAUSE_BD_SLOT;       //clear bd
        if (bd_slot)
            cause |= MIPS_CP0_CAUSE_BD_SLOT;
        else
            cause &= ~MIPS_CP0_CAUSE_BD_SLOT;

    }

    cause &= ~MIPS_CP0_CAUSE_EXC_MASK;  //clear exec-code
    cause |= (exc_code << 2);
    cp0->reg[MIPS_CP0_CAUSE] = cause;
    if (exc_code == MIPS_CP0_CAUSE_INTERRUPT)
        cpu->irq_cause = 0;

    /* Set EXL bit in status register */
    /*TODO: RESET SOFT RESET AND NMI EXCEPTION */
    cp0->reg[MIPS_CP0_STATUS] |= MIPS_CP0_STATUS_EXL;



    /* clear ERL bit in status register */
    /*TODO: RESET SOFT RESET AND NMI EXCEPTION */
    cp0->reg[MIPS_CP0_STATUS] &= ~MIPS_CP0_STATUS_ERL;


    if (cp0->reg[MIPS_CP0_STATUS] & MIPS_CP0_STATUS_BEV)
    {
        if ((exc_code == MIPS_CP0_CAUSE_TLB_LOAD) || (exc_code == MIPS_CP0_CAUSE_TLB_SAVE))
        {
            if (cp0->reg[MIPS_CP0_STATUS] & MIPS_CP0_STATUS_EXL)
                new_pc = 0xffffffffbfc00380ULL;
            else
                new_pc = 0xffffffffbfc00200ULL;
        }
        else if (exc_code == MIPS_CP0_CAUSE_INTERRUPT)
        {
            if (cp0->reg[MIPS_CP0_CAUSE] & MIPS_CP0_CAUSE_IV)
                new_pc = 0xffffffffbfc00400ULL;
            else
                new_pc = 0xffffffffbfc00380ULL;

        }
        else
            new_pc = 0xffffffffbfc00380ULL;

    }
    else
    {
        if ((exc_code == MIPS_CP0_CAUSE_TLB_LOAD) || (exc_code == MIPS_CP0_CAUSE_TLB_SAVE))
        {
            if (cp0->reg[MIPS_CP0_STATUS] & MIPS_CP0_STATUS_EXL)
                new_pc = 0xffffffff80000180ULL;
            else
                new_pc = 0xffffffff80000000ULL;
        }
        else if (exc_code == MIPS_CP0_CAUSE_INTERRUPT)
        {
            if (cp0->reg[MIPS_CP0_CAUSE] & MIPS_CP0_CAUSE_IV)
                new_pc = 0xffffffff80000200ULL;
            else
                new_pc = 0xffffffff80000180ULL;
        }
        else
            new_pc = 0xffffffff80000180ULL;
    }

    cpu->pc = (m_va_t) new_pc;

    /* Clear the pending IRQ flag */
    cpu->irq_pending = 0;


}


/* Execute fpu instruction */
void  mips64_exec_soft_fpu(cpu_mips_t * cpu)
{
    mips_cp0_t *cp0 = &cpu->cp0;
    cp0->reg[MIPS_CP0_CAUSE] |= 0x10000000;     //CE=1
    mips64_trigger_exception(cpu, MIPS_CP0_CAUSE_CP_UNUSABLE, cpu->is_in_bdslot);

}

extern int tttt;

/* Execute ERET instruction */
void  mips64_exec_eret(cpu_mips_t * cpu)
{
    mips_cp0_t *cp0 = &cpu->cp0;

    if (cp0->reg[MIPS_CP0_STATUS] & MIPS_CP0_STATUS_ERL)
    {
        cp0->reg[MIPS_CP0_STATUS] &= ~MIPS_CP0_STATUS_ERL;
        cpu->pc = cp0->reg[MIPS_CP0_ERR_EPC];

    }
    else
    {
        cp0->reg[MIPS_CP0_STATUS] &= ~MIPS_CP0_STATUS_EXL;
        cpu->pc = cp0->reg[MIPS_CP0_EPC];
    }
    /* We have to clear the LLbit */
    cpu->ll_bit = 0;

}

/* Execute BREAK instruction */
void  mips64_exec_break(cpu_mips_t * cpu, u_int code)
{
    //mips64_dump_regs(cpu);
    printf("exec break cpu->pc %x\n", cpu->pc);

    /* XXX TODO: Branch Delay slot */
    mips64_trigger_exception(cpu, MIPS_CP0_CAUSE_BP, 0);

}

/* Trigger a Trap Exception */
void  mips64_trigger_trap_exception(cpu_mips_t * cpu)
{
    /* XXX TODO: Branch Delay slot */
    printf("MIPS64: TRAP exception, CPU=%p\n", cpu);
    mips64_trigger_exception(cpu, MIPS_CP0_CAUSE_TRAP, 0);
}

/* Execute SYSCALL instruction */
void  mips64_exec_syscall(cpu_mips_t * cpu)
{
#if DEBUG_SYSCALL
    printf("MIPS: SYSCALL at PC=0x%" LL "x (RA=0x%" LL "x)\n"
           "   a0=0x%" LL "x, a1=0x%" LL "x, a2=0x%" LL "x, a3=0x%" LL "x\n",
           cpu->pc, cpu->gpr[MIPS_GPR_RA],
           cpu->gpr[MIPS_GPR_A0], cpu->gpr[MIPS_GPR_A1], cpu->gpr[MIPS_GPR_A2], cpu->gpr[MIPS_GPR_A3]);
#endif

    if (cpu->is_in_bdslot == 0)
        mips64_trigger_exception(cpu, MIPS_CP0_CAUSE_SYSCALL, 0);
    else
        mips64_trigger_exception(cpu, MIPS_CP0_CAUSE_SYSCALL, 1);
}

/* Trigger IRQs */
void forced_inline  mips64_trigger_irq(cpu_mips_t * cpu)
{
    if (mips64_update_irq_flag(cpu))
        mips64_trigger_exception(cpu, MIPS_CP0_CAUSE_INTERRUPT, 0);
}

/* Set an IRQ */
void mips64_set_irq(cpu_mips_t * cpu, m_uint8_t irq)
{
    m_uint32_t m;
    m = (1 << (irq + MIPS_CP0_CAUSE_ISHIFT)) & MIPS_CP0_CAUSE_IMASK;
    //atomic_or(&cpu->irq_cause,m);
    cpu->irq_cause |= m;


}

/* Clear an IRQ */
void mips64_clear_irq(cpu_mips_t * cpu, m_uint8_t irq)
{
    m_uint32_t m;

    m = (1 << (irq + MIPS_CP0_CAUSE_ISHIFT)) & MIPS_CP0_CAUSE_IMASK;
    cpu->irq_cause &= ~m;
    //atomic_and(&cpu->irq_cause,~m);

    if (!cpu->irq_cause)
        cpu->irq_pending = 0;
}
