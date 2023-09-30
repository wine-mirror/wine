/*
	getcpucpuflags: get cpuflags for ia32

	copyright ?-2007 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http:#mpg123.org
	initially written by KIMURA Takuhiro (for 3DNow!)
	extended for general use by Thomas Orgis
*/

#ifndef MPG123_H_GETCPUFLAGS
#define MPG123_H_GETCPUFLAGS

#include "config.h"

/* standard level flags part 1 (ECX)*/
#define FLAG_SSE3      0x00000001
#define FLAG_SSSE3     0x00000200
#define FLAG_AVX       0x1C000000
/* standard level flags part 2 (EDX) */
#define FLAG2_MMX       0x00800000
#define FLAG2_SSE       0x02000000
#define FLAG2_SSE2      0x04000000
#define FLAG2_FPU       0x00000001
/* cpuid extended level 1 (AMD) */
#define XFLAG_MMX      0x00800000
#define XFLAG_3DNOW    0x80000000
#define XFLAG_3DNOWEXT 0x40000000
/* eXtended Control Register 0 */
#define XCR0FLAG_AVX   0x00000006


struct cpuflags
{
#if defined(OPT_ARM) || defined(OPT_NEON) || defined(OPT_NEON64)
	unsigned int has_neon;
#else
	unsigned int id;
	unsigned int std;
	unsigned int std2;
	unsigned int ext;
	unsigned int xcr0_lo;
#endif
};

unsigned int INT123_getcpuflags(struct cpuflags* cf);

#if ((defined OPT_X86) || (defined OPT_X86_64) || (defined OPT_NEON) || (defined OPT_NEON64)) && (defined OPT_MULTI)

#ifdef WANT_GETCPUFLAGS
#include <string.h>
// Wrapper needed for ignorant clang memory sanitizer that chokes
// because it does not know the assembly code intialized the vlaues.
static unsigned int wrap_getcpuflags(struct cpuflags* cf)
{
	memset(cf, 0, sizeof(*cf));
	return INT123_getcpuflags(cf);
}
#endif

// We really evaluate the CPU flags.
#define OPT_CPU_FLAGS

/* checks the family */
#define cpu_i586(s) ( ((s.id & 0xf00)>>8) == 0 || ((s.id & 0xf00)>>8) > 4 )
/* checking some flags... */
#define cpu_fpu(s) (FLAG2_FPU & s.std2)
#define cpu_mmx(s) (FLAG2_MMX & s.std2 || XFLAG_MMX & s.ext)
#define cpu_3dnow(s) (XFLAG_3DNOW & s.ext)
#define cpu_3dnowext(s) (XFLAG_3DNOWEXT & s.ext)
#define cpu_sse(s) (FLAG2_SSE & s.std2)
#define cpu_sse2(s) (FLAG2_SSE2 & s.std2)
#define cpu_sse3(s) (FLAG_SSE3 & s.std)
#define cpu_avx(s) ((FLAG_AVX & s.std) == FLAG_AVX && (XCR0FLAG_AVX & s.xcr0_lo) == XCR0FLAG_AVX)
#define cpu_fast_sse(s) ((((s.id & 0xf00)>>8) == 6 && FLAG_SSSE3 & s.std) /* for Intel/VIA; family 6 CPUs with SSSE3 */ || \
						   (((s.id & 0xf00)>>8) == 0xf && (((s.id & 0x0ff00000)>>20) > 0 && ((s.id & 0x0ff00000)>>20) != 5))) /* for AMD; family > 0xF CPUs except Bobcat */
#define cpu_neon(s) (s.has_neon)

#else

/* Faking stuff for non-multi builds. The same code for synth function choice is used.
   Just no runtime dependency of result... */
#define cpu_flags nothing
#define cpu_i586(s)     1
#define cpu_fpu(s)      1
#define cpu_mmx(s)      1
#define cpu_3dnow(s)    1
#define cpu_3dnowext(s) 1
#define cpu_sse(s)      1
#define cpu_sse2(s)     1
#define cpu_sse3(s)     1
#define cpu_avx(s)      1
#define cpu_neon(s)     1

#endif


#endif
