#include <syscall.h>

        .data                           // begin data segment
eoln:   .ascii  "\n"
space:  .ascii  " "

        .text                           // begin code segment
start:  .globl  start                   // entry point for ld

        addi    $s0, $a0, -1            // argc - 1
        beq     $s0, $zero, done        // if (argc == 0) goto gone
        addi    $s1, $a1, 4             // argv + 4
loop:
        jal     print                   // print string
        lw      $a0, 0($s1)             // arg 1: *argv

        addi    $s0, $s0, -1            // --argc
        la      $a1, eoln               // arg2: newline
        beq     $s0, $zero, last        // if (argc == 0) goto last
        li      $a0, 1                  // arg1: stdout
        la      $a1, space              // arg2: space
last:
        li      $a2, 1                  // arg3: length
        syscall SYS_write               // call the kernel: write()
        nop                             // ignore errors
        nop

        bne     $s0, $zero, loop        // if (argc == 0) goto gone
        addi    $s1, $s1, 4             // ++argv
done:
        li      $a0, 0                  // arg1: exit status
        syscall SYS_exit                // call the kernel: exit()
        // no return

print:  move    $a1, $a0                // arg2: string

	addi	$a2, $a0, 1             // compute length
strlen:
	lb	$v0, 0($a0)		// get byte from string
	bne	$v0, $zero, strlen	// continue if not end
	addi	$a0, $a0, 1		// increment pointer

	subu	$a2, $a0, $a2		// arg3: length

        li      $a0, 1                  // arg1: stdout
        syscall SYS_write               // call the kernel: write()
        nop                             // ignore errors
        nop

        jr      $ra
        nop
