main()
{
    __asm__
        syscall 97
        nop
        nop
    __endasm__

    printf ("Hello, SmallC World!\n");
}
