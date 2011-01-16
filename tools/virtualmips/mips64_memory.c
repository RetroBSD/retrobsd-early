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


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <assert.h>

#include "cpu.h"
#include "vm.h"
#include "mips64_memory.h"
#include "device.h"
#include "utils.h"
#include "mips64_cp0.h"
#include "gdb_interface.h"

void bad_memory_access(cpu_mips_t * cpu, m_va_t vaddr)
{
    printf("cpu->pc  %x vaddr %x\n", cpu->pc, vaddr);
    if (cpu->vm->mipsy_debug_mode)
        bad_memory_access_gdb(cpu->vm);
    else
        assert(0);
}

/* MTS access with special access mask */
void mips_access_special(cpu_mips_t * cpu, m_va_t vaddr, m_uint32_t mask,
                         u_int op_code, u_int op_type, u_int op_size, m_reg_t * data, u_int * exc)
{
    m_reg_t vpn;
    m_uint8_t exc_code;


    switch (mask)
    {
    case MTS_ACC_U:

        if (op_type == MTS_READ)
            *data = 0;
        break;

    case MTS_ACC_T:
    case MTS_ACC_M:
    case MTS_ACC_AE:
        //if (op_code != MIPS_MEMOP_LOOKUP)
        //lookup also raise exception
        {
            cpu->cp0.reg[MIPS_CP0_BADVADDR] = vaddr;
            //clear vpn of entry hi
            cpu->cp0.reg[MIPS_CP0_TLB_HI] &= ~(mips64_cp0_get_vpn2_mask(cpu));
            //set VPN of entryhi
            vpn = vaddr & mips64_cp0_get_vpn2_mask(cpu);
            cpu->cp0.reg[MIPS_CP0_TLB_HI] |= vpn;

            //set context register
            cpu->cp0.reg[MIPS_CP0_CONTEXT] &= ~MIPS_CP0_CONTEXT_BADVPN2_MASK;
            vaddr = (vaddr >> 13) << 4;
            vaddr = vaddr & MIPS_CP0_CONTEXT_BADVPN2_MASK;
            cpu->cp0.reg[MIPS_CP0_CONTEXT] |= vaddr;

            if (mask == MTS_ACC_M)
                exc_code = MIPS_CP0_CAUSE_TLB_MOD;
            else if (mask == MTS_ACC_T)
            {
                if (op_type == MTS_READ)
                    exc_code = MIPS_CP0_CAUSE_TLB_LOAD;
                else
                    exc_code = MIPS_CP0_CAUSE_TLB_SAVE;
            }
            else if (mask == MTS_ACC_AE)
            {
                if (op_type == MTS_READ)
                    exc_code = MIPS_CP0_CAUSE_ADDR_LOAD;
                else
                    exc_code = MIPS_CP0_CAUSE_ADDR_SAVE;
            }
            else
                assert(0);


            mips64_trigger_exception(cpu, exc_code, cpu->is_in_bdslot);

        }

        *exc = 1;
        break;


    }
}


#if   0
64 bit operation.for future use.yajin
/* === MTS for 64-bit address space ======================================= */
#ifdef  MTS_ADDR_SIZE
#undef MTS_ADDR_SIZE
#endif
#define MTS_ADDR_SIZE      64
//#define MTS_ENTRY  (mts64_entry_t)
#define MTS_CACHE(cpu)  ( cpu->mts_u.mts64_cache )
/* Forward declarations */
static void *mips_mts64_access(cpu_mips_t * cpu, m_va_t vaddr,
                               u_int op_code, u_int op_size,
                               u_int op_type, m_reg_t * data, u_int * exc, m_uint8_t * has_set_value, u_int is_fromgdb);
static int mips_mts64_translate(cpu_mips_t * cpu, m_va_t vaddr, m_uint32_t * phys_page);

/* Initialize the MTS subsystem for the specified CPU */
int mips_mts64_init(cpu_mips_t * cpu)
{
    size_t len;

    /* Initialize the cache entries to 0 (empty) */
    len = MTS64_HASH_SIZE * sizeof(mts64_entry_t);
    if (!(MTS_CACHE(cpu) = malloc(len)))
        return (-1);

    memset(cpu->mts_u.mts64_cache, 0xFF, len);
    cpu->mts_lookups = 0;
    cpu->mts_misses = 0;
    return (0);
}

/* Free memory used by MTS */
void mips_mts64_shutdown(cpu_mips_t * cpu)
{
    /* Free the cache itself */
    free(cpu->mts_u.mts64_cache);
    cpu->mts_u.mts64_cache = NULL;
}

/* Show MTS detailed information (debugging only!) */
void mips_mts64_show_stats(cpu_mips_t * cpu)
{
#if DEBUG_MTS_MAP_VIRT
    mts64_entry_t *entry;
    u_int i, count;
#endif

    printf("\nCPU%u: MTS%d statistics:\n", cpu->id, MTS_ADDR_SIZE);

#if DEBUG_MTS_MAP_VIRT
    /* Valid hash entries */
    for (count = 0, i = 0; i < MTS64_HASH_SIZE; i++)
    {
        entry = &(cpu->mts_u.mts64_cache[i]);

        if (!(entry->gvpa & MTS_INV_ENTRY_MASK))
        {
            printf("    %4u: vaddr=0x%8.8llx, paddr=0x%8.8llx, hpa=%p\n",
                   i, (m_uint64_t) entry->gvpa, (m_uint64_t) entry->gppa, (void *) entry->hpa);
            count++;
        }
    }

    printf("   %u/%u valid hash entries.\n", count, MTS64_HASH_SIZE);
#endif

    printf("   Total lookups: %llu, misses: %llu, efficiency: %g%%\n",
           cpu->mts_lookups, cpu->mts_misses, 100 - ((double) (cpu->mts_misses * 100) / (double) cpu->mts_lookups));
}

/* Invalidate the complete MTS cache */
void mips_mts64_invalidate_cache(cpu_mips_t * cpu)
{
    size_t len;

    len = MTS64_HASH_SIZE * sizeof(mts64_entry_t);
    memset(cpu->mts_u.mts64_cache, 0xFF, len);
}

/* Invalidate partially the MTS cache, given a TLB entry index */
void mips_mts64_invalidate_tlb_entry(cpu_mips_t * cpu, u_int tlb_index)
{
    mips_mts64_invalidate_cache(cpu);
}

/* 
 * MTS mapping.
 *
 * It is NOT inlined since it triggers a GCC bug on my config (x86, GCC 3.3.5)
 */
static no_inline mts64_entry_t *mips_mts64_map(cpu_mips_t * cpu, u_int op_type, mts_map_t * map,
                                               mts64_entry_t * entry, mts64_entry_t * alt_entry)
{
    struct vdevice *dev;
    m_uint32_t offset;
    // m_iptr_t host_ptr;
    // int cow;

    if (!(dev = dev_lookup(cpu->vm, map->paddr)))
        return NULL;

    /* if (dev->flags & VDEVICE_FLAG_SPARSE) {
       host_ptr = dev_sparse_get_host_addr(cpu->vm,dev,map->paddr,op_type,&cow);

       entry->gvpa  = map->vaddr;
       entry->gppa  = map->paddr;
       entry->hpa   = host_ptr;
       entry->flags = (cow) ? MTS_FLAG_COW : 0;
       return entry;
       } */

    if (!dev->host_addr || (dev->flags & VDEVICE_FLAG_NO_MTS_MMAP))
    {
        offset = map->paddr - dev->phys_addr;

        alt_entry->gvpa = map->vaddr;
        alt_entry->gppa = map->paddr;
        alt_entry->hpa = (dev->id << MTS_DEVID_SHIFT) + offset;
        alt_entry->flags = MTS_FLAG_DEV;
        return alt_entry;
    }

    entry->gvpa = map->vaddr;
    entry->gppa = map->paddr;
    entry->hpa = dev->host_addr + (map->paddr - dev->phys_addr);
    entry->flags = 0;

    return entry;
}

/* MTS lookup */
static void *mips_mts64_lookup(cpu_mips_t * cpu, m_va_t vaddr)
{
    m_reg_t data;
    u_int exc;
    m_uint8_t has_set_value = FALSE;
    return (mips_mts64_access(cpu, vaddr, MIPS_MEMOP_LOOKUP, 4, MTS_READ, &data, &exc, &has_set_value, 0));
}

/* === MIPS Memory Operations ============================================= */

/* LB: Load Byte */
u_int mips_mts64_gdb_lb(cpu_mips_t * cpu, m_va_t vaddr, void *cur)
{
    // m_reg_t data;
    void *haddr;
    u_int exc;
    m_uint8_t has_set_value = FALSE;

    haddr = mips_mts64_access(cpu, vaddr, MIPS_MEMOP_LB, 1, MTS_READ, (m_reg_t *) cur, &exc, &has_set_value, 1);

    if ((exc) || (haddr == NULL))
        *(m_uint8_t *) cur = 0x0;
    else
        *(m_uint8_t *) cur = (*(m_uint8_t *) haddr);

    return (0);
}


/* LB: Load Byte */
u_int mips_mts64_lb(cpu_mips_t * cpu, m_va_t vaddr, u_int reg)
{
    m_reg_t data;
    void *haddr;
    u_int exc;
    m_uint8_t has_set_value = FALSE;


    haddr = mips_mts64_access(cpu, vaddr, MIPS_MEMOP_LB, 1, MTS_READ, &data, &exc, &has_set_value, 0);
    if (exc)
        return exc;
    if ((haddr == NULL) && (has_set_value == FALSE))
    {
        bad_memory_access(cpu, vaddr);
    }
    if (likely(haddr != NULL))
        data = *(m_uint8_t *) haddr;
    if (likely(!exc))
        cpu->gpr[reg] = sign_extend(data, 8);
    return (0);
}

/* LBU: Load Byte Unsigned */
u_int mips_mts64_lbu(cpu_mips_t * cpu, m_va_t vaddr, u_int reg)
{
    m_reg_t data;
    void *haddr;
    u_int exc;
    m_uint8_t has_set_value = FALSE;

    haddr = mips_mts64_access(cpu, vaddr, MIPS_MEMOP_LBU, 1, MTS_READ, &data, &exc, &has_set_value, 0);
    if (exc)
        return exc;
    if ((haddr == NULL) && (has_set_value == FALSE))
    {
        bad_memory_access(cpu, vaddr);
    }
    if (likely(haddr != NULL))
        data = *(m_uint8_t *) haddr;
    if (likely(!exc))
        cpu->gpr[reg] = data & 0xff;
    return (0);
}

/* LH: Load Half-Word */
u_int mips_mts64_lh(cpu_mips_t * cpu, m_va_t vaddr, u_int reg)
{
    m_reg_t data;
    void *haddr;
    u_int exc;
    m_uint8_t has_set_value = FALSE;

    haddr = mips_mts64_access(cpu, vaddr, MIPS_MEMOP_LH, 2, MTS_READ, &data, &exc, &has_set_value, 0);
    if (exc)
        return exc;
    if ((haddr == NULL) && (has_set_value == FALSE))
    {
        bad_memory_access(cpu, vaddr);
    }
    if (likely(haddr != NULL))
        data = vmtoh16(*(m_uint16_t *) haddr);
    if (likely(!exc))
        cpu->gpr[reg] = sign_extend(data, 16);
    return (0);
}

/* LHU: Load Half-Word Unsigned */
u_int mips_mts64_lhu(cpu_mips_t * cpu, m_va_t vaddr, u_int reg)
{
    m_reg_t data;
    void *haddr;
    u_int exc;
    m_uint8_t has_set_value = FALSE;

    haddr = mips_mts64_access(cpu, vaddr, MIPS_MEMOP_LHU, 2, MTS_READ, &data, &exc, &has_set_value, 0);
    if (exc)
        return exc;
    if ((haddr == NULL) && (has_set_value == FALSE))
    {
        bad_memory_access(cpu, vaddr);
    }
    if (likely(haddr != NULL))
        data = vmtoh16(*(m_uint16_t *) haddr);
    if (likely(!exc))
        cpu->gpr[reg] = data & 0xffff;
    return (exc);
}

