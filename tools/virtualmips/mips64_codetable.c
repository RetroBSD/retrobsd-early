/*
 * Code dispatch table.
 *
 * Copyright (C) yajin 2008 <yajinzhou@gmail.com >
 *
 * This file is part of the virtualmips distribution.
 * See LICENSE file for terms of the license.
 */

static int unknown_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    printf ("unknown instruction. pc %x insn %x\n", cpu->pc, insn);
    exit (EXIT_FAILURE);
}

static int add_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int rd = bits (insn, 11, 15);
    m_reg_t res;

    /* TODO: Exception handling */
    res = (m_uint32_t) cpu->gpr[rs] + (m_uint32_t) cpu->gpr[rt];
    cpu->gpr[rd] = sign_extend (res, 32);
    return (0);
}

static int addi_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int imm = bits (insn, 0, 15);
    m_uint32_t res, val = sign_extend (imm, 16);

    /* TODO: Exception handling */
    res = (m_uint32_t) cpu->gpr[rs] + val;
    cpu->gpr[rt] = sign_extend (res, 32);
    return (0);
}

static int addiu_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int imm = bits (insn, 0, 15);
    m_uint32_t res, val = sign_extend (imm, 16);

    res = (m_uint32_t) cpu->gpr[rs] + val;
    cpu->gpr[rt] = sign_extend (res, 32);

    return (0);
}

static int addu_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int rd = bits (insn, 11, 15);
    m_uint32_t res;

    res = (m_uint32_t) cpu->gpr[rs] + (m_uint32_t) cpu->gpr[rt];
    cpu->gpr[rd] = sign_extend (res, 32);
    return (0);
}

static int and_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int rd = bits (insn, 11, 15);

    cpu->gpr[rd] = cpu->gpr[rs] & cpu->gpr[rt];
    return (0);

}

static int andi_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int imm = bits (insn, 0, 15);

    cpu->gpr[rt] = cpu->gpr[rs] & imm;
    return (0);
}

static int bcond_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    uint16_t special_func = bits (insn, 16, 20);
    return mips_bcond_opcodes[special_func].func (cpu, insn);
}

static int beq_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int offset = bits (insn, 0, 15);
    m_va_t new_pc;
    int res;

    /* compute the new pc */
    new_pc = (cpu->pc + 4) + sign_extend (offset << 2, 18);

    /* take the branch if gpr[rs] == gpr[rt] */
    res = (cpu->gpr[rs] == cpu->gpr[rt]);

    /* exec the instruction in the delay slot */
    int ins_res = mips64_exec_bdslot (cpu);
    if (likely (!ins_res)) {
        if (res)
            cpu->pc = new_pc;
        else
            cpu->pc += 8;
    }

    return (1);
}

static int beql_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int offset = bits (insn, 0, 15);
    m_va_t new_pc;
    int res;

    /* compute the new pc */
    new_pc = (cpu->pc + 4) + sign_extend (offset << 2, 18);

    /* take the branch if gpr[rs] == gpr[rt] */
    res = (cpu->gpr[rs] == cpu->gpr[rt]);

    /* take the branch if the test result is true */
    if (res) {
        int ins_res = mips64_exec_bdslot (cpu);
        if (likely (!ins_res))
            cpu->pc = new_pc;
    } else
        cpu->pc += 8;

    return (1);
}

static int bgez_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int offset = bits (insn, 0, 15);
    m_va_t new_pc;
    int res;

    /* compute the new pc */
    new_pc = (cpu->pc + 4) + sign_extend (offset << 2, 18);

    /* take the branch if gpr[rs] >= 0 */
    res = ((m_ireg_t) cpu->gpr[rs] >= 0);

    /* exec the instruction in the delay slot */
    /* exec the instruction in the delay slot */
    int ins_res = mips64_exec_bdslot (cpu);

    if (likely (!ins_res)) {
        /* take the branch if the test result is true */
        if (res)
            cpu->pc = new_pc;
        else
            cpu->pc += 8;
    }
    return (1);
}

static int bgezal_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int offset = bits (insn, 0, 15);
    m_va_t new_pc;
    int res;

    /* compute the new pc */
    new_pc = (cpu->pc + 4) + sign_extend (offset << 2, 18);

    /* set the return address (instruction after the delay slot) */
    cpu->gpr[MIPS_GPR_RA] = cpu->pc + 8;

    /* take the branch if gpr[rs] >= 0 */
    res = ((m_ireg_t) cpu->gpr[rs] >= 0);

    /* exec the instruction in the delay slot */
    int ins_res = mips64_exec_bdslot (cpu);

    if (likely (!ins_res)) {
        /* take the branch if the test result is true */
        if (res)
            cpu->pc = new_pc;
        else
            cpu->pc += 8;
    }
    return (1);
}

static int bgezall_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int offset = bits (insn, 0, 15);
    m_va_t new_pc;
    int res;

    /* compute the new pc */
    new_pc = (cpu->pc + 4) + sign_extend (offset << 2, 18);

    /* set the return address (instruction after the delay slot) */
    cpu->gpr[MIPS_GPR_RA] = cpu->pc + 8;

    /* take the branch if gpr[rs] >= 0 */
    res = ((m_ireg_t) cpu->gpr[rs] >= 0);

    /* take the branch if the test result is true */
    if (res) {
        mips64_exec_bdslot (cpu);
        cpu->pc = new_pc;
    } else
        cpu->pc += 8;

    return (1);

}

static int bgezl_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int offset = bits (insn, 0, 15);
    m_va_t new_pc;
    int res;

    /* compute the new pc */
    new_pc = (cpu->pc + 4) + sign_extend (offset << 2, 18);

    /* take the branch if gpr[rs] >= 0 */
    res = ((m_ireg_t) cpu->gpr[rs] >= 0);

    /* take the branch if the test result is true */
    if (res) {
        mips64_exec_bdslot (cpu);
        cpu->pc = new_pc;
    } else
        cpu->pc += 8;

    return (1);

}

static int bgtz_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int offset = bits (insn, 0, 15);
    m_va_t new_pc;
    int res;

    /* compute the new pc */
    new_pc = (cpu->pc + 4) + sign_extend (offset << 2, 18);

    /* take the branch if gpr[rs] > 0 */
    res = ((m_ireg_t) cpu->gpr[rs] > 0);

    /* exec the instruction in the delay slot */
    int ins_res = mips64_exec_bdslot (cpu);

    if (likely (!ins_res)) {
        /* take the branch if the test result is true */
        if (res)
            cpu->pc = new_pc;
        else
            cpu->pc += 8;
    }
    return (1);
}

