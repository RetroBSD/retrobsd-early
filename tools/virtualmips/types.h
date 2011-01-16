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
#if defined (__x86_64__) || defined(__ia64) || defined(__s390x__) || defined(__alpha__) 
typedef unsigned long uint64_t;
#else
typedef unsigned long long uint64_t;
#endif
#endif

/* if Solaris/__sun__, don't typedef int8_t, as it will be typedef'd
   prior to this and will cause an error in compliation, conflicting
   with /usr/include/sys/int_types.h, line 75 */
#ifndef __sun__
typedef signed char int8_t;
#endif
typedef signed short int16_t;
typedef signed int int32_t;
// Linux/Sparc64 defines int64_t
#if !(defined (__sparc_v9__) && defined(__linux__))
#if defined (__x86_64__) || defined(__ia64) || defined(__s390x__) || defined(__alpha__)
typedef signed long int64_t;
#else
typedef signed long long int64_t;
#endif
#endif





/*used in dynamips. so just typedef again*/
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


#endif
