/*
	mpg123lib_intern: Common non-public stuff for libmpg123

	copyright 1995-2023 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org

	derived from the old mpg123.h
*/

#ifndef MPG123_H_INTERN
#define MPG123_H_INTERN

#define MPG123_RATES 9
#define MPG123_ENCODINGS 12

#include "config.h" /* Load this before _anything_ */

#include "../common/abi_align.h"

#include "../compat/compat.h"

// Only portable API plays a role in the library itself, outside of lfs_wrap.c.
// Also, we need to ensure no suffix renaming for the primary implementations.
#define MPG123_PORTABLE_API
#define MPG123_ENUM_API
#include "mpg123.h"

#define SKIP_JUNK 1

#ifndef M_PI
# define M_PI       3.14159265358979323846
#endif
#ifndef M_SQRT2
# define M_SQRT2	1.41421356237309504880
#endif

#ifdef SUNOS
#define memmove(dst,src,size) bcopy(src,dst,size)
#endif

// The real type is either 32 bit float or integer. We do not attempt
// double precision for MPEG audio decoding (or any audio storage/compression
// format, even).
// The calctables tool works in double precision float, but converts to fixed
// point 'real' on output in addition to floating point.

// The base type used in the computation code.
// With C11 we  could rely on tgmath for sin() to use the right precision.
#ifdef CALCTABLES
#	define clreal double
#	define COS    cos
#	define SIN    sin
#	define TAN    tan
#	define POW    pow
#else
#	define clreal float
#	define COS    cosf
#	define SIN    sinf
#	define TAN    tanf
#	define POW    powf
#endif

#if defined(REAL_IS_FLOAT) && !defined(CALCTABLES)

#define real float

#elif defined(REAL_IS_FIXED) || defined(CALCTABLES)

#ifdef RUNTIME_TABLES
#error "Runtime tables are only for floating point decoders."
#endif

# define real  int32_t
# define dreal int64_t

/*
  for fixed-point decoders, use pre-calculated tables to avoid expensive floating-point maths
  undef this macro for run-time calculation
*/

# define REAL_RADIX				24
# define REAL_FACTOR			16777216.0

static inline int32_t double_to_long_rounded(double x, double scalefac)
{
	x *= scalefac;
	x += (x > 0) ? 0.5 : -0.5;
	return (int32_t)x;
}

static inline int32_t scale_rounded(int32_t x, int shift)
{
	x += (x >> 31);
	x >>= (shift - 1);
	x += (x & 1);
	return (x >> 1);
}

# ifdef __GNUC__
#  if defined(OPT_I386)
/* for i386_nofpu decoder */
#   define REAL_MUL_ASM(x, y, radix) \
({ \
	long _x=(x), _y=(y); \
	__asm__ ( \
		"imull %1 \n\t" \
		"shrdl %2, %%edx, %0 \n\t" \
		: "+&a" (_x) \
		: "mr" (_y), "I" (radix) \
		: "%edx", "cc" \
	); \
	_x; \
})

#   define REAL_MUL_SCALE_LAYER3_ASM(x, y, radix) \
({ \
	long _x=(x), _y=(y), _radix=(radix); \
	__asm__ ( \
		"imull %1 \n\t" \
		"shrdl %%cl, %%edx, %0 \n\t" \
		: "+&a" (_x) \
		: "mr" (_y), "c" (_radix) \
		: "%edx", "cc" \
	); \
	_x; \
})
#  elif defined(OPT_PPC)
/* for powerpc */
#   define REAL_MUL_ASM(x, y, radix) \
({ \
	long _x=(x), _y=(y), _mull, _mulh; \
	__asm__ ( \
		"mullw %0, %2, %3 \n\t" \
		"mulhw %1, %2, %3 \n\t" \
		"srwi %0, %0, %4 \n\t" \
		"rlwimi %0, %1, %5, 0, %6 \n\t" \
		: "=&r" (_mull), "=&r" (_mulh) \
		: "r" (_x), "r" (_y), "i" (radix), "i" (32-(radix)), "i" ((radix)-1) \
	); \
	_mull; \
})