static int bgtzl_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int offset = bits (insn, 0, 15);
    m_va_t new_pc;
    int res;

    /* compute the new pc */
    new_pc = (cpu->pc + 4) + sign_extend (offset << 2, 18);

    /* take the branch if gpr[rs] > 0 */
    res = ((m_ireg_t) cpu->gpr[rs] > 0);

    /* take the branch if the test result is true */
    if (res) {
        mips64_exec_bdslot (cpu);
        cpu->pc = new_pc;
    } else
        cpu->pc += 8;

    return (1);
}

static int blez_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int offset = bits (insn, 0, 15);
    m_va_t new_pc;
    int res;

    /* compute the new pc */
    new_pc = (cpu->pc + 4) + sign_extend (offset << 2, 18);

    /* take the branch if gpr[rs] <= 0 */
    res = ((m_ireg_t) cpu->gpr[rs] <= 0);

    /* exec the instruction in the delay slot */
    int ins_res = mips64_exec_bdslot (cpu);

    if (likely (!ins_res)) {
        /* take the branch if the test result is true */
        if (res)
            cpu->pc = new_pc;
        else
            cpu->pc += 8;
    }
    return (1);
}

static int blezl_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int offset = bits (insn, 0, 15);
    m_va_t new_pc;
    int res;

    /* compute the new pc */
    new_pc = (cpu->pc + 4) + sign_extend (offset << 2, 18);

    /* take the branch if gpr[rs] <= 0 */
    res = ((m_ireg_t) cpu->gpr[rs] <= 0);

    /* take the branch if the test result is true */
    if (res) {
        mips64_exec_bdslot (cpu);
        cpu->pc = new_pc;
    } else
        cpu->pc += 8;

    return (1);
}

static int bltz_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int offset = bits (insn, 0, 15);
    m_va_t new_pc;
    int res;

    /* compute the new pc */
    new_pc = (cpu->pc + 4) + sign_extend (offset << 2, 18);

    /* take the branch if gpr[rs] < 0 */
    res = ((m_ireg_t) cpu->gpr[rs] < 0);

    /* exec the instruction in the delay slot */
    int ins_res = mips64_exec_bdslot (cpu);

    if (likely (!ins_res)) {
        /* take the branch if the test result is true */
        if (res)
            cpu->pc = new_pc;
        else
            cpu->pc += 8;
    }
    return (1);
}

static int bltzal_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int offset = bits (insn, 0, 15);
    m_va_t new_pc;
    int res;

    /* compute the new pc */
    new_pc = (cpu->pc + 4) + sign_extend (offset << 2, 18);

    /* set the return address (instruction after the delay slot) */
    cpu->gpr[MIPS_GPR_RA] = cpu->pc + 8;

    /* take the branch if gpr[rs] < 0 */
    res = ((m_ireg_t) cpu->gpr[rs] < 0);

    /* exec the instruction in the delay slot */
    int ins_res = mips64_exec_bdslot (cpu);

    if (likely (!ins_res)) {
        /* take the branch if the test result is true */
        if (res)
            cpu->pc = new_pc;
        else
            cpu->pc += 8;
    }
    return (1);
}

static int bltzall_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int offset = bits (insn, 0, 15);
    m_va_t new_pc;
    int res;

    /* compute the new pc */
    new_pc = (cpu->pc + 4) + sign_extend (offset << 2, 18);

    /* set the return address (instruction after the delay slot) */
    cpu->gpr[MIPS_GPR_RA] = cpu->pc + 8;

    /* take the branch if gpr[rs] < 0 */
    res = ((m_ireg_t) cpu->gpr[rs] < 0);

    /* take the branch if the test result is true */
    if (res) {
        mips64_exec_bdslot (cpu);
        cpu->pc = new_pc;
    } else
        cpu->pc += 8;

    return (1);
}

static int bltzl_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int offset = bits (insn, 0, 15);
    m_va_t new_pc;
    int res;

    /* compute the new pc */
    new_pc = (cpu->pc + 4) + sign_extend (offset << 2, 18);

    /* take the branch if gpr[rs] < 0 */
    res = ((m_ireg_t) cpu->gpr[rs] < 0);

    /* take the branch if the test result is true */
    if (res) {
        mips64_exec_bdslot (cpu);
        cpu->pc = new_pc;
    } else
        cpu->pc += 8;

    return (1);
}

static int bne_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int offset = bits (insn, 0, 15);
    m_va_t new_pc;
    int res;

    /* compute the new pc */
    new_pc = (cpu->pc + 4) + sign_extend (offset << 2, 18);

    /* take the branch if gpr[rs] != gpr[rt] */
    res = (cpu->gpr[rs] != cpu->gpr[rt]);

    /* exec the instruction in the delay slot */
    int ins_res = mips64_exec_bdslot (cpu);

    if (likely (!ins_res)) {
        /* take the branch if the test result is true */
        if (res)
            cpu->pc = new_pc;
        else
            cpu->pc += 8;
    }

    return (1);
}

static int bnel_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int offset = bits (insn, 0, 15);
    m_va_t new_pc;
    int res;

    /* compute the new pc */
    new_pc = (cpu->pc + 4) + sign_extend (offset << 2, 18);

    /* take the branch if gpr[rs] != gpr[rt] */
    res = (cpu->gpr[rs] != cpu->gpr[rt]);

    /* take the branch if the test result is true */
    if (res) {
        mips64_exec_bdslot (cpu);
        cpu->pc = new_pc;
    } else
        cpu->pc += 8;
    return (1);
}

static int break_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    u_int code = bits (insn, 6, 25);

    mips64_exec_break (cpu, code);
    return (1);
}

static int cache_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int base = bits (insn, 21, 25);
    int op = bits (insn, 16, 20);
    int offset = bits (insn, 0, 15);

    return (mips64_exec_memop2 (cpu, MIPS_MEMOP_CACHE, base, offset, op,
            FALSE));
}

static int cfc0_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int clz_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rd = bits (insn, 11, 15);
    int i;
    m_uint32_t val;
    val = 32;
    for (i = 31; i >= 0; i--) {
        if (cpu->gpr[rs] & (1 << i)) {
            val = 31 - i;
            break;
        }
    }
    cpu->gpr[rd] = val;
    return (0);

}