/* LW: Load Word */
u_int mips_mts64_lw(cpu_mips_t * cpu, m_va_t vaddr, u_int reg)
{
    m_reg_t data;
    void *haddr;
    u_int exc;
    m_uint8_t has_set_value = FALSE;

    haddr = mips_mts64_access(cpu, vaddr, MIPS_MEMOP_LW, 4, MTS_READ, &data, &exc, &has_set_value, 0);
    if (exc)
        return exc;
    if ((haddr == NULL) && (has_set_value == FALSE))
    {
        bad_memory_access(cpu, vaddr);
    }
    if (likely(haddr != NULL))
        data = vmtoh32(*(m_uint32_t *) haddr);
    if (likely(!exc))
        cpu->gpr[reg] = sign_extend(data, 32);
    return (exc);
}

/* LWU: Load Word Unsigned */
u_int mips_mts64_lwu(cpu_mips_t * cpu, m_va_t vaddr, u_int reg)
{
    m_reg_t data;
    void *haddr;
    u_int exc;
    m_uint8_t has_set_value = FALSE;

    haddr = mips_mts64_access(cpu, vaddr, MIPS_MEMOP_LWU, 4, MTS_READ, &data, &exc, &has_set_value, 0);
    if (exc)
        return exc;
    if ((haddr == NULL) && (has_set_value == FALSE))
    {
        bad_memory_access(cpu, vaddr);
    }
    if (likely(haddr != NULL))
        data = vmtoh32(*(m_uint32_t *) haddr);
    if (likely(!exc))
        cpu->gpr[reg] = data & 0xffffffff;
    return (exc);
}

/* LD: Load Double-Word */
u_int mips_mts64_ld(cpu_mips_t * cpu, m_va_t vaddr, u_int reg)
{
    m_reg_t data;
    void *haddr;
    u_int exc;
    m_uint8_t has_set_value = FALSE;

    haddr = mips_mts64_access(cpu, vaddr, MIPS_MEMOP_LD, 8, MTS_READ, &data, &exc, &has_set_value, 0);
    if (exc)
        return exc;
    if ((haddr == NULL) && (has_set_value == FALSE))
    {
        bad_memory_access(cpu, vaddr);
    }
    if (likely(haddr != NULL))
        data = (m_reg_t) (vmtoh64(*(m_uint64_t *) haddr));
    if (likely(!exc))
        cpu->gpr[reg] = data;
    return (exc);
}

/* SB: Store Byte */
u_int mips_mts64_sb(cpu_mips_t * cpu, m_va_t vaddr, u_int reg)
{
    m_reg_t data;
    void *haddr;
    u_int exc;
    m_uint8_t has_set_value = FALSE;

    data = cpu->gpr[reg] & 0xff;
    haddr = mips_mts64_access(cpu, vaddr, MIPS_MEMOP_SB, 1, MTS_WRITE, &data, &exc, &has_set_value, 0);
    if (exc)
        return exc;
    if ((haddr == NULL) && (has_set_value == FALSE))
    {
        bad_memory_access(cpu, vaddr);
    }
    if (likely(haddr != NULL))
        *(m_uint8_t *) haddr = data;
    return (exc);
}

/* SH: Store Half-Word */
u_int mips_mts64_sh(cpu_mips_t * cpu, m_va_t vaddr, u_int reg)
{
    m_reg_t data;
    void *haddr;
    u_int exc;
    m_uint8_t has_set_value = FALSE;

    data = cpu->gpr[reg] & 0xffff;

    haddr = mips_mts64_access(cpu, vaddr, MIPS_MEMOP_SH, 2, MTS_WRITE, &data, &exc, &has_set_value, 0);

    if (exc)
        return exc;
    if ((haddr == NULL) && (has_set_value == FALSE))
    {
        bad_memory_access(cpu, vaddr);
    }
    if (likely(haddr != NULL))
        *(m_uint16_t *) haddr = htovm16(data);
    return (exc);
}

/* SW: Store Word */
u_int mips_mts64_sw(cpu_mips_t * cpu, m_va_t vaddr, u_int reg)
{
    m_reg_t data;
    void *haddr;
    u_int exc;
    m_uint8_t has_set_value = FALSE;

    data = cpu->gpr[reg] & 0xffffffff;
    haddr = mips_mts64_access(cpu, vaddr, MIPS_MEMOP_SW, 4, MTS_WRITE, &data, &exc, &has_set_value, 0);
    if (exc)
        return exc;
    if ((haddr == NULL) && (has_set_value == FALSE))
    {
        bad_memory_access(cpu, vaddr);
    }
    if (likely(haddr != NULL))
        *(m_uint32_t *) haddr = htovm32(data);
    return (exc);
}

/* SD: Store Double-Word */
u_int mips_mts64_sd(cpu_mips_t * cpu, m_va_t vaddr, u_int reg)
{
    m_reg_t data;
    void *haddr;
    u_int exc;
    m_uint8_t has_set_value = FALSE;

    data = cpu->gpr[reg];
    haddr = mips_mts64_access(cpu, vaddr, MIPS_MEMOP_SD, 8, MTS_WRITE, &data, &exc, &has_set_value, 0);
    if (exc)
        return exc;
    if ((haddr == NULL) && (has_set_value == FALSE))
    {
        bad_memory_access(cpu, vaddr);
    }
    if (likely(haddr != NULL))
        *(m_reg_t *) (m_uint64_t *) haddr = (m_reg_t) (htovm64(data));
    return (exc);
}

/* LDC1: Load Double-Word To Coprocessor 1 */
u_int mips_mts64_ldc1(cpu_mips_t * cpu, m_va_t vaddr, u_int reg)
{
    /* m_uint64_t data;
       void *haddr;
       u_int exc;
       m_uint8_t has_set_value=FALSE;

       haddr = mips_mts64_access(cpu,vaddr,MIPS_MEMOP_LDC1,8,MTS_READ,&data,&exc,&has_set_value);
       if ((haddr==NULL)&&(has_set_value==FALSE))  
       { 
       bad_memory_access(cpu,vaddr);
       }
       if (likely(haddr != NULL)) data = vmtoh64(*(m_uint64_t *)haddr);
       if (likely(!exc)) cpu->fpu.reg[reg] = data;
       return(exc); */
    printf("ldc1 pc %x\n", cpu->pc);
    exit(-1);
    return 0;
}

/* LWL: Load Word Left */
u_int mips_mts64_lwl(cpu_mips_t * cpu, m_va_t vaddr, u_int reg)
{
    m_reg_t r_mask, naddr;
    m_reg_t data;
    u_int m_shift;
    void *haddr;
    u_int exc;
    m_uint8_t has_set_value = FALSE;

    naddr = vaddr & ~(0x03);
    haddr = mips_mts64_access(cpu, naddr, MIPS_MEMOP_LWL, 4, MTS_READ, &data, &exc, &has_set_value, 0);
    if (exc)
        return exc;
    if ((haddr == NULL) && (has_set_value == FALSE))
    {
        bad_memory_access(cpu, vaddr);
    }

    if (likely(haddr != NULL))
        data = vmtoh32(*(m_uint32_t *) haddr);

    if (likely(!exc))
    {
        m_shift = (vaddr & 0x03) << 3;
        r_mask = (1ULL << m_shift) - 1;
        data <<= m_shift;

        cpu->gpr[reg] &= r_mask;
        cpu->gpr[reg] |= data;
        cpu->gpr[reg] = sign_extend(cpu->gpr[reg], 32);
    }
    return (exc);
}

/* LWR: Load Word Right */
u_int mips_mts64_lwr(cpu_mips_t * cpu, m_va_t vaddr, u_int reg)
{
    m_reg_t r_mask, naddr;
    m_reg_t data;
    u_int m_shift;
    void *haddr;
    u_int exc;
    m_uint8_t has_set_value = FALSE;

    naddr = vaddr & ~(0x03);
    haddr = mips_mts64_access(cpu, naddr, MIPS_MEMOP_LWR, 4, MTS_READ, &data, &exc, &has_set_value, 0);
    if (exc)
        return exc;
    if ((haddr == NULL) && (has_set_value == FALSE))
    {
        bad_memory_access(cpu, vaddr);
    }

    if (likely(haddr != NULL))
        data = vmtoh32(*(m_uint32_t *) haddr);

    if (likely(!exc))
    {
        m_shift = ((vaddr & 0x03) + 1) << 3;
        r_mask = (1ULL << m_shift) - 1;

        data = sign_extend(data >> (32 - m_shift), 32);
        r_mask = sign_extend(r_mask, 32);

        cpu->gpr[reg] &= ~r_mask;
        cpu->gpr[reg] |= data;
    }
    return (exc);
}

/* LDL: Load Double-Word Left */
u_int mips_mts64_ldl(cpu_mips_t * cpu, m_va_t vaddr, u_int reg)
{
    m_reg_t r_mask, naddr;
    m_reg_t data;
    u_int m_shift;
    void *haddr;
    u_int exc;
    m_uint8_t has_set_value = FALSE;

    naddr = vaddr & ~(0x07);
    haddr = mips_mts64_access(cpu, naddr, MIPS_MEMOP_LDL, 8, MTS_READ, &data, &exc, &has_set_value, 0);
    if (exc)
        return exc;
    if ((haddr == NULL) && (has_set_value == FALSE))
    {
        bad_memory_access(cpu, vaddr);
    }

    if (likely(haddr != NULL))
        data = vmtoh64(*(m_uint64_t *) haddr);

    if (likely(!exc))
    {
        m_shift = (vaddr & 0x07) << 3;
        r_mask = (1ULL << m_shift) - 1;
        data <<= m_shift;

        cpu->gpr[reg] &= r_mask;
        cpu->gpr[reg] |= data;
    }
    return (exc);
}

/* LDR: Load Double-Word Right */
u_int mips_mts64_ldr(cpu_mips_t * cpu, m_va_t vaddr, u_int reg)
{
    m_reg_t r_mask, naddr;
    m_reg_t data;
    u_int m_shift;
    void *haddr;
    u_int exc;
    m_uint8_t has_set_value = FALSE;

    naddr = vaddr & ~(0x07);
    haddr = mips_mts64_access(cpu, naddr, MIPS_MEMOP_LDR, 8, MTS_READ, &data, &exc, &has_set_value, 0);
    if (exc)
        return exc;
    if ((haddr == NULL) && (has_set_value == FALSE))
    {
        bad_memory_access(cpu, vaddr);
    }

    if (likely(haddr != NULL))
        data = vmtoh64(*(m_uint64_t *) haddr);

    if (likely(!exc))
    {
        m_shift = ((vaddr & 0x07) + 1) << 3;
        r_mask = (1ULL << m_shift) - 1;
        data >>= (64 - m_shift);

        cpu->gpr[reg] &= ~r_mask;
        cpu->gpr[reg] |= data;
    }
    return (exc);
}

