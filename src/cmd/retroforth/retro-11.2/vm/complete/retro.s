### -*- mode: asm -*-
### Ngaro Virtual Machine for 32-bit Linux systems
### Peter Salvi, 2011

### Compile with:
###   as ngaro.s -o ngaro.o   (add --gstabs for debug info)
###   ld ngaro.o -o ngaro

### Tests:
### - passes core
### - passes io_files
### - passes io_output
### - passes vocabs
	
### Parameters
	
	.equ DATA_STACK_DEPTH, 128
	.equ RETURN_STACK_DEPTH, 1024
	.equ MEMORY_SIZE, 1000000
	.equ MAX_STRING_LENGTH, 1024
	.equ MAX_OPEN_FILES, 8

### Allocations
	
	.section .bss

	.lcomm stats, 64	# see asm-generic/stat.h
	.lcomm termios_old, 36	# see asm-generic/termbits.h
	.lcomm termios, 36
	.lcomm buffer, 1
	.lcomm winsize, 8	# see asm-generic/termios.h
	.lcomm filename, 4
	
	.lcomm data, DATA_STACK_DEPTH*4
	.lcomm return, RETURN_STACK_DEPTH*4
	.lcomm ports, 5*4
	.lcomm memory, MEMORY_SIZE*4
	.lcomm str, MAX_STRING_LENGTH
	.lcomm input, MAX_OPEN_FILES*4

### Data

	.section .data

arg_error:
	.byte 21
	.ascii "Usage: ngaro [image]\n"
default_image:
	.ascii "retroImage"
	.byte 0
file_error:
	.byte 23
	.ascii "Cannot open image file\n"
stack_error:
	.byte 16
	.ascii "Stack underflow\n"
clear_screen:
	.byte 10
	.ascii "\033[2J\033[1;1H"
instr:
	.long ins_nop, ins_lit, ins_dup, ins_drop, ins_swap, ins_push
	.long ins_pop, ins_loop, ins_jump, ins_return, ins_lt_jump
	.long ins_gt_jump, ins_ne_jump, ins_eq_jump, ins_fetch, ins_store
	.long ins_add, ins_subtract, ins_multiply, ins_divmod, ins_and
	.long ins_or, ins_xor, ins_shl, ins_shr, ins_zero_exit
	.long ins_inc, ins_dec, ins_in, ins_out, ins_wait
file_modes:
	.long 0, 01101, 02101, 2 # r, trunc/create/w, append/create/w, r/w
input_id:
	.long 0
	
### Code

	.section .text
	.globl _start
_start:
	## Save the original IO settings
	movl $54, %eax		# ioctl
	movl $0, %ebx		# stdin
	movl $0x5401, %ecx	# tcgets request
	leal termios_old, %edx	# save original termios struct
	int $0x80

	## Check command line arguments
	popl %eax
	cmpl $1, %eax
	je argc_1
	cmpl $2, %eax
	je argc_2

	## Wrong number of arguments
	leal arg_error, %ecx
	call write_str
	jmp quit

argc_1:
	## No image specified; try to load 'retroImage'
	popl %eax		# maintain stack consistence
	leal default_image, %ebx
	jmp open_image

argc_2:
	popl %eax		# program name
	popl %ebx		# image file name
	movl %ebx, filename

open_image:
	## Open file
	movl $5, %eax		# open
	movl $0, %ecx		# read only
	movl $0644, %edx	# permissions
	int $0x80
	testl %eax, %eax
	jge open_ok

	## Cannot open file
	leal file_error, %ecx
	call write_str
	jmp quit

open_ok:
	movl %eax, %ebx

	## Get file size
	movl $108, %eax		# fstat
	leal stats, %ecx	# stat record
	int $0x80

	## Read file into memory
	movl $3, %eax		# read
	leal memory, %ecx	# buffer
	movl stats+20, %edx	# count
	int $0x80

	## Close file
	movl $6, %eax		# close
	int $0x80

