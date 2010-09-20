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

/* no longer used, for backwards compatibility only */
struct wine_pthread_functions;
static void *pthread_functions[8];

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
#if defined(__sparc__) && defined(__GNUC__)
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
#elif defined(__ALPHA__) && defined(__GNUC__)
__ASM_GLOBAL_FUNC( wine_switch_to_stack,
                   "mov $16,$0\n\t" /* func */
                   "mov $17,$16\n\t" /* arg */
                   "mov $18,$30\n\t" /* stack */
                   "jsr $31,($0),0\n\t" /* call func */
                   "L1:\tbr $31,L1") /* loop */
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

#if defined(__i386__) && defined(__GNUC__)
__ASM_GLOBAL_FUNC( wine_call_on_stack,
                   "pushl %ebp\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                   __ASM_CFI(".cfi_rel_offset %ebp,0\n\t")
                   "pushl %esi\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                   __ASM_CFI(".cfi_rel_offset %esi,0\n\t")
                   "movl %esp,%esi\n\t"
                   __ASM_CFI(".cfi_def_cfa_register %esi\n\t")
                   "movl 12(%esp),%ecx\n\t"  /* func */
                   "movl 16(%esp),%edx\n\t"  /* arg */
                   "movl 20(%esp),%eax\n\t"  /* stack */
                   "andl $~15,%eax\n\t"
                   "subl $12,%eax\n\t"
                   "movl %eax,%esp\n\t"
                   "pushl %edx\n\t"
                   "xorl %ebp,%ebp\n\t"
                   "call *%ecx\n\t"
                   "movl %esi,%esp\n\t"
                   "popl %esi\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset -4\n\t")
                   __ASM_CFI(".cfi_same_value %esi\n\t")
                   "popl %ebp\n\t"
                   __ASM_CFI(".cfi_def_cfa %esp,4\n\t")
                   __ASM_CFI(".cfi_same_value %ebp\n\t")
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
                   __ASM_CFI(".cfi_adjust_cfa_offset 8\n\t")
                   __ASM_CFI(".cfi_rel_offset %rbp,0\n\t")
                   "movq %rsp,%rbp\n\t"
                   __ASM_CFI(".cfi_def_cfa_register %rbp\n\t")
                   "movq %rdi,%rax\n\t" /* func */
                   "movq %rsi,%rdi\n\t" /* arg */
                   "andq $~15,%rdx\n\t" /* stack */
                   "movq %rdx,%rsp\n\t"
                   "callq *%rax\n\t"    /* call func */
                   "movq %rbp,%rsp\n\t"
                   __ASM_CFI(".cfi_def_cfa_register %rsp\n\t")
                   "popq %rbp\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset -8\n\t")
                   __ASM_CFI(".cfi_same_value %rbp\n\t")
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
#elif defined(__arm__) && defined(__GNUC__)
__ASM_GLOBAL_FUNC( wine_call_on_stack,
                   "push {r4,LR}\n\t"   /* save return address on stack */
                   "mov r4, sp\n\t"     /* store old sp in local var */
                   "mov sp, r2\n\t"     /* stack */
                   "mov r2, r0\n\t"     /* func -> scratch register */
                   "mov r0, r1\n\t"     /* arg */
                   "mov LR, PC\n\t"     /* return after branch */
                   "mov PC, r2\n\t"     /* call func */
                   "mov sp, r4\n\t"     /* restore old sp from local var */
                   "pop {r4,PC}")       /* fetch return address into pc */
#else
#error You must implement wine_switch_to_stack for your platform
#endif
