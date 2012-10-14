#include <stdio.h>
#include "defs.h"
#include "data.h"

/*
 * Some predefinitions:
 *
 * INTSIZE is the size of an integer in the target machine
 * BYTEOFF is the offset of an byte within an integer on the
 *         target machine. (ie: 8080,pdp11 = 0, 6809 = 1,
 *         360 = 3)
 * This compiler assumes that an integer is the SAME length as
 * a pointer - in fact, the compiler uses INTSIZE for both.
 */
#define INTSIZE 4
#define BYTEOFF 0

/*
 * Print all assembler info before any code is generated.
 */
header()
{
    outstr ("#\tSmall C MIPS32\n#\tCoder 1.0, 2012/06/18\n#");
    FEvers();
    nl();
    //ol ("global\tTlneg");
    //ol ("global\tTcase");
    //ol ("global\tTeq");
    //ol ("global\tTne");
    //ol ("global\tTlt");
    //ol ("global\tTle");
    //ol ("global\tTgt");
    //ol ("global\tTge");
    //ol ("global\tTult");
    //ol ("global\tTule");
    //ol ("global\tTugt");
    //ol ("global\tTuge");
    //ol ("global\tTbool");
    //ol ("global\tTmult");
    //ol ("global\tTdiv");
    //ol ("global\tTmod");
}

nl()
{
    outbyte (EOL);
}

galign(t)
    int t;
{
    int sign;
    if (t < 0) {
        sign = 1;
        t = -t;
    } else
        sign = 0;
    t = (t + INTSIZE - 1) & ~(INTSIZE - 1);
    t = sign? -t: t;
    return (t);
}

/*
 * Return size of an integer.
 */
intsize()
{
    return INTSIZE;
}

/*
 * Return offset of ls byte within word.
 * (ie: 8080 & pdp11 is 0, 6809 is 1, 360 is 3)
 */
byteoff()
{
    return BYTEOFF;
}

/*
 * Output internal generated label prefix.
 */
olprfix()
{
    outstr ("._");
}

/*
 * Output a label definition terminator.
 */
col()
{
    outstr (":");
}

/*
 * Begin a comment line for the assembler.
 */
comment()
{
    outbyte ('#');
}

/*
 * Output a prefix in front of user labels.
 */
prefix()
{
    //outbyte ('_');
}

/*
 * Print any assembler stuff needed after all code.
 */
trailer()
{
}

/*
 * Function prologue.
 */
prologue()
{
    /* todo */
}

fentry()
{
    ol("addiu\t$sp, $sp, -4");
    ol("sw\t$ra, 0($sp)");
}

/*
 * Text (code) segment.
 */
gtext()
{
    ol (".text");
}

/*
 * Data segment.
 */
gdata()
{
    ol (".data");
}

/*
 * Output the variable symbol at scptr as an extrn or a public.
 */
ppubext (scptr)
    char *scptr;
{
    if (scptr[STORAGE] == STATIC)
        return;
    ot (".globl\t");
    //prefix();
    outstr (scptr);
    nl();
}

/*
 * Output the function symbol at scptr as an extrn or a public
 */
fpubext (scptr)
    char *scptr;
{
    ppubext (scptr);
}

/*
 *  Output a decimal number to the assembler file.
 */
onum (num)
    int num;
{
    outdec (num);
}

/*
 * Fetch a static memory cell into the primary register.
 */
getmem (sym)
    char    *sym;
{
    if ((sym[IDENT] != POINTER) & (sym[TYPE] == CCHAR)) {
        ot ("lb\t$v0, ");
        //prefix();
        outstr (sym + NAME);
    } else {
        ot ("lw\t$v0, ");
        //prefix();
        outstr (sym + NAME);
    }
}

/*
 * Fetch the address of the specified symbol into the primary register.
 */
getloc (sym)
    char    *sym;
{
    if (sym[STORAGE] == LSTATIC) {
        ot ("la $v0, ");
        printlabel(glint(sym));
        nl();
    } else {
        //ot ("la $v0, ");
	ot("addiu\t$v0, $sp, ");
        onum (glint(sym) - stkp);
        //outstr ("($sp)\n");
	nl();
    }
}

/*
 * Store the primary register into the specified static memory cell.
 */
putmem (sym)
    char *sym;
{
    if ((sym[IDENT] != POINTER) & (sym[TYPE] == CCHAR)) {
        ot ("sb\t$v0, ");
    } else
        ot ("sw\t$v0, ");

    //prefix();
    outstr (sym + NAME);
    nl();
}

/*
 * Store the specified object type in the primary register
 * at the address on the top of the stack.
 */
putstk (typeobj)
    char typeobj;
{
    ol ("lw\t$t1, 0($sp)");
    ol ("addiu\t$sp, $sp, 4");
    if (typeobj == CCHAR)
        ol ("sb\t$v0, 0($t1)");
    else
        ol ("sw\t$v0, 0($t1)");
    stkp = stkp + INTSIZE;
}

