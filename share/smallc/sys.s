	.text

write:	
	lw	$a0, 8($sp)
	lw	$a1, 4($sp)
	lw	$a2, 0($sp)

	addiu   $sp, $sp, -4
	sw	$ra, 0($sp)

	syscall	4

	jal	serrn
	nop

	lw	$ra, 0($sp)
	addiu   $sp, $sp, 4

	jr	$ra
	nop

exit:
	lw	$a0, 0($sp)
	addiu   $sp, $sp, -4
	syscall	1
	nop

serrn:
        la	$t1, errno
	sw	$t0, 0($t1)
	jr	$ra
	nop

	syscall	4

	jal	serrn
	nop

	.data
errno:	.byte	0,0,0,0

	.globl write
	.globl exit
        .globl errno

