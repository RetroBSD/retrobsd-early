/*
 * Total 12 registers for MIPS architecture:
 *	0  - $s0
 *	1  - $s1
 *	2  - $s2
 *	3  - $s3
 *	4  - $s4
 *	5  - $s5
 *	6  - $s6
 *	7  - $s7
 *	8  - $s8
 *	9  - $ra - return address
 *	10 - $gp - global data pointer
 *	11 - $sp - stack pointer
 */
typedef int jmp_buf [12];

int setjmp (jmp_buf env);
void longjmp (jmp_buf env, int val);
