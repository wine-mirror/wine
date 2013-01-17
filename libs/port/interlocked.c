/*
 * interlocked functions
 *
 * Copyright 1996 Alexandre Julliard
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

#include "config.h"
#include "wine/port.h"
#include <assert.h>

#ifdef __i386__

#if defined(_MSC_VER)

__declspec(naked) int interlocked_cmpxchg( int *dest, int xchg, int compare )
{
    __asm mov eax, 12[esp];
    __asm mov ecx, 8[esp];
    __asm mov edx, 4[esp];
    __asm lock cmpxchg [edx], ecx;
    __asm ret;
}

__declspec(naked) void *interlocked_cmpxchg_ptr( void **dest, void *xchg, void *compare )
{
    __asm mov eax, 12[esp];
    __asm mov ecx, 8[esp];
    __asm mov edx, 4[esp];
    __asm lock cmpxchg [edx], ecx;
    __asm ret;
}

__declspec(naked) __int64 interlocked_cmpxchg64( __int64 *dest, __int64 xchg, __int64 compare)
{
    __asm push ebx;
    __asm push esi;
    __asm mov esi, 12[esp];
    __asm mov ebx, 16[esp];
    __asm mov ecx, 20[esp];
    __asm mov eax, 24[esp];
    __asm mov edx, 28[esp];
    __asm lock cmpxchg8b [esi];
    __asm pop esi;
    __asm pop ebx;
    __asm ret;
}

__declspec(naked) int interlocked_xchg( int *dest, int val )
{
    __asm mov eax, 8[esp];
    __asm mov edx, 4[esp];
    __asm lock xchg [edx], eax;
    __asm ret;
}

__declspec(naked) void *interlocked_xchg_ptr( void **dest, void *val )
{
    __asm mov eax, 8[esp];
    __asm mov edx, 4[esp];
    __asm lock xchg [edx], eax;
    __asm ret;
}

__declspec(naked) int interlocked_xchg_add( int *dest, int incr )
{
    __asm mov eax, 8[esp];
    __asm mov edx, 4[esp];
    __asm lock xadd [edx], eax;
    __asm ret;
}

#else
/* use gcc compatible asm code as default for __i386__ */

__ASM_GLOBAL_FUNC(interlocked_cmpxchg,
                  "movl 12(%esp),%eax\n\t"
                  "movl 8(%esp),%ecx\n\t"
                  "movl 4(%esp),%edx\n\t"
                  "lock; cmpxchgl %ecx,(%edx)\n\t"
                  "ret")
__ASM_GLOBAL_FUNC(interlocked_cmpxchg_ptr,
                  "movl 12(%esp),%eax\n\t"
                  "movl 8(%esp),%ecx\n\t"
                  "movl 4(%esp),%edx\n\t"
                  "lock; cmpxchgl %ecx,(%edx)\n\t"
                  "ret")
 __ASM_GLOBAL_FUNC(interlocked_cmpxchg64,
                   "push %ebx\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                   __ASM_CFI(".cfi_rel_offset %ebx,0\n\t")
                   "push %esi\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                   __ASM_CFI(".cfi_rel_offset %esi,0\n\t")
                   "movl 12(%esp),%esi\n\t"
                   "movl 16(%esp),%ebx\n\t"
                   "movl 20(%esp),%ecx\n\t"
                   "movl 24(%esp),%eax\n\t"
                   "movl 28(%esp),%edx\n\t"
                   "lock; cmpxchg8b (%esi)\n\t"
                   "pop %esi\n\t"
                   __ASM_CFI(".cfi_same_value %esi\n\t")
                   __ASM_CFI(".cfi_adjust_cfa_offset -4\n\t")
                   "pop %ebx\n\t"
                   __ASM_CFI(".cfi_same_value %ebx\n\t")
                   __ASM_CFI(".cfi_adjust_cfa_offset -4\n\t")
                   "ret")
__ASM_GLOBAL_FUNC(interlocked_xchg,
                  "movl 8(%esp),%eax\n\t"
                  "movl 4(%esp),%edx\n\t"
                  "lock; xchgl %eax,(%edx)\n\t"
                  "ret")
