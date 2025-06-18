/*
 * Arm64EC support
 *
 * Copyright 2023 Jacek Caban for CodeWeavers
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

#ifdef __arm64ec__

#include "windef.h"
#include "winnt.h"
#include "wine/asm.h"

void *__os_arm64x_check_icall_cfg = 0;
void *__os_arm64x_dispatch_call_no_redirect = 0;
void *__os_arm64x_dispatch_fptr = 0;
void *__os_arm64x_dispatch_ret = 0;
void *__os_arm64x_get_x64_information = 0;
void *__os_arm64x_set_x64_information = 0;

void *__os_arm64x_helper3 = 0;
void *__os_arm64x_helper4 = 0;
void *__os_arm64x_helper5 = 0;
void *__os_arm64x_helper6 = 0;
void *__os_arm64x_helper7 = 0;
void *__os_arm64x_helper8 = 0;

void *__guard_check_icall_fptr = 0;
void *__guard_dispatch_icall_fptr = 0;

asm( ".section .data,\"drw\"\n"
     ".balign 8\n"
     ".globl __os_arm64x_dispatch_icall\n"
     "__os_arm64x_dispatch_icall:\n"
     ".globl __os_arm64x_check_icall\n"
     "__os_arm64x_check_icall:\n"
     ".xword 0\n"
     ".globl __os_arm64x_dispatch_call\n"
     "__os_arm64x_dispatch_call:\n"
     ".globl __os_arm64x_check_call\n"
     "__os_arm64x_check_call:\n"
     ".xword 0\n" );

__ASM_GLOBAL_FUNC( __icall_helper_arm64ec,
                   "stp fp, lr, [sp, #-0x10]!\n\t"
                   ".seh_save_fplr_x 16\n\t"
                   "mov fp, sp\n\t"
                   ".seh_set_fp\n\t"
                   ".seh_endprologue\n\t"
                   "adrp x16, __os_arm64x_dispatch_icall\n\t"
                   "ldr x16, [x16, #:lo12:__os_arm64x_dispatch_icall]\n\t"
                   "blr x16\n\t"
                   ".seh_startepilogue\n\t"
                   "ldp fp, lr, [sp], #0x10\n\t"
                   ".seh_save_fplr_x 0x10\n\t"
                   ".seh_endepilogue\n\t"
		   "br x11" )

asm( "\t.section .rdata,\"dr\"\n"
     "\t.globl __chpe_metadata\n"
     "\t.balign 4\n"
     "__chpe_metadata:\n"
     "\t.word 2\n"
     "\t.rva __hybrid_code_map\n"
     "\t.word __hybrid_code_map_count\n"
     "\t.rva __x64_code_ranges_to_entry_points\n"
     "\t.rva __arm64x_redirection_metadata\n"
     "\t.rva __os_arm64x_dispatch_call_no_redirect\n"
     "\t.rva __os_arm64x_dispatch_ret\n"
     "\t.rva __os_arm64x_check_call\n"
     "\t.rva __os_arm64x_check_icall\n"
     "\t.rva __os_arm64x_check_icall_cfg\n"
     "\t.rva __arm64x_native_entrypoint\n"
     "\t.rva __hybrid_auxiliary_iat\n"
     "\t.word __x64_code_ranges_to_entry_points_count\n"
     "\t.word __arm64x_redirection_metadata_count\n"
     "\t.rva __os_arm64x_get_x64_information\n"
     "\t.rva __os_arm64x_set_x64_information\n"
     "\t.rva __arm64x_extra_rfe_table\n"
     "\t.word __arm64x_extra_rfe_table_size\n"
     "\t.rva __os_arm64x_dispatch_fptr\n"
     "\t.rva __hybrid_auxiliary_iat_copy\n"
     "\t.rva __hybrid_auxiliary_delayload_iat\n"
     "\t.rva __hybrid_auxiliary_delayload_iat_copy\n"
     "\t.word __hybrid_image_info_bitfield\n"
     "\t.rva __os_arm64x_helper3\n"
     "\t.rva __os_arm64x_helper4\n"
     "\t.rva __os_arm64x_helper5\n"
     "\t.rva __os_arm64x_helper6\n"
     "\t.rva __os_arm64x_helper7\n"
     "\t.rva __os_arm64x_helper8\n" );

#endif /* __arm64ec__ */
