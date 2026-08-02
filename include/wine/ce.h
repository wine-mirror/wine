/*
 * WineCE definitions
 *
 * Copyright 2014 André Hentschel
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_WINE_CE_H
#define __WINE_WINE_CE_H

#ifndef __WINE_CONFIG_H
# error You must include config.h to use this header
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define SPFIX "bic SP, SP, #7\n\t"

extern long wrap0( void* );
__ASM_GLOBAL_FUNC( wrap0,
                   "push {r4, LR}\n\t"
                   __ASM_CFI(".cfi_def_cfa_offset 8\n\t")
                   __ASM_CFI(".cfi_offset r4, -8\n\t")
                   __ASM_CFI(".cfi_offset lr, -4\n\t")
                   "mov r4, sp\n\t"
                   __ASM_CFI(".cfi_def_cfa_register r4\n\t")
                   SPFIX
                   "blx r0\n\t"
                   "mov sp, r4\n\t"
                   __ASM_CFI(".cfi_def_cfa_register sp\n\t")
                   "pop {r4, PC}\n\t"
                   __ASM_CFI(".cfi_def_cfa_offset -8\n\t") )
#define FIXFUNC0(dll,func1,func2)                                       \
    long WINAPI WINECE_##func1(void) {                                       \
    static long (WINAPI *p##func2)(void); TRACE("\n");                  \
    if (!p##func2) p##func2 = (void *)GetProcAddress(h##dll,#func2);    \
    return wrap0(p##func2); }

extern long wrap1( long, void*);
__ASM_GLOBAL_FUNC( wrap1,
                   "push {r4, LR}\n\t"
                   __ASM_CFI(".cfi_def_cfa_offset 8\n\t")
                   __ASM_CFI(".cfi_offset r4, -8\n\t")
                   __ASM_CFI(".cfi_offset lr, -4\n\t")
                   "mov r4, sp\n\t"
                   __ASM_CFI(".cfi_def_cfa_register r4\n\t")
                   SPFIX
                   "blx r1\n\t"
                   "mov sp, r4\n\t"
                   __ASM_CFI(".cfi_def_cfa_register sp\n\t")
                   "pop {r4, PC}\n\t"
                   __ASM_CFI(".cfi_def_cfa_offset -8\n\t") )
#define FIXFUNC1(dll,func1,func2)                                       \
    long WINAPI WINECE_##func1(long a) {                                     \
    static long (WINAPI *p##func2)(long); TRACE("\n");                  \
    if (!p##func2) p##func2 = (void *)GetProcAddress(h##dll,#func2);    \
    return wrap1(a,p##func2); }

extern long wrap2( long,long, void*);
__ASM_GLOBAL_FUNC( wrap2,
                   "push {r4, LR}\n\t"
                   __ASM_CFI(".cfi_def_cfa_offset 8\n\t")
                   __ASM_CFI(".cfi_offset r4, -8\n\t")
                   __ASM_CFI(".cfi_offset lr, -4\n\t")
                   "mov r4, sp\n\t"
                   __ASM_CFI(".cfi_def_cfa_register r4\n\t")
                   SPFIX
                   "blx r2\n\t"
                   "mov sp, r4\n\t"
                   __ASM_CFI(".cfi_def_cfa_register sp\n\t")
                   "pop {r4, PC}\n\t"
                   __ASM_CFI(".cfi_def_cfa_offset -8\n\t") )
#define FIXFUNC2(dll,func1,func2)                                       \
    long WINAPI WINECE_##func1(long a,long b) {                              \
    static long (WINAPI *p##func2)(long,long); TRACE("\n");             \
    if (!p##func2) p##func2 = (void *)GetProcAddress(h##dll,#func2);    \
    return wrap2(a,b,p##func2); }

extern long wrap3( long,long,long, void*);
__ASM_GLOBAL_FUNC( wrap3,
                   "push {r4, LR}\n\t"
                   __ASM_CFI(".cfi_def_cfa_offset 8\n\t")
                   __ASM_CFI(".cfi_offset r4, -8\n\t")
                   __ASM_CFI(".cfi_offset lr, -4\n\t")
                   "mov r4, sp\n\t"
                   __ASM_CFI(".cfi_def_cfa_register r4\n\t")
                   SPFIX
                   "blx r3\n\t"
                   "mov sp, r4\n\t"
                   __ASM_CFI(".cfi_def_cfa_register sp\n\t")
                   "pop {r4, PC}\n\t"
                   __ASM_CFI(".cfi_def_cfa_offset -8\n\t") )
#define FIXFUNC3(dll,func1,func2)                                       \
    long WINAPI WINECE_##func1(long a,long b,long c) {                       \
    static long (WINAPI *p##func2)(long,long,long); TRACE("\n");        \
    if (!p##func2) p##func2 = (void *)GetProcAddress(h##dll,#func2);    \
    return wrap3(a,b,c,p##func2); }

extern long wrap4( long,long,long,long, void*);
__ASM_GLOBAL_FUNC( wrap4,
                   "push {r4-r5, LR}\n\t"
                   __ASM_CFI(".cfi_def_cfa_offset 12\n\t")
                   __ASM_CFI(".cfi_offset r4, -12\n\t")
                   __ASM_CFI(".cfi_offset r5, -8\n\t")
                   __ASM_CFI(".cfi_offset lr, -4\n\t")
                   "mov r4, sp\n\t"
                   __ASM_CFI(".cfi_def_cfa_register r4\n\t")
                   SPFIX
                   "ldr r5, [r4, #12]\n\t"
                   "blx r5\n\t"
                   "mov sp, r4\n\t"
                   __ASM_CFI(".cfi_def_cfa_register sp\n\t")
                   "pop {r4-r5, PC}\n\t"
                   __ASM_CFI(".cfi_def_cfa_offset -12\n\t") )
#define FIXFUNC4(dll,func1,func2)                                       \
    long WINAPI WINECE_##func1(long a,long b,long c,long d) {                \
    static long (WINAPI *p##func2)(long,long,long,long); TRACE("\n");   \
    if (!p##func2) p##func2 = (void *)GetProcAddress(h##dll,#func2);    \
    return wrap4(a,b,c,d,p##func2); }

extern long wrap5(long,long,long,long, long, void*);
__ASM_GLOBAL_FUNC( wrap5,
                   "push {r4-r5, LR}\n\t"
                   __ASM_CFI(".cfi_def_cfa_offset 12\n\t")
                   __ASM_CFI(".cfi_offset r4, -12\n\t")
                   __ASM_CFI(".cfi_offset r5, -8\n\t")
                   __ASM_CFI(".cfi_offset lr, -4\n\t")
                   "mov r4, sp\n\t"
                   __ASM_CFI(".cfi_def_cfa_register r4\n\t")
                   SPFIX
                   "push {r5}\n\t"
                   "ldr r5, [r4, #12]\n\tpush {r5}\n\t"
                   "ldr r5, [r4, #16]\n\t"
                   "blx r5\n\t"
                   "mov sp, r4\n\t"
                   __ASM_CFI(".cfi_def_cfa_register sp\n\t")
                   "pop {r4-r5, PC}\n\t"
                   __ASM_CFI(".cfi_def_cfa_offset -12\n\t") )
#define FIXFUNC5(dll,func1,func2)                                           \
    long WINAPI WINECE_##func1(long a,long b,long c,long d,long e) {             \
    static long (WINAPI *p##func2)(long,long,long,long,long); TRACE("\n");  \
    if (!p##func2) p##func2 = (void *)GetProcAddress(h##dll,#func2);        \
    return wrap5(a,b,c,d,e,p##func2); }

extern long wrap6(long,long,long,long, long,long, void*);
__ASM_GLOBAL_FUNC( wrap6,
                   "push {r4-r5, LR}\n\t"
                   __ASM_CFI(".cfi_def_cfa_offset 12\n\t")
                   __ASM_CFI(".cfi_offset r4, -12\n\t")
                   __ASM_CFI(".cfi_offset r5, -8\n\t")
                   __ASM_CFI(".cfi_offset lr, -4\n\t")
                   "mov r4, sp\n\t"
                   __ASM_CFI(".cfi_def_cfa_register r4\n\t")
                   SPFIX
                   "ldr r5, [r4, #16]\n\tpush {r5}\n\t"
                   "ldr r5, [r4, #12]\n\tpush {r5}\n\t"
                   "ldr r5, [r4, #20]\n\t"
                   "blx r5\n\t"
                   "mov sp, r4\n\t"
                   __ASM_CFI(".cfi_def_cfa_register sp\n\t")
                   "pop {r4-r5, PC}\n\t"
                   __ASM_CFI(".cfi_def_cfa_offset -12\n\t") )
#define FIXFUNC6(dll,func1,func2)                                               \
    long WINAPI WINECE_##func1(long a,long b,long c,long d,long e,long f) {          \
    static long (WINAPI *p##func2)(long,long,long,long,long,long); TRACE("\n"); \
    if (!p##func2) p##func2 = (void *)GetProcAddress(h##dll,#func2);            \
    return wrap6(a,b,c,d,e,f,p##func2); }

extern long wrap7(long,long,long,long, long,long,long, void*);
__ASM_GLOBAL_FUNC( wrap7,
                   "push {r4-r5, LR}\n\t"
                   __ASM_CFI(".cfi_def_cfa_offset 12\n\t")
                   __ASM_CFI(".cfi_offset r4, -12\n\t")
                   __ASM_CFI(".cfi_offset r5, -8\n\t")
                   __ASM_CFI(".cfi_offset lr, -4\n\t")
                   "mov r4, sp\n\t"
                   __ASM_CFI(".cfi_def_cfa_register r4\n\t")
                   SPFIX
                   "push {r5}\n\t"
                   "ldr r5, [r4, #20]\n\tpush {r5}\n\t"
                   "ldr r5, [r4, #16]\n\tpush {r5}\n\t"
                   "ldr r5, [r4, #12]\n\tpush {r5}\n\t"
                   "ldr r5, [r4, #24]\n\t"
                   "blx r5\n\t"
                   "mov sp, r4\n\t"
                   __ASM_CFI(".cfi_def_cfa_register sp\n\t")
                   "pop {r4-r5, PC}\n\t"
                   __ASM_CFI(".cfi_def_cfa_offset -12\n\t") )
#define FIXFUNC7(dll,func1,func2)                                                       \
    long WINAPI WINECE_##func1(long a,long b,long c,long d,long e,long f,long g) {           \
    static long (WINAPI *p##func2)(long,long,long,long,long,long,long); TRACE("\n");    \
    if (!p##func2) p##func2 = (void *)GetProcAddress(h##dll,#func2);                    \
    return wrap7(a,b,c,d,e,f,g,p##func2); }

extern long wrap8(long,long,long,long, long,long,long,long, void*);
__ASM_GLOBAL_FUNC( wrap8,
                   "push {r4-r5, LR}\n\t"
                   __ASM_CFI(".cfi_def_cfa_offset 12\n\t")
                   __ASM_CFI(".cfi_offset r4, -12\n\t")
                   __ASM_CFI(".cfi_offset r5, -8\n\t")
                   __ASM_CFI(".cfi_offset lr, -4\n\t")
                   "mov r4, sp\n\t"
                   __ASM_CFI(".cfi_def_cfa_register r4\n\t")
                   SPFIX
                   "ldr r5, [r4, #24]\n\tpush {r5}\n\t"
                   "ldr r5, [r4, #20]\n\tpush {r5}\n\t"
                   "ldr r5, [r4, #16]\n\tpush {r5}\n\t"
                   "ldr r5, [r4, #12]\n\tpush {r5}\n\t"
                   "ldr r5, [r4, #28]\n\t"
                   "blx r5\n\t"
                   "mov sp, r4\n\t"
                   __ASM_CFI(".cfi_def_cfa_register sp\n\t")
                   "pop {r4-r5, PC}\n\t"
                   __ASM_CFI(".cfi_def_cfa_offset -12\n\t") )
#define FIXFUNC8(dll,func1,func2)                                                       \
    long WINAPI WINECE_##func1(long a,long b,long c,long d,long e,long f,long g,long h) {    \
    static long (WINAPI *p##func2)(long,long,long,long,long,long,long); TRACE("\n");    \
    if (!p##func2) p##func2 = (void *)GetProcAddress(h##dll,#func2);                    \
    return wrap8(a,b,c,d,e,f,g,h,p##func2); }

extern long wrap9(long,long,long,long, long,long,long,long, long,void*);
__ASM_GLOBAL_FUNC( wrap9,
                   "push {r4-r5, LR}\n\t"
                   __ASM_CFI(".cfi_def_cfa_offset 12\n\t")
                   __ASM_CFI(".cfi_offset r4, -12\n\t")
                   __ASM_CFI(".cfi_offset r5, -8\n\t")
                   __ASM_CFI(".cfi_offset lr, -4\n\t")
                   "mov r4, sp\n\t"
                   __ASM_CFI(".cfi_def_cfa_register r4\n\t")
                   SPFIX
                   "push {r5}\n\t"
                   "ldr r5, [r4, #28]\n\tpush {r5}\n\t"
                   "ldr r5, [r4, #24]\n\tpush {r5}\n\t"
                   "ldr r5, [r4, #20]\n\tpush {r5}\n\t"
                   "ldr r5, [r4, #16]\n\tpush {r5}\n\t"
                   "ldr r5, [r4, #12]\n\tpush {r5}\n\t"
                   "ldr r5, [r4, #32]\n\t"
                   "blx r5\n\t"
                   "mov sp, r4\n\t"
                   __ASM_CFI(".cfi_def_cfa_register sp\n\t")
                   "pop {r4-r5, PC}\n\t"
                   __ASM_CFI(".cfi_def_cfa_offset -12\n\t") )
#define FIXFUNC9(dll,func1,func2)                                                               \
    long WINAPI WINECE_##func1(long a,long b,long c,long d,long e,long f,long g,long h,long i) {     \
    static long (WINAPI *p##func2)(long,long,long,long,long,long,long,long,long); TRACE("\n");  \
    if (!p##func2) p##func2 = (void *)GetProcAddress(h##dll,#func2);                            \
    return wrap9(a,b,c,d,e,f,g,h,i,p##func2); }

extern long wrap10(long,long,long,long, long,long,long,long, long,long,void*);
__ASM_GLOBAL_FUNC( wrap10,
                   "push {r4-r5, LR}\n\t"
                   __ASM_CFI(".cfi_def_cfa_offset 12\n\t")
                   __ASM_CFI(".cfi_offset r4, -12\n\t")
                   __ASM_CFI(".cfi_offset r5, -8\n\t")
                   __ASM_CFI(".cfi_offset lr, -4\n\t")
                   "mov r4, sp\n\t"
                   __ASM_CFI(".cfi_def_cfa_register r4\n\t")
                   SPFIX
                   "ldr r5, [r4, #32]\n\tpush {r5}\n\t"
                   "ldr r5, [r4, #28]\n\tpush {r5}\n\t"
                   "ldr r5, [r4, #24]\n\tpush {r5}\n\t"
                   "ldr r5, [r4, #20]\n\tpush {r5}\n\t"
                   "ldr r5, [r4, #16]\n\tpush {r5}\n\t"
                   "ldr r5, [r4, #12]\n\tpush {r5}\n\t"
                   "ldr r5, [r4, #36]\n\t"
                   "blx r5\n\t"
                   "mov sp, r4\n\t"
                   __ASM_CFI(".cfi_def_cfa_register sp\n\t")
                   "pop {r4-r5, PC}\n\t"
                   __ASM_CFI(".cfi_def_cfa_offset -12\n\t") )
#define FIXFUNC10(dll,func1,func2)                                                                  \
    long WINAPI WINECE_##func1(long a,long b,long c,long d,long e,long f,long g,long h,long i,long j) {  \
    static long (WINAPI *p##func2)(long,long,long,long,long,long,long,long,long,long); TRACE("\n"); \
    if (!p##func2) p##func2 = (void *)GetProcAddress(h##dll,#func2);                                \
    return wrap10(a,b,c,d,e,f,g,h,i,j,p##func2); }

extern long wrap11(long,long,long,long, long,long,long,long, long,long,long,void*);
__ASM_GLOBAL_FUNC( wrap11,
                   "push {r4-r5, LR}\n\t"
                   __ASM_CFI(".cfi_def_cfa_offset 12\n\t")
                   __ASM_CFI(".cfi_offset r4, -12\n\t")
                   __ASM_CFI(".cfi_offset r5, -8\n\t")
                   __ASM_CFI(".cfi_offset lr, -4\n\t")
                   "mov r4, sp\n\t"
                   __ASM_CFI(".cfi_def_cfa_register r4\n\t")
                   SPFIX
                   "push {r5}\n\t"
                   "ldr r5, [r4, #36]\n\tpush {r5}\n\t"
                   "ldr r5, [r4, #32]\n\tpush {r5}\n\t"
                   "ldr r5, [r4, #28]\n\tpush {r5}\n\t"
                   "ldr r5, [r4, #24]\n\tpush {r5}\n\t"
                   "ldr r5, [r4, #20]\n\tpush {r5}\n\t"
                   "ldr r5, [r4, #16]\n\tpush {r5}\n\t"
                   "ldr r5, [r4, #12]\n\tpush {r5}\n\t"
                   "ldr r5, [r4, #40]\n\t"
                   "blx r5\n\t"
                   "mov sp, r4\n\t"
                   __ASM_CFI(".cfi_def_cfa_register sp\n\t")
                   "pop {r4-r5, PC}\n\t"
                   __ASM_CFI(".cfi_def_cfa_offset -12\n\t") )
#define FIXFUNC11(dll,func1,func2)                                                                          \
    long WINAPI WINECE_##func1(long a,long b,long c,long d,long e,long f,long g,long h,long i,long j,long k) {   \
    static long (WINAPI *p##func2)(long,long,long,long,long,long,long,long,long,long,long); TRACE("\n");    \
    if (!p##func2) p##func2 = (void *)GetProcAddress(h##dll,#func2);                                        \
    return wrap11(a,b,c,d,e,f,g,h,i,j,k,p##func2); }

extern long wrap12(long,long,long,long, long,long,long,long, long,long,long,long,void*);
__ASM_GLOBAL_FUNC( wrap12,
                   "push {r4-r5, LR}\n\t"
                   __ASM_CFI(".cfi_def_cfa_offset 12\n\t")
                   __ASM_CFI(".cfi_offset r4, -12\n\t")
                   __ASM_CFI(".cfi_offset r5, -8\n\t")
                   __ASM_CFI(".cfi_offset lr, -4\n\t")
                   "mov r4, sp\n\t"
                   __ASM_CFI(".cfi_def_cfa_register r4\n\t")
                   SPFIX
                   "ldr r5, [r4, #40]\n\tpush {r5}\n\t"
                   "ldr r5, [r4, #36]\n\tpush {r5}\n\t"
                   "ldr r5, [r4, #32]\n\tpush {r5}\n\t"
                   "ldr r5, [r4, #28]\n\tpush {r5}\n\t"
                   "ldr r5, [r4, #24]\n\tpush {r5}\n\t"
                   "ldr r5, [r4, #20]\n\tpush {r5}\n\t"
                   "ldr r5, [r4, #16]\n\tpush {r5}\n\t"
                   "ldr r5, [r4, #12]\n\tpush {r5}\n\t"
                   "ldr r5, [r4, #44]\n\t"
                   "blx r5\n\t"
                   "mov sp, r4\n\t"
                   __ASM_CFI(".cfi_def_cfa_register sp\n\t")
                   "pop {r4-r5, PC}\n\t"
                   __ASM_CFI(".cfi_def_cfa_offset -12\n\t") )
#define FIXFUNC12(dll,func1,func2)                                                                                  \
    long WINAPI WINECE_##func1(long a,long b,long c,long d,long e,long f,long g,long h,long i,long j,long k,long l) {    \
    static long (WINAPI *p##func2)(long,long,long,long,long,long,long,long,long,long,long,long); TRACE("\n");       \
    if (!p##func2) p##func2 = (void *)GetProcAddress(h##dll,#func2);                                                \
    return wrap12(a,b,c,d,e,f,g,h,i,j,k,l,p##func2); }

extern long wrap13(long,long,long,long, long,long,long,long, long,long,long,long, long,void*);
__ASM_GLOBAL_FUNC( wrap13,
                   "push {r4-r5, LR}\n\t"
                   __ASM_CFI(".cfi_def_cfa_offset 12\n\t")
                   __ASM_CFI(".cfi_offset r4, -12\n\t")
                   __ASM_CFI(".cfi_offset r5, -8\n\t")
                   __ASM_CFI(".cfi_offset lr, -4\n\t")
                   "mov r4, sp\n\t"
                   __ASM_CFI(".cfi_def_cfa_register r4\n\t")
                   SPFIX
                   "push {r5}\n\t"
                   "ldr r5, [r4, #44]\n\tpush {r5}\n\t"
                   "ldr r5, [r4, #40]\n\tpush {r5}\n\t"
                   "ldr r5, [r4, #36]\n\tpush {r5}\n\t"
                   "ldr r5, [r4, #32]\n\tpush {r5}\n\t"
                   "ldr r5, [r4, #28]\n\tpush {r5}\n\t"
                   "ldr r5, [r4, #24]\n\tpush {r5}\n\t"
                   "ldr r5, [r4, #20]\n\tpush {r5}\n\t"
                   "ldr r5, [r4, #16]\n\tpush {r5}\n\t"
                   "ldr r5, [r4, #12]\n\tpush {r5}\n\t"
                   "ldr r5, [r4, #48]\n\t"
                   "blx r5\n\t"
                   "mov sp, r4\n\t"
                   __ASM_CFI(".cfi_def_cfa_register sp\n\t")
                   "pop {r4-r5, PC}\n\t"
                   __ASM_CFI(".cfi_def_cfa_offset -12\n\t") )
#define FIXFUNC13(dll,func1,func2)                                                                                      \
    long WINAPI WINECE_##func1(long a,long b,long c,long d,long e,long f,long g,long h,long i,long j,long k,long l,long m) { \
    static long (WINAPI *p##func2)(long,long,long,long,long,long,long,long,long,long,long,long,long); TRACE("\n");      \
    if (!p##func2) p##func2 = (void *)GetProcAddress(h##dll,#func2);                                                    \
    return wrap13(a,b,c,d,e,f,g,h,i,j,k,l,m,p##func2); }

#ifdef __cplusplus
}
#endif

#endif  /* __WINE_WINE_CE_H */
