/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the Wine project.
 * No warranty is given; refer to the file DISCLAIMER.PD within this package.
 */

#ifndef _INC_INTRIN
#define _INC_INTRIN

#if defined(__i386__) || (defined(__x86_64__) && !defined(__arm64ec__))
# include <x86intrin.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __has_builtin
# define __has_builtin(x) 0
#endif

#if defined(__aarch64__) || defined(__arm64ec__)

unsigned char    __ldar8(const volatile unsigned char *);
unsigned short   __ldar16(const volatile unsigned short *);
unsigned int     __ldar32(const volatile unsigned int *);
unsigned __int64 __ldar64(const volatile unsigned __int64 *);
void __stlr8(volatile unsigned char *, unsigned char);
void __stlr16(volatile unsigned short *, unsigned short);
void __stlr32(volatile unsigned int *, unsigned int);
void __stlr64(volatile unsigned __int64 *, unsigned __int64);

#elif defined(__i386__) || defined(__x86_64__)

#if __has_builtin(__cpuidex) || (defined(_MSC_VER) && !defined(__clang__))
void __cpuidex(int info[4], int ax, int cx);
#pragma intrinsic(__cpuidex)
#else
static inline void __cpuidex(int info[4], int ax, int cx)
{
  __asm__ ("cpuid" : "=a"(info[0]), "=b" (info[1]), "=c"(info[2]), "=d"(info[3]) : "a"(ax), "c"(cx));
}
#endif

#if __has_builtin(__cpuid) || (defined(_MSC_VER) && !defined(__clang__))
void __cpuid(int info[4], int ax);
#pragma intrinsic(__cpuid)
#else
static inline void __cpuid(int info[4], int ax)
{
    return __cpuidex(info, ax, 0);
}
#endif

#endif

#if __has_builtin(__popcnt) || (defined(_MSC_VER) && !defined(__clang__))
unsigned int __popcnt(unsigned int);
#pragma intrinsic(__popcnt)
#else
static inline unsigned int __popcnt(unsigned int i)
{
    return __builtin_popcount( i );
}
#endif

#if __has_builtin(__popcnt16) || (defined(_MSC_VER) && !defined(__clang__))
unsigned short __popcnt16(unsigned short);
#pragma intrinsic(__popcnt16)
#else
static inline unsigned short __popcnt16(unsigned short i)
{
    return __builtin_popcount( i );
}
#endif

#if defined(__x86_64__) || defined(__aarch64__)
#if __has_builtin(__popcnt64) || (defined(_MSC_VER) && !defined(__clang__))
unsigned __int64 __popcnt64(unsigned __int64);
#pragma intrinsic(__popcnt64)
#else
static inline unsigned __int64 __popcnt64(unsigned __int64 i)
{
    return __builtin_popcountll( i );
}
#endif
#endif

#if defined(__aarch64__) || defined(__arm64ec__)
typedef enum _tag_ARM64INTR_BARRIER_TYPE
{
    _ARM64_BARRIER_OSHLD  = 0x1,
    _ARM64_BARRIER_OSHST  = 0x2,
    _ARM64_BARRIER_OSH    = 0x3,
    _ARM64_BARRIER_NSHLD  = 0x5,
    _ARM64_BARRIER_NSHST  = 0x6,
    _ARM64_BARRIER_NSH    = 0x7,
    _ARM64_BARRIER_ISHLD  = 0x9,
    _ARM64_BARRIER_ISHST  = 0xa,
    _ARM64_BARRIER_ISH    = 0xb,
    _ARM64_BARRIER_LD     = 0xd,
    _ARM64_BARRIER_ST     = 0xe,
    _ARM64_BARRIER_SY     = 0xf
} _ARM64INTR_BARRIER_TYPE;
#endif

#ifdef __arm__
typedef enum _tag_ARMINTR_BARRIER_TYPE
{
    _ARM_BARRIER_OSHST  = 0x2,
    _ARM_BARRIER_OSH    = 0x3,
    _ARM_BARRIER_NSHST  = 0x6,
    _ARM_BARRIER_NSH    = 0x7,
    _ARM_BARRIER_ISHST  = 0xa,
    _ARM_BARRIER_ISH    = 0xb,
    _ARM_BARRIER_ST     = 0xe,
    _ARM_BARRIER_SY     = 0xf
} _ARMINTR_BARRIER_TYPE;
#endif

