/*
 * vcomp fork implementation
 *
 * Copyright 2012 Dan Kegel
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

#if 0
#pragma makedep arm64ec_x64
#endif

#include <stdarg.h>

#include "windef.h"
#include "wine/asm.h"

#ifdef __i386__

__ASM_GLOBAL_FUNC( _vcomp_fork_call_wrapper,
                   "pushl %ebp\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                   __ASM_CFI(".cfi_rel_offset %ebp,0\n\t")
                   "movl %esp,%ebp\n\t"
                   __ASM_CFI(".cfi_def_cfa_register %ebp\n\t")
                   "pushl %esi\n\t"
                   __ASM_CFI(".cfi_rel_offset %esi,-4\n\t")
                   "pushl %edi\n\t"
                   __ASM_CFI(".cfi_rel_offset %edi,-8\n\t")
                   "movl 12(%ebp),%edx\n\t"
                   "movl %esp,%edi\n\t"
                   "shll $2,%edx\n\t"
                   "jz 1f\n\t"
                   "subl %edx,%edi\n\t"
                   "andl $~15,%edi\n\t"
                   "movl %edi,%esp\n\t"
                   "movl 12(%ebp),%ecx\n\t"
                   "movl 16(%ebp),%esi\n\t"
                   "cld\n\t"
                   "rep; movsl\n"
                   "1:\tcall *8(%ebp)\n\t"
                   "leal -8(%ebp),%esp\n\t"
                   "popl %edi\n\t"
                   __ASM_CFI(".cfi_same_value %edi\n\t")
                   "popl %esi\n\t"
                   __ASM_CFI(".cfi_same_value %esi\n\t")
                   "popl %ebp\n\t"
                   __ASM_CFI(".cfi_def_cfa %esp,4\n\t")
                   __ASM_CFI(".cfi_same_value %ebp\n\t")
                   "ret" )

#elif defined __x86_64__

__ASM_GLOBAL_FUNC( _vcomp_fork_call_wrapper,
                   "pushq %rbp\n\t"
                   __ASM_SEH(".seh_pushreg %rbp\n\t")
                   __ASM_CFI(".cfi_adjust_cfa_offset 8\n\t")
                   __ASM_CFI(".cfi_rel_offset %rbp,0\n\t")
                   "movq %rsp,%rbp\n\t"
                   __ASM_SEH(".seh_setframe %rbp,0\n\t")
                   __ASM_CFI(".cfi_def_cfa_register %rbp\n\t")
                   "pushq %rsi\n\t"
                   __ASM_SEH(".seh_pushreg %rsi\n\t")
                   __ASM_CFI(".cfi_rel_offset %rsi,-8\n\t")
                   "pushq %rdi\n\t"
                   __ASM_SEH(".seh_pushreg %rdi\n\t")
                   __ASM_SEH(".seh_endprologue\n\t")
                   __ASM_CFI(".cfi_rel_offset %rdi,-16\n\t")
                   "movq %rcx,%rax\n\t"
                   "movq $4,%rcx\n\t"
                   "cmp %rcx,%rdx\n\t"
                   "cmovgq %rdx,%rcx\n\t"
                   "leaq 0(,%rcx,8),%rdx\n\t"
                   "subq %rdx,%rsp\n\t"
                   "andq $~15,%rsp\n\t"
                   "movq %rsp,%rdi\n\t"
                   "movq %r8,%rsi\n\t"
                   "rep; movsq\n\t"
                   "movq 0(%rsp),%rcx\n\t"
                   "movq 8(%rsp),%rdx\n\t"
                   "movq 16(%rsp),%r8\n\t"
                   "movq 24(%rsp),%r9\n\t"
                   "callq *%rax\n\t"
                   "leaq -16(%rbp),%rsp\n\t"
                   "popq %rdi\n\t"
                   __ASM_CFI(".cfi_same_value %rdi\n\t")
                   "popq %rsi\n\t"
                   __ASM_CFI(".cfi_same_value %rsi\n\t")
                   __ASM_CFI(".cfi_def_cfa_register %rsp\n\t")
                   "popq %rbp\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset -8\n\t")
                   __ASM_CFI(".cfi_same_value %rbp\n\t")
                   "ret" )

#elif defined __arm__

__ASM_GLOBAL_FUNC( _vcomp_fork_call_wrapper,
                   "push {fp, lr}\n\t"
                   ".seh_save_regs_w {fp, lr}\n\t"
                   "mov fp, sp\n\t"
                   ".seh_save_sp fp\n\t"
                   ".seh_endprologue\n\t"
                   "lsl r3, r1, #2\n\t"
                   "tst r1, #1\n\t"
                   "it ne\n\t"
                   "addne r3, r3, #4\n\t"
                   "cmp r3, #16\n\t"
                   "it lt\n\t"
                   "movlt r3, #16\n\t"
                   "sub sp, sp, r3\n\t"
                   "mov ip, r0\n\t"
                   "mov r3, #0\n\t"
                   "cmp r1, #0\n\t"
                   "beq 2f\n"
                   "1:\tldr r0, [r2, r3]\n\t"
                   "str r0, [sp, r3]\n\t"
                   "add r3, r3, #4\n\t"
                   "subs r1, r1, #1\n\t"
                   "bne 1b\n"
                   "2:\tpop {r0-r3}\n\t"
                   "blx ip\n\t"
                   "mov sp, fp\n\t"
                   "pop {fp, pc}" )

#elif defined __aarch64__

__ASM_GLOBAL_FUNC( _vcomp_fork_call_wrapper,
                   "stp x29, x30, [SP,#-16]!\n\t"
                   ".seh_save_fplr_x 16\n\t"
                   "mov x29, SP\n\t"
                   ".seh_set_fp\n\t"
                   ".seh_endprologue\n\t"
                   "mov x9, x0\n\t"
                   "cbz w1, 4f\n\t"
                   "lsl w8, w1, #3\n\t"
                   "cmp w8, #64\n\t"
                   "b.ge 1f\n\t"
                   "mov w8, #64\n"
                   "1:\ttbz w8, #3, 2f\n\t"
                   "add w8, w8, #8\n"
                   "2:\tsub x10, x29, x8\n\t"
                   "mov sp, x10\n"
                   "3:\tldr x0, [x2], #8\n\t"
                   "str x0, [x10], #8\n\t"
                   "subs w1, w1, #1\n\t"
                   "b.ne 3b\n\t"
                   "ldp x0, x1, [sp], #16\n\t"
                   "ldp x2, x3, [sp], #16\n\t"
                   "ldp x4, x5, [sp], #16\n\t"
                   "ldp x6, x7, [sp], #16\n"
                   "4:\tblr x9\n\t"
                   "mov SP, x29\n\t"
                   "ldp x29, x30, [SP], #16\n\t"
                   "ret" )

#endif
