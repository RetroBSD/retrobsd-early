/*
 * hello.s   by Serge Vakulenko
 *
 * This is an example of MIPS assembly program for RetroBSD.
 * To compile this program type:
 *  as hello.s -o hello.o
 *  ld hello.o -o hello
 *
 */
#include <syscall.h>

        .data                               // begin data segment
hello:  .ascii  "Hello, assembly world!\n"  // a null terminated string

        .text                               // begin code segment
        .globl  start                       // ld entry point
start:
        li      $a0, 0                      // arg 1: stdout fd
        la      $a1, hello                  // arg 2: string address
        li      $a2, 23                     // arg 3: string length
        syscall SYS_write                   // call the kernel: write()
        nop                                 // returned here on error
        nop                                 // skip two words on success

        li      $a0, 0                      // arg 1: exit status
        syscall SYS_exit                    // call the kernel: exit()
        // no return
