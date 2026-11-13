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

#elif defined(__riscv) && (__riscv_xlen == 64)

__ASM_GLOBAL_FUNC( _vcomp_fork_call_wrapper,
                   "addi sp, sp, -48\n\t"
                   "sd ra, 40(sp)\n\t"
                   "sd s0, 32(sp)\n\t"
                   "sd s1, 24(sp)\n\t"
                   "sd s2, 16(sp)\n\t"
                   "sd s3, 8(sp)\n\t"
                   "mv s0, a0\n\t"
                   "mv s1, a1\n\t"
                   "mv s2, a2\n\t"
                   "mv s3, sp\n\t"
                   "slli t0, s1, 3\n\t"
                   "li t1, 64\n\t"
                   "bgeu t0, t1, 1f\n\t"
                   "mv t0, t1\n"
                   "1:\taddi t0, t0, 15\n\t"
                   "andi t0, t0, -16\n\t"
                   "sub sp, sp, t0\n\t"
                   "mv t2, sp\n\t"
                   "mv t3, s2\n\t"
                   "mv t4, s1\n\t"
                   "beqz t4, 2f\n"
                   "3:\tld t5, 0(t3)\n\t"
                   "sd t5, 0(t2)\n\t"
                   "addi t2, t2, 8\n\t"
                   "addi t3, t3, 8\n\t"
                   "addi t4, t4, -1\n\t"
                   "bnez t4, 3b\n"
                   "2:\tld a0, 0(sp)\n\t"
                   "ld a1, 8(sp)\n\t"
                   "ld a2, 16(sp)\n\t"
                   "ld a3, 24(sp)\n\t"
                   "ld a4, 32(sp)\n\t"
                   "ld a5, 40(sp)\n\t"
                   "ld a6, 48(sp)\n\t"
                   "ld a7, 56(sp)\n\t"
                   "jalr ra, s0, 0\n\t"
                   "mv sp, s3\n\t"
                   "ld s3, 8(sp)\n\t"
                   "ld s2, 16(sp)\n\t"
                   "ld s1, 24(sp)\n\t"
                   "ld s0, 32(sp)\n\t"
                   "ld ra, 40(sp)\n\t"
                   "addi sp, sp, 48\n\t"
                   "ret" )

#endif
