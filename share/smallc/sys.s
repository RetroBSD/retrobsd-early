	.text

#
# unsigned int inp( unsigned int a )
# Pito
inp:
        lw      $a0, 0($sp)
        # errno handling code after syscall is not ideal,
        # but I don't think the assembler handles specifying
        # relocations for %hi and %lo yet, so the handling
        # shown in the retrobsd code is not really doable

        syscall 153
	j       serrn
        nop
        jr      $ra
        nop

#
#  outp( unsigned int a, unsigned int d )
#  Pito
outp:
        lw      $a0, 4($sp)
        lw      $a1, 0($sp)
        # errno handling code after syscall is not ideal,
        # but I don't think the assembler handles specifying
        # relocations for %hi and %lo yet, so the handling
        # shown in the retrobsd code is not really doable
        syscall 152
        nop
        j       serrn
        nop
        jr      $ra
        nop

#
# int open( char* file, int flags, int mode )
#
open:
	lw	$a0, 8($sp)
	lw	$a1, 4($sp)
	lw	$a2, 0($sp)

	# errno handling code after syscall is not ideal,
	# but I don't think the assembler handles specifying
	# relocations for %hi and %lo yet, so the handling
	# shown in the retrobsd code is not really doable

	syscall	5
	nop
	j	serrn
	nop

	jr	$ra
	nop

#
# int read( int fd, void* dest, int count)
# returns: count of chars read or -1 if error (see errno)
#
read:
	lw	$a0, 8($sp)
	lw	$a1, 4($sp)
	lw	$a2, 0($sp)

	# errno handling code after syscall is not ideal,
	# but I don't think the assembler handles specifying
	# relocations for %hi and %lo yet, so the handling
	# shown in the retrobsd code is not really doable

	syscall	3
	nop
	j	serrn
	nop

	jr	$ra
	nop



#
# int write( int fd, void* string, int count );
# returns: count of chars written or -1 if error (see errno)
#
write:
	lw	$a0, 8($sp)
	lw	$a1, 4($sp)
	lw	$a2, 0($sp)

	# errno handling code after syscall is not ideal,
	# but I don't think the assembler handles specifying
	# relocations for %hi and %lo yet, so the handling
	# shown in the retrobsd code is not really doable

	syscall	4
	nop
	j	serrn
	nop

	jr	$ra
	nop

#
# int close( int fd );
#
close:
	lw	$a0, 0($sp)

	# errno handling code after syscall is not ideal,
	# but I don't think the assembler handles specifying
	# relocations for %hi and %lo yet, so the handling
	# shown in the retrobsd code is not really doable

	syscall	6
	nop
	j	serrn
	nop

	jr	$ra
	nop

#
# exit( int n );
#
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

Tcallstk:
	jr	$t1
	nop

	.data
errno:	.byte	0,0,0,0

        .globl inp
        .globl outp
	.globl open
	.globl read
	.globl write
	.globl close
	.globl exit
        .globl errno
        .globl Tcase
        .globl Tcallstk
