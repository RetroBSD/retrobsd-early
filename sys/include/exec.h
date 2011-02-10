/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef _EXEC_
#define _EXEC_
/*
 * Header prepended to each a.out file.
 */
struct	exec {
	int	a_magic;	/* magic number */
unsigned int	a_text;		/* size of text segment */
unsigned int	a_data;		/* size of initialized data */
unsigned int	a_bss;		/* size of uninitialized data */
unsigned int	a_syms;		/* size of symbol table */
unsigned int	a_entry; 	/* entry point */
unsigned int	a_unused;	/* not used */
unsigned int	a_flag; 	/* relocation info stripped */
};

/* a_magic */
#define OMAGIC		0407    /* old impure format */
#define NMAGIC		0410    /* read-only text */
#define ZMAGIC		0413    /* demand load format */

#endif