#ifdef _MSC_VER

unsigned char _BitScanForward(unsigned long*,unsigned long);
unsigned char _BitScanReverse(unsigned long*,unsigned long);
void          _ReadBarrier(void);
void          _ReadWriteBarrier(void);
void          _WriteBarrier(void);

long          _InterlockedAnd(long volatile *, long);
char          _InterlockedAnd8(char volatile *, char);
short         _InterlockedAnd16(short volatile *, short);
long          _InterlockedCompareExchange(long volatile *, long, long);
char          _InterlockedCompareExchange8(char volatile *, char, char);
short         _InterlockedCompareExchange16(short volatile *, short, short);
__int64       _InterlockedCompareExchange64(__int64 volatile *, __int64, __int64);
void *        _InterlockedCompareExchangePointer(void * volatile *, void *, void *);
long          _InterlockedDecrement(long volatile *);
short         _InterlockedDecrement16(short volatile *);
long          _InterlockedExchange(long volatile *, long);
char          _InterlockedExchange8(char volatile *, char);
short         _InterlockedExchange16(short volatile *, short);
long          _InterlockedExchangeAdd(long volatile *, long);
char          _InterlockedExchangeAdd8(char volatile *, char);
short         _InterlockedExchangeAdd16(short volatile *, short);
void *        _InterlockedExchangePointer(void * volatile *, void *);
long          _InterlockedIncrement(long volatile *);
short         _InterlockedIncrement16(short volatile *);
long          _InterlockedOr(long volatile *, long);
char          _InterlockedOr8(char volatile *, char);
short         _InterlockedOr16(short volatile *, short);
long          _InterlockedXor(long volatile *, long);
char          _InterlockedXor8(char volatile *, char);
short         _InterlockedXor16(short volatile *, short);

#ifndef __i386__
__int64       _InterlockedAnd64(__int64 volatile *, __int64);
__int64       _InterlockedDecrement64(__int64 volatile *);
__int64       _InterlockedExchange64(__int64 volatile *, __int64);
__int64       _InterlockedExchangeAdd64(__int64 volatile *, __int64);
__int64       _InterlockedIncrement64(__int64 volatile *);
__int64       _InterlockedOr64(__int64 volatile *, __int64);
__int64       _InterlockedXor64(__int64 volatile *, __int64);
#endif