### Enter non-canonical mode (do not wait for enter, no echo)

	## Get IO settings for modification
	movl $54, %eax		# ioctl
	movl $0, %ebx		# stdin
	movl $0x5401, %ecx	# tcgets request
	leal termios, %edx	# termios struct to be modified
	int $0x80

	## Clear the flags
	movl $0xfff5, %edi	# clear echo & icanon flag
	andl %edi, termios+12

	## Set the new IO mode
	movl $54, %eax		# ioctl
	movl $0, %ebx		# stdin
	movl $0x5402, %ecx	# tcsets request
	leal termios, %edx	# new termios struct
	int $0x80
	
### Start the VM

vm_start:
	movl $0, %ebp		# ip
	movl $-1, %edi		# data stack pointer
	movl $-1, %esi		# return stack pointer
	movl $0, input		# stdin is the primary input
	movl $0, input_id	# and it is the default
vm_loop:
	cmp $(MEMORY_SIZE/4), %ebp
	jge vm_start		# restart the VM if ip is out of range
	movl memory(,%ebp,4), %eax
	cmpl $30, %eax
	jg implicit_call
	call *instr(,%eax,4)
	incl %ebp		# next instruction
	jmp vm_loop
implicit_call:
	incl %esi
	movl %ebp, return(,%esi,4)
	movl %eax, %ebp
	jmp vm_loop
	
quit:
	## Restore canonical mode
	movl $54, %eax		# ioctl
	movl $0, %ebx		# stdin
	movl $0x5402, %ecx	# tcsets request
	leal termios_old, %edx	# original termios struct
	int $0x80
	
	## Quit
	movl $1, %eax		# exit
	movl $0, %ebx		# exit code
	int $0x80

### Procedures
	
write_str:			# %ecx: string address (1st byte: string length)
	movl $4, %eax		# write
	movl $1, %ebx		# stdout
	xorl %edx, %edx
	movb (%ecx), %dl	# length
	incl %ecx		# string
	int $0x80
	ret

stack_underflow:
	leal stack_error, %ecx
	call write_str
	ret

convert_str:			# stack: string in memory; result in str
	xorl %eax, %eax
	movl data(,%edi,4), %ebx
	xorl %ecx, %ecx
convert_loop:
	movb memory(,%ebx,4), %al
	movb %al, str(%ecx)
	incl %ebx
	incl %ecx
	testb %al, %al
	jne convert_loop
	ret

revert_str:     		# stack: destination in memory; source in %ebx
	xorl %eax, %eax
	movl data(,%edi,4), %ecx
revert_loop:
	movb (%ebx), %al
	movb %al, memory(,%ecx,4)
	incl %ebx
	incl %ecx
	testb %al, %al
	jne revert_loop
	ret

get_window_size:
	movl $54, %eax		# ioctl
	movl $0, %ebx		# stdin
	movl $0x5413, %ecx	# tiocgwinsz request
	leal winsize, %edx	# winsize struct
	int $0x80
	ret
	
### Some macros for convenience
	
.macro check_data_1
	testl %edi, %edi
	jl stack_underflow
.endm

.macro check_data_2
	testl %edi, %edi
	jle stack_underflow
.endm

.macro check_return_1
	testl %esi, %esi
	jl stack_underflow
.endm

.macro jump_if test
	check_data_2
	subl $2, %edi
	movl data+4(,%edi,4), %eax
	cmpl %eax, data+8(,%edi,4)
	\test ins_jump_\test
	incl %ebp
	ret
ins_jump_\test:
	movl memory+4(,%ebp,4), %ebp
	decl %ebp
	ret
.endm

.macro port_neq n branch k=0
	movl ports+\n*4, %eax
	cmpl $\k, %eax
	jne \branch
.endm

.macro port_eq n branch k=0
	movl ports+\n*4, %eax
	cmpl $\k, %eax
	je \branch
.endm

.macro set_port n k
	movl \k, ports+\n*4
.endm

### Instructions
	
ins_nop:
	ret

ins_lit:
	incl %ebp
	movl memory(,%ebp,4), %eax
	incl %edi
	movl %eax, data(,%edi,4)
	ret

ins_dup:
	check_data_1
	movl data(,%edi,4), %eax
	incl %edi
	movl %eax, data(,%edi,4)
	ret

ins_drop:
	check_data_1
	decl %edi
	ret

ins_swap:
	check_data_2
	movl data-4(,%edi,4), %eax
	xchgl data(,%edi,4), %eax
	movl %eax, data-4(,%edi,4)
	ret

