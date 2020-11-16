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
@ stdcall -arch=x86_64,arm64 __C_specific_handler(ptr long ptr ptr) ucrtbase.__C_specific_handler
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
@ cdecl -arch=x86_64,arm64 _local_unwind(ptr ptr) ucrtbase._local_unwind
@ cdecl -arch=i386 _local_unwind2(ptr long) ucrtbase._local_unwind2
@ cdecl -arch=i386 _local_unwind4(ptr ptr long) ucrtbase._local_unwind4
@ cdecl -arch=i386 _longjmpex(ptr long) ucrtbase._longjmpex
@ cdecl -arch=i386 _o__CIacos() ucrtbase._o__CIacos
@ cdecl -arch=i386 _o__CIasin() ucrtbase._o__CIasin
@ cdecl -arch=i386 _o__CIatan() ucrtbase._o__CIatan
@ cdecl -arch=i386 _o__CIatan2() ucrtbase._o__CIatan2
@ cdecl -arch=i386 _o__CIcos() ucrtbase._o__CIcos
@ cdecl -arch=i386 _o__CIcosh() ucrtbase._o__CIcosh
@ cdecl -arch=i386 _o__CIexp() ucrtbase._o__CIexp
@ cdecl -arch=i386 _o__CIfmod() ucrtbase._o__CIfmod
@ cdecl -arch=i386 _o__CIlog() ucrtbase._o__CIlog
@ cdecl -arch=i386 _o__CIlog10() ucrtbase._o__CIlog10
@ cdecl -arch=i386 _o__CIpow() ucrtbase._o__CIpow
@ cdecl -arch=i386 _o__CIsin() ucrtbase._o__CIsin
@ cdecl -arch=i386 _o__CIsinh() ucrtbase._o__CIsinh
@ cdecl -arch=i386 _o__CIsqrt() ucrtbase._o__CIsqrt
@ cdecl -arch=i386 _o__CItan() ucrtbase._o__CItan
@ cdecl -arch=i386 _o__CItanh() ucrtbase._o__CItanh
@ cdecl _o__Getdays() ucrtbase._o__Getdays
@ cdecl _o__Getmonths() ucrtbase._o__Getmonths
@ cdecl _o__Gettnames() ucrtbase._o__Gettnames
@ cdecl _o__Strftime(ptr long str ptr ptr) ucrtbase._o__Strftime
@ cdecl _o__W_Getdays() ucrtbase._o__W_Getdays
@ cdecl _o__W_Getmonths() ucrtbase._o__W_Getmonths
@ cdecl _o__W_Gettnames() ucrtbase._o__W_Gettnames
@ cdecl _o__Wcsftime(ptr long wstr ptr ptr) ucrtbase._o__Wcsftime
@ cdecl _o____lc_codepage_func() ucrtbase._o____lc_codepage_func
@ cdecl _o____lc_collate_cp_func() ucrtbase._o____lc_collate_cp_func
@ cdecl _o____lc_locale_name_func() ucrtbase._o____lc_locale_name_func
@ cdecl _o____mb_cur_max_func() ucrtbase._o____mb_cur_max_func
@ cdecl _o___acrt_iob_func(long) ucrtbase._o___acrt_iob_func
@ cdecl _o___conio_common_vcprintf(int64 str ptr ptr) ucrtbase._o___conio_common_vcprintf
@ stub _o___conio_common_vcprintf_p
@ stub _o___conio_common_vcprintf_s
@ stub _o___conio_common_vcscanf
@ cdecl _o___conio_common_vcwprintf(int64 wstr ptr ptr) ucrtbase._o___conio_common_vcwprintf
@ stub _o___conio_common_vcwprintf_p
@ stub _o___conio_common_vcwprintf_s
@ stub _o___conio_common_vcwscanf
@ cdecl _o___daylight() ucrtbase._o___daylight
@ cdecl _o___dstbias() ucrtbase._o___dstbias
@ cdecl _o___fpe_flt_rounds() ucrtbase._o___fpe_flt_rounds
@ cdecl -arch=i386 -norelay _o___libm_sse2_acos() ucrtbase._o___libm_sse2_acos
@ cdecl -arch=i386 -norelay _o___libm_sse2_acosf() ucrtbase._o___libm_sse2_acosf
@ cdecl -arch=i386 -norelay _o___libm_sse2_asin() ucrtbase._o___libm_sse2_asin
@ cdecl -arch=i386 -norelay _o___libm_sse2_asinf() ucrtbase._o___libm_sse2_asinf
@ cdecl -arch=i386 -norelay _o___libm_sse2_atan() ucrtbase._o___libm_sse2_atan
@ cdecl -arch=i386 -norelay _o___libm_sse2_atan2() ucrtbase._o___libm_sse2_atan2
@ cdecl -arch=i386 -norelay _o___libm_sse2_atanf() ucrtbase._o___libm_sse2_atanf
@ cdecl -arch=i386 -norelay _o___libm_sse2_cos() ucrtbase._o___libm_sse2_cos
@ cdecl -arch=i386 -norelay _o___libm_sse2_cosf() ucrtbase._o___libm_sse2_cosf
@ cdecl -arch=i386 -norelay _o___libm_sse2_exp() ucrtbase._o___libm_sse2_exp
@ cdecl -arch=i386 -norelay _o___libm_sse2_expf() ucrtbase._o___libm_sse2_expf
@ cdecl -arch=i386 -norelay _o___libm_sse2_log() ucrtbase._o___libm_sse2_log
@ cdecl -arch=i386 -norelay _o___libm_sse2_log10() ucrtbase._o___libm_sse2_log10
@ cdecl -arch=i386 -norelay _o___libm_sse2_log10f() ucrtbase._o___libm_sse2_log10f
@ cdecl -arch=i386 -norelay _o___libm_sse2_logf() ucrtbase._o___libm_sse2_logf
@ cdecl -arch=i386 -norelay _o___libm_sse2_pow() ucrtbase._o___libm_sse2_pow
@ cdecl -arch=i386 -norelay _o___libm_sse2_powf() ucrtbase._o___libm_sse2_powf
@ cdecl -arch=i386 -norelay _o___libm_sse2_sin() ucrtbase._o___libm_sse2_sin
@ cdecl -arch=i386 -norelay _o___libm_sse2_sinf() ucrtbase._o___libm_sse2_sinf
@ cdecl -arch=i386 -norelay _o___libm_sse2_tan() ucrtbase._o___libm_sse2_tan
@ cdecl -arch=i386 -norelay _o___libm_sse2_tanf() ucrtbase._o___libm_sse2_tanf
@ cdecl _o___p___argc() ucrtbase._o___p___argc
@ cdecl _o___p___argv() ucrtbase._o___p___argv
@ cdecl _o___p___wargv() ucrtbase._o___p___wargv
@ cdecl _o___p__acmdln() ucrtbase._o___p__acmdln
@ cdecl _o___p__commode() ucrtbase._o___p__commode
@ cdecl _o___p__environ() ucrtbase._o___p__environ
@ cdecl _o___p__fmode() ucrtbase._o___p__fmode
@ stub _o___p__mbcasemap
@ cdecl _o___p__mbctype() ucrtbase._o___p__mbctype
@ cdecl _o___p__pgmptr() ucrtbase._o___p__pgmptr
@ cdecl _o___p__wcmdln() ucrtbase._o___p__wcmdln
@ cdecl _o___p__wenviron() ucrtbase._o___p__wenviron
@ cdecl _o___p__wpgmptr() ucrtbase._o___p__wpgmptr
@ cdecl _o___pctype_func() ucrtbase._o___pctype_func
@ stub _o___pwctype_func
@ cdecl _o___std_exception_copy(ptr ptr) ucrtbase._o___std_exception_copy
@ cdecl _o___std_exception_destroy(ptr) ucrtbase._o___std_exception_destroy
@ cdecl _o___std_type_info_destroy_list(ptr) ucrtbase._o___std_type_info_destroy_list
@ cdecl _o___std_type_info_name(ptr ptr) ucrtbase._o___std_type_info_name
@ cdecl _o___stdio_common_vfprintf(int64 ptr str ptr ptr) ucrtbase._o___stdio_common_vfprintf
@ stub _o___stdio_common_vfprintf_p
@ cdecl _o___stdio_common_vfprintf_s(int64 ptr str ptr ptr) ucrtbase._o___stdio_common_vfprintf_s
@ cdecl _o___stdio_common_vfscanf(int64 ptr str ptr ptr) ucrtbase._o___stdio_common_vfscanf
@ cdecl _o___stdio_common_vfwprintf(int64 ptr wstr ptr ptr) ucrtbase._o___stdio_common_vfwprintf
@ stub _o___stdio_common_vfwprintf_p
@ cdecl _o___stdio_common_vfwprintf_s(int64 ptr wstr ptr ptr) ucrtbase._o___stdio_common_vfwprintf_s
@ cdecl _o___stdio_common_vfwscanf(int64 ptr wstr ptr ptr) ucrtbase._o___stdio_common_vfwscanf
@ cdecl _o___stdio_common_vsnprintf_s(int64 ptr long long str ptr ptr) ucrtbase._o___stdio_common_vsnprintf_s
@ cdecl _o___stdio_common_vsnwprintf_s(int64 ptr long long wstr ptr ptr) ucrtbase._o___stdio_common_vsnwprintf_s
@ cdecl _o___stdio_common_vsprintf(int64 ptr long str ptr ptr) ucrtbase._o___stdio_common_vsprintf
@ cdecl _o___stdio_common_vsprintf_p(int64 ptr long str ptr ptr) ucrtbase._o___stdio_common_vsprintf_p
@ cdecl _o___stdio_common_vsprintf_s(int64 ptr long str ptr ptr) ucrtbase._o___stdio_common_vsprintf_s
@ cdecl _o___stdio_common_vsscanf(int64 ptr long str ptr ptr) ucrtbase._o___stdio_common_vsscanf
@ cdecl _o___stdio_common_vswprintf(int64 ptr long wstr ptr ptr) ucrtbase._o___stdio_common_vswprintf
@ cdecl _o___stdio_common_vswprintf_p(int64 ptr long wstr ptr ptr) ucrtbase._o___stdio_common_vswprintf_p
@ cdecl _o___stdio_common_vswprintf_s(int64 ptr long wstr ptr ptr) ucrtbase._o___stdio_common_vswprintf_s
@ cdecl _o___stdio_common_vswscanf(int64 ptr long wstr ptr ptr) ucrtbase._o___stdio_common_vswscanf
@ cdecl _o___timezone() ucrtbase._o___timezone
@ cdecl _o___tzname() ucrtbase._o___tzname
@ cdecl _o___wcserror(wstr) ucrtbase._o___wcserror
@ cdecl _o__access(str long) ucrtbase._o__access
@ cdecl _o__access_s(str long) ucrtbase._o__access_s
@ cdecl _o__aligned_free(ptr) ucrtbase._o__aligned_free
@ cdecl _o__aligned_malloc(long long) ucrtbase._o__aligned_malloc
@ cdecl _o__aligned_msize(ptr long long) ucrtbase._o__aligned_msize
@ cdecl _o__aligned_offset_malloc(long long long) ucrtbase._o__aligned_offset_malloc
@ cdecl _o__aligned_offset_realloc(ptr long long long) ucrtbase._o__aligned_offset_realloc
@ stub _o__aligned_offset_recalloc
@ cdecl _o__aligned_realloc(ptr long long) ucrtbase._o__aligned_realloc
@ stub _o__aligned_recalloc
@ cdecl _o__atodbl(ptr str) ucrtbase._o__atodbl
@ cdecl _o__atodbl_l(ptr str ptr) ucrtbase._o__atodbl_l
@ cdecl _o__atof_l(str ptr) ucrtbase._o__atof_l
@ cdecl _o__atoflt(ptr str) ucrtbase._o__atoflt
@ cdecl _o__atoflt_l(ptr str ptr) ucrtbase._o__atoflt_l
@ cdecl -ret64 _o__atoi64(str) ucrtbase._o__atoi64
@ cdecl -ret64 _o__atoi64_l(str ptr) ucrtbase._o__atoi64_l
@ cdecl _o__atoi_l(str ptr) ucrtbase._o__atoi_l
@ cdecl _o__atol_l(str ptr) ucrtbase._o__atol_l
@ cdecl _o__atoldbl(ptr str) ucrtbase._o__atoldbl
@ cdecl _o__atoldbl_l(ptr str ptr) ucrtbase._o__atoldbl_l
@ cdecl -ret64 _o__atoll_l(str ptr) ucrtbase._o__atoll_l
@ cdecl _o__beep(long long) ucrtbase._o__beep
@ cdecl _o__beginthread(ptr long ptr) ucrtbase._o__beginthread
@ cdecl _o__beginthreadex(ptr long ptr ptr long ptr) ucrtbase._o__beginthreadex
@ cdecl _o__cabs(long) ucrtbase._o__cabs
@ cdecl _o__callnewh(long) ucrtbase._o__callnewh
@ cdecl _o__calloc_base(long long) ucrtbase._o__calloc_base
@ cdecl _o__cexit() ucrtbase._o__cexit
@ cdecl _o__cgets(ptr) ucrtbase._o__cgets
@ stub _o__cgets_s
@ stub _o__cgetws
@ stub _o__cgetws_s
@ cdecl _o__chdir(str) ucrtbase._o__chdir
@ cdecl _o__chdrive(long) ucrtbase._o__chdrive
@ cdecl _o__chmod(str long) ucrtbase._o__chmod
@ cdecl _o__chsize(long long) ucrtbase._o__chsize
@ cdecl _o__chsize_s(long int64) ucrtbase._o__chsize_s
@ cdecl _o__close(long) ucrtbase._o__close
@ cdecl _o__commit(long) ucrtbase._o__commit
@ cdecl _o__configthreadlocale(long) ucrtbase._o__configthreadlocale
@ cdecl _o__configure_narrow_argv(long) ucrtbase._o__configure_narrow_argv
@ cdecl _o__configure_wide_argv(long) ucrtbase._o__configure_wide_argv
@ cdecl _o__controlfp_s(ptr long long) ucrtbase._o__controlfp_s
@ cdecl _o__cputs(str) ucrtbase._o__cputs
@ cdecl _o__cputws(wstr) ucrtbase._o__cputws
@ cdecl _o__creat(str long) ucrtbase._o__creat
@ cdecl _o__create_locale(long str) ucrtbase._o__create_locale
@ cdecl _o__crt_atexit(ptr) ucrtbase._o__crt_atexit
@ cdecl _o__ctime32_s(str long ptr) ucrtbase._o__ctime32_s
@ cdecl _o__ctime64_s(str long ptr) ucrtbase._o__ctime64_s
@ cdecl _o__cwait(ptr long long) ucrtbase._o__cwait
@ stub _o__d_int
@ cdecl _o__dclass(double) ucrtbase._o__dclass
@ cdecl _o__difftime32(long long) ucrtbase._o__difftime32
@ cdecl _o__difftime64(int64 int64) ucrtbase._o__difftime64
@ stub _o__dlog
@ stub _o__dnorm
@ cdecl _o__dpcomp(double double) ucrtbase._o__dpcomp
@ stub _o__dpoly
@ stub _o__dscale
@ cdecl _o__dsign(double) ucrtbase._o__dsign
@ stub _o__dsin
@ cdecl _o__dtest(ptr) ucrtbase._o__dtest
@ stub _o__dunscale
@ cdecl _o__dup(long) ucrtbase._o__dup
@ cdecl _o__dup2(long long) ucrtbase._o__dup2
@ cdecl _o__dupenv_s(ptr ptr str) ucrtbase._o__dupenv_s
@ cdecl _o__ecvt(double long ptr ptr) ucrtbase._o__ecvt
@ cdecl _o__ecvt_s(str long double long ptr ptr) ucrtbase._o__ecvt_s
@ cdecl _o__endthread() ucrtbase._o__endthread
@ cdecl _o__endthreadex(long) ucrtbase._o__endthreadex
@ cdecl _o__eof(long) ucrtbase._o__eof
@ cdecl _o__errno() ucrtbase._o__errno
@ cdecl _o__except1(long long double double long ptr) ucrtbase._o__except1
@ cdecl _o__execute_onexit_table(ptr) ucrtbase._o__execute_onexit_table
@ cdecl _o__execv(str ptr) ucrtbase._o__execv
@ cdecl _o__execve(str ptr ptr) ucrtbase._o__execve
@ cdecl _o__execvp(str ptr) ucrtbase._o__execvp
@ cdecl _o__execvpe(str ptr ptr) ucrtbase._o__execvpe
@ cdecl _o__exit(long) ucrtbase._o__exit
@ cdecl _o__expand(ptr long) ucrtbase._o__expand
@ cdecl _o__fclose_nolock(ptr) ucrtbase._o__fclose_nolock
@ cdecl _o__fcloseall() ucrtbase._o__fcloseall
@ cdecl _o__fcvt(double long ptr ptr) ucrtbase._o__fcvt
@ cdecl _o__fcvt_s(ptr long double long ptr ptr) ucrtbase._o__fcvt_s
@ stub _o__fd_int
@ cdecl _o__fdclass(float) ucrtbase._o__fdclass
@ stub _o__fdexp
@ stub _o__fdlog
@ cdecl _o__fdopen(long str) ucrtbase._o__fdopen
@ cdecl _o__fdpcomp(float float) ucrtbase._o__fdpcomp
@ stub _o__fdpoly
@ stub _o__fdscale
@ cdecl _o__fdsign(float) ucrtbase._o__fdsign
@ stub _o__fdsin
@ cdecl _o__fflush_nolock(ptr) ucrtbase._o__fflush_nolock
@ cdecl _o__fgetc_nolock(ptr) ucrtbase._o__fgetc_nolock
@ cdecl _o__fgetchar() ucrtbase._o__fgetchar
@ cdecl _o__fgetwc_nolock(ptr) ucrtbase._o__fgetwc_nolock
@ cdecl _o__fgetwchar() ucrtbase._o__fgetwchar
@ cdecl _o__filelength(long) ucrtbase._o__filelength
@ cdecl -ret64 _o__filelengthi64(long) ucrtbase._o__filelengthi64
@ cdecl _o__fileno(ptr) ucrtbase._o__fileno
@ cdecl _o__findclose(long) ucrtbase._o__findclose
@ cdecl _o__findfirst32(str ptr) ucrtbase._o__findfirst32
@ stub _o__findfirst32i64
@ cdecl _o__findfirst64(str ptr) ucrtbase._o__findfirst64
@ cdecl _o__findfirst64i32(str ptr) ucrtbase._o__findfirst64i32
@ cdecl _o__findnext32(long ptr) ucrtbase._o__findnext32
@ stub _o__findnext32i64
@ cdecl _o__findnext64(long ptr) ucrtbase._o__findnext64
@ cdecl _o__findnext64i32(long ptr) ucrtbase._o__findnext64i32
@ cdecl _o__flushall() ucrtbase._o__flushall
@ cdecl _o__fpclass(double) ucrtbase._o__fpclass
@ cdecl -arch=!i386 _o__fpclassf(float) ucrtbase._o__fpclassf
@ cdecl _o__fputc_nolock(long ptr) ucrtbase._o__fputc_nolock
@ cdecl _o__fputchar(long) ucrtbase._o__fputchar
@ cdecl _o__fputwc_nolock(long ptr) ucrtbase._o__fputwc_nolock
@ cdecl _o__fputwchar(long) ucrtbase._o__fputwchar
@ cdecl _o__fread_nolock(ptr long long ptr) ucrtbase._o__fread_nolock
@ cdecl _o__fread_nolock_s(ptr long long long ptr) ucrtbase._o__fread_nolock_s
@ cdecl _o__free_base(ptr) ucrtbase._o__free_base
@ cdecl _o__free_locale(ptr) ucrtbase._o__free_locale
@ cdecl _o__fseek_nolock(ptr long long) ucrtbase._o__fseek_nolock
@ cdecl _o__fseeki64(ptr int64 long) ucrtbase._o__fseeki64
@ cdecl _o__fseeki64_nolock(ptr int64 long) ucrtbase._o__fseeki64_nolock
@ cdecl _o__fsopen(str str long) ucrtbase._o__fsopen
@ cdecl _o__fstat32(long ptr) ucrtbase._o__fstat32
@ cdecl _o__fstat32i64(long ptr) ucrtbase._o__fstat32i64
@ cdecl _o__fstat64(long ptr) ucrtbase._o__fstat64
@ cdecl _o__fstat64i32(long ptr) ucrtbase._o__fstat64i32
@ cdecl _o__ftell_nolock(ptr) ucrtbase._o__ftell_nolock
@ cdecl -ret64 _o__ftelli64(ptr) ucrtbase._o__ftelli64
@ cdecl -ret64 _o__ftelli64_nolock(ptr) ucrtbase._o__ftelli64_nolock
@ cdecl _o__ftime32(ptr) ucrtbase._o__ftime32
@ cdecl _o__ftime32_s(ptr) ucrtbase._o__ftime32_s
@ cdecl _o__ftime64(ptr) ucrtbase._o__ftime64
@ cdecl _o__ftime64_s(ptr) ucrtbase._o__ftime64_s
@ cdecl _o__fullpath(ptr str long) ucrtbase._o__fullpath
@ cdecl _o__futime32(long ptr) ucrtbase._o__futime32
@ cdecl _o__futime64(long ptr) ucrtbase._o__futime64
@ cdecl _o__fwrite_nolock(ptr long long ptr) ucrtbase._o__fwrite_nolock
@ cdecl _o__gcvt(double long str) ucrtbase._o__gcvt
@ cdecl _o__gcvt_s(ptr long double long) ucrtbase._o__gcvt_s
@ cdecl _o__get_daylight(ptr) ucrtbase._o__get_daylight
@ cdecl _o__get_doserrno(ptr) ucrtbase._o__get_doserrno
@ cdecl _o__get_dstbias(ptr) ucrtbase._o__get_dstbias
@ cdecl _o__get_errno(ptr) ucrtbase._o__get_errno
@ cdecl _o__get_fmode(ptr) ucrtbase._o__get_fmode
@ cdecl _o__get_heap_handle() ucrtbase._o__get_heap_handle
@ cdecl _o__get_initial_narrow_environment() ucrtbase._o__get_initial_narrow_environment
@ cdecl _o__get_initial_wide_environment() ucrtbase._o__get_initial_wide_environment
@ cdecl _o__get_invalid_parameter_handler() ucrtbase._o__get_invalid_parameter_handler
@ cdecl _o__get_narrow_winmain_command_line() ucrtbase._o__get_narrow_winmain_command_line
@ cdecl _o__get_osfhandle(long) ucrtbase._o__get_osfhandle
@ cdecl _o__get_pgmptr(ptr) ucrtbase._o__get_pgmptr
@ cdecl _o__get_stream_buffer_pointers(ptr ptr ptr ptr) ucrtbase._o__get_stream_buffer_pointers
@ cdecl _o__get_terminate() ucrtbase._o__get_terminate
@ cdecl _o__get_thread_local_invalid_parameter_handler() ucrtbase._o__get_thread_local_invalid_parameter_handler
@ cdecl _o__get_timezone(ptr) ucrtbase._o__get_timezone
@ cdecl _o__get_tzname(ptr str long long) ucrtbase._o__get_tzname
@ cdecl _o__get_wide_winmain_command_line() ucrtbase._o__get_wide_winmain_command_line
@ cdecl _o__get_wpgmptr(ptr) ucrtbase._o__get_wpgmptr
@ cdecl _o__getc_nolock(ptr) ucrtbase._o__getc_nolock
@ cdecl _o__getch() ucrtbase._o__getch
@ cdecl _o__getch_nolock() ucrtbase._o__getch_nolock
@ cdecl _o__getche() ucrtbase._o__getche
@ cdecl _o__getche_nolock() ucrtbase._o__getche_nolock
@ cdecl _o__getcwd(str long) ucrtbase._o__getcwd
@ cdecl _o__getdcwd(long str long) ucrtbase._o__getdcwd
@ cdecl _o__getdiskfree(long ptr) ucrtbase._o__getdiskfree
@ cdecl _o__getdllprocaddr(long str long) ucrtbase._o__getdllprocaddr
@ cdecl _o__getdrive() ucrtbase._o__getdrive
@ cdecl _o__getdrives() ucrtbase._o__getdrives
@ cdecl _o__getmbcp() ucrtbase._o__getmbcp
@ stub _o__getsystime
@ cdecl _o__getw(ptr) ucrtbase._o__getw
@ cdecl _o__getwc_nolock(ptr) ucrtbase._o__getwc_nolock
@ cdecl _o__getwch() ucrtbase._o__getwch
@ cdecl _o__getwch_nolock() ucrtbase._o__getwch_nolock
@ cdecl _o__getwche() ucrtbase._o__getwche
@ cdecl _o__getwche_nolock() ucrtbase._o__getwche_nolock
@ cdecl _o__getws(ptr) ucrtbase._o__getws
@ stub _o__getws_s
@ cdecl _o__gmtime32(ptr) ucrtbase._o__gmtime32
@ cdecl _o__gmtime32_s(ptr ptr) ucrtbase._o__gmtime32_s
@ cdecl _o__gmtime64(ptr) ucrtbase._o__gmtime64
@ cdecl _o__gmtime64_s(ptr ptr) ucrtbase._o__gmtime64_s
@ cdecl _o__heapchk() ucrtbase._o__heapchk
@ cdecl _o__heapmin() ucrtbase._o__heapmin
@ cdecl _o__hypot(double double) ucrtbase._o__hypot
@ cdecl _o__hypotf(float float) ucrtbase._o__hypotf
@ cdecl _o__i64toa(int64 ptr long) ucrtbase._o__i64toa
@ cdecl _o__i64toa_s(int64 ptr long long) ucrtbase._o__i64toa_s
@ cdecl _o__i64tow(int64 ptr long) ucrtbase._o__i64tow
@ cdecl _o__i64tow_s(int64 ptr long long) ucrtbase._o__i64tow_s
@ cdecl _o__initialize_narrow_environment() ucrtbase._o__initialize_narrow_environment
@ cdecl _o__initialize_onexit_table(ptr) ucrtbase._o__initialize_onexit_table
@ cdecl _o__initialize_wide_environment() ucrtbase._o__initialize_wide_environment
@ cdecl _o__invalid_parameter_noinfo() ucrtbase._o__invalid_parameter_noinfo
@ cdecl _o__invalid_parameter_noinfo_noreturn() ucrtbase._o__invalid_parameter_noinfo_noreturn
@ cdecl _o__isatty(long) ucrtbase._o__isatty
@ cdecl _o__isctype(long long) ucrtbase._o__isctype
@ cdecl _o__isctype_l(long long ptr) ucrtbase._o__isctype_l
@ cdecl _o__isleadbyte_l(long ptr) ucrtbase._o__isleadbyte_l
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
@ cdecl _o__ismbbkana(long) ucrtbase._o__ismbbkana
@ cdecl _o__ismbbkana_l(long ptr) ucrtbase._o__ismbbkana_l
@ stub _o__ismbbkprint
@ stub _o__ismbbkprint_l
@ stub _o__ismbbkpunct
@ stub _o__ismbbkpunct_l
@ cdecl _o__ismbblead(long) ucrtbase._o__ismbblead
@ cdecl _o__ismbblead_l(long ptr) ucrtbase._o__ismbblead_l
@ stub _o__ismbbprint
@ stub _o__ismbbprint_l
@ stub _o__ismbbpunct
@ stub _o__ismbbpunct_l
@ cdecl _o__ismbbtrail(long) ucrtbase._o__ismbbtrail
@ cdecl _o__ismbbtrail_l(long ptr) ucrtbase._o__ismbbtrail_l
@ cdecl _o__ismbcalnum(long) ucrtbase._o__ismbcalnum
@ cdecl _o__ismbcalnum_l(long ptr) ucrtbase._o__ismbcalnum_l
@ cdecl _o__ismbcalpha(long) ucrtbase._o__ismbcalpha
@ cdecl _o__ismbcalpha_l(long ptr) ucrtbase._o__ismbcalpha_l
@ stub _o__ismbcblank
@ stub _o__ismbcblank_l
@ cdecl _o__ismbcdigit(long) ucrtbase._o__ismbcdigit
@ cdecl _o__ismbcdigit_l(long ptr) ucrtbase._o__ismbcdigit_l
@ cdecl _o__ismbcgraph(long) ucrtbase._o__ismbcgraph
@ cdecl _o__ismbcgraph_l(long ptr) ucrtbase._o__ismbcgraph_l
@ cdecl _o__ismbchira(long) ucrtbase._o__ismbchira
@ stub _o__ismbchira_l
@ cdecl _o__ismbckata(long) ucrtbase._o__ismbckata
@ stub _o__ismbckata_l
@ cdecl _o__ismbcl0(long) ucrtbase._o__ismbcl0
@ cdecl _o__ismbcl0_l(long ptr) ucrtbase._o__ismbcl0_l
@ cdecl _o__ismbcl1(long) ucrtbase._o__ismbcl1
@ cdecl _o__ismbcl1_l(long ptr) ucrtbase._o__ismbcl1_l
@ cdecl _o__ismbcl2(long) ucrtbase._o__ismbcl2
@ cdecl _o__ismbcl2_l(long ptr) ucrtbase._o__ismbcl2_l
@ cdecl _o__ismbclegal(long) ucrtbase._o__ismbclegal
@ cdecl _o__ismbclegal_l(long ptr) ucrtbase._o__ismbclegal_l
@ stub _o__ismbclower
@ cdecl _o__ismbclower_l(long ptr) ucrtbase._o__ismbclower_l
@ cdecl _o__ismbcprint(long) ucrtbase._o__ismbcprint
@ cdecl _o__ismbcprint_l(long ptr) ucrtbase._o__ismbcprint_l
@ cdecl _o__ismbcpunct(long) ucrtbase._o__ismbcpunct
@ cdecl _o__ismbcpunct_l(long ptr) ucrtbase._o__ismbcpunct_l
@ cdecl _o__ismbcspace(long) ucrtbase._o__ismbcspace
@ cdecl _o__ismbcspace_l(long ptr) ucrtbase._o__ismbcspace_l
@ cdecl _o__ismbcsymbol(long) ucrtbase._o__ismbcsymbol
@ stub _o__ismbcsymbol_l
@ cdecl _o__ismbcupper(long) ucrtbase._o__ismbcupper
@ cdecl _o__ismbcupper_l(long ptr) ucrtbase._o__ismbcupper_l
@ cdecl _o__ismbslead(ptr ptr) ucrtbase._o__ismbslead
@ stub _o__ismbslead_l
@ cdecl _o__ismbstrail(ptr ptr) ucrtbase._o__ismbstrail
@ stub _o__ismbstrail_l
@ cdecl _o__iswctype_l(long long ptr) ucrtbase._o__iswctype_l
@ cdecl _o__itoa(long ptr long) ucrtbase._o__itoa
@ cdecl _o__itoa_s(long ptr long long) ucrtbase._o__itoa_s
@ cdecl _o__itow(long ptr long) ucrtbase._o__itow
@ cdecl _o__itow_s(long ptr long long) ucrtbase._o__itow_s
@ cdecl _o__j0(double) ucrtbase._o__j0
@ cdecl _o__j1(double) ucrtbase._o__j1
@ cdecl _o__jn(long double) ucrtbase._o__jn
@ cdecl _o__kbhit() ucrtbase._o__kbhit
@ stub _o__ld_int
@ cdecl _o__ldclass(double) ucrtbase._o__ldclass
@ stub _o__ldexp
@ stub _o__ldlog
@ cdecl _o__ldpcomp(double double) ucrtbase._o__ldpcomp
@ stub _o__ldpoly
@ stub _o__ldscale
@ cdecl _o__ldsign(double) ucrtbase._o__ldsign
@ stub _o__ldsin
@ cdecl _o__ldtest(ptr) ucrtbase._o__ldtest
@ stub _o__ldunscale
@ cdecl _o__lfind(ptr ptr ptr long ptr) ucrtbase._o__lfind
@ cdecl _o__lfind_s(ptr ptr ptr long ptr ptr) ucrtbase._o__lfind_s
@ cdecl -arch=i386 -norelay _o__libm_sse2_acos_precise() ucrtbase._o__libm_sse2_acos_precise
@ cdecl -arch=i386 -norelay _o__libm_sse2_asin_precise() ucrtbase._o__libm_sse2_asin_precise
@ cdecl -arch=i386 -norelay _o__libm_sse2_atan_precise() ucrtbase._o__libm_sse2_atan_precise
@ cdecl -arch=i386 -norelay _o__libm_sse2_cos_precise() ucrtbase._o__libm_sse2_cos_precise
@ cdecl -arch=i386 -norelay _o__libm_sse2_exp_precise() ucrtbase._o__libm_sse2_exp_precise
@ cdecl -arch=i386 -norelay _o__libm_sse2_log10_precise() ucrtbase._o__libm_sse2_log10_precise
@ cdecl -arch=i386 -norelay _o__libm_sse2_log_precise() ucrtbase._o__libm_sse2_log_precise
@ cdecl -arch=i386 -norelay _o__libm_sse2_pow_precise() ucrtbase._o__libm_sse2_pow_precise
@ cdecl -arch=i386 -norelay _o__libm_sse2_sin_precise() ucrtbase._o__libm_sse2_sin_precise
@ cdecl -arch=i386 -norelay _o__libm_sse2_sqrt_precise() ucrtbase._o__libm_sse2_sqrt_precise
@ cdecl -arch=i386 -norelay _o__libm_sse2_tan_precise() ucrtbase._o__libm_sse2_tan_precise
@ cdecl _o__loaddll(str) ucrtbase._o__loaddll
@ cdecl _o__localtime32(ptr) ucrtbase._o__localtime32
@ cdecl _o__localtime32_s(ptr ptr) ucrtbase._o__localtime32_s
@ cdecl _o__localtime64(ptr) ucrtbase._o__localtime64
@ cdecl _o__localtime64_s(ptr ptr) ucrtbase._o__localtime64_s
@ cdecl _o__lock_file(ptr) ucrtbase._o__lock_file
@ cdecl _o__locking(long long long) ucrtbase._o__locking
@ cdecl _o__logb(double) ucrtbase._o__logb
@ cdecl -arch=!i386 _o__logbf(float) ucrtbase._o__logbf
@ cdecl _o__lsearch(ptr ptr ptr long ptr) ucrtbase._o__lsearch
@ stub _o__lsearch_s
@ cdecl _o__lseek(long long long) ucrtbase._o__lseek
@ cdecl -ret64 _o__lseeki64(long int64 long) ucrtbase._o__lseeki64
@ cdecl _o__ltoa(long ptr long) ucrtbase._o__ltoa
@ cdecl _o__ltoa_s(long ptr long long) ucrtbase._o__ltoa_s
@ cdecl _o__ltow(long ptr long) ucrtbase._o__ltow
@ cdecl _o__ltow_s(long ptr long long) ucrtbase._o__ltow_s
@ cdecl _o__makepath(ptr str str str str) ucrtbase._o__makepath
@ cdecl _o__makepath_s(ptr long str str str str) ucrtbase._o__makepath_s
@ cdecl _o__malloc_base(long) ucrtbase._o__malloc_base
@ cdecl _o__mbbtombc(long) ucrtbase._o__mbbtombc
@ stub _o__mbbtombc_l
@ cdecl _o__mbbtype(long long) ucrtbase._o__mbbtype
@ cdecl _o__mbbtype_l(long long ptr) ucrtbase._o__mbbtype_l
@ cdecl _o__mbccpy(ptr ptr) ucrtbase._o__mbccpy
@ cdecl _o__mbccpy_l(ptr ptr ptr) ucrtbase._o__mbccpy_l
@ cdecl _o__mbccpy_s(ptr long ptr ptr) ucrtbase._o__mbccpy_s
@ cdecl _o__mbccpy_s_l(ptr long ptr ptr ptr) ucrtbase._o__mbccpy_s_l
@ cdecl _o__mbcjistojms(long) ucrtbase._o__mbcjistojms
@ stub _o__mbcjistojms_l
@ cdecl _o__mbcjmstojis(long) ucrtbase._o__mbcjmstojis
@ stub _o__mbcjmstojis_l
@ cdecl _o__mbclen(ptr) ucrtbase._o__mbclen
@ stub _o__mbclen_l
@ cdecl _o__mbctohira(long) ucrtbase._o__mbctohira
@ stub _o__mbctohira_l
@ cdecl _o__mbctokata(long) ucrtbase._o__mbctokata
@ stub _o__mbctokata_l
@ cdecl _o__mbctolower(long) ucrtbase._o__mbctolower
@ stub _o__mbctolower_l
@ cdecl _o__mbctombb(long) ucrtbase._o__mbctombb
@ stub _o__mbctombb_l
@ cdecl _o__mbctoupper(long) ucrtbase._o__mbctoupper
@ stub _o__mbctoupper_l
@ stub _o__mblen_l
@ cdecl _o__mbsbtype(str long) ucrtbase._o__mbsbtype
@ stub _o__mbsbtype_l
@ cdecl _o__mbscat_s(ptr long str) ucrtbase._o__mbscat_s
@ cdecl _o__mbscat_s_l(ptr long str ptr) ucrtbase._o__mbscat_s_l
@ cdecl _o__mbschr(str long) ucrtbase._o__mbschr
@ stub _o__mbschr_l
@ cdecl _o__mbscmp(str str) ucrtbase._o__mbscmp
@ cdecl _o__mbscmp_l(str str ptr) ucrtbase._o__mbscmp_l
@ cdecl _o__mbscoll(str str) ucrtbase._o__mbscoll
@ cdecl _o__mbscoll_l(str str ptr) ucrtbase._o__mbscoll_l
@ cdecl _o__mbscpy_s(ptr long str) ucrtbase._o__mbscpy_s
@ cdecl _o__mbscpy_s_l(ptr long str ptr) ucrtbase._o__mbscpy_s_l
@ cdecl _o__mbscspn(str str) ucrtbase._o__mbscspn
@ cdecl _o__mbscspn_l(str str ptr) ucrtbase._o__mbscspn_l
@ cdecl _o__mbsdec(ptr ptr) ucrtbase._o__mbsdec
@ stub _o__mbsdec_l
@ cdecl _o__mbsicmp(str str) ucrtbase._o__mbsicmp
@ stub _o__mbsicmp_l
@ cdecl _o__mbsicoll(str str) ucrtbase._o__mbsicoll
@ cdecl _o__mbsicoll_l(str str ptr) ucrtbase._o__mbsicoll_l
@ cdecl _o__mbsinc(str) ucrtbase._o__mbsinc
@ stub _o__mbsinc_l
@ cdecl _o__mbslen(str) ucrtbase._o__mbslen
@ cdecl _o__mbslen_l(str ptr) ucrtbase._o__mbslen_l
@ cdecl _o__mbslwr(str) ucrtbase._o__mbslwr
@ stub _o__mbslwr_l
@ cdecl _o__mbslwr_s(str long) ucrtbase._o__mbslwr_s
@ stub _o__mbslwr_s_l
@ cdecl _o__mbsnbcat(str str long) ucrtbase._o__mbsnbcat
@ stub _o__mbsnbcat_l
@ cdecl _o__mbsnbcat_s(str long ptr long) ucrtbase._o__mbsnbcat_s
@ stub _o__mbsnbcat_s_l
@ cdecl _o__mbsnbcmp(str str long) ucrtbase._o__mbsnbcmp
@ stub _o__mbsnbcmp_l
@ cdecl _o__mbsnbcnt(ptr long) ucrtbase._o__mbsnbcnt
@ stub _o__mbsnbcnt_l
@ cdecl _o__mbsnbcoll(str str long) ucrtbase._o__mbsnbcoll
@ cdecl _o__mbsnbcoll_l(str str long ptr) ucrtbase._o__mbsnbcoll_l
@ cdecl _o__mbsnbcpy(ptr str long) ucrtbase._o__mbsnbcpy
@ stub _o__mbsnbcpy_l
@ cdecl _o__mbsnbcpy_s(ptr long str long) ucrtbase._o__mbsnbcpy_s
@ cdecl _o__mbsnbcpy_s_l(ptr long str long ptr) ucrtbase._o__mbsnbcpy_s_l
@ cdecl _o__mbsnbicmp(str str long) ucrtbase._o__mbsnbicmp
@ stub _o__mbsnbicmp_l
@ cdecl _o__mbsnbicoll(str str long) ucrtbase._o__mbsnbicoll
@ cdecl _o__mbsnbicoll_l(str str long ptr) ucrtbase._o__mbsnbicoll_l
@ cdecl _o__mbsnbset(ptr long long) ucrtbase._o__mbsnbset
@ stub _o__mbsnbset_l
@ stub _o__mbsnbset_s
@ stub _o__mbsnbset_s_l
@ cdecl _o__mbsncat(str str long) ucrtbase._o__mbsncat
@ stub _o__mbsncat_l
@ stub _o__mbsncat_s
@ stub _o__mbsncat_s_l
@ cdecl _o__mbsnccnt(str long) ucrtbase._o__mbsnccnt
@ stub _o__mbsnccnt_l
@ cdecl _o__mbsncmp(str str long) ucrtbase._o__mbsncmp
@ stub _o__mbsncmp_l
@ stub _o__mbsncoll
@ stub _o__mbsncoll_l
@ cdecl _o__mbsncpy(ptr str long) ucrtbase._o__mbsncpy
@ stub _o__mbsncpy_l
@ stub _o__mbsncpy_s
@ stub _o__mbsncpy_s_l
@ cdecl _o__mbsnextc(str) ucrtbase._o__mbsnextc
@ cdecl _o__mbsnextc_l(str ptr) ucrtbase._o__mbsnextc_l
@ cdecl _o__mbsnicmp(str str long) ucrtbase._o__mbsnicmp
@ stub _o__mbsnicmp_l
@ stub _o__mbsnicoll
@ stub _o__mbsnicoll_l
@ cdecl _o__mbsninc(str long) ucrtbase._o__mbsninc
@ stub _o__mbsninc_l
@ cdecl _o__mbsnlen(str long) ucrtbase._o__mbsnlen
@ cdecl _o__mbsnlen_l(str long ptr) ucrtbase._o__mbsnlen_l
@ cdecl _o__mbsnset(ptr long long) ucrtbase._o__mbsnset
@ stub _o__mbsnset_l
@ stub _o__mbsnset_s
@ stub _o__mbsnset_s_l
@ cdecl _o__mbspbrk(str str) ucrtbase._o__mbspbrk
@ stub _o__mbspbrk_l
@ cdecl _o__mbsrchr(str long) ucrtbase._o__mbsrchr
@ stub _o__mbsrchr_l
@ cdecl _o__mbsrev(str) ucrtbase._o__mbsrev
@ stub _o__mbsrev_l
@ cdecl _o__mbsset(ptr long) ucrtbase._o__mbsset
@ stub _o__mbsset_l
@ stub _o__mbsset_s
@ stub _o__mbsset_s_l
@ cdecl _o__mbsspn(str str) ucrtbase._o__mbsspn
@ cdecl _o__mbsspn_l(str str ptr) ucrtbase._o__mbsspn_l
@ cdecl _o__mbsspnp(str str) ucrtbase._o__mbsspnp
@ stub _o__mbsspnp_l
@ cdecl _o__mbsstr(str str) ucrtbase._o__mbsstr
@ stub _o__mbsstr_l
@ cdecl _o__mbstok(str str) ucrtbase._o__mbstok
@ cdecl _o__mbstok_l(str str ptr) ucrtbase._o__mbstok_l
@ cdecl _o__mbstok_s(str str ptr) ucrtbase._o__mbstok_s
@ cdecl _o__mbstok_s_l(str str ptr ptr) ucrtbase._o__mbstok_s_l
@ cdecl _o__mbstowcs_l(ptr str long ptr) ucrtbase._o__mbstowcs_l
@ cdecl _o__mbstowcs_s_l(ptr ptr long str long ptr) ucrtbase._o__mbstowcs_s_l
@ cdecl _o__mbstrlen(str) ucrtbase._o__mbstrlen
@ cdecl _o__mbstrlen_l(str ptr) ucrtbase._o__mbstrlen_l
@ stub _o__mbstrnlen
@ stub _o__mbstrnlen_l
@ cdecl _o__mbsupr(str) ucrtbase._o__mbsupr
@ stub _o__mbsupr_l
@ cdecl _o__mbsupr_s(str long) ucrtbase._o__mbsupr_s
@ stub _o__mbsupr_s_l
@ cdecl _o__mbtowc_l(ptr str long ptr) ucrtbase._o__mbtowc_l
@ cdecl _o__memicmp(str str long) ucrtbase._o__memicmp
@ cdecl _o__memicmp_l(str str long ptr) ucrtbase._o__memicmp_l
@ cdecl _o__mkdir(str) ucrtbase._o__mkdir
@ cdecl _o__mkgmtime32(ptr) ucrtbase._o__mkgmtime32
@ cdecl _o__mkgmtime64(ptr) ucrtbase._o__mkgmtime64
@ cdecl _o__mktemp(str) ucrtbase._o__mktemp
@ cdecl _o__mktemp_s(str long) ucrtbase._o__mktemp_s
@ cdecl _o__mktime32(ptr) ucrtbase._o__mktime32
@ cdecl _o__mktime64(ptr) ucrtbase._o__mktime64
@ cdecl _o__msize(ptr) ucrtbase._o__msize
@ cdecl _o__nextafter(double double) ucrtbase._o__nextafter
@ cdecl -arch=x86_64 _o__nextafterf(float float) ucrtbase._o__nextafterf
@ cdecl _o__open_osfhandle(long long) ucrtbase._o__open_osfhandle
@ cdecl _o__pclose(ptr) ucrtbase._o__pclose
@ cdecl _o__pipe(ptr long long) ucrtbase._o__pipe
@ cdecl _o__popen(str str) ucrtbase._o__popen
@ cdecl _o__purecall() ucrtbase._o__purecall
@ cdecl _o__putc_nolock(long ptr) ucrtbase._o__putc_nolock
@ cdecl _o__putch(long) ucrtbase._o__putch
@ cdecl _o__putch_nolock(long) ucrtbase._o__putch_nolock
@ cdecl _o__putenv(str) ucrtbase._o__putenv
@ cdecl _o__putenv_s(str str) ucrtbase._o__putenv_s
@ cdecl _o__putw(long ptr) ucrtbase._o__putw
@ cdecl _o__putwc_nolock(long ptr) ucrtbase._o__putwc_nolock
@ cdecl _o__putwch(long) ucrtbase._o__putwch
@ cdecl _o__putwch_nolock(long) ucrtbase._o__putwch_nolock
@ cdecl _o__putws(wstr) ucrtbase._o__putws
@ cdecl _o__read(long ptr long) ucrtbase._o__read
@ cdecl _o__realloc_base(ptr long) ucrtbase._o__realloc_base
@ cdecl _o__recalloc(ptr long long) ucrtbase._o__recalloc
@ cdecl _o__register_onexit_function(ptr ptr) ucrtbase._o__register_onexit_function
@ cdecl _o__resetstkoflw() ucrtbase._o__resetstkoflw
@ cdecl _o__rmdir(str) ucrtbase._o__rmdir
@ cdecl _o__rmtmp() ucrtbase._o__rmtmp
@ cdecl _o__scalb(double long) ucrtbase._o__scalb
@ cdecl -arch=x86_64 _o__scalbf(float long) ucrtbase._o__scalbf
@ cdecl _o__searchenv(str str ptr) ucrtbase._o__searchenv
@ cdecl _o__searchenv_s(str str ptr long) ucrtbase._o__searchenv_s
@ cdecl _o__seh_filter_dll(long ptr) ucrtbase._o__seh_filter_dll
@ cdecl _o__seh_filter_exe(long ptr) ucrtbase._o__seh_filter_exe
@ cdecl _o__set_abort_behavior(long long) ucrtbase._o__set_abort_behavior
@ cdecl _o__set_app_type(long) ucrtbase._o__set_app_type
@ cdecl _o__set_doserrno(long) ucrtbase._o__set_doserrno
@ cdecl _o__set_errno(long) ucrtbase._o__set_errno
@ cdecl _o__set_fmode(long) ucrtbase._o__set_fmode
@ cdecl _o__set_invalid_parameter_handler(ptr) ucrtbase._o__set_invalid_parameter_handler
@ cdecl _o__set_new_handler(ptr) ucrtbase._o__set_new_handler
@ cdecl _o__set_new_mode(long) ucrtbase._o__set_new_mode
@ cdecl _o__set_thread_local_invalid_parameter_handler(ptr) ucrtbase._o__set_thread_local_invalid_parameter_handler
@ cdecl _o__seterrormode(long) ucrtbase._o__seterrormode
@ cdecl _o__setmbcp(long) ucrtbase._o__setmbcp
@ cdecl _o__setmode(long long) ucrtbase._o__setmode
@ stub _o__setsystime
@ cdecl _o__sleep(long) ucrtbase._o__sleep
@ varargs _o__sopen(str long long) ucrtbase._o__sopen
@ cdecl _o__sopen_dispatch(str long long long ptr long) ucrtbase._o__sopen_dispatch
@ cdecl _o__sopen_s(ptr str long long long) ucrtbase._o__sopen_s
@ cdecl _o__spawnv(long str ptr) ucrtbase._o__spawnv
@ cdecl _o__spawnve(long str ptr ptr) ucrtbase._o__spawnve
@ cdecl _o__spawnvp(long str ptr) ucrtbase._o__spawnvp
@ cdecl _o__spawnvpe(long str ptr ptr) ucrtbase._o__spawnvpe
@ cdecl _o__splitpath(str ptr ptr ptr ptr) ucrtbase._o__splitpath
@ cdecl _o__splitpath_s(str ptr long ptr long ptr long ptr long) ucrtbase._o__splitpath_s
@ cdecl _o__stat32(str ptr) ucrtbase._o__stat32
@ cdecl _o__stat32i64(str ptr) ucrtbase._o__stat32i64
@ cdecl _o__stat64(str ptr) ucrtbase._o__stat64
@ cdecl _o__stat64i32(str ptr) ucrtbase._o__stat64i32
@ cdecl _o__strcoll_l(str str ptr) ucrtbase._o__strcoll_l
@ cdecl _o__strdate(ptr) ucrtbase._o__strdate
@ cdecl _o__strdate_s(ptr long) ucrtbase._o__strdate_s
@ cdecl _o__strdup(str) ucrtbase._o__strdup
@ cdecl _o__strerror(long) ucrtbase._o__strerror
@ stub _o__strerror_s
@ cdecl _o__strftime_l(ptr long str ptr ptr) ucrtbase._o__strftime_l
@ cdecl _o__stricmp(str str) ucrtbase._o__stricmp
@ cdecl _o__stricmp_l(str str ptr) ucrtbase._o__stricmp_l
@ cdecl _o__stricoll(str str) ucrtbase._o__stricoll
@ cdecl _o__stricoll_l(str str ptr) ucrtbase._o__stricoll_l
@ cdecl _o__strlwr(str) ucrtbase._o__strlwr
@ cdecl _o__strlwr_l(str ptr) ucrtbase._o__strlwr_l
@ cdecl _o__strlwr_s(ptr long) ucrtbase._o__strlwr_s
@ cdecl _o__strlwr_s_l(ptr long ptr) ucrtbase._o__strlwr_s_l
@ cdecl _o__strncoll(str str long) ucrtbase._o__strncoll
@ cdecl _o__strncoll_l(str str long ptr) ucrtbase._o__strncoll_l
@ cdecl _o__strnicmp(str str long) ucrtbase._o__strnicmp
@ cdecl _o__strnicmp_l(str str long ptr) ucrtbase._o__strnicmp_l
@ cdecl _o__strnicoll(str str long) ucrtbase._o__strnicoll
@ cdecl _o__strnicoll_l(str str long ptr) ucrtbase._o__strnicoll_l
@ cdecl _o__strnset_s(str long long long) ucrtbase._o__strnset_s
@ stub _o__strset_s
@ cdecl _o__strtime(ptr) ucrtbase._o__strtime
@ cdecl _o__strtime_s(ptr long) ucrtbase._o__strtime_s
@ cdecl _o__strtod_l(str ptr ptr) ucrtbase._o__strtod_l
@ cdecl _o__strtof_l(str ptr ptr) ucrtbase._o__strtof_l
@ cdecl -ret64 _o__strtoi64(str ptr long) ucrtbase._o__strtoi64
@ cdecl -ret64 _o__strtoi64_l(str ptr long ptr) ucrtbase._o__strtoi64_l
@ cdecl _o__strtol_l(str ptr long ptr) ucrtbase._o__strtol_l
@ cdecl _o__strtold_l(str ptr ptr) ucrtbase._o__strtold_l
@ cdecl -ret64 _o__strtoll_l(str ptr long ptr) ucrtbase._o__strtoll_l
@ cdecl -ret64 _o__strtoui64(str ptr long) ucrtbase._o__strtoui64
@ cdecl -ret64 _o__strtoui64_l(str ptr long ptr) ucrtbase._o__strtoui64_l
@ cdecl _o__strtoul_l(str ptr long ptr) ucrtbase._o__strtoul_l
@ cdecl -ret64 _o__strtoull_l(str ptr long ptr) ucrtbase._o__strtoull_l
@ cdecl _o__strupr(str) ucrtbase._o__strupr
@ cdecl _o__strupr_l(str ptr) ucrtbase._o__strupr_l
@ cdecl _o__strupr_s(str long) ucrtbase._o__strupr_s
@ cdecl _o__strupr_s_l(str long ptr) ucrtbase._o__strupr_s_l
@ cdecl _o__strxfrm_l(ptr str long ptr) ucrtbase._o__strxfrm_l
@ cdecl _o__swab(str str long) ucrtbase._o__swab
@ cdecl _o__tell(long) ucrtbase._o__tell
@ cdecl -ret64 _o__telli64(long) ucrtbase._o__telli64
@ cdecl _o__timespec32_get(ptr long) ucrtbase._o__timespec32_get
@ cdecl _o__timespec64_get(ptr long) ucrtbase._o__timespec64_get
@ cdecl _o__tolower(long) ucrtbase._o__tolower
@ cdecl _o__tolower_l(long ptr) ucrtbase._o__tolower_l
@ cdecl _o__toupper(long) ucrtbase._o__toupper
@ cdecl _o__toupper_l(long ptr) ucrtbase._o__toupper_l
@ cdecl _o__towlower_l(long ptr) ucrtbase._o__towlower_l
@ cdecl _o__towupper_l(long ptr) ucrtbase._o__towupper_l
@ cdecl _o__tzset() ucrtbase._o__tzset
@ cdecl _o__ui64toa(int64 ptr long) ucrtbase._o__ui64toa
@ cdecl _o__ui64toa_s(int64 ptr long long) ucrtbase._o__ui64toa_s
@ cdecl _o__ui64tow(int64 ptr long) ucrtbase._o__ui64tow
@ cdecl _o__ui64tow_s(int64 ptr long long) ucrtbase._o__ui64tow_s
@ cdecl _o__ultoa(long ptr long) ucrtbase._o__ultoa
@ cdecl _o__ultoa_s(long ptr long long) ucrtbase._o__ultoa_s
@ cdecl _o__ultow(long ptr long) ucrtbase._o__ultow
@ cdecl _o__ultow_s(long ptr long long) ucrtbase._o__ultow_s
@ cdecl _o__umask(long) ucrtbase._o__umask
@ stub _o__umask_s
@ cdecl _o__ungetc_nolock(long ptr) ucrtbase._o__ungetc_nolock
@ cdecl _o__ungetch(long) ucrtbase._o__ungetch
@ cdecl _o__ungetch_nolock(long) ucrtbase._o__ungetch_nolock
@ cdecl _o__ungetwc_nolock(long ptr) ucrtbase._o__ungetwc_nolock
@ cdecl _o__ungetwch(long) ucrtbase._o__ungetwch
@ cdecl _o__ungetwch_nolock(long) ucrtbase._o__ungetwch_nolock
@ cdecl _o__unlink(str) ucrtbase._o__unlink
@ cdecl _o__unloaddll(long) ucrtbase._o__unloaddll
@ cdecl _o__unlock_file(ptr) ucrtbase._o__unlock_file
@ cdecl _o__utime32(str ptr) ucrtbase._o__utime32
@ cdecl _o__utime64(str ptr) ucrtbase._o__utime64
@ cdecl _o__waccess(wstr long) ucrtbase._o__waccess
@ cdecl _o__waccess_s(wstr long) ucrtbase._o__waccess_s
@ cdecl _o__wasctime(ptr) ucrtbase._o__wasctime
@ cdecl _o__wasctime_s(ptr long ptr) ucrtbase._o__wasctime_s
@ cdecl _o__wchdir(wstr) ucrtbase._o__wchdir
@ cdecl _o__wchmod(wstr long) ucrtbase._o__wchmod
@ cdecl _o__wcreat(wstr long) ucrtbase._o__wcreat
@ cdecl _o__wcreate_locale(long wstr) ucrtbase._o__wcreate_locale
@ cdecl _o__wcscoll_l(wstr wstr ptr) ucrtbase._o__wcscoll_l
@ cdecl _o__wcsdup(wstr) ucrtbase._o__wcsdup
@ cdecl _o__wcserror(long) ucrtbase._o__wcserror
@ cdecl _o__wcserror_s(ptr long long) ucrtbase._o__wcserror_s
@ cdecl _o__wcsftime_l(ptr long wstr ptr ptr) ucrtbase._o__wcsftime_l
@ cdecl _o__wcsicmp(wstr wstr) ucrtbase._o__wcsicmp
@ cdecl _o__wcsicmp_l(wstr wstr ptr) ucrtbase._o__wcsicmp_l
@ cdecl _o__wcsicoll(wstr wstr) ucrtbase._o__wcsicoll
@ cdecl _o__wcsicoll_l(wstr wstr ptr) ucrtbase._o__wcsicoll_l
@ cdecl _o__wcslwr(wstr) ucrtbase._o__wcslwr
@ cdecl _o__wcslwr_l(wstr ptr) ucrtbase._o__wcslwr_l
@ cdecl _o__wcslwr_s(wstr long) ucrtbase._o__wcslwr_s
@ cdecl _o__wcslwr_s_l(wstr long ptr) ucrtbase._o__wcslwr_s_l
@ cdecl _o__wcsncoll(wstr wstr long) ucrtbase._o__wcsncoll
@ cdecl _o__wcsncoll_l(wstr wstr long ptr) ucrtbase._o__wcsncoll_l
@ cdecl _o__wcsnicmp(wstr wstr long) ucrtbase._o__wcsnicmp
@ cdecl _o__wcsnicmp_l(wstr wstr long ptr) ucrtbase._o__wcsnicmp_l
@ cdecl _o__wcsnicoll(wstr wstr long) ucrtbase._o__wcsnicoll
@ cdecl _o__wcsnicoll_l(wstr wstr long ptr) ucrtbase._o__wcsnicoll_l
@ cdecl _o__wcsnset(wstr long long) ucrtbase._o__wcsnset
@ cdecl _o__wcsnset_s(wstr long long long) ucrtbase._o__wcsnset_s
@ cdecl _o__wcsset(wstr long) ucrtbase._o__wcsset
@ cdecl _o__wcsset_s(wstr long long) ucrtbase._o__wcsset_s
@ cdecl _o__wcstod_l(wstr ptr ptr) ucrtbase._o__wcstod_l
@ cdecl _o__wcstof_l(wstr ptr ptr) ucrtbase._o__wcstof_l
@ cdecl -ret64 _o__wcstoi64(wstr ptr long) ucrtbase._o__wcstoi64
@ cdecl -ret64 _o__wcstoi64_l(wstr ptr long ptr) ucrtbase._o__wcstoi64_l
@ cdecl _o__wcstol_l(wstr ptr long ptr) ucrtbase._o__wcstol_l
@ cdecl _o__wcstold_l(wstr ptr ptr) ucrtbase._o__wcstold_l
@ cdecl -ret64 _o__wcstoll_l(wstr ptr long ptr) ucrtbase._o__wcstoll_l
@ cdecl _o__wcstombs_l(ptr ptr long ptr) ucrtbase._o__wcstombs_l
@ cdecl _o__wcstombs_s_l(ptr ptr long wstr long ptr) ucrtbase._o__wcstombs_s_l
@ cdecl -ret64 _o__wcstoui64(wstr ptr long) ucrtbase._o__wcstoui64
@ cdecl -ret64 _o__wcstoui64_l(wstr ptr long ptr) ucrtbase._o__wcstoui64_l
@ cdecl _o__wcstoul_l(wstr ptr long ptr) ucrtbase._o__wcstoul_l
@ cdecl -ret64 _o__wcstoull_l(wstr ptr long ptr) ucrtbase._o__wcstoull_l
@ cdecl _o__wcsupr(wstr) ucrtbase._o__wcsupr
@ cdecl _o__wcsupr_l(wstr ptr) ucrtbase._o__wcsupr_l
@ cdecl _o__wcsupr_s(wstr long) ucrtbase._o__wcsupr_s
@ cdecl _o__wcsupr_s_l(wstr long ptr) ucrtbase._o__wcsupr_s_l
@ cdecl _o__wcsxfrm_l(ptr wstr long ptr) ucrtbase._o__wcsxfrm_l
@ cdecl _o__wctime32(ptr) ucrtbase._o__wctime32
@ cdecl _o__wctime32_s(ptr long ptr) ucrtbase._o__wctime32_s
@ cdecl _o__wctime64(ptr) ucrtbase._o__wctime64
@ cdecl _o__wctime64_s(ptr long ptr) ucrtbase._o__wctime64_s
@ cdecl _o__wctomb_l(ptr long ptr) ucrtbase._o__wctomb_l
@ cdecl _o__wctomb_s_l(ptr ptr long long ptr) ucrtbase._o__wctomb_s_l
@ cdecl _o__wdupenv_s(ptr ptr wstr) ucrtbase._o__wdupenv_s
@ cdecl _o__wexecv(wstr ptr) ucrtbase._o__wexecv
@ cdecl _o__wexecve(wstr ptr ptr) ucrtbase._o__wexecve
@ cdecl _o__wexecvp(wstr ptr) ucrtbase._o__wexecvp
@ cdecl _o__wexecvpe(wstr ptr ptr) ucrtbase._o__wexecvpe
@ cdecl _o__wfdopen(long wstr) ucrtbase._o__wfdopen
@ cdecl _o__wfindfirst32(wstr ptr) ucrtbase._o__wfindfirst32
@ stub _o__wfindfirst32i64
@ cdecl _o__wfindfirst64(wstr ptr) ucrtbase._o__wfindfirst64
@ cdecl _o__wfindfirst64i32(wstr ptr) ucrtbase._o__wfindfirst64i32
@ cdecl _o__wfindnext32(long ptr) ucrtbase._o__wfindnext32
@ stub _o__wfindnext32i64
@ cdecl _o__wfindnext64(long ptr) ucrtbase._o__wfindnext64
@ cdecl _o__wfindnext64i32(long ptr) ucrtbase._o__wfindnext64i32
@ cdecl _o__wfopen(wstr wstr) ucrtbase._o__wfopen
@ cdecl _o__wfopen_s(ptr wstr wstr) ucrtbase._o__wfopen_s
@ cdecl _o__wfreopen(wstr wstr ptr) ucrtbase._o__wfreopen
@ cdecl _o__wfreopen_s(ptr wstr wstr ptr) ucrtbase._o__wfreopen_s
@ cdecl _o__wfsopen(wstr wstr long) ucrtbase._o__wfsopen
@ cdecl _o__wfullpath(ptr wstr long) ucrtbase._o__wfullpath
@ cdecl _o__wgetcwd(wstr long) ucrtbase._o__wgetcwd
@ cdecl _o__wgetdcwd(long wstr long) ucrtbase._o__wgetdcwd
@ cdecl _o__wgetenv(wstr) ucrtbase._o__wgetenv
@ cdecl _o__wgetenv_s(ptr ptr long wstr) ucrtbase._o__wgetenv_s
@ cdecl _o__wmakepath(ptr wstr wstr wstr wstr) ucrtbase._o__wmakepath
@ cdecl _o__wmakepath_s(ptr long wstr wstr wstr wstr) ucrtbase._o__wmakepath_s
@ cdecl _o__wmkdir(wstr) ucrtbase._o__wmkdir
@ cdecl _o__wmktemp(wstr) ucrtbase._o__wmktemp
@ cdecl _o__wmktemp_s(wstr long) ucrtbase._o__wmktemp_s
@ cdecl _o__wperror(wstr) ucrtbase._o__wperror
@ cdecl _o__wpopen(wstr wstr) ucrtbase._o__wpopen
@ cdecl _o__wputenv(wstr) ucrtbase._o__wputenv
@ cdecl _o__wputenv_s(wstr wstr) ucrtbase._o__wputenv_s
@ cdecl _o__wremove(wstr) ucrtbase._o__wremove
@ cdecl _o__wrename(wstr wstr) ucrtbase._o__wrename
@ cdecl _o__write(long ptr long) ucrtbase._o__write
@ cdecl _o__wrmdir(wstr) ucrtbase._o__wrmdir
@ cdecl _o__wsearchenv(wstr wstr ptr) ucrtbase._o__wsearchenv
@ cdecl _o__wsearchenv_s(wstr wstr ptr long) ucrtbase._o__wsearchenv_s
@ cdecl _o__wsetlocale(long wstr) ucrtbase._o__wsetlocale
@ cdecl _o__wsopen_dispatch(wstr long long long ptr long) ucrtbase._o__wsopen_dispatch
@ cdecl _o__wsopen_s(ptr wstr long long long) ucrtbase._o__wsopen_s
@ cdecl _o__wspawnv(long wstr ptr) ucrtbase._o__wspawnv
@ cdecl _o__wspawnve(long wstr ptr ptr) ucrtbase._o__wspawnve
@ cdecl _o__wspawnvp(long wstr ptr) ucrtbase._o__wspawnvp
@ cdecl _o__wspawnvpe(long wstr ptr ptr) ucrtbase._o__wspawnvpe
@ cdecl _o__wsplitpath(wstr ptr ptr ptr ptr) ucrtbase._o__wsplitpath
@ cdecl _o__wsplitpath_s(wstr ptr long ptr long ptr long ptr long) ucrtbase._o__wsplitpath_s
@ cdecl _o__wstat32(wstr ptr) ucrtbase._o__wstat32
@ cdecl _o__wstat32i64(wstr ptr) ucrtbase._o__wstat32i64
@ cdecl _o__wstat64(wstr ptr) ucrtbase._o__wstat64
@ cdecl _o__wstat64i32(wstr ptr) ucrtbase._o__wstat64i32
@ cdecl _o__wstrdate(ptr) ucrtbase._o__wstrdate
@ cdecl _o__wstrdate_s(ptr long) ucrtbase._o__wstrdate_s
@ cdecl _o__wstrtime(ptr) ucrtbase._o__wstrtime
@ cdecl _o__wstrtime_s(ptr long) ucrtbase._o__wstrtime_s
@ cdecl _o__wsystem(wstr) ucrtbase._o__wsystem
@ cdecl _o__wtmpnam_s(ptr long) ucrtbase._o__wtmpnam_s
@ cdecl _o__wtof(wstr) ucrtbase._o__wtof
@ cdecl _o__wtof_l(wstr ptr) ucrtbase._o__wtof_l
@ cdecl _o__wtoi(wstr) ucrtbase._o__wtoi
@ cdecl -ret64 _o__wtoi64(wstr) ucrtbase._o__wtoi64
@ cdecl -ret64 _o__wtoi64_l(wstr ptr) ucrtbase._o__wtoi64_l
@ cdecl _o__wtoi_l(wstr ptr) ucrtbase._o__wtoi_l
@ cdecl _o__wtol(wstr) ucrtbase._o__wtol
@ cdecl _o__wtol_l(wstr ptr) ucrtbase._o__wtol_l
@ cdecl -ret64 _o__wtoll(wstr) ucrtbase._o__wtoll
@ cdecl -ret64 _o__wtoll_l(wstr ptr) ucrtbase._o__wtoll_l
@ cdecl _o__wunlink(wstr) ucrtbase._o__wunlink
@ cdecl _o__wutime32(wstr ptr) ucrtbase._o__wutime32
@ cdecl _o__wutime64(wstr ptr) ucrtbase._o__wutime64
@ cdecl _o__y0(double) ucrtbase._o__y0
@ cdecl _o__y1(double) ucrtbase._o__y1
@ cdecl _o__yn(long double) ucrtbase._o__yn
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
@ cdecl _o_strtold(str ptr) ucrtbase._o_strtold
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
@ cdecl _o_wcstold(wstr ptr ptr) ucrtbase._o_wcstold
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
