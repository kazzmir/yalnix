/*--------------------------------------------------------------------
This source distribution is placed in the public domain by its author,
Jason Papadopoulos. You may use it for any purpose, free of charge,
without having to notify anyone. I disclaim any responsibility for any
errors.

Optionally, please be nice and tell me if you find this source to be
useful. Again optionally, if you add to the functionality present here
please consider making those additions public too, so that others may 
benefit from your work.	
       				   --jasonp@boo.net 10/3/07
--------------------------------------------------------------------*/

#ifndef _UTIL_H_
#define _UTIL_H_

/* system-specific stuff ---------------------------------------*/

#ifdef WIN32

	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#include <process.h>

#else /* !WIN32 */

	#include <sys/types.h>
	#include <sys/stat.h>
	#include <fcntl.h>
	#include <unistd.h>
	#include <errno.h>
	#include <pthread.h>

#endif /* WIN32 */

/* system-independent header files ------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <time.h>
#ifndef _MSC_VER
#include <inttypes.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* basic types  -------------------------------------------------------*/

#ifdef _MSC_VER

	typedef __int8 int8;
	typedef __int16 int16;
	typedef __int32 int32;
	typedef __int64 int64;
	typedef unsigned __int8 uint8;
	typedef unsigned __int16 uint16;
	typedef unsigned __int32 uint32;
	typedef unsigned __int64 uint64;

	/* portable 64-bit formatting */
	#define PRId64 "I64d"
	#define PRIu64 "I64u"
	#define PRIx64 "I64x"

#else
	typedef unsigned char uint8;
	typedef unsigned short uint16;
	typedef uint32_t uint32;
	typedef uint64_t uint64;
	
	#ifndef RS6K
	typedef char int8;
	typedef short int16;
	typedef int32_t int32;
	typedef int64_t int64;
	#endif
#endif

#ifdef WIN32
	typedef HANDLE thread_id_t;
#else
	typedef pthread_t thread_id_t;
#endif

/* useful functions ---------------------------------------------------*/

void * aligned_malloc(uint32 len, uint32 align);
void aligned_free(void *newptr);
uint64 read_clock(void);

#define MIN(a,b) ((a) < (b)? (a) : (b))
#define MAX(a,b) ((a) > (b)? (a) : (b))

#if defined(_MSC_VER)
	#define INLINE __inline
	#define getpid _getpid
#elif !defined(RS6K)
	#define INLINE inline
#else
	#define INLINE /* nothing */
#endif

#if defined(__GNUC__) && __GNUC__ >= 3
	#define PREFETCH(addr) __builtin_prefetch(addr) 
#elif defined(_MSC_VER) && _MSC_VER >= 1400
	#define PREFETCH(addr) PreFetchCacheLine(PF_TEMPORAL_LEVEL_1, addr)
#else
	#define PREFETCH(addr) /* nothing */
#endif

#ifndef M_LN2
#define M_LN2		0.69314718055994530942
#endif

static INLINE uint32 
get_rand(uint32 *rand_seed, uint32 *rand_carry) {
   
	/* A multiply-with-carry generator by George Marsaglia.
	   The period is about 2^63. */

	#define RAND_MULT 2131995753

	uint64 temp;

	temp = (uint64)(*rand_seed) * 
		       (uint64)RAND_MULT + 
		       (uint64)(*rand_carry);
	*rand_seed = (uint32)temp;
	*rand_carry = (uint32)(temp >> 32);
	return (uint32)temp;
}

/* for turning on CPU-specific code */

enum cpu_type {
	cpu_generic,
	cpu_pentium,
	cpu_pentium2,
	cpu_pentium3,
	cpu_pentium4,
	cpu_pentium_m,
	cpu_core,
	cpu_athlon,
	cpu_athlon_xp,
	cpu_opteron
};

void get_cache_sizes(uint32 *level1_cache, uint32 *level2_cache);
enum cpu_type get_cpu_type(void);

/* CPU-specific capabilities */

/* assume for all CPUs, even non-x86 CPUs. These guard
   assembly language that has other guards anyway, and
   the only CPU that doesn't have these instructions is
   the classic Pentium */

#define HAS_CMOV
// #define HAS_MMX

#if defined(CPU_GENERIC)
	#define MANUAL_PREFETCH
	#if !defined(WIN32) && !defined(__i386__)
		#define HAS_MANY_REGISTERS
	#endif

#elif defined(CPU_PENTIUM2) 
	#define MANUAL_PREFETCH

#elif defined(CPU_ATHLON)
	#define MANUAL_PREFETCH
	#define HAS_AMD_MMX

#elif defined(CPU_PENTIUM3) 
	#define MANUAL_PREFETCH
	#define HAS_SSE

#elif defined(CPU_ATHLON_XP)
	#define HAS_SSE

#elif defined(CPU_PENTIUM4) || defined(CPU_PENTIUM_M) || \
	defined(CPU_CORE) || defined(CPU_OPTERON)
	#define HAS_SSE
	#define HAS_SSE2
	#if !defined(WIN32) && !defined(__i386__)
		#define HAS_MANY_REGISTERS
	#endif
#endif


#ifdef __cplusplus
}
#endif

#endif /* _UTIL_H_ */