#   define REAL_MUL_SCALE_LAYER3_ASM(x, y, radix) \
({ \
	long _x=(x), _y=(y), _radix=(radix), _mull, _mulh, _radix2; \
	__asm__ ( \
		"mullw %0, %3, %4 \n\t" \
		"mulhw %1, %3, %4 \n\t" \
		"subfic %2, %5, 32 \n\t" \
		"srw %0, %0, %5 \n\t" \
		"slw %1, %1, %2 \n\t" \
		"or %0, %0, %1 \n\t" \
		: "=&r" (_mull), "=&r" (_mulh), "=&r" (_radix2) \
		: "r" (_x), "r" (_y), "r" (_radix) \
		: "cc" \
	); \
	_mull; \
})
#  elif defined(OPT_ARM)
/* for arm */
#   define REAL_MUL_ASM(x, y, radix) \
({ \
	long _x=(x), _y=(y), _mull, _mulh; \
	__asm__ ( \
		"smull %0, %1, %2, %3 \n\t" \
		"mov %0, %0, lsr %4 \n\t" \
		"orr %0, %0, %1, lsl %5 \n\t" \
		: "=&r" (_mull), "=&r" (_mulh) \
		: "r" (_x), "r" (_y), "M" (radix), "M" (32-(radix)) \
	); \
	_mull; \
})

#   define REAL_MUL_SCALE_LAYER3_ASM(x, y, radix) \
({ \
	long _x=(x), _y=(y), _radix=(radix), _mull, _mulh, _radix2; \
	__asm__ ( \
		"smull %0, %1, %3, %4 \n\t" \
		"mov %0, %0, lsr %5 \n\t" \
		"rsb %2, %5, #32 \n\t" \
		"mov %1, %1, lsl %2 \n\t" \
		"orr %0, %0, %1 \n\t" \
		: "=&r" (_mull), "=&r" (_mulh), "=&r" (_radix2) \
		: "r" (_x), "r" (_y), "r" (_radix) \
	); \
	_mull; \
})
#  endif
# endif

/* I just changed the (int) to (real) there... seemed right. */
# define DOUBLE_TO_REAL(x)					(double_to_long_rounded(x, REAL_FACTOR))
# define SCALE_15								32768.0
# define DOUBLE_TO_REAL_15(x)				(double_to_long_rounded(x, SCALE_15))
# define SCALE_POW43							8192.0
# define DOUBLE_TO_REAL_POW43(x)			(double_to_long_rounded(x, SCALE_POW43))
# define SCALE_LAYER12                  1073741824.0
# define DOUBLE_TO_REAL_SCALE_LAYER12(x)	(double_to_long_rounded(x, SCALE_LAYER12))
# define DOUBLE_TO_REAL_SCALE_LAYER3(x, y)	(double_to_long_rounded(x, pow(2.0,gainpow2_scale[y])))
# define REAL_TO_DOUBLE(x)					((double)(x) / REAL_FACTOR)
# ifdef REAL_MUL_ASM
#  define REAL_MUL(x, y)					REAL_MUL_ASM(x, y, REAL_RADIX)
#  define REAL_MUL_15(x, y)					REAL_MUL_ASM(x, y, 15)
#  define REAL_MUL_SCALE_LAYER12(x, y)		REAL_MUL_ASM(x, y, 15 + 30 - REAL_RADIX)
# else
#  define REAL_MUL(x, y)					(((dreal)(x) * (dreal)(y)) >> REAL_RADIX)
#  define REAL_MUL_15(x, y)					(((dreal)(x) * (dreal)(y)) >> 15)
#  define REAL_MUL_SCALE_LAYER12(x, y)		(((dreal)(x) * (dreal)(y)) >> (15 + 30 - REAL_RADIX))
# endif
# ifdef REAL_MUL_SCALE_LAYER3_ASM
#  define REAL_MUL_SCALE_LAYER3(x, y, z)	REAL_MUL_SCALE_LAYER3_ASM(x, y, 13 + gainpow2_scale[z] - REAL_RADIX)
# else
#  define REAL_MUL_SCALE_LAYER3(x, y, z)	(((dreal)(x) * (dreal)(y)) >> (13 + gainpow2_scale[z] - REAL_RADIX))
# endif
# define REAL_SCALE_LAYER12(x)				((real)((x) >> (30 - REAL_RADIX)))
# define REAL_SCALE_LAYER3(x, y)			((real)((x) >> (gainpow2_scale[y] - REAL_RADIX)))
# ifdef ACCURATE_ROUNDING
#  define REAL_MUL_SYNTH(x, y)				REAL_MUL(x, y)
#  define REAL_SCALE_DCT64(x)				(x)
#  define REAL_SCALE_WINDOW(x)				(x)
# else
#  define REAL_MUL_SYNTH(x, y)				((x) * (y))
#  define REAL_SCALE_DCT64(x)				((x) >> 8)
#  define REAL_SCALE_WINDOW(x)				scale_rounded(x, 16)
# endif

#else

#error "Simple float or fixed-point, nothing else makes sense."

#endif