#if defined(__arm__) || defined(__aarch64__) || defined(__arm64ec__)
long          _InterlockedAdd(long volatile *, long);
long          _InterlockedAdd_acq(long volatile *, long);
long          _InterlockedAdd_nf(long volatile *, long);
long          _InterlockedAdd_rel(long volatile *, long);
__int64       _InterlockedAdd64(__int64 volatile *, __int64);
__int64       _InterlockedAdd64_acq(__int64 volatile *, __int64);
__int64       _InterlockedAdd64_nf(__int64 volatile *, __int64);
__int64       _InterlockedAdd64_rel(__int64 volatile *, __int64);
long          _InterlockedAnd_acq(long volatile *, long);
long          _InterlockedAnd_nf(long volatile *, long);
long          _InterlockedAnd_rel(long volatile *, long);
char          _InterlockedAnd8_acq(char volatile *, char);
char          _InterlockedAnd8_nf(char volatile *, char);
char          _InterlockedAnd8_rel(char volatile *, char);
short         _InterlockedAnd16_acq(short volatile *, short);
short         _InterlockedAnd16_nf(short volatile *, short);
short         _InterlockedAnd16_rel(short volatile *, short);
__int64       _InterlockedAnd64_acq(__int64 volatile *, __int64);
__int64       _InterlockedAnd64_nf(__int64 volatile *, __int64);
__int64       _InterlockedAnd64_rel(__int64 volatile *, __int64);
long          _InterlockedCompareExchange_acq(long volatile *, long, long);
long          _InterlockedCompareExchange_nf(long volatile *, long, long);
long          _InterlockedCompareExchange_rel(long volatile *, long, long);
char          _InterlockedCompareExchange8_acq(char volatile *, char, char);
char          _InterlockedCompareExchange8_nf(char volatile *, char, char);
char          _InterlockedCompareExchange8_rel(char volatile *, char, char);
short         _InterlockedCompareExchange16_acq(short volatile *, short, short);
short         _InterlockedCompareExchange16_nf(short volatile *, short, short);
short         _InterlockedCompareExchange16_rel(short volatile *, short, short);
__int64       _InterlockedCompareExchange64_acq(__int64 volatile *, __int64, __int64);
__int64       _InterlockedCompareExchange64_nf(__int64 volatile *, __int64, __int64);
__int64       _InterlockedCompareExchange64_rel(__int64 volatile *, __int64, __int64);
void *        _InterlockedCompareExchangePointer_acq(void * volatile *, void *, void *);
void *        _InterlockedCompareExchangePointer_nf(void * volatile *, void *, void *);
void *        _InterlockedCompareExchangePointer_rel(void * volatile *, void *, void *);
long          _InterlockedDecrement_acq(long volatile *);
long          _InterlockedDecrement_nf(long volatile *);
long          _InterlockedDecrement_rel(long volatile *);
short         _InterlockedDecrement16_acq(short volatile *);
short         _InterlockedDecrement16_nf(short volatile *);
short         _InterlockedDecrement16_rel(short volatile *);
__int64       _InterlockedDecrement64_acq(__int64 volatile *);
__int64       _InterlockedDecrement64_nf(__int64 volatile *);
__int64       _InterlockedDecrement64_rel(__int64 volatile *);
long          _InterlockedExchange_acq(long volatile *, long);
long          _InterlockedExchange_nf(long volatile *, long);
long          _InterlockedExchange_rel(long volatile *, long);
char          _InterlockedExchange8_acq(char volatile *, char);
char          _InterlockedExchange8_nf(char volatile *, char);
char          _InterlockedExchange8_rel(char volatile *, char);
short         _InterlockedExchange16_acq(short volatile *, short);
short         _InterlockedExchange16_nf(short volatile *, short);
short         _InterlockedExchange16_rel(short volatile *, short);
__int64       _InterlockedExchange64_acq(__int64 volatile *, __int64);
__int64       _InterlockedExchange64_nf(__int64 volatile *, __int64);
__int64       _InterlockedExchange64_rel(__int64 volatile *, __int64);
void *        _InterlockedExchangePointer_acq(void * volatile *, void *);
void *        _InterlockedExchangePointer_nf(void * volatile *, void *);
void *        _InterlockedExchangePointer_rel(void * volatile *, void *);
long          _InterlockedExchangeAdd_acq(long volatile *, long);
long          _InterlockedExchangeAdd_nf(long volatile *, long);
long          _InterlockedExchangeAdd_rel(long volatile *, long);
char          _InterlockedExchangeAdd8_acq(char volatile *, char);
char          _InterlockedExchangeAdd8_nf(char volatile *, char);
char          _InterlockedExchangeAdd8_rel(char volatile *, char);
short         _InterlockedExchangeAdd16_acq(short volatile *, short);
short         _InterlockedExchangeAdd16_nf(short volatile *, short);
short         _InterlockedExchangeAdd16_rel(short volatile *, short);
__int64       _InterlockedExchangeAdd64_acq(__int64 volatile *, __int64);
__int64       _InterlockedExchangeAdd64_nf(__int64 volatile *, __int64);
__int64       _InterlockedExchangeAdd64_rel(__int64 volatile *, __int64);
long          _InterlockedIncrement_acq(long volatile *);
long          _InterlockedIncrement_nf(long volatile *);
long          _InterlockedIncrement_rel(long volatile *);
short         _InterlockedIncrement16_acq(short volatile *);
short         _InterlockedIncrement16_nf(short volatile *);
short         _InterlockedIncrement16_rel(short volatile *);
__int64       _InterlockedIncrement64_acq(__int64 volatile *);
__int64       _InterlockedIncrement64_nf(__int64 volatile *);
__int64       _InterlockedIncrement64_rel(__int64 volatile *);
long          _InterlockedOr_acq(long volatile *, long);
long          _InterlockedOr_nf(long volatile *, long);
long          _InterlockedOr_rel(long volatile *, long);
char          _InterlockedOr8_acq(char volatile *, char);
char          _InterlockedOr8_nf(char volatile *, char);
char          _InterlockedOr8_rel(char volatile *, char);
short         _InterlockedOr16_acq(short volatile *, short);
short         _InterlockedOr16_nf(short volatile *, short);
short         _InterlockedOr16_rel(short volatile *, short);
__int64       _InterlockedOr64_acq(__int64 volatile *, __int64);
__int64       _InterlockedOr64_nf(__int64 volatile *, __int64);
__int64       _InterlockedOr64_rel(__int64 volatile *, __int64);
long          _InterlockedXor_acq(long volatile *, long);
long          _InterlockedXor_nf(long volatile *, long);
long          _InterlockedXor_rel(long volatile *, long);
char          _InterlockedXor8_acq(char volatile *, char);
char          _InterlockedXor8_nf(char volatile *, char);
char          _InterlockedXor8_rel(char volatile *, char);
short         _InterlockedXor16_acq(short volatile *, short);
short         _InterlockedXor16_nf(short volatile *, short);
short         _InterlockedXor16_rel(short volatile *, short);
__int64       _InterlockedXor64_acq(__int64 volatile *, __int64);
__int64       _InterlockedXor64_nf(__int64 volatile *, __int64);
__int64       _InterlockedXor64_rel(__int64 volatile *, __int64);