/* SWL: Store Word Left */
u_int mips_mts64_swl(cpu_mips_t * cpu, m_va_t vaddr, u_int reg)
{
    m_reg_t d_mask, naddr;
    m_reg_t data;
    u_int r_shift;
    void *haddr;
    u_int exc;
    m_uint8_t has_set_value = FALSE;

    naddr = vaddr & ~(0x03ULL);
    haddr = mips_mts64_access(cpu, naddr, MIPS_MEMOP_SWL, 4, MTS_READ, &data, &exc, &has_set_value, 0);
    if (exc)
        return exc;
    if ((haddr == NULL) && (has_set_value == FALSE))
    {
        bad_memory_access(cpu, vaddr);
    }
    if (unlikely(exc))
        return (exc);

    if (likely(haddr != NULL))
        data = vmtoh32(*(m_uint32_t *) haddr);

    r_shift = (vaddr & 0x03) << 3;
    d_mask = 0xffffffff >> r_shift;

    data &= ~d_mask;
    data |= (cpu->gpr[reg] & 0xffffffff) >> r_shift;

    haddr = mips_mts64_access(cpu, naddr, MIPS_MEMOP_SWL, 4, MTS_WRITE, &data, &exc, &has_set_value, 0);
    if ((haddr == NULL) && (has_set_value == FALSE))
    {
        bad_memory_access(cpu, vaddr);
    }
    if (likely(haddr != NULL))
        *(m_uint32_t *) haddr = htovm32(data);
    return (exc);
}

/* SWR: Store Word Right */
u_int mips_mts64_swr(cpu_mips_t * cpu, m_va_t vaddr, u_int reg)
{
    m_reg_t d_mask, naddr;
    m_reg_t data;
    u_int r_shift;
    void *haddr;
    u_int exc;
    m_uint8_t has_set_value = FALSE;

    naddr = vaddr & ~(0x03);
    haddr = mips_mts64_access(cpu, naddr, MIPS_MEMOP_SWR, 4, MTS_READ, &data, &exc, &has_set_value, 0);
    if (exc)
        return exc;
    if ((haddr == NULL) && (has_set_value == FALSE))
    {
        bad_memory_access(cpu, vaddr);
    }
    if (unlikely(exc))
        return (exc);

    if (likely(haddr != NULL))
        data = vmtoh32(*(m_uint32_t *) haddr);

    r_shift = ((vaddr & 0x03) + 1) << 3;
    d_mask = 0xffffffff >> r_shift;

    data &= d_mask;
    data |= (cpu->gpr[reg] << (32 - r_shift)) & 0xffffffff;

    haddr = mips_mts64_access(cpu, naddr, MIPS_MEMOP_SWR, 4, MTS_WRITE, &data, &exc, &has_set_value, 0);
    if ((haddr == NULL) && (has_set_value == FALSE))
    {
        bad_memory_access(cpu, vaddr);
    }
    if (likely(haddr != NULL))
        *(m_uint32_t *) haddr = htovm32(data);
    return (exc);
}

/* SDL: Store Double-Word Left */
u_int mips_mts64_sdl(cpu_mips_t * cpu, m_va_t vaddr, u_int reg)
{
    m_reg_t d_mask, naddr;
    m_reg_t data;
    u_int r_shift;
    void *haddr;
    u_int exc;
    m_uint8_t has_set_value = FALSE;

    naddr = vaddr & ~(0x07);
    haddr = mips_mts64_access(cpu, naddr, MIPS_MEMOP_SDL, 8, MTS_READ, &data, &exc, &has_set_value, 0);
    if (exc)
        return exc;
    if ((haddr == NULL) && (has_set_value == FALSE))
    {
        bad_memory_access(cpu, vaddr);
    }
    if (unlikely(exc))
        return (exc);

    if (likely(haddr != NULL))
        data = (m_reg_t) (vmtoh64(*(m_uint64_t *) haddr));

    r_shift = (vaddr & 0x07) << 3;
    d_mask = 0xffffffffffffffffULL >> r_shift;

    data &= ~d_mask;
    data |= cpu->gpr[reg] >> r_shift;

    haddr = mips_mts64_access(cpu, naddr, MIPS_MEMOP_SDL, 8, MTS_WRITE, &data, &exc, &has_set_value, 0);
    if ((haddr == NULL) && (has_set_value == FALSE))
    {
        bad_memory_access(cpu, vaddr);
    }
    if (likely(haddr != NULL))
        *(m_reg_t *) (m_uint64_t *) haddr = (m_reg_t) (htovm64(data));
    return (exc);
}

/* SDR: Store Double-Word Right */
u_int mips_mts64_sdr(cpu_mips_t * cpu, m_va_t vaddr, u_int reg)
{
    m_reg_t d_mask, naddr;
    m_reg_t data;
    u_int r_shift;
    void *haddr;
    u_int exc;
    m_uint8_t has_set_value = FALSE;

    naddr = vaddr & ~(0x07);
    haddr = mips_mts64_access(cpu, naddr, MIPS_MEMOP_SDR, 8, MTS_READ, &data, &exc, &has_set_value, 0);
    if (exc)
        return exc;
    if ((haddr == NULL) && (has_set_value == FALSE))
    {
        bad_memory_access(cpu, vaddr);
    }
    if (unlikely(exc))
        return (exc);

    if (likely(haddr != NULL))
        data = vmtoh64(*(m_uint64_t *) haddr);

    r_shift = ((vaddr & 0x07) + 1) << 3;
    d_mask = 0xffffffffffffffffULL >> r_shift;

    data &= d_mask;
    data |= cpu->gpr[reg] << (64 - r_shift);

    haddr = mips_mts64_access(cpu, naddr, MIPS_MEMOP_SDR, 8, MTS_WRITE, &data, &exc, &has_set_value, 0);
    if ((haddr == NULL) && (has_set_value == FALSE))
    {
        bad_memory_access(cpu, vaddr);
    }
    if (likely(haddr != NULL))
        *(m_reg_t *) (m_uint64_t *) haddr = (m_reg_t) (htovm64(data));
    return (exc);
}

/* LL: Load Linked */
u_int mips_mts64_ll(cpu_mips_t * cpu, m_va_t vaddr, u_int reg)
{
    m_reg_t data;
    void *haddr;
    u_int exc;
    m_uint8_t has_set_value = FALSE;

    haddr = mips_mts64_access(cpu, vaddr, MIPS_MEMOP_LL, 4, MTS_READ, &data, &exc, &has_set_value, 0);
    if (exc)
        return exc;
    if ((haddr == NULL) && (has_set_value == FALSE))
    {
        bad_memory_access(cpu, vaddr);
    }
    if (likely(haddr != NULL))
        data = vmtoh32(*(m_uint32_t *) haddr);

    if (likely(!exc))
    {
        cpu->gpr[reg] = sign_extend(data, 32);
        cpu->ll_bit = 1;
    }

    return (exc);
}

/* SC: Store Conditional */
u_int mips_mts64_sc(cpu_mips_t * cpu, m_va_t vaddr, u_int reg)
{
    m_reg_t data;
    void *haddr;
    u_int exc = 0;
    m_uint8_t has_set_value = FALSE;

    if (cpu->ll_bit)
    {
        data = cpu->gpr[reg] & 0xffffffff;
        haddr = mips_mts64_access(cpu, vaddr, MIPS_MEMOP_SC, 4, MTS_WRITE, &data, &exc, &has_set_value, 0);
        if (exc)
            return exc;
        if ((haddr == NULL) && (has_set_value == FALSE))
        {
            bad_memory_access(cpu, vaddr);
        }
        if (likely(haddr != NULL))
            *(m_uint32_t *) haddr = htovm32(data);
    }

    if (likely(!exc))
        cpu->gpr[reg] = cpu->ll_bit;
    return (exc);
}

/* SDC1: Store Double-Word from Coprocessor 1 */
u_int mips_mts64_sdc1(cpu_mips_t * cpu, m_va_t vaddr, u_int reg)
{
    /*   m_uint64_t data;
       void *haddr;
       u_int exc;
       m_uint8_t has_set_value=FALSE;

       data = cpu->fpu.reg[reg];
       haddr = mips_mts64_access(cpu,vaddr,MIPS_MEMOP_SDC1,8,MTS_WRITE,
       &data,&exc,&has_set_value);
       if ((haddr==NULL)&&(has_set_value==FALSE))  
       { 
       bad_memory_access(cpu,vaddr);
       }
       if (likely(haddr != NULL)) *(m_uint64_t *)haddr = htovm64(data);
       return(exc); */
    printf("sdc1 pc %x\n", cpu->pc);
    exit(-1);
    return 0;
}

/* CACHE: Cache operation */
u_int mips_mts64_cache(cpu_mips_t * cpu, m_va_t vaddr, u_int op)
{
    /* mips64_jit_tcb_t *block;
       m_uint32_t pc_hash;

       #if DEBUG_CACHE
       cpu_log(cpu->gen,
       "MTS","CACHE: PC=0x%llx, vaddr=0x%llx, cache=%u, code=%u\n",
       cpu->pc, vaddr, op & 0x3, op >> 2);
       #endif

       if (cpu->exec_blk_map) {
       pc_hash = mips64_jit_get_pc_hash(vaddr);
       block = cpu->exec_blk_map[pc_hash];

       if (block && (block->start_pc == (vaddr & MIPS_MIN_PAGE_MASK))) {
       #if DEBUG_CACHE
       cpu_log(cpu->gen,"MTS",
       "CACHE: removing compiled page at 0x%llx, pc=0x%llx\n",
       block->start_pc,cpu->pc);
       #endif
       cpu->exec_blk_map[pc_hash] = NULL;
       mips64_jit_tcb_free(cpu,block,TRUE);
       }
       else
       {
       #if DEBUG_CACHE
       cpu_log(cpu->gen,"MTS",
       "CACHE: trying to remove page 0x%llx with pc=0x%llx\n",
       vaddr, cpu->pc);
       #endif

       }
       } */
    printf("cache pc %x\n", cpu->pc);
    exit(-1);
    return 0;

}

/* === MTS Cache Management ============================================= */

/* MTS map/unmap/rebuild "API" functions */
void mips_mts64_api_map(cpu_mips_t * cpu, m_va_t vaddr, m_pa_t paddr, m_uint32_t len, int cache_access, int tlb_index)
{
    /* nothing to do, the cache will be filled on-the-fly */
}

void mips_mts64_api_unmap(cpu_mips_t * cpu, m_va_t vaddr, m_uint32_t len, m_uint32_t val, int tlb_index)
{
    /* Invalidate the TLB entry or the full cache if no index is specified */
    if (tlb_index != -1)
        mips_mts64_invalidate_tlb_entry(cpu, tlb_index);
    else
        mips_mts64_invalidate_cache(cpu);
}

void mips_mts64_api_rebuild(cpu_mips_t * cpu)
{
    mips_mts64_invalidate_cache(cpu);
}

/* ======================================================================== */

