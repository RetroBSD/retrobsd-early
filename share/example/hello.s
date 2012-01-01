#
# hello.s   by Serge Vakulenko
#
# This is an example of MIPS assembly program for RetroBSD.
# To compile this program type:
#   as hello.s -o hello.o
#   ld hello.o -o hello
#
        .data                               # begin data segment
hello:  .ascii  "Hello, assembly world!\n"  # a null terminated string

        .text                               # begin code segment
        .globl  start                       # for ld linking
start:
        li      $a0, 0                      # load stdout fd
        la      $a1, hello                  # load string address
        li      $a2, 23                     # load string length
        syscall 4                           # call the kernel: write()
        nop                                 # returned here on error
        nop

        syscall 1                           # call the kernel: exit()
                                            # no return
# That`s all folks!