/*
 * Fetch the specified object type indirect through the primary
 * register into the primary register.
 */
indirect (typeobj)
    char typeobj;
{
    if (typeobj == CCHAR)
        ol ("lb\t$v0, 0($v0)");
    else
        ol ("lw\t$v0, 0($v0)");
}

/*
 * Swap the primary and secondary registers.
 */
swap()
{
    ol ("move\t$at, $v0\n\tmove\t$v0, $v1\n\tmove\t$v1, $at");
}

/*
 * Print partial instruction to get an immediate value into
 * the primary register.
 */
immed()
{
    ot ("la\t$v0, ");
}

immedi()
{
    ot ("li\t$v0, ");
}

/*
 * Push the primary register onto the stack.
 */
gpush()
{
    ol ("addiu\t$sp, $sp, -4");
    ol ("sw\t$v0, 0($sp)");
    stkp = stkp - INTSIZE;
}

/*
 * Pop the top of the stack into the secondary register.
 */
gpop()
{
    ol ("lw\t$v1, 0($sp)");
    ol ("addiu\t$sp, $sp, 4");
    stkp = stkp + INTSIZE;
}

/*
 * Swap the primary register and the top of the stack.
 */
swapstk()
{
    ol ("move\t$t1, $v0");
    ol ("lw\t$v0, 0($sp)");
    ol ("sw\t$t1, 0($sp)");
    //ol ("mov.l\t(%sp)+,%d2\nmov.l\t%d0,-(%sp)\nmov.l\t%d2,%d0");
}

/*
 * Call the specified subroutine name.
 */
gcall (sname)
    char *sname;
{
    //ol("addiu\t$sp, $sp, -4");
    //ol("sw\t$ra, 0($sp)");
    if (*sname == '^') {
        ot ("jal\tT");
        outstr (++sname);
	nl();
	ot ("nop" ); /* fill delay slot */
    } else {
        ot ("jal\t");
        //prefix();
        outstr (sname);
	nl();
	ot ("nop"); /* fill delay slot */
    }
    nl();
    //ol("lw\t$ra, 0($sp)");
    //ol("addiu\t$sp, $sp, 4");
}

/*
 * Return from subroutine.
 */
gret()
{
    ol("lw\t$ra, 0($sp)");
    ol("addiu\t$sp, $sp, 4");
    ol("jr\t$ra");
    ol ("nop");
}

/*
 * Perform subroutine call to value on top of stack.
 */
callstk()
{
    ////ol("addiu\t$sp, $sp, -4");
    ////ol("sw\t$ra, 0($sp)");
    ol ("jsr\t0(sp)+\nnop");
    stkp = stkp + INTSIZE;
    ////ol("lw\t$ra, 0($sp)");
    ////ol("addiu\t$sp, $sp, 4");
}

/*
 * Jump to specified internal label number.
 */
jump (label)
    int label;
{
    ot ("j\t");
    printlabel (label);
    nl();
    ol ("nop");
}

/*
 * Test the primary register and jump if false to label.
 */
testjump (label, ft)
    int label;
    int ft;
{
    //ol ("cmp.l\t%d0,&0");
    if (ft)
        //ot ("beq\t");
        //ot("blez\t$v0, ");
        ot("bne\t$v0, $zero, ");
    else
        //ot ("bne\t");
        //ot("bgtz\t$v0, ");
        ot("beq\t$v0, $zero, ");
    printlabel (label);
    nl();
    ol("nop"); // fill delay slot
}

/*
 * Print pseudo-op to define a byte.
 */
defbyte()
{
    ot (".byte\t");
}

/*
 * Print pseudo-op to define storage.
 */
defstorage()
{
    ot (".space\t");
}

/*
 * Print pseudo-op to define a word.
 */
defword()
{
    ot (".word\t");
}

/*
 * Modify the stack pointer to the new value indicated.
 */
modstk (newstkp)
    int newstkp;
{
    int k;

    k = newstkp - stkp;
    if (k % INTSIZE)
        error("Bad stack alignment (compiler error)");
    if (k == 0)
        return (newstkp);
    ot ("addiu\t");
    outstr ("$sp, $sp, ");
    onum (k);
    nl();
    return (newstkp);
}

/*
 * Multiply the primary register by INTSIZE.
 */
gaslint()
{
    ol ("sll\t$v0, $v0, 2");
}

/*
 * Divide the primary register by INTSIZE.
 */
gasrint()
{
    ol ("sra\t$v0, $v0, 2");
}

/*
 * Case jump instruction.
 */
gjcase()
{
    gcall ("^case");
}

/*
 * Add the primary and secondary registers.
 * If lval2 is int pointer and lval is int, scale lval.
 */