/* Initialize memory access vectors */
void mips_mts64_init_memop_vectors(cpu_mips_t * cpu)
{
    /* XXX TODO:
     *  - LD/SD forbidden in Supervisor/User modes with 32-bit addresses.
     */

    cpu->addr_mode = 64;

    /* API vectors */
    cpu->mts_map = mips_mts64_api_map;
    cpu->mts_unmap = mips_mts64_api_unmap;

    /* Memory lookup operation */
    cpu->mem_op_lookup = mips_mts64_lookup;

    /* Translation operation */
    cpu->translate = mips_mts64_translate;

    /* Shutdown operation */
    cpu->mts_shutdown = mips_mts64_shutdown;

    /* Rebuild MTS data structures */
    cpu->mts_rebuild = mips_mts64_api_rebuild;

    /* Show statistics */
    cpu->mts_show_stats = mips_mts64_show_stats;

    cpu->mips_mts_gdb_lb = mips_mts64_gdb_lb;

    /* Load Operations */
    cpu->mem_op_fn[MIPS_MEMOP_LB] = mips_mts64_lb;
    cpu->mem_op_fn[MIPS_MEMOP_LBU] = mips_mts64_lbu;
    cpu->mem_op_fn[MIPS_MEMOP_LH] = mips_mts64_lh;
    cpu->mem_op_fn[MIPS_MEMOP_LHU] = mips_mts64_lhu;
    cpu->mem_op_fn[MIPS_MEMOP_LW] = mips_mts64_lw;
    cpu->mem_op_fn[MIPS_MEMOP_LWU] = mips_mts64_lwu;
    cpu->mem_op_fn[MIPS_MEMOP_LD] = mips_mts64_ld;
    cpu->mem_op_fn[MIPS_MEMOP_LDL] = mips_mts64_ldl;
    cpu->mem_op_fn[MIPS_MEMOP_LDR] = mips_mts64_ldr;

    /* Store Operations */
    cpu->mem_op_fn[MIPS_MEMOP_SB] = mips_mts64_sb;
    cpu->mem_op_fn[MIPS_MEMOP_SH] = mips_mts64_sh;
    cpu->mem_op_fn[MIPS_MEMOP_SW] = mips_mts64_sw;
    cpu->mem_op_fn[MIPS_MEMOP_SD] = mips_mts64_sd;

    /* Load Left/Right operations */
    cpu->mem_op_fn[MIPS_MEMOP_LWL] = mips_mts64_lwl;
    cpu->mem_op_fn[MIPS_MEMOP_LWR] = mips_mts64_lwr;
    cpu->mem_op_fn[MIPS_MEMOP_LDL] = mips_mts64_ldl;
    cpu->mem_op_fn[MIPS_MEMOP_LDR] = mips_mts64_ldr;

    /* Store Left/Right operations */
    cpu->mem_op_fn[MIPS_MEMOP_SWL] = mips_mts64_swl;
    cpu->mem_op_fn[MIPS_MEMOP_SWR] = mips_mts64_swr;
    cpu->mem_op_fn[MIPS_MEMOP_SDL] = mips_mts64_sdl;
    cpu->mem_op_fn[MIPS_MEMOP_SDR] = mips_mts64_sdr;

    /* LL/SC - Load Linked / Store Conditional */
    cpu->mem_op_fn[MIPS_MEMOP_LL] = mips_mts64_ll;
    cpu->mem_op_fn[MIPS_MEMOP_SC] = mips_mts64_sc;

    /* Coprocessor 1 memory access functions */
    cpu->mem_op_fn[MIPS_MEMOP_LDC1] = mips_mts64_ldc1;
    cpu->mem_op_fn[MIPS_MEMOP_SDC1] = mips_mts64_sdc1;

    /* Cache Operation */
    cpu->mem_op_fn[MIPS_MEMOP_CACHE] = mips_mts64_cache;
}








#endif








/* === MTS for 32-bit address space ======================================= */
#ifdef  MTS_ADDR_SIZE
#undef MTS_ADDR_SIZE
#endif
#define MTS_ADDR_SIZE      32
/* Forward declarations */
static void *mips_mts32_access(cpu_mips_t * cpu, m_va_t vaddr,
                               u_int op_code, u_int op_size,
                               u_int op_type, m_reg_t * data, u_int * exc, m_uint8_t * has_set_value, u_int is_fromgdb);


static int mips_mts32_translate(cpu_mips_t * cpu, m_va_t vaddr, m_uint32_t * phys_page);

/* Initialize the MTS subsystem for the specified CPU */
int mips_mts32_init(cpu_mips_t * cpu)
{
    size_t len;

    /* Initialize the cache entries to 0 (empty) */
    len = MTS32_HASH_SIZE * sizeof(mts32_entry_t);
    if (!(cpu->mts_u.mts32_cache = malloc(len)))
        return (-1);

    memset(cpu->mts_u.mts32_cache, 0xFF, len);
    cpu->mts_lookups = 0;
    cpu->mts_misses = 0;
    return (0);
}

/* Free memory used by MTS */
void mips_mts32_shutdown(cpu_mips_t * cpu)
{
    /* Free the cache itself */
    free(cpu->mts_u.mts32_cache);
    cpu->mts_u.mts32_cache = NULL;
}

/* Show MTS detailed information (debugging only!) */
void mips_mts32_show_stats(cpu_mips_t * cpu)
{

#if DEBUG_MTS_MAP_VIRT
    mts32_entry_t *entry;
    u_int i, count;
#endif

    printf("\nCPU%u: MTS%d statistics:\n", cpu->id, MTS_ADDR_SIZE);

#if DEBUG_MTS_MAP_VIRT
    /* Valid hash entries */
    for (count = 0, i = 0; i < MTS32_HASH_SIZE; i++)
    {
        entry = &(cpu->mts_u.mts32_cache[i]);

        if (!(entry->gvpa & MTS_INV_ENTRY_MASK))
        {
            printf("    %4u: vaddr=0x%8.8llx, paddr=0x%8.8llx, hpa=%p\n",
                   i, (m_uint64_t) entry->gvpa, (m_uint64_t) entry->gppa, (void *) entry->hpa);
            count++;
        }
    }

    printf("   %u/%u valid hash entries.\n", count, MTS32_HASH_SIZE);
#endif

    printf("   Total lookups: %llu, misses: %llu, efficiency: %g%%\n",
           cpu->mts_lookups, cpu->mts_misses, 100 - ((double) (cpu->mts_misses * 100) / (double) cpu->mts_lookups));
}

/* Invalidate the complete MTS cache */
void mips_mts32_invalidate_cache(cpu_mips_t * cpu)
{
    size_t len;

    len = MTS32_HASH_SIZE * sizeof(mts32_entry_t);
    memset(cpu->mts_u.mts32_cache, 0xFF, len);
}

/* Invalidate partially the MTS cache, given a TLB entry index */
void mips_mts32_invalidate_tlb_entry(cpu_mips_t * cpu, m_va_t vaddr)
{

    mts32_entry_t *entry;
    m_uint32_t hash_bucket;

    hash_bucket = MTS32_HASH(vaddr);
    entry = &cpu->mts_u.mts32_cache[hash_bucket];
    memset(entry, 0xFF, sizeof(mts32_entry_t));

}

/* 
 * MTS mapping.
 *
 * It is NOT inlined since it triggers a GCC bug on my config (x86, GCC 3.3.5)
 */
static no_inline mts32_entry_t *mips_mts32_map(cpu_mips_t * cpu, u_int op_type, mts_map_t * map,
                                               mts32_entry_t * entry, mts32_entry_t * alt_entry, u_int is_fromgdb)
{
    struct vdevice *dev;
    m_uint32_t offset;

    if (!(dev = dev_lookup(cpu->vm, map->paddr)))
    {
        if (!is_fromgdb)
        {
            printf("no device!");
            printf("cpu->pc %x vaddr %x paddr %x \n", cpu->pc, map->vaddr, map->paddr);
            exit(-1);
        }

        return NULL;
    }

    if (!dev->host_addr || (dev->flags & VDEVICE_FLAG_NO_MTS_MMAP))
    {
        offset = map->paddr - dev->phys_addr;

        alt_entry->gvpa = map->vaddr;
        alt_entry->gppa = map->paddr;
        alt_entry->hpa = (dev->id << MTS_DEVID_SHIFT) + offset;
        alt_entry->flags = MTS_FLAG_DEV;
        alt_entry->mapped = map->mapped;
        return alt_entry;
    }
    ASSERT(dev->host_addr != 0, "dev->host_addr can not be null\n");
    entry->gvpa = map->vaddr;
    entry->gppa = map->paddr;
    entry->hpa = dev->host_addr + (map->paddr - dev->phys_addr);
    entry->flags = 0;
    entry->asid = map->asid;
    entry->g_bit = map->g_bit;
    entry->dirty_bit = map->dirty;
    entry->mapped = map->mapped;
    return entry;
}

/* MTS lookup */
static  void *mips_mts32_lookup(cpu_mips_t * cpu, m_va_t vaddr)
{
    m_reg_t data;
    u_int exc;
    m_uint8_t has_set_value = FALSE;
    return (mips_mts32_access(cpu, vaddr, MIPS_MEMOP_LOOKUP, 4, MTS_READ, &data, &exc, &has_set_value, 0));
}

/* === MIPS Memory Operations ============================================= */

u_int mips_mts32_gdb_lb(cpu_mips_t * cpu, m_va_t vaddr, void *cur)
{
    // m_reg_t data;
    void *haddr;
    u_int exc;
    m_uint8_t has_set_value = FALSE;

    haddr = mips_mts32_access(cpu, vaddr, MIPS_MEMOP_LB, 1, MTS_READ, (m_reg_t *) cur, &exc, &has_set_value, 1);

    if ((exc) || (haddr == NULL))
        *(m_uint8_t *) cur = 0x0;
    else
        *(m_uint8_t *) cur = (*(m_uint8_t *) haddr);

    return (0);
}

/* LB: Load Byte */
u_int  mips_mts32_lb(cpu_mips_t * cpu, m_va_t vaddr, u_int reg)
{
    m_reg_t data;
    void *haddr;
    u_int exc;
    m_uint8_t has_set_value = FALSE;


    haddr = mips_mts32_access(cpu, vaddr, MIPS_MEMOP_LB, 1, MTS_READ, &data, &exc, &has_set_value, 0);
    if (exc)
        return exc;
    if ((haddr == NULL) && (has_set_value == FALSE))
    {
        bad_memory_access(cpu, vaddr);
    }


    if (likely(has_set_value == FALSE))
        data = *(m_uint8_t *) haddr;
    if (likely(!exc))
        cpu->gpr[reg] = sign_extend(data, 8);
    return (exc);
}

/* LBU: Load Byte Unsigned */
u_int  mips_mts32_lbu(cpu_mips_t * cpu, m_va_t vaddr, u_int reg)
{
    m_reg_t data;
    void *haddr;
    u_int exc;
    m_uint8_t has_set_value = FALSE;

    haddr = mips_mts32_access(cpu, vaddr, MIPS_MEMOP_LBU, 1, MTS_READ, &data, &exc, &has_set_value, 0);
    if (exc)
        return exc;
    if ((haddr == NULL) && (has_set_value == FALSE))
    {
        bad_memory_access(cpu, vaddr);
    }
    if (likely(has_set_value == FALSE))
        data = *(m_uint8_t *) haddr;
    if (likely(!exc))
        cpu->gpr[reg] = data & 0xff;
    return (exc);
}

/* LH: Load Half-Word */
u_int  mips_mts32_lh(cpu_mips_t * cpu, m_va_t vaddr, u_int reg)
{
    m_reg_t data;
    void *haddr;
    u_int exc;
    m_uint8_t has_set_value = FALSE;

    haddr = mips_mts32_access(cpu, vaddr, MIPS_MEMOP_LH, 2, MTS_READ, &data, &exc, &has_set_value, 0);
    if (exc)
        return exc;
    if ((haddr == NULL) && (has_set_value == FALSE))
    {
        bad_memory_access(cpu, vaddr);
    }
    if (likely(has_set_value == FALSE))
        data = vmtoh16(*(m_uint16_t *) haddr);
    if (likely(!exc))
        cpu->gpr[reg] = sign_extend(data, 16);
    return (exc);
}

/* LHU: Load Half-Word Unsigned */
u_int  mips_mts32_lhu(cpu_mips_t * cpu, m_va_t vaddr, u_int reg)
{
    m_reg_t data;
    void *haddr;
    u_int exc;
    m_uint8_t has_set_value = FALSE;


    haddr = mips_mts32_access(cpu, vaddr, MIPS_MEMOP_LHU, 2, MTS_READ, &data, &exc, &has_set_value, 0);
    if (exc)
        return exc;
    if ((haddr == NULL) && (has_set_value == FALSE))
    {
        bad_memory_access(cpu, vaddr);
    }
    if (likely(has_set_value == FALSE))
        data = vmtoh16(*(m_uint16_t *) haddr);
    if (likely(!exc))
        cpu->gpr[reg] = data & 0xffff;
    return (exc);
}

