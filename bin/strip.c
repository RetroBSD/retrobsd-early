/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include <a.out.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/file.h>

struct	exec head;
int	status;

main(argc, argv)
	char *argv[];
{
	register i;

	signal(SIGHUP, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	for (i = 1; i < argc; i++) {
		strip(argv[i]);
		if (status > 1)
			break;
	}
	exit(status);
}

strip(name)
	char *name;
{
	register f;
	long size;

	f = open(name, O_RDWR);
	if (f < 0) {
		fprintf(stderr, "strip: "); perror(name);
		status = 1;
		goto out;
	}
	if (read(f, (char *)&head, sizeof (head)) < 0 || N_BADMAG(head)) {
		printf("strip: %s not in a.out format\n", name);
		status = 1;
		goto out;
	}
	if (head.a_syms == 0 && head.a_magic != OMAGIC)
		goto out;

	size = N_DATOFF(head) + head.a_data;
	if (ftruncate(f, size) < 0) {
		fprintf(stderr, "strip: ");
		perror(name);
		status = 1;
		goto out;
	}
	head.a_magic = XMAGIC;
	head.a_reltext = 0;
	head.a_reldata = 0;
	head.a_syms = 0;
	(void) lseek(f, (long)0, L_SET);
	(void) write(f, (char *)&head, sizeof (head));
out:
	close(f);
}
