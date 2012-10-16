	.text

_write:	
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

_exit:
	lw	$a0, 0($sp)
	addiu   $sp, $sp, -4
	syscall	1
	nop

serrn:
        la	$t1, _errno
	sw	$t0, 0($t1)
	jr	$ra
	nop

#
# $v0 = value to switch on
# 0($sp) = pointer to list of value,ptr cases
#         ended where ptr=0, value is used as pointer to jump to in default case
# looks like stack is popped as part of this
# FIXME - The assembler/linker only stores the bottom 16 bits
# of the labels in pair, so we construct the address by merging the 16 bits
# in the cell with the upper 16 bits in the return address of the code that
# called this. Is there a way to get the assembler linker to store the full
# address? If so, that should be used instead.
#
Tcase:
	lw	$t1, 0($sp)	# t1=pointer to list of value/ptr pairs
	addiu	$sp, $sp, 4	# pop stack that held pointer
.Tcl:
        lw      $t2, 0($t1)     # get value from pair
        lw      $t3, 4($t1)     # get ptr from pair
        beq     $t3, $zero, .Tcd
        nop

	beq     $t2, $v0, .Tcm
        nop 

        addiu   $t1, $t1, 8          # t1 += size of pair
        j       .Tcl
	nop

.Tcd:   
        move	$t3, $t2
.Tcm:
	lui     $t2, 0xffff
	and	$t2, $t2, $ra
        or      $t3, $t3, $t2
	jr	$t3
        nop

	.data
_errno:	.byte	0,0,0,0

	.globl _write
	.globl _exit
        .globl _errno
        .globl Tcase
