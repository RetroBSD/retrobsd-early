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
    outstr ("$_");
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
        ot ("la $v0, ");
        onum (glint(sym) - stkp);
        outstr ("($sp)\n");
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
    ol ("lw\t$at, ($sp)");
    ol ("addiu\t$sp, 4");
    if (typeobj == CCHAR)
        ol ("sb\t$v0, ($at)");
    else
        ol ("sw\t$v0, ($at)");
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
        ol ("lb\t$v0, ($v0)");
    else
        ol ("lw\t$v0, ($v0)");
}

/*
 * Swap the primary and secondary registers.
 */
swap()
{
    ol ("move\t$at, %v0\n\tmove\t$v0, $v1\n\tmove\t$v1, $at");
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
    ol ("addiu\t$sp, -4");
    ol ("sw\t$v0, ($sp)");
    stkp = stkp - INTSIZE;
}

/*
 * Pop the top of the stack into the secondary register.
 */
gpop()
{
    ol ("lw\t$v1, ($sp)");
    ol ("addiu\t$sp, 4");
    stkp = stkp + INTSIZE;
}

/*
 * Swap the primary register and the top of the stack.
 */
swapstk()
{
    ol ("mov.l\t(%sp)+,%d2\nmov.l\t%d0,-(%sp)\nmov.l\t%d2,%d0");
}

/*
 * Call the specified subroutine name.
 */
gcall (sname)
    char *sname;
{
    if (*sname == '^') {
        ot ("jsr\tT");
        outstr (++sname);
    } else {
        ot ("jsr\t");
        //prefix();
        outstr (sname);
    }
    nl();
}

/*
 * Return from subroutine.
 */
gret()
{
    ol ("jr\t$ra\nnop");
}

/*
 * Perform subroutine call to value on top of stack.
 */
callstk()
{
    ol ("jsr\t(%sp)+");
    stkp = stkp + INTSIZE;
}

/*
 * Jump to specified internal label number.
 */
jump (label)
    int label;
{
    ot ("j\t");
    printlabel (label);
    ol ("\nnop");
}

/*
 * Test the primary register and jump if false to label.
 */
testjump (label, ft)
    int label;
    int ft;
{
    ol ("cmp.l\t%d0,&0");
    if (ft)
        ot ("beq\t");
    else
        ot ("bne\t");
    printlabel (label);
    nl();
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
    ot ("add.l\t&");
    onum (k);
    outstr (",sp");
    nl();
    return (newstkp);
}

/*
 * Multiply the primary register by INTSIZE.
 */
gaslint()
{
    ol ("asl.l\t&2,%d0");
}

/*
 * Divide the primary register by INTSIZE.
 */
gasrint()
{
    ol ("asr.l\t&2,%d0");
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
    if (dbltest (lval2, lval)) {
        ol ("asl.l\t&2,(%sp)");
    }
    ol ("add.l\t(%sp)+,%d0");
    stkp = stkp + INTSIZE;
}

/*
 * Subtract the primary register from the secondary.
 */
gsub()
{
    ol ("mov.l\t(%sp)+,%d2");
    ol ("sub.l\t%d0,%d2");
    ol ("mov.l\t%d2,%d0");
    stkp = stkp + INTSIZE;
}

/*
 * Multiply the primary and secondary registers.
 * (result in primary)
 */
gmult()
{
    gcall ("^mult");
    stkp = stkp + INTSIZE;
}

/*
 * Divide the secondary register by the primary.
 * (quotient in primary, remainder in secondary)
 */
gdiv()
{
    gcall ("^div");
    stkp = stkp + INTSIZE;
}

/*
 * Compute the remainder (mod) of the secondary register
 * divided by the primary register.
 * (remainder in primary, quotient in secondary)
 */
gmod()
{
    gcall ("^mod");
    stkp = stkp + INTSIZE;
}

/*
 * Inclusive 'or' the primary and secondary registers.
 */
