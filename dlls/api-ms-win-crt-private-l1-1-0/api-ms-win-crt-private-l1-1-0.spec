@ cdecl _CreateFrameInfo(ptr ptr) ucrtbase._CreateFrameInfo
@ stdcall _CxxThrowException(ptr ptr) ucrtbase._CxxThrowException
@ cdecl -arch=i386 -norelay _EH_prolog() ucrtbase._EH_prolog
@ cdecl _FindAndUnlinkFrame(ptr) ucrtbase._FindAndUnlinkFrame
@ stub _GetImageBase
@ stub _GetThrowImageBase
@ cdecl _IsExceptionObjectToBeDestroyed(ptr) ucrtbase._IsExceptionObjectToBeDestroyed
@ stub _NLG_Dispatch2
@ stub _NLG_Return
@ stub _NLG_Return2
@ stub _SetImageBase
@ stub _SetThrowImageBase
@ cdecl _SetWinRTOutOfMemoryExceptionCallback(ptr) ucrtbase._SetWinRTOutOfMemoryExceptionCallback
@ cdecl __AdjustPointer(ptr ptr) ucrtbase.__AdjustPointer
@ stub __BuildCatchObject
@ stub __BuildCatchObjectHelper
@ stdcall -arch=x86_64 __C_specific_handler(ptr long ptr ptr) ucrtbase.__C_specific_handler
@ stub __C_specific_handler_noexcept
@ cdecl -arch=i386,x86_64,arm,arm64 __CxxDetectRethrow(ptr) ucrtbase.__CxxDetectRethrow
@ cdecl -arch=i386,x86_64,arm,arm64 __CxxExceptionFilter(ptr ptr long ptr) ucrtbase.__CxxExceptionFilter
@ cdecl -arch=i386,x86_64,arm,arm64 -norelay __CxxFrameHandler(ptr ptr ptr ptr) ucrtbase.__CxxFrameHandler
@ cdecl -arch=i386,x86_64,arm,arm64 -norelay __CxxFrameHandler2(ptr ptr ptr ptr) ucrtbase.__CxxFrameHandler2
@ cdecl -arch=i386,x86_64,arm,arm64 -norelay __CxxFrameHandler3(ptr ptr ptr ptr) ucrtbase.__CxxFrameHandler3
@ stdcall -arch=i386 __CxxLongjmpUnwind(ptr) ucrtbase.__CxxLongjmpUnwind
@ cdecl -arch=i386,x86_64,arm,arm64 __CxxQueryExceptionSize() ucrtbase.__CxxQueryExceptionSize
@ cdecl __CxxRegisterExceptionObject(ptr ptr) ucrtbase.__CxxRegisterExceptionObject
@ cdecl __CxxUnregisterExceptionObject(ptr long) ucrtbase.__CxxUnregisterExceptionObject
@ cdecl __DestructExceptionObject(ptr) ucrtbase.__DestructExceptionObject
@ stub __FrameUnwindFilter
@ stub __GetPlatformExceptionInfo
@ stub __NLG_Dispatch2
@ stub __NLG_Return2
@ cdecl __RTCastToVoid(ptr) ucrtbase.__RTCastToVoid
@ cdecl __RTDynamicCast(ptr long ptr ptr long) ucrtbase.__RTDynamicCast
@ cdecl __RTtypeid(ptr) ucrtbase.__RTtypeid
@ stub __TypeMatch
@ cdecl __current_exception() ucrtbase.__current_exception
@ cdecl __current_exception_context() ucrtbase.__current_exception_context
@ stub __dcrt_get_wide_environment_from_os
@ stub __dcrt_initial_narrow_environment
@ stub __intrinsic_abnormal_termination
@ cdecl -arch=i386,x86_64,arm,arm64 -norelay __intrinsic_setjmp(ptr) ucrtbase.__intrinsic_setjmp
@ cdecl -arch=x86_64,arm64 -norelay __intrinsic_setjmpex(ptr ptr) ucrtbase.__intrinsic_setjmpex
@ cdecl __processing_throw() ucrtbase.__processing_throw
@ stub __report_gsfailure
@ cdecl __std_exception_copy(ptr ptr) ucrtbase.__std_exception_copy
@ cdecl __std_exception_destroy(ptr) ucrtbase.__std_exception_destroy
@ cdecl __std_terminate() ucrtbase.terminate
@ cdecl __std_type_info_compare(ptr ptr) ucrtbase.__std_type_info_compare
@ cdecl __std_type_info_destroy_list(ptr) ucrtbase.__std_type_info_destroy_list
@ cdecl __std_type_info_hash(ptr) ucrtbase.__std_type_info_hash
@ cdecl __std_type_info_name(ptr ptr) ucrtbase.__std_type_info_name
@ cdecl __unDName(ptr str long ptr ptr long) ucrtbase.__unDName
@ cdecl __unDNameEx(ptr str long ptr ptr ptr long) ucrtbase.__unDNameEx
@ cdecl __uncaught_exception() ucrtbase.__uncaught_exception
@ stub __uncaught_exceptions
@ cdecl -arch=i386 -norelay _chkesp() ucrtbase._chkesp
@ cdecl -arch=i386 _except_handler2(ptr ptr ptr ptr) ucrtbase._except_handler2
@ cdecl -arch=i386 _except_handler3(ptr ptr ptr ptr) ucrtbase._except_handler3
@ cdecl -arch=i386 _except_handler4_common(ptr ptr ptr ptr ptr ptr) ucrtbase._except_handler4_common
@ cdecl _get_purecall_handler() ucrtbase._get_purecall_handler
@ cdecl _get_unexpected() ucrtbase._get_unexpected
@ cdecl -arch=i386 _global_unwind2(ptr) ucrtbase._global_unwind2
@ stub _is_exception_typeof
@ cdecl -arch=x86_64 _local_unwind(ptr ptr) ucrtbase._local_unwind
@ cdecl -arch=i386 _local_unwind2(ptr long) ucrtbase._local_unwind2
@ cdecl -arch=i386 _local_unwind4(ptr ptr long) ucrtbase._local_unwind4
@ cdecl -arch=i386 _longjmpex(ptr long) ucrtbase._longjmpex
@ stub _o__CIacos
@ stub _o__CIasin
@ stub _o__CIatan
@ stub _o__CIatan2
@ stub _o__CIcos
@ stub _o__CIcosh
@ stub _o__CIexp
@ stub _o__CIfmod
@ stub _o__CIlog
@ stub _o__CIlog10
@ stub _o__CIpow
@ stub _o__CIsin
@ stub _o__CIsinh
@ stub _o__CIsqrt
@ stub _o__CItan
@ stub _o__CItanh
@ stub _o__Getdays
@ stub _o__Getmonths
@ stub _o__Gettnames
@ stub _o__Strftime
@ stub _o__W_Getdays
@ stub _o__W_Getmonths
@ stub _o__W_Gettnames
@ stub _o__Wcsftime
@ stub _o____lc_codepage_func
@ stub _o____lc_collate_cp_func
@ stub _o____lc_locale_name_func
@ stub _o____mb_cur_max_func
@ cdecl _o___acrt_iob_func(long) ucrtbase._o___acrt_iob_func
@ stub _o___conio_common_vcprintf
@ stub _o___conio_common_vcprintf_p
@ stub _o___conio_common_vcprintf_s
@ stub _o___conio_common_vcscanf
@ stub _o___conio_common_vcwprintf
@ stub _o___conio_common_vcwprintf_p
@ stub _o___conio_common_vcwprintf_s
@ stub _o___conio_common_vcwscanf
@ stub _o___daylight
@ stub _o___dstbias
@ stub _o___fpe_flt_rounds
@ stub _o___libm_sse2_acos
@ stub _o___libm_sse2_acosf
@ stub _o___libm_sse2_asin
@ stub _o___libm_sse2_asinf
@ stub _o___libm_sse2_atan
@ stub _o___libm_sse2_atan2
@ stub _o___libm_sse2_atanf
@ stub _o___libm_sse2_cos
@ stub _o___libm_sse2_cosf
@ stub _o___libm_sse2_exp
@ stub _o___libm_sse2_expf
@ stub _o___libm_sse2_log
@ stub _o___libm_sse2_log10
@ stub _o___libm_sse2_log10f
@ stub _o___libm_sse2_logf
@ stub _o___libm_sse2_pow
@ stub _o___libm_sse2_powf
@ stub _o___libm_sse2_sin
@ stub _o___libm_sse2_sinf
@ stub _o___libm_sse2_tan
@ stub _o___libm_sse2_tanf
@ cdecl _o___p___argc() ucrtbase._o___p___argc
@ stub _o___p___argv
@ cdecl _o___p___wargv() ucrtbase._o___p___wargv
@ stub _o___p__acmdln
@ cdecl _o___p__commode() ucrtbase._o___p__commode
@ stub _o___p__environ
@ stub _o___p__fmode
@ stub _o___p__mbcasemap
@ stub _o___p__mbctype
@ stub _o___p__pgmptr
@ stub _o___p__wcmdln
@ stub _o___p__wenviron
@ stub _o___p__wpgmptr
@ stub _o___pctype_func
@ stub _o___pwctype_func
@ stub _o___std_exception_copy
@ cdecl _o___std_exception_destroy(ptr) ucrtbase._o___std_exception_destroy
@ cdecl _o___std_type_info_destroy_list(ptr) ucrtbase._o___std_type_info_destroy_list
@ stub _o___std_type_info_name
@ cdecl _o___stdio_common_vfprintf(int64 ptr str ptr ptr) ucrtbase._o___stdio_common_vfprintf
@ stub _o___stdio_common_vfprintf_p
@ stub _o___stdio_common_vfprintf_s
@ stub _o___stdio_common_vfscanf
@ cdecl _o___stdio_common_vfwprintf(int64 ptr wstr ptr ptr) ucrtbase._o___stdio_common_vfwprintf
@ stub _o___stdio_common_vfwprintf_p
@ stub _o___stdio_common_vfwprintf_s
@ stub _o___stdio_common_vfwscanf
@ cdecl _o___stdio_common_vsnprintf_s(int64 ptr long long str ptr ptr) ucrtbase._o___stdio_common_vsnprintf_s
@ stub _o___stdio_common_vsnwprintf_s
@ cdecl _o___stdio_common_vsprintf(int64 ptr long str ptr ptr) ucrtbase._o___stdio_common_vsprintf
@ stub _o___stdio_common_vsprintf_p
@ cdecl _o___stdio_common_vsprintf_s(int64 ptr long str ptr ptr) ucrtbase._o___stdio_common_vsprintf_s
@ stub _o___stdio_common_vsscanf
@ cdecl _o___stdio_common_vswprintf(int64 ptr long wstr ptr ptr) ucrtbase._o___stdio_common_vswprintf
@ stub _o___stdio_common_vswprintf_p
@ stub _o___stdio_common_vswprintf_s
@ stub _o___stdio_common_vswscanf
@ stub _o___timezone
@ stub _o___tzname
@ stub _o___wcserror
@ stub _o__access
@ stub _o__access_s
@ stub _o__aligned_free
@ stub _o__aligned_malloc
@ stub _o__aligned_msize
@ stub _o__aligned_offset_malloc
@ stub _o__aligned_offset_realloc
@ stub _o__aligned_offset_recalloc
@ stub _o__aligned_realloc
@ stub _o__aligned_recalloc
@ stub _o__atodbl
@ stub _o__atodbl_l
@ stub _o__atof_l
@ stub _o__atoflt
@ stub _o__atoflt_l
@ stub _o__atoi64
@ stub _o__atoi64_l
@ stub _o__atoi_l
@ stub _o__atol_l
@ stub _o__atoldbl
@ stub _o__atoldbl_l
@ stub _o__atoll_l
@ stub _o__beep
@ stub _o__beginthread
@ stub _o__beginthreadex
@ stub _o__cabs
@ stub _o__callnewh
@ stub _o__calloc_base
@ stub _o__cexit
@ stub _o__cgets
@ stub _o__cgets_s
@ stub _o__cgetws
@ stub _o__cgetws_s
@ stub _o__chdir
@ stub _o__chdrive
@ stub _o__chmod
@ stub _o__chsize
@ stub _o__chsize_s
@ stub _o__close
@ stub _o__commit
@ cdecl _o__configthreadlocale(long) ucrtbase._o__configthreadlocale
@ stub _o__configure_narrow_argv
@ cdecl _o__configure_wide_argv(long) ucrtbase._o__configure_wide_argv
@ cdecl _o__controlfp_s(ptr long long) ucrtbase._o__controlfp_s
@ stub _o__cputs
@ stub _o__cputws
@ stub _o__creat
@ stub _o__create_locale
@ cdecl _o__crt_atexit(ptr) ucrtbase._o__crt_atexit
@ stub _o__ctime32_s
@ stub _o__ctime64_s
@ stub _o__cwait
@ stub _o__d_int
@ stub _o__dclass
@ stub _o__difftime32
@ stub _o__difftime64
@ stub _o__dlog
@ stub _o__dnorm
@ stub _o__dpcomp
@ stub _o__dpoly
@ stub _o__dscale
@ stub _o__dsign
@ stub _o__dsin
@ stub _o__dtest
@ stub _o__dunscale
@ stub _o__dup
@ stub _o__dup2
@ stub _o__dupenv_s
@ stub _o__ecvt
@ stub _o__ecvt_s
@ stub _o__endthread
@ stub _o__endthreadex
@ stub _o__eof
@ cdecl _o__errno() ucrtbase._o__errno
@ stub _o__except1
@ cdecl _o__execute_onexit_table(ptr) ucrtbase._o__execute_onexit_table
@ stub _o__execv
@ stub _o__execve
@ stub _o__execvp
@ stub _o__execvpe
@ stub _o__exit
@ stub _o__expand
@ stub _o__fclose_nolock
@ stub _o__fcloseall
@ stub _o__fcvt
@ stub _o__fcvt_s
@ stub _o__fd_int
@ stub _o__fdclass
@ stub _o__fdexp
@ stub _o__fdlog
@ stub _o__fdopen
@ stub _o__fdpcomp
@ stub _o__fdpoly
@ stub _o__fdscale
@ stub _o__fdsign
@ stub _o__fdsin
@ stub _o__fflush_nolock
@ stub _o__fgetc_nolock
@ stub _o__fgetchar
@ stub _o__fgetwc_nolock
@ stub _o__fgetwchar
@ stub _o__filelength
@ stub _o__filelengthi64
@ cdecl _o__fileno(ptr) ucrtbase._o__fileno
@ stub _o__findclose
@ stub _o__findfirst32
@ stub _o__findfirst32i64
@ stub _o__findfirst64
@ stub _o__findfirst64i32
@ stub _o__findnext32
@ stub _o__findnext32i64
@ stub _o__findnext64
@ stub _o__findnext64i32
@ stub _o__flushall
@ cdecl _o__fpclass(double) ucrtbase._o__fpclass
@ stub _o__fpclassf
@ stub _o__fputc_nolock
@ stub _o__fputchar
@ stub _o__fputwc_nolock
@ stub _o__fputwchar
@ stub _o__fread_nolock
@ stub _o__fread_nolock_s
@ stub _o__free_base
@ stub _o__free_locale
@ stub _o__fseek_nolock
@ stub _o__fseeki64
@ stub _o__fseeki64_nolock
@ stub _o__fsopen
@ stub _o__fstat32
@ stub _o__fstat32i64
@ stub _o__fstat64
@ stub _o__fstat64i32
@ stub _o__ftell_nolock
@ stub _o__ftelli64
@ stub _o__ftelli64_nolock
@ stub _o__ftime32
@ stub _o__ftime32_s
@ stub _o__ftime64
@ stub _o__ftime64_s
@ stub _o__fullpath
@ stub _o__futime32
@ stub _o__futime64
@ stub _o__fwrite_nolock
@ stub _o__gcvt
@ stub _o__gcvt_s
@ stub _o__get_daylight
@ stub _o__get_doserrno
@ stub _o__get_dstbias
@ stub _o__get_errno
@ stub _o__get_fmode
@ stub _o__get_heap_handle
@ stub _o__get_initial_narrow_environment
@ cdecl _o__get_initial_wide_environment() ucrtbase._o__get_initial_wide_environment
@ stub _o__get_invalid_parameter_handler
@ stub _o__get_narrow_winmain_command_line
@ stub _o__get_osfhandle
@ stub _o__get_pgmptr
@ stub _o__get_stream_buffer_pointers
@ stub _o__get_terminate
@ stub _o__get_thread_local_invalid_parameter_handler
@ stub _o__get_timezone
@ stub _o__get_tzname
@ stub _o__get_wide_winmain_command_line
@ stub _o__get_wpgmptr
@ stub _o__getc_nolock
@ stub _o__getch
@ stub _o__getch_nolock
@ stub _o__getche
@ stub _o__getche_nolock
@ stub _o__getcwd
@ stub _o__getdcwd
@ stub _o__getdiskfree
@ stub _o__getdllprocaddr
@ stub _o__getdrive
@ stub _o__getdrives
@ stub _o__getmbcp
@ stub _o__getsystime
@ stub _o__getw
@ stub _o__getwc_nolock
@ stub _o__getwch
@ stub _o__getwch_nolock
@ stub _o__getwche
@ stub _o__getwche_nolock
@ stub _o__getws
@ stub _o__getws_s
@ stub _o__gmtime32
@ stub _o__gmtime32_s
@ stub _o__gmtime64
@ stub _o__gmtime64_s
@ stub _o__heapchk
@ stub _o__heapmin
@ stub _o__hypot
@ stub _o__hypotf
@ stub _o__i64toa
@ stub _o__i64toa_s
@ stub _o__i64tow
@ stub _o__i64tow_s
@ stub _o__initialize_narrow_environment
@ cdecl _o__initialize_onexit_table(ptr) ucrtbase._o__initialize_onexit_table
@ cdecl _o__initialize_wide_environment() ucrtbase._o__initialize_wide_environment
@ stub _o__invalid_parameter_noinfo
@ stub _o__invalid_parameter_noinfo_noreturn
@ stub _o__isatty
@ stub _o__isctype
@ stub _o__isctype_l
@ stub _o__isleadbyte_l
@ stub _o__ismbbalnum
@ stub _o__ismbbalnum_l
@ stub _o__ismbbalpha
@ stub _o__ismbbalpha_l
@ stub _o__ismbbblank
@ stub _o__ismbbblank_l
@ stub _o__ismbbgraph
@ stub _o__ismbbgraph_l
@ stub _o__ismbbkalnum
@ stub _o__ismbbkalnum_l
@ stub _o__ismbbkana
@ stub _o__ismbbkana_l
@ stub _o__ismbbkprint
@ stub _o__ismbbkprint_l
@ stub _o__ismbbkpunct
@ stub _o__ismbbkpunct_l
@ stub _o__ismbblead
@ stub _o__ismbblead_l
@ stub _o__ismbbprint
@ stub _o__ismbbprint_l
@ stub _o__ismbbpunct
@ stub _o__ismbbpunct_l
@ stub _o__ismbbtrail
@ stub _o__ismbbtrail_l
@ stub _o__ismbcalnum
@ stub _o__ismbcalnum_l
@ stub _o__ismbcalpha
@ stub _o__ismbcalpha_l
@ stub _o__ismbcblank
@ stub _o__ismbcblank_l
@ stub _o__ismbcdigit
@ stub _o__ismbcdigit_l
@ stub _o__ismbcgraph
@ stub _o__ismbcgraph_l
@ stub _o__ismbchira
@ stub _o__ismbchira_l
@ stub _o__ismbckata
@ stub _o__ismbckata_l
@ stub _o__ismbcl0
@ stub _o__ismbcl0_l
@ stub _o__ismbcl1
@ stub _o__ismbcl1_l
@ stub _o__ismbcl2
@ stub _o__ismbcl2_l
@ stub _o__ismbclegal
@ stub _o__ismbclegal_l
@ stub _o__ismbclower
@ stub _o__ismbclower_l
@ stub _o__ismbcprint
@ stub _o__ismbcprint_l
@ stub _o__ismbcpunct
@ stub _o__ismbcpunct_l
@ stub _o__ismbcspace
@ stub _o__ismbcspace_l
@ stub _o__ismbcsymbol
@ stub _o__ismbcsymbol_l
@ stub _o__ismbcupper
@ stub _o__ismbcupper_l
@ stub _o__ismbslead
@ stub _o__ismbslead_l
@ stub _o__ismbstrail
@ stub _o__ismbstrail_l
@ stub _o__iswctype_l
@ stub _o__itoa
@ stub _o__itoa_s
@ stub _o__itow
@ stub _o__itow_s
@ stub _o__j0
@ stub _o__j1
@ stub _o__jn
@ stub _o__kbhit
@ stub _o__ld_int
@ stub _o__ldclass
@ stub _o__ldexp
@ stub _o__ldlog
@ stub _o__ldpcomp
@ stub _o__ldpoly
@ stub _o__ldscale
@ stub _o__ldsign
@ stub _o__ldsin
@ stub _o__ldtest
@ stub _o__ldunscale
@ stub _o__lfind
@ stub _o__lfind_s
@ stub _o__libm_sse2_acos_precise
@ stub _o__libm_sse2_asin_precise
@ stub _o__libm_sse2_atan_precise
@ stub _o__libm_sse2_cos_precise
@ stub _o__libm_sse2_exp_precise
@ stub _o__libm_sse2_log10_precise
@ stub _o__libm_sse2_log_precise
@ stub _o__libm_sse2_pow_precise
@ stub _o__libm_sse2_sin_precise
@ stub _o__libm_sse2_sqrt_precise
@ stub _o__libm_sse2_tan_precise
@ stub _o__loaddll
@ stub _o__localtime32
@ stub _o__localtime32_s
@ stub _o__localtime64
@ stub _o__localtime64_s
@ stub _o__lock_file
@ stub _o__locking
@ stub _o__logb
@ stub _o__logbf
@ stub _o__lsearch
@ stub _o__lsearch_s
@ stub _o__lseek
@ stub _o__lseeki64
@ stub _o__ltoa
@ stub _o__ltoa_s
@ stub _o__ltow
@ stub _o__ltow_s
@ stub _o__makepath
@ stub _o__makepath_s
@ stub _o__malloc_base
@ stub _o__mbbtombc
@ stub _o__mbbtombc_l
@ stub _o__mbbtype
@ stub _o__mbbtype_l
@ stub _o__mbccpy
@ stub _o__mbccpy_l
@ stub _o__mbccpy_s
@ stub _o__mbccpy_s_l
@ stub _o__mbcjistojms
@ stub _o__mbcjistojms_l
@ stub _o__mbcjmstojis
@ stub _o__mbcjmstojis_l
@ stub _o__mbclen
@ stub _o__mbclen_l
@ stub _o__mbctohira
@ stub _o__mbctohira_l
@ stub _o__mbctokata
@ stub _o__mbctokata_l
@ stub _o__mbctolower
@ stub _o__mbctolower_l
@ stub _o__mbctombb
@ stub _o__mbctombb_l
@ stub _o__mbctoupper
@ stub _o__mbctoupper_l
@ stub _o__mblen_l
@ stub _o__mbsbtype
@ stub _o__mbsbtype_l
@ stub _o__mbscat_s
@ stub _o__mbscat_s_l
@ stub _o__mbschr
@ stub _o__mbschr_l
@ stub _o__mbscmp
@ stub _o__mbscmp_l
@ stub _o__mbscoll
@ stub _o__mbscoll_l
@ stub _o__mbscpy_s
@ stub _o__mbscpy_s_l
@ stub _o__mbscspn
@ stub _o__mbscspn_l
@ stub _o__mbsdec
@ stub _o__mbsdec_l
@ stub _o__mbsicmp
@ stub _o__mbsicmp_l
@ stub _o__mbsicoll
@ stub _o__mbsicoll_l
@ stub _o__mbsinc
@ stub _o__mbsinc_l
@ stub _o__mbslen
@ stub _o__mbslen_l
@ stub _o__mbslwr
@ stub _o__mbslwr_l
@ stub _o__mbslwr_s
@ stub _o__mbslwr_s_l
@ stub _o__mbsnbcat
@ stub _o__mbsnbcat_l
@ stub _o__mbsnbcat_s
@ stub _o__mbsnbcat_s_l
@ stub _o__mbsnbcmp
@ stub _o__mbsnbcmp_l
@ stub _o__mbsnbcnt
@ stub _o__mbsnbcnt_l
@ stub _o__mbsnbcoll
@ stub _o__mbsnbcoll_l
@ stub _o__mbsnbcpy
@ stub _o__mbsnbcpy_l
@ stub _o__mbsnbcpy_s
@ stub _o__mbsnbcpy_s_l
@ stub _o__mbsnbicmp
@ stub _o__mbsnbicmp_l
@ stub _o__mbsnbicoll
@ stub _o__mbsnbicoll_l
@ stub _o__mbsnbset
@ stub _o__mbsnbset_l
@ stub _o__mbsnbset_s
@ stub _o__mbsnbset_s_l
@ stub _o__mbsncat
@ stub _o__mbsncat_l
@ stub _o__mbsncat_s
@ stub _o__mbsncat_s_l
@ stub _o__mbsnccnt
@ stub _o__mbsnccnt_l
@ stub _o__mbsncmp
@ stub _o__mbsncmp_l
@ stub _o__mbsncoll
@ stub _o__mbsncoll_l
@ stub _o__mbsncpy
@ stub _o__mbsncpy_l
@ stub _o__mbsncpy_s
@ stub _o__mbsncpy_s_l
@ stub _o__mbsnextc
@ stub _o__mbsnextc_l
@ stub _o__mbsnicmp
@ stub _o__mbsnicmp_l
@ stub _o__mbsnicoll
@ stub _o__mbsnicoll_l
@ stub _o__mbsninc
@ stub _o__mbsninc_l
@ stub _o__mbsnlen
@ stub _o__mbsnlen_l
@ stub _o__mbsnset
@ stub _o__mbsnset_l
@ stub _o__mbsnset_s
@ stub _o__mbsnset_s_l
@ stub _o__mbspbrk
@ stub _o__mbspbrk_l
@ stub _o__mbsrchr
@ stub _o__mbsrchr_l
@ stub _o__mbsrev
@ stub _o__mbsrev_l
@ stub _o__mbsset
@ stub _o__mbsset_l
@ stub _o__mbsset_s
@ stub _o__mbsset_s_l
@ stub _o__mbsspn
@ stub _o__mbsspn_l
@ stub _o__mbsspnp
@ stub _o__mbsspnp_l
@ stub _o__mbsstr
@ stub _o__mbsstr_l
@ stub _o__mbstok
@ stub _o__mbstok_l
@ stub _o__mbstok_s
@ stub _o__mbstok_s_l
@ stub _o__mbstowcs_l
@ stub _o__mbstowcs_s_l
@ cdecl _o__mbstrlen(str) ucrtbase._o__mbstrlen
@ stub _o__mbstrlen_l
@ stub _o__mbstrnlen
@ stub _o__mbstrnlen_l
@ stub _o__mbsupr
@ stub _o__mbsupr_l
@ stub _o__mbsupr_s
@ stub _o__mbsupr_s_l
@ stub _o__mbtowc_l
@ stub _o__memicmp
@ stub _o__memicmp_l
@ stub _o__mkdir
@ stub _o__mkgmtime32
@ stub _o__mkgmtime64
@ stub _o__mktemp
@ stub _o__mktemp_s
@ stub _o__mktime32
@ stub _o__mktime64
@ stub _o__msize
@ stub _o__nextafter
@ stub _o__nextafterf
@ stub _o__open_osfhandle
@ stub _o__pclose
@ stub _o__pipe
@ stub _o__popen
@ stub _o__purecall
@ stub _o__putc_nolock
@ stub _o__putch
@ stub _o__putch_nolock
@ stub _o__putenv
@ stub _o__putenv_s
@ stub _o__putw
@ stub _o__putwc_nolock
@ stub _o__putwch
@ stub _o__putwch_nolock
@ stub _o__putws
@ stub _o__read
@ stub _o__realloc_base
@ stub _o__recalloc
@ cdecl _o__register_onexit_function(ptr ptr) ucrtbase._o__register_onexit_function
@ stub _o__resetstkoflw
@ stub _o__rmdir
@ stub _o__rmtmp
@ stub _o__scalb
@ stub _o__scalbf
@ stub _o__searchenv
@ stub _o__searchenv_s
@ cdecl _o__seh_filter_dll(long ptr) ucrtbase._o__seh_filter_dll
@ cdecl _o__seh_filter_exe(long ptr) ucrtbase._o__seh_filter_exe
@ stub _o__set_abort_behavior
@ cdecl _o__set_app_type(long) ucrtbase._o__set_app_type
@ stub _o__set_doserrno
@ stub _o__set_errno
@ cdecl _o__set_fmode(long) ucrtbase._o__set_fmode
@ stub _o__set_invalid_parameter_handler
@ stub _o__set_new_handler
@ cdecl _o__set_new_mode(long) ucrtbase._o__set_new_mode
@ stub _o__set_thread_local_invalid_parameter_handler
@ stub _o__seterrormode
@ stub _o__setmbcp
@ cdecl _o__setmode(long long) ucrtbase._o__setmode
@ stub _o__setsystime
@ stub _o__sleep
@ stub _o__sopen
@ stub _o__sopen_dispatch
@ stub _o__sopen_s
@ stub _o__spawnv
@ stub _o__spawnve
@ stub _o__spawnvp
@ stub _o__spawnvpe
@ stub _o__splitpath
@ stub _o__splitpath_s
@ stub _o__stat32
@ stub _o__stat32i64
@ stub _o__stat64
@ stub _o__stat64i32
@ stub _o__strcoll_l
@ stub _o__strdate
@ stub _o__strdate_s
@ cdecl _o__strdup(str) ucrtbase._o__strdup
@ stub _o__strerror
@ stub _o__strerror_s
@ stub _o__strftime_l
@ cdecl _o__stricmp(str str) ucrtbase._o__stricmp
@ stub _o__stricmp_l
@ stub _o__stricoll
@ stub _o__stricoll_l
@ stub _o__strlwr
@ stub _o__strlwr_l
@ stub _o__strlwr_s
@ stub _o__strlwr_s_l
@ stub _o__strncoll
@ stub _o__strncoll_l
@ cdecl _o__strnicmp(str str long) ucrtbase._o__strnicmp
@ stub _o__strnicmp_l
@ stub _o__strnicoll
@ stub _o__strnicoll_l
@ stub _o__strnset_s
@ stub _o__strset_s
@ stub _o__strtime
@ stub _o__strtime_s
@ stub _o__strtod_l
@ stub _o__strtof_l
@ stub _o__strtoi64
@ stub _o__strtoi64_l
@ stub _o__strtol_l
@ stub _o__strtold_l
@ stub _o__strtoll_l
@ stub _o__strtoui64
@ stub _o__strtoui64_l
@ stub _o__strtoul_l
@ stub _o__strtoull_l
@ stub _o__strupr
@ stub _o__strupr_l
@ stub _o__strupr_s
@ stub _o__strupr_s_l
@ stub _o__strxfrm_l
@ stub _o__swab
@ stub _o__tell
@ stub _o__telli64
@ stub _o__timespec32_get
@ stub _o__timespec64_get
@ stub _o__tolower
@ stub _o__tolower_l
@ stub _o__toupper
@ stub _o__toupper_l
@ stub _o__towlower_l
@ stub _o__towupper_l
@ stub _o__tzset
@ stub _o__ui64toa
@ stub _o__ui64toa_s
@ stub _o__ui64tow
@ stub _o__ui64tow_s
@ stub _o__ultoa
@ stub _o__ultoa_s
@ stub _o__ultow
@ stub _o__ultow_s
@ stub _o__umask
@ stub _o__umask_s
@ stub _o__ungetc_nolock
@ stub _o__ungetch
@ stub _o__ungetch_nolock
@ stub _o__ungetwc_nolock
@ stub _o__ungetwch
@ stub _o__ungetwch_nolock
@ stub _o__unlink
@ stub _o__unloaddll
@ stub _o__unlock_file
@ stub _o__utime32
@ stub _o__utime64
@ stub _o__waccess
@ stub _o__waccess_s
@ stub _o__wasctime
@ stub _o__wasctime_s
@ stub _o__wchdir
@ stub _o__wchmod
@ stub _o__wcreat
@ stub _o__wcreate_locale
@ stub _o__wcscoll_l
@ stub _o__wcsdup
@ stub _o__wcserror
@ stub _o__wcserror_s
@ stub _o__wcsftime_l
@ stub _o__wcsicmp
@ stub _o__wcsicmp_l
@ stub _o__wcsicoll
@ stub _o__wcsicoll_l
@ stub _o__wcslwr
@ stub _o__wcslwr_l
@ stub _o__wcslwr_s
@ stub _o__wcslwr_s_l
@ stub _o__wcsncoll
@ stub _o__wcsncoll_l
@ stub _o__wcsnicmp
@ stub _o__wcsnicmp_l
@ stub _o__wcsnicoll
@ stub _o__wcsnicoll_l
@ stub _o__wcsnset
@ stub _o__wcsnset_s
@ stub _o__wcsset
@ stub _o__wcsset_s
@ stub _o__wcstod_l
@ stub _o__wcstof_l
@ stub _o__wcstoi64
@ stub _o__wcstoi64_l
@ stub _o__wcstol_l
@ stub _o__wcstold_l
@ stub _o__wcstoll_l
@ stub _o__wcstombs_l
@ stub _o__wcstombs_s_l
@ stub _o__wcstoui64
@ stub _o__wcstoui64_l
@ stub _o__wcstoul_l
@ stub _o__wcstoull_l
@ stub _o__wcsupr
@ stub _o__wcsupr_l
@ stub _o__wcsupr_s
@ stub _o__wcsupr_s_l
@ stub _o__wcsxfrm_l
@ stub _o__wctime32
@ stub _o__wctime32_s
@ stub _o__wctime64
@ stub _o__wctime64_s
@ stub _o__wctomb_l
@ stub _o__wctomb_s_l
@ stub _o__wdupenv_s
@ stub _o__wexecv
@ stub _o__wexecve
@ stub _o__wexecvp
@ stub _o__wexecvpe
@ stub _o__wfdopen
@ stub _o__wfindfirst32
@ stub _o__wfindfirst32i64
@ stub _o__wfindfirst64
@ stub _o__wfindfirst64i32
@ stub _o__wfindnext32
@ stub _o__wfindnext32i64
@ stub _o__wfindnext64
@ stub _o__wfindnext64i32
@ cdecl _o__wfopen(wstr wstr) ucrtbase._o__wfopen
@ stub _o__wfopen_s
@ stub _o__wfreopen
@ stub _o__wfreopen_s
@ stub _o__wfsopen
@ stub _o__wfullpath
@ stub _o__wgetcwd
@ stub _o__wgetdcwd
@ cdecl _o__wgetenv(wstr) ucrtbase._o__wgetenv
@ stub _o__wgetenv_s
@ stub _o__wmakepath
@ stub _o__wmakepath_s
@ stub _o__wmkdir
@ stub _o__wmktemp
@ stub _o__wmktemp_s
@ stub _o__wperror
@ stub _o__wpopen
@ stub _o__wputenv
@ stub _o__wputenv_s
@ stub _o__wremove
@ stub _o__wrename
@ stub _o__write
@ stub _o__wrmdir
@ stub _o__wsearchenv
@ stub _o__wsearchenv_s
@ cdecl _o__wsetlocale(long wstr) ucrtbase._o__wsetlocale
@ stub _o__wsopen_dispatch
@ stub _o__wsopen_s
@ stub _o__wspawnv
@ stub _o__wspawnve
@ stub _o__wspawnvp
@ stub _o__wspawnvpe
@ stub _o__wsplitpath
@ stub _o__wsplitpath_s
@ stub _o__wstat32
@ stub _o__wstat32i64
@ stub _o__wstat64
@ stub _o__wstat64i32
@ stub _o__wstrdate
@ stub _o__wstrdate_s
@ stub _o__wstrtime
@ stub _o__wstrtime_s
@ stub _o__wsystem
@ stub _o__wtmpnam_s
@ stub _o__wtof
@ stub _o__wtof_l
@ stub _o__wtoi
@ stub _o__wtoi64
@ stub _o__wtoi64_l
@ stub _o__wtoi_l
@ stub _o__wtol
@ stub _o__wtol_l
@ stub _o__wtoll
@ stub _o__wtoll_l
@ stub _o__wunlink
@ stub _o__wutime32
@ stub _o__wutime64
@ stub _o__y0
@ stub _o__y1
@ stub _o__yn
@ cdecl _o_abort() ucrtbase._o_abort
@ cdecl _o_acos(double) ucrtbase._o_acos
@ cdecl -arch=!i386 _o_acosf(float) ucrtbase._o_acosf
@ cdecl _o_acosh(double) ucrtbase._o_acosh
@ cdecl _o_acoshf(float) ucrtbase._o_acoshf
@ cdecl _o_acoshl(double) ucrtbase._o_acoshl
@ cdecl _o_asctime(ptr) ucrtbase._o_asctime
@ cdecl _o_asctime_s(ptr long ptr) ucrtbase._o_asctime_s
@ cdecl _o_asin(double) ucrtbase._o_asin
@ cdecl -arch=!i386 _o_asinf(float) ucrtbase._o_asinf
@ cdecl _o_asinh(double) ucrtbase._o_asinh
@ cdecl _o_asinhf(float) ucrtbase._o_asinhf
@ cdecl _o_asinhl(double) ucrtbase._o_asinhl
@ cdecl _o_atan(double) ucrtbase._o_atan
@ cdecl _o_atan2(double double) ucrtbase._o_atan2
@ cdecl -arch=!i386 _o_atan2f(float float) ucrtbase._o_atan2f
@ cdecl -arch=!i386 _o_atanf(float) ucrtbase._o_atanf
@ cdecl _o_atanh(double) ucrtbase._o_atanh
@ cdecl _o_atanhf(float) ucrtbase._o_atanhf
@ cdecl _o_atanhl(double) ucrtbase._o_atanhl
@ cdecl _o_atof(str) ucrtbase._o_atof
@ cdecl _o_atoi(str) ucrtbase._o_atoi
@ cdecl _o_atol(str) ucrtbase._o_atol
@ cdecl -ret64 _o_atoll(str) ucrtbase._o_atoll
@ cdecl _o_bsearch(ptr ptr long long ptr) ucrtbase._o_bsearch
@ cdecl _o_bsearch_s(ptr ptr long long ptr ptr) ucrtbase._o_bsearch_s
@ cdecl _o_btowc(long) ucrtbase._o_btowc
@ cdecl _o_calloc(long long) ucrtbase._o_calloc
@ cdecl _o_cbrt(double) ucrtbase._o_cbrt
@ cdecl _o_cbrtf(float) ucrtbase._o_cbrtf
@ cdecl _o_ceil(double) ucrtbase._o_ceil
@ cdecl -arch=!i386 _o_ceilf(float) ucrtbase._o_ceilf
@ cdecl _o_clearerr(ptr) ucrtbase._o_clearerr
@ cdecl _o_clearerr_s(ptr) ucrtbase._o_clearerr_s
@ cdecl _o_cos(double) ucrtbase._o_cos
@ cdecl -arch=!i386 _o_cosf(float) ucrtbase._o_cosf
@ cdecl _o_cosh(double) ucrtbase._o_cosh
@ cdecl -arch=!i386 _o_coshf(float) ucrtbase._o_coshf
@ cdecl _o_erf(double) ucrtbase._o_erf
@ cdecl _o_erfc(double) ucrtbase._o_erfc
@ cdecl _o_erfcf(float) ucrtbase._o_erfcf
@ cdecl _o_erfcl(double) ucrtbase._o_erfcl
@ cdecl _o_erff(float) ucrtbase._o_erff
@ cdecl _o_erfl(double) ucrtbase._o_erfl
@ cdecl _o_exit(long) ucrtbase._o_exit
@ cdecl _o_exp(double) ucrtbase._o_exp
@ cdecl _o_exp2(double) ucrtbase._o_exp2
@ cdecl _o_exp2f(float) ucrtbase._o_exp2f
@ cdecl _o_exp2l(double) ucrtbase._o_exp2l
@ cdecl -arch=!i386 _o_expf(float) ucrtbase._o_expf
@ cdecl _o_fabs(double) ucrtbase._o_fabs
@ cdecl _o_fclose(ptr) ucrtbase._o_fclose
@ cdecl _o_feof(ptr) ucrtbase._o_feof
@ cdecl _o_ferror(ptr) ucrtbase._o_ferror
@ cdecl _o_fflush(ptr) ucrtbase._o_fflush
@ cdecl _o_fgetc(ptr) ucrtbase._o_fgetc
@ cdecl _o_fgetpos(ptr ptr) ucrtbase._o_fgetpos
@ cdecl _o_fgets(ptr long ptr) ucrtbase._o_fgets
@ cdecl _o_fgetwc(ptr) ucrtbase._o_fgetwc
@ cdecl _o_fgetws(ptr long ptr) ucrtbase._o_fgetws
@ cdecl _o_floor(double) ucrtbase._o_floor
@ cdecl -arch=!i386 _o_floorf(float) ucrtbase._o_floorf
@ cdecl _o_fma(double double double) ucrtbase._o_fma
@ cdecl _o_fmaf(float float float) ucrtbase._o_fmaf
@ cdecl _o_fmal(double double double) ucrtbase._o_fmal
@ cdecl _o_fmod(double double) ucrtbase._o_fmod
@ cdecl -arch=!i386 _o_fmodf(float float) ucrtbase._o_fmodf
@ cdecl _o_fopen(str str) ucrtbase._o_fopen
@ cdecl _o_fopen_s(ptr str str) ucrtbase._o_fopen_s
@ cdecl _o_fputc(long ptr) ucrtbase._o_fputc
@ cdecl _o_fputs(str ptr) ucrtbase._o_fputs
@ cdecl _o_fputwc(long ptr) ucrtbase._o_fputwc
@ cdecl _o_fputws(wstr ptr) ucrtbase._o_fputws
@ cdecl _o_fread(ptr long long ptr) ucrtbase._o_fread
@ cdecl _o_fread_s(ptr long long long ptr) ucrtbase._o_fread_s
@ cdecl _o_free(ptr) ucrtbase._o_free
@ cdecl _o_freopen(str str ptr) ucrtbase._o_freopen
@ cdecl _o_freopen_s(ptr str str ptr) ucrtbase._o_freopen_s
@ cdecl _o_frexp(double ptr) ucrtbase._o_frexp
@ cdecl _o_fseek(ptr long long) ucrtbase._o_fseek
@ cdecl _o_fsetpos(ptr ptr) ucrtbase._o_fsetpos
@ cdecl _o_ftell(ptr) ucrtbase._o_ftell
@ cdecl _o_fwrite(ptr long long ptr) ucrtbase._o_fwrite
@ cdecl _o_getc(ptr) ucrtbase._o_getc
@ cdecl _o_getchar() ucrtbase._o_getchar
@ cdecl _o_getenv(str) ucrtbase._o_getenv
@ cdecl _o_getenv_s(ptr ptr long str) ucrtbase._o_getenv_s
@ cdecl _o_gets(str) ucrtbase._o_gets
@ cdecl _o_gets_s(ptr long) ucrtbase._o_gets_s
@ cdecl _o_getwc(ptr) ucrtbase._o_getwc
@ cdecl _o_getwchar() ucrtbase._o_getwchar
@ cdecl _o_hypot(double double) ucrtbase._o_hypot
@ cdecl _o_is_wctype(long long) ucrtbase._o_is_wctype
@ cdecl _o_isalnum(long) ucrtbase._o_isalnum
@ cdecl _o_isalpha(long) ucrtbase._o_isalpha
@ cdecl _o_isblank(long) ucrtbase._o_isblank
@ cdecl _o_iscntrl(long) ucrtbase._o_iscntrl
@ cdecl _o_isdigit(long) ucrtbase._o_isdigit
@ cdecl _o_isgraph(long) ucrtbase._o_isgraph
@ cdecl _o_isleadbyte(long) ucrtbase._o_isleadbyte
@ cdecl _o_islower(long) ucrtbase._o_islower
@ cdecl _o_isprint(long) ucrtbase._o_isprint
@ cdecl _o_ispunct(long) ucrtbase._o_ispunct
@ cdecl _o_isspace(long) ucrtbase._o_isspace
@ cdecl _o_isupper(long) ucrtbase._o_isupper
@ cdecl _o_iswalnum(long) ucrtbase._o_iswalnum
@ cdecl _o_iswalpha(long) ucrtbase._o_iswalpha
@ cdecl _o_iswascii(long) ucrtbase._o_iswascii
@ cdecl _o_iswblank(long) ucrtbase._o_iswblank
@ cdecl _o_iswcntrl(long) ucrtbase._o_iswcntrl
@ cdecl _o_iswctype(long long) ucrtbase._o_iswctype
@ cdecl _o_iswdigit(long) ucrtbase._o_iswdigit
@ cdecl _o_iswgraph(long) ucrtbase._o_iswgraph
@ cdecl _o_iswlower(long) ucrtbase._o_iswlower
@ cdecl _o_iswprint(long) ucrtbase._o_iswprint
@ cdecl _o_iswpunct(long) ucrtbase._o_iswpunct
@ cdecl _o_iswspace(long) ucrtbase._o_iswspace
@ cdecl _o_iswupper(long) ucrtbase._o_iswupper
@ cdecl _o_iswxdigit(long) ucrtbase._o_iswxdigit
@ cdecl _o_isxdigit(long) ucrtbase._o_isxdigit
@ cdecl _o_ldexp(double long) ucrtbase._o_ldexp
@ cdecl _o_lgamma(double) ucrtbase._o_lgamma
@ cdecl _o_lgammaf(float) ucrtbase._o_lgammaf
@ cdecl _o_lgammal(double) ucrtbase._o_lgammal
@ cdecl -ret64 _o_llrint(double) ucrtbase._o_llrint
@ cdecl -ret64 _o_llrintf(float) ucrtbase._o_llrintf
@ cdecl -ret64 _o_llrintl(double) ucrtbase._o_llrintl
@ cdecl -ret64 _o_llround(double) ucrtbase._o_llround
@ cdecl -ret64 _o_llroundf(float) ucrtbase._o_llroundf
@ cdecl -ret64 _o_llroundl(double) ucrtbase._o_llroundl
@ cdecl _o_localeconv() ucrtbase._o_localeconv
@ cdecl _o_log(double) ucrtbase._o_log
@ cdecl _o_log10(double) ucrtbase._o_log10
@ cdecl -arch=!i386 _o_log10f(float) ucrtbase._o_log10f
@ cdecl _o_log1p(double) ucrtbase._o_log1p
@ cdecl _o_log1pf(float) ucrtbase._o_log1pf
@ cdecl _o_log1pl(double) ucrtbase._o_log1pl
@ cdecl _o_log2(double) ucrtbase._o_log2
@ cdecl _o_log2f(float) ucrtbase._o_log2f
@ cdecl _o_log2l(double) ucrtbase._o_log2l
@ cdecl _o_logb(double) ucrtbase._o_logb
@ cdecl _o_logbf(float) ucrtbase._o_logbf
@ cdecl _o_logbl(double) ucrtbase._o_logbl
@ cdecl -arch=!i386 _o_logf(float) ucrtbase._o_logf
@ cdecl _o_lrint(double) ucrtbase._o_lrint
@ cdecl _o_lrintf(float) ucrtbase._o_lrintf
@ cdecl _o_lrintl(double) ucrtbase._o_lrintl
@ cdecl _o_lround(double) ucrtbase._o_lround
@ cdecl _o_lroundf(float) ucrtbase._o_lroundf
@ cdecl _o_lroundl(double) ucrtbase._o_lroundl
@ cdecl _o_malloc(long) ucrtbase._o_malloc
@ cdecl _o_mblen(ptr long) ucrtbase._o_mblen
@ cdecl _o_mbrlen(ptr long ptr) ucrtbase._o_mbrlen
@ stub _o_mbrtoc16
@ stub _o_mbrtoc32
@ cdecl _o_mbrtowc(ptr str long ptr) ucrtbase._o_mbrtowc
@ cdecl _o_mbsrtowcs(ptr ptr long ptr) ucrtbase._o_mbsrtowcs
@ cdecl _o_mbsrtowcs_s(ptr ptr long ptr long ptr) ucrtbase._o_mbsrtowcs_s
@ cdecl _o_mbstowcs(ptr str long) ucrtbase._o_mbstowcs
@ cdecl _o_mbstowcs_s(ptr ptr long str long) ucrtbase._o_mbstowcs_s
@ cdecl _o_mbtowc(ptr str long) ucrtbase._o_mbtowc
@ cdecl _o_memcpy_s(ptr long ptr long) ucrtbase._o_memcpy_s
@ cdecl _o_modf(double ptr) ucrtbase._o_modf
@ cdecl -arch=!i386 _o_modff(float ptr) ucrtbase._o_modff
@ cdecl _o_nan(str) ucrtbase._o_nan
@ cdecl _o_nanf(str) ucrtbase._o_nanf
@ cdecl _o_nanl(str) ucrtbase._o_nanl
@ cdecl _o_nearbyint(double) ucrtbase._o_nearbyint
@ cdecl _o_nearbyintf(float) ucrtbase._o_nearbyintf
@ cdecl _o_nearbyintl(double) ucrtbase._o_nearbyintl
@ cdecl _o_nextafter(double double) ucrtbase._o_nextafter
@ cdecl _o_nextafterf(float float) ucrtbase._o_nextafterf
@ cdecl _o_nextafterl(double double) ucrtbase._o_nextafterl
@ cdecl _o_nexttoward(double double) ucrtbase._o_nexttoward
@ cdecl _o_nexttowardf(float double) ucrtbase._o_nexttowardf
@ cdecl _o_nexttowardl(double double) ucrtbase._o_nexttowardl
@ cdecl _o_pow(double double) ucrtbase._o_pow
@ cdecl -arch=!i386 _o_powf(float float) ucrtbase._o_powf
@ cdecl _o_putc(long ptr) ucrtbase._o_putc
@ cdecl _o_putchar(long) ucrtbase._o_putchar
@ cdecl _o_puts(str) ucrtbase._o_puts
@ cdecl _o_putwc(long ptr) ucrtbase._o_putwc
@ cdecl _o_putwchar(long) ucrtbase._o_putwchar
@ cdecl _o_qsort(ptr long long ptr) ucrtbase._o_qsort
@ cdecl _o_qsort_s(ptr long long ptr ptr) ucrtbase._o_qsort_s
@ cdecl _o_raise(long) ucrtbase._o_raise
@ cdecl _o_rand() ucrtbase._o_rand
@ cdecl _o_rand_s(ptr) ucrtbase._o_rand_s
@ cdecl _o_realloc(ptr long) ucrtbase._o_realloc
@ cdecl _o_remainder(double double) ucrtbase._o_remainder
@ cdecl _o_remainderf(float float) ucrtbase._o_remainderf
@ cdecl _o_remainderl(double double) ucrtbase._o_remainderl
@ cdecl _o_remove(str) ucrtbase._o_remove
@ cdecl _o_remquo(double double ptr) ucrtbase._o_remquo
@ cdecl _o_remquof(float float ptr) ucrtbase._o_remquof
@ cdecl _o_remquol(double double ptr) ucrtbase._o_remquol
@ cdecl _o_rename(str str) ucrtbase._o_rename
@ cdecl _o_rewind(ptr) ucrtbase._o_rewind
@ cdecl _o_rint(double) ucrtbase._o_rint
@ cdecl _o_rintf(float) ucrtbase._o_rintf
@ cdecl _o_rintl(double) ucrtbase._o_rintl
@ cdecl _o_round(double) ucrtbase._o_round
@ cdecl _o_roundf(float) ucrtbase._o_roundf
@ cdecl _o_roundl(double) ucrtbase._o_roundl
@ cdecl _o_scalbln(double long) ucrtbase._o_scalbln
@ cdecl _o_scalblnf(float long) ucrtbase._o_scalblnf
@ cdecl _o_scalblnl(double long) ucrtbase._o_scalblnl
@ cdecl _o_scalbn(double long) ucrtbase._o_scalbn
@ cdecl _o_scalbnf(float long) ucrtbase._o_scalbnf
@ cdecl _o_scalbnl(double long) ucrtbase._o_scalbnl
@ cdecl _o_set_terminate(ptr) ucrtbase._o_set_terminate
@ cdecl _o_setbuf(ptr ptr) ucrtbase._o_setbuf
@ cdecl _o_setlocale(long str) ucrtbase._o_setlocale
@ cdecl _o_setvbuf(ptr str long long) ucrtbase._o_setvbuf
@ cdecl _o_sin(double) ucrtbase._o_sin
@ cdecl -arch=!i386 _o_sinf(float) ucrtbase._o_sinf
@ cdecl _o_sinh(double) ucrtbase._o_sinh
@ cdecl -arch=!i386 _o_sinhf(float) ucrtbase._o_sinhf
@ cdecl _o_sqrt(double) ucrtbase._o_sqrt
@ cdecl -arch=!i386 _o_sqrtf(float) ucrtbase._o_sqrtf
@ cdecl _o_srand(long) ucrtbase._o_srand
@ cdecl _o_strcat_s(str long str) ucrtbase._o_strcat_s
@ cdecl _o_strcoll(str str) ucrtbase._o_strcoll
@ cdecl _o_strcpy_s(ptr long str) ucrtbase._o_strcpy_s
@ cdecl _o_strerror(long) ucrtbase._o_strerror
@ cdecl _o_strerror_s(ptr long long) ucrtbase._o_strerror_s
@ cdecl _o_strftime(ptr long str ptr) ucrtbase._o_strftime
@ cdecl _o_strncat_s(str long str long) ucrtbase._o_strncat_s
@ cdecl _o_strncpy_s(ptr long str long) ucrtbase._o_strncpy_s
@ cdecl _o_strtod(str ptr) ucrtbase._o_strtod
@ cdecl _o_strtof(str ptr) ucrtbase._o_strtof
@ cdecl _o_strtok(str str) ucrtbase._o_strtok
@ cdecl _o_strtok_s(ptr str ptr) ucrtbase._o_strtok_s
@ cdecl _o_strtol(str ptr long) ucrtbase._o_strtol
@ stub _o_strtold
@ cdecl -ret64 _o_strtoll(str ptr long) ucrtbase._o_strtoll
@ cdecl _o_strtoul(str ptr long) ucrtbase._o_strtoul
@ cdecl -ret64 _o_strtoull(str ptr long) ucrtbase._o_strtoull
@ cdecl _o_system(str) ucrtbase._o_system
@ cdecl _o_tan(double) ucrtbase._o_tan
@ cdecl -arch=!i386 _o_tanf(float) ucrtbase._o_tanf
@ cdecl _o_tanh(double) ucrtbase._o_tanh
@ cdecl -arch=!i386 _o_tanhf(float) ucrtbase._o_tanhf
@ cdecl _o_terminate() ucrtbase._o_terminate
@ cdecl _o_tgamma(double) ucrtbase._o_tgamma
@ cdecl _o_tgammaf(float) ucrtbase._o_tgammaf
@ cdecl _o_tgammal(double) ucrtbase._o_tgammal
@ cdecl _o_tmpfile_s(ptr) ucrtbase._o_tmpfile_s
@ cdecl _o_tmpnam_s(ptr long) ucrtbase._o_tmpnam_s
@ cdecl _o_tolower(long) ucrtbase._o_tolower
@ cdecl _o_toupper(long) ucrtbase._o_toupper
@ cdecl _o_towlower(long) ucrtbase._o_towlower
@ cdecl _o_towupper(long) ucrtbase._o_towupper
@ cdecl _o_ungetc(long ptr) ucrtbase._o_ungetc
@ cdecl _o_ungetwc(long ptr) ucrtbase._o_ungetwc
@ cdecl _o_wcrtomb(ptr long ptr) ucrtbase._o_wcrtomb
@ cdecl _o_wcrtomb_s(ptr ptr long long ptr) ucrtbase._o_wcrtomb_s
@ cdecl _o_wcscat_s(wstr long wstr) ucrtbase._o_wcscat_s
@ cdecl _o_wcscoll(wstr wstr) ucrtbase._o_wcscoll
@ cdecl _o_wcscpy(ptr wstr) ucrtbase._o_wcscpy
@ cdecl _o_wcscpy_s(ptr long wstr) ucrtbase._o_wcscpy_s
@ cdecl _o_wcsftime(ptr long wstr ptr) ucrtbase._o_wcsftime
@ cdecl _o_wcsncat_s(wstr long wstr long) ucrtbase._o_wcsncat_s
@ cdecl _o_wcsncpy_s(ptr long wstr long) ucrtbase._o_wcsncpy_s
@ cdecl _o_wcsrtombs(ptr ptr long ptr) ucrtbase._o_wcsrtombs
@ cdecl _o_wcsrtombs_s(ptr ptr long ptr long ptr) ucrtbase._o_wcsrtombs_s
@ cdecl _o_wcstod(wstr ptr) ucrtbase._o_wcstod
@ cdecl _o_wcstof(ptr ptr) ucrtbase._o_wcstof
@ cdecl _o_wcstok(wstr wstr ptr) ucrtbase._o_wcstok
@ cdecl _o_wcstok_s(ptr wstr ptr) ucrtbase._o_wcstok_s
@ cdecl _o_wcstol(wstr ptr long) ucrtbase._o_wcstol
@ stub _o_wcstold
@ cdecl -ret64 _o_wcstoll(wstr ptr long) ucrtbase._o_wcstoll
@ cdecl _o_wcstombs(ptr ptr long) ucrtbase._o_wcstombs
@ cdecl _o_wcstombs_s(ptr ptr long wstr long) ucrtbase._o_wcstombs_s
@ cdecl _o_wcstoul(wstr ptr long) ucrtbase._o_wcstoul
@ cdecl -ret64 _o_wcstoull(wstr ptr long) ucrtbase._o_wcstoull
@ cdecl _o_wctob(long) ucrtbase._o_wctob
@ cdecl _o_wctomb(ptr long) ucrtbase._o_wctomb
@ cdecl _o_wctomb_s(ptr ptr long long) ucrtbase._o_wctomb_s
@ cdecl _o_wmemcpy_s(ptr long ptr long) ucrtbase._o_wmemcpy_s
@ cdecl _o_wmemmove_s(ptr long ptr long) ucrtbase._o_wmemmove_s
@ cdecl _purecall() ucrtbase._purecall
@ stdcall -arch=i386 _seh_longjmp_unwind(ptr) ucrtbase._seh_longjmp_unwind
@ stdcall -arch=i386 _seh_longjmp_unwind4(ptr) ucrtbase._seh_longjmp_unwind4
@ cdecl _set_purecall_handler(ptr) ucrtbase._set_purecall_handler
@ cdecl _set_se_translator(ptr) ucrtbase._set_se_translator
@ cdecl -arch=i386 -norelay _setjmp3(ptr long) ucrtbase._setjmp3
@ cdecl -arch=i386,x86_64,arm,arm64 longjmp(ptr long) ucrtbase.longjmp
@ cdecl memchr(ptr long long) ucrtbase.memchr
@ cdecl memcmp(ptr ptr long) ucrtbase.memcmp
@ cdecl memcpy(ptr ptr long) ucrtbase.memcpy
@ cdecl memmove(ptr ptr long) ucrtbase.memmove
@ cdecl set_unexpected(ptr) ucrtbase.set_unexpected
@ cdecl -arch=arm,x86_64 -norelay -private setjmp(ptr) ucrtbase.setjmp
@ cdecl strchr(str long) ucrtbase.strchr
@ cdecl strrchr(str long) ucrtbase.strrchr
@ cdecl strstr(str str) ucrtbase.strstr
@ stub unexpected
@ cdecl wcschr(wstr long) ucrtbase.wcschr
@ cdecl wcsrchr(wstr long) ucrtbase.wcsrchr
@ cdecl wcsstr(wstr wstr) ucrtbase.wcsstr
