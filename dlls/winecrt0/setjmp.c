/*
 * setjmpex/longjmp functions for Wine exception handling
 *
 * Copyright (c) 1999, 2010 Alexandre Julliard
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

#include <stdarg.h>
#include "winternl.h"
#include "wine/exception.h"
#include "wine/asm.h"

#if defined(__GNUC__) || defined(__clang__)

#if defined(__i386__)

__ASM_GLOBAL_FUNC( __wine_setjmpex,
                   "movl 4(%esp),%ecx\n\t"   /* jmp_buf */
                   "movl %ebp,0(%ecx)\n\t"   /* jmp_buf.Ebp */
                   "movl %ebx,4(%ecx)\n\t"   /* jmp_buf.Ebx */
                   "movl %edi,8(%ecx)\n\t"   /* jmp_buf.Edi */
                   "movl %esi,12(%ecx)\n\t"  /* jmp_buf.Esi */
                   "movl %esp,16(%ecx)\n\t"  /* jmp_buf.Esp */
                   "movl 0(%esp),%eax\n\t"
                   "movl %eax,20(%ecx)\n\t"  /* jmp_buf.Eip */
                   "xorl %eax,%eax\n\t"
                   "ret" )

__ASM_GLOBAL_FUNC( __wine_longjmp,
                   "movl 4(%esp),%ecx\n\t"   /* jmp_buf */
                   "movl 8(%esp),%eax\n\t"   /* retval */
                   "movl 0(%ecx),%ebp\n\t"   /* jmp_buf.Ebp */
                   "movl 4(%ecx),%ebx\n\t"   /* jmp_buf.Ebx */
                   "movl 8(%ecx),%edi\n\t"   /* jmp_buf.Edi */
                   "movl 12(%ecx),%esi\n\t"  /* jmp_buf.Esi */
                   "movl 16(%ecx),%esp\n\t"  /* jmp_buf.Esp */
                   "addl $4,%esp\n\t"        /* get rid of return address */
                   "jmp *20(%ecx)\n\t"       /* jmp_buf.Eip */ )

#elif defined(__arm64ec__)

int __cdecl __attribute__((naked)) __wine_setjmpex( __wine_jmp_buf *buf, EXCEPTION_REGISTRATION_RECORD *frame )
{
    asm( "stp x29, x30, [sp, #-16]!\n\t"
         "stp x1, x27,  [x0]\n\t"          /* jmp_buf->Frame,Rbx */
         "add x1, sp, #16\n\t"
         "stp x1, x29,  [x0, #0x10]\n\t"   /* jmp_buf->Rsp,Rbp */
         "mov x29, sp\n\t"
         "stp x25, x26, [x0, #0x20]\n\t"   /* jmp_buf->Rsi,Rdi */
         "stp x19, x20, [x0, #0x30]\n\t"   /* jmp_buf->R12,R13 */
         "stp x21, x22, [x0, #0x40]\n\t"   /* jmp_buf->R14,R15 */
         "str x30,      [x0, #0x50]\n\t"   /* jmp_buf->Rip */
         "stp d8, d9,   [x0, #0x80]\n\t"   /* jmp_buf->Xmm8,Xmm9 */
         "stp d10, d11, [x0, #0xa0]\n\t"   /* jmp_buf->Xmm10,Xmm11 */
         "stp d12, d13, [x0, #0xc0]\n\t"   /* jmp_buf->Xmm12,Xmm13 */
         "stp d14, d15, [x0, #0xe0]\n\t"   /* jmp_buf->Xmm14,Xmm15 */
         "adrp x8, " __ASM_NAME("__os_arm64x_get_x64_information") "\n\t"
         "ldr x8, [x8, :lo12:" __ASM_NAME("__os_arm64x_get_x64_information") "]\n\t"
         "add x1, x0, #0x58\n\t"           /* jmp_buf->Mxcsr */
         "mov x0, #0\n\t"
         "blr x8\n\t"
         "mov x0, #0\n\t"
         "ldp x29, x30, [sp], #16\n\t"
         "ret" );
}