gadd (lval, lval2)
    int *lval, *lval2;
{
    ol ("lw\t$t1, 0($sp)");
    ol ("addiu\t$sp, $sp, 4");
    if (dbltest (lval2, lval)) {
        //ol ("asl.l\t&2,(%sp)");
        ol("sll\t$t1, $t1, 2");
    }
    //ol ("add.l\t(%sp)+,%d0");
    ol ("add\t$v0, $v0, $t1");
    stkp = stkp + INTSIZE;
}

/*
 * Subtract the primary register from the secondary. // *** from TOS
 */
gsub()
{
    ol ("lw\t$t1, 0($sp)");
    ol ("addiu\t$sp, $sp, 4");
    ol ("sub\t$v0, $t1, $v0");
    stkp = stkp + INTSIZE;
}

/*
 * Multiply the primary and secondary registers.
 * (result in primary)
 */
gmult()
{
    ol ("lw\t$t1, 0($sp)");
    ol ("addiu\t$sp, $sp, 4");
    ol ("mult\t$v0, $t1");
    ol ("mflo\t$v0");
    //gcall ("^mult");
    stkp = stkp + INTSIZE;
}

/*
 * Divide the secondary register by the primary.
 * (quotient in primary, remainder in secondary)
 */
gdiv()
{
    ol ("lw\t$t1, 0($sp)");
    ol ("addiu\t$sp, $sp, 4");
    ol ("div\t$t1, $v0");
    ol ("mflo\t$v0");
    ol ("mfhi\t$t1");
    //gcall ("^div");
    stkp = stkp + INTSIZE;
}

/*
 * Compute the remainder (mod) of the secondary register
 * divided by the primary register.
 * (remainder in primary, quotient in secondary)
 */
gmod()
{
    ol ("lw\t$t1, 0($sp)");
    ol ("addiu\t$sp, $sp, 4");
    ol ("div\t$t1, $v0");
    ol ("mflo\t$t1");
    ol ("mfhi\t$v0");
    //gcall ("^mod");
    stkp = stkp + INTSIZE;
}

/*
 * Inclusive 'or' the primary and secondary registers.
 */
gor()
{
    ol ("lw\t$t1, 0($sp)");
    ol ("addiu\t$sp, $sp, 4");
    ol ("or\t$v0, $v0, $t1");
    //ol ("or.l\t(%sp)+,%d0");
    stkp = stkp + INTSIZE;
}

/*
 * Exclusive 'or' the primary and secondary registers.
 */
gxor()
{
    ol ("lw\t$t1, 0($sp)");
    ol ("addiu\t$sp, $sp, 4");
    ol ("xor\t$v0, $v0, $t1");
    //ol ("mov.l\t(%sp)+,%d1");
    //ol ("eor.l\t%d1,%d0");
    stkp = stkp + INTSIZE;
}

/*
 * 'And' the primary and secondary registers.
 */
gand()
{
    //ol ("and.l\t(%sp)+,%d0");
    ol ("lw\t$t1, 0($sp)");
    ol ("addiu\t$sp, $sp, 4");
    ol ("and\t$v0, $v0, $t1");
    stkp = stkp + INTSIZE;
}

/*
 * Arithmetic shift right the secondary register the number of
 * times in the primary register.
 * (results in primary register)
 */
gasr()
{
    ol ("lw\t$t1, 0($sp)");
    ol ("addiu\t$sp, $sp, 4");
    ol ("srav\t$v0, $t1, $v0");
    stkp = stkp + INTSIZE;
}

/*
 * Arithmetic shift left the secondary register the number of
 * times in the primary register.
 * (results in primary register)
 */
gasl()
{
    ol ("lw\t$t1, 0($sp)");
    ol ("addiu\t$sp, $sp, 4");
    ol ("sllv\t$v0, $t1, $v0");
    stkp = stkp + INTSIZE;
}

/*
 * Two's complement of primary register.
 */
gneg()
{
    ol ("sub\t$v0, $zero, $v0");
}

/*
 * Logical complement of primary register.
 */
glneg()
{
    //gcall ("^lneg");
    ol ("sltu\t$t1, $v0, $zero");
    ol ("sltu\t$t2, $zero, $v0");
    ol ("or\t$v0, $t1, $t2");
    ol ("xori\t$v0, $v0, 1");
}

/*
 * One's complement of primary register.
 */
gcom()
{
    ol ("addiu\t$t1, $zero, -1");
    ol ("xor\t$v0, $v0, $t1");
}

/*
 * Convert primary register into logical value.
 */
gbool()
{
    ol ("sltu\t$t1, $v0, $zero");
    ol ("sltu\t$t2, $zero, $v0");
    ol ("or\t$v0, $t1, $t2");
    //gcall ("^bool");
}

/*
 * Increment the primary register by 1 if char, INTSIZE if int.
 */
