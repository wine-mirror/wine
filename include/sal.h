/*
 * Copyright (C) 2018 Alistair Leslie-Hughes
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

#ifndef __WINE_SAL_H__
#define __WINE_SAL_H__

#define _Always_(exp)

#define _Analysis_assume_(x)
#define _Analysis_noreturn_

#define _At_(a, b)
#define _At_buffer_(a, b, c, d)

#define _Check_return_

#define _COM_Outptr_
#define _COM_Outptr_result_maybenull_

#define _Deref_in_range_(cnt)
#define _Deref_out_
#define _Deref_out_z_
#define _Deref_out_opt_
#define _Deref_out_opt_z_
#define _Deref_out_range_(x, y)
#define _Deref_post_maybenull_
#define _Deref_post_opt_cap_(count)
#define _Deref_post_opt_valid_
#define _Deref_post_opt_z_cap_(count)
#define _Deref_post_valid_
#define _Deref_post_z_
#define _Deref_pre_maybenull_
#define _Deref_pre_z_
#define _Deref_prepost_opt_z_
#define _Deref_ret_z_

#define _Field_range_(min, max)

#define _In_
#define _In_bytecount_(count)
#define _In_bytecount_c_(count)
#define _In_bytecount_x_(count)
#define _In_count_(count)
#define _In_count_c_(count)
#define _In_count_x_(count)
#define _In_opt_
#define _In_opt_bytecount_(count)
#define _In_opt_bytecount_c_(count)
#define _In_opt_bytecount_x_(count)
#define _In_opt_count_(count)
#define _In_opt_count_c_(count)
#define _In_opt_count_x_(count)
#define _In_opt_ptrdiff_count_(count)
#define _In_opt_z_
#define _In_opt_z_count_(count)
#define _In_opt_z_count_c_(count)
#define _In_opt_z_bytecount_(count)
#define _In_opt_z_bytecount_c_(count)
#define _In_ptrdiff_count_(count)
#define _In_range_(min, max)
#define _In_reads_(count)
#define _In_reads_bytes_(count)
#define _In_reads_bytes_opt_(count)
#define _In_reads_opt_(count)
#define _In_reads_opt_z_(count)
#define _In_reads_or_z_(count)
#define _In_reads_or_z_opt_(count)
#define _In_reads_z_(count)
#define _In_z_
#define _In_z_bytecount_(count)
#define _In_z_bytecount_c_(count)
#define _In_z_count_(count)
#define _In_z_count_c_(count)

#define _Inout_
#define _Inout_bytecap_x_(count)
#define _Inout_cap_(count)
#define _Inout_cap_c_(count)
#define _Inout_opt_
#define _Inout_opt_cap_c_(count)
#define _Inout_opt_z_
#define _Inout_opt_z_bytecap_(count)
#define _Inout_updates_(count)
#define _Inout_updates_bytes_(count)
#define _Inout_updates_bytes_all_opt_(count)
#define _Inout_updates_bytes_opt_(count)
#define _Inout_updates_opt_(count)
#define _Inout_updates_z_(count)
#define _Inout_updates_opt_z_(count)
#define _Inout_z_
#define _Inout_z_bytecap_(count)
#define _Inout_z_cap_(count)
#define _Inout_z_cap_c_(count)

#define _Must_inspect_result_

#define _Null_
#define _Null_terminated_

#define _On_failure_(d)

#define _Out_
#define _Outptr_
#define _Out_bytecap_(count)
#define _Out_bytecap_c_(count)
#define _Out_bytecap_x_(count)
#define _Out_bytecapcount_(count)
#define _Out_bytecap_post_bytecount_(count1, count2)
#define _Out_cap_(count)
#define _Out_cap_c_(count)
#define _Out_cap_m_(count1, count2)
#define _Out_cap_x_(count)
#define _Out_cap_post_count_(count1, count2)
#define _Out_opt_
#define _Out_opt_bytecap_(count)
#define _Out_opt_bytecap_c_(count)
#define _Out_opt_bytecap_x_(count)
#define _Out_opt_bytecap_post_bytecount_(count1, count2)
#define _Out_opt_bytecapcount_(count)
#define _Out_opt_cap_(count)
#define _Out_opt_cap_m_(count1, count2)
#define _Out_opt_cap_c_(count)
#define _Out_opt_cap_x_(count)
#define _Out_opt_cap_post_count_(count1, count2)
#define _Out_opt_ptrdiff_cap_(count)
#define _Out_opt_z_bytecap_(count)
#define _Out_opt_z_bytecap_c_(count)
#define _Out_opt_z_bytecap_x_(count)
#define _Out_opt_z_bytecap_post_bytecount_(count1, count2)
#define _Out_opt_z_cap_(count)
#define _Out_opt_z_cap_c_(count)
#define _Out_opt_z_cap_m_(count1, count2)
#define _Out_opt_z_cap_x_(count)
#define _Out_opt_z_cap_post_count_(count1, count2)
#define _Out_ptrdiff_cap_(count)
#define _Out_range_(x, y)
#define _Out_writes_(count)
#define _Out_writes_all_(count)
#define _Out_writes_bytes_(count)
#define _Out_writes_bytes_all_(count)
#define _Out_writes_bytes_all_opt_(count)
#define _Out_writes_bytes_opt_(count)
#define _Out_writes_bytes_to_(count1, count2)
#define _Out_writes_bytes_to_opt_(count1, count2)
#define _Out_writes_opt_(count)
#define _Out_writes_opt_z_(count)
#define _Out_writes_to_(count1, count2)
#define _Out_writes_to_opt_(count1, count2)
#define _Out_writes_z_(count)
#define _Out_z_bytecap_(count)
#define _Out_z_bytecap_c_(count)
#define _Out_z_bytecap_x_(count)
#define _Out_z_bytecap_post_bytecount_(count1, count2)
#define _Out_z_cap_(count)
#define _Out_z_cap_c_(count)
#define _Out_z_cap_m_(count1, count2)
#define _Out_z_cap_x_(count)
#define _Out_z_cap_post_count_(count1, count2)

#define _Outptr_opt_
#define _Outptr_opt_result_buffer_(count)
#define _Outptr_opt_result_maybenull_
#define _Outptr_opt_result_maybenull_z_
#define _Outptr_opt_result_z_
#define _Outptr_result_buffer_(count)
#define _Outptr_result_buffer_all_maybenull_(ptr)
#define _Outptr_result_buffer_maybenull_(count)
#define _Outptr_result_maybenull_
#define _Outptr_result_maybenull_z_
#define _Outptr_result_nullonfailure_
#define _Outptr_result_z_

#define _Post_count_(count)
#define _Post_count_c_(count)
#define _Post_equal_to_(exp)
#define _Post_invalid_
#define _Post_maybez_
#define _Post_ptr_invalid_
#define _Post_readable_byte_size_(count)
#define _Post_readable_size_(count)
#define _Post_satisfies_(exp)
#define _Post_writable_byte_size_(count)
#define _Post_writable_size_(count)
#define _Post_z_

#define _Pre_cap_for_(count)
#define _Pre_maybenull_
#define _Pre_notnull_
#define _Pre_null_
#define _Pre_opt_z_
#define _Pre_valid_
#define _Pre_writable_size_(count)
#define _Pre_z_
#define _Prepost_z_

#define _Printf_format_string_
#define _Printf_format_string_params_(count)

#define _Reserved_

#define _Result_nullonfailure_

#define _Ret_maybenull_
#define _Ret_maybenull_z_
#define _Ret_notnull_
#define _Ret_opt_
#define _Ret_opt_bytecap_(count)
#define _Ret_opt_bytecap_x_(count)
#define _Ret_opt_z_cap_(count)
#define _Ret_range_(exp,exp2)
#define _Ret_writes_bytes_maybenull_(count)
#define _Ret_writes_maybenull_(count)
#define _Ret_writes_z_(count)
#define _Ret_z_

#define _Return_type_success_(count)

#define _Scanf_format_string_
#define _Scanf_format_string_params_(count)
#define _Scanf_s_format_string_
#define _Scanf_s_format_string_params_(count)

#define _Success_(exp)

#define _When_(exp1, exp2)

#define __bcount(size)
#define __bcount_opt(size)

#define __deref_bcount(size)
#define __deref_bcount_opt(size)
#define __deref_ecount(size)
#define __deref_ecount_opt(size)
#define __deref_inout
#define __deref_inout_bcount(size)
#define __deref_inout_bcount_full(size)
#define __deref_inout_bcount_full_opt(size)
#define __deref_inout_bcount_nz(size)
#define __deref_inout_bcount_nz_opt(size)
#define __deref_inout_bcount_opt(size)
#define __deref_inout_bcount_part(size,length)
#define __deref_inout_bcount_part_opt(size,length)
#define __deref_inout_bcount_z(size)
#define __deref_inout_bcount_z_opt(size)
#define __deref_inout_ecount(size)
#define __deref_inout_ecount_full(size)
#define __deref_inout_ecount_full_opt(size)
#define __deref_inout_ecount_nz(size)
#define __deref_inout_ecount_nz_opt(size)
#define __deref_inout_ecount_opt(size)
#define __deref_inout_ecount_part(size,length)
#define __deref_inout_ecount_part_opt(size,length)
#define __deref_inout_ecount_z(size)
#define __deref_inout_ecount_z_opt(size)
#define __deref_inout_nz
#define __deref_inout_nz_opt
#define __deref_inout_opt
#define __deref_inout_z
#define __deref_inout_z_opt
#define __deref_opt_bcount(size)
#define __deref_opt_bcount_opt(size)
#define __deref_opt_ecount(size)
#define __deref_opt_ecount_opt(size)
#define __deref_opt_inout
#define __deref_opt_inout_bcount(size)
#define __deref_opt_inout_bcount_full(size)
#define __deref_opt_inout_bcount_full_opt(size)
#define __deref_opt_inout_bcount_nz(size)
#define __deref_opt_inout_bcount_nz_opt(size)
#define __deref_opt_inout_bcount_opt(size)
#define __deref_opt_inout_bcount_part(size,length)
#define __deref_opt_inout_bcount_part_opt(size,length)
#define __deref_opt_inout_bcount_z(size)
#define __deref_opt_inout_bcount_z_opt(size)
#define __deref_opt_inout_ecount(size)
#define __deref_opt_inout_ecount_full(size)
#define __deref_opt_inout_ecount_full_opt(size)
#define __deref_opt_inout_ecount_nz(size)
#define __deref_opt_inout_ecount_nz_opt(size)
#define __deref_opt_inout_ecount_opt(size)
#define __deref_opt_inout_ecount_part(size,length)
#define __deref_opt_inout_ecount_part_opt(size,length)
#define __deref_opt_inout_ecount_z(size)
#define __deref_opt_inout_ecount_z_opt(size)
#define __deref_opt_inout_nz
#define __deref_opt_inout_nz_opt
#define __deref_opt_inout_opt
#define __deref_opt_inout_z
#define __deref_opt_inout_z_opt
#define __deref_opt_out
#define __deref_opt_out_bcount(size)
#define __deref_opt_out_bcount_full(size)
#define __deref_opt_out_bcount_full_opt(size)
#define __deref_opt_out_bcount_nz_opt(size)
#define __deref_opt_out_bcount_opt(size)
#define __deref_opt_out_bcount_part(size,length)
#define __deref_opt_out_bcount_part_opt(size,length)
#define __deref_opt_out_bcount_z_opt(size)
#define __deref_opt_out_ecount(size)
#define __deref_opt_out_ecount_full(size)
#define __deref_opt_out_ecount_full_opt(size)
#define __deref_opt_out_ecount_nz_opt(size)
#define __deref_opt_out_ecount_opt(size)
#define __deref_opt_out_ecount_part(size,length)
#define __deref_opt_out_ecount_part_opt(size,length)
#define __deref_opt_out_ecount_z_opt(size)
#define __deref_opt_out_nz_opt
#define __deref_opt_out_opt
#define __deref_opt_out_z
#define __deref_opt_out_z_opt
#define __deref_out
#define __deref_out_bcount(size)
#define __deref_out_bcount_full(size)
#define __deref_out_bcount_full_opt(size)
#define __deref_out_bcount_nz(size)
#define __deref_out_bcount_nz_opt(size)
#define __deref_out_bcount_opt(size)
#define __deref_out_bcount_part(size,length)
#define __deref_out_bcount_part_opt(size,length)
#define __deref_out_bcount_z(size)
#define __deref_out_bcount_z_opt(size)
#define __deref_out_ecount(size)
#define __deref_out_ecount_full(size)
#define __deref_out_ecount_full_opt(size)
#define __deref_out_ecount_nz(size)
#define __deref_out_ecount_nz_opt(size)
#define __deref_out_ecount_opt(size)
#define __deref_out_ecount_part(size,length)
#define __deref_out_ecount_part_opt(size,length)
#define __deref_out_ecount_z(size)
#define __deref_out_ecount_z_opt(size)
#define __deref_out_nz
#define __deref_out_nz_opt
#define __deref_out_opt
#define __deref_out_z
#define __deref_out_z_opt

#define __ecount(size)
#define __ecount_opt(size)

/* #define __in */ /* conflicts with system headers */
#define __in_bcount(size)
#define __in_bcount_nz(size)
#define __in_bcount_nz_opt(size)
#define __in_bcount_opt(size)
#define __in_bcount_z(size)
#define __in_bcount_z_opt(size)
#define __in_ecount(size)
#define __in_ecount_nz(size)
#define __in_ecount_nz_opt(size)
#define __in_ecount_opt(size)
#define __in_ecount_z(size)
#define __in_ecount_z_opt(size)
#define __in_nz
#define __in_nz_opt
#define __in_opt
#define __in_z
#define __in_z_opt
#define __inout
#define __inout_bcount(size)
#define __inout_bcount_full(size)
#define __inout_bcount_full_opt(size)
#define __inout_bcount_nz(size)
#define __inout_bcount_nz_opt(size)
#define __inout_bcount_opt(size)
#define __inout_bcount_part(size,length)
#define __inout_bcount_part_opt(size,length)
#define __inout_bcount_z(size)
#define __inout_bcount_z_opt(size)
#define __inout_ecount(size)
#define __inout_ecount_full(size)
#define __inout_ecount_full_opt(size)
#define __inout_ecount_nz(size)
#define __inout_ecount_nz_opt(size)
#define __inout_ecount_opt(size)
#define __inout_ecount_part(size,length)
#define __inout_ecount_part_opt(size,length)
#define __inout_ecount_z(size)
#define __inout_ecount_z_opt(size)
#define __inout_ecount_z_opt(size)
#define __inout_nz
#define __inout_nz_opt
#define __inout_opt
#define __inout_z
#define __inout_z_opt