static int cop0_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    uint16_t special_func = bits (insn, 21, 25);
//printf ("cop0 instruction. func %x\n", special_func);
    return mips_cop0_opcodes[special_func].func (cpu, insn);
}

static int cop1_op (cpu_mips_t * cpu, mips_insn_t insn)
{
#if SOFT_FPU
    mips64_exec_soft_fpu (cpu);
    return (1);
#else
    return unknown_op (cpu, insn);
#endif
}

static int cop1x_op (cpu_mips_t * cpu, mips_insn_t insn)
{
#if SOFT_FPU
    mips64_exec_soft_fpu (cpu);
    return (1);
#else
    return unknown_op (cpu, insn);
#endif
}

static int cop2_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int dadd_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int daddi_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int daddiu_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int daddu_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int ddiv_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int ddivu_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int div_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);

    cpu->lo = (m_int32_t) cpu->gpr[rs] / (m_int32_t) cpu->gpr[rt];
    cpu->hi = (m_int32_t) cpu->gpr[rs] % (m_int32_t) cpu->gpr[rt];

    cpu->lo = sign_extend (cpu->lo, 32);
    cpu->hi = sign_extend (cpu->hi, 32);
    return (0);
}

static int divu_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);

    if (cpu->gpr[rt] == 0)
        return (0);

    cpu->lo = (m_uint32_t) cpu->gpr[rs] / (m_uint32_t) cpu->gpr[rt];
    cpu->hi = (m_uint32_t) cpu->gpr[rs] % (m_uint32_t) cpu->gpr[rt];

    cpu->lo = sign_extend (cpu->lo, 32);
    cpu->hi = sign_extend (cpu->hi, 32);
    return (0);

}

static int dmfc0_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int dmtc0_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int dmult_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int dmultu_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int dsll_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int dsllv_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int dsrlv_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int dsrav_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int dsub_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int dsubu_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int dsrl_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int dsra_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int dsll32_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int dsrl32_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int dsra32_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int eret_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    mips64_exec_eret (cpu);
    return (1);
}

static int j_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    u_int instr_index = bits (insn, 0, 25);
    m_va_t new_pc;

    /* compute the new pc */
    new_pc = cpu->pc & ~((1 << 28) - 1);
    new_pc |= instr_index << 2;

    /* exec the instruction in the delay slot */
    int ins_res = mips64_exec_bdslot (cpu);
    if (likely (!ins_res))
        cpu->pc = new_pc;
    return (1);
}

static int jal_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    u_int instr_index = bits (insn, 0, 25);
    m_va_t new_pc;

    /* compute the new pc */
    new_pc = cpu->pc & ~((1 << 28) - 1);
    new_pc |= instr_index << 2;

    /* set the return address (instruction after the delay slot) */
    cpu->gpr[MIPS_GPR_RA] = cpu->pc + 8;

    int ins_res = mips64_exec_bdslot (cpu);
    if (likely (!ins_res))
        cpu->pc = new_pc;

    return (1);
}

static int jalr_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rd = bits (insn, 11, 15);
    m_va_t new_pc;

    /* set the return pc (instruction after the delay slot) in GPR[rd] */
    cpu->gpr[rd] = cpu->pc + 8;

    /* get the new pc */
    new_pc = cpu->gpr[rs];

    int ins_res = mips64_exec_bdslot (cpu);
    if (likely (!ins_res))
        cpu->pc = new_pc;
    return (1);

}

static int jr_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    m_va_t new_pc;

    /* get the new pc */
    new_pc = cpu->gpr[rs];

    int ins_res = mips64_exec_bdslot (cpu);
    if (likely (!ins_res))
        cpu->pc = new_pc;
    return (1);

}

static int seh_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rt = bits (insn, 16, 20);
    int rd = bits (insn, 11, 15);
    int func = bits (insn, 0, 10);
//printf ("seh rt=%d, rd=%d, func=%x\n", rt, rd, func);
    switch (func) {
    case 0x420:
        /* seb - sign extend byte */
        cpu->gpr[rd] = sign_extend (cpu->gpr[rt], 8);
        return (0);
    case 0x620:
        /* seh - sign extend halfword */
        cpu->gpr[rd] = sign_extend (cpu->gpr[rt], 16);
        return (0);
    }
    return unknown_op (cpu, insn);
}

static int lb_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int base = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int offset = bits (insn, 0, 15);

    return (mips64_exec_memop2 (cpu, MIPS_MEMOP_LB, base, offset, rt, TRUE));
}

static int lbu_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int base = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int offset = bits (insn, 0, 15);

    return (mips64_exec_memop2 (cpu, MIPS_MEMOP_LBU, base, offset, rt, TRUE));
}

static int ld_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int ldc1_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int ldc2_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int ldl_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int ldr_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int lh_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int base = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int offset = bits (insn, 0, 15);

    return (mips64_exec_memop2 (cpu, MIPS_MEMOP_LH, base, offset, rt, TRUE));
}

static int lhu_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int base = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int offset = bits (insn, 0, 15);

    return (mips64_exec_memop2 (cpu, MIPS_MEMOP_LHU, base, offset, rt, TRUE));
}

static int ll_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int base = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int offset = bits (insn, 0, 15);

    return (mips64_exec_memop2 (cpu, MIPS_MEMOP_LL, base, offset, rt, TRUE));
}

static int lld_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int lui_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rt = bits (insn, 16, 20);
    int imm = bits (insn, 0, 15);

    cpu->gpr[rt] = sign_extend (imm, 16) << 16;
    return (0);
}

static int lw_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int base = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int offset = bits (insn, 0, 15);

    return (mips64_exec_memop2 (cpu, MIPS_MEMOP_LW, base, offset, rt, TRUE));

}

static int lwc1_op (cpu_mips_t * cpu, mips_insn_t insn)
{
#if SOFT_FPU
    mips64_exec_soft_fpu (cpu);
    return (1);
#else
    return unknown_op (cpu, insn);
#endif

}

static int lwc2_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int lwl_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int base = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int offset = bits (insn, 0, 15);

    return (mips64_exec_memop2 (cpu, MIPS_MEMOP_LWL, base, offset, rt, TRUE));

}

static int lwr_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int base = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int offset = bits (insn, 0, 15);

    return (mips64_exec_memop2 (cpu, MIPS_MEMOP_LWR, base, offset, rt, TRUE));
}

static int lwu_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int base = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int offset = bits (insn, 0, 15);

    return (mips64_exec_memop2 (cpu, MIPS_MEMOP_LWU, base, offset, rt, TRUE));
}

