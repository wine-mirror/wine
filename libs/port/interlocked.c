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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#ifdef __i386__

#ifdef __GNUC__

__ASM_GLOBAL_FUNC(interlocked_cmpxchg,
                  "movl 12(%esp),%eax\n\t"
                  "movl 8(%esp),%ecx\n\t"
                  "movl 4(%esp),%edx\n\t"
                  "lock; cmpxchgl %ecx,(%edx)\n\t"
                  "ret");
__ASM_GLOBAL_FUNC(interlocked_cmpxchg_ptr,
                  "movl 12(%esp),%eax\n\t"
                  "movl 8(%esp),%ecx\n\t"
                  "movl 4(%esp),%edx\n\t"
                  "lock; cmpxchgl %ecx,(%edx)\n\t"
                  "ret");
__ASM_GLOBAL_FUNC(interlocked_xchg,
                  "movl 8(%esp),%eax\n\t"
                  "movl 4(%esp),%edx\n\t"
                  "lock; xchgl %eax,(%edx)\n\t"
                  "ret");
__ASM_GLOBAL_FUNC(interlocked_xchg_ptr,
                  "movl 8(%esp),%eax\n\t"
                  "movl 4(%esp),%edx\n\t"
                  "lock; xchgl %eax,(%edx)\n\t"
                  "ret");
__ASM_GLOBAL_FUNC(interlocked_xchg_add,
                  "movl 8(%esp),%eax\n\t"
                  "movl 4(%esp),%edx\n\t"
                  "lock; xaddl %eax,(%edx)\n\t"
                  "ret");

#elif defined(_MSC_VER)

__declspec(naked) long interlocked_cmpxchg( long *dest, long xchg, long compare )
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

__declspec(naked) long interlocked_xchg( long *dest, long val )
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

__declspec(naked) long interlocked_xchg_add( long *dest, long incr )
{
    __asm mov eax, 8[esp];
    __asm mov edx, 4[esp];
    __asm lock xadd [edx], eax;
    __asm ret;
}

#else
# error You must implement the interlocked* functions for your compiler
#endif

#elif defined(__powerpc__)
void* interlocked_cmpxchg_ptr( void **dest, void* xchg, void* compare)
{
    long ret = 0;
    long scratch;
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
    return (void*)ret;
}

long interlocked_cmpxchg( long *dest, long xchg, long compare)
{
    long ret = 0;
    long scratch;
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

long interlocked_xchg_add( long *dest, long incr )
{
    long ret = 0;
    long zero = 0;
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

long interlocked_xchg( long* dest, long val )
{
    long ret = 0;
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
        "      bne- 0b \n"
        "      isync\n"
        : "=&r"(ret)
        : "r"(dest), "r"(val)
        : "cr0","memory","r0");
    return ret;
}

#elif defined(__sparc__) && defined(__sun__)

/*
 * As the earlier Sparc processors lack necessary atomic instructions,
 * I'm simply falling back to the library-provided _lwp_mutex routines
 * to ensure mutual exclusion in a way appropriate for the current
 * architecture.
 *
 * FIXME:  If we have the compare-and-swap instruction (Sparc v9 and above)
 *         we could use this to speed up the Interlocked operations ...
 */
#include <synch.h>
static lwp_mutex_t interlocked_mutex = DEFAULTMUTEX;

long interlocked_cmpxchg( long *dest, long xchg, long compare )
{
    _lwp_mutex_lock( &interlocked_mutex );
    if (*dest == compare) *dest = xchg;
    else compare = *dest;
    _lwp_mutex_unlock( &interlocked_mutex );
    return compare;
}

void *interlocked_cmpxchg_ptr( void **dest, void *xchg, void *compare )
{
    _lwp_mutex_lock( &interlocked_mutex );
    if (*dest == compare) *dest = xchg;
    else compare = *dest;
    _lwp_mutex_unlock( &interlocked_mutex );
    return compare;
}

long interlocked_xchg( long *dest, long val )
{
    long retv;
    _lwp_mutex_lock( &interlocked_mutex );
    retv = *dest;
    *dest = val;
    _lwp_mutex_unlock( &interlocked_mutex );
    return retv;
}

void *interlocked_xchg_ptr( void **dest, void *val )
{
    long retv;
    _lwp_mutex_lock( &interlocked_mutex );
    retv = *dest;
    *dest = val;
    _lwp_mutex_unlock( &interlocked_mutex );
    return retv;
}

long interlocked_xchg_add( long *dest, long incr )
{
    long retv;
    _lwp_mutex_lock( &interlocked_mutex );
    retv = *dest;
    *dest += incr;
    _lwp_mutex_unlock( &interlocked_mutex );
    return retv;
}

#elif defined(__ALPHA__) && defined(__GNUC__)

__ASM_GLOBAL_FUNC(interlocked_cmpxchg,
                  "L0cmpxchg:\n\t"
                  "ldl_l $0,0($16)\n\t"
                  "cmpeq $0,$18,$1\n\t"
                  "beq   $1,L1cmpxchg\n\t"
                  "mov   $17,$0\n\t"
                  "stl_c $0,0($16)\n\t"
                  "beq   $0,L0cmpxchg\n\t"
                  "mov   $18,$0\n"
                  "L1cmpxchg:\n\t"
                  "mb");

__ASM_GLOBAL_FUNC(interlocked_cmpxchg_ptr,
                  "L0cmpxchg_ptr:\n\t"
                  "ldq_l $0,0($16)\n\t"
                  "cmpeq $0,$18,$1\n\t"
                  "beq   $1,L1cmpxchg_ptr\n\t"
                  "mov   $17,$0\n\t"
                  "stq_c $0,0($16)\n\t"
                  "beq   $0,L0cmpxchg_ptr\n\t"
                  "mov   $18,$0\n"
                  "L1cmpxchg_ptr:\n\t"
                  "mb");

__ASM_GLOBAL_FUNC(interlocked_xchg,
                  "L0xchg:\n\t"
                  "ldl_l $0,0($16)\n\t"
                  "mov   $17,$1\n\t"
                  "stl_c $1,0($16)\n\t"
                  "beq   $1,L0xchg\n\t"
                  "mb");

__ASM_GLOBAL_FUNC(interlocked_xchg_ptr,
                  "L0xchg_ptr:\n\t"
                  "ldq_l $0,0($16)\n\t"
                  "mov   $17,$1\n\t"
                  "stq_c $1,0($16)\n\t"
                  "beq   $1,L0xchg_ptr\n\t"
                  "mb");

__ASM_GLOBAL_FUNC(interlocked_xchg_add,
                  "L0xchg_add:\n\t"
                  "ldl_l $0,0($16)\n\t"
                  "addl  $0,$17,$1\n\t"
                  "stl_c $1,0($16)\n\t"
                  "beq   $1,L0xchg_add\n\t"
                  "mb");

#else
# error You must implement the interlocked* functions for your CPU
#endif
