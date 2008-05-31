/*
 * Wine portability routines
 *
 * Copyright 2000 Alexandre Julliard
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

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "wine/library.h"
#include "wine/pthread.h"

static struct wine_pthread_functions pthread_functions;

/***********************************************************************
 *           wine_pthread_get_functions
 */
void wine_pthread_get_functions( struct wine_pthread_functions *functions, size_t size )
{
    memcpy( functions, &pthread_functions, min( size, sizeof(pthread_functions) ));
}


/***********************************************************************
 *           wine_pthread_set_functions
 */
void wine_pthread_set_functions( const struct wine_pthread_functions *functions, size_t size )
{
    memcpy( &pthread_functions, functions, min( size, sizeof(pthread_functions) ));
}


/***********************************************************************
 *           wine_switch_to_stack
 *
 * Switch to the specified stack and call the function.
 */
void DECLSPEC_NORETURN wine_switch_to_stack( void (*func)(void *), void *arg, void *stack );
#if defined(__i386__) && defined(__GNUC__)
__ASM_GLOBAL_FUNC( wine_switch_to_stack,
                   "movl 4(%esp),%ecx\n\t"  /* func */
                   "movl 8(%esp),%edx\n\t"  /* arg */
                   "movl 12(%esp),%esp\n\t"  /* stack */
                   "andl $~15,%esp\n\t"
                   "subl $12,%esp\n\t"
                   "pushl %edx\n\t"
                   "xorl %ebp,%ebp\n\t"
                   "call *%ecx\n\t"
                   "int $3" /* we never return here */ )
#elif defined(__i386__) && defined(_MSC_VER)
__declspec(naked) void wine_switch_to_stack( void (*func)(void *), void *arg, void *stack )
{
  __asm mov ecx, 4[esp];
  __asm mov edx, 8[esp];
  __asm mov esp, 12[esp];
  __asm push edx;
  __asm xor ebp, ebp;
  __asm call [ecx];
  __asm int 3;
}
#elif defined(__sparc__) && defined(__GNUC__)
__ASM_GLOBAL_FUNC( wine_switch_to_stack,
                   "mov %o0, %l0\n\t" /* store first argument */
                   "mov %o1, %l1\n\t" /* store second argument */
                   "sub %o2, 96, %sp\n\t" /* store stack */
                   "call %l0, 0\n\t" /* call func */
                   "mov %l1, %o0\n\t" /* delay slot:  arg for func */
                   "ta 0x01") /* breakpoint - we never get here */
#elif defined(__powerpc__) && defined(__APPLE__)
__ASM_GLOBAL_FUNC( wine_switch_to_stack,
                   "mtctr r3\n\t" /* func -> ctr */
                   "mr r3,r4\n\t" /* args -> function param 1 (r3) */
                   "mr r1,r5\n\t" /* stack */
                   "subi r1,r1,0x100\n\t" /* adjust stack pointer */
                   "bctrl\n" /* call ctr */
                   "1:\tb 1b") /* loop */
#elif defined(__powerpc__) && defined(__GNUC__)
__ASM_GLOBAL_FUNC( wine_switch_to_stack,
                   "mtctr 3\n\t" /* func -> ctr */
                   "mr 3,4\n\t" /* args -> function param 1 (r3) */
                   "mr 1,5\n\t" /* stack */
                   "subi 1, 1, 16\n\t" /* allocate space for the callee */
                   "li 0, 0\n\t" /* load zero */
                   "stw 0, 0(1)\n\t" /* create a bottom stack frame */
                   "bctrl\n\t" /* call ctr */
                   "1:\tb 1b") /* loop */
#elif defined(__ALPHA__) && defined(__GNUC__)
__ASM_GLOBAL_FUNC( wine_switch_to_stack,
                   "mov $16,$0\n\t" /* func */
                   "mov $17,$16\n\t" /* arg */
                   "mov $18,$30\n\t" /* stack */
                   "jsr $31,($0),0\n\t" /* call func */
                   "L1:\tbr $31,L1") /* loop */