static int mad_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int index = bits (insn, 0, 5);
    return mips_mad_opcodes[index].func (cpu, insn);
}

static int madd_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    m_int64_t val, temp;

    val = (m_int32_t) (m_int32_t) cpu->gpr[rs];
    val *= (m_int32_t) (m_int32_t) cpu->gpr[rt];

    temp = cpu->hi;
    temp = temp << 32;
    temp += cpu->lo;
    val += temp;

    cpu->lo = sign_extend (val, 32);
    cpu->hi = sign_extend (val >> 32, 32);
    return (0);
}

static int maddu_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    m_int64_t val, temp;

    val = (m_uint32_t) (m_uint32_t) cpu->gpr[rs];
    val *= (m_uint32_t) (m_uint32_t) cpu->gpr[rt];

    temp = cpu->hi;
    temp = temp << 32;
    temp += cpu->lo;
    val += temp;

    cpu->lo = sign_extend (val, 32);
    cpu->hi = sign_extend (val >> 32, 32);
    return (0);
}

static int mfc0_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rt = bits (insn, 16, 20);
    int rd = bits (insn, 11, 15);
    int sel = bits (insn, 0, 2);
    //mfc rt,rd

    mips64_cp0_exec_mfc0 (cpu, rt, rd, sel);
    return (0);
}

static int mfhi_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rd = bits (insn, 11, 15);

    if (rd)
        cpu->gpr[rd] = cpu->hi;
    return (0);
}

static int mflo_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rd = bits (insn, 11, 15);

    if (rd)
        cpu->gpr[rd] = cpu->lo;
    return (0);
}

static int movc_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int movz_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rd = bits (insn, 11, 15);
    int rt = bits (insn, 16, 20);

    if ((cpu->gpr[rt]) == 0)
        cpu->gpr[rd] = sign_extend (cpu->gpr[rs], 32);
    return (0);
}

static int movn_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rd = bits (insn, 11, 15);
    int rt = bits (insn, 16, 20);

    // printf("pc %x rs %x rd %x rt %x\n",cpu->pc,rs,rd,rt);
    if ((cpu->gpr[rt]) != 0)
        cpu->gpr[rd] = sign_extend (cpu->gpr[rs], 32);
    return (0);
}

static int msub_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    m_int64_t val, temp;

    val = (m_int32_t) (m_int32_t) cpu->gpr[rs];
    val *= (m_int32_t) (m_int32_t) cpu->gpr[rt];

    temp = cpu->hi;
    temp = temp << 32;
    temp += cpu->lo;

    temp -= val;
    //val += temp;

    cpu->lo = sign_extend (temp, 32);
    cpu->hi = sign_extend (temp >> 32, 32);
    return (0);
}

static int msubu_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    m_int64_t val, temp;

    val = (m_uint32_t) (m_uint32_t) cpu->gpr[rs];
    val *= (m_uint32_t) (m_uint32_t) cpu->gpr[rt];

    temp = cpu->hi;
    temp = temp << 32;
    temp += cpu->lo;

    temp -= val;
    //val += temp;

    cpu->lo = sign_extend (temp, 32);
    cpu->hi = sign_extend (temp >> 32, 32);
    return (0);
}

static int mtc0_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rt = bits (insn, 16, 20);
    int rd = bits (insn, 11, 15);
    int sel = bits (insn, 0, 2);

    //printf("cpu->pc %x insn %x\n",cpu->pc,insn);
    mips64_cp0_exec_mtc0 (cpu, rt, rd, sel);
    return (0);
}

static int mthi_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);

    cpu->hi = cpu->gpr[rs];
    return (0);
}

static int mtlo_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);

    cpu->lo = cpu->gpr[rs];
    return (0);
}

static int mul_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int rd = bits (insn, 11, 15);
    m_int32_t val;

    /* note: after this instruction, HI/LO regs are undefined */
    val = (m_int32_t) cpu->gpr[rs] * (m_int32_t) cpu->gpr[rt];
    cpu->gpr[rd] = sign_extend (val, 32);
    return (0);
}

static int mult_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    m_int64_t val;

    val = (m_int64_t) (m_int32_t) cpu->gpr[rs];
    val *= (m_int64_t) (m_int32_t) cpu->gpr[rt];

    cpu->lo = sign_extend (val, 32);
    cpu->hi = sign_extend (val >> 32, 32);
    return (0);
}

static int multu_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    m_int64_t val;              //must be 64 bit. not m_reg_t !!!

    val = (m_reg_t) (m_uint32_t) cpu->gpr[rs];
    val *= (m_reg_t) (m_uint32_t) cpu->gpr[rt];
    cpu->lo = sign_extend (val, 32);
    cpu->hi = sign_extend (val >> 32, 32);
    return (0);
}

static int nor_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int rd = bits (insn, 11, 15);

    cpu->gpr[rd] = ~(cpu->gpr[rs] | cpu->gpr[rt]);
    return (0);
}

static int or_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int rd = bits (insn, 11, 15);

    cpu->gpr[rd] = cpu->gpr[rs] | cpu->gpr[rt];
    return (0);
}

static int ori_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int imm = bits (insn, 0, 15);

    cpu->gpr[rt] = cpu->gpr[rs] | imm;
    return (0);
}

static int pref_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return (0);
}

static int sb_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int base = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int offset = bits (insn, 0, 15);
    return (mips64_exec_memop2 (cpu, MIPS_MEMOP_SB, base, offset, rt, FALSE));
}

static int sc_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int base = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int offset = bits (insn, 0, 15);

    return (mips64_exec_memop2 (cpu, MIPS_MEMOP_SC, base, offset, rt, TRUE));
}

static int scd_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int sd_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int sdc1_op (cpu_mips_t * cpu, mips_insn_t insn)
{
#if SOFT_FPU
    mips64_exec_soft_fpu (cpu);
    return (1);
#else
    return unknown_op (cpu, insn);
#endif
}

static int sdc2_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int sdl_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int sdr_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int sh_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int base = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int offset = bits (insn, 0, 15);

    return (mips64_exec_memop2 (cpu, MIPS_MEMOP_SH, base, offset, rt, FALSE));
}

static int sll_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rt = bits (insn, 16, 20);
    int rd = bits (insn, 11, 15);
    int sa = bits (insn, 6, 10);
    m_uint32_t res;

    res = (m_uint32_t) cpu->gpr[rt] << sa;
    cpu->gpr[rd] = sign_extend (res, 32);
    return (0);
}