void __dmb(unsigned int);
#pragma intrinsic(__dmb)
#endif

#if defined(__x86_64__) || defined(__aarch64__)
unsigned char _BitScanForward64(unsigned long*,unsigned __int64);
unsigned char _BitScanReverse64(unsigned long*,unsigned __int64);
unsigned char _InterlockedCompareExchange128(__int64 volatile * , __int64, __int64, __int64 *);
#endif

#if defined(__aarch64__) || defined(__arm64ec__)
unsigned char _InterlockedCompareExchange128_acq(__int64 volatile *, __int64, __int64, __int64 *);
unsigned char _InterlockedCompareExchange128_nf(__int64 volatile *, __int64, __int64, __int64 *);
unsigned char _InterlockedCompareExchange128_rel(__int64 volatile *, __int64, __int64, __int64 *);

unsigned __int64 __getReg(int);
#pragma intrinsic(__getReg)
#endif

#if defined(__x86_64__) && !defined(__arm64ec__)
long          _InterlockedAnd_np(long volatile *, long);
char          _InterlockedAnd8_np(char volatile *, char);
short         _InterlockedAnd16_np(short volatile *, short);
__int64       _InterlockedAnd64_np(__int64 volatile *, __int64);
long          _InterlockedCompareExchange_np(long volatile *, long, long);
short         _InterlockedCompareExchange16_np(short volatile *, short, short);
__int64       _InterlockedCompareExchange64_np(__int64 volatile *, __int64, __int64);
unsigned char _InterlockedCompareExchange128_np(__int64 volatile *, __int64, __int64, __int64 *);
void *        _InterlockedCompareExchangePointer_np(void * volatile *, void *, void *);
long          _InterlockedOr_np(long volatile *, long);
char          _InterlockedOr8_np(char volatile *, char);
short         _InterlockedOr16_np(short volatile *, short);
__int64       _InterlockedOr64_np(__int64 volatile *, __int64);
long          _InterlockedXor_np(long volatile *, long);
char          _InterlockedXor8_np(char volatile *, char);
short         _InterlockedXor16_np(short volatile *, short);
__int64       _InterlockedXor64_np(__int64 volatile *, __int64);

unsigned __int64 __shiftright128(unsigned __int64, unsigned __int64, unsigned char);
unsigned __int64 _umul128(unsigned __int64, unsigned __int64, unsigned __int64*);
#endif

#endif  /* _MSC_VER */

#ifdef __cplusplus
}
#endif

#endif /* _INC_INTRIN */