void __cdecl __attribute__((naked)) __wine_longjmp( __wine_jmp_buf *buf, int retval )
{
    asm( "mov x19, x0\n\t"
         "mov x20, x1\n\t"
         "adrp x8, " __ASM_NAME("__os_arm64x_set_x64_information") "\n\t"
         "ldr x8, [x8, :lo12:" __ASM_NAME("__os_arm64x_set_x64_information") "]\n\t"
         "ldr w1, [x0, #0x58]\n\t"         /* jmp_buf->Mxcsr */
         "mov x0, #0\n\t"
         "blr x8\n\t"
         "mov x1, x19\n\t"                 /* jmp_buf */
         "mov x0, x20\n\t"                 /* retval */
         "ldr x27,      [x1, #0x08]\n\t"   /* jmp_buf->Rbx */
         "ldp x2,  x29, [x1, #0x10]\n\t"   /* jmp_buf->Rsp,Rbp */
         "ldp x25, x26, [x1, #0x20]\n\t"   /* jmp_buf->Rsi,Rdi */
         "ldp x19, x20, [x1, #0x30]\n\t"   /* jmp_buf->R12,R13 */
         "ldp x21, x22, [x1, #0x40]\n\t"   /* jmp_buf->R14,R15 */
         "ldr x30,      [x1, #0x50]\n\t"   /* jmp_buf->Rip */
         "ldp d8,  d9,  [x1, #0x80]\n\t"   /* jmp_buf->Xmm8,Xmm9 */
         "ldp d10, d11, [x1, #0x90]\n\t"   /* jmp_buf->Xmm10,Xmm11 */
         "ldp d12, d13, [x1, #0xa0]\n\t"   /* jmp_buf->Xmm12,Xmm13 */
         "ldp d14, d15, [x1, #0xb0]\n\t"   /* jmp_buf->Xmm14,Xmm15 */
         "mov sp, x2\n\t"
         "ret" );
}

#elif defined(__x86_64__)

__ASM_GLOBAL_FUNC( __wine_setjmpex,
                   "movq %rdx,(%rcx)\n\t"          /* jmp_buf->Frame */
                   "movq %rbx,0x8(%rcx)\n\t"       /* jmp_buf->Rbx */
                   "leaq 0x8(%rsp),%rax\n\t"
                   "movq %rax,0x10(%rcx)\n\t"      /* jmp_buf->Rsp */
                   "movq %rbp,0x18(%rcx)\n\t"      /* jmp_buf->Rbp */
                   "movq %rsi,0x20(%rcx)\n\t"      /* jmp_buf->Rsi */
                   "movq %rdi,0x28(%rcx)\n\t"      /* jmp_buf->Rdi */
                   "movq %r12,0x30(%rcx)\n\t"      /* jmp_buf->R12 */
                   "movq %r13,0x38(%rcx)\n\t"      /* jmp_buf->R13 */
                   "movq %r14,0x40(%rcx)\n\t"      /* jmp_buf->R14 */
                   "movq %r15,0x48(%rcx)\n\t"      /* jmp_buf->R15 */
                   "movq (%rsp),%rax\n\t"
                   "movq %rax,0x50(%rcx)\n\t"      /* jmp_buf->Rip */
                   "stmxcsr 0x58(%rcx)\n\t"        /* jmp_buf->MxCsr */
                   "fnstcw 0x5c(%rcx)\n\t"         /* jmp_buf->FpCsr */
                   "movdqa %xmm6,0x60(%rcx)\n\t"   /* jmp_buf->Xmm6 */
                   "movdqa %xmm7,0x70(%rcx)\n\t"   /* jmp_buf->Xmm7 */
                   "movdqa %xmm8,0x80(%rcx)\n\t"   /* jmp_buf->Xmm8 */
                   "movdqa %xmm9,0x90(%rcx)\n\t"   /* jmp_buf->Xmm9 */
                   "movdqa %xmm10,0xa0(%rcx)\n\t"  /* jmp_buf->Xmm10 */
                   "movdqa %xmm11,0xb0(%rcx)\n\t"  /* jmp_buf->Xmm11 */
                   "movdqa %xmm12,0xc0(%rcx)\n\t"  /* jmp_buf->Xmm12 */
                   "movdqa %xmm13,0xd0(%rcx)\n\t"  /* jmp_buf->Xmm13 */
                   "movdqa %xmm14,0xe0(%rcx)\n\t"  /* jmp_buf->Xmm14 */
                   "movdqa %xmm15,0xf0(%rcx)\n\t"  /* jmp_buf->Xmm15 */
                   "xorq %rax,%rax\n\t"
                   "retq" )