/* LW: Load Word */
u_int  mips_mts32_lw(cpu_mips_t * cpu, m_va_t vaddr, u_int reg)
{
    m_reg_t data;
    void *haddr;
    u_int exc;
    m_uint8_t has_set_value = FALSE;
    haddr = mips_mts32_access(cpu, vaddr, MIPS_MEMOP_LW, 4, MTS_READ, &data, &exc, &has_set_value, 0);
    if (exc)
        return exc;
    if ((haddr == NULL) && (has_set_value == FALSE))
    {
        bad_memory_access(cpu, vaddr);
    }

    if (likely(has_set_value == FALSE))
        data = vmtoh32(*(m_uint32_t *) haddr);
    if (likely(!exc))
        cpu->gpr[reg] = sign_extend(data, 32);


    return (exc);
}

/* LWU: Load Word Unsigned */
u_int  mips_mts32_lwu(cpu_mips_t * cpu, m_va_t vaddr, u_int reg)
{
    m_reg_t data;
    void *haddr;
    u_int exc;
    m_uint8_t has_set_value = FALSE;


    haddr = mips_mts32_access(cpu, vaddr, MIPS_MEMOP_LWU, 4, MTS_READ, &data, &exc, &has_set_value, 0);
    if (exc)
        return exc;
    if ((haddr == NULL) && (has_set_value == FALSE))
    {
        bad_memory_access(cpu, vaddr);
    }
    if (likely(has_set_value == FALSE))
        data = vmtoh32(*(m_uint32_t *) haddr);
    if (likely(!exc))
        cpu->gpr[reg] = data & 0xffffffff;
    return (exc);
}

/* LD: Load Double-Word */
u_int  mips_mts32_ld(cpu_mips_t * cpu, m_va_t vaddr, u_int reg)
{
    m_reg_t data;
    void *haddr;
    u_int exc;
    m_uint8_t has_set_value = FALSE;


    haddr = mips_mts32_access(cpu, vaddr, MIPS_MEMOP_LD, 8, MTS_READ, &data, &exc, &has_set_value, 0);
    if (exc)
        return exc;
    if ((haddr == NULL) && (has_set_value == FALSE))
    {
        bad_memory_access(cpu, vaddr);
    }
    if (likely(has_set_value == FALSE))
        data = vmtoh64(*(m_uint64_t *) haddr);
    if (likely(!exc))
        cpu->gpr[reg] = data;
    return (exc);
}

/* SB: Store Byte */
u_int  mips_mts32_sb(cpu_mips_t * cpu, m_va_t vaddr, u_int reg)
{
    m_reg_t data;
    void *haddr = NULL;
    u_int exc;
    m_uint8_t has_set_value = FALSE;

    data = cpu->gpr[reg] & 0xff;
    haddr = mips_mts32_access(cpu, vaddr, MIPS_MEMOP_SB, 1, MTS_WRITE, &data, &exc, &has_set_value, 0);
    if (exc)
        return exc;
    if ((haddr == NULL) && (has_set_value == FALSE))
    {
        bad_memory_access(cpu, vaddr);
    }
    if (likely(has_set_value == FALSE))
    {

        *(m_uint8_t *) haddr = data;
    }


    return (exc);
}

/* SH: Store Half-Word */
u_int  mips_mts32_sh(cpu_mips_t * cpu, m_va_t vaddr, u_int reg)
{
    m_reg_t data;
    void *haddr;
    u_int exc;
    m_uint8_t has_set_value = FALSE;


    data = cpu->gpr[reg] & 0xffff;
    haddr = mips_mts32_access(cpu, vaddr, MIPS_MEMOP_SH, 2, MTS_WRITE, &data, &exc, &has_set_value, 0);
    if (exc)
        return exc;
    if ((haddr == NULL) && (has_set_value == FALSE))
    {
        bad_memory_access(cpu, vaddr);
    }
    if (likely(has_set_value == FALSE))
    {

        *(m_uint16_t *) haddr = htovm16(data);
    }

    return (exc);
}

/* SW: Store Word */
u_int  mips_mts32_sw(cpu_mips_t * cpu, m_va_t vaddr, u_int reg)
{
    m_reg_t data;
    void *haddr;
    u_int exc;
    m_uint8_t has_set_value = FALSE;


    data = cpu->gpr[reg] & 0xffffffff;


    haddr = mips_mts32_access(cpu, vaddr, MIPS_MEMOP_SW, 4, MTS_WRITE, &data, &exc, &has_set_value, 0);
    if (exc)
        return exc;
    if ((haddr == NULL) && (has_set_value == FALSE))
    {
        bad_memory_access(cpu, vaddr);
    }
    if (likely(has_set_value == FALSE))
    {

        *(m_uint32_t *) haddr = htovm32(data);
    }


    return (exc);
}

/* SD: Store Double-Word */
u_int  mips_mts32_sd(cpu_mips_t * cpu, m_va_t vaddr, u_int reg)
{
    m_reg_t data;
    void *haddr;
    u_int exc;
    m_uint8_t has_set_value = FALSE;


    data = cpu->gpr[reg];
    haddr = mips_mts32_access(cpu, vaddr, MIPS_MEMOP_SD, 8, MTS_WRITE, &data, &exc, &has_set_value, 0);
    if (exc)
        return exc;
    if ((haddr == NULL) && (has_set_value == FALSE))
    {
        bad_memory_access(cpu, vaddr);
    }
    if (likely(has_set_value == FALSE))
        *(m_uint64_t *) haddr = htovm64(data);
    return (exc);
}

/* LDC1: Load Double-Word To Coprocessor 1 */
u_int  mips_mts32_ldc1(cpu_mips_t * cpu, m_va_t vaddr, u_int reg)
{
    printf("mips_mts32_ldc1 pc %x\n", cpu->pc);
    exit(-1);
    return 0;
}


u_int  mips_mts32_lwl(cpu_mips_t * cpu, m_va_t vaddr, u_int reg)
{
    void *haddr = NULL;
    u_int exc;
    m_uint32_t data, naddr, shift = 0, mask1 = 0, mask2 = 0;
    m_uint8_t has_set_value = FALSE;


    naddr = vaddr & ~(0x03);
    haddr = mips_mts32_access(cpu, naddr, MIPS_MEMOP_LWL, 4, MTS_READ, &data, &exc, &has_set_value, 0);

    if (exc)
        return exc;
    if ((haddr == NULL) && (has_set_value == FALSE))
    {
        bad_memory_access(cpu, vaddr);
    }
    if (has_set_value == FALSE)
    {
        data = vmtoh32(*(m_reg_t *) haddr);

        switch (vaddr & 0x3)
        {
#if	GUEST_BYTE_ORDER==ARCH_LITTLE_ENDIAN
        case 0x0:
#elif GUEST_BYTE_ORDER==ARCH_BIG_ENDIAN
        case 0x3:
#endif
            mask1 = 0xff;
            mask2 = 0xff000000;
            shift = 24;
            break;
#if	GUEST_BYTE_ORDER==ARCH_LITTLE_ENDIAN
        case 0x1:
#elif GUEST_BYTE_ORDER==ARCH_BIG_ENDIAN
        case 0x2:
#endif
            mask1 = 0xffff;
            mask2 = 0xffff0000;
            shift = 16;
            break;
#if	GUEST_BYTE_ORDER==ARCH_LITTLE_ENDIAN
        case 0x2:
#elif GUEST_BYTE_ORDER==ARCH_BIG_ENDIAN
        case 0x1:
#endif
            mask1 = 0xffffff;
            mask2 = 0xffffff00;
            shift = 8;
            break;
#if	GUEST_BYTE_ORDER==ARCH_LITTLE_ENDIAN
        case 0x3:
#elif GUEST_BYTE_ORDER==ARCH_BIG_ENDIAN
        case 0x0:
#endif
            mask1 = 0xffffffff;
            mask2 = 0xffffffff;
            shift = 0;
            break;
        }

        data = (data & mask1) << shift;
        data &= mask2;
        cpu->gpr[reg] &= ~mask2;
        cpu->gpr[reg] |= data;
        cpu->gpr[reg] = sign_extend(cpu->gpr[reg], 32);

    }
    return 0;
}

u_int  mips_mts32_lwr(cpu_mips_t * cpu, m_va_t vaddr, u_int reg)
{
    void *haddr = NULL;
    u_int exc;
    m_uint32_t data, naddr, shift = 0, mask1 = 0, mask2 = 0;
    m_uint8_t has_set_value = FALSE;


    naddr = vaddr & ~(0x03);
    haddr = mips_mts32_access(cpu, naddr, MIPS_MEMOP_LWR, 4, MTS_READ, &data, &exc, &has_set_value, 0);

    if (exc)
        return exc;
    if ((haddr == NULL) && (has_set_value == FALSE))
    {
        bad_memory_access(cpu, vaddr);
    }

    if (has_set_value == FALSE)
    {

        data = vmtoh32(*(m_reg_t *) haddr);


        switch (vaddr & 0x3)
        {
#if	GUEST_BYTE_ORDER==ARCH_LITTLE_ENDIAN
        case 0x3:
#elif GUEST_BYTE_ORDER==ARCH_BIG_ENDIAN
        case 0x0:
#endif
            mask1 = 0xff;
            mask2 = 0xff000000;
            shift = 24;
            break;
#if	GUEST_BYTE_ORDER==ARCH_LITTLE_ENDIAN
        case 0x2:
#elif GUEST_BYTE_ORDER==ARCH_BIG_ENDIAN
        case 0x1:
#endif
            mask1 = 0xffff;
            mask2 = 0xffff0000;
            shift = 16;
            break;
#if	GUEST_BYTE_ORDER==ARCH_LITTLE_ENDIAN
        case 0x1:
#elif GUEST_BYTE_ORDER==ARCH_BIG_ENDIAN
        case 0x2:
#endif

            mask1 = 0xffffff;
            mask2 = 0xffffff00;
            shift = 8;
            break;
#if	GUEST_BYTE_ORDER==ARCH_LITTLE_ENDIAN
        case 0x0:
#elif GUEST_BYTE_ORDER==ARCH_BIG_ENDIAN
        case 0x3:
#endif
            mask1 = 0xffffffff;
            mask2 = 0xffffffff;
            shift = 0;
            break;
        }

        data = (data & mask2) >> shift;
        data &= mask1;
        cpu->gpr[reg] &= ~mask1;
        cpu->gpr[reg] |= data;
        cpu->gpr[reg] = sign_extend(cpu->gpr[reg], 32);

    }
    return 0;
}



/* LDL: Load Double-Word Left */
u_int  mips_mts32_ldl(cpu_mips_t * cpu, m_va_t vaddr, u_int reg)
{
    m_reg_t r_mask, naddr;
    m_reg_t data;
    u_int m_shift;
    void *haddr;
    u_int exc;
    m_uint8_t has_set_value = FALSE;


    naddr = vaddr & ~(0x07);
    haddr = mips_mts32_access(cpu, naddr, MIPS_MEMOP_LDL, 8, MTS_READ, &data, &exc, &has_set_value, 0);
    if (exc)
        return exc;
    if ((haddr == NULL) && (has_set_value == FALSE))
    {
        bad_memory_access(cpu, vaddr);
    }

    if (likely(haddr != NULL))
        data = (m_reg_t) (vmtoh64(*(m_uint64_t *) haddr));

    if (likely(!exc))
    {
        m_shift = (vaddr & 0x07) << 3;
        r_mask = (1ULL << m_shift) - 1;
        data <<= m_shift;

        cpu->gpr[reg] &= r_mask;
        cpu->gpr[reg] |= data;
    }
    return (exc);
}

