 /*
  * Copyright (C) yajin 2008 <yajinzhou@gmail.com >
  *     
  * This file is part of the virtualmips distribution. 
  * See LICENSE file for terms of the license. 
  *
  */


#ifndef __TYPES_H__
#define __TYPES_H__


/*types from qemu*/
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

// Linux/Sparc64 defines uint64_t
#if !(defined (__sparc_v9__) && defined(__linux__))
/* XXX may be done for all 64 bits targets ? */
#if defined (__x86_64__) || defined(__ia64)
typedef unsigned long uint64_t;
#else
typedef unsigned long long uint64_t;
#endif
#endif

#ifndef __sun__
typedef signed char int8_t;
#endif
typedef signed short int16_t;
typedef signed int int32_t;
// Linux/Sparc64 defines int64_t
#if !(defined (__sparc_v9__) && defined(__linux__))
#if defined (__x86_64__) || defined(__ia64)
typedef signed long int64_t;
#else
typedef signed long long int64_t;
#endif
#endif




/*used in dynamips. so just typedef again*/
/* Common types */
typedef uint8_t m_uint8_t;
typedef int8_t m_int8_t;

typedef uint16_t m_uint16_t;
typedef int16_t m_int16_t;

typedef uint32_t m_uint32_t;
typedef int32_t m_int32_t;

typedef uint64_t m_uint64_t;
typedef int64_t m_int64_t;

typedef unsigned long m_iptr_t;
typedef m_uint64_t m_tmcnt_t;


#if defined (__x86_64__) || defined(__ia64)
/*function label address*/
typedef m_uint64_t  m_hiptr_t;
#else
typedef m_uint32_t  m_hiptr_t;
#endif

typedef void (*pvoid)(void);

/* MIPS instruction */
typedef m_uint32_t mips_insn_t;

/* True/False definitions */
#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE  1
#endif

//for gdb interface
/* Used for functions which can fail */
enum result_t
{ SUCCESS, FAILURE, STALL, BUSERROR, SCFAILURE };

/* Forward declarations */
typedef struct cpu_mips cpu_mips_t;
typedef struct vm_instance vm_instance_t;
typedef struct vdevice vdevice_t;
typedef struct mips64_jit_tcb mips64_jit_tcb_t;
typedef struct insn_exec_page insn_exec_page_t;

#endif
