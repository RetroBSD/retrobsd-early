#ifndef _SWAP_H
#define _SWAP_H

#ifdef KERNEL
#include "ioctl.h"
#else
#include <sys/ioctl.h>
#endif

#define TFALLOC _IOWR('s',1,off_t)

#endif