/* LDR: Load Double-Word Right */
u_int  mips_mts32_ldr(cpu_mips_t * cpu, m_va_t vaddr, u_int reg)
{
    m_reg_t r_mask, naddr;
    m_reg_t data;
    u_int m_shift;
    void *haddr;
    u_int exc;
    m_uint8_t has_set_value = FALSE;


    naddr = vaddr & ~(0x07);
    haddr = mips_mts32_access(cpu, naddr, MIPS_MEMOP_LDR, 8, MTS_READ, &data, &exc, &has_set_value, 0);
    if (exc)
        return exc;
    if ((haddr == NULL) && (has_set_value == FALSE))
    {
        bad_memory_access(cpu, vaddr);
    }

    if (likely(haddr != NULL))
        data = (m_reg_t) (vmtoh64(*(m_uint64_t *) haddr));

    if (likely(!exc))
    {
        m_shift = ((vaddr & 0x07) + 1) << 3;
        r_mask = (1ULL << m_shift) - 1;
        data >>= (64 - m_shift);

        cpu->gpr[reg] &= ~r_mask;
        cpu->gpr[reg] |= data;
    }
    return (exc);
}


u_int  mips_mts32_swl(cpu_mips_t * cpu, m_va_t vaddr, u_int reg)
{
    void *haddr = NULL;
    u_int exc;
    m_uint32_t data, naddr, temp, mask1 = 0, mask2 = 0, shift = 0;
    m_uint8_t has_set_value = FALSE;

    naddr = vaddr & ~(0x03ULL);
    data = cpu->gpr[reg] & 0xffffffff;
    haddr = mips_mts32_access(cpu, naddr, MIPS_MEMOP_SWL, 4, MTS_WRITE, &data, &exc, &has_set_value, 0);
    if (exc)
        return exc;
    if ((haddr == NULL) && (has_set_value == FALSE))
    {
        bad_memory_access(cpu, vaddr);
    }

    if (has_set_value == FALSE)
    {
        switch (vaddr & 0x3)
        {
#if	GUEST_BYTE_ORDER==ARCH_LITTLE_ENDIAN
        case 0x0:
#elif GUEST_BYTE_ORDER==ARCH_BIG_ENDIAN
        case 0x3:
#endif
            mask1 = 0xff;
            mask2 = 0xff000000;
            shift = 24;
            break;
#if	GUEST_BYTE_ORDER==ARCH_LITTLE_ENDIAN
        case 0x1:
#elif GUEST_BYTE_ORDER==ARCH_BIG_ENDIAN
        case 0x2:
#endif
            mask1 = 0xffff;
            mask2 = 0xffff0000;
            shift = 16;
            break;
#if	GUEST_BYTE_ORDER==ARCH_LITTLE_ENDIAN
        case 0x2:
#elif GUEST_BYTE_ORDER==ARCH_BIG_ENDIAN
        case 0x1:
#endif
            mask1 = 0xffffff;
            mask2 = 0xffffff00;
            shift = 8;
            break;
#if	GUEST_BYTE_ORDER==ARCH_LITTLE_ENDIAN
        case 0x3:
#elif GUEST_BYTE_ORDER==ARCH_BIG_ENDIAN
        case 0x0:
#endif
            mask1 = 0xffffffff;
            mask2 = 0xffffffff;
            shift = 0;
            break;
        }


        data = (data & mask2) >> shift;
        data &= mask1;
        temp = vmtoh32(*(m_uint32_t *) haddr);

        temp &= ~mask1;
        temp = temp | data;
        *(m_uint32_t *) haddr = htovm32(temp);

    }

    return 0;
}


u_int  mips_mts32_swr(cpu_mips_t * cpu, m_va_t vaddr, u_int reg)
{
    void *haddr = NULL;
    u_int exc;
    m_uint32_t data, naddr, temp, mask1 = 0, mask2 = 0, shift = 0;
    m_uint8_t has_set_value = FALSE;



    naddr = vaddr & ~(0x03ULL);
    data = cpu->gpr[reg] & 0xffffffff;
    haddr = mips_mts32_access(cpu, naddr, MIPS_MEMOP_SWR, 4, MTS_WRITE, &data, &exc, &has_set_value, 0);
    if (exc)
        return exc;
    if ((haddr == NULL) && (has_set_value == FALSE))
    {
        bad_memory_access(cpu, vaddr);
    }
    if (has_set_value == FALSE)
    {
        switch (vaddr & 0x3)
        {
#if	GUEST_BYTE_ORDER==ARCH_LITTLE_ENDIAN
        case 0x3:
#elif GUEST_BYTE_ORDER==ARCH_BIG_ENDIAN
        case 0x0:
#endif
            mask1 = 0xff;
            mask2 = 0xff000000;
            shift = 24;
            break;
#if	GUEST_BYTE_ORDER==ARCH_LITTLE_ENDIAN
        case 0x2:
#elif GUEST_BYTE_ORDER==ARCH_BIG_ENDIAN
        case 0x1:
#endif
            mask1 = 0xffff;
            mask2 = 0xffff0000;
            shift = 16;
            break;
#if	GUEST_BYTE_ORDER==ARCH_LITTLE_ENDIAN
        case 0x1:
#elif GUEST_BYTE_ORDER==ARCH_BIG_ENDIAN
        case 0x2:
#endif
            mask1 = 0xffffff;
            mask2 = 0xffffff00;
            shift = 8;
            break;
#if	GUEST_BYTE_ORDER==ARCH_LITTLE_ENDIAN
        case 0x0:
#elif GUEST_BYTE_ORDER==ARCH_BIG_ENDIAN
        case 0x3:
#endif
            mask1 = 0xffffffff;
            mask2 = 0xffffffff;
            shift = 0;
            break;
        }

        data = (data & mask1) << shift;
        data &= mask2;
        temp = vmtoh32(*(m_uint32_t *) haddr);

        temp &= ~mask2;
        temp = temp | data;
        *(m_uint32_t *) haddr = htovm32(temp);
    }

    return 0;
}




/* SDL: Store Double-Word Left */
u_int  mips_mts32_sdl(cpu_mips_t * cpu, m_va_t vaddr, u_int reg)
{
    m_reg_t d_mask, naddr;
    m_reg_t data;
    u_int r_shift;
    void *haddr;
    u_int exc;
    m_uint8_t has_set_value = FALSE;


    naddr = vaddr & ~(0x07);
    haddr = mips_mts32_access(cpu, naddr, MIPS_MEMOP_SDL, 8, MTS_READ, &data, &exc, &has_set_value, 0);
    if (exc)
        return exc;
    if ((haddr == NULL) && (has_set_value == FALSE))
    {
        bad_memory_access(cpu, vaddr);
    }
    if (unlikely(exc))
        return (exc);

    if (likely(haddr != NULL))
        data = (m_reg_t) (vmtoh64(*(m_uint64_t *) haddr));

    r_shift = (vaddr & 0x07) << 3;
    d_mask = 0xffffffffffffffffULL >> r_shift;

    data &= ~d_mask;
    data |= cpu->gpr[reg] >> r_shift;

    haddr = mips_mts32_access(cpu, naddr, MIPS_MEMOP_SDL, 8, MTS_WRITE, &data, &exc, &has_set_value, 0);
    if ((haddr == NULL) && (has_set_value == FALSE))
    {
        bad_memory_access(cpu, vaddr);
    }
    if (likely(haddr != NULL))
        *(m_reg_t *) (m_uint64_t *) haddr = (m_reg_t) (htovm64(data));
    return (exc);
}

/* SDR: Store Double-Word Right */
u_int  mips_mts32_sdr(cpu_mips_t * cpu, m_va_t vaddr, u_int reg)
{
    m_reg_t d_mask, naddr;
    m_reg_t data;
    u_int r_shift;
    void *haddr;
    u_int exc;

    m_uint8_t has_set_value = FALSE;

    naddr = vaddr & ~(0x07);
    haddr = mips_mts32_access(cpu, naddr, MIPS_MEMOP_SDR, 8, MTS_READ, &data, &exc, &has_set_value, 0);
    if (exc)
        return exc;
    if ((haddr == NULL) && (has_set_value == FALSE))
    {
        bad_memory_access(cpu, vaddr);
    }
    if (unlikely(exc))
        return (exc);

    if (likely(haddr != NULL))
        data = vmtoh64(*(m_uint64_t *) haddr);

    r_shift = ((vaddr & 0x07) + 1) << 3;
    d_mask = 0xffffffffffffffffULL >> r_shift;

    data &= d_mask;
    data |= cpu->gpr[reg] << (64 - r_shift);

    haddr = mips_mts32_access(cpu, naddr, MIPS_MEMOP_SDR, 8, MTS_WRITE, &data, &exc, &has_set_value, 0);
    if ((haddr == NULL) && (has_set_value == FALSE))
    {
        bad_memory_access(cpu, vaddr);
    }
    if (likely(haddr != NULL))
        *(m_reg_t *) (m_uint64_t *) haddr = (m_reg_t) (htovm64(data));
    return (exc);
}

/* LL: Load Linked */
u_int  mips_mts32_ll(cpu_mips_t * cpu, m_va_t vaddr, u_int reg)
{
    m_reg_t data;
    void *haddr;
    u_int exc;
    m_uint8_t has_set_value = FALSE;


    haddr = mips_mts32_access(cpu, vaddr, MIPS_MEMOP_LL, 4, MTS_READ, &data, &exc, &has_set_value, 0);
    if (exc)
        return exc;
    if ((haddr == NULL) && (has_set_value == FALSE))
    {
        bad_memory_access(cpu, vaddr);
    }
    if (likely(haddr != NULL))
        data = vmtoh32(*(m_uint32_t *) haddr);

    if (likely(!exc))
    {
        cpu->gpr[reg] = sign_extend(data, 32);
        cpu->ll_bit = 1;
    }

    return (exc);
}

/* SC: Store Conditional */
u_int  mips_mts32_sc(cpu_mips_t * cpu, m_va_t vaddr, u_int reg)
{
    m_reg_t data;
    void *haddr;
    u_int exc = 0;
    m_uint8_t has_set_value = FALSE;


    if (cpu->ll_bit)
    {
        data = cpu->gpr[reg] & 0xffffffff;
        haddr = mips_mts32_access(cpu, vaddr, MIPS_MEMOP_SC, 4, MTS_WRITE, &data, &exc, &has_set_value, 0);
        if (exc)
            return exc;
        if ((haddr == NULL) && (has_set_value == FALSE))
        {
            bad_memory_access(cpu, vaddr);
        }
        if (likely(haddr != NULL))
            *(m_uint32_t *) haddr = htovm32(data);
    }

    if (likely(!exc))
        cpu->gpr[reg] = cpu->ll_bit;
    return (exc);
}

/* SDC1: Store Double-Word from Coprocessor 1 */
u_int  mips_mts32_sdc1(cpu_mips_t * cpu, m_va_t vaddr, u_int reg)
{
    /* m_uint64_t data;
       void *haddr;
       u_int exc;
       m_uint8_t has_set_value=FALSE;


       data = cpu->fpu.reg[reg];
       haddr = mips_mts32_access(cpu,vaddr,MIPS_MEMOP_SDC1,8,MTS_WRITE,
       &data,&exc,&has_set_value);
       if ((haddr==NULL)&&(has_set_value==FALSE))  
       { 
       bad_memory_access(cpu,vaddr);
       }
       if (likely(haddr != NULL)) *(m_uint64_t *)haddr = htovm64(data);
       return(exc); */
    printf("mips_mts32_sdc1 pc %x\n", cpu->pc);
    exit(-1);
    return 0;
}