#elif defined(__x86_64__) && defined(__GNUC__)
__ASM_GLOBAL_FUNC( wine_switch_to_stack,
                   "movq %rdi,%rax\n\t" /* func */
                   "movq %rsi,%rdi\n\t" /* arg */
                   "andq $~15,%rdx\n\t" /* stack */
                   "movq %rdx,%rsp\n\t"
                   "xorq %rbp,%rbp\n\t"
                   "callq *%rax\n\t"    /* call func */
                   "int $3")
#else
void DECLSPEC_NORETURN wine_switch_to_stack( void (*func)(void *), void *arg, void *stack )
{
    wine_call_on_stack( (int (*)(void *))func, arg, stack );
    abort();
}
#endif


/***********************************************************************
 *           wine_call_on_stack
 *
 * Switch to the specified stack to call the function and return.
 */
int wine_call_on_stack( int (*func)(void *), void *arg, void *stack );
#if defined(__i386__) && defined(__GNUC__)
__ASM_GLOBAL_FUNC( wine_call_on_stack,
                   "pushl %ebp\n\t"
                   "pushl %esi\n\t"
                   "movl 12(%esp),%ecx\n\t"  /* func */
                   "movl 16(%esp),%edx\n\t"  /* arg */
                   "movl 20(%esp),%esi\n\t"  /* stack */
                   "andl $~15,%esi\n\t"
                   "subl $12,%esi\n\t"
                   "xchgl %esi,%esp\n\t"
                   "pushl %edx\n\t"
                   "xorl %ebp,%ebp\n\t"
                   "call *%ecx\n\t"
                   "movl %esi,%esp\n\t"
                   "popl %esi\n\t"
                   "popl %ebp\n\t"
                   "ret" )
#elif defined(__i386__) && defined(_MSC_VER)
__declspec(naked) int wine_call_on_stack( int (*func)(void *), void *arg, void *stack )
{
  __asm push ebp;
  __asm push esi;
  __asm mov ecx, 12[esp];
  __asm mov edx, 16[esp];
  __asm mov esi, 20[esp];
  __asm xchg esp, esi;
  __asm push edx;
  __asm xor ebp, ebp;
  __asm call [ecx];
  __asm mov esp, esi;
  __asm pop esi;
  __asm pop ebp
  __asm ret;
}
#elif defined(__x86_64__) && defined(__GNUC__)
__ASM_GLOBAL_FUNC( wine_call_on_stack,
                   "pushq %rbp\n\t"
                   "pushq %rbx\n\t"
                   "movq %rsp,%rbx\n\t"
                   "movq %rdi,%rax\n\t" /* func */
                   "movq %rsi,%rdi\n\t" /* arg */
                   "andq $~15,%rdx\n\t" /* stack */
                   "movq %rdx,%rsp\n\t"
                   "xorq %rbp,%rbp\n\t"
                   "callq *%rax\n\t"    /* call func */
                   "movq %rbx,%rsp\n\t"
                   "popq %rbx\n\t"
                   "popq %rbp\n\t"
                   "ret")
#elif defined(__powerpc__) && defined(__GNUC__)
__ASM_GLOBAL_FUNC( wine_call_on_stack,
                   "mflr 0\n\t"         /* get return address */
                   "stw 0, 4(1)\n\t"    /* save return address */
                   "subi 5, 5, 16\n\t" /* reserve space on new stack */
                   "stw 1, 12(5)\n\t"   /* store old sp */
                   "mtctr 3\n\t"        /* func -> ctr */
                   "mr 3,4\n\t"         /* args -> function param 1 (r3) */
                   "mr 1,5\n\t"         /* stack */
                   "li 0, 0\n\t"        /* zero */
                   "stw 0, 0(1)\n\t"    /* bottom of stack */
                   "stwu 1, -16(1)\n\t" /* create a frame for this function */
                   "bctrl\n\t"          /* call ctr */
                   "lwz 1, 28(1)\n\t"   /* fetch old sp */
                   "lwz 0, 4(1)\n\t"    /* fetch return address */
                   "mtlr 0\n\t"         /* return address -> lr */
                   "blr")               /* return */
#else
#error You must implement wine_switch_to_stack for your platform
#endif