__ASM_GLOBAL_FUNC(interlocked_xchg_ptr,
                  "movl 8(%esp),%eax\n\t"
                  "movl 4(%esp),%edx\n\t"
                  "lock; xchgl %eax,(%edx)\n\t"
                  "ret")
__ASM_GLOBAL_FUNC(interlocked_xchg_add,
                  "movl 8(%esp),%eax\n\t"
                  "movl 4(%esp),%edx\n\t"
                  "lock; xaddl %eax,(%edx)\n\t"
                  "ret")

#endif

#elif defined(__x86_64__)

__ASM_GLOBAL_FUNC(interlocked_cmpxchg,
                  "mov %edx, %eax\n\t"
                  "lock cmpxchgl %esi,(%rdi)\n\t"
                  "ret")
__ASM_GLOBAL_FUNC(interlocked_cmpxchg_ptr,
                  "mov %rdx, %rax\n\t"
                  "lock cmpxchgq %rsi,(%rdi)\n\t"
                  "ret")
__ASM_GLOBAL_FUNC(interlocked_cmpxchg64,
                  "mov %rdx, %rax\n\t"
                  "lock cmpxchgq %rsi,(%rdi)\n\t"
                  "ret")
__ASM_GLOBAL_FUNC(interlocked_xchg,
                  "mov %esi, %eax\n\t"
                  "lock xchgl %eax, (%rdi)\n\t"
                  "ret")
__ASM_GLOBAL_FUNC(interlocked_xchg_ptr,
                  "mov %rsi, %rax\n\t"
                  "lock xchgq %rax,(%rdi)\n\t"
                  "ret")
__ASM_GLOBAL_FUNC(interlocked_xchg_add,
                  "mov %esi, %eax\n\t"
                  "lock xaddl %eax, (%rdi)\n\t"
                  "ret")
__ASM_GLOBAL_FUNC(interlocked_cmpxchg128,
                  "push %rbx\n\t"
                  __ASM_CFI(".cfi_adjust_cfa_offset 8\n\t")
                  __ASM_CFI(".cfi_rel_offset %rbx,0\n\t")
                  "mov %rcx,%r8\n\t"  /* compare */
                  "mov %rdx,%rbx\n\t" /* xchg_low */
                  "mov %rsi,%rcx\n\t" /* xchg_high */
                  "mov 0(%r8),%rax\n\t"
                  "mov 8(%r8),%rdx\n\t"
                  "lock cmpxchg16b (%rdi)\n\t"
                  "mov %rax,0(%r8)\n\t"
                  "mov %rdx,8(%r8)\n\t"
                  "setz %al\n\t"
                  "pop %rbx\n\t"
                  __ASM_CFI(".cfi_adjust_cfa_offset -8\n\t")
                  __ASM_CFI(".cfi_same_value %rbx\n\t")
                  "ret")

#elif defined(__powerpc__)
void* interlocked_cmpxchg_ptr( void **dest, void* xchg, void* compare)
{
    void *ret = 0;
    void *scratch;
    __asm__ __volatile__(
        "0:    lwarx %0,0,%2\n"
        "      xor. %1,%4,%0\n"
        "      bne 1f\n"
        "      stwcx. %3,0,%2\n"
        "      bne- 0b\n"
        "      isync\n"
        "1:    "
        : "=&r"(ret), "=&r"(scratch)
        : "r"(dest), "r"(xchg), "r"(compare)
        : "cr0","memory");
    return ret;
}

__int64 interlocked_cmpxchg64( __int64 *dest, __int64 xchg, __int64 compare)
{
    /* FIXME: add code */
    assert(0);
}

int interlocked_cmpxchg( int *dest, int xchg, int compare)
{
    int ret = 0;
    int scratch;
    __asm__ __volatile__(
        "0:    lwarx %0,0,%2\n"
        "      xor. %1,%4,%0\n"
        "      bne 1f\n"
        "      stwcx. %3,0,%2\n"
        "      bne- 0b\n"
        "      isync\n"
        "1:    "
        : "=&r"(ret), "=&r"(scratch)
        : "r"(dest), "r"(xchg), "r"(compare)
        : "cr0","memory","r0");
    return ret;
}

