#	Small C M68000
#	Coder (1.2,84/11/28)
#	Front End (2.7,84/11/28)
	global	Tlneg
	global	Tcase
	global	Teq
	global	Tne
	global	Tlt
	global	Tle
	global	Tgt
	global	Tge
	global	Tult
	global	Tule
	global	Tugt
	global	Tuge
	global	Tbool
	global	Tmult
	global	Tdiv
	global	Tmod
	text
main:

	mov.l	&LL0+0,%d0
	mov.l	%d0,-(%sp)
	mov.l	&1,%d3
	jsr	printf
	add.l	&4,sp
LL1:

	rts
	data
LL0:
	byte	104,101,108,108,111,10,0
	global	etext
	global	edata
	global	main
	global	printf

#0 error(s) in compilation
#	literal pool:7
#	global pool:56
#	Macro pool:44
