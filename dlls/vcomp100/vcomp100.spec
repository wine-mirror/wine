@ stub _vcomp_atomic_add_i1
@ stub _vcomp_atomic_add_i2
@ stub _vcomp_atomic_add_i4
@ stub _vcomp_atomic_add_i8
@ stub _vcomp_atomic_add_r4
@ stub _vcomp_atomic_add_r8
@ stub _vcomp_atomic_and_i1
@ stub _vcomp_atomic_and_i2
@ stub _vcomp_atomic_and_i4
@ stub _vcomp_atomic_and_i8
@ stub _vcomp_atomic_div_i1
@ stub _vcomp_atomic_div_i2
@ stub _vcomp_atomic_div_i4
@ stub _vcomp_atomic_div_i8
@ stub _vcomp_atomic_div_r4
@ stub _vcomp_atomic_div_r8
@ stub _vcomp_atomic_div_ui1
@ stub _vcomp_atomic_div_ui2
@ stub _vcomp_atomic_div_ui4
@ stub _vcomp_atomic_div_ui8
@ stub _vcomp_atomic_mul_i1
@ stub _vcomp_atomic_mul_i2
@ stub _vcomp_atomic_mul_i4
@ stub _vcomp_atomic_mul_i8
@ stub _vcomp_atomic_mul_r4
@ stub _vcomp_atomic_mul_r8
@ stub _vcomp_atomic_or_i1
@ stub _vcomp_atomic_or_i2
@ stub _vcomp_atomic_or_i4
@ stub _vcomp_atomic_or_i8
@ stub _vcomp_atomic_shl_i1
@ stub _vcomp_atomic_shl_i2
@ stub _vcomp_atomic_shl_i4
@ stub _vcomp_atomic_shl_i8
@ stub _vcomp_atomic_shr_i1
@ stub _vcomp_atomic_shr_i2
@ stub _vcomp_atomic_shr_i4
@ stub _vcomp_atomic_shr_i8
@ stub _vcomp_atomic_shr_ui1
@ stub _vcomp_atomic_shr_ui2
@ stub _vcomp_atomic_shr_ui4
@ stub _vcomp_atomic_shr_ui8
@ stub _vcomp_atomic_sub_i1
@ stub _vcomp_atomic_sub_i2
@ stub _vcomp_atomic_sub_i4
@ stub _vcomp_atomic_sub_i8
@ stub _vcomp_atomic_sub_r4
@ stub _vcomp_atomic_sub_r8
@ stub _vcomp_atomic_xor_i1
@ stub _vcomp_atomic_xor_i2
@ stub _vcomp_atomic_xor_i4
@ stub _vcomp_atomic_xor_i8
@ cdecl _vcomp_barrier() vcomp._vcomp_barrier
@ stub _vcomp_copyprivate_broadcast
@ stub _vcomp_copyprivate_receive
@ stub _vcomp_enter_critsect
@ stub _vcomp_flush
@ stub _vcomp_for_dynamic_init
@ stub _vcomp_for_dynamic_init_i8
@ stub _vcomp_for_dynamic_next
@ stub _vcomp_for_dynamic_next_i8
@ stub _vcomp_for_static_end
@ stub _vcomp_for_static_init
@ stub _vcomp_for_static_init_i8
@ stub _vcomp_for_static_simple_init
@ stub _vcomp_for_static_simple_init_i8
@ varargs _vcomp_fork(long long ptr) vcomp._vcomp_fork
@ stub _vcomp_get_thread_num
@ stub _vcomp_leave_critsect
@ stub _vcomp_master_barrier
@ stub _vcomp_master_begin
@ stub _vcomp_master_end
@ stub _vcomp_ordered_begin
@ stub _vcomp_ordered_end
@ stub _vcomp_ordered_loop_end
@ stub _vcomp_reduction_i1
@ stub _vcomp_reduction_i2
@ stub _vcomp_reduction_i4
@ stub _vcomp_reduction_i8
@ stub _vcomp_reduction_r4
@ stub _vcomp_reduction_r8
@ stub _vcomp_reduction_u1
@ stub _vcomp_reduction_u2
@ stub _vcomp_reduction_u4
@ stub _vcomp_reduction_u8
@ stub _vcomp_sections_init
@ stub _vcomp_sections_next
@ cdecl _vcomp_set_num_threads(long) vcomp._vcomp_set_num_threads
@ cdecl _vcomp_single_begin(long) vcomp._vcomp_single_begin
@ cdecl _vcomp_single_end() vcomp._vcomp_single_end
@ stub omp_destroy_lock
@ stub omp_destroy_nest_lock
@ cdecl omp_get_dynamic() vcomp.omp_get_dynamic
@ cdecl omp_get_max_threads() vcomp.omp_get_max_threads
@ cdecl omp_get_nested() vcomp.omp_get_nested
@ cdecl omp_get_num_procs() vcomp.omp_get_num_procs
@ cdecl omp_get_num_threads() vcomp.omp_get_num_threads
@ cdecl omp_get_thread_num() vcomp.omp_get_thread_num
@ stub omp_get_wtick
@ cdecl omp_get_wtime() vcomp.omp_get_wtime
@ stub omp_in_parallel
@ stub omp_init_lock
@ stub omp_init_nest_lock
@ cdecl omp_set_dynamic(long) vcomp.omp_set_dynamic
@ stub omp_set_lock
@ stub omp_set_nest_lock
@ cdecl omp_set_nested(long) vcomp.omp_set_nested
@ cdecl omp_set_num_threads(long) vcomp.omp_set_num_threads
@ stub omp_test_lock
@ stub omp_test_nest_lock
@ stub omp_unset_lock
@ stub omp_unset_nest_lock
