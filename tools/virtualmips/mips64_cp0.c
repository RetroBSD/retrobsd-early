/*
 * Cisco router simulation platform.
 * Copyright (c) 2005,2006 Christophe Fillot (cf@utc.fr)
 *
 * MIPS Coprocessor 0 (System Coprocessor) implementation.
 * We don't use the JIT here, since there is no high performance needed.
 */

  /*
   * Copyright (C) yajin 2008 <yajinzhou@gmail.com >
   *     
   * This file is part of the virtualmips distribution. 
   * See LICENSE file for terms of the license. 
   *
   */


#define _GNU_SOURCE
#include<stdlib.h>
#include <assert.h>


#include "mips64.h"
#include "mips64_memory.h"
#include "mips64_cp0.h"
#include "cpu.h"



/* Get value of random register */
static inline u_int mips64_cp0_get_random_reg(cpu_mips_t * cpu)
{
   int random_value;
   random_value = (int) ((double) (cpu->cp0.tlb_entries) * rand() / (RAND_MAX + 1.0));
   return random_value;

}

/* Get a cp0 register (fast version) */
static inline m_cp0_reg_t mips64_cp0_get_reg_fast(cpu_mips_t * cpu, u_int cp0_reg, u_int sel)
{
   mips_cp0_t *cp0 = &cpu->cp0;
   switch (cp0_reg)
   {

   case MIPS_CP0_RANDOM:
      return (mips64_cp0_get_random_reg(cpu));
   case MIPS_CP0_CONFIG:
      ASSERT((1 << sel) & (cp0->config_usable), "Unimplemented configure register sel 0x%x\n", sel);
      return cp0->config_reg[sel];
   case MIPS_CP0_STATUS:
   default:
      return (cp0->reg[cp0_reg]);
   }
}



/* Get a cp0 register */
m_cp0_reg_t mips64_cp0_get_reg(cpu_mips_t * cpu, u_int cp0_reg)
{
   return (mips64_cp0_get_reg_fast(cpu, cp0_reg, 0));
}

/* MFC0 */
void mips64_cp0_exec_mfc0(cpu_mips_t * cpu, u_int gp_reg, u_int cp0_reg, u_int sel)
{
   cpu->gpr[gp_reg] = sign_extend(mips64_cp0_get_reg_fast(cpu, cp0_reg, sel), 32);

}

void mips64_cp0_exec_mtc0(cpu_mips_t * cpu, u_int gp_reg, u_int cp0_reg, u_int sel)
{
   mips64_cp0_set_reg(cpu, cp0_reg, sel, cpu->gpr[gp_reg] & 0xffffffff);
}


/* Set a cp0 register */
inline void mips64_cp0_set_reg(cpu_mips_t * cpu, u_int cp0_reg, u_int sel, m_uint32_t val)
{
   mips_cp0_t *cp0 = &cpu->cp0;

   switch (cp0_reg)
   {
   case MIPS_CP0_STATUS:
   case MIPS_CP0_CAUSE:

      cp0->reg[cp0_reg] = val;


      break;
   case MIPS_CP0_TLB_HI:
      cp0->reg[cp0_reg] = val;

      break;

   case MIPS_CP0_TLB_LO_0:
   case MIPS_CP0_TLB_LO_1:

      cp0->reg[cp0_reg] = val;
      break;
   case MIPS_CP0_PAGEMASK:
      cp0->reg[cp0_reg] = val;
      break;
   case MIPS_CP0_INDEX:
      cp0->reg[cp0_reg] = val;
      break;

   case MIPS_CP0_RANDOM:
   case MIPS_CP0_PRID:
      /* read only registers */
      break;

   case MIPS_CP0_COMPARE:

      //Write to compare will clear timer interrupt
      mips64_clear_irq(cpu, MIPS_TIMER_INTERRUPT);
      mips64_update_irq_flag(cpu);
      cp0->reg[cp0_reg] = val;

      break;
   case MIPS_CP0_EPC:
      cp0->reg[MIPS_CP0_EPC] = val;

      break;
   case MIPS_CP0_CONFIG:
      ASSERT((1 << sel) & (cp0->config_usable), "Unimplemented configure register sel 0x%x\n", sel);
      ASSERT(((sel != 1) && (sel != 2) && (sel != 3)), "Writing to read only configure register sel 0x%x\n", sel);
      if (sel == 0)
      {
         /*only bit 0:2 is writable */
         val &= 0x3;
         cp0->config_reg[sel] &= 0xfffffffc;
         cp0->config_reg[sel] += val;
      }
      else
         cp0->config_reg[sel] = val;

      break;
   case MIPS_CP0_WIRED:
      /* read only registers */

      break;

   default:
      cp0->reg[cp0_reg] = val;
   }
}



