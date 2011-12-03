.text
#.set noreorder

start:
	addiu	$sp,$sp,-24

	jal	phello
	nop

	li	$a0,0
	syscall	1
	nop

phello:
	addiu   $sp, $sp, -24
	sw      $ra, -20($sp)

	move	$a0,$zero
	la	$a1,message
	li	$a2,3

	syscall	4

	nop

	lw      $ra, 20($sp)
	jr      $ra

message:
	.ascii "hi\n"