/* CACHE: Cache operation */
u_int  mips_mts32_cache(cpu_mips_t * cpu, m_va_t vaddr, u_int op)
{
#if 0
    mips64_jit_tcb_t *block;
    m_uint32_t pc_hash;

#if DEBUG_CACHE
    cpu_log(cpu->gen, "MTS", "CACHE: PC=0x%llx, vaddr=0x%llx, cache=%u, code=%u\n", cpu->pc, vaddr, op & 0x3, op >> 2);
#endif

    if (cpu->exec_blk_map)
    {
        pc_hash = mips64_jit_get_pc_hash(vaddr);
        block = cpu->exec_blk_map[pc_hash];

        if (block && (block->start_pc == (vaddr & MIPS_MIN_PAGE_MASK)))
        {
#if DEBUG_CACHE
            cpu_log(cpu->gen, "MTS", "CACHE: removing compiled page at 0x%llx, pc=0x%llx\n", block->start_pc, cpu->pc);
#endif
            cpu->exec_blk_map[pc_hash] = NULL;
            mips64_jit_tcb_free(cpu, block, TRUE);
        }
        else
        {
#if DEBUG_CACHE
            cpu_log(cpu->gen, "MTS", "CACHE: trying to remove page 0x%llx with pc=0x%llx\n", vaddr, cpu->pc);
#endif

        }
    }
#endif

    return (0);
}

/* === MTS Cache Management ============================================= */

/* MTS map/unmap/rebuild "API" functions */
void mips_mts32_api_map(cpu_mips_t * cpu, m_va_t vaddr, m_pa_t paddr, m_uint32_t len, int cache_access, int tlb_index)
{
    /* nothing to do, the cache will be filled on-the-fly */
}

void mips_mts32_api_unmap(cpu_mips_t * cpu, m_va_t vaddr, m_uint32_t len, m_uint32_t val, int tlb_index)
{
    /* Invalidate the TLB entry or the full cache if no index is specified */
    if (tlb_index != -1)
        mips_mts32_invalidate_tlb_entry(cpu, vaddr);
    else
        mips_mts32_invalidate_cache(cpu);
}

void mips_mts32_api_rebuild(cpu_mips_t * cpu)
{
    mips_mts32_invalidate_cache((cpu));
}

/* ======================================================================== */

/* Initialize memory access vectors */
void mips_mts32_init_memop_vectors(cpu_mips_t * cpu)
{
    /* XXX TODO:
     *  - LD/SD forbidden in Supervisor/User modes with 32-bit addresses.
     */

    cpu->addr_mode = 32;

    /* API vectors */
    cpu->mts_map = mips_mts32_api_map;
    cpu->mts_unmap = mips_mts32_api_unmap;

    /* Memory lookup operation */
    cpu->mem_op_lookup = mips_mts32_lookup;

    /* Translation operation */
    cpu->translate = mips_mts32_translate;

    /* Shutdown operation */
    cpu->mts_shutdown = mips_mts32_shutdown;

    /* Rebuild MTS data structures */
    cpu->mts_rebuild = mips_mts32_api_rebuild;

    /* Show statistics */
    //cpu->mts_show_stats = mips_mts32_show_stats;

    cpu->mips_mts_gdb_lb = mips_mts32_gdb_lb;

    /* Load Operations */
    cpu->mem_op_fn[MIPS_MEMOP_LB] = mips_mts32_lb;
    cpu->mem_op_fn[MIPS_MEMOP_LBU] = mips_mts32_lbu;
    cpu->mem_op_fn[MIPS_MEMOP_LH] = mips_mts32_lh;
    cpu->mem_op_fn[MIPS_MEMOP_LHU] = mips_mts32_lhu;
    cpu->mem_op_fn[MIPS_MEMOP_LW] = mips_mts32_lw;
    cpu->mem_op_fn[MIPS_MEMOP_LWU] = mips_mts32_lwu;
    cpu->mem_op_fn[MIPS_MEMOP_LD] = mips_mts32_ld;
    cpu->mem_op_fn[MIPS_MEMOP_LDL] = mips_mts32_ldl;
    cpu->mem_op_fn[MIPS_MEMOP_LDR] = mips_mts32_ldr;

    /* Store Operations */
    cpu->mem_op_fn[MIPS_MEMOP_SB] = mips_mts32_sb;
    cpu->mem_op_fn[MIPS_MEMOP_SH] = mips_mts32_sh;
    cpu->mem_op_fn[MIPS_MEMOP_SW] = mips_mts32_sw;
    cpu->mem_op_fn[MIPS_MEMOP_SD] = mips_mts32_sd;

    /* Load Left/Right operations */
    cpu->mem_op_fn[MIPS_MEMOP_LWL] = mips_mts32_lwl;
    cpu->mem_op_fn[MIPS_MEMOP_LWR] = mips_mts32_lwr;
    cpu->mem_op_fn[MIPS_MEMOP_LDL] = mips_mts32_ldl;
    cpu->mem_op_fn[MIPS_MEMOP_LDR] = mips_mts32_ldr;

    /* Store Left/Right operations */
    cpu->mem_op_fn[MIPS_MEMOP_SWL] = mips_mts32_swl;
    cpu->mem_op_fn[MIPS_MEMOP_SWR] = mips_mts32_swr;
    cpu->mem_op_fn[MIPS_MEMOP_SDL] = mips_mts32_sdl;
    cpu->mem_op_fn[MIPS_MEMOP_SDR] = mips_mts32_sdr;

    /* LL/SC - Load Linked / Store Conditional */
    cpu->mem_op_fn[MIPS_MEMOP_LL] = mips_mts32_ll;
    cpu->mem_op_fn[MIPS_MEMOP_SC] = mips_mts32_sc;

    /* Coprocessor 1 memory access functions */
    cpu->mem_op_fn[MIPS_MEMOP_LDC1] = mips_mts32_ldc1;
    cpu->mem_op_fn[MIPS_MEMOP_SDC1] = mips_mts32_sdc1;

    /* Cache Operation */
    cpu->mem_op_fn[MIPS_MEMOP_CACHE] = mips_mts32_cache;
}


#if 0
/* === Specific operations for MTS64 ====================================== */

/* MTS64 slow lookup */
static mts64_entry_t *mips_mts64_slow_lookup(cpu_mips_t * cpu, m_uint64_t vaddr,
                                             u_int op_code, u_int op_size,
                                             u_int op_type, m_reg_t * data,
                                             u_int * exc, mts64_entry_t * alt_entry, u_int is_fromgdb)
{
    m_uint32_t hash_bucket, zone, sub_zone, cca;
    mts64_entry_t *entry;
    mts_map_t map;

    map.tlb_index = -1;
    hash_bucket = MTS64_HASH(vaddr);
    entry = &cpu->mts_u.mts64_cache[hash_bucket];
    zone = vaddr >> 40;

#if DEBUG_MTS_STATS
    cpu->mts_misses++;
#endif

    switch (zone)
    {
    case 0x000000:             /* xkuseg */
    case 0x400000:             /* xksseg */
    case 0xc00000:             /* xkseg */
        /* trigger TLB exception if no matching entry found */
        if (!mips64_cp0_tlb_lookup(cpu, vaddr, &map))
            goto err_tlb;

        if (!(entry = mips_mts64_map(cpu, op_type, &map, entry, alt_entry)))
            goto err_undef;

        return (entry);

    case 0xffffff:
        sub_zone = (vaddr >> 29) & 0x7FF;

        switch (sub_zone)
        {
        case 0x7fc:            /* ckseg0 */
            map.vaddr = vaddr & MIPS_MIN_PAGE_MASK;
            map.paddr = map.vaddr - 0xFFFFFFFF80000000ULL;
            //map.cached = TRUE;

            if (!(entry = mips_mts64_map(cpu, op_type, &map, entry, alt_entry)))
                goto err_undef;

            return (entry);

        case 0x7fd:            /* ckseg1 */
            map.vaddr = vaddr & MIPS_MIN_PAGE_MASK;
            map.paddr = map.vaddr - 0xFFFFFFFFA0000000ULL;
            //map.cached = FALSE;

            if (!(entry = mips_mts64_map(cpu, op_type, &map, entry, alt_entry)))
                goto err_undef;

            return (entry);

        case 0x7fe:            /* cksseg */
        case 0x7ff:            /* ckseg3 */
            /* trigger TLB exception if no matching entry found */
            if (!mips64_cp0_tlb_lookup(cpu, vaddr, &map))
                goto err_tlb;

            if (!(entry = mips_mts64_map(cpu, op_type, &map, entry, alt_entry)))
                goto err_undef;

            return (entry);

        default:
            /* Invalid zone: generate Address Error (AE) exception */
            goto err_address;
        }
        break;

        /* xkphys */
    case 0x800000:
    case 0x880000:
    case 0x900000:
    case 0x980000:
    case 0xa00000:
    case 0xa80000:
    case 0xb00000:
    case 0xb80000:
        cca = (vaddr >> MIPS64_XKPHYS_CCA_SHIFT) & 0x03;
        //map.cached = mips64_cca_cached(cca);
        map.vaddr = vaddr & MIPS_MIN_PAGE_MASK;
        map.paddr = (vaddr & MIPS64_XKPHYS_PHYS_MASK);
        map.paddr &= MIPS_MIN_PAGE_MASK;

        if (!(entry = mips_mts64_map(cpu, op_type, &map, entry, alt_entry)))
            goto err_undef;

        return (entry);

    default:
        /* Invalid zone: generate Address Error (AE) exception */
        goto err_address;
    }

  err_undef:
    if (is_fromgdb)
        return NULL;
    mips_access_special(cpu, vaddr, MTS_ACC_U, op_code, op_type, op_size, data, exc);
    return NULL;
  err_address:
    if (is_fromgdb)
        return NULL;
    mips_access_special(cpu, vaddr, MTS_ACC_AE, op_code, op_type, op_size, data, exc);
    return NULL;
  err_tlb:
    if (is_fromgdb)
        return NULL;
    mips_access_special(cpu, vaddr, MTS_ACC_T, op_code, op_type, op_size, data, exc);
    return NULL;
}


/* MTS64 access */
static void *mips_mts64_access(cpu_mips_t * cpu, m_va_t vaddr,
                               u_int op_code, u_int op_size,
                               u_int op_type, m_reg_t * data, u_int * exc, m_uint8_t * has_set_value, u_int is_fromgdb)
{
    mts64_entry_t *entry, alt_entry;
    m_uint32_t hash_bucket;
    m_iptr_t haddr;
    u_int dev_id;
    int cow;

#if MEMLOG_ENABLE
    /* Record the memory access */
    memlog_rec_access(cpu->gen, vaddr, *data, op_size, op_type);
#endif

    *exc = 0;
    hash_bucket = MTS64_HASH(vaddr);
    entry = &cpu->mts_u.mts64_cache[hash_bucket];

#if DEBUG_MTS_STATS
    cpu->mts_lookups++;
#endif

    /* Copy-On-Write for sparse device ? */
    cow = (op_type == MTS_WRITE) && (entry->flags & MTS_FLAG_COW);

    /* Slow lookup if nothing found in cache */
    if (unlikely(((vaddr & MIPS_MIN_PAGE_MASK) != entry->gvpa) || cow))
    {
        entry = mips_mts64_slow_lookup(cpu, vaddr, op_code, op_size, op_type, data, exc, &alt_entry, 0);
        if (!entry)
            return NULL;

        if (entry->flags & MTS_FLAG_DEV)
        {
            dev_id = (entry->hpa & MTS_DEVID_MASK) >> MTS_DEVID_SHIFT;
            haddr = entry->hpa & MTS_DEVOFF_MASK;
            haddr += vaddr - entry->gvpa;
            return (dev_access_fast(cpu, dev_id, haddr, op_size, op_type, data, has_set_value));
        }
    }

    /* Raw memory access */
    haddr = entry->hpa + (vaddr & MIPS_MIN_PAGE_IMASK);
#if MEMLOG_ENABLE
    memlog_update_read(cpu->gen, haddr);
#endif
    return ((void *) haddr);
}

