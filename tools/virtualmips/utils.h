 /*
  * Copyright (C) yajin 2008 <yajinzhou@gmail.com >
  *
  * This file is part of the virtualmips distribution.
  * See LICENSE file for terms of the license.
  *
  */

#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdarg.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <time.h>
#include <netinet/in.h>
#include <stdio.h>
#include <assert.h>

#include "config.h"
#include "types.h"
#include "system.h"

/* Endianness */
#define ARCH_BIG_ENDIAN     0x4321
#define ARCH_LITTLE_ENDIAN  0x1234

#if defined(PPC) || defined(__powerpc__) || defined(__ppc__)
#define HOST_BYTE_ORDER ARCH_BIG_ENDIAN
#elif defined(__sparc) || defined(__sparc__)
#define HOST_BYTE_ORDER ARCH_BIG_ENDIAN
#elif defined(__alpha) || defined(__alpha__)
#define HOST_BYTE_ORDER ARCH_LITTLE_ENDIAN
#elif defined(__i386) || defined(__i386__) || defined(i386)
#define HOST_BYTE_ORDER ARCH_LITTLE_ENDIAN
#elif defined(__x86_64__)
#define HOST_BYTE_ORDER ARCH_LITTLE_ENDIAN
#endif

#ifndef HOST_BYTE_ORDER
#error Please define your architecture in utils.h!
#endif

#if __GNUC__ > 2
#define forced_inline inline __attribute__((always_inline))
#define no_inline __attribute__ ((noinline))
/* http://kerneltrap.org/node/4705 */
#define likely(x)    __builtin_expect((x),1)
#define unlikely(x)  __builtin_expect((x),0)
#else
#define forced_inline inline
#define no_inline
#define likely(x)    (x)
#define unlikely(x)  (x)
#endif

#define fastcall   __attribute__((regparm(3)))
#define asmlinkage __attribute__((regparm(0)))

#define ASSERT(a,format,args...)  do{ if ((format!=NULL)&&(!(a)))   fprintf(stderr,format, ##args); assert((a));} while(0)

/* BOOT METHOD */
enum {
    BOOT_BINARY = 1,
    BOOT_ELF = 2,
};
/* BOOT FROM */
enum {
    BOOT_FROM_NOR_FLASH = 1,
    BOOT_FROM_NAND_FLASH = 2,
};
/* FLASH TYPE */
enum {
    FLASH_TYPE_NOR_FLASH = 1,
    FLASH_TYPE_NAND_FLASH = 2,
};

/* Size of a field in a structure */
#define SIZEOF(st,field) (sizeof(((st *)NULL)->field))

/* Compute offset of a field in a structure */
#define OFFSET(st,f)     ((long)&((st *)(NULL))->f)
/* Max and min macro */
#define m_max(a,b) (((a) > (b)) ? (a) : (b))
#define m_min(a,b) (((a) < (b)) ? (a) : (b))

/* MTS mapping info */
typedef struct {
    m_va_t vaddr;
    m_pa_t paddr;
    m_uint64_t len;

    // m_uint32_t cached;
    m_uint32_t tlb_index;
    m_uint8_t mapped;
    m_uint8_t dirty;
    m_uint8_t valid;
    m_uint32_t asid;
    m_uint8_t g_bit;

} mts_map_t;

/* Invalid VTLB entry */
#define MTS_INV_ENTRY_MASK  0x00000001

/* MTS entry flags */
#define MTS_FLAG_DEV   0x000000001      /* Virtual device used */
#define MTS_FLAG_COW   0x000000002      /* Copy-On-Write */
#define MTS_FLAG_EXEC  0x000000004      /* Exec page */

/* Virtual TLB entry (32-bit MMU) */

struct mts32_entry {
    m_uint32_t gvpa;            /* Guest Virtual Page Address */
    m_uint32_t gppa;            /* Guest Physical Page Address */
    m_iptr_t hpa;               /* Host Page Address */
    m_uint32_t asid;
    m_uint8_t g_bit;
    m_uint8_t dirty_bit;
    m_uint8_t mapped;
    m_uint32_t flags;           /* Flags */
} __attribute__ ((aligned (16)));
typedef struct mts32_entry mts32_entry_t;

/* Virtual TLB entry (64-bit MMU) */

struct mts64_entry {
    m_va_t gvpa;                /* Guest Virtual Page Address */
    m_pa_t gppa;                /* Guest Physical Page Address */
    m_iptr_t hpa;               /* Host Page Address */
    m_uint32_t flags;           /* Flags */
} __attribute__ ((aligned (16)));
typedef struct mts64_entry mts64_entry_t;

/* Host register allocation */
#define HREG_FLAG_ALLOC_LOCKED  1
#define HREG_FLAG_ALLOC_FORCED  2

struct hreg_map {
    int hreg, vreg;
    int flags;
    struct hreg_map *prev, *next;
};

/* Global logfile */
extern FILE *log_file;

/* Check status of a bit */
static inline int check_bit (u_int old, u_int new, u_int bit)
{
    int mask = 1 << bit;

    if ((old & mask) && !(new & mask))
        return (1);             /* bit unset */

    if (!(old & mask) && (new & mask))
        return (2);             /* bit set */

    /* no change */
    return (0);
}

/* Sign-extension */
#if DATA_WIDTH==64
static forced_inline m_int64_t sign_extend (m_int64_t x, int len)
#elif DATA_WIDTH==32
static forced_inline m_int32_t sign_extend (m_int32_t x, int len)
#else
#error Undefined DATA_WIDTH
#endif
{
    len = DATA_WIDTH - len;
    return (x << len) >> len;
}