__ASM_GLOBAL_FUNC( __wine_longjmp,
                   "movq %rdx,%rax\n\t"            /* retval */
                   "movq 0x8(%rcx),%rbx\n\t"       /* jmp_buf->Rbx */
                   "movq 0x18(%rcx),%rbp\n\t"      /* jmp_buf->Rbp */
                   "movq 0x20(%rcx),%rsi\n\t"      /* jmp_buf->Rsi */
                   "movq 0x28(%rcx),%rdi\n\t"      /* jmp_buf->Rdi */
                   "movq 0x30(%rcx),%r12\n\t"      /* jmp_buf->R12 */
                   "movq 0x38(%rcx),%r13\n\t"      /* jmp_buf->R13 */
                   "movq 0x40(%rcx),%r14\n\t"      /* jmp_buf->R14 */
                   "movq 0x48(%rcx),%r15\n\t"      /* jmp_buf->R15 */
                   "ldmxcsr 0x58(%rcx)\n\t"        /* jmp_buf->MxCsr */
                   "fnclex\n\t"
                   "fldcw 0x5c(%rcx)\n\t"          /* jmp_buf->FpCsr */
                   "movdqa 0x60(%rcx),%xmm6\n\t"   /* jmp_buf->Xmm6 */
                   "movdqa 0x70(%rcx),%xmm7\n\t"   /* jmp_buf->Xmm7 */
                   "movdqa 0x80(%rcx),%xmm8\n\t"   /* jmp_buf->Xmm8 */
                   "movdqa 0x90(%rcx),%xmm9\n\t"   /* jmp_buf->Xmm9 */
                   "movdqa 0xa0(%rcx),%xmm10\n\t"  /* jmp_buf->Xmm10 */
                   "movdqa 0xb0(%rcx),%xmm11\n\t"  /* jmp_buf->Xmm11 */
                   "movdqa 0xc0(%rcx),%xmm12\n\t"  /* jmp_buf->Xmm12 */
                   "movdqa 0xd0(%rcx),%xmm13\n\t"  /* jmp_buf->Xmm13 */
                   "movdqa 0xe0(%rcx),%xmm14\n\t"  /* jmp_buf->Xmm14 */
                   "movdqa 0xf0(%rcx),%xmm15\n\t"  /* jmp_buf->Xmm15 */
                   "movq 0x50(%rcx),%rdx\n\t"      /* jmp_buf->Rip */
                   "movq 0x10(%rcx),%rsp\n\t"      /* jmp_buf->Rsp */
                   "jmp *%rdx" )

#elif defined(__arm__)

__ASM_GLOBAL_FUNC( __wine_setjmpex,
                   "stm r0, {r1,r4-r11}\n"         /* jmp_buf->Frame,R4..R11 */
                   "str sp, [r0, #0x24]\n\t"       /* jmp_buf->Sp */
                   "str lr, [r0, #0x28]\n\t"       /* jmp_buf->Pc */
#ifndef __SOFTFP__
                   "vmrs r2, fpscr\n\t"
                   "str r2, [r0, #0x2c]\n\t"       /* jmp_buf->Fpscr */
                   "add r0, r0, #0x30\n\t"
                   "vstm r0, {d8-d15}\n\t"         /* jmp_buf->D[0..7] */
#endif
                   "mov r0, #0\n\t"
                   "bx lr" )

__ASM_GLOBAL_FUNC( __wine_longjmp,
                   "ldm r0, {r3-r11}\n\t"          /* jmp_buf->Frame,R4..R11 */
                   "ldr sp, [r0, #0x24]\n\t"       /* jmp_buf->Sp */
                   "ldr r2, [r0, #0x28]\n\t"       /* jmp_buf->Pc */
#ifndef __SOFTFP__
                   "ldr r3, [r0, #0x2c]\n\t"       /* jmp_buf->Fpscr */
                   "vmsr fpscr, r3\n\t"
                   "add r0, r0, #0x30\n\t"
                   "vldm r0, {d8-d15}\n\t"         /* jmp_buf->D[0..7] */
#endif
                   "mov r0, r1\n\t"                /* retval */
                   "bx r2" )