gor()
{
    ol ("or.l\t(%sp)+,%d0");
    stkp = stkp + INTSIZE;
}

/*
 * Exclusive 'or' the primary and secondary registers.
 */
gxor()
{
    ol ("mov.l\t(%sp)+,%d1");
    ol ("eor.l\t%d1,%d0");
    stkp = stkp + INTSIZE;
}

/*
 * 'And' the primary and secondary registers.
 */
gand()
{
    ol ("and.l\t(%sp)+,%d0");
    stkp = stkp + INTSIZE;
}

/*
 * Arithmetic shift right the secondary register the number of
 * times in the primary register.
 * (results in primary register)
 */
gasr()
{
    ol ("mov.l\t(%sp)+,%d1");
    ol ("asr.l\t%d0,%d1");
    ol ("mov.l\t%d1,%d0");
    stkp = stkp + INTSIZE;
}

/*
 * Arithmetic shift left the secondary register the number of
 * times in the primary register.
 * (results in primary register)
 */
gasl()
{
    ol ("mov.l\t(%sp)+,%d1");
    ol ("asl.l\t%d0,%d1");
    ol ("mov.l\t%d1,%d0");
    stkp = stkp + INTSIZE;
}

/*
 * Two's complement of primary register.
 */
gneg()
{
    ol ("neg.l\t%d0");
}

/*
 * Logical complement of primary register.
 */
glneg()
{
    gcall ("^lneg");
}

/*
 * One's complement of primary register.
 */
gcom()
{
    ol ("not\t%d0");
}

/*
 * Convert primary register into logical value.
 */
gbool()
{
    gcall ("^bool");
}

/*
 * Increment the primary register by 1 if char, INTSIZE if int.
 */
ginc (lval)
    int lval[];
{
    if (lval[2] == CINT)
        ol ("addq.l\t&4,%d0");
    else
        ol ("addq.l\t&1,%d0");
}

/*
 * Decrement the primary register by one if char, INTSIZE if int.
 */
gdec (lval)
    int lval[];
{
    if (lval[2] == CINT)
        ol ("subq.l\t&4,%d0");
    else
        ol ("subq.l\t&1,%d0");
}

/*
 * Following are the conditional operators.
 * They compare the secondary register against the primary register
 * and put a literl 1 in the primary if the condition is true,
 * otherwise they clear the primary register.
 */

/*
 * equal
 */
geq()
{
    gcall ("^eq");
    stkp = stkp + INTSIZE;
}

/*
 * not equal
 */
gne()
{
    gcall ("^ne");
    stkp = stkp + INTSIZE;
}

/*
 * less than (signed)
 */
glt()
{
    gcall ("^lt");
    stkp = stkp + INTSIZE;
}

/*
 * less than or equal (signed)
 */
gle()
{
    gcall ("^le");
    stkp = stkp + INTSIZE;
}

/*
 * greater than (signed)
 */
ggt()
{
    gcall ("^gt");
    stkp = stkp + INTSIZE;
}

/*
 * greater than or equal (signed)
 */
gge()
{
    gcall ("^ge");
    stkp = stkp + INTSIZE;
}

/*
 * less than (unsigned)
 */
gult()
{
    gcall ("^ult");
    stkp = stkp + INTSIZE;
}

/*
 * less than or equal (unsigned)
 */
gule()
{
    gcall ("^ule");
    stkp = stkp + INTSIZE;
}

/*
 * greater than (unsigned)
 */
gugt()
{
    gcall ("^ugt");
    stkp = stkp + INTSIZE;
}

/*
 * greater than or equal (unsigned)
 */
guge()
{
    gcall ("^uge");
    stkp = stkp + INTSIZE;
}

/*
 * Squirrel away argument count in a register that modstk/getloc/stloc
 * doesn't touch.
 */
gnargs (d)
    int d;
{
    ot ("mov.l\t&");
    onum (d);
    outstr (",%d3\n");
}