/* Get the VPN2 mask */
m_cp0_reg_t mips64_cp0_get_vpn2_mask(cpu_mips_t * cpu)
{
   if (cpu->addr_mode == 64)
      return ((m_cp0_reg_t) MIPS_TLB_VPN2_MASK_64);
   else
      return ((m_cp0_reg_t) MIPS_TLB_VPN2_MASK_32);
}


/* TLBP: Probe a TLB entry */
void  mips64_cp0_exec_tlbp(cpu_mips_t * cpu)
{
   mips_cp0_t *cp0 = &cpu->cp0;

   m_uint32_t vpn2, hi_reg, vpn2_mask, page_mask, hi_addr;
   tlb_entry_t *entry;
   u_int asid;
   int i;
   vpn2_mask = mips64_cp0_get_vpn2_mask(cpu);
   hi_reg = cp0->reg[MIPS_CP0_TLB_HI];
   asid = hi_reg & MIPS_TLB_ASID_MASK;
   vpn2 = hi_reg & vpn2_mask;

   cp0->reg[MIPS_CP0_INDEX] = 0x80000000;
   for (i = 0; i < cp0->tlb_entries; i++)
   {
      entry = &cp0->tlb[i];
      page_mask = ~(entry->mask + 0x1FFF);
      hi_addr = entry->hi & mips64_cp0_get_vpn2_mask(cpu);
      if (((vpn2 & page_mask) == (hi_addr & page_mask)) &&
          (((entry->hi & MIPS_TLB_G_MASK)) || ((entry->hi & MIPS_TLB_ASID_MASK) == asid)))
      {
         cp0->reg[MIPS_CP0_INDEX] = i;
         cp0->reg[MIPS_CP0_INDEX] &= ~0x80000000ULL;
         return;
#if DEBUG_TLB_ACTIVITY
         cpu_log(cpu, "", "CPU: CP0_TLBP returned %x\n", i);
#endif

      }


   }


}

/* Get the page size corresponding to a page mask */
static inline m_uint32_t get_page_size(m_uint32_t page_mask)
{
   return ((page_mask + 0x2000) >> 1);
}

void mips64_cp0_unmap_tlb_to_mts(cpu_mips_t * cpu, int index)
{
   m_va_t v0_addr, v1_addr;
   m_uint32_t page_size;
   tlb_entry_t *entry;

   entry = &cpu->cp0.tlb[index];

   page_size = get_page_size(entry->mask);
   v0_addr = entry->hi & mips64_cp0_get_vpn2_mask(cpu);
   v1_addr = v0_addr + page_size;


   if (entry->lo0 & MIPS_TLB_V_MASK)
      cpu->mts_unmap(cpu, v0_addr, page_size, MTS_ACC_T, index);

   if (entry->lo1 & MIPS_TLB_V_MASK)
      cpu->mts_unmap(cpu, v1_addr, page_size, MTS_ACC_T, index);
}


/* TLBW: Write a TLB entry */
static forced_inline void mips64_cp0_exec_tlbw(cpu_mips_t * cpu, u_int index)
{
   mips_cp0_t *cp0 = &cpu->cp0;
   tlb_entry_t *entry;

   if (index < cp0->tlb_entries)
   {
      entry = &cp0->tlb[index];

      /* Unmap the old entry if it was valid */
      mips64_cp0_unmap_tlb_to_mts(cpu, index);

      entry->mask = cp0->reg[MIPS_CP0_PAGEMASK];
      entry->hi = cp0->reg[MIPS_CP0_TLB_HI];
      entry->lo0 = cp0->reg[MIPS_CP0_TLB_LO_0];
      entry->lo1 = cp0->reg[MIPS_CP0_TLB_LO_1];
      /* if G bit is set in lo0 and lo1, set it in hi */
      if ((entry->lo0 & entry->lo1) & MIPS_CP0_LO_G_MASK)
         entry->hi |= MIPS_TLB_G_MASK;

      /* Clear G bit in TLB lo0 and lo1 */
      entry->lo0 &= ~MIPS_CP0_LO_G_MASK;
      entry->lo1 &= ~MIPS_CP0_LO_G_MASK;

   }
}



/* TLBWI: Write Indexed TLB entry */
void  mips64_cp0_exec_tlbwi(cpu_mips_t * cpu)
{
   m_uint32_t index;

/*FIX ME: 
  May be a bug in tlblhandler of 2.6.11.  by yajin.

 IN kernel, TLBL exception handler run a tlbp and then tlbw without checking tlbp is successful.
 assume  
 2aaa8a9c  lw  t0,(s0) 
 1.First we need to load instruction in 2aaa8a9c. A tlbl exception occurs. tlbp failed and tlbw
 write tlb entry into entry x.
 2. And then exceute the instruction. load a data from a memory. A tlbl exception occurs. tlbp failed
 and tlbw write tlb entry into entry x.  and return to pc=2aaa8a9c and do the process 1. 
 This will cause a infinate loop.

 In fact, we need to check whether is successfully. If success, tlbw. otherwise tlbwr.

 So I first check whether last tlbp failed(check highest bit of index register). If tlbp failed, write 
 tlbwr instead tlbw. It works well.
	 */

   if (cpu->cp0.reg[MIPS_CP0_INDEX] & 0x80000000)
   {
      mips64_cp0_exec_tlbwr(cpu);
   }
   else
   {

      index = cpu->cp0.reg[MIPS_CP0_INDEX];
      mips64_cp0_exec_tlbw(cpu, index);
   }


}

