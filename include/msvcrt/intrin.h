/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the Wine project.
 * No warranty is given; refer to the file DISCLAIMER.PD within this package.
 */

#ifndef _INC_INTRIN
#define _INC_INTRIN

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__i386__) || defined(__x86_64__)
static inline void __cpuidex(int info[4], int ax, int cx)
{
  __asm__ ("cpuid" : "=a"(info[0]), "=b" (info[1]), "=c"(info[2]), "=d"(info[3]) : "a"(ax), "c"(cx));
}
static inline void __cpuid(int info[4], int ax)
{
    return __cpuidex(info, ax, 0);
}
#endif

#ifdef __cplusplus
}
#endif

#endif /* _INC_INTRIN */
