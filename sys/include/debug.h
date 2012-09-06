#ifndef _DEBUG_H
#define _DEBUG_H

#ifdef GLOBAL_DEBUG
#define DEBUG(...) printf(__VA_ARGS__)
#else
#define DEBUG(...)
#endif

#endif