#elif defined(__aarch64__)

__ASM_GLOBAL_FUNC( __wine_setjmpex,
                   "str x1,       [x0]\n\t"        /* jmp_buf->Frame */
                   "stp x19, x20, [x0, #0x10]\n\t" /* jmp_buf->X19, X20 */
                   "stp x21, x22, [x0, #0x20]\n\t" /* jmp_buf->X21, X22 */
                   "stp x23, x24, [x0, #0x30]\n\t" /* jmp_buf->X23, X24 */
                   "stp x25, x26, [x0, #0x40]\n\t" /* jmp_buf->X25, X26 */
                   "stp x27, x28, [x0, #0x50]\n\t" /* jmp_buf->X27, X28 */
                   "stp x29, x30, [x0, #0x60]\n\t" /* jmp_buf->Fp,  Lr  */
                   "mov x2,  sp\n\t"
                   "str x2,       [x0, #0x70]\n\t" /* jmp_buf->Sp */
                   "mrs x2,  fpcr\n\t"
                   "str w2,       [x0, #0x78]\n\t" /* jmp_buf->Fpcr */
                   "mrs x2,  fpsr\n\t"
                   "str w2,       [x0, #0x7c]\n\t" /* jmp_buf->Fpsr */
                   "stp d8,  d9,  [x0, #0x80]\n\t" /* jmp_buf->D[0-1] */
                   "stp d10, d11, [x0, #0x90]\n\t" /* jmp_buf->D[2-3] */
                   "stp d12, d13, [x0, #0xa0]\n\t" /* jmp_buf->D[4-5] */
                   "stp d14, d15, [x0, #0xb0]\n\t" /* jmp_buf->D[6-7] */
                   "mov x0, #0\n\t"
                   "ret" )

__ASM_GLOBAL_FUNC( __wine_longjmp,
                   "ldp x19, x20, [x0, #0x10]\n\t" /* jmp_buf->X19, X20 */
                   "ldp x21, x22, [x0, #0x20]\n\t" /* jmp_buf->X21, X22 */
                   "ldp x23, x24, [x0, #0x30]\n\t" /* jmp_buf->X23, X24 */
                   "ldp x25, x26, [x0, #0x40]\n\t" /* jmp_buf->X25, X26 */
                   "ldp x27, x28, [x0, #0x50]\n\t" /* jmp_buf->X27, X28 */
                   "ldp x29, x30, [x0, #0x60]\n\t" /* jmp_buf->Fp,  Lr  */
                   "ldr x2,       [x0, #0x70]\n\t" /* jmp_buf->Sp */
                   "mov sp,  x2\n\t"
                   "ldr w2,       [x0, #0x78]\n\t" /* jmp_buf->Fpcr */
                   "msr fpcr, x2\n\t"
                   "ldr w2,       [x0, #0x7c]\n\t" /* jmp_buf->Fpsr */
                   "msr fpsr, x2\n\t"
                   "ldp d8,  d9,  [x0, #0x80]\n\t" /* jmp_buf->D[0-1] */
                   "ldp d10, d11, [x0, #0x90]\n\t" /* jmp_buf->D[2-3] */
                   "ldp d12, d13, [x0, #0xa0]\n\t" /* jmp_buf->D[4-5] */
                   "ldp d14, d15, [x0, #0xb0]\n\t" /* jmp_buf->D[6-7] */
                   "mov x0, x1\n\t"                /* retval */
                   "ret" )

#else

int __cdecl __wine_setjmpex( __wine_jmp_buf *buf, EXCEPTION_REGISTRATION_RECORD *frame )
{
    return setjmp( buf );
}

void __cdecl __wine_longjmp( __wine_jmp_buf *buf, int retval )
{
    for (;;) longjmp( buf, retval );
}

#endif

#endif /* __GNUC__ || __clang__ */