ins_push:
	check_data_1
	incl %esi
	movl data(,%edi,4), %eax
	movl %eax, return(,%esi,4)
	decl %edi
	ret

ins_pop:
	check_return_1
	incl %edi
	movl return(,%esi,4), %eax
	movl %eax, data(,%edi,4)
	decl %esi
	ret

ins_loop:
	check_data_1
	decl data(,%edi,4)	# this is how retro.c works; not in the docs
	jle ins_loop_branch
	movl memory+4(,%ebp,4), %ebp
	decl %ebp
	ret
ins_loop_branch:
	incl %ebp
	decl %edi
	ret

ins_jump:
	movl memory+4(,%ebp,4), %ebp
	decl %ebp
	ret

ins_return:
	check_return_1
	movl return(,%esi,4), %ebp
	decl %esi
	ret

ins_lt_jump:
	jump_if jl

ins_gt_jump:
	jump_if jg
	
ins_ne_jump:
	jump_if jne

ins_eq_jump:
	jump_if je

ins_fetch:
	check_data_1
	movl data(,%edi,4), %eax
	movl memory(,%eax,4), %ebx
	movl %ebx, data(,%edi,4)
	ret

ins_store:
	check_data_2
	movl data(,%edi,4), %eax
	decl %edi
	movl data(,%edi,4), %ebx
	decl %edi
	movl %ebx, memory(,%eax,4)
	ret

ins_add:
	check_data_2
	movl data(,%edi,4), %eax
	decl %edi
	addl %eax, data(,%edi,4)
	ret

ins_subtract:
	check_data_2
	movl data(,%edi,4), %eax
	decl %edi
	subl %eax, data(,%edi,4)
	ret

ins_multiply:
	check_data_2
	movl data(,%edi,4), %eax
	decl %edi
	imull data(,%edi,4)
	movl %eax, data(,%edi,4)
	ret

ins_divmod:
	check_data_2
	movl data-4(,%edi,4), %eax
	xorl %edx, %edx
	testl %eax, %eax
	jl dividend_negative
	idivl data(,%edi,4)
	movl %edx, data-4(,%edi,4)
	movl %eax, data(,%edi,4)
	ret
dividend_negative:
	negl %eax
	idivl data(,%edi,4)
	negl %eax
	negl %edx
	movl %edx, data-4(,%edi,4)
	movl %eax, data(,%edi,4)
	ret

ins_and:
	check_data_2
	movl data(,%edi,4), %eax
	decl %edi
	andl %eax, data(,%edi,4)
	ret

ins_or:
	check_data_2
	movl data(,%edi,4), %eax
	decl %edi
	orl %eax, data(,%edi,4)
	ret

ins_xor:
	check_data_2
	movl data(,%edi,4), %eax
	decl %edi
	xorl %eax, data(,%edi,4)
	ret

ins_shl:
	check_data_2
	movl data(,%edi,4), %ecx
	decl %edi
	shll %cl, data(,%edi,4)
	ret

ins_shr:
	check_data_2
	movl data(,%edi,4), %ecx
	decl %edi
	shrl %cl, data(,%edi,4)
	ret

ins_zero_exit:
	check_data_1
	movl data(,%edi,4), %eax
	testl %eax, %eax
	je ins_zero_exit_branch
	ret
ins_zero_exit_branch:
	decl %edi
	movl return(,%esi,4), %ebp
	decl %esi
	ret

ins_inc:
	check_data_1
	incl data(,%edi,4)
	ret

ins_dec:
	check_data_1
	decl data(,%edi,4)
	ret

ins_in:
	check_data_1
	movl data(,%edi,4), %eax
	movl ports(,%eax,4), %ebx
	movl %ebx, data(,%edi,4)
	movl $0, ports(,%eax,4)
	ret

ins_out:
	check_data_2
	movl data(,%edi,4), %eax
	decl %edi
	movl data(,%edi,4), %ebx
	decl %edi
	movl %ebx, ports(,%eax,4)
	set_port 0 $0
	ret

ins_wait:
	port_neq 0 no_port

port1:	## Input	
	port_neq 1 port2 1
	set_port 0 $0