static int sllv_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int rd = bits (insn, 11, 15);
    m_uint32_t res;

    res = (m_uint32_t) cpu->gpr[rt] << (cpu->gpr[rs] & 0x1f);
    cpu->gpr[rd] = sign_extend (res, 32);
    return (0);
}

static int slt_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int rd = bits (insn, 11, 15);

    if ((m_ireg_t) cpu->gpr[rs] < (m_ireg_t) cpu->gpr[rt])
        cpu->gpr[rd] = 1;
    else
        cpu->gpr[rd] = 0;

    return (0);
}

static int slti_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int imm = bits (insn, 0, 15);
    m_ireg_t val = sign_extend (imm, 16);

    if ((m_ireg_t) cpu->gpr[rs] < val)
        cpu->gpr[rt] = 1;
    else
        cpu->gpr[rt] = 0;

    return (0);
}

static int sltiu_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int imm = bits (insn, 0, 15);
    m_reg_t val = sign_extend (imm, 16);

    if (cpu->gpr[rs] < val)
        cpu->gpr[rt] = 1;
    else
        cpu->gpr[rt] = 0;

    return (0);
}

static int sltu_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int rd = bits (insn, 11, 15);

    if (cpu->gpr[rs] < cpu->gpr[rt])
        cpu->gpr[rd] = 1;
    else
        cpu->gpr[rd] = 0;

    return (0);
}

static int spec_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    uint16_t special_func = bits (insn, 0, 5);
    return mips_spec_opcodes[special_func].func (cpu, insn);
}

static int sra_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rt = bits (insn, 16, 20);
    int rd = bits (insn, 11, 15);
    int sa = bits (insn, 6, 10);
    m_int32_t res;

    res = (m_int32_t) cpu->gpr[rt] >> sa;
    cpu->gpr[rd] = sign_extend (res, 32);
    return (0);
}

static int srav_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int rd = bits (insn, 11, 15);
    m_int32_t res;

    res = (m_int32_t) cpu->gpr[rt] >> (cpu->gpr[rs] & 0x1f);
    cpu->gpr[rd] = sign_extend (res, 32);
    return (0);
}

static int srl_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rt = bits (insn, 16, 20);
    int rd = bits (insn, 11, 15);
    int sa = bits (insn, 6, 10);
    m_uint32_t res;

    res = (m_uint32_t) cpu->gpr[rt] >> sa;
    cpu->gpr[rd] = sign_extend (res, 32);
    return (0);
}

static int srlv_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int rd = bits (insn, 11, 15);
    m_uint32_t res;

    res = (m_uint32_t) cpu->gpr[rt] >> (cpu->gpr[rs] & 0x1f);
    cpu->gpr[rd] = sign_extend (res, 32);
    return (0);
}

static int sub_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int rd = bits (insn, 11, 15);
    m_uint32_t res;

    /* TODO: Exception handling */
    res = (m_uint32_t) cpu->gpr[rs] - (m_uint32_t) cpu->gpr[rt];
    cpu->gpr[rd] = sign_extend (res, 32);
    return (0);
}

static int subu_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int rd = bits (insn, 11, 15);
    m_uint32_t res;
    res = (m_uint32_t) cpu->gpr[rs] - (m_uint32_t) cpu->gpr[rt];
    cpu->gpr[rd] = sign_extend (res, 32);

    return (0);
}

static int sw_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int base = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int offset = bits (insn, 0, 15);

    return (mips64_exec_memop2 (cpu, MIPS_MEMOP_SW, base, offset, rt, FALSE));
}

static int swc1_op (cpu_mips_t * cpu, mips_insn_t insn)
{
#if SOFT_FPU
    mips64_exec_soft_fpu (cpu);
    return (1);
#else
    return unknown_op (cpu, insn);
#endif
}

static int swc2_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int swl_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int base = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int offset = bits (insn, 0, 15);

    return (mips64_exec_memop2 (cpu, MIPS_MEMOP_SWL, base, offset, rt,
            FALSE));
}

static int swr_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int base = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int offset = bits (insn, 0, 15);

    return (mips64_exec_memop2 (cpu, MIPS_MEMOP_SWR, base, offset, rt,
            FALSE));
}

static int sync_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return (0);
}

static int syscall_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    mips64_exec_syscall (cpu);
    return (1);
}

static int teq_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);

    if (unlikely (cpu->gpr[rs] == cpu->gpr[rt])) {
        mips64_trigger_trap_exception (cpu);
        return (1);
    }
    return (0);
}

static int teqi_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int imm = bits (insn, 0, 15);
    m_reg_t val = sign_extend (imm, 16);

    if (unlikely (cpu->gpr[rs] == val)) {
        mips64_trigger_trap_exception (cpu);
        return (1);
    }
    return (0);
}

static int mfmc0_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rt = bits (insn, 16, 20);
    int rd = bits (insn, 11, 15);
    int func = bits (insn, 0, 5);

    if (rd != 12)
        return unknown_op (cpu, insn);

    cpu->gpr[rt] = cpu->cp0.reg [MIPS_CP0_STATUS];
    if (func & 0x20) {
        /* ei - enable interrupts */
        cpu->cp0.reg [MIPS_CP0_STATUS] |= MIPS_CP0_STATUS_IE;
    } else {
        /* di - disable interrupts */
        cpu->cp0.reg [MIPS_CP0_STATUS] &= MIPS_CP0_STATUS_IE;
    }
    return 0;
}

static int tlb_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    uint16_t func = bits (insn, 0, 5);
    return mips_tlb_opcodes[func].func (cpu, insn);
}

static int tlbp_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    mips64_cp0_exec_tlbp (cpu);
    return (0);
}

static int tlbr_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    mips64_cp0_exec_tlbr (cpu);
    return (0);
}

static int tlbwi_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    mips64_cp0_exec_tlbwi (cpu);
    return (0);
}

static int tlbwr_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    mips64_cp0_exec_tlbwr (cpu);
    return (0);
}

static int tge_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int tgei_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int tgeiu_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int tgeu_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int tlt_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int tlti_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int tltiu_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int tltu_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int tne_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);

    if ((m_ireg_t) cpu->gpr[rs] != (m_ireg_t) cpu->gpr[rt]) {
        /*take a trap */
        mips64_trigger_trap_exception (cpu);
        return (1);
    } else
        return (0);
}

static int tnei_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int wait_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    usleep (1000);
    return (0);
}