/* Sign-extension (32-bit) */
static forced_inline m_int32_t sign_extend_32 (m_int32_t x, int len)
{
    len = 32 - len;
    return (x << len) >> len;
}

/* Extract bits from a 32-bit values */
static inline int bits (m_uint32_t val, int start, int end)
{
    return ((val >> start) & ((1 << (end - start + 1)) - 1));
}

/* Normalize a size */
static inline u_int normalize_size (u_int val, u_int nb, int shift)
{
    return (((val + nb - 1) & ~(nb - 1)) >> shift);
}

/* Convert a 16-bit number between little and big endian */
static forced_inline m_uint16_t swap16 (m_uint16_t value)
{
    return ((value >> 8) | ((value & 0xFF) << 8));
}

/* Convert a 32-bit number between little and big endian */
static forced_inline m_uint32_t swap32 (m_uint32_t value)
{
    m_uint32_t result;

    result = value >> 24;
    result |= ((value >> 16) & 0xff) << 8;
    result |= ((value >> 8) & 0xff) << 16;
    result |= (value & 0xff) << 24;
    return (result);
}

/* Convert a 64-bit number between little and big endian */
static forced_inline m_uint64_t swap64 (m_uint64_t value)
{
    m_uint64_t result;

    result = (m_uint64_t) swap32 (value & 0xffffffff) << 32;
    result |= swap32 (value >> 32);
    return (result);
}

/* Get current time in number of msec since epoch */
static inline m_tmcnt_t m_gettime (void)
{
    struct timeval tvp;

    gettimeofday (&tvp, NULL);
    return (((m_tmcnt_t) tvp.tv_sec * 1000) +
        ((m_tmcnt_t) tvp.tv_usec / 1000));
}

/* Get current time in number of usec since epoch */
static inline m_tmcnt_t m_gettime_usec (void)
{
    struct timeval tvp;

    gettimeofday (&tvp, NULL);
    return (((m_tmcnt_t) tvp.tv_sec * 1000000) + (m_tmcnt_t) tvp.tv_usec);
}

#ifdef __CYGWIN__
#define GET_TIMEZONE _timezone
#else
#define GET_TIMEZONE timezone
#endif

/* Get current time in number of ms (localtime) */
static inline m_tmcnt_t m_gettime_adj (void)
{
    struct timeval tvp;
    struct tm tmx;
    time_t gmt_adjust;
    time_t ct;

    gettimeofday (&tvp, NULL);
    ct = tvp.tv_sec;
    localtime_r (&ct, &tmx);

#if defined(__CYGWIN__) || defined(SUNOS)
    gmt_adjust = -(tmx.tm_isdst ? GET_TIMEZONE - 3600 : GET_TIMEZONE);
#else
    gmt_adjust = tmx.tm_gmtoff;
#endif

    tvp.tv_sec += gmt_adjust;
    return (((m_tmcnt_t) tvp.tv_sec * 1000) +
        ((m_tmcnt_t) tvp.tv_usec / 1000));
}

//#if 0
#define DEBUGGING_DISABLED
//#endif
#ifndef DEBUGGING_DISABLED
#define SIM_DEBUG(arg1) Debug arg1
#else
#define SIM_DEBUG(arg1)
#endif

/*return a file size*/
unsigned int get_file_size (const char *filename);

/* Dynamic sprintf */
char *dyn_sprintf (const char *fmt, ...);

/* Split a string */
int m_strsplit (char *str, char delim, char **array, int max_count);

/* Tokenize a string */
int m_strtok (char *str, char delim, char **array, int max_count);

/* Quote a string */
char *m_strquote (char *buffer, size_t buf_len, char *str);

/* Ugly function that dumps a structure in hexa and ascii. */
void mem_dump (FILE * f_output, u_char * pkt, u_int len);

/* Logging function */
void m_flog (FILE * fd, char *module, char *fmt, va_list ap);

/* Logging function */
//void m_log(char *module,char *fmt,...);

/* Returns a line from specified file (remove trailing '\n') */
char *m_fgets (char *buffer, int size, FILE * fd);

/* Read a file and returns it in a buffer */
ssize_t m_read_file (char *filename, char **buffer);

/* Allocate aligned memory */
void *m_memalign (size_t boundary, size_t size);

/* Block specified signal for calling thread */
int m_signal_block (int sig);

/* Unblock specified signal for calling thread */
int m_signal_unblock (int sig);

/* Set non-blocking mode on a file descriptor */
int m_fd_set_non_block (int fd);

/* Map a memory zone from a file */
u_char *memzone_map_file (int fd, size_t len);

/* Map a memory zone from a file, with copy-on-write (COW) */
u_char *memzone_map_cow_file (int fd, size_t len);

/* Create a file to serve as a memory zone */
int memzone_create_file (char *filename, size_t len, u_char ** ptr);

/* Open a file to serve as a COW memory zone */
int memzone_open_cow_file (char *filename, size_t len, u_char ** ptr);

/* Open a file and map it in memory */
int memzone_open_file (char *filename, u_char ** ptr, off_t * fsize);

/* Compute NVRAM checksum */
m_uint16_t nvram_cksum (m_uint16_t * ptr, size_t count);

/* Byte-swap a memory block */
void mem_bswap32 (void *ptr, size_t len);

/* Reverse a byte */
m_uint8_t m_reverse_u8 (m_uint8_t val);
void Debug (char flag, char *format, ...);

#endif