input_again:
	movl $3, %eax		  # read
	movl input_id, %ecx
	movl input(,%ecx,4), %ebx # handle
	leal buffer, %ecx	  # buffer
	movl $1, %edx		  # count
	int $0x80
	testl %eax, %eax
	jne input_ok
	movl input_id, %eax
	testl %eax, %eax
	je end_of_input
	movl input(,%eax,4), %ebx # handle
	movl $6, %eax		  # close
	int $0x80
	decl input_id
	jmp input_again
end_of_input:
	popl %eax		# restore stack
	jmp quit
input_ok:
	xorl %eax, %eax
	movb buffer, %al
	set_port 1 %eax
	
port2:  ## Output
	port_neq 2 port4 1
	set_port 0 $0
	check_data_1
	cmpl $0, data(,%edi,4)
	jge port2_char
	leal clear_screen, %ecx
	call write_str
	jmp port2_done
port2_char:
	movl $4, %eax		 # write
	movl $1, %ebx		 # stdout
	leal data(,%edi,4), %ecx # string
	movl $1, %edx
	int $0x80
port2_done:
	set_port 2 $0
	decl %edi

port4:  ## File operations
	port_eq 4 port5
	set_port 0 $0

port4_p1: ## Save image
	cmpl $1, %eax
	jne port4_p2
	movl $5, %eax		# open
	movl filename, %ebx	# filename
	movl $01101, %ecx	# mode (trunc/create/w)
	movl $0644, %edx	# permissions
	int $0x80
	testl %eax, %eax
	jl cannot_save
	movl %eax, %ebx		# handle
	movl $4, %eax		# write
	leal memory, %ecx	# buffer
	movl memory+12, %edx	# count
	shll $2, %edx		# count
	int $0x80
	movl $6, %eax		# close
	int $0x80
cannot_save:
	set_port 4 $0
	jmp port5

port4_p2: ## Add input source
	cmpl $2, %eax
	jne port4_1
	check_data_1
	call convert_str
	decl %edi
	movl $5, %eax		# open
	leal str, %ebx  	# filename
	movl $0, %ecx   	# mode (r)
	movl $0644, %edx	# permissions
	int $0x80
	testl %eax, %eax
	jl cannot_include
	incl input_id
	movl input_id, %ebx
	movl %eax, input(,%ebx,4)
cannot_include:
	set_port 4 $0
	jmp port5

port4_1: ## Open file
	cmpl $-1, %eax
	jne port4_2
	check_data_2
	movl data(,%edi,4), %edx
	decl %edi
	call convert_str
	movl file_modes(,%edx,4), %ecx  # open mode
	leal str, %ebx		        # filename
	movl $5, %eax			# open
	movl $0644, %edx		# permissions
	int $0x80
	decl %edi
	testl %eax, %eax
	jge file_open_ok
	set_port 4 $0
	jmp port5
file_open_ok:
	set_port 4 %eax
	jmp port5

port4_2: ## Read byte
	cmpl $-2, %eax
	jne port4_3
	check_data_1
	movl $3, %eax		  # read
	movl data(,%edi,4), %ebx  # handle
	leal buffer, %ecx	  # buffer
	movl $1, %edx		  # count
	int $0x80
	decl %edi
	testl %eax, %eax
	jge read_byte_ok
	set_port 4 $0
	jmp port5
read_byte_ok:
	xorl %eax, %eax
	movb buffer, %al
	set_port 4 %eax
	jmp port5

port4_3: ## Write byte
	cmpl $-3, %eax
	jne port4_4
	check_data_2
	movl $4, %eax		  # write
	movl data(,%edi,4), %ebx  # handle
	movl data-4(,%edi,4), %ecx
	movb %cl, buffer
	leal buffer, %ecx	  # buffer
	movl $1, %edx		  # count
	int $0x80
	subl $2, %edi
	testl %eax, %eax
	jge write_byte_ok
	set_port 4 $0
	jmp port5
write_byte_ok:
	set_port 4 $1
	jmp port5

port4_4: ## Close file
	cmpl $-4, %eax
	jne port4_5
	check_data_1
	movl $6, %eax		  # close
	movl data(,%edi,4), %ebx  # handle
	int $0x80
	decl %edi
	set_port 4 %eax
	jmp port5