#ifndef DOUBLE_TO_REAL
# define DOUBLE_TO_REAL(x)					(real)(x)
#endif
#ifndef DOUBLE_TO_REAL_15
# define DOUBLE_TO_REAL_15(x)				(real)(x)
#endif
#ifndef DOUBLE_TO_REAL_POW43
# define DOUBLE_TO_REAL_POW43(x)			(real)(x)
#endif
#ifndef DOUBLE_TO_REAL_SCALE_LAYER12
# define DOUBLE_TO_REAL_SCALE_LAYER12(x)	(real)(x)
#endif
#ifndef DOUBLE_TO_REAL_SCALE_LAYER3
# define DOUBLE_TO_REAL_SCALE_LAYER3(x, y)	(real)(x)
#endif
#ifndef REAL_TO_DOUBLE
# define REAL_TO_DOUBLE(x)					(x)
#endif

#ifndef REAL_MUL
# define REAL_MUL(x, y)						((x) * (y))
#endif
#ifndef REAL_MUL_SYNTH
# define REAL_MUL_SYNTH(x, y)				((x) * (y))
#endif
#ifndef REAL_MUL_15
# define REAL_MUL_15(x, y)					((x) * (y))
#endif
#ifndef REAL_MUL_SCALE_LAYER12
# define REAL_MUL_SCALE_LAYER12(x, y)		((x) * (y))
#endif
#ifndef REAL_MUL_SCALE_LAYER3
# define REAL_MUL_SCALE_LAYER3(x, y, z)		((x) * (y))
#endif
#ifndef REAL_SCALE_LAYER12
# define REAL_SCALE_LAYER12(x)				(x)
#endif
#ifndef REAL_SCALE_LAYER3
# define REAL_SCALE_LAYER3(x, y)			(x)
#endif
#ifndef REAL_SCALE_DCT64
# define REAL_SCALE_DCT64(x)				(x)
#endif

/* used to be: AUDIOBUFSIZE = n*64 with n=1,2,3 ...
   now: factor on minimum frame buffer size (which takes upsampling into account) */
#define		AUDIOBUFSIZE		2

#include "../common/true.h"

#define         MAX_NAME_SIZE           81
#define         SBLIMIT                 32
#define         SCALE_BLOCK             12
#define         SSLIMIT                 18

/* Same as MPG_M_* */
#define         MPG_MD_STEREO           0
#define         MPG_MD_JOINT_STEREO     1
#define         MPG_MD_DUAL_CHANNEL     2
#define         MPG_MD_MONO             3

/* We support short or float output samples...
   Short integer amplitude is scaled by this. */
#define SHORT_SCALE 32768
/* That scales a short-scaled value to a 32bit integer scaled one
   value = 2**31/2**15 */
#define S32_RESCALE 65536

/* Pre Shift fo 16 to 8 bit converter table */
#define AUSHIFT (3)

#include "optimize.h"
#include "decode.h"
#include "parse.h"
#include "frame.h"

/* fr is a mpg123_handle* by convention here... */
#define NOQUIET  (!(fr->p.flags & MPG123_QUIET))
#define VERBOSE  (NOQUIET && fr->p.verbose)
#define VERBOSE2 (NOQUIET && fr->p.verbose > 1)
#define VERBOSE3 (NOQUIET && fr->p.verbose > 2)
#define VERBOSE4 (NOQUIET && fr->p.verbose > 3)
#define PVERB(mp, level) (!((mp)->flags & MPG123_QUIET) && (mp)->verbose >= (level))

int INT123_decode_update(mpg123_handle *mh);
/* residing in format.c  */
int64_t INT123_decoder_synth_bytes(mpg123_handle *fr , int64_t s);
int64_t INT123_samples_to_bytes(mpg123_handle *fr , int64_t s);
int64_t INT123_bytes_to_samples(mpg123_handle *fr , int64_t b);
int64_t INT123_outblock_bytes(mpg123_handle *fr, int64_t s);
/* Postprocessing format conversion of freshly decoded buffer. */
void INT123_postprocess_buffer(mpg123_handle *fr);

int INT123_open_fixed_pre(mpg123_handle *mh, int channels, int encoding);
int INT123_open_fixed_post(mpg123_handle *mh, int channels, int encoding);

/* If networking is enabled and we really mean internal networking, the timeout_read function is available. */
#if defined (NETWORK) && !defined (WANT_WIN32_SOCKETS)
/* Does not work with win32 */
#define TIMEOUT_READ
#endif

// Change a given linear factor by the given dB value, bounded
// to +/- 60 dB.
static inline double dbchange(double base_factor, double db)
{
	double nscale = base_factor * pow(10, db/20);
	if(nscale < 0.001) // -60 dB
		nscale = 0.001;
	if(nscale > 1000)
		nscale = 1000; // +60 dB
	return nscale;
}

#endif