/* MTS64 virtual address to physical page translation */
static int mips_mts64_translate(cpu_mips_t * cpu, m_va_t vaddr, m_uint32_t * phys_page)
{
    mts64_entry_t *entry, alt_entry;
    m_uint32_t hash_bucket;
    m_reg_t data = 0;
    u_int exc = 0;

    hash_bucket = MTS64_HASH(vaddr);
    entry = &cpu->mts_u.mts64_cache[hash_bucket];

    /* Slow lookup if nothing found in cache */
    if (unlikely((vaddr & MIPS_MIN_PAGE_MASK) != entry->gvpa))
    {
        entry = mips_mts64_slow_lookup(cpu, vaddr, MIPS_MEMOP_LOOKUP, 4, MTS_READ, &data, &exc, &alt_entry, 0);
        if (!entry)
            return (-1);
    }

    *phys_page = entry->gppa >> MIPS_MIN_PAGE_SHIFT;
    return (0);
}

#endif
/* === Specific operations for MTS32 ====================================== */



/* MTS32 slow lookup */
static mts32_entry_t *mips_mts32_slow_lookup(cpu_mips_t * cpu, m_uint64_t vaddr,
                                             u_int op_code, u_int op_size,
                                             u_int op_type, m_reg_t * data,
                                             u_int * exc, mts32_entry_t * alt_entry, u_int is_fromgdb)
{
    m_uint32_t hash_bucket, zone;
    mts32_entry_t *entry;
    mts_map_t map;

    map.tlb_index = -1;
    hash_bucket = MTS32_HASH(vaddr);
    entry = &cpu->mts_u.mts32_cache[hash_bucket];
    zone = (vaddr >> 29) & 0x7;


#if DEBUG_MTS_STATS
    cpu->mts_misses++;
#endif

    switch (zone)
    {
    case 0x00:
    case 0x01:
    case 0x02:
    case 0x03:                 /* kuseg */
        /* trigger TLB exception if no matching entry found */
        if (!mips64_cp0_tlb_lookup(cpu, vaddr, &map))
            goto err_tlb;

        if ((map.valid & 0x1) != 0x1)
            goto err_tlb;
        if ((MTS_WRITE == op_type) && ((map.dirty & 0x1) != 0x1))
            goto err_mod;


        map.mapped = TRUE;
        if (!(entry = mips_mts32_map(cpu, op_type, &map, entry, alt_entry, is_fromgdb)))
            goto err_undef;

        return (entry);

    case 0x04:                 /* kseg0 */
        map.vaddr = vaddr & MIPS_MIN_PAGE_MASK;
        map.paddr = map.vaddr - (m_pa_t) 0xFFFFFFFF80000000ULL;
        map.mapped = FALSE;

        if (!(entry = mips_mts32_map(cpu, op_type, &map, entry, alt_entry, is_fromgdb)))
            goto err_undef;
        return (entry);

    case 0x05:                 /* kseg1 */
        map.vaddr = vaddr & MIPS_MIN_PAGE_MASK;
        map.paddr = map.vaddr - (m_pa_t) 0xFFFFFFFFA0000000ULL;
        map.mapped = FALSE;

        if (!(entry = mips_mts32_map(cpu, op_type, &map, entry, alt_entry, is_fromgdb)))
            goto err_undef;
        return (entry);

    case 0x06:                 /* ksseg */
    case 0x07:                 /* kseg3 */
        //ASSERT(0,"not implemented upper 1G memory space \n");
        /* trigger TLB exception if no matching entry found */
        if (!mips64_cp0_tlb_lookup(cpu, vaddr, &map))
            goto err_tlb;
        if ((map.valid & 0x1) != 0x1)
            goto err_tlb;
        if ((MTS_WRITE == op_type) && ((map.dirty & 0x1) != 0x1))
            goto err_mod;
        map.mapped = TRUE;
        if (!(entry = mips_mts32_map(cpu, op_type, &map, entry, alt_entry, is_fromgdb)))
            goto err_undef;

        return (entry);

        break;
    }
  err_mod:
    if (is_fromgdb)
        return NULL;
    mips_access_special(cpu, vaddr, MTS_ACC_M, op_code, op_type, op_size, data, exc);
    return NULL;
  err_undef:
    if (is_fromgdb)
        return NULL;
    mips_access_special(cpu, vaddr, MTS_ACC_U, op_code, op_type, op_size, data, exc);
    return NULL;
  err_tlb:
    if (is_fromgdb)
        return NULL;
    mips_access_special(cpu, vaddr, MTS_ACC_T, op_code, op_type, op_size, data, exc);
    return NULL;
}

static forced_inline int mips_mts32_check_tlbcache(cpu_mips_t * cpu, m_va_t vaddr, u_int op_type, mts32_entry_t * entry)
{
    m_uint32_t asid;
    mips_cp0_t *cp0 = &cpu->cp0;
    asid = cp0->reg[MIPS_CP0_TLB_HI] & MIPS_TLB_ASID_MASK;
    if (((m_uint32_t) vaddr & MIPS_MIN_PAGE_MASK) != entry->gvpa)
        return 0;
    if (entry->mapped == TRUE)
    {
        if ((op_type == MTS_WRITE) && (!entry->dirty_bit))
            return 0;
        if ((!entry->g_bit) && (asid != entry->asid))
            return 0;
    }
    return 1;
}

extern int tttt;
/* MTS32 access */
static
    void *mips_mts32_access(cpu_mips_t * cpu, m_va_t vaddr,
                            u_int op_code, u_int op_size,
                            u_int op_type, m_reg_t * data, u_int * exc, m_uint8_t * has_set_value, u_int is_fromgdb)
{
    mts32_entry_t *entry, alt_entry;
    m_uint32_t hash_bucket;
    m_iptr_t haddr;
    u_int dev_id;


/*
A job needs to be done first: check whether access is aligned!!!
MIPS FPU Emulator use an unaligned lw access to cause exception and then handle it.
see arch/mips/math-emu/dsemul.c  mips_dsemul

I did not check it before version 0.04 and hwclock/qtopia always segment fault.  
Very hard to find this problem!!!!
yajin
*/

    if (MTS_HALF_WORD == op_size)
    {
        if (unlikely((vaddr & 0x00000001UL) != 0x0))
        {
            goto err_addr;
        }
    }
    else if (MTS_WORD == op_size)
    {
        if ((op_code != MIPS_MEMOP_LWL) && (op_code != MIPS_MEMOP_LWR)
            && (op_code != MIPS_MEMOP_SWL) && (op_code != MIPS_MEMOP_SWR))
        {
            if (unlikely((vaddr & 0x00000003UL) != 0x0))
                goto err_addr;
        }

    }


    *exc = 0;
    hash_bucket = MTS32_HASH(vaddr);
    entry = &cpu->mts_u.mts32_cache[hash_bucket];

    if (unlikely(mips_mts32_check_tlbcache(cpu, vaddr, op_type, entry) == 0))
    {
        entry = mips_mts32_slow_lookup(cpu, vaddr, op_code, op_size, op_type, data, exc, &alt_entry, is_fromgdb);
        if (!entry)
            return NULL;
        if (entry->flags & MTS_FLAG_DEV)
        {
            dev_id = (entry->hpa & MTS_DEVID_MASK) >> MTS_DEVID_SHIFT;
            haddr = entry->hpa & MTS_DEVOFF_MASK;
            haddr += vaddr - entry->gvpa;

            return (dev_access_fast(cpu, dev_id, haddr, op_size, op_type, data, has_set_value));
        }
    }


    /* Raw memory access */
    haddr = entry->hpa + (vaddr & MIPS_MIN_PAGE_IMASK);
    return ((void *) haddr);
  err_addr:
    if (is_fromgdb)
        return NULL;
    mips_access_special(cpu, vaddr, MTS_ACC_AE, op_code, op_type, op_size, data, exc);
    return NULL;

}

/* MTS32 virtual address to physical address translation */
static int mips_mts32_translate(cpu_mips_t * cpu, m_va_t vaddr, m_pa_t * phys_page)
{
    mts32_entry_t *entry, alt_entry;
    m_uint32_t hash_bucket;
    m_reg_t data = 0;
    u_int exc = 0;


    hash_bucket = MTS32_HASH(vaddr);
    entry = &cpu->mts_u.mts32_cache[hash_bucket];


    if (unlikely(mips_mts32_check_tlbcache(cpu, vaddr, MTS_READ, entry) == 0))
    {
        entry = mips_mts32_slow_lookup(cpu, vaddr, MIPS_MEMOP_LOOKUP, 4, MTS_READ, &data, &exc, &alt_entry, 0);
        if (!entry)
            return (-1);

        ASSERT(!(entry->flags & MTS_FLAG_DEV), "error when translating virtual address to phyaddrss \n");
    }
    *phys_page = entry->gppa >> MIPS_MIN_PAGE_SHIFT;
    return (0);


}

/* ======================================================================== */

/* Shutdown MTS subsystem */
void mips_mem_shutdown(cpu_mips_t * cpu)
{
    if (cpu->mts_shutdown != NULL)
        cpu->mts_shutdown(cpu);
}

/* Set the address mode */
int mips_set_addr_mode(cpu_mips_t * cpu, u_int addr_mode)
{
    if (cpu->addr_mode != addr_mode)
    {
        mips_mem_shutdown(cpu);

        switch (addr_mode)
        {
        case 32:
            mips_mts32_init(cpu);
            mips_mts32_init_memop_vectors(cpu);
            break;
            /*case 64:
               TODO: 64 bit memory operation
               mips_mts64_init(cpu);
               mips_mts64_init_memop_vectors(cpu);
               break; */
        default:
            fprintf(stderr, "mts_set_addr_mode: internal error (addr_mode=%u)\n", addr_mode);
            exit(EXIT_FAILURE);
        }
    }

    return (0);
}



/*------------------DMA------------------------*/



/* Get host pointer for the physical ram address */
void *physmem_get_hptr(vm_instance_t * vm, m_pa_t paddr, u_int op_size, u_int op_type, m_uint32_t * data)
{

    struct vdevice *dev;
    m_uint32_t offset;

    m_uint8_t has_set_value;

    if (!(dev = dev_lookup(vm, paddr)))
        return NULL;

    /*Only for RAM */
    if ((dev->host_addr != 0) && !(dev->flags & VDEVICE_FLAG_NO_MTS_MMAP))
        return ((void *) dev->host_addr + (paddr - dev->phys_addr));

    if (op_size == 0)
        return NULL;

    ASSERT(0, "physmem_get_hptr error\n");
    offset = paddr - dev->phys_addr;
    return (dev->handler(vm->cpu, dev, offset, op_size, op_type, data, &has_set_value));
}

/* DMA transfer operation */
void physmem_dma_transfer(vm_instance_t * vm, m_pa_t src, m_pa_t dst, size_t len)
{
    m_uint32_t dummy;
    u_char *sptr, *dptr;
    size_t clen, sl, dl;

    while (len > 0)
    {
        sptr = physmem_get_hptr(vm, src, 0, MTS_READ, &dummy);
        dptr = physmem_get_hptr(vm, dst, 0, MTS_WRITE, &dummy);

        if (!sptr || !dptr)
        {
            vm_log(vm, "DMA", "unable to transfer from 0x%" LL "x to 0x%" LL "x\n", src, dst);
            ASSERT(0, "physmem_dma_transfer src %x dst %x\n", src, dst);
            return;
        }

        sl = VM_PAGE_SIZE - (src & VM_PAGE_IMASK);
        dl = VM_PAGE_SIZE - (dst & VM_PAGE_IMASK);
        clen = m_min(sl, dl);
        clen = m_min(clen, len);

        memcpy(dptr, sptr, clen);

        src += clen;
        dst += clen;
        len -= clen;
    }
}
