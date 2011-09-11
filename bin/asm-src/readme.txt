This is a work in progress and is mostly a toy program at this point.
It was written initially as a weekend hack and it only really 
understands machine instruction assembly (that is, it will assemble 
"addu $t1,$t2,$t3", but it will not assemble "addu $t1, $t2, 1" - you 
must use the addiu instruction instead). The project was started
because I was unable to find any existing assembler or compiler that
was anywhere near small enough to run in the space available.

It will be faster to list what the assembler does do then to list the 
things it does not.

Command Line:

	asm input_file

	Assembles the input_file and generates an a.out file.

It accepts R, I and J type MIPS instructions shown in the PDF at
http://www.cs.sunysb.edu/~lw/spim/MIPSinstHex.pdf. It also supports
the pseudo instructions la, li and move. Symbols are handled poorly 
and anything other than simple references are likely to be generated 
incorrectly. Also, most of the generated instructions have not been 
verified, so if your program does not run correctly, it might be that 
the assembler has not generated the correct instructions.

At present, the following directives are understood:

	.text		- accepted but not required necessary
	.align n	
	.ascii "string"
	.word	n

As time permits I hope to turn this into something useful. The
next version will, hopefully, but significantly different from
the current.





