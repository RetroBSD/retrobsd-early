/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * System call names.
 */
#ifndef pdp11
char *syscallnames[] = {
	"indir",		/*   0 = indir */
	"exit",			/*   1 = exit */
	"fork",			/*   2 = fork */
	"read",			/*   3 = read */
	"write",		/*   4 = write */
	"open",			/*   5 = open */
	"close",		/*   6 = close */
	"wait4",		/*   7 = wait4 */
	"#8",			/*   8 = (old creat) */
	"link",			/*   9 = link */
	"unlink",		/*  10 = unlink */
	"execv",		/*  11 = execv */
	"chdir",		/*  12 = chdir */
	"fchdir",		/*  13 = fchdir */
	"mknod",		/*  14 = mknod */
	"chmod",		/*  15 = chmod */
	"chown",		/*  16 = chown; now 3 args */
	"chflags",		/*  17 = chflags */
	"fchflags",		/*  18 = fchflags */
	"lseek",		/*  19 = lseek */
	"getpid",		/*  20 = getpid */
	"mount",		/*  21 = mount */
	"umount",		/*  22 = umount */
	"__sysctl",		/*  23 = __sysctl */
	"getuid",		/*  24 = getuid */
	"geteuid",		/*  25 = geteuid */
	"ptrace",		/*  26 = ptrace */
	"getppid",		/*  27 = getppid */
	"statfs",		/*  28 = statfs */
	"fstatfs",		/*  29 = fstatfs */
	"getfsstat",		/*  30 = getfsstat */
	"sigaction",		/*  31 = sigaction */
	"sigprocmask",		/*  32 = sigprocmask */
	"access",		/*  33 = access */
	"sigpending",		/*  34 = sigpending */
	"sigaltstack",		/*  35 = sigaltstack */
	"sync",			/*  36 = sync */
	"kill",			/*  37 = kill */
	"stat",			/*  38 = stat */
	"getlogin",		/*  39 = getlogin */
	"lstat",		/*  40 = lstat */
	"dup",			/*  41 = dup */
	"pipe",			/*  42 = pipe */
	"setlogin",		/*  43 = setlogin */
	"profil",		/*  44 = profil */
	"setuid",		/*  45 = setuid */
	"seteuid",		/*  46 = seteuid */
	"getgid",		/*  47 = getgid */
	"getegid",		/*  48 = getegid */
	"setgid",		/*  49 = setgid */
	"setegid",		/*  50 = setegid */
	"#51",			/*  51 - unused */
	"phys",			/*  52 = (2.9) set phys addr */
	"lock",			/*  53 = (2.9) lock in core */
	"ioctl",		/*  54 = ioctl */
	"reboot",		/*  55 = reboot */
	"sigwait",		/*  56 = sigwait */
	"symlink",		/*  57 = symlink */
	"readlink",		/*  58 = readlink */
	"execve",		/*  59 = execve */
	"umask",		/*  60 = umask */
	"chroot",		/*  61 = chroot */
	"fstat",		/*  62 = fstat */
	"#63",			/*  63 = unused */
	"#64",			/*  64 = (old getpagesize) */
	"pselect",		/*  65 = pselect */
	"vfork",		/*  66 = vfork */
	"#67",			/*  67 = unused */
	"#68",			/*  68 = unused */
	"sbrk",			/*  69 = sbrk */
	"#70",			/*  70 = unused */
	"#71",			/*  71 = unused */
	"#72",			/*  72 = unused */
	"#73",			/*  73 = unused */
	"#74",			/*  74 = unused */
	"#75",			/*  75 = unused */
	"vhangup",		/*  76 = vhangup */
	"#77",			/*  77 = unused */
	"#78",			/*  78 = unused */
	"getgroups",		/*  79 = getgroups */
	"setgroups",		/*  80 = setgroups */
	"getpgrp",		/*  81 = getpgrp */
	"setpgrp",		/*  82 = setpgrp */
	"setitimer",		/*  83 = setitimer */
	"old wait",		/*  84 = wait,wait3 COMPAT*/
	"#85",			/*  85 = unused */
	"getitimer",		/*  86 = getitimer */
	"#87",			/*  87 = (old gethostname) */
	"#88",			/*  88 = (old sethostname) */
	"getdtablesize",	/*  89 = getdtablesize */
	"dup2",			/*  90 = dup2 */
	"#91",			/*  91 = unused */
	"fcntl",		/*  92 = fcntl */
	"select",		/*  93 = select */
	"#94",			/*  94 = unused */
	"fsync",		/*  95 = fsync */
	"setpriority",		/*  96 = setpriority */
	"socket",		/*  97 = socket */
	"connect",		/*  98 = connect */
	"accept",		/*  99 = accept */
	"getpriority",		/* 100 = getpriority */
	"send",			/* 101 = send */
	"recv",			/* 102 = recv */
	"sigreturn",		/* 103 = sigreturn */
	"bind",			/* 104 = bind */
	"setsockopt",		/* 105 = setsockopt */
	"listen",		/* 106 = listen */
	"sigsuspend",		/* 107 = sigsuspend */
	"#108",			/* 108 = (old sigvec) */
	"#109",			/* 109 = (old sigblock) */
	"#110",			/* 110 = (old sigsetmask) */
	"#111",			/* 111 = (old sigpause)  */
	"sigstack",		/* 112 = sigstack COMPAT-43 */
	"recvmsg",		/* 113 = recvmsg */
	"sendmsg",		/* 114 = sendmsg */
	"#115",			/* 115 = unused */
	"gettimeofday",		/* 116 = gettimeofday */
	"getrusage",		/* 117 = getrusage */
	"getsockopt",		/* 118 = getsockopt */
	"#119",			/* 119 = unused */
	"readv",		/* 120 = readv */
	"writev",		/* 121 = writev */
	"settimeofday",		/* 122 = settimeofday */
	"fchown",		/* 123 = fchown */
	"fchmod",		/* 124 = fchmod */
	"recvfrom",		/* 125 = recvfrom */
	"#126",			/* 126 = (old setreuid) */
	"#127",			/* 127 = (old setregid) */
	"rename",		/* 128 = rename */
	"truncate",		/* 129 = truncate */
	"ftruncate",		/* 130 = ftruncate */
	"flock",		/* 131 = flock */
	"#132",			/* 132 = unused */
	"sendto",		/* 133 = sendto */
	"shutdown",		/* 134 = shutdown */
	"socketpair",		/* 135 = socketpair */
	"mkdir",		/* 136 = mkdir */
	"rmdir",		/* 137 = rmdir */
	"utimes",		/* 138 = utimes */
	"#139",			/* 139 = unused */
	"adjtime",		/* 140 = adjtime */
	"getpeername",		/* 141 = getpeername */
	"#142",			/* 142 = (old gethostid) */
	"#143",			/* 143 = (old sethostid) */
	"getrlimit",		/* 144 = getrlimit */
	"setrlimit",		/* 145 = setrlimit */
	"killpg",		/* 146 = killpg */
	"#147",			/* 147 = unused */
	"setquota",		/* 148 = setquota */
	"quota",		/* 149 = quota */
	"getsockname",		/* 150 = getsockname */
	"#151",			/* 151 = unused */
	"nostk",		/* 152 = nostk */
	"fetchi",		/* 153 = fetchi */
	"ucall",		/* 154 = ucall */
	"fperr",		/* 155 = fperr */
};
#endif