int interlocked_xchg_add( int *dest, int incr )
{
    int ret = 0;
    int zero = 0;
    __asm__ __volatile__(
        "0:    lwarx %0, %3, %1\n"
        "      add %0, %2, %0\n"
        "      stwcx. %0, %3, %1\n"
        "      bne- 0b\n"
        "      isync\n"
        : "=&r" (ret)
        : "r"(dest), "r"(incr), "r"(zero)
        : "cr0", "memory", "r0"
    );
    return ret-incr;
}

int interlocked_xchg( int* dest, int val )
{
    int ret = 0;
    __asm__ __volatile__(
        "0:    lwarx %0,0,%1\n"
        "      stwcx. %2,0,%1\n"
        "      bne- 0b\n"
        "      isync\n"
        : "=&r"(ret)
        : "r"(dest), "r"(val)
        : "cr0","memory","r0");
    return ret;
}

void* interlocked_xchg_ptr( void** dest, void* val )
{
    void *ret = NULL;
    __asm__ __volatile__(
        "0:    lwarx %0,0,%1\n"
        "      stwcx. %2,0,%1\n"
        "      bne- 0b\n"
        "      isync\n"
        : "=&r"(ret)
        : "r"(dest), "r"(val)
        : "cr0","memory","r0");
    return ret;
}

#else

#include <pthread.h>

static pthread_mutex_t interlocked_mutex = PTHREAD_MUTEX_INITIALIZER;

int interlocked_cmpxchg( int *dest, int xchg, int compare )
{
    pthread_mutex_lock( &interlocked_mutex );

    if (*dest == compare)
        *dest = xchg;
    else
        compare = *dest;

    pthread_mutex_unlock( &interlocked_mutex );
    return compare;
}

void *interlocked_cmpxchg_ptr( void **dest, void *xchg, void *compare )
{
    pthread_mutex_lock( &interlocked_mutex );

    if (*dest == compare)
        *dest = xchg;
    else
        compare = *dest;

    pthread_mutex_unlock( &interlocked_mutex );
    return compare;
}

__int64 interlocked_cmpxchg64( __int64 *dest, __int64 xchg, __int64 compare )
{
    pthread_mutex_lock( &interlocked_mutex );

    if (*dest == compare)
        *dest = xchg;
    else
        compare = *dest;

    pthread_mutex_unlock( &interlocked_mutex );
    return compare;
}

int interlocked_xchg( int *dest, int val )
{
    int retv;
    pthread_mutex_lock( &interlocked_mutex );
    retv = *dest;
    *dest = val;
    pthread_mutex_unlock( &interlocked_mutex );
    return retv;
}

void *interlocked_xchg_ptr( void **dest, void *val )
{
    void *retv;
    pthread_mutex_lock( &interlocked_mutex );
    retv = *dest;
    *dest = val;
    pthread_mutex_unlock( &interlocked_mutex );
    return retv;
}

int interlocked_xchg_add( int *dest, int incr )
{
    int retv;
    pthread_mutex_lock( &interlocked_mutex );
    retv = *dest;
    *dest += incr;
    pthread_mutex_unlock( &interlocked_mutex );
    return retv;
}

int interlocked_cmpxchg128( __int64 *dest, __int64 xchg_high, __int64 xchg_low, __int64 *compare )
{
    int retv;
    pthread_mutex_lock( &interlocked_mutex );
    if (dest[0] == compare[0] && dest[1] == compare[1])
    {
        dest[0] = xchg_low;
        dest[1] = xchg_high;
        retv = 1;
    }
    else
    {
        compare[0] = dest[0];
        compare[1] = dest[1];
        retv = 0;
    }
    pthread_mutex_unlock( &interlocked_mutex );
    return retv;
}

#endif