/* TLBWR: Write Random TLB entry */
void  mips64_cp0_exec_tlbwr(cpu_mips_t * cpu)
{
   mips64_cp0_exec_tlbw(cpu, mips64_cp0_get_random_reg(cpu));
}

/* TLBR: Read Indexed TLB entry */
void  mips64_cp0_exec_tlbr(cpu_mips_t * cpu)
{
   mips_cp0_t *cp0 = &cpu->cp0;
   tlb_entry_t *entry;
   u_int index;

   index = cp0->reg[MIPS_CP0_INDEX];

#if DEBUG_TLB_ACTIVITY
   cpu_log(cpu, "TLB", "CP0_TLBR: reading entry %u.\n", index);
#endif

   if (index < cp0->tlb_entries)
   {
      entry = &cp0->tlb[index];


      cp0->reg[MIPS_CP0_PAGEMASK] = entry->mask;
      cp0->reg[MIPS_CP0_TLB_HI] = entry->hi;
      cp0->reg[MIPS_CP0_TLB_LO_0] = entry->lo0;
      cp0->reg[MIPS_CP0_TLB_LO_1] = entry->lo1;
      if (entry->hi & MIPS_TLB_G_MASK)
      {
         cp0->reg[MIPS_CP0_TLB_LO_0] |= MIPS_CP0_LO_G_MASK;
         cp0->reg[MIPS_CP0_TLB_LO_1] |= MIPS_CP0_LO_G_MASK;
         cp0->reg[MIPS_CP0_TLB_HI] &= ~MIPS_TLB_G_MASK;
      }


   }
}



int mips64_cp0_tlb_lookup(cpu_mips_t * cpu, m_va_t vaddr, mts_map_t * res)
{
   mips_cp0_t *cp0 = &cpu->cp0;

   m_va_t vpn_addr, hi_addr, page_mask, page_size;
   tlb_entry_t *entry;
   u_int asid;
   int i;




   vpn_addr = vaddr & mips64_cp0_get_vpn2_mask(cpu);

   asid = cp0->reg[MIPS_CP0_TLB_HI] & MIPS_TLB_ASID_MASK;
   for (i = 0; i < cp0->tlb_entries; i++)
   {
      entry = &cp0->tlb[i];
      page_mask = ~(entry->mask + 0x1FFF);
      hi_addr = entry->hi & mips64_cp0_get_vpn2_mask(cpu);


      if (((vpn_addr & page_mask) == (hi_addr & page_mask)) &&
          ((entry->hi & MIPS_TLB_G_MASK) || ((entry->hi & MIPS_TLB_ASID_MASK) == asid)))
      {
         page_size = get_page_size(entry->mask);
         if ((vaddr & page_size) == 0)
         {
            res->tlb_index = i;

            res->vaddr = vaddr & MIPS_MIN_PAGE_MASK;
            res->paddr = (entry->lo0 & MIPS_TLB_PFN_MASK) << 6;
            res->paddr += ((res->vaddr) & (page_size - 1));
            res->paddr &= cpu->addr_bus_mask;

            res->dirty = (entry->lo0 & MIPS_TLB_D_MASK) >> MIPS_TLB_D_SHIT;
            res->valid = (entry->lo0 & MIPS_TLB_V_MASK) >> MIPS_TLB_V_SHIT;
            res->asid = asid;
            res->g_bit = entry->hi & MIPS_TLB_G_MASK;
            return (TRUE);
         }
         else
         {
            res->tlb_index = i;
            res->vaddr = vaddr & MIPS_MIN_PAGE_MASK;
            res->paddr = (entry->lo1 & MIPS_TLB_PFN_MASK) << 6;
            res->paddr += ((res->vaddr) & (page_size - 1));
            res->paddr &= cpu->addr_bus_mask;

            res->dirty = (entry->lo1 & MIPS_TLB_D_MASK) >> MIPS_TLB_D_SHIT;
            res->valid = (entry->lo1 & MIPS_TLB_V_MASK) >> MIPS_TLB_V_SHIT;

            res->asid = asid;
            res->g_bit = entry->hi & MIPS_TLB_G_MASK;

            return (TRUE);
         }
      }
   }

   return FALSE;


}