/* #define __out */ /* conflicts with system headers */
#define __out_bcount(size)
#define __out_bcount_full(size)
#define __out_bcount_full_opt(size)
#define __out_bcount_full_z(size)
#define __out_bcount_full_z_opt(size)
#define __out_bcount_nz(size)
#define __out_bcount_nz_opt(size)
#define __out_bcount_opt(size)
#define __out_bcount_part(size,length)
#define __out_bcount_part_opt(size,length)
#define __out_bcount_part_z(size,length)
#define __out_bcount_part_z_opt(size,length)
#define __out_bcount_z(size)
#define __out_bcount_z_opt(size)
#define __out_ecount(size)
#define __out_ecount_full(size)
#define __out_ecount_full_opt(size)
#define __out_ecount_full_z(size)
#define __out_ecount_full_z_opt(size)
#define __out_ecount_nz(size)
#define __out_ecount_nz_opt(size)
#define __out_ecount_opt(size)
#define __out_ecount_part(size,length)
#define __out_ecount_part_opt(size,length)
#define __out_ecount_part_z(size,length)
#define __out_ecount_part_z_opt(size,length)
#define __out_ecount_z(size)
#define __out_ecount_z_opt(size)
#define __out_nz
#define __out_nz_opt
#define __out_opt
#define __out_z
#define __out_z_opt

#endif