ginc (lval)
    int lval[];
{
    if (lval[2] == CINT)
        //ol ("addq.l\t&4,%d0");
	ol("addiu\t$v0, $v0, 4");
    else
        //ol ("addq.l\t&1,%d0");
	ol("addiu\t$v0, $v0, 1");
}

/*
 * Decrement the primary register by one if char, INTSIZE if int.
 */
gdec (lval)
    int lval[];
{
    if (lval[2] == CINT)
        //ol ("subq.l\t&4,%d0");
	ol("addiu\t$v0, $v0, -4");
    else
        //ol ("subq.l\t&1,%d0");
	ol("addiu\t$v0, $v0, -1");
}

/*
 * Following are the conditional operators.
 * They compare the secondary register against the primary register
 * and put a literl 1 in the primary if the condition is true,
 * otherwise they clear the primary register.
 */
///// BEEP BEEP actually, compare tos
/*
 * equal
 */
geq()
{
    ol("lw\t$t1, 0($sp)");
    ol("sltu\t$t2, $v0, $t1");
    ol("sltu\t$v0, $t1, $v0");
    ol("or\t$v0, $v0, $t2");
    ol("xori\t$v0, $v0, 1");
    ol("addiu\t$sp, $sp, 4");
    //gcall ("^eq");
    stkp = stkp + INTSIZE;
}

/*
 * not equal
 */
gne()
{
    ol("lw\t$t1, 0($sp)");
    ol("sltu\t$t2, $v0, $t1");
    ol("sltu\t$v0, $t1, $v0");
    ol("or\t$v0, $v0, $t2");
    ol("addiu\t$sp, $sp, 4");
    //gcall ("^ne");
    stkp = stkp + INTSIZE;
}

/*
 * less than (signed) - TOS < primary
 */
glt()
{
    ol("lw\t$t1, 0($sp)");
    ol("addiu\t$sp, $sp, 4");
    ol("slt\t$v0, $t1, $v0");
    //gcall ("^lt");
    stkp = stkp + INTSIZE;
}

/*
 * less than or equal (signed) TOS <= primary
 */
gle()
{
    ol("lw\t$t1, 0($sp)");
    ol("addiu\t$sp, $sp, 4");
    ol("slt\t$v0, $v0, $t1"); // primary < tos
    ol("xori\t$v0, $v0, 1");  // primary >= tos
    //gcall ("^le");
    stkp = stkp + INTSIZE;
}

/*
 * greater than (signed) TOS > primary
 */
ggt()
{
    ol("lw\t$t1, 0($sp)");
    ol("addiu\t$sp, $sp, 4");
    ol("slt\t$v0, $v0, $t1");   //pimary < TOS
    //ol("xori\t$v0, $v0, 1");
    //gcall ("^gt");
    stkp = stkp + INTSIZE;
}

/*
 * greater than or equal (signed) TOS >= primary
 */
gge()
{
    ol("lw\t$t1, 0($sp)");
    ol("addiu\t$sp, $sp, 4");
    ol("slt\t$v0, $t1, $v0");   //tos < primary
    ol("xori\t$v0, $v0, 1");    //tos >= primary
    //gcall ("^ge");
    stkp = stkp + INTSIZE;
}

/*
 * less than (unsigned)
 */
gult()
{
    ol("lw\t$t1, 0($sp)");
    ol("addiu\t$sp, $sp, 4");
    ol("sltu\t$v0, $t1, $v0");
    //gcall ("^ult");
    stkp = stkp + INTSIZE;
}

/*
 * less than or equal (unsigned)
 */
gule()
{
    ol("lw\t$t1, 0($sp)");
    ol("addiu\t$sp, $sp, 4");
    ol("sltu\t$v0, $v0, $t1"); // primary < tos
    ol("xori\t$v0, $v0, 1");  // primary >= tos
    //gcall ("^ule");
    stkp = stkp + INTSIZE;
}

/*
 * greater than (unsigned)
 */
gugt()
{
    ol("lw\t$t1, 0($sp)");
    ol("addiu\t$sp, $sp, 4");
    ol("sltu\t$v0, $v0, $t1");   //pimary < TOS
    //gcall ("^ugt");
    stkp = stkp + INTSIZE;
}

/*
 * greater than or equal (unsigned)
 */
guge()
{
    ol("lw\t$t1, 0($sp)");
    ol("addiu\t$sp, $sp, 4");
    ol("sltu\t$v0, $t1, $v0");   //tos < primary
    ol("xori\t$v0, $v0, 1");    //tos >= primary
    //gcall ("^uge");
    stkp = stkp + INTSIZE;
}

/*
 * Squirrel away argument count in a register that modstk/getloc/stloc
 * doesn't touch.
 */
gnargs (d)
    int d;
{
    /* Empty for now. */
}