port4_5: ## Location in file
	cmpl $-5, %eax
	jne port4_6
	check_data_1
	movl $19, %eax		  # lseek
	movl data(,%edi,4), %ebx  # handle
	movl $0, %ecx		  # offset
	movl $1, %edx		  # seek from current position
	int $0x80
	decl %edi
	set_port 4 %eax
	jmp port5

port4_6: ## Seek in file
	cmpl $-6, %eax
	jne port4_7
	check_data_2
	movl $19, %eax		   # lseek
	movl data(,%edi,4), %ebx   # handle
	movl data-4(,%edi,4), %ecx # offset
	movl $0, %edx		   # seek from the beginning
	int $0x80
	subl $2, %edi
	set_port 4 %eax
	jmp port5

port4_7: ## File size
	cmpl $-7, %eax
	jne port4_8
	check_data_1
	movl $108, %eax		  # fstat
	movl data(,%edi,4), %ebx  # handle
	leal stats, %ecx	  # stat record
	int $0x80
	decl %edi
	testl %eax, %eax
	jge file_size_ok
	set_port 4 $0
	jmp port5
file_size_ok:
	movl stats+20, %eax
	set_port 4 %eax
	jmp port5

port4_8: ## Delete file
	cmpl $-8, %eax
	jne port5
	check_data_1
	call convert_str
	movl $10, %eax		  # unlink
	leal str, %ebx            # filename
	int $0x80
	decl %edi
	testl %eax, %eax
	je delete_file_ok
	set_port 4 $0
	jmp port5
delete_file_ok:
	set_port 4 $-1

port5:  ## Various features
	port_eq 5 no_port
	set_port 0 $0

port5_1: ## Memory size
	cmpl $-1, %eax
	jne port5_5
	set_port 5 $MEMORY_SIZE
	jmp no_port

port5_5: ## Data stack size
	cmpl $-5, %eax
	jne port5_6
	movl %edi, %ebx
	incl %ebx
	set_port 5 %ebx
	jmp no_port

port5_6: ## Return stack size
	cmpl $-6, %eax
	jne port5_8
	movl %esi, %ebx
	incl %ebx
	set_port 5 %ebx
	jmp no_port

port5_8: ## Current time
	cmpl $-8, %eax
	jne port5_9
	movl $13, %eax		# time
	movl $0, %ebx
	int $0x80
	set_port 5 %eax
	jmp no_port

port5_9: ## Quit
	cmpl $-9, %eax
	jne port5_10
	popl %eax		# restore stack
	jmp quit

port5_10: ## Environment variable search
	cmpl $-10, %eax
	jne port5_11
	check_data_2
	call convert_str
	decl %edi
	movl %esp, %edx
	popl %eax		# return address
	popl %eax		# NULL (end of command-line arguments)
env_loop:			# find str in the environment and put in %ebx
	popl %ebx
	testl %ebx, %ebx
	je env_not_found
	xorl %ecx, %ecx
compare_loop:			# compare the string at %ebx with str
	movb str(%ecx), %al
	testb %al, %al
	je str_ended
	cmpb $61, (%ebx)	# '='
	je env_loop		# premature end of environment variable string
	cmpb (%ebx), %al
	jne env_loop		# strings differ
	incl %ebx
	incl %ecx
	jmp compare_loop
str_ended:
	cmpb $61, (%ebx)	# '='
	jne env_loop		# premature end of str
	incl %ebx
	call revert_str
	jmp env_end
env_not_found:
	movl data(,%edi,4), %ecx
 	movb $0, memory(,%ecx,4)
env_end:
	movl %edx, %esp
	decl %edi
	set_port 5 $0
	jmp no_port

port5_11: ## Window width
	cmpl $-11, %eax
	jne port5_12
	call get_window_size
	xorl %eax, %eax
	movw winsize, %ax
	set_port 5 %eax
	jmp no_port

port5_12: ## Window height
	cmpl $-11, %eax
	jne port5_end
	call get_window_size
	xorl %eax, %eax
	movw winsize+2, %ax
	set_port 5 %eax
	jmp no_port

port5_end:
	set_port 5 $0

no_port:
	ret