static int xor_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int rd = bits (insn, 11, 15);

    cpu->gpr[rd] = cpu->gpr[rs] ^ cpu->gpr[rt];
    return (0);
}

static int xori_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    int rs = bits (insn, 21, 25);
    int rt = bits (insn, 16, 20);
    int imm = bits (insn, 0, 15);

    cpu->gpr[rt] = cpu->gpr[rs] ^ imm;
    return (0);

}

static int undef_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int unknownBcondOp (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);

}

static int unknowncop0_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int unknownmad_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int unknownSpecOp (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

static int unknowntlb_op (cpu_mips_t * cpu, mips_insn_t insn)
{
    return unknown_op (cpu, insn);
}

/*instruction table*/

static struct mips64_op_desc mips_opcodes[] = {
    {"spec", spec_op, 0x00},
    {"bcond", bcond_op, 0x01},
    {"j", j_op, 0x02},
    {"jal", jal_op, 0x03},
    {"beq", beq_op, 0x04},
    {"bne", bne_op, 0x05},
    {"blez", blez_op, 0x06},
    {"bgtz", bgtz_op, 0x07},
    {"addi", addi_op, 0x08},
    {"addiu", addiu_op, 0x09},
    {"slti", slti_op, 0x0A},
    {"sltiu", sltiu_op, 0x0B},
    {"andi", andi_op, 0x0C},
    {"ori", ori_op, 0x0D},
    {"xori", xori_op, 0x0E},
    {"lui", lui_op, 0x0F},
    {"cop0", cop0_op, 0x10},
    {"cop1", cop1_op, 0x11},
    {"cop2", cop2_op, 0x12},
    {"cop1x", cop1x_op, 0x13},
    {"beql", beql_op, 0x14},
    {"bnel", bnel_op, 0x15},
    {"blezl", blezl_op, 0x16},
    {"bgtzl", bgtzl_op, 0x17},
    {"daddi", daddi_op, 0x18},
    {"daddiu", daddiu_op, 0x19},
    {"ldl", ldl_op, 0x1A},
    {"ldr", ldr_op, 0x1B},
    {"undef", mad_op, 0x1C},
    {"undef", undef_op, 0x1D},
    {"undef", undef_op, 0x1E},
    {"seh", seh_op, 0x1F},
    {"lb", lb_op, 0x20},
    {"lh", lh_op, 0x21},
    {"lwl", lwl_op, 0x22},
    {"lw", lw_op, 0x23},
    {"lbu", lbu_op, 0x24},
    {"lhu", lhu_op, 0x25},
    {"lwr", lwr_op, 0x26},
    {"lwu", lwu_op, 0x27},
    {"sb", sb_op, 0x28},
    {"sh", sh_op, 0x29},
    {"swl", swl_op, 0x2A},
    {"sw", sw_op, 0x2B},
    {"sdl", sdl_op, 0x2C},
    {"sdr", sdr_op, 0x2D},
    {"swr", swr_op, 0x2E},
    {"cache", cache_op, 0x2F},
    {"ll", ll_op, 0x30},
    {"lwc1", lwc1_op, 0x31},
    {"lwc2", lwc2_op, 0x32},
    {"pref", pref_op, 0x33},
    {"lld", lld_op, 0x34},
    {"ldc1", ldc1_op, 0x35},
    {"ldc2", ldc2_op, 0x36},
    {"ld", ld_op, 0x37},
    {"sc", sc_op, 0x38},
    {"swc1", swc1_op, 0x39},
    {"swc2", swc2_op, 0x3A},
    {"undef", undef_op, 0x3B},
    {"scd", scd_op, 0x3C},
    {"sdc1", sdc1_op, 0x3D},
    {"sdc2", sdc2_op, 0x3E},
    {"sd", sd_op, 0x3F},
};

/* Based on the func field of spec opcode */
static struct mips64_op_desc mips_spec_opcodes[] = {
    {"sll", sll_op, 0x00},
    {"movc", movc_op, 0x01},
    {"srl", srl_op, 0x02},
    {"sra", sra_op, 0x03},
    {"sllv", sllv_op, 0x04},
    {"unknownSpec", unknownSpecOp, 0x05},
    {"srlv", srlv_op, 0x06},
    {"srav", srav_op, 0x07},
    {"jr", jr_op, 0x08},
    {"jalr", jalr_op, 0x09},
    {"movz", movz_op, 0x0A},
    {"movn", movn_op, 0x0B},
    {"syscall", syscall_op, 0x0C},
    {"break", break_op, 0x0D},
    {"spim", unknownSpecOp, 0x0E},
    {"sync", sync_op, 0x0F},
    {"mfhi", mfhi_op, 0x10},
    {"mthi", mthi_op, 0x11},
    {"mflo", mflo_op, 0x12},
    {"mtlo", mtlo_op, 0x13},
    {"dsllv", dsllv_op, 0x14},
    {"unknownSpec", unknownSpecOp, 0x15},
    {"dsrlv", dsrlv_op, 0x16},
    {"dsrav", dsrav_op, 0x17},
    {"mult", mult_op, 0x18},
    {"multu", multu_op, 0x19},
    {"div", div_op, 0x1A},
    {"divu", divu_op, 0x1B},
    {"dmult", dmult_op, 0x1C},
    {"dmultu", dmultu_op, 0x1D},
    {"ddiv", ddiv_op, 0x1E},
    {"ddivu", ddivu_op, 0x1F},
    {"add", add_op, 0x20},
    {"addu", addu_op, 0x21},
    {"sub", sub_op, 0x22},
    {"subu", subu_op, 0x23},
    {"and", and_op, 0x24},
    {"or", or_op, 0x25},
    {"xor", xor_op, 0x26},
    {"nor", nor_op, 0x27},
    {"unknownSpec", unknownSpecOp, 0x28},
    {"unknownSpec", unknownSpecOp, 0x29},
    {"slt", slt_op, 0x2A},
    {"sltu", sltu_op, 0x2B},
    {"dadd", dadd_op, 0x2C},
    {"daddu", daddu_op, 0x2D},
    {"dsub", dsub_op, 0x2E},
    {"dsubu", dsubu_op, 0x2F},
    {"tge", tge_op, 0x30},
    {"tgeu", tgeu_op, 0x31},
    {"tlt", tlt_op, 0x32},
    {"tltu", tltu_op, 0x33},
    {"teq", teq_op, 0x34},
    {"unknownSpec", unknownSpecOp, 0x35},
    {"tne", tne_op, 0x36},
    {"unknownSpec", unknownSpecOp, 0x37},
    {"dsll", dsll_op, 0x38},
    {"unknownSpec", unknownSpecOp, 0x39},
    {"dsrl", dsrl_op, 0x3A},
    {"dsra", dsra_op, 0x3B},
    {"dsll32", dsll32_op, 0x3C},
    {"unknownSpec", unknownSpecOp, 0x3D},
    {"dsrl32", dsrl32_op, 0x3E},
    {"dsra32", dsra32_op, 0x3F}
};

/* Based on the rt field of bcond opcodes */
static struct mips64_op_desc mips_bcond_opcodes[] = {
    {"bltz", bltz_op, 0x00},
    {"bgez", bgez_op, 0x01},
    {"bltzl", bltzl_op, 0x02},
    {"bgezl", bgezl_op, 0x03},
    {"spimi", unknownBcondOp, 0x04},
    {"unknownBcond", unknownBcondOp, 0x05},
    {"unknownBcond", unknownBcondOp, 0x06},
    {"unknownBcond", unknownBcondOp, 0x07},
    {"tgei", tgei_op, 0x08},
    {"tgeiu", tgeiu_op, 0x09},
    {"tlti", tlti_op, 0x0A},
    {"tltiu", tltiu_op, 0x0B},
    {"teqi", teqi_op, 0x0C},
    {"unknownBcond", unknownBcondOp, 0x0D},
    {"tnei", tnei_op, 0x0E},
    {"unknownBcond", unknownBcondOp, 0x0F},
    {"bltzal", bltzal_op, 0x10},
    {"bgezal", bgezal_op, 0x11},
    {"bltzall", bltzall_op, 0x12},
    {"bgezall", bgezall_op, 0x13},
    {"unknownBcond", unknownBcondOp, 0x14},
    {"unknownBcond", unknownBcondOp, 0x15},
    {"unknownBcond", unknownBcondOp, 0x16},
    {"unknownBcond", unknownBcondOp, 0x17},
    {"unknownBcond", unknownBcondOp, 0x18},
    {"unknownBcond", unknownBcondOp, 0x19},
    {"unknownBcond", unknownBcondOp, 0x1A},
    {"unknownBcond", unknownBcondOp, 0x1B},
    {"unknownBcond", unknownBcondOp, 0x1C},
    {"unknownBcond", unknownBcondOp, 0x1D},
    {"unknownBcond", unknownBcondOp, 0x1E},
    {"unknownBcond", unknownBcondOp, 0x1F}
};

static struct mips64_op_desc mips_cop0_opcodes[] = {
    {"mfc0", mfc0_op, 0x0},
    {"dmfc0", dmfc0_op, 0x1},
    {"cfc0", cfc0_op, 0x2},
    {"unknowncop0", unknowncop0_op, 0x3},
    {"mtc0", mtc0_op, 0x4},
    {"dmtc0", dmtc0_op, 0x5},
    {"unknowncop0", unknowncop0_op, 0x6},
    {"unknowncop0", unknowncop0_op, 0x7},
    {"unknowncop0", unknowncop0_op, 0x8},
    {"unknowncop0", unknowncop0_op, 0x9},
    {"unknowncop0", unknowncop0_op, 0xa},
    {"unknowncop0", mfmc0_op, 0xb},
    {"unknowncop0", unknowncop0_op, 0xc},
    {"unknowncop0", unknowncop0_op, 0xd},
    {"unknowncop0", unknowncop0_op, 0xe},
    {"unknowncop0", unknowncop0_op, 0xf},
    {"tlb", tlb_op, 0x10},
    {"unknowncop0", unknowncop0_op, 0x11},
    {"unknowncop0", unknowncop0_op, 0x12},
    {"unknowncop0", unknowncop0_op, 0x13},
    {"unknowncop0", unknowncop0_op, 0x14},
    {"unknowncop0", unknowncop0_op, 0x15},
    {"unknowncop0", unknowncop0_op, 0x16},
    {"unknowncop0", unknowncop0_op, 0x17},
    {"unknowncop0", unknowncop0_op, 0x18},
    {"unknowncop0", unknowncop0_op, 0x19},
    {"unknowncop0", unknowncop0_op, 0x1a},
    {"unknowncop0", unknowncop0_op, 0x1b},
    {"unknowncop0", unknowncop0_op, 0x1c},
    {"unknowncop0", unknowncop0_op, 0x1d},
    {"unknowncop0", unknowncop0_op, 0x1e},
    {"unknowncop0", unknowncop0_op, 0x1f},
};

static struct mips64_op_desc mips_mad_opcodes[] = {
    {"mad", madd_op, 0x0},
    {"maddu", maddu_op, 0x1},
    {"mul", mul_op, 0x2},
    {"unknownmad_op", unknownmad_op, 0x3},
    {"msub", msub_op, 0x4},
    {"msubu", msubu_op, 0x5},
    {"unknownmad_op", unknownmad_op, 0x6},
    {"unknownmad_op", unknownmad_op, 0x7},
    {"unknownmad_op", unknownmad_op, 0x8},
    {"unknownmad_op", unknownmad_op, 0x9},
    {"unknownmad_op", unknownmad_op, 0xa},
    {"unknownmad_op", unknownmad_op, 0xb},
    {"unknownmad_op", unknownmad_op, 0xc},
    {"unknownmad_op", unknownmad_op, 0xd},
    {"unknownmad_op", unknownmad_op, 0xe},
    {"unknownmad_op", unknownmad_op, 0xf},
    {"unknownmad_op", unknownmad_op, 0x10},
    {"unknownmad_op", unknownmad_op, 0x11},
    {"unknownmad_op", unknownmad_op, 0x12},
    {"unknownmad_op", unknownmad_op, 0x13},
    {"unknownmad_op", unknownmad_op, 0x14},
    {"unknownmad_op", unknownmad_op, 0x15},
    {"unknownmad_op", unknownmad_op, 0x16},
    {"unknownmad_op", unknownmad_op, 0x17},
    {"unknownmad_op", unknownmad_op, 0x18},
    {"unknownmad_op", unknownmad_op, 0x19},
    {"unknownmad_op", unknownmad_op, 0x1a},
    {"unknownmad_op", unknownmad_op, 0x1b},
    {"unknownmad_op", unknownmad_op, 0x1c},
    {"unknownmad_op", unknownmad_op, 0x1d},
    {"unknownmad_op", unknownmad_op, 0x1e},
    {"unknownmad_op", unknownmad_op, 0x1f},
    {"clz", clz_op, 0x20},
    {"unknownmad_op", unknownmad_op, 0x21},
    {"unknownmad_op", unknownmad_op, 0x22},
    {"unknownmad_op", unknownmad_op, 0x23},
    {"unknownmad_op", unknownmad_op, 0x24},
    {"unknownmad_op", unknownmad_op, 0x25},
    {"unknownmad_op", unknownmad_op, 0x26},
    {"unknownmad_op", unknownmad_op, 0x27},
    {"unknownmad_op", unknownmad_op, 0x28},
    {"unknownmad_op", unknownmad_op, 0x29},
    {"unknownmad_op", unknownmad_op, 0x2a},
    {"unknownmad_op", unknownmad_op, 0x2b},
    {"unknownmad_op", unknownmad_op, 0x2c},
    {"unknownmad_op", unknownmad_op, 0x2d},
    {"unknownmad_op", unknownmad_op, 0x2e},
    {"unknownmad_op", unknownmad_op, 0x2f},
    {"unknownmad_op", unknownmad_op, 0x30},
    {"unknownmad_op", unknownmad_op, 0x31},
    {"unknownmad_op", unknownmad_op, 0x32},
    {"unknownmad_op", unknownmad_op, 0x33},
    {"unknownmad_op", unknownmad_op, 0x34},
    {"unknownmad_op", unknownmad_op, 0x35},
    {"unknownmad_op", unknownmad_op, 0x36},
    {"unknownmad_op", unknownmad_op, 0x37},
    {"unknownmad_op", unknownmad_op, 0x38},
    {"unknownmad_op", unknownmad_op, 0x39},
    {"unknownmad_op", unknownmad_op, 0x3a},
    {"unknownmad_op", unknownmad_op, 0x3b},
    {"unknownmad_op", unknownmad_op, 0x3c},
    {"unknownmad_op", unknownmad_op, 0x3d},
    {"unknownmad_op", unknownmad_op, 0x3e},
    {"unknownmad_op", unknownmad_op, 0x3f},

};

static struct mips64_op_desc mips_tlb_opcodes[] = {
    {"unknowntlb_op", unknowntlb_op, 0x0},
    {"tlbr", tlbr_op, 0x1},
    {"tlbwi", tlbwi_op, 0x2},
    {"unknowntlb_op", unknowntlb_op, 0x3},
    {"unknowntlb_op", unknowntlb_op, 0x4},
    {"unknowntlb_op", unknowntlb_op, 0x5},
    {"tlbwi", tlbwr_op, 0x6},
    {"unknowntlb_op", unknowntlb_op, 0x7},
    {"tlbp", tlbp_op, 0x8},
    {"unknowntlb_op", unknowntlb_op, 0x9},
    {"unknowntlb_op", unknowntlb_op, 0xa},
    {"unknowntlb_op", unknowntlb_op, 0xb},
    {"unknowntlb_op", unknowntlb_op, 0xc},
    {"unknowntlb_op", unknowntlb_op, 0xd},
    {"unknowntlb_op", unknowntlb_op, 0xe},
    {"unknowntlb_op", unknowntlb_op, 0xf},
    {"unknowntlb_op", unknowntlb_op, 0x10},
    {"unknowntlb_op", unknowntlb_op, 0x11},
    {"unknowntlb_op", unknowntlb_op, 0x12},
    {"unknowntlb_op", unknowntlb_op, 0x13},
    {"unknowntlb_op", unknowntlb_op, 0x14},
    {"unknowntlb_op", unknowntlb_op, 0x15},
    {"unknowntlb_op", unknowntlb_op, 0x16},
    {"unknowntlb_op", unknowntlb_op, 0x17},
    {"eret", eret_op, 0x18},
    {"unknowntlb_op", unknowntlb_op, 0x19},
    {"unknowntlb_op", unknowntlb_op, 0x1a},
    {"unknowntlb_op", unknowntlb_op, 0x1b},
    {"unknowntlb_op", unknowntlb_op, 0x1c},
    {"unknowntlb_op", unknowntlb_op, 0x1d},
    {"unknowntlb_op", unknowntlb_op, 0x1e},
    {"unknowntlb_op", unknowntlb_op, 0x1f},
    {"wait", wait_op, 0x20},
    {"unknowntlb_op", unknowntlb_op, 0x21},
    {"unknowntlb_op", unknowntlb_op, 0x22},
    {"unknowntlb_op", unknowntlb_op, 0x23},
    {"unknowntlb_op", unknowntlb_op, 0x24},
    {"unknowntlb_op", unknowntlb_op, 0x25},
    {"unknowntlb_op", unknowntlb_op, 0x26},
    {"unknowntlb_op", unknowntlb_op, 0x27},
    {"unknowntlb_op", unknowntlb_op, 0x28},
    {"unknowntlb_op", unknowntlb_op, 0x29},
    {"unknowntlb_op", unknowntlb_op, 0x2a},
    {"unknowntlb_op", unknowntlb_op, 0x2b},
    {"unknowntlb_op", unknowntlb_op, 0x2c},
    {"unknowntlb_op", unknowntlb_op, 0x2d},
    {"unknowntlb_op", unknowntlb_op, 0x2e},
    {"unknowntlb_op", unknowntlb_op, 0x2f},
    {"unknowntlb_op", unknowntlb_op, 0x30},
    {"unknowntlb_op", unknowntlb_op, 0x31},
    {"unknowntlb_op", unknowntlb_op, 0x32},
    {"unknowntlb_op", unknowntlb_op, 0x33},
    {"unknowntlb_op", unknowntlb_op, 0x34},
    {"unknowntlb_op", unknowntlb_op, 0x35},
    {"unknowntlb_op", unknowntlb_op, 0x36},
    {"unknowntlb_op", unknowntlb_op, 0x37},
    {"unknowntlb_op", unknowntlb_op, 0x38},
    {"unknowntlb_op", unknowntlb_op, 0x39},
    {"unknowntlb_op", unknowntlb_op, 0x3a},
    {"unknowntlb_op", unknowntlb_op, 0x3b},
    {"unknowntlb_op", unknowntlb_op, 0x3c},
    {"unknowntlb_op", unknowntlb_op, 0x3d},
    {"unknowntlb_op", unknowntlb_op, 0x3e},
    {"unknowntlb_op", unknowntlb_op, 0x3f},
};
