@ cdecl -arch=i386 _CIacos()
@ cdecl -arch=i386 _CIasin()
@ cdecl -arch=i386 _CIatan()
@ cdecl -arch=i386 _CIatan2()
@ cdecl -arch=i386 _CIcos()
@ cdecl -arch=i386 _CIcosh()
@ cdecl -arch=i386 _CIexp()
@ cdecl -arch=i386 _CIfmod()
@ cdecl -arch=i386 _CIlog()
@ cdecl -arch=i386 _CIlog10()
@ cdecl -arch=i386 _CIpow()
@ cdecl -arch=i386 _CIsin()
@ cdecl -arch=i386 _CIsinh()
@ cdecl -arch=i386 _CIsqrt()
@ cdecl -arch=i386 _CItan()
@ cdecl -arch=i386 _CItanh()
@ cdecl _Cbuild(ptr double double) MSVCR120__Cbuild
@ stub _Cmulcc
@ stub _Cmulcr
@ cdecl _CreateFrameInfo(ptr ptr)
@ stdcall _CxxThrowException(ptr ptr)
@ cdecl -arch=i386 -norelay _EH_prolog()
@ cdecl _Exit(long) MSVCRT__exit
@ stub _FCbuild
@ stub _FCmulcc
@ stub _FCmulcr
@ cdecl _FindAndUnlinkFrame(ptr)
@ stub _GetImageBase
@ stub _GetThrowImageBase
@ cdecl _Getdays()
@ cdecl _Getmonths()
@ cdecl _Gettnames()
@ cdecl _IsExceptionObjectToBeDestroyed(ptr)
@ stub _LCbuild
@ stub _LCmulcc
@ stub _LCmulcr
@ stub _SetImageBase
@ stub _SetThrowImageBase
@ stub _NLG_Dispatch2
@ stub _NLG_Return
@ stub _NLG_Return2
@ cdecl _SetWinRTOutOfMemoryExceptionCallback(ptr) MSVCR120__SetWinRTOutOfMemoryExceptionCallback
@ cdecl _Strftime(ptr long str ptr ptr)
@ cdecl _W_Getdays()
@ cdecl _W_Getmonths()
@ cdecl _W_Gettnames()
@ cdecl _Wcsftime(ptr long wstr ptr ptr)
@ cdecl __AdjustPointer(ptr ptr)
@ stub __BuildCatchObject
@ stub __BuildCatchObjectHelper
@ stdcall -arch=x86_64,arm64 __C_specific_handler(ptr long ptr ptr) ntdll.__C_specific_handler
@ cdecl -arch=i386,x86_64,arm,arm64 __CxxDetectRethrow(ptr)
@ cdecl -arch=i386,x86_64,arm,arm64 __CxxExceptionFilter(ptr ptr long ptr)
@ cdecl -arch=i386,x86_64,arm,arm64 -norelay __CxxFrameHandler(ptr ptr ptr ptr)
@ cdecl -arch=i386,x86_64,arm,arm64 -norelay __CxxFrameHandler2(ptr ptr ptr ptr) __CxxFrameHandler
@ cdecl -arch=i386,x86_64,arm,arm64 -norelay __CxxFrameHandler3(ptr ptr ptr ptr) __CxxFrameHandler
@ stdcall -arch=i386 __CxxLongjmpUnwind(ptr)
@ cdecl -arch=i386,x86_64,arm,arm64 __CxxQueryExceptionSize()
@ cdecl __CxxRegisterExceptionObject(ptr ptr)
@ cdecl __CxxUnregisterExceptionObject(ptr long)
@ cdecl __DestructExceptionObject(ptr)
@ stub __FrameUnwindFilter
@ stub __GetPlatformExceptionInfo
@ stub __NLG_Dispatch2
@ stub __NLG_Return2
@ cdecl __RTCastToVoid(ptr) MSVCRT___RTCastToVoid
@ cdecl __RTDynamicCast(ptr long ptr ptr long) MSVCRT___RTDynamicCast
@ cdecl __RTtypeid(ptr) MSVCRT___RTtypeid
@ stub __TypeMatch
@ cdecl ___lc_codepage_func()
@ cdecl ___lc_collate_cp_func()
@ cdecl ___lc_locale_name_func()
@ cdecl ___mb_cur_max_func() MSVCRT____mb_cur_max_func
@ cdecl ___mb_cur_max_l_func(ptr)
@ cdecl __acrt_iob_func(long) MSVCRT___acrt_iob_func
@ cdecl __conio_common_vcprintf(int64 str ptr ptr) MSVCRT__conio_common_vcprintf
@ stub __conio_common_vcprintf_p
@ stub __conio_common_vcprintf_s
@ stub __conio_common_vcscanf
@ cdecl __conio_common_vcwprintf(int64 wstr ptr ptr) MSVCRT__conio_common_vcwprintf
@ stub __conio_common_vcwprintf_p
@ stub __conio_common_vcwprintf_s
@ stub __conio_common_vcwscanf
@ cdecl -arch=i386 __control87_2(long long ptr ptr)
@ cdecl __current_exception()
@ cdecl __current_exception_context()
@ cdecl __daylight() MSVCRT___p__daylight
@ stub __dcrt_get_wide_environment_from_os
@ stub __dcrt_initial_narrow_environment
@ cdecl __doserrno() MSVCRT___doserrno
@ cdecl __dstbias() MSVCRT___p__dstbias
@ cdecl __fpe_flt_rounds()
@ cdecl __fpecode()
@ cdecl __initialize_lconv_for_unsigned_char() __lconv_init
@ stub __intrinsic_abnormal_termination
@ cdecl -arch=i386,x86_64,arm,arm64 -norelay __intrinsic_setjmp(ptr) MSVCRT__setjmp
@ cdecl -arch=x86_64,arm64 -norelay __intrinsic_setjmpex(ptr ptr) __wine_setjmpex
@ cdecl __isascii(long) MSVCRT___isascii
@ cdecl __iscsym(long) MSVCRT___iscsym
@ cdecl __iscsymf(long) MSVCRT___iscsymf
@ stub __iswcsym
@ stub __iswcsymf
@ cdecl -arch=i386 -norelay __libm_sse2_acos() MSVCRT___libm_sse2_acos
@ cdecl -arch=i386 -norelay __libm_sse2_acosf() MSVCRT___libm_sse2_acosf
@ cdecl -arch=i386 -norelay __libm_sse2_asin() MSVCRT___libm_sse2_asin
@ cdecl -arch=i386 -norelay __libm_sse2_asinf() MSVCRT___libm_sse2_asinf
@ cdecl -arch=i386 -norelay __libm_sse2_atan() MSVCRT___libm_sse2_atan
@ cdecl -arch=i386 -norelay __libm_sse2_atan2() MSVCRT___libm_sse2_atan2
@ cdecl -arch=i386 -norelay __libm_sse2_atanf() MSVCRT___libm_sse2_atanf
@ cdecl -arch=i386 -norelay __libm_sse2_cos() MSVCRT___libm_sse2_cos
@ cdecl -arch=i386 -norelay __libm_sse2_cosf() MSVCRT___libm_sse2_cosf
@ cdecl -arch=i386 -norelay __libm_sse2_exp() MSVCRT___libm_sse2_exp
@ cdecl -arch=i386 -norelay __libm_sse2_expf() MSVCRT___libm_sse2_expf
@ cdecl -arch=i386 -norelay __libm_sse2_log() MSVCRT___libm_sse2_log
@ cdecl -arch=i386 -norelay __libm_sse2_log10() MSVCRT___libm_sse2_log10
@ cdecl -arch=i386 -norelay __libm_sse2_log10f() MSVCRT___libm_sse2_log10f
@ cdecl -arch=i386 -norelay __libm_sse2_logf() MSVCRT___libm_sse2_logf
@ cdecl -arch=i386 -norelay __libm_sse2_pow() MSVCRT___libm_sse2_pow
@ cdecl -arch=i386 -norelay __libm_sse2_powf() MSVCRT___libm_sse2_powf
@ cdecl -arch=i386 -norelay __libm_sse2_sin() MSVCRT___libm_sse2_sin
@ cdecl -arch=i386 -norelay __libm_sse2_sinf() MSVCRT___libm_sse2_sinf
@ cdecl -arch=i386 -norelay __libm_sse2_tan() MSVCRT___libm_sse2_tan
@ cdecl -arch=i386 -norelay __libm_sse2_tanf() MSVCRT___libm_sse2_tanf
@ cdecl __p___argc() MSVCRT___p___argc
@ cdecl __p___argv() MSVCRT___p___argv
@ cdecl __p___wargv() MSVCRT___p___wargv
@ cdecl __p__acmdln() MSVCRT___p__acmdln
@ cdecl __p__commode()
@ cdecl __p__environ() MSVCRT___p__environ
@ cdecl __p__fmode() MSVCRT___p__fmode
@ stub __p__mbcasemap()
@ cdecl __p__mbctype()
@ cdecl __p__pgmptr() MSVCRT___p__pgmptr
@ cdecl __p__wcmdln() MSVCRT___p__wcmdln
@ cdecl __p__wenviron() MSVCRT___p__wenviron
@ cdecl __p__wpgmptr() MSVCRT___p__wpgmptr
@ cdecl __pctype_func() MSVCRT___pctype_func
@ cdecl __processing_throw()
@ stub __pwctype_func
@ cdecl __pxcptinfoptrs() MSVCRT___pxcptinfoptrs
@ stub __report_gsfailure
@ cdecl __setusermatherr(ptr) MSVCRT___setusermatherr
@ cdecl __std_exception_copy(ptr ptr) MSVCRT___std_exception_copy
@ cdecl __std_exception_destroy(ptr) MSVCRT___std_exception_destroy
@ cdecl __std_type_info_compare(ptr ptr) MSVCRT_type_info_compare
@ cdecl __std_type_info_destroy_list(ptr) MSVCRT_type_info_destroy_list
@ cdecl __std_type_info_hash(ptr) MSVCRT_type_info_hash
@ cdecl __std_type_info_name(ptr ptr) MSVCRT_type_info_name_list
@ cdecl __stdio_common_vfprintf(int64 ptr str ptr ptr) MSVCRT__stdio_common_vfprintf
@ stub __stdio_common_vfprintf_p
@ cdecl __stdio_common_vfprintf_s(int64 ptr str ptr ptr) MSVCRT__stdio_common_vfprintf_s
@ cdecl __stdio_common_vfscanf(int64 ptr str ptr ptr) MSVCRT__stdio_common_vfscanf
@ cdecl __stdio_common_vfwprintf(int64 ptr wstr ptr ptr) MSVCRT__stdio_common_vfwprintf
@ stub __stdio_common_vfwprintf_p
@ cdecl __stdio_common_vfwprintf_s(int64 ptr wstr ptr ptr) MSVCRT__stdio_common_vfwprintf_s
@ cdecl __stdio_common_vfwscanf(int64 ptr wstr ptr ptr) MSVCRT__stdio_common_vfwscanf
@ cdecl __stdio_common_vsnprintf_s(int64 ptr long long str ptr ptr) MSVCRT__stdio_common_vsnprintf_s
@ cdecl __stdio_common_vsnwprintf_s(int64 ptr long long wstr ptr ptr) MSVCRT__stdio_common_vsnwprintf_s
@ cdecl -norelay __stdio_common_vsprintf(int64 ptr long str ptr ptr)
@ cdecl __stdio_common_vsprintf_p(int64 ptr long str ptr ptr) MSVCRT__stdio_common_vsprintf_p
@ cdecl __stdio_common_vsprintf_s(int64 ptr long str ptr ptr) MSVCRT__stdio_common_vsprintf_s
@ cdecl __stdio_common_vsscanf(int64 ptr long str ptr ptr) MSVCRT__stdio_common_vsscanf
@ cdecl __stdio_common_vswprintf(int64 ptr long wstr ptr ptr) MSVCRT__stdio_common_vswprintf
@ cdecl __stdio_common_vswprintf_p(int64 ptr long wstr ptr ptr) MSVCRT__stdio_common_vswprintf_p
@ cdecl __stdio_common_vswprintf_s(int64 ptr long wstr ptr ptr) MSVCRT__stdio_common_vswprintf_s
@ cdecl __stdio_common_vswscanf(int64 ptr long wstr ptr ptr) MSVCRT__stdio_common_vswscanf
@ cdecl __strncnt(str long) MSVCRT___strncnt
@ cdecl __sys_errlist()
@ cdecl __sys_nerr()
@ cdecl __threadhandle() kernel32.GetCurrentThread
@ cdecl __threadid() kernel32.GetCurrentThreadId
@ cdecl __timezone() MSVCRT___p__timezone
@ cdecl __toascii(long) MSVCRT___toascii
@ cdecl __tzname() __p__tzname
@ cdecl __unDName(ptr str long ptr ptr long)
@ cdecl __unDNameEx(ptr str long ptr ptr ptr long)
@ cdecl __uncaught_exception() MSVCRT___uncaught_exception
@ cdecl __wcserror(wstr) MSVCRT___wcserror
@ cdecl __wcserror_s(ptr long wstr) MSVCRT___wcserror_s
@ stub __wcsncnt
@ cdecl -ret64 _abs64(int64)
@ cdecl _access(str long) MSVCRT__access
@ cdecl _access_s(str long) MSVCRT__access_s
@ cdecl _aligned_free(ptr)
@ cdecl _aligned_malloc(long long)
@ cdecl _aligned_msize(ptr long long)
@ cdecl _aligned_offset_malloc(long long long)
@ cdecl _aligned_offset_realloc(ptr long long long)
@ stub _aligned_offset_recalloc
@ cdecl _aligned_realloc(ptr long long)
@ stub _aligned_recalloc
@ cdecl _assert(str str long)
@ cdecl _atodbl(ptr str) MSVCRT__atodbl
@ cdecl _atodbl_l(ptr str ptr) MSVCRT__atodbl_l
@ cdecl _atof_l(str ptr) MSVCRT__atof_l
@ cdecl _atoflt(ptr str) MSVCRT__atoflt
@ cdecl _atoflt_l(ptr str ptr) MSVCRT__atoflt_l
@ cdecl -ret64 _atoi64(str) MSVCRT__atoi64
@ cdecl -ret64 _atoi64_l(str ptr) MSVCRT__atoi64_l
@ cdecl _atoi_l(str ptr) MSVCRT__atoi_l
@ cdecl _atol_l(str ptr) MSVCRT__atol_l
@ cdecl _atoldbl(ptr str) MSVCRT__atoldbl
@ cdecl _atoldbl_l(ptr str ptr) MSVCRT__atoldbl_l
@ cdecl -ret64 _atoll_l(str ptr) MSVCRT__atoll_l
@ cdecl _beep(long long) MSVCRT__beep
@ cdecl _beginthread(ptr long ptr)
@ cdecl _beginthreadex(ptr long ptr ptr long ptr)
@ cdecl _byteswap_uint64(int64)
@ cdecl _byteswap_ulong(long) MSVCRT__byteswap_ulong
@ cdecl _byteswap_ushort(long)
@ cdecl _c_exit() MSVCRT__c_exit
@ cdecl _cabs(long) MSVCRT__cabs
@ cdecl _callnewh(long)
@ cdecl _calloc_base(long long)
@ cdecl _cexit() MSVCRT__cexit
@ cdecl _cgets(ptr)
@ stub _cgets_s
@ stub _cgetws
@ stub _cgetws_s
@ cdecl _chdir(str) MSVCRT__chdir
@ cdecl _chdrive(long) MSVCRT__chdrive
@ cdecl _chgsign(double) MSVCRT__chgsign
@ cdecl _chgsignf(float) MSVCRT__chgsignf
@ cdecl -arch=i386 -norelay _chkesp()
@ cdecl _chmod(str long) MSVCRT__chmod
@ cdecl _chsize(long long) MSVCRT__chsize
@ cdecl _chsize_s(long int64) MSVCRT__chsize_s
@ cdecl _clearfp()
@ cdecl _close(long) MSVCRT__close
@ cdecl _commit(long) MSVCRT__commit
@ cdecl _configthreadlocale(long)
@ cdecl _configure_narrow_argv(long)
@ cdecl _configure_wide_argv(long)
@ cdecl _control87(long long)
@ cdecl _controlfp(long long)
@ cdecl _controlfp_s(ptr long long)
@ cdecl _copysign(double double) MSVCRT__copysign
@ cdecl _copysignf(float float) MSVCRT__copysignf
@ cdecl _cputs(str)
@ cdecl _cputws(wstr)
@ cdecl _creat(str long) MSVCRT__creat
@ cdecl _create_locale(long str) MSVCRT__create_locale
@ cdecl _crt_at_quick_exit(ptr) MSVCRT__crt_at_quick_exit
@ cdecl _crt_atexit(ptr) MSVCRT__crt_atexit
@ cdecl _crt_debugger_hook(long) MSVCRT__crt_debugger_hook
@ cdecl _ctime32(ptr) MSVCRT__ctime32
@ cdecl _ctime32_s(str long ptr) MSVCRT__ctime32_s
@ cdecl _ctime64(ptr) MSVCRT__ctime64
@ cdecl _ctime64_s(str long ptr) MSVCRT__ctime64_s
@ cdecl _cwait(ptr long long)
@ stub _d_int
@ cdecl _dclass(double) MSVCR120__dclass
@ stub _dexp
@ cdecl _difftime32(long long) MSVCRT__difftime32
@ cdecl _difftime64(int64 int64) MSVCRT__difftime64
@ stub _dlog
@ stub _dnorm
@ cdecl _dpcomp(double double) MSVCR120__dpcomp
@ stub _dpoly
@ stub _dscale
@ cdecl _dsign(double) MSVCR120__dsign
@ stub _dsin
@ cdecl _dtest(ptr) MSVCR120__dtest
@ stub _dunscale
@ cdecl _dup(long) MSVCRT__dup
@ cdecl _dup2(long long) MSVCRT__dup2
@ cdecl _dupenv_s(ptr ptr str)
@ cdecl _ecvt(double long ptr ptr) MSVCRT__ecvt
@ cdecl _ecvt_s(str long double long ptr ptr) MSVCRT__ecvt_s
@ cdecl _endthread()
@ cdecl _endthreadex(long)
@ cdecl _eof(long) MSVCRT__eof
@ cdecl _errno() MSVCRT__errno
@ cdecl _except1(long long double double long ptr)
@ cdecl -arch=i386 _except_handler2(ptr ptr ptr ptr)
@ cdecl -arch=i386 _except_handler3(ptr ptr ptr ptr)
@ cdecl -arch=i386 _except_handler4_common(ptr ptr ptr ptr ptr ptr)
@ varargs _execl(str str)
@ varargs _execle(str str)
@ varargs _execlp(str str)
@ varargs _execlpe(str str)
@ cdecl _execute_onexit_table(ptr) MSVCRT__execute_onexit_table
@ cdecl _execv(str ptr)
@ cdecl _execve(str ptr ptr) MSVCRT__execve
@ cdecl _execvp(str ptr)
@ cdecl _execvpe(str ptr ptr)
@ cdecl _exit(long) MSVCRT__exit
@ cdecl _expand(ptr long)
@ cdecl _fclose_nolock(ptr) MSVCRT__fclose_nolock
@ cdecl _fcloseall() MSVCRT__fcloseall
@ cdecl _fcvt(double long ptr ptr) MSVCRT__fcvt
@ cdecl _fcvt_s(ptr long double long ptr ptr) MSVCRT__fcvt_s
@ stub _fd_int
@ cdecl _fdclass(float) MSVCR120__fdclass
@ stub _fdexp
@ stub _fdlog
@ stub _fdnorm
@ cdecl _fdopen(long str) MSVCRT__fdopen
@ cdecl _fdpcomp(float float) MSVCR120__fdpcomp
@ stub _fdpoly
@ stub _fdscale
@ cdecl _fdsign(float) MSVCR120__fdsign
@ stub _fdsin
@ cdecl _fdtest(ptr) MSVCR120__fdtest
@ stub _fdunscale
@ cdecl _fflush_nolock(ptr) MSVCRT__fflush_nolock
@ cdecl _fgetc_nolock(ptr) MSVCRT__fgetc_nolock
@ cdecl _fgetchar() MSVCRT__fgetchar
@ cdecl _fgetwc_nolock(ptr) MSVCRT__fgetwc_nolock
@ cdecl _fgetwchar() MSVCRT__fgetwchar
@ cdecl _filelength(long) MSVCRT__filelength
@ cdecl -ret64 _filelengthi64(long) MSVCRT__filelengthi64
@ cdecl _fileno(ptr) MSVCRT__fileno
@ cdecl _findclose(long) MSVCRT__findclose
@ cdecl _findfirst32(str ptr) MSVCRT__findfirst32
@ stub _findfirst32i64
@ cdecl _findfirst64(str ptr) MSVCRT__findfirst64
@ cdecl _findfirst64i32(str ptr) MSVCRT__findfirst64i32
@ cdecl _findnext32(long ptr) MSVCRT__findnext32
@ stub _findnext32i64
@ cdecl _findnext64(long ptr) MSVCRT__findnext64
@ cdecl _findnext64i32(long ptr) MSVCRT__findnext64i32
@ cdecl _finite(double) MSVCRT__finite
@ cdecl -arch=!i386 _finitef(float) MSVCRT__finitef
@ cdecl _flushall() MSVCRT__flushall
@ cdecl _fpclass(double) MSVCRT__fpclass
@ cdecl -arch=!i386 _fpclassf(float) MSVCRT__fpclassf
@ cdecl -arch=i386,x86_64,arm,arm64 _fpieee_flt(long ptr ptr)
@ cdecl _fpreset()
@ cdecl _fputc_nolock(long ptr) MSVCRT__fputc_nolock
@ cdecl _fputchar(long) MSVCRT__fputchar
@ cdecl _fputwc_nolock(long ptr) MSVCRT__fputwc_nolock
@ cdecl _fputwchar(long) MSVCRT__fputwchar
@ cdecl _fread_nolock(ptr long long ptr) MSVCRT__fread_nolock
@ cdecl _fread_nolock_s(ptr long long long ptr) MSVCRT__fread_nolock_s
@ cdecl _free_base(ptr)
@ cdecl _free_locale(ptr) MSVCRT__free_locale
@ cdecl _fseek_nolock(ptr long long) MSVCRT__fseek_nolock
@ cdecl _fseeki64(ptr int64 long) MSVCRT__fseeki64
@ cdecl _fseeki64_nolock(ptr int64 long) MSVCRT__fseeki64_nolock
@ cdecl _fsopen(str str long) MSVCRT__fsopen
@ cdecl _fstat32(long ptr) MSVCRT__fstat32
@ cdecl _fstat32i64(long ptr) MSVCRT__fstat32i64
@ cdecl _fstat64(long ptr) MSVCRT__fstat64
@ cdecl _fstat64i32(long ptr) MSVCRT__fstat64i32
@ cdecl _ftell_nolock(ptr) MSVCRT__ftell_nolock
@ cdecl -ret64 _ftelli64(ptr) MSVCRT__ftelli64
@ cdecl -ret64 _ftelli64_nolock(ptr) MSVCRT__ftelli64_nolock
@ cdecl _ftime32(ptr) MSVCRT__ftime32
@ cdecl _ftime32_s(ptr) MSVCRT__ftime32_s
@ cdecl _ftime64(ptr) MSVCRT__ftime64
@ cdecl _ftime64_s(ptr) MSVCRT__ftime64_s
@ cdecl -arch=i386 -ret64 _ftol() MSVCRT__ftol
@ cdecl _fullpath(ptr str long) MSVCRT__fullpath
@ cdecl _futime32(long ptr)
@ cdecl _futime64(long ptr)
@ cdecl _fwrite_nolock(ptr long long ptr) MSVCRT__fwrite_nolock
@ cdecl _gcvt(double long str) MSVCRT__gcvt
@ cdecl _gcvt_s(ptr long double long) MSVCRT__gcvt_s
@ cdecl -arch=win64 _get_FMA3_enable() MSVCRT__get_FMA3_enable
@ cdecl _get_current_locale() MSVCRT__get_current_locale
@ cdecl _get_daylight(ptr)
@ cdecl _get_doserrno(ptr)
@ cdecl _get_dstbias(ptr) MSVCRT__get_dstbias
@ cdecl _get_errno(ptr)
@ cdecl _get_fmode(ptr) MSVCRT__get_fmode
@ cdecl _get_heap_handle()
@ cdecl _get_initial_narrow_environment()
@ cdecl _get_initial_wide_environment()
@ cdecl _get_invalid_parameter_handler()
@ cdecl _get_narrow_winmain_command_line()
@ cdecl _get_osfhandle(long) MSVCRT__get_osfhandle
@ cdecl _get_pgmptr(ptr)
@ cdecl _get_printf_count_output() MSVCRT__get_printf_count_output
@ cdecl _get_purecall_handler()
@ cdecl _get_stream_buffer_pointers(ptr ptr ptr ptr) MSVCRT__get_stream_buffer_pointers
@ cdecl _get_terminate() MSVCRT__get_terminate
@ cdecl _get_thread_local_invalid_parameter_handler()
@ cdecl _get_timezone(ptr)
@ cdecl _get_tzname(ptr str long long) MSVCRT__get_tzname
@ cdecl _get_unexpected() MSVCRT__get_unexpected
@ cdecl _get_wide_winmain_command_line()
@ cdecl _get_wpgmptr(ptr)
@ cdecl _getc_nolock(ptr) MSVCRT__fgetc_nolock
@ cdecl _getch()
@ cdecl _getch_nolock()
@ cdecl _getche()
@ cdecl _getche_nolock()
@ cdecl _getcwd(str long) MSVCRT__getcwd
@ cdecl _getdcwd(long str long) MSVCRT__getdcwd
@ cdecl _getdiskfree(long ptr) MSVCRT__getdiskfree
@ cdecl _getdllprocaddr(long str long)
@ cdecl _getdrive() MSVCRT__getdrive
@ cdecl _getdrives() kernel32.GetLogicalDrives
@ cdecl _getmaxstdio() MSVCRT__getmaxstdio
@ cdecl _getmbcp()
@ cdecl _getpid() _getpid
@ stub _getsystime(ptr)
@ cdecl _getw(ptr) MSVCRT__getw
@ cdecl _getwc_nolock(ptr) MSVCRT__fgetwc_nolock
@ cdecl _getwch()
@ cdecl _getwch_nolock()
@ cdecl _getwche()
@ cdecl _getwche_nolock()
@ cdecl _getws(ptr) MSVCRT__getws
@ stub _getws_s
@ cdecl -arch=i386 _global_unwind2(ptr)
@ cdecl _gmtime32(ptr) MSVCRT__gmtime32
@ cdecl _gmtime32_s(ptr ptr) MSVCRT__gmtime32_s
@ cdecl _gmtime64(ptr) MSVCRT__gmtime64
@ cdecl _gmtime64_s(ptr ptr) MSVCRT__gmtime64_s
@ cdecl _heapchk()
@ cdecl _heapmin()
@ cdecl _heapwalk(ptr)
@ cdecl _hypot(double double)
@ cdecl _hypotf(float float) MSVCRT__hypotf
@ cdecl _i64toa(int64 ptr long) ntdll._i64toa
@ cdecl _i64toa_s(int64 ptr long long) MSVCRT__i64toa_s
@ cdecl _i64tow(int64 ptr long) ntdll._i64tow
@ cdecl _i64tow_s(int64 ptr long long) MSVCRT__i64tow_s
@ cdecl _initialize_narrow_environment()
@ cdecl _initialize_onexit_table(ptr) MSVCRT__initialize_onexit_table
@ cdecl _initialize_wide_environment()
@ cdecl _initterm(ptr ptr)
@ cdecl _initterm_e(ptr ptr)
@ cdecl _invalid_parameter_noinfo()
@ cdecl _invalid_parameter_noinfo_noreturn()
@ stub _invoke_watson
@ stub _is_exception_typeof
@ cdecl _isalnum_l(long ptr) MSVCRT__isalnum_l
@ cdecl _isalpha_l(long ptr) MSVCRT__isalpha_l
@ cdecl _isatty(long) MSVCRT__isatty
@ cdecl _isblank_l(long ptr) MSVCRT__isblank_l
@ cdecl _iscntrl_l(long ptr) MSVCRT__iscntrl_l
@ cdecl _isctype(long long) MSVCRT__isctype
@ cdecl _isctype_l(long long ptr) MSVCRT__isctype_l
@ cdecl _isdigit_l(long ptr) MSVCRT__isdigit_l
@ cdecl _isgraph_l(long ptr) MSVCRT__isgraph_l
@ cdecl _isleadbyte_l(long ptr) MSVCRT__isleadbyte_l
@ cdecl _islower_l(long ptr) MSVCRT__islower_l
@ stub _ismbbalnum(long)
@ stub _ismbbalnum_l
@ stub _ismbbalpha(long)
@ stub _ismbbalpha_l
@ stub _ismbbblank
@ stub _ismbbblank_l
@ stub _ismbbgraph(long)
@ stub _ismbbgraph_l
@ stub _ismbbkalnum(long)
@ stub _ismbbkalnum_l
@ cdecl _ismbbkana(long)
@ cdecl _ismbbkana_l(long ptr)
@ stub _ismbbkprint(long)
@ stub _ismbbkprint_l
@ stub _ismbbkpunct(long)
@ stub _ismbbkpunct_l
@ cdecl _ismbblead(long)
@ cdecl _ismbblead_l(long ptr)
@ stub _ismbbprint(long)
@ stub _ismbbprint_l
@ stub _ismbbpunct(long)
@ stub _ismbbpunct_l
@ cdecl _ismbbtrail(long)
@ cdecl _ismbbtrail_l(long ptr)
@ cdecl _ismbcalnum(long)
@ cdecl _ismbcalnum_l(long ptr)
@ cdecl _ismbcalpha(long)
@ cdecl _ismbcalpha_l(long ptr)
@ stub _ismbcblank
@ stub _ismbcblank_l
@ cdecl _ismbcdigit(long)
@ cdecl _ismbcdigit_l(long ptr)
@ cdecl _ismbcgraph(long)
@ cdecl _ismbcgraph_l(long ptr)
@ cdecl _ismbchira(long)
@ stub _ismbchira_l
@ cdecl _ismbckata(long)
@ stub _ismbckata_l
@ cdecl _ismbcl0(long)
@ cdecl _ismbcl0_l(long ptr)
@ cdecl _ismbcl1(long)
@ cdecl _ismbcl1_l(long ptr)
@ cdecl _ismbcl2(long)
@ cdecl _ismbcl2_l(long ptr)
@ cdecl _ismbclegal(long)
@ cdecl _ismbclegal_l(long ptr)
@ stub _ismbclower(long)
@ cdecl _ismbclower_l(long ptr)
@ cdecl _ismbcprint(long)
@ cdecl _ismbcprint_l(long ptr)
@ cdecl _ismbcpunct(long)
@ cdecl _ismbcpunct_l(long ptr)
@ cdecl _ismbcspace(long)
@ cdecl _ismbcspace_l(long ptr)
@ cdecl _ismbcsymbol(long)
@ stub _ismbcsymbol_l
@ cdecl _ismbcupper(long)
@ cdecl _ismbcupper_l(long ptr)
@ cdecl _ismbslead(ptr ptr)
@ stub _ismbslead_l
@ cdecl _ismbstrail(ptr ptr)
@ stub _ismbstrail_l
@ cdecl _isnan(double) MSVCRT__isnan
@ cdecl -arch=x86_64 _isnanf(float) MSVCRT__isnanf
@ cdecl _isprint_l(long ptr) MSVCRT__isprint_l
@ cdecl _ispunct_l(long ptr) MSVCRT__ispunct_l
@ cdecl _isspace_l(long ptr) MSVCRT__isspace_l
@ cdecl _isupper_l(long ptr) MSVCRT__isupper_l
@ cdecl _iswalnum_l(long ptr) MSVCRT__iswalnum_l
@ cdecl _iswalpha_l(long ptr) MSVCRT__iswalpha_l
@ cdecl _iswblank_l(long ptr) MSVCRT__iswblank_l
@ cdecl _iswcntrl_l(long ptr) MSVCRT__iswcntrl_l
@ stub _iswcsym_l
@ stub _iswcsymf_l
@ cdecl _iswctype_l(long long ptr) MSVCRT__iswctype_l
@ cdecl _iswdigit_l(long ptr) MSVCRT__iswdigit_l
@ cdecl _iswgraph_l(long ptr) MSVCRT__iswgraph_l
@ cdecl _iswlower_l(long ptr) MSVCRT__iswlower_l
@ cdecl _iswprint_l(long ptr) MSVCRT__iswprint_l
@ cdecl _iswpunct_l(long ptr) MSVCRT__iswpunct_l
@ cdecl _iswspace_l(long ptr) MSVCRT__iswspace_l
@ cdecl _iswupper_l(long ptr) MSVCRT__iswupper_l
@ cdecl _iswxdigit_l(long ptr) MSVCRT__iswxdigit_l
@ cdecl _isxdigit_l(long ptr) MSVCRT__isxdigit_l
@ cdecl _itoa(long ptr long) MSVCRT__itoa
@ cdecl _itoa_s(long ptr long long) MSVCRT__itoa_s
@ cdecl _itow(long ptr long) ntdll._itow
@ cdecl _itow_s(long ptr long long) MSVCRT__itow_s
@ cdecl _j0(double) MSVCRT__j0
@ cdecl _j1(double) MSVCRT__j1
@ cdecl _jn(long double) MSVCRT__jn
@ cdecl _kbhit()
@ stub _ld_int
@ cdecl _ldclass(double) MSVCR120__ldclass
@ stub _ldexp
@ stub _ldlog
@ cdecl _ldpcomp(double double) MSVCR120__dpcomp
@ stub _ldpoly
@ stub _ldscale
@ cdecl _ldsign(double) MSVCR120__dsign
@ stub _ldsin
@ cdecl _ldtest(ptr) MSVCR120__ldtest
@ stub _ldunscale
@ cdecl _lfind(ptr ptr ptr long ptr)
@ cdecl _lfind_s(ptr ptr ptr long ptr ptr)
@ cdecl -arch=i386 -norelay _libm_sse2_acos_precise() MSVCRT___libm_sse2_acos
@ cdecl -arch=i386 -norelay _libm_sse2_asin_precise() MSVCRT___libm_sse2_asin
@ cdecl -arch=i386 -norelay _libm_sse2_atan_precise() MSVCRT___libm_sse2_atan
@ cdecl -arch=i386 -norelay _libm_sse2_cos_precise() MSVCRT___libm_sse2_cos
@ cdecl -arch=i386 -norelay _libm_sse2_exp_precise() MSVCRT___libm_sse2_exp
@ cdecl -arch=i386 -norelay _libm_sse2_log10_precise() MSVCRT___libm_sse2_log10
@ cdecl -arch=i386 -norelay _libm_sse2_log_precise() MSVCRT___libm_sse2_log
@ cdecl -arch=i386 -norelay _libm_sse2_pow_precise() MSVCRT___libm_sse2_pow
@ cdecl -arch=i386 -norelay _libm_sse2_sin_precise() MSVCRT___libm_sse2_sin
@ cdecl -arch=i386 -norelay _libm_sse2_sqrt_precise() MSVCRT___libm_sse2_sqrt_precise
@ cdecl -arch=i386 -norelay _libm_sse2_tan_precise() MSVCRT___libm_sse2_tan
@ cdecl _loaddll(str)
@ cdecl -arch=x86_64,arm64 _local_unwind(ptr ptr)
@ cdecl -arch=i386 _local_unwind2(ptr long)
@ cdecl -arch=i386 _local_unwind4(ptr ptr long)
@ cdecl _localtime32(ptr) MSVCRT__localtime32
@ cdecl _localtime32_s(ptr ptr)
@ cdecl _localtime64(ptr) MSVCRT__localtime64
@ cdecl _localtime64_s(ptr ptr)
@ cdecl _lock_file(ptr) MSVCRT__lock_file
@ cdecl _lock_locales()
@ cdecl _locking(long long long) MSVCRT__locking
@ cdecl _logb(double) MSVCRT__logb
@ cdecl -arch=!i386 _logbf(float) MSVCRT__logbf
@ cdecl -arch=i386 _longjmpex(ptr long) MSVCRT_longjmp
@ cdecl _lrotl(long long) MSVCRT__lrotl
@ cdecl _lrotr(long long) MSVCRT__lrotr
@ cdecl _lsearch(ptr ptr ptr long ptr)
@ stub _lsearch_s
@ cdecl _lseek(long long long) MSVCRT__lseek
@ cdecl -ret64 _lseeki64(long int64 long) MSVCRT__lseeki64
@ cdecl _ltoa(long ptr long) ntdll._ltoa
@ cdecl _ltoa_s(long ptr long long) MSVCRT__ltoa_s
@ cdecl _ltow(long ptr long) ntdll._ltow
@ cdecl _ltow_s(long ptr long long) MSVCRT__ltow_s
@ cdecl _makepath(ptr str str str str) MSVCRT__makepath
@ cdecl _makepath_s(ptr long str str str str) MSVCRT__makepath_s
@ cdecl _malloc_base(long)
@ cdecl _mbbtombc(long)
@ stub _mbbtombc_l
@ cdecl _mbbtype(long long)
@ cdecl _mbbtype_l(long long ptr)
@ stub _mbcasemap
@ cdecl _mbccpy(ptr ptr)
@ cdecl _mbccpy_l(ptr ptr ptr)
@ cdecl _mbccpy_s(ptr long ptr ptr)
@ cdecl _mbccpy_s_l(ptr long ptr ptr ptr)
@ cdecl _mbcjistojms(long)
@ stub _mbcjistojms_l
@ cdecl _mbcjmstojis(long)
@ stub _mbcjmstojis_l
@ cdecl _mbclen(ptr)
@ stub _mbclen_l
@ cdecl _mbctohira(long)
@ stub _mbctohira_l
@ cdecl _mbctokata(long)
@ stub _mbctokata_l
@ cdecl _mbctolower(long)
@ stub _mbctolower_l
@ cdecl _mbctombb(long)
@ stub _mbctombb_l
@ cdecl _mbctoupper(long)
@ stub _mbctoupper_l
@ stub _mblen_l
@ cdecl _mbsbtype(str long)
@ stub _mbsbtype_l
@ cdecl _mbscat_s(ptr long str)
@ cdecl _mbscat_s_l(ptr long str ptr)
@ cdecl _mbschr(str long)
@ stub _mbschr_l
@ cdecl _mbscmp(str str)
@ cdecl _mbscmp_l(str str ptr)
@ cdecl _mbscoll(str str)
@ cdecl _mbscoll_l(str str ptr)
@ cdecl _mbscpy_s(ptr long str)
@ cdecl _mbscpy_s_l(ptr long str ptr)
@ cdecl _mbscspn(str str)
@ cdecl _mbscspn_l(str str ptr)
@ cdecl _mbsdec(ptr ptr)
@ stub _mbsdec_l
@ stub _mbsdup(str)
@ cdecl _mbsicmp(str str)
@ stub _mbsicmp_l
@ cdecl _mbsicoll(str str)
@ cdecl _mbsicoll_l(str str ptr)
@ cdecl _mbsinc(str)
@ stub _mbsinc_l
@ cdecl _mbslen(str)
@ cdecl _mbslen_l(str ptr)
@ cdecl _mbslwr(str)
@ stub _mbslwr_l
@ cdecl _mbslwr_s(str long)
@ stub _mbslwr_s_l
@ cdecl _mbsnbcat(str str long)
@ stub _mbsnbcat_l
@ cdecl _mbsnbcat_s(str long ptr long)
@ stub _mbsnbcat_s_l
@ cdecl _mbsnbcmp(str str long)
@ stub _mbsnbcmp_l
@ cdecl _mbsnbcnt(ptr long)
@ stub _mbsnbcnt_l
@ cdecl _mbsnbcoll(str str long)
@ cdecl _mbsnbcoll_l(str str long ptr)
@ cdecl _mbsnbcpy(ptr str long)
@ stub _mbsnbcpy_l
@ cdecl _mbsnbcpy_s(ptr long str long)
@ cdecl _mbsnbcpy_s_l(ptr long str long ptr)
@ cdecl _mbsnbicmp(str str long)
@ stub _mbsnbicmp_l
@ cdecl _mbsnbicoll(str str long)
@ cdecl _mbsnbicoll_l(str str long ptr)
@ cdecl _mbsnbset(ptr long long)
@ stub _mbsnbset_l
@ stub _mbsnbset_s
@ stub _mbsnbset_s_l
@ cdecl _mbsncat(str str long)
@ stub _mbsncat_l
@ stub _mbsncat_s
@ stub _mbsncat_s_l
@ cdecl _mbsnccnt(str long)
@ stub _mbsnccnt_l
@ cdecl _mbsncmp(str str long)
@ stub _mbsncmp_l
@ stub _mbsncoll(str str long)
@ stub _mbsncoll_l
@ cdecl _mbsncpy(ptr str long)
@ stub _mbsncpy_l
@ stub _mbsncpy_s
@ stub _mbsncpy_s_l
@ cdecl _mbsnextc(str)
@ cdecl _mbsnextc_l(str ptr)
@ cdecl _mbsnicmp(str str long)
@ stub _mbsnicmp_l
@ stub _mbsnicoll(str str long)
@ stub _mbsnicoll_l
@ cdecl _mbsninc(str long)
@ stub _mbsninc_l
@ cdecl _mbsnlen(str long)
@ cdecl _mbsnlen_l(str long ptr)
@ cdecl _mbsnset(ptr long long)
@ stub _mbsnset_l
@ stub _mbsnset_s
@ stub _mbsnset_s_l
@ cdecl _mbspbrk(str str)
@ stub _mbspbrk_l
@ cdecl _mbsrchr(str long)
@ stub _mbsrchr_l
@ cdecl _mbsrev(str)
@ stub _mbsrev_l
@ cdecl _mbsset(ptr long)
@ stub _mbsset_l
@ stub _mbsset_s
@ stub _mbsset_s_l
@ cdecl _mbsspn(str str)
@ cdecl _mbsspn_l(str str ptr)
@ cdecl _mbsspnp(str str)
@ stub _mbsspnp_l
@ cdecl _mbsstr(str str)
@ stub _mbsstr_l
@ cdecl _mbstok(str str)
@ cdecl _mbstok_l(str str ptr)
@ cdecl _mbstok_s(str str ptr)
@ cdecl _mbstok_s_l(str str ptr ptr)
@ cdecl _mbstowcs_l(ptr str long ptr) MSVCRT__mbstowcs_l
@ cdecl _mbstowcs_s_l(ptr ptr long str long ptr) MSVCRT__mbstowcs_s_l
@ cdecl _mbstrlen(str)
@ cdecl _mbstrlen_l(str ptr)
@ stub _mbstrnlen
@ stub _mbstrnlen_l
@ cdecl _mbsupr(str)
@ stub _mbsupr_l
@ cdecl _mbsupr_s(str long)
@ stub _mbsupr_s_l
@ cdecl _mbtowc_l(ptr str long ptr) MSVCRT_mbtowc_l
@ cdecl _memccpy(ptr ptr long long) ntdll._memccpy
@ cdecl _memicmp(str str long) MSVCRT__memicmp
@ cdecl _memicmp_l(str str long ptr) MSVCRT__memicmp_l
@ cdecl _mkdir(str) MSVCRT__mkdir
@ cdecl _mkgmtime32(ptr) MSVCRT__mkgmtime32
@ cdecl _mkgmtime64(ptr) MSVCRT__mkgmtime64
@ cdecl _mktemp(str) MSVCRT__mktemp
@ cdecl _mktemp_s(str long) MSVCRT__mktemp_s
@ cdecl _mktime32(ptr) MSVCRT__mktime32
@ cdecl _mktime64(ptr) MSVCRT__mktime64
@ cdecl _msize(ptr)
@ cdecl _nextafter(double double) MSVCRT__nextafter
@ cdecl -arch=x86_64 _nextafterf(float float) MSVCRT__nextafterf
@ cdecl -arch=i386 _o__CIacos() _CIacos
@ cdecl -arch=i386 _o__CIasin() _CIasin
@ cdecl -arch=i386 _o__CIatan() _CIatan
@ cdecl -arch=i386 _o__CIatan2() _CIatan2
@ cdecl -arch=i386 _o__CIcos() _CIcos
@ cdecl -arch=i386 _o__CIcosh() _CIcosh
@ cdecl -arch=i386 _o__CIexp() _CIexp
@ cdecl -arch=i386 _o__CIfmod() _CIfmod
@ cdecl -arch=i386 _o__CIlog() _CIlog
@ cdecl -arch=i386 _o__CIlog10() _CIlog10
@ cdecl -arch=i386 _o__CIpow() _CIpow
@ cdecl -arch=i386 _o__CIsin() _CIsin
@ cdecl -arch=i386 _o__CIsinh() _CIsinh
@ cdecl -arch=i386 _o__CIsqrt() _CIsqrt
@ cdecl -arch=i386 _o__CItan() _CItan
@ cdecl -arch=i386 _o__CItanh() _CItanh
@ cdecl _o__Getdays() _Getdays
@ cdecl _o__Getmonths() _Getmonths
@ cdecl _o__Gettnames() _Gettnames
@ cdecl _o__Strftime(ptr long str ptr ptr) _Strftime
@ cdecl _o__W_Getdays() _W_Getdays
@ cdecl _o__W_Getmonths() _W_Getmonths
@ cdecl _o__W_Gettnames() _W_Gettnames
@ cdecl _o__Wcsftime(ptr long wstr ptr ptr) _Wcsftime
@ cdecl _o____lc_codepage_func() ___lc_codepage_func
@ cdecl _o____lc_collate_cp_func() ___lc_collate_cp_func
@ cdecl _o____lc_locale_name_func() ___lc_locale_name_func
@ cdecl _o____mb_cur_max_func() MSVCRT____mb_cur_max_func
@ cdecl _o___acrt_iob_func(long) MSVCRT___acrt_iob_func
@ cdecl _o___conio_common_vcprintf(int64 str ptr ptr) MSVCRT__conio_common_vcprintf
@ stub _o___conio_common_vcprintf_p
@ stub _o___conio_common_vcprintf_s
@ stub _o___conio_common_vcscanf
@ cdecl _o___conio_common_vcwprintf(int64 wstr ptr ptr) MSVCRT__conio_common_vcwprintf
@ stub _o___conio_common_vcwprintf_p
@ stub _o___conio_common_vcwprintf_s
@ stub _o___conio_common_vcwscanf
@ cdecl _o___daylight() MSVCRT___p__daylight
@ cdecl _o___dstbias() MSVCRT___p__dstbias
@ cdecl _o___fpe_flt_rounds() __fpe_flt_rounds
@ cdecl -arch=i386 -norelay _o___libm_sse2_acos() MSVCRT___libm_sse2_acos
@ cdecl -arch=i386 -norelay _o___libm_sse2_acosf() MSVCRT___libm_sse2_acosf
@ cdecl -arch=i386 -norelay _o___libm_sse2_asin() MSVCRT___libm_sse2_asin
@ cdecl -arch=i386 -norelay _o___libm_sse2_asinf() MSVCRT___libm_sse2_asinf
@ cdecl -arch=i386 -norelay _o___libm_sse2_atan() MSVCRT___libm_sse2_atan
@ cdecl -arch=i386 -norelay _o___libm_sse2_atan2() MSVCRT___libm_sse2_atan2
@ cdecl -arch=i386 -norelay _o___libm_sse2_atanf() MSVCRT___libm_sse2_atanf
@ cdecl -arch=i386 -norelay _o___libm_sse2_cos() MSVCRT___libm_sse2_cos
@ cdecl -arch=i386 -norelay _o___libm_sse2_cosf() MSVCRT___libm_sse2_cosf
@ cdecl -arch=i386 -norelay _o___libm_sse2_exp() MSVCRT___libm_sse2_exp
@ cdecl -arch=i386 -norelay _o___libm_sse2_expf() MSVCRT___libm_sse2_expf
@ cdecl -arch=i386 -norelay _o___libm_sse2_log() MSVCRT___libm_sse2_log
@ cdecl -arch=i386 -norelay _o___libm_sse2_log10() MSVCRT___libm_sse2_log10
@ cdecl -arch=i386 -norelay _o___libm_sse2_log10f() MSVCRT___libm_sse2_log10f
@ cdecl -arch=i386 -norelay _o___libm_sse2_logf() MSVCRT___libm_sse2_logf
@ cdecl -arch=i386 -norelay _o___libm_sse2_pow() MSVCRT___libm_sse2_pow
@ cdecl -arch=i386 -norelay _o___libm_sse2_powf() MSVCRT___libm_sse2_powf
@ cdecl -arch=i386 -norelay _o___libm_sse2_sin() MSVCRT___libm_sse2_sin
@ cdecl -arch=i386 -norelay _o___libm_sse2_sinf() MSVCRT___libm_sse2_sinf
@ cdecl -arch=i386 -norelay _o___libm_sse2_tan() MSVCRT___libm_sse2_tan
@ cdecl -arch=i386 -norelay _o___libm_sse2_tanf() MSVCRT___libm_sse2_tanf
@ cdecl _o___p___argc() MSVCRT___p___argc
@ cdecl _o___p___argv() MSVCRT___p___argv
@ cdecl _o___p___wargv() MSVCRT___p___wargv
@ cdecl _o___p__acmdln() MSVCRT___p__acmdln
@ cdecl _o___p__commode() __p__commode
@ cdecl _o___p__environ() MSVCRT___p__environ
@ cdecl _o___p__fmode() MSVCRT___p__fmode
@ stub _o___p__mbcasemap
@ cdecl _o___p__mbctype() __p__mbctype
@ cdecl _o___p__pgmptr() MSVCRT___p__pgmptr
@ cdecl _o___p__wcmdln() MSVCRT___p__wcmdln
@ cdecl _o___p__wenviron() MSVCRT___p__wenviron
@ cdecl _o___p__wpgmptr() MSVCRT___p__wpgmptr
@ cdecl _o___pctype_func() MSVCRT___pctype_func
@ stub _o___pwctype_func
@ cdecl _o___std_exception_copy(ptr ptr) MSVCRT___std_exception_copy
@ cdecl _o___std_exception_destroy(ptr) MSVCRT___std_exception_destroy
@ cdecl _o___std_type_info_destroy_list(ptr) MSVCRT_type_info_destroy_list
@ cdecl _o___std_type_info_name(ptr ptr) MSVCRT_type_info_name_list
@ cdecl _o___stdio_common_vfprintf(int64 ptr str ptr ptr) MSVCRT__stdio_common_vfprintf
@ stub _o___stdio_common_vfprintf_p
@ cdecl _o___stdio_common_vfprintf_s(int64 ptr str ptr ptr) MSVCRT__stdio_common_vfprintf_s
@ cdecl _o___stdio_common_vfscanf(int64 ptr str ptr ptr) MSVCRT__stdio_common_vfscanf
@ cdecl _o___stdio_common_vfwprintf(int64 ptr wstr ptr ptr) MSVCRT__stdio_common_vfwprintf
@ stub _o___stdio_common_vfwprintf_p
@ cdecl _o___stdio_common_vfwprintf_s(int64 ptr wstr ptr ptr) MSVCRT__stdio_common_vfwprintf_s
@ cdecl _o___stdio_common_vfwscanf(int64 ptr wstr ptr ptr) MSVCRT__stdio_common_vfwscanf
@ cdecl _o___stdio_common_vsnprintf_s(int64 ptr long long str ptr ptr) MSVCRT__stdio_common_vsnprintf_s
@ cdecl _o___stdio_common_vsnwprintf_s(int64 ptr long long wstr ptr ptr) MSVCRT__stdio_common_vsnwprintf_s
@ cdecl _o___stdio_common_vsprintf(int64 ptr long str ptr ptr) __stdio_common_vsprintf
@ cdecl _o___stdio_common_vsprintf_p(int64 ptr long str ptr ptr) MSVCRT__stdio_common_vsprintf_p
@ cdecl _o___stdio_common_vsprintf_s(int64 ptr long str ptr ptr) MSVCRT__stdio_common_vsprintf_s
@ cdecl _o___stdio_common_vsscanf(int64 ptr long str ptr ptr) MSVCRT__stdio_common_vsscanf
@ cdecl _o___stdio_common_vswprintf(int64 ptr long wstr ptr ptr) MSVCRT__stdio_common_vswprintf
@ cdecl _o___stdio_common_vswprintf_p(int64 ptr long wstr ptr ptr) MSVCRT__stdio_common_vswprintf_p
@ cdecl _o___stdio_common_vswprintf_s(int64 ptr long wstr ptr ptr) MSVCRT__stdio_common_vswprintf_s
@ cdecl _o___stdio_common_vswscanf(int64 ptr long wstr ptr ptr) MSVCRT__stdio_common_vswscanf
@ cdecl _o___timezone() MSVCRT___p__timezone
@ cdecl _o___tzname() __p__tzname
@ cdecl _o___wcserror(wstr) MSVCRT___wcserror
@ cdecl _o__access(str long) MSVCRT__access
@ cdecl _o__access_s(str long) MSVCRT__access_s
@ cdecl _o__aligned_free(ptr) _aligned_free
@ cdecl _o__aligned_malloc(long long) _aligned_malloc
@ cdecl _o__aligned_msize(ptr long long) _aligned_msize
@ cdecl _o__aligned_offset_malloc(long long long) _aligned_offset_malloc
@ cdecl _o__aligned_offset_realloc(ptr long long long) _aligned_offset_realloc
@ stub _o__aligned_offset_recalloc
@ cdecl _o__aligned_realloc(ptr long long) _aligned_realloc
@ stub _o__aligned_recalloc
@ cdecl _o__atodbl(ptr str) MSVCRT__atodbl
@ cdecl _o__atodbl_l(ptr str ptr) MSVCRT__atodbl_l
@ cdecl _o__atof_l(str ptr) MSVCRT__atof_l
@ cdecl _o__atoflt(ptr str) MSVCRT__atoflt
@ cdecl _o__atoflt_l(ptr str ptr) MSVCRT__atoflt_l
@ cdecl -ret64 _o__atoi64(str) MSVCRT__atoi64
@ cdecl -ret64 _o__atoi64_l(str ptr) MSVCRT__atoi64_l
@ cdecl _o__atoi_l(str ptr) MSVCRT__atoi_l
@ cdecl _o__atol_l(str ptr) MSVCRT__atol_l
@ cdecl _o__atoldbl(ptr str) MSVCRT__atoldbl
@ cdecl _o__atoldbl_l(ptr str ptr) MSVCRT__atoldbl_l
@ cdecl -ret64 _o__atoll_l(str ptr) MSVCRT__atoll_l
@ cdecl _o__beep(long long) MSVCRT__beep
@ cdecl _o__beginthread(ptr long ptr) _beginthread
@ cdecl _o__beginthreadex(ptr long ptr ptr long ptr) _beginthreadex
@ cdecl _o__cabs(long) MSVCRT__cabs
@ cdecl _o__callnewh(long) _callnewh
@ cdecl _o__calloc_base(long long) _calloc_base
@ cdecl _o__cexit() MSVCRT__cexit
@ cdecl _o__cgets(ptr) _cgets
@ stub _o__cgets_s
@ stub _o__cgetws
@ stub _o__cgetws_s
@ cdecl _o__chdir(str) MSVCRT__chdir
@ cdecl _o__chdrive(long) MSVCRT__chdrive
@ cdecl _o__chmod(str long) MSVCRT__chmod
@ cdecl _o__chsize(long long) MSVCRT__chsize
@ cdecl _o__chsize_s(long int64) MSVCRT__chsize_s
@ cdecl _o__close(long) MSVCRT__close
@ cdecl _o__commit(long) MSVCRT__commit
@ cdecl _o__configthreadlocale(long) _configthreadlocale
@ cdecl _o__configure_narrow_argv(long) _configure_narrow_argv
@ cdecl _o__configure_wide_argv(long) _configure_wide_argv
@ cdecl _o__controlfp_s(ptr long long) _controlfp_s
@ cdecl _o__cputs(str) _cputs
@ cdecl _o__cputws(wstr) _cputws
@ cdecl _o__creat(str long) MSVCRT__creat
@ cdecl _o__create_locale(long str) MSVCRT__create_locale
@ cdecl _o__crt_atexit(ptr) MSVCRT__crt_atexit
@ cdecl _o__ctime32_s(str long ptr) MSVCRT__ctime32_s
@ cdecl _o__ctime64_s(str long ptr) MSVCRT__ctime64_s
@ cdecl _o__cwait(ptr long long) _cwait
@ stub _o__d_int
@ cdecl _o__dclass(double) MSVCR120__dclass
@ cdecl _o__difftime32(long long) MSVCRT__difftime32
@ cdecl _o__difftime64(int64 int64) MSVCRT__difftime64
@ stub _o__dlog
@ stub _o__dnorm
@ cdecl _o__dpcomp(double double) MSVCR120__dpcomp
@ stub _o__dpoly
@ stub _o__dscale
@ cdecl _o__dsign(double) MSVCR120__dsign
@ stub _o__dsin
@ cdecl _o__dtest(ptr) MSVCR120__dtest
@ stub _o__dunscale
@ cdecl _o__dup(long) MSVCRT__dup
@ cdecl _o__dup2(long long) MSVCRT__dup2
@ cdecl _o__dupenv_s(ptr ptr str) _dupenv_s
@ cdecl _o__ecvt(double long ptr ptr) MSVCRT__ecvt
@ cdecl _o__ecvt_s(str long double long ptr ptr) MSVCRT__ecvt_s
@ cdecl _o__endthread() _endthread
@ cdecl _o__endthreadex(long) _endthreadex
@ cdecl _o__eof(long) MSVCRT__eof
@ cdecl _o__errno() MSVCRT__errno
@ cdecl _o__except1(long long double double long ptr) _except1
@ cdecl _o__execute_onexit_table(ptr) MSVCRT__execute_onexit_table
@ cdecl _o__execv(str ptr) _execv
@ cdecl _o__execve(str ptr ptr) MSVCRT__execve
@ cdecl _o__execvp(str ptr) _execvp
@ cdecl _o__execvpe(str ptr ptr) _execvpe
@ cdecl _o__exit(long) MSVCRT__exit
@ cdecl _o__expand(ptr long) _expand
@ cdecl _o__fclose_nolock(ptr) MSVCRT__fclose_nolock
@ cdecl _o__fcloseall() MSVCRT__fcloseall
@ cdecl _o__fcvt(double long ptr ptr) MSVCRT__fcvt
@ cdecl _o__fcvt_s(ptr long double long ptr ptr) MSVCRT__fcvt_s
@ stub _o__fd_int
@ cdecl _o__fdclass(float) MSVCR120__fdclass
@ stub _o__fdexp
@ stub _o__fdlog
@ cdecl _o__fdopen(long str) MSVCRT__fdopen
@ cdecl _o__fdpcomp(float float) MSVCR120__fdpcomp
@ stub _o__fdpoly
@ stub _o__fdscale
@ cdecl _o__fdsign(float) MSVCR120__fdsign
@ stub _o__fdsin
@ cdecl _o__fflush_nolock(ptr) MSVCRT__fflush_nolock
@ cdecl _o__fgetc_nolock(ptr) MSVCRT__fgetc_nolock
@ cdecl _o__fgetchar() MSVCRT__fgetchar
@ cdecl _o__fgetwc_nolock(ptr) MSVCRT__fgetwc_nolock
@ cdecl _o__fgetwchar() MSVCRT__fgetwchar
@ cdecl _o__filelength(long) MSVCRT__filelength
@ cdecl -ret64 _o__filelengthi64(long) MSVCRT__filelengthi64
@ cdecl _o__fileno(ptr) MSVCRT__fileno
@ cdecl _o__findclose(long) MSVCRT__findclose
@ cdecl _o__findfirst32(str ptr) MSVCRT__findfirst32
@ stub _o__findfirst32i64
@ cdecl _o__findfirst64(str ptr) MSVCRT__findfirst64
@ cdecl _o__findfirst64i32(str ptr) MSVCRT__findfirst64i32
@ cdecl _o__findnext32(long ptr) MSVCRT__findnext32
@ stub _o__findnext32i64
@ cdecl _o__findnext64(long ptr) MSVCRT__findnext64
@ cdecl _o__findnext64i32(long ptr) MSVCRT__findnext64i32
@ cdecl _o__flushall() MSVCRT__flushall
@ cdecl _o__fpclass(double) MSVCRT__fpclass
@ cdecl -arch=!i386 _o__fpclassf(float) MSVCRT__fpclassf
@ cdecl _o__fputc_nolock(long ptr) MSVCRT__fputc_nolock
@ cdecl _o__fputchar(long) MSVCRT__fputchar
@ cdecl _o__fputwc_nolock(long ptr) MSVCRT__fputwc_nolock
@ cdecl _o__fputwchar(long) MSVCRT__fputwchar
@ cdecl _o__fread_nolock(ptr long long ptr) MSVCRT__fread_nolock
@ cdecl _o__fread_nolock_s(ptr long long long ptr) MSVCRT__fread_nolock_s
@ cdecl _o__free_base(ptr) _free_base
@ cdecl _o__free_locale(ptr) MSVCRT__free_locale
@ cdecl _o__fseek_nolock(ptr long long) MSVCRT__fseek_nolock
@ cdecl _o__fseeki64(ptr int64 long) MSVCRT__fseeki64
@ cdecl _o__fseeki64_nolock(ptr int64 long) MSVCRT__fseeki64_nolock
@ cdecl _o__fsopen(str str long) MSVCRT__fsopen
@ cdecl _o__fstat32(long ptr) MSVCRT__fstat32
@ cdecl _o__fstat32i64(long ptr) MSVCRT__fstat32i64
@ cdecl _o__fstat64(long ptr) MSVCRT__fstat64
@ cdecl _o__fstat64i32(long ptr) MSVCRT__fstat64i32
@ cdecl _o__ftell_nolock(ptr) MSVCRT__ftell_nolock
@ cdecl -ret64 _o__ftelli64(ptr) MSVCRT__ftelli64
@ cdecl -ret64 _o__ftelli64_nolock(ptr) MSVCRT__ftelli64_nolock
@ cdecl _o__ftime32(ptr) MSVCRT__ftime32
@ cdecl _o__ftime32_s(ptr) MSVCRT__ftime32_s
@ cdecl _o__ftime64(ptr) MSVCRT__ftime64
@ cdecl _o__ftime64_s(ptr) MSVCRT__ftime64_s
@ cdecl _o__fullpath(ptr str long) MSVCRT__fullpath
@ cdecl _o__futime32(long ptr) _futime32
@ cdecl _o__futime64(long ptr) _futime64
@ cdecl _o__fwrite_nolock(ptr long long ptr) MSVCRT__fwrite_nolock
@ cdecl _o__gcvt(double long str) MSVCRT__gcvt
@ cdecl _o__gcvt_s(ptr long double long) MSVCRT__gcvt_s
@ cdecl _o__get_daylight(ptr) _get_daylight
@ cdecl _o__get_doserrno(ptr) _get_doserrno
@ cdecl _o__get_dstbias(ptr) MSVCRT__get_dstbias
@ cdecl _o__get_errno(ptr) _get_errno
@ cdecl _o__get_fmode(ptr) MSVCRT__get_fmode
@ cdecl _o__get_heap_handle() _get_heap_handle
@ cdecl _o__get_initial_narrow_environment() _get_initial_narrow_environment
@ cdecl _o__get_initial_wide_environment() _get_initial_wide_environment
@ cdecl _o__get_invalid_parameter_handler() _get_invalid_parameter_handler
@ cdecl _o__get_narrow_winmain_command_line() _get_narrow_winmain_command_line
@ cdecl _o__get_osfhandle(long) MSVCRT__get_osfhandle
@ cdecl _o__get_pgmptr(ptr) _get_pgmptr
@ cdecl _o__get_stream_buffer_pointers(ptr ptr ptr ptr) MSVCRT__get_stream_buffer_pointers
@ cdecl _o__get_terminate() MSVCRT__get_terminate
@ cdecl _o__get_thread_local_invalid_parameter_handler() _get_thread_local_invalid_parameter_handler
@ cdecl _o__get_timezone(ptr) _get_timezone
@ cdecl _o__get_tzname(ptr str long long) MSVCRT__get_tzname
@ cdecl _o__get_wide_winmain_command_line() _get_wide_winmain_command_line
@ cdecl _o__get_wpgmptr(ptr) _get_wpgmptr
@ cdecl _o__getc_nolock(ptr) MSVCRT__fgetc_nolock
@ cdecl _o__getch() _getch
@ cdecl _o__getch_nolock() _getch_nolock
@ cdecl _o__getche() _getche
@ cdecl _o__getche_nolock() _getche_nolock
@ cdecl _o__getcwd(str long) MSVCRT__getcwd
@ cdecl _o__getdcwd(long str long) MSVCRT__getdcwd
@ cdecl _o__getdiskfree(long ptr) MSVCRT__getdiskfree
@ cdecl _o__getdllprocaddr(long str long) _getdllprocaddr
@ cdecl _o__getdrive() MSVCRT__getdrive
@ cdecl _o__getdrives() kernel32.GetLogicalDrives
@ cdecl _o__getmbcp() _getmbcp
@ stub _o__getsystime
@ cdecl _o__getw(ptr) MSVCRT__getw
@ cdecl _o__getwc_nolock(ptr) MSVCRT__fgetwc_nolock
@ cdecl _o__getwch() _getwch
@ cdecl _o__getwch_nolock() _getwch_nolock
@ cdecl _o__getwche() _getwche
@ cdecl _o__getwche_nolock() _getwche_nolock
@ cdecl _o__getws(ptr) MSVCRT__getws
@ stub _o__getws_s
@ cdecl _o__gmtime32(ptr) MSVCRT__gmtime32
@ cdecl _o__gmtime32_s(ptr ptr) MSVCRT__gmtime32_s
@ cdecl _o__gmtime64(ptr) MSVCRT__gmtime64
@ cdecl _o__gmtime64_s(ptr ptr) MSVCRT__gmtime64_s
@ cdecl _o__heapchk() _heapchk
@ cdecl _o__heapmin() _heapmin
@ cdecl _o__hypot(double double) _hypot
@ cdecl _o__hypotf(float float) MSVCRT__hypotf
@ cdecl _o__i64toa(int64 ptr long) ntdll._i64toa
@ cdecl _o__i64toa_s(int64 ptr long long) MSVCRT__i64toa_s
@ cdecl _o__i64tow(int64 ptr long) ntdll._i64tow
@ cdecl _o__i64tow_s(int64 ptr long long) MSVCRT__i64tow_s
@ cdecl _o__initialize_narrow_environment() _initialize_narrow_environment
@ cdecl _o__initialize_onexit_table(ptr) MSVCRT__initialize_onexit_table
@ cdecl _o__initialize_wide_environment() _initialize_wide_environment
@ cdecl _o__invalid_parameter_noinfo() _invalid_parameter_noinfo
@ cdecl _o__invalid_parameter_noinfo_noreturn() _invalid_parameter_noinfo_noreturn
@ cdecl _o__isatty(long) MSVCRT__isatty
@ cdecl _o__isctype(long long) MSVCRT__isctype
@ cdecl _o__isctype_l(long long ptr) MSVCRT__isctype_l
@ cdecl _o__isleadbyte_l(long ptr) MSVCRT__isleadbyte_l
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
@ cdecl _o__ismbbkana(long) _ismbbkana
@ cdecl _o__ismbbkana_l(long ptr) _ismbbkana_l
@ stub _o__ismbbkprint
@ stub _o__ismbbkprint_l
@ stub _o__ismbbkpunct
@ stub _o__ismbbkpunct_l
@ cdecl _o__ismbblead(long) _ismbblead
@ cdecl _o__ismbblead_l(long ptr) _ismbblead_l
@ stub _o__ismbbprint
@ stub _o__ismbbprint_l
@ stub _o__ismbbpunct
@ stub _o__ismbbpunct_l
@ cdecl _o__ismbbtrail(long) _ismbbtrail
@ cdecl _o__ismbbtrail_l(long ptr) _ismbbtrail_l
@ cdecl _o__ismbcalnum(long) _ismbcalnum
@ cdecl _o__ismbcalnum_l(long ptr) _ismbcalnum_l
@ cdecl _o__ismbcalpha(long) _ismbcalpha
@ cdecl _o__ismbcalpha_l(long ptr) _ismbcalpha_l
@ stub _o__ismbcblank
@ stub _o__ismbcblank_l
@ cdecl _o__ismbcdigit(long) _ismbcdigit
@ cdecl _o__ismbcdigit_l(long ptr) _ismbcdigit_l
@ cdecl _o__ismbcgraph(long) _ismbcgraph
@ cdecl _o__ismbcgraph_l(long ptr) _ismbcgraph_l
@ cdecl _o__ismbchira(long) _ismbchira
@ stub _o__ismbchira_l
@ cdecl _o__ismbckata(long) _ismbckata
@ stub _o__ismbckata_l
@ cdecl _o__ismbcl0(long) _ismbcl0
@ cdecl _o__ismbcl0_l(long ptr) _ismbcl0_l
@ cdecl _o__ismbcl1(long) _ismbcl1
@ cdecl _o__ismbcl1_l(long ptr) _ismbcl1_l
@ cdecl _o__ismbcl2(long) _ismbcl2
@ cdecl _o__ismbcl2_l(long ptr) _ismbcl2_l
@ cdecl _o__ismbclegal(long) _ismbclegal
@ cdecl _o__ismbclegal_l(long ptr) _ismbclegal_l
@ stub _o__ismbclower
@ cdecl _o__ismbclower_l(long ptr) _ismbclower_l
@ cdecl _o__ismbcprint(long) _ismbcprint
@ cdecl _o__ismbcprint_l(long ptr) _ismbcprint_l
@ cdecl _o__ismbcpunct(long) _ismbcpunct
@ cdecl _o__ismbcpunct_l(long ptr) _ismbcpunct_l
@ cdecl _o__ismbcspace(long) _ismbcspace
@ cdecl _o__ismbcspace_l(long ptr) _ismbcspace_l
@ cdecl _o__ismbcsymbol(long) _ismbcsymbol
@ stub _o__ismbcsymbol_l
@ cdecl _o__ismbcupper(long) _ismbcupper
@ cdecl _o__ismbcupper_l(long ptr) _ismbcupper_l
@ cdecl _o__ismbslead(ptr ptr) _ismbslead
@ stub _o__ismbslead_l
@ cdecl _o__ismbstrail(ptr ptr) _ismbstrail
@ stub _o__ismbstrail_l
@ cdecl _o__iswctype_l(long long ptr) MSVCRT__iswctype_l
@ cdecl _o__itoa(long ptr long) MSVCRT__itoa
@ cdecl _o__itoa_s(long ptr long long) MSVCRT__itoa_s
@ cdecl _o__itow(long ptr long) ntdll._itow
@ cdecl _o__itow_s(long ptr long long) MSVCRT__itow_s
@ cdecl _o__j0(double) MSVCRT__j0
@ cdecl _o__j1(double) MSVCRT__j1
@ cdecl _o__jn(long double) MSVCRT__jn
@ cdecl _o__kbhit() _kbhit
@ stub _o__ld_int
@ cdecl _o__ldclass(double) MSVCR120__ldclass
@ stub _o__ldexp
@ stub _o__ldlog
@ cdecl _o__ldpcomp(double double) MSVCR120__dpcomp
@ stub _o__ldpoly
@ stub _o__ldscale
@ cdecl _o__ldsign(double) MSVCR120__dsign
@ stub _o__ldsin
@ cdecl _o__ldtest(ptr) MSVCR120__ldtest
@ stub _o__ldunscale
@ cdecl _o__lfind(ptr ptr ptr long ptr) _lfind
@ cdecl _o__lfind_s(ptr ptr ptr long ptr ptr) _lfind_s
@ cdecl -arch=i386 -norelay _o__libm_sse2_acos_precise() MSVCRT___libm_sse2_acos
@ cdecl -arch=i386 -norelay _o__libm_sse2_asin_precise() MSVCRT___libm_sse2_asin
@ cdecl -arch=i386 -norelay _o__libm_sse2_atan_precise() MSVCRT___libm_sse2_atan
@ cdecl -arch=i386 -norelay _o__libm_sse2_cos_precise() MSVCRT___libm_sse2_cos
@ cdecl -arch=i386 -norelay _o__libm_sse2_exp_precise() MSVCRT___libm_sse2_exp
@ cdecl -arch=i386 -norelay _o__libm_sse2_log10_precise() MSVCRT___libm_sse2_log10
@ cdecl -arch=i386 -norelay _o__libm_sse2_log_precise() MSVCRT___libm_sse2_log
@ cdecl -arch=i386 -norelay _o__libm_sse2_pow_precise() MSVCRT___libm_sse2_pow
@ cdecl -arch=i386 -norelay _o__libm_sse2_sin_precise() MSVCRT___libm_sse2_sin
@ cdecl -arch=i386 -norelay _o__libm_sse2_sqrt_precise() MSVCRT___libm_sse2_sqrt_precise
@ cdecl -arch=i386 -norelay _o__libm_sse2_tan_precise() MSVCRT___libm_sse2_tan
@ cdecl _o__loaddll(str) _loaddll
@ cdecl _o__localtime32(ptr) MSVCRT__localtime32
@ cdecl _o__localtime32_s(ptr ptr) _localtime32_s
@ cdecl _o__localtime64(ptr) MSVCRT__localtime64
@ cdecl _o__localtime64_s(ptr ptr) _localtime64_s
@ cdecl _o__lock_file(ptr) MSVCRT__lock_file
@ cdecl _o__locking(long long long) MSVCRT__locking
@ cdecl _o__logb(double) MSVCRT__logb
@ cdecl -arch=!i386 _o__logbf(float) MSVCRT__logbf
@ cdecl _o__lsearch(ptr ptr ptr long ptr) _lsearch
@ stub _o__lsearch_s
@ cdecl _o__lseek(long long long) MSVCRT__lseek
@ cdecl -ret64 _o__lseeki64(long int64 long) MSVCRT__lseeki64
@ cdecl _o__ltoa(long ptr long) ntdll._ltoa
@ cdecl _o__ltoa_s(long ptr long long) MSVCRT__ltoa_s
@ cdecl _o__ltow(long ptr long) ntdll._ltow
@ cdecl _o__ltow_s(long ptr long long) MSVCRT__ltow_s
@ cdecl _o__makepath(ptr str str str str) MSVCRT__makepath
@ cdecl _o__makepath_s(ptr long str str str str) MSVCRT__makepath_s
@ cdecl _o__malloc_base(long) _malloc_base
@ cdecl _o__mbbtombc(long) _mbbtombc
@ stub _o__mbbtombc_l
@ cdecl _o__mbbtype(long long) _mbbtype
@ cdecl _o__mbbtype_l(long long ptr) _mbbtype_l
@ cdecl _o__mbccpy(ptr ptr) _mbccpy
@ cdecl _o__mbccpy_l(ptr ptr ptr) _mbccpy_l
@ cdecl _o__mbccpy_s(ptr long ptr ptr) _mbccpy_s
@ cdecl _o__mbccpy_s_l(ptr long ptr ptr ptr) _mbccpy_s_l
@ cdecl _o__mbcjistojms(long) _mbcjistojms
@ stub _o__mbcjistojms_l
@ cdecl _o__mbcjmstojis(long) _mbcjmstojis
@ stub _o__mbcjmstojis_l
@ cdecl _o__mbclen(ptr) _mbclen
@ stub _o__mbclen_l
@ cdecl _o__mbctohira(long) _mbctohira
@ stub _o__mbctohira_l
@ cdecl _o__mbctokata(long) _mbctokata
@ stub _o__mbctokata_l
@ cdecl _o__mbctolower(long) _mbctolower
@ stub _o__mbctolower_l
@ cdecl _o__mbctombb(long) _mbctombb
@ stub _o__mbctombb_l
@ cdecl _o__mbctoupper(long) _mbctoupper
@ stub _o__mbctoupper_l
@ stub _o__mblen_l
@ cdecl _o__mbsbtype(str long) _mbsbtype
@ stub _o__mbsbtype_l
@ cdecl _o__mbscat_s(ptr long str) _mbscat_s
@ cdecl _o__mbscat_s_l(ptr long str ptr) _mbscat_s_l
@ cdecl _o__mbschr(str long) _mbschr
@ stub _o__mbschr_l
@ cdecl _o__mbscmp(str str) _mbscmp
@ cdecl _o__mbscmp_l(str str ptr) _mbscmp_l
@ cdecl _o__mbscoll(str str) _mbscoll
@ cdecl _o__mbscoll_l(str str ptr) _mbscoll_l
@ cdecl _o__mbscpy_s(ptr long str) _mbscpy_s
@ cdecl _o__mbscpy_s_l(ptr long str ptr) _mbscpy_s_l
@ cdecl _o__mbscspn(str str) _mbscspn
@ cdecl _o__mbscspn_l(str str ptr) _mbscspn_l
@ cdecl _o__mbsdec(ptr ptr) _mbsdec
@ stub _o__mbsdec_l
@ cdecl _o__mbsicmp(str str) _mbsicmp
@ stub _o__mbsicmp_l
@ cdecl _o__mbsicoll(str str) _mbsicoll
@ cdecl _o__mbsicoll_l(str str ptr) _mbsicoll_l
@ cdecl _o__mbsinc(str) _mbsinc
@ stub _o__mbsinc_l
@ cdecl _o__mbslen(str) _mbslen
@ cdecl _o__mbslen_l(str ptr) _mbslen_l
@ cdecl _o__mbslwr(str) _mbslwr
@ stub _o__mbslwr_l
@ cdecl _o__mbslwr_s(str long) _mbslwr_s
@ stub _o__mbslwr_s_l
@ cdecl _o__mbsnbcat(str str long) _mbsnbcat
@ stub _o__mbsnbcat_l
@ cdecl _o__mbsnbcat_s(str long ptr long) _mbsnbcat_s
@ stub _o__mbsnbcat_s_l
@ cdecl _o__mbsnbcmp(str str long) _mbsnbcmp
@ stub _o__mbsnbcmp_l
@ cdecl _o__mbsnbcnt(ptr long) _mbsnbcnt
@ stub _o__mbsnbcnt_l
@ cdecl _o__mbsnbcoll(str str long) _mbsnbcoll
@ cdecl _o__mbsnbcoll_l(str str long ptr) _mbsnbcoll_l
@ cdecl _o__mbsnbcpy(ptr str long) _mbsnbcpy
@ stub _o__mbsnbcpy_l
@ cdecl _o__mbsnbcpy_s(ptr long str long) _mbsnbcpy_s
@ cdecl _o__mbsnbcpy_s_l(ptr long str long ptr) _mbsnbcpy_s_l
@ cdecl _o__mbsnbicmp(str str long) _mbsnbicmp
@ stub _o__mbsnbicmp_l
@ cdecl _o__mbsnbicoll(str str long) _mbsnbicoll
@ cdecl _o__mbsnbicoll_l(str str long ptr) _mbsnbicoll_l
@ cdecl _o__mbsnbset(ptr long long) _mbsnbset
@ stub _o__mbsnbset_l
@ stub _o__mbsnbset_s
@ stub _o__mbsnbset_s_l
@ cdecl _o__mbsncat(str str long) _mbsncat
@ stub _o__mbsncat_l
@ stub _o__mbsncat_s
@ stub _o__mbsncat_s_l
@ cdecl _o__mbsnccnt(str long) _mbsnccnt
@ stub _o__mbsnccnt_l
@ cdecl _o__mbsncmp(str str long) _mbsncmp
@ stub _o__mbsncmp_l
@ stub _o__mbsncoll
@ stub _o__mbsncoll_l
@ cdecl _o__mbsncpy(ptr str long) _mbsncpy
@ stub _o__mbsncpy_l
@ stub _o__mbsncpy_s
@ stub _o__mbsncpy_s_l
@ cdecl _o__mbsnextc(str) _mbsnextc
@ cdecl _o__mbsnextc_l(str ptr) _mbsnextc_l
@ cdecl _o__mbsnicmp(str str long) _mbsnicmp
@ stub _o__mbsnicmp_l
@ stub _o__mbsnicoll
@ stub _o__mbsnicoll_l
@ cdecl _o__mbsninc(str long) _mbsninc
@ stub _o__mbsninc_l
@ cdecl _o__mbsnlen(str long) _mbsnlen
@ cdecl _o__mbsnlen_l(str long ptr) _mbsnlen_l
@ cdecl _o__mbsnset(ptr long long) _mbsnset
@ stub _o__mbsnset_l
@ stub _o__mbsnset_s
@ stub _o__mbsnset_s_l
@ cdecl _o__mbspbrk(str str) _mbspbrk
@ stub _o__mbspbrk_l
@ cdecl _o__mbsrchr(str long) _mbsrchr
@ stub _o__mbsrchr_l
@ cdecl _o__mbsrev(str) _mbsrev
@ stub _o__mbsrev_l
@ cdecl _o__mbsset(ptr long) _mbsset
@ stub _o__mbsset_l
@ stub _o__mbsset_s
@ stub _o__mbsset_s_l
@ cdecl _o__mbsspn(str str) _mbsspn
@ cdecl _o__mbsspn_l(str str ptr) _mbsspn_l
@ cdecl _o__mbsspnp(str str) _mbsspnp
@ stub _o__mbsspnp_l
@ cdecl _o__mbsstr(str str) _mbsstr
@ stub _o__mbsstr_l
@ cdecl _o__mbstok(str str) _mbstok
@ cdecl _o__mbstok_l(str str ptr) _mbstok_l
@ cdecl _o__mbstok_s(str str ptr) _mbstok_s
@ cdecl _o__mbstok_s_l(str str ptr ptr) _mbstok_s_l
@ cdecl _o__mbstowcs_l(ptr str long ptr) MSVCRT__mbstowcs_l
@ cdecl _o__mbstowcs_s_l(ptr ptr long str long ptr) MSVCRT__mbstowcs_s_l
@ cdecl _o__mbstrlen(str) _mbstrlen
@ cdecl _o__mbstrlen_l(str ptr) _mbstrlen_l
@ stub _o__mbstrnlen
@ stub _o__mbstrnlen_l
@ cdecl _o__mbsupr(str) _mbsupr
@ stub _o__mbsupr_l
@ cdecl _o__mbsupr_s(str long) _mbsupr_s
@ stub _o__mbsupr_s_l
@ cdecl _o__mbtowc_l(ptr str long ptr) MSVCRT_mbtowc_l
@ cdecl _o__memicmp(str str long) MSVCRT__memicmp
@ cdecl _o__memicmp_l(str str long ptr) MSVCRT__memicmp_l
@ cdecl _o__mkdir(str) MSVCRT__mkdir
@ cdecl _o__mkgmtime32(ptr) MSVCRT__mkgmtime32
@ cdecl _o__mkgmtime64(ptr) MSVCRT__mkgmtime64
@ cdecl _o__mktemp(str) MSVCRT__mktemp
@ cdecl _o__mktemp_s(str long) MSVCRT__mktemp_s
@ cdecl _o__mktime32(ptr) MSVCRT__mktime32
@ cdecl _o__mktime64(ptr) MSVCRT__mktime64
@ cdecl _o__msize(ptr) _msize
@ cdecl _o__nextafter(double double) MSVCRT__nextafter
@ cdecl -arch=x86_64 _o__nextafterf(float float) MSVCRT__nextafterf
@ cdecl _o__open_osfhandle(long long) MSVCRT__open_osfhandle
@ cdecl _o__pclose(ptr) MSVCRT__pclose
@ cdecl _o__pipe(ptr long long) MSVCRT__pipe
@ cdecl _o__popen(str str) MSVCRT__popen
@ cdecl _o__purecall() _purecall
@ cdecl _o__putc_nolock(long ptr) MSVCRT__fputc_nolock
@ cdecl _o__putch(long) _putch
@ cdecl _o__putch_nolock(long) _putch_nolock
@ cdecl _o__putenv(str) _putenv
@ cdecl _o__putenv_s(str str) _putenv_s
@ cdecl _o__putw(long ptr) MSVCRT__putw
@ cdecl _o__putwc_nolock(long ptr) MSVCRT__fputwc_nolock
@ cdecl _o__putwch(long) _putwch
@ cdecl _o__putwch_nolock(long) _putwch_nolock
@ cdecl _o__putws(wstr) MSVCRT__putws
@ cdecl _o__read(long ptr long) MSVCRT__read
@ cdecl _o__realloc_base(ptr long) _realloc_base
@ cdecl _o__recalloc(ptr long long) _recalloc
@ cdecl _o__register_onexit_function(ptr ptr) MSVCRT__register_onexit_function
@ cdecl _o__resetstkoflw() MSVCRT__resetstkoflw
@ cdecl _o__rmdir(str) MSVCRT__rmdir
@ cdecl _o__rmtmp() MSVCRT__rmtmp
@ cdecl _o__scalb(double long) MSVCRT__scalb
@ cdecl -arch=x86_64 _o__scalbf(float long) MSVCRT__scalbf
@ cdecl _o__searchenv(str str ptr) MSVCRT__searchenv
@ cdecl _o__searchenv_s(str str ptr long) MSVCRT__searchenv_s
@ cdecl _o__seh_filter_dll(long ptr) __CppXcptFilter
@ cdecl _o__seh_filter_exe(long ptr) _XcptFilter
@ cdecl _o__set_abort_behavior(long long) MSVCRT__set_abort_behavior
@ cdecl _o__set_app_type(long) MSVCRT___set_app_type
@ cdecl _o__set_doserrno(long) _set_doserrno
@ cdecl _o__set_errno(long) _set_errno
@ cdecl _o__set_fmode(long) MSVCRT__set_fmode
@ cdecl _o__set_invalid_parameter_handler(ptr) _set_invalid_parameter_handler
@ cdecl _o__set_new_handler(ptr) MSVCRT_set_new_handler
@ cdecl _o__set_new_mode(long) MSVCRT__set_new_mode
@ cdecl _o__set_thread_local_invalid_parameter_handler(ptr) _set_thread_local_invalid_parameter_handler
@ cdecl _o__seterrormode(long) _seterrormode
@ cdecl _o__setmbcp(long) _setmbcp
@ cdecl _o__setmode(long long) MSVCRT__setmode
@ stub _o__setsystime
@ cdecl _o__sleep(long) MSVCRT__sleep
@ varargs _o__sopen(str long long) MSVCRT__sopen
@ cdecl _o__sopen_dispatch(str long long long ptr long) MSVCRT__sopen_dispatch
@ cdecl _o__sopen_s(ptr str long long long) MSVCRT__sopen_s
@ cdecl _o__spawnv(long str ptr) MSVCRT__spawnv
@ cdecl _o__spawnve(long str ptr ptr) MSVCRT__spawnve
@ cdecl _o__spawnvp(long str ptr) MSVCRT__spawnvp
@ cdecl _o__spawnvpe(long str ptr ptr) MSVCRT__spawnvpe
@ cdecl _o__splitpath(str ptr ptr ptr ptr) MSVCRT__splitpath
@ cdecl _o__splitpath_s(str ptr long ptr long ptr long ptr long) MSVCRT__splitpath_s
@ cdecl _o__stat32(str ptr) MSVCRT__stat32
@ cdecl _o__stat32i64(str ptr) MSVCRT__stat32i64
@ cdecl _o__stat64(str ptr) MSVCRT_stat64
@ cdecl _o__stat64i32(str ptr) MSVCRT__stat64i32
@ cdecl _o__strcoll_l(str str ptr) MSVCRT_strcoll_l
@ cdecl _o__strdate(ptr) MSVCRT__strdate
@ cdecl _o__strdate_s(ptr long) _strdate_s
@ cdecl _o__strdup(str) MSVCRT__strdup
@ cdecl _o__strerror(long) MSVCRT__strerror
@ stub _o__strerror_s
@ cdecl _o__strftime_l(ptr long str ptr ptr) MSVCRT__strftime_l
@ cdecl _o__stricmp(str str) MSVCRT__stricmp
@ cdecl _o__stricmp_l(str str ptr) MSVCRT__stricmp_l
@ cdecl _o__stricoll(str str) MSVCRT__stricoll
@ cdecl _o__stricoll_l(str str ptr) MSVCRT__stricoll_l
@ cdecl _o__strlwr(str) MSVCRT__strlwr
@ cdecl _o__strlwr_l(str ptr) _strlwr_l
@ cdecl _o__strlwr_s(ptr long) MSVCRT__strlwr_s
@ cdecl _o__strlwr_s_l(ptr long ptr) MSVCRT__strlwr_s_l
@ cdecl _o__strncoll(str str long) MSVCRT__strncoll
@ cdecl _o__strncoll_l(str str long ptr) MSVCRT__strncoll_l
@ cdecl _o__strnicmp(str str long) MSVCRT__strnicmp
@ cdecl _o__strnicmp_l(str str long ptr) MSVCRT__strnicmp_l
@ cdecl _o__strnicoll(str str long) MSVCRT__strnicoll
@ cdecl _o__strnicoll_l(str str long ptr) MSVCRT__strnicoll_l
@ cdecl _o__strnset_s(str long long long) MSVCRT__strnset_s
@ stub _o__strset_s
@ cdecl _o__strtime(ptr) MSVCRT__strtime
@ cdecl _o__strtime_s(ptr long) _strtime_s
@ cdecl _o__strtod_l(str ptr ptr) MSVCRT_strtod_l
@ cdecl _o__strtof_l(str ptr ptr) MSVCRT__strtof_l
@ cdecl -ret64 _o__strtoi64(str ptr long) MSVCRT_strtoi64
@ cdecl -ret64 _o__strtoi64_l(str ptr long ptr) MSVCRT_strtoi64_l
@ cdecl _o__strtol_l(str ptr long ptr) MSVCRT__strtol_l
@ cdecl _o__strtold_l(str ptr ptr) MSVCRT_strtod_l
@ cdecl -ret64 _o__strtoll_l(str ptr long ptr) MSVCRT_strtoi64_l
@ cdecl -ret64 _o__strtoui64(str ptr long) MSVCRT_strtoui64
@ cdecl -ret64 _o__strtoui64_l(str ptr long ptr) MSVCRT_strtoui64_l
@ cdecl _o__strtoul_l(str ptr long ptr) MSVCRT_strtoul_l
@ cdecl -ret64 _o__strtoull_l(str ptr long ptr) MSVCRT_strtoui64_l
@ cdecl _o__strupr(str) MSVCRT__strupr
@ cdecl _o__strupr_l(str ptr) MSVCRT__strupr_l
@ cdecl _o__strupr_s(str long) MSVCRT__strupr_s
@ cdecl _o__strupr_s_l(str long ptr) MSVCRT__strupr_s_l
@ cdecl _o__strxfrm_l(ptr str long ptr) MSVCRT__strxfrm_l
@ cdecl _o__swab(str str long) MSVCRT__swab
@ cdecl _o__tell(long) MSVCRT__tell
@ cdecl -ret64 _o__telli64(long) _telli64
@ cdecl _o__timespec32_get(ptr long) _timespec32_get
@ cdecl _o__timespec64_get(ptr long) _timespec64_get
@ cdecl _o__tolower(long) MSVCRT__tolower
@ cdecl _o__tolower_l(long ptr) MSVCRT__tolower_l
@ cdecl _o__toupper(long) MSVCRT__toupper
@ cdecl _o__toupper_l(long ptr) MSVCRT__toupper_l
@ cdecl _o__towlower_l(long ptr) MSVCRT__towlower_l
@ cdecl _o__towupper_l(long ptr) MSVCRT__towupper_l
@ cdecl _o__tzset() MSVCRT__tzset
@ cdecl _o__ui64toa(int64 ptr long) ntdll._ui64toa
@ cdecl _o__ui64toa_s(int64 ptr long long) MSVCRT__ui64toa_s
@ cdecl _o__ui64tow(int64 ptr long) ntdll._ui64tow
@ cdecl _o__ui64tow_s(int64 ptr long long) MSVCRT__ui64tow_s
@ cdecl _o__ultoa(long ptr long) ntdll._ultoa
@ cdecl _o__ultoa_s(long ptr long long) MSVCRT__ultoa_s
@ cdecl _o__ultow(long ptr long) ntdll._ultow
@ cdecl _o__ultow_s(long ptr long long) MSVCRT__ultow_s
@ cdecl _o__umask(long) MSVCRT__umask
@ stub _o__umask_s
@ cdecl _o__ungetc_nolock(long ptr) MSVCRT__ungetc_nolock
@ cdecl _o__ungetch(long) _ungetch
@ cdecl _o__ungetch_nolock(long) _ungetch_nolock
@ cdecl _o__ungetwc_nolock(long ptr) MSVCRT__ungetwc_nolock
@ cdecl _o__ungetwch(long) _ungetwch
@ cdecl _o__ungetwch_nolock(long) _ungetwch_nolock
@ cdecl _o__unlink(str) MSVCRT__unlink
@ cdecl _o__unloaddll(long) _unloaddll
@ cdecl _o__unlock_file(ptr) MSVCRT__unlock_file
@ cdecl _o__utime32(str ptr) _utime32
@ cdecl _o__utime64(str ptr) _utime64
@ cdecl _o__waccess(wstr long) MSVCRT__waccess
@ cdecl _o__waccess_s(wstr long) MSVCRT__waccess_s
@ cdecl _o__wasctime(ptr) MSVCRT__wasctime
@ cdecl _o__wasctime_s(ptr long ptr) MSVCRT__wasctime_s
@ cdecl _o__wchdir(wstr) MSVCRT__wchdir
@ cdecl _o__wchmod(wstr long) MSVCRT__wchmod
@ cdecl _o__wcreat(wstr long) MSVCRT__wcreat
@ cdecl _o__wcreate_locale(long wstr) MSVCRT__wcreate_locale
@ cdecl _o__wcscoll_l(wstr wstr ptr) MSVCRT__wcscoll_l
@ cdecl _o__wcsdup(wstr) MSVCRT__wcsdup
@ cdecl _o__wcserror(long) MSVCRT__wcserror
@ cdecl _o__wcserror_s(ptr long long) MSVCRT__wcserror_s
@ cdecl _o__wcsftime_l(ptr long wstr ptr ptr) MSVCRT__wcsftime_l
@ cdecl _o__wcsicmp(wstr wstr) MSVCRT__wcsicmp
@ cdecl _o__wcsicmp_l(wstr wstr ptr) MSVCRT__wcsicmp_l
@ cdecl _o__wcsicoll(wstr wstr) MSVCRT__wcsicoll
@ cdecl _o__wcsicoll_l(wstr wstr ptr) MSVCRT__wcsicoll_l
@ cdecl _o__wcslwr(wstr) MSVCRT__wcslwr
@ cdecl _o__wcslwr_l(wstr ptr) MSVCRT__wcslwr_l
@ cdecl _o__wcslwr_s(wstr long) MSVCRT__wcslwr_s
@ cdecl _o__wcslwr_s_l(wstr long ptr) MSVCRT__wcslwr_s_l
@ cdecl _o__wcsncoll(wstr wstr long) MSVCRT__wcsncoll
@ cdecl _o__wcsncoll_l(wstr wstr long ptr) MSVCRT__wcsncoll_l
@ cdecl _o__wcsnicmp(wstr wstr long) MSVCRT__wcsnicmp
@ cdecl _o__wcsnicmp_l(wstr wstr long ptr) MSVCRT__wcsnicmp_l
@ cdecl _o__wcsnicoll(wstr wstr long) MSVCRT__wcsnicoll
@ cdecl _o__wcsnicoll_l(wstr wstr long ptr) MSVCRT__wcsnicoll_l
@ cdecl _o__wcsnset(wstr long long) MSVCRT__wcsnset
@ cdecl _o__wcsnset_s(wstr long long long) MSVCRT__wcsnset_s
@ cdecl _o__wcsset(wstr long) MSVCRT__wcsset
@ cdecl _o__wcsset_s(wstr long long) MSVCRT__wcsset_s
@ cdecl _o__wcstod_l(wstr ptr ptr) MSVCRT__wcstod_l
@ cdecl _o__wcstof_l(wstr ptr ptr) MSVCRT__wcstof_l
@ cdecl -ret64 _o__wcstoi64(wstr ptr long) MSVCRT__wcstoi64
@ cdecl -ret64 _o__wcstoi64_l(wstr ptr long ptr) MSVCRT__wcstoi64_l
@ cdecl _o__wcstol_l(wstr ptr long ptr) MSVCRT__wcstol_l
@ cdecl _o__wcstold_l(wstr ptr ptr) MSVCRT__wcstod_l
@ cdecl -ret64 _o__wcstoll_l(wstr ptr long ptr) MSVCRT__wcstoi64_l
@ cdecl _o__wcstombs_l(ptr ptr long ptr) MSVCRT__wcstombs_l
@ cdecl _o__wcstombs_s_l(ptr ptr long wstr long ptr) MSVCRT__wcstombs_s_l
@ cdecl -ret64 _o__wcstoui64(wstr ptr long) MSVCRT__wcstoui64
@ cdecl -ret64 _o__wcstoui64_l(wstr ptr long ptr) MSVCRT__wcstoui64_l
@ cdecl _o__wcstoul_l(wstr ptr long ptr) MSVCRT__wcstoul_l
@ cdecl -ret64 _o__wcstoull_l(wstr ptr long ptr) MSVCRT__wcstoui64_l
@ cdecl _o__wcsupr(wstr) MSVCRT__wcsupr
@ cdecl _o__wcsupr_l(wstr ptr) MSVCRT__wcsupr_l
@ cdecl _o__wcsupr_s(wstr long) MSVCRT__wcsupr_s
@ cdecl _o__wcsupr_s_l(wstr long ptr) MSVCRT__wcsupr_s_l
@ cdecl _o__wcsxfrm_l(ptr wstr long ptr) MSVCRT__wcsxfrm_l
@ cdecl _o__wctime32(ptr) MSVCRT__wctime32
@ cdecl _o__wctime32_s(ptr long ptr) MSVCRT__wctime32_s
@ cdecl _o__wctime64(ptr) MSVCRT__wctime64
@ cdecl _o__wctime64_s(ptr long ptr) MSVCRT__wctime64_s
@ cdecl _o__wctomb_l(ptr long ptr) MSVCRT__wctomb_l
@ cdecl _o__wctomb_s_l(ptr ptr long long ptr) MSVCRT__wctomb_s_l
@ cdecl _o__wdupenv_s(ptr ptr wstr) _wdupenv_s
@ cdecl _o__wexecv(wstr ptr) _wexecv
@ cdecl _o__wexecve(wstr ptr ptr) _wexecve
@ cdecl _o__wexecvp(wstr ptr) _wexecvp
@ cdecl _o__wexecvpe(wstr ptr ptr) _wexecvpe
@ cdecl _o__wfdopen(long wstr) MSVCRT__wfdopen
@ cdecl _o__wfindfirst32(wstr ptr) MSVCRT__wfindfirst32
@ stub _o__wfindfirst32i64
@ cdecl _o__wfindfirst64(wstr ptr) MSVCRT__wfindfirst64
@ cdecl _o__wfindfirst64i32(wstr ptr) MSVCRT__wfindfirst64i32
@ cdecl _o__wfindnext32(long ptr) MSVCRT__wfindnext32
@ stub _o__wfindnext32i64
@ cdecl _o__wfindnext64(long ptr) MSVCRT__wfindnext64
@ cdecl _o__wfindnext64i32(long ptr) MSVCRT__wfindnext64i32
@ cdecl _o__wfopen(wstr wstr) MSVCRT__wfopen
@ cdecl _o__wfopen_s(ptr wstr wstr) MSVCRT__wfopen_s
@ cdecl _o__wfreopen(wstr wstr ptr) MSVCRT__wfreopen
@ cdecl _o__wfreopen_s(ptr wstr wstr ptr) MSVCRT__wfreopen_s
@ cdecl _o__wfsopen(wstr wstr long) MSVCRT__wfsopen
@ cdecl _o__wfullpath(ptr wstr long) MSVCRT__wfullpath
@ cdecl _o__wgetcwd(wstr long) MSVCRT__wgetcwd
@ cdecl _o__wgetdcwd(long wstr long) MSVCRT__wgetdcwd
@ cdecl _o__wgetenv(wstr) MSVCRT__wgetenv
@ cdecl _o__wgetenv_s(ptr ptr long wstr) _wgetenv_s
@ cdecl _o__wmakepath(ptr wstr wstr wstr wstr) MSVCRT__wmakepath
@ cdecl _o__wmakepath_s(ptr long wstr wstr wstr wstr) MSVCRT__wmakepath_s
@ cdecl _o__wmkdir(wstr) MSVCRT__wmkdir
@ cdecl _o__wmktemp(wstr) MSVCRT__wmktemp
@ cdecl _o__wmktemp_s(wstr long) MSVCRT__wmktemp_s
@ cdecl _o__wperror(wstr) MSVCRT__wperror
@ cdecl _o__wpopen(wstr wstr) MSVCRT__wpopen
@ cdecl _o__wputenv(wstr) _wputenv
@ cdecl _o__wputenv_s(wstr wstr) _wputenv_s
@ cdecl _o__wremove(wstr) MSVCRT__wremove
@ cdecl _o__wrename(wstr wstr) MSVCRT__wrename
@ cdecl _o__write(long ptr long) MSVCRT__write
@ cdecl _o__wrmdir(wstr) MSVCRT__wrmdir
@ cdecl _o__wsearchenv(wstr wstr ptr) MSVCRT__wsearchenv
@ cdecl _o__wsearchenv_s(wstr wstr ptr long) MSVCRT__wsearchenv_s
@ cdecl _o__wsetlocale(long wstr) MSVCRT__wsetlocale
@ cdecl _o__wsopen_dispatch(wstr long long long ptr long) MSVCRT__wsopen_dispatch
@ cdecl _o__wsopen_s(ptr wstr long long long) MSVCRT__wsopen_s
@ cdecl _o__wspawnv(long wstr ptr) MSVCRT__wspawnv
@ cdecl _o__wspawnve(long wstr ptr ptr) MSVCRT__wspawnve
@ cdecl _o__wspawnvp(long wstr ptr) MSVCRT__wspawnvp
@ cdecl _o__wspawnvpe(long wstr ptr ptr) MSVCRT__wspawnvpe
@ cdecl _o__wsplitpath(wstr ptr ptr ptr ptr) MSVCRT__wsplitpath
@ cdecl _o__wsplitpath_s(wstr ptr long ptr long ptr long ptr long) MSVCRT__wsplitpath_s
@ cdecl _o__wstat32(wstr ptr) MSVCRT__wstat32
@ cdecl _o__wstat32i64(wstr ptr) MSVCRT__wstat32i64
@ cdecl _o__wstat64(wstr ptr) MSVCRT__wstat64
@ cdecl _o__wstat64i32(wstr ptr) MSVCRT__wstat64i32
@ cdecl _o__wstrdate(ptr) MSVCRT__wstrdate
@ cdecl _o__wstrdate_s(ptr long) _wstrdate_s
@ cdecl _o__wstrtime(ptr) MSVCRT__wstrtime
@ cdecl _o__wstrtime_s(ptr long) _wstrtime_s
@ cdecl _o__wsystem(wstr) _wsystem
@ cdecl _o__wtmpnam_s(ptr long) MSVCRT__wtmpnam_s
@ cdecl _o__wtof(wstr) MSVCRT__wtof
@ cdecl _o__wtof_l(wstr ptr) MSVCRT__wtof_l
@ cdecl _o__wtoi(wstr) MSVCRT__wtoi
@ cdecl -ret64 _o__wtoi64(wstr) MSVCRT__wtoi64
@ cdecl -ret64 _o__wtoi64_l(wstr ptr) MSVCRT__wtoi64_l
@ cdecl _o__wtoi_l(wstr ptr) MSVCRT__wtoi_l
@ cdecl _o__wtol(wstr) MSVCRT__wtol
@ cdecl _o__wtol_l(wstr ptr) MSVCRT__wtol_l
@ cdecl -ret64 _o__wtoll(wstr) MSVCRT__wtoll
@ cdecl -ret64 _o__wtoll_l(wstr ptr) MSVCRT__wtoll_l
@ cdecl _o__wunlink(wstr) MSVCRT__wunlink
@ cdecl _o__wutime32(wstr ptr) _wutime32
@ cdecl _o__wutime64(wstr ptr) _wutime64
@ cdecl _o__y0(double) MSVCRT__y0
@ cdecl _o__y1(double) MSVCRT__y1
@ cdecl _o__yn(long double) MSVCRT__yn
@ cdecl _o_abort() MSVCRT_abort
@ cdecl _o_acos(double) MSVCRT_acos
@ cdecl -arch=!i386 _o_acosf(float) MSVCRT_acosf
@ cdecl _o_acosh(double) MSVCR120_acosh
@ cdecl _o_acoshf(float) MSVCR120_acoshf
@ cdecl _o_acoshl(double) MSVCR120_acoshl
@ cdecl _o_asctime(ptr) MSVCRT_asctime
@ cdecl _o_asctime_s(ptr long ptr) MSVCRT_asctime_s
@ cdecl _o_asin(double) MSVCRT_asin
@ cdecl -arch=!i386 _o_asinf(float) MSVCRT_asinf
@ cdecl _o_asinh(double) MSVCR120_asinh
@ cdecl _o_asinhf(float) MSVCR120_asinhf
@ cdecl _o_asinhl(double) MSVCR120_asinhl
@ cdecl _o_atan(double) MSVCRT_atan
@ cdecl _o_atan2(double double) MSVCRT_atan2
@ cdecl -arch=!i386 _o_atan2f(float float) MSVCRT_atan2f
@ cdecl -arch=!i386 _o_atanf(float) MSVCRT_atanf
@ cdecl _o_atanh(double) MSVCR120_atanh
@ cdecl _o_atanhf(float) MSVCR120_atanhf
@ cdecl _o_atanhl(double) MSVCR120_atanhl
@ cdecl _o_atof(str) MSVCRT_atof
@ cdecl _o_atoi(str) MSVCRT_atoi
@ cdecl _o_atol(str) MSVCRT_atol
@ cdecl -ret64 _o_atoll(str) MSVCRT_atoll
@ cdecl _o_bsearch(ptr ptr long long ptr) MSVCRT_bsearch
@ cdecl _o_bsearch_s(ptr ptr long long ptr ptr) MSVCRT_bsearch_s
@ cdecl _o_btowc(long) MSVCRT_btowc
@ cdecl _o_calloc(long long) MSVCRT_calloc
@ cdecl _o_cbrt(double) MSVCR120_cbrt
@ cdecl _o_cbrtf(float) MSVCR120_cbrtf
@ cdecl _o_ceil(double) MSVCRT_ceil
@ cdecl -arch=!i386 _o_ceilf(float) MSVCRT_ceilf
@ cdecl _o_clearerr(ptr) MSVCRT_clearerr
@ cdecl _o_clearerr_s(ptr) MSVCRT_clearerr_s
@ cdecl _o_cos(double) MSVCRT_cos
@ cdecl -arch=!i386 _o_cosf(float) MSVCRT_cosf
@ cdecl _o_cosh(double) MSVCRT_cosh
@ cdecl -arch=!i386 _o_coshf(float) MSVCRT_coshf
@ cdecl _o_erf(double) MSVCR120_erf
@ cdecl _o_erfc(double) MSVCR120_erfc
@ cdecl _o_erfcf(float) MSVCR120_erfcf
@ cdecl _o_erfcl(double) MSVCR120_erfcl
@ cdecl _o_erff(float) MSVCR120_erff
@ cdecl _o_erfl(double) MSVCR120_erfl
@ cdecl _o_exit(long) MSVCRT_exit
@ cdecl _o_exp(double) MSVCRT_exp
@ cdecl _o_exp2(double) MSVCR120_exp2
@ cdecl _o_exp2f(float) MSVCR120_exp2f
@ cdecl _o_exp2l(double) MSVCR120_exp2l
@ cdecl -arch=!i386 _o_expf(float) MSVCRT_expf
@ cdecl _o_fabs(double) MSVCRT_fabs
@ cdecl _o_fclose(ptr) MSVCRT_fclose
@ cdecl _o_feof(ptr) MSVCRT_feof
@ cdecl _o_ferror(ptr) MSVCRT_ferror
@ cdecl _o_fflush(ptr) MSVCRT_fflush
@ cdecl _o_fgetc(ptr) MSVCRT_fgetc
@ cdecl _o_fgetpos(ptr ptr) MSVCRT_fgetpos
@ cdecl _o_fgets(ptr long ptr) MSVCRT_fgets
@ cdecl _o_fgetwc(ptr) MSVCRT_fgetwc
@ cdecl _o_fgetws(ptr long ptr) MSVCRT_fgetws
@ cdecl _o_floor(double) MSVCRT_floor
@ cdecl -arch=!i386 _o_floorf(float) MSVCRT_floorf
@ cdecl _o_fma(double double double) MSVCRT_fma
@ cdecl _o_fmaf(float float float) MSVCRT_fmaf
@ cdecl _o_fmal(double double double) MSVCRT_fma
@ cdecl _o_fmod(double double) MSVCRT_fmod
@ cdecl -arch=!i386 _o_fmodf(float float) MSVCRT_fmodf
@ cdecl _o_fopen(str str) MSVCRT_fopen
@ cdecl _o_fopen_s(ptr str str) MSVCRT_fopen_s
@ cdecl _o_fputc(long ptr) MSVCRT_fputc
@ cdecl _o_fputs(str ptr) MSVCRT_fputs
@ cdecl _o_fputwc(long ptr) MSVCRT_fputwc
@ cdecl _o_fputws(wstr ptr) MSVCRT_fputws
@ cdecl _o_fread(ptr long long ptr) MSVCRT_fread
@ cdecl _o_fread_s(ptr long long long ptr) MSVCRT_fread_s
@ cdecl _o_free(ptr) MSVCRT_free
@ cdecl _o_freopen(str str ptr) MSVCRT_freopen
@ cdecl _o_freopen_s(ptr str str ptr) MSVCRT_freopen_s
@ cdecl _o_frexp(double ptr) MSVCRT_frexp
@ cdecl _o_fseek(ptr long long) MSVCRT_fseek
@ cdecl _o_fsetpos(ptr ptr) MSVCRT_fsetpos
@ cdecl _o_ftell(ptr) MSVCRT_ftell
@ cdecl _o_fwrite(ptr long long ptr) MSVCRT_fwrite
@ cdecl _o_getc(ptr) MSVCRT_getc
@ cdecl _o_getchar() MSVCRT_getchar
@ cdecl _o_getenv(str) MSVCRT_getenv
@ cdecl _o_getenv_s(ptr ptr long str) getenv_s
@ cdecl _o_gets(str) MSVCRT_gets
@ cdecl _o_gets_s(ptr long) MSVCRT_gets_s
@ cdecl _o_getwc(ptr) MSVCRT_getwc
@ cdecl _o_getwchar() MSVCRT_getwchar
@ cdecl _o_hypot(double double) _hypot
@ cdecl _o_is_wctype(long long) MSVCRT_iswctype
@ cdecl _o_isalnum(long) MSVCRT_isalnum
@ cdecl _o_isalpha(long) MSVCRT_isalpha
@ cdecl _o_isblank(long) MSVCRT_isblank
@ cdecl _o_iscntrl(long) MSVCRT_iscntrl
@ cdecl _o_isdigit(long) MSVCRT_isdigit
@ cdecl _o_isgraph(long) MSVCRT_isgraph
@ cdecl _o_isleadbyte(long) MSVCRT_isleadbyte
@ cdecl _o_islower(long) MSVCRT_islower
@ cdecl _o_isprint(long) MSVCRT_isprint
@ cdecl _o_ispunct(long) MSVCRT_ispunct
@ cdecl _o_isspace(long) MSVCRT_isspace
@ cdecl _o_isupper(long) MSVCRT_isupper
@ cdecl _o_iswalnum(long) MSVCRT_iswalnum
@ cdecl _o_iswalpha(long) MSVCRT_iswalpha
@ cdecl _o_iswascii(long) MSVCRT_iswascii
@ cdecl _o_iswblank(long) MSVCRT_iswblank
@ cdecl _o_iswcntrl(long) MSVCRT_iswcntrl
@ cdecl _o_iswctype(long long) MSVCRT_iswctype
@ cdecl _o_iswdigit(long) MSVCRT_iswdigit
@ cdecl _o_iswgraph(long) MSVCRT_iswgraph
@ cdecl _o_iswlower(long) MSVCRT_iswlower
@ cdecl _o_iswprint(long) MSVCRT_iswprint
@ cdecl _o_iswpunct(long) MSVCRT_iswpunct
@ cdecl _o_iswspace(long) MSVCRT_iswspace
@ cdecl _o_iswupper(long) MSVCRT_iswupper
@ cdecl _o_iswxdigit(long) MSVCRT_iswxdigit
@ cdecl _o_isxdigit(long) MSVCRT_isxdigit
@ cdecl _o_ldexp(double long) MSVCRT_ldexp
@ cdecl _o_lgamma(double) MSVCR120_lgamma
@ cdecl _o_lgammaf(float) MSVCR120_lgammaf
@ cdecl _o_lgammal(double) MSVCR120_lgammal
@ cdecl -ret64 _o_llrint(double) MSVCR120_llrint
@ cdecl -ret64 _o_llrintf(float) MSVCR120_llrintf
@ cdecl -ret64 _o_llrintl(double) MSVCR120_llrintl
@ cdecl -ret64 _o_llround(double) MSVCR120_llround
@ cdecl -ret64 _o_llroundf(float) MSVCR120_llroundf
@ cdecl -ret64 _o_llroundl(double) MSVCR120_llroundl
@ cdecl _o_localeconv() MSVCRT_localeconv
@ cdecl _o_log(double) MSVCRT_log
@ cdecl _o_log10(double) MSVCRT_log10
@ cdecl -arch=!i386 _o_log10f(float) MSVCRT_log10f
@ cdecl _o_log1p(double) MSVCR120_log1p
@ cdecl _o_log1pf(float) MSVCR120_log1pf
@ cdecl _o_log1pl(double) MSVCR120_log1pl
@ cdecl _o_log2(double) MSVCR120_log2
@ cdecl _o_log2f(float) MSVCR120_log2f
@ cdecl _o_log2l(double) MSVCR120_log2l
@ cdecl _o_logb(double) MSVCRT__logb
@ cdecl _o_logbf(float) MSVCRT__logbf
@ cdecl _o_logbl(double) MSVCRT__logb
@ cdecl -arch=!i386 _o_logf(float) MSVCRT_logf
@ cdecl _o_lrint(double) MSVCR120_lrint
@ cdecl _o_lrintf(float) MSVCR120_lrintf
@ cdecl _o_lrintl(double) MSVCR120_lrintl
@ cdecl _o_lround(double) MSVCR120_lround
@ cdecl _o_lroundf(float) MSVCR120_lroundf
@ cdecl _o_lroundl(double) MSVCR120_lroundl
@ cdecl _o_malloc(long) MSVCRT_malloc
@ cdecl _o_mblen(ptr long) MSVCRT_mblen
@ cdecl _o_mbrlen(ptr long ptr) MSVCRT_mbrlen
@ stub _o_mbrtoc16
@ stub _o_mbrtoc32
@ cdecl _o_mbrtowc(ptr str long ptr) MSVCRT_mbrtowc
@ cdecl _o_mbsrtowcs(ptr ptr long ptr) MSVCRT_mbsrtowcs
@ cdecl _o_mbsrtowcs_s(ptr ptr long ptr long ptr) MSVCRT_mbsrtowcs_s
@ cdecl _o_mbstowcs(ptr str long) MSVCRT_mbstowcs
@ cdecl _o_mbstowcs_s(ptr ptr long str long) MSVCRT__mbstowcs_s
@ cdecl _o_mbtowc(ptr str long) MSVCRT_mbtowc
@ cdecl _o_memcpy_s(ptr long ptr long) MSVCRT_memcpy_s
@ cdecl _o_memset(ptr long long) memset
@ cdecl _o_modf(double ptr) MSVCRT_modf
@ cdecl -arch=!i386 _o_modff(float ptr) MSVCRT_modff
@ cdecl _o_nan(str) MSVCR120_nan
@ cdecl _o_nanf(str) MSVCR120_nanf
@ cdecl _o_nanl(str) MSVCR120_nan
@ cdecl _o_nearbyint(double) MSVCRT_nearbyint
@ cdecl _o_nearbyintf(float) MSVCRT_nearbyintf
@ cdecl _o_nearbyintl(double) MSVCRT_nearbyint
@ cdecl _o_nextafter(double double) MSVCRT__nextafter
@ cdecl _o_nextafterf(float float) MSVCRT__nextafterf
@ cdecl _o_nextafterl(double double) MSVCRT__nextafter
@ cdecl _o_nexttoward(double double) MSVCRT_nexttoward
@ cdecl _o_nexttowardf(float double) MSVCRT_nexttowardf
@ cdecl _o_nexttowardl(double double) MSVCRT_nexttoward
@ cdecl _o_pow(double double) MSVCRT_pow
@ cdecl -arch=!i386 _o_powf(float float) MSVCRT_powf
@ cdecl _o_putc(long ptr) MSVCRT_putc
@ cdecl _o_putchar(long) MSVCRT_putchar
@ cdecl _o_puts(str) MSVCRT_puts
@ cdecl _o_putwc(long ptr) MSVCRT_fputwc
@ cdecl _o_putwchar(long) MSVCRT__fputwchar
@ cdecl _o_qsort(ptr long long ptr) MSVCRT_qsort
@ cdecl _o_qsort_s(ptr long long ptr ptr) MSVCRT_qsort_s
@ cdecl _o_raise(long) MSVCRT_raise
@ cdecl _o_rand() MSVCRT_rand
@ cdecl _o_rand_s(ptr) MSVCRT_rand_s
@ cdecl _o_realloc(ptr long) MSVCRT_realloc
@ cdecl _o_remainder(double double) MSVCR120_remainder
@ cdecl _o_remainderf(float float) MSVCR120_remainderf
@ cdecl _o_remainderl(double double) MSVCR120_remainderl
@ cdecl _o_remove(str) MSVCRT_remove
@ cdecl _o_remquo(double double ptr) MSVCR120_remquo
@ cdecl _o_remquof(float float ptr) MSVCR120_remquof
@ cdecl _o_remquol(double double ptr) MSVCR120_remquol
@ cdecl _o_rename(str str) MSVCRT_rename
@ cdecl _o_rewind(ptr) MSVCRT_rewind
@ cdecl _o_rint(double) MSVCR120_rint
@ cdecl _o_rintf(float) MSVCR120_rintf
@ cdecl _o_rintl(double) MSVCR120_rintl
@ cdecl _o_round(double) MSVCR120_round
@ cdecl _o_roundf(float) MSVCR120_roundf
@ cdecl _o_roundl(double) MSVCR120_roundl
@ cdecl _o_scalbln(double long) MSVCRT__scalb
@ cdecl _o_scalblnf(float long) MSVCRT__scalbf
@ cdecl _o_scalblnl(double long) MSVCR120_scalbnl
@ cdecl _o_scalbn(double long) MSVCRT__scalb
@ cdecl _o_scalbnf(float long) MSVCRT__scalbf
@ cdecl _o_scalbnl(double long) MSVCR120_scalbnl
@ cdecl _o_set_terminate(ptr) MSVCRT_set_terminate
@ cdecl _o_setbuf(ptr ptr) MSVCRT_setbuf
@ cdecl _o_setlocale(long str) MSVCRT_setlocale
@ cdecl _o_setvbuf(ptr str long long) MSVCRT_setvbuf
@ cdecl _o_sin(double) MSVCRT_sin
@ cdecl -arch=!i386 _o_sinf(float) MSVCRT_sinf
@ cdecl _o_sinh(double) MSVCRT_sinh
@ cdecl -arch=!i386 _o_sinhf(float) MSVCRT_sinhf
@ cdecl _o_sqrt(double) MSVCRT_sqrt
@ cdecl -arch=!i386 _o_sqrtf(float) MSVCRT_sqrtf
@ cdecl _o_srand(long) MSVCRT_srand
@ cdecl _o_strcat_s(str long str) MSVCRT_strcat_s
@ cdecl _o_strcoll(str str) MSVCRT_strcoll
@ cdecl _o_strcpy_s(ptr long str) MSVCRT_strcpy_s
@ cdecl _o_strerror(long) MSVCRT_strerror
@ cdecl _o_strerror_s(ptr long long) MSVCRT_strerror_s
@ cdecl _o_strftime(ptr long str ptr) MSVCRT_strftime
@ cdecl _o_strncat_s(str long str long) MSVCRT_strncat_s
@ cdecl _o_strncpy_s(ptr long str long) MSVCRT_strncpy_s
@ cdecl _o_strtod(str ptr) MSVCRT_strtod
@ cdecl _o_strtof(str ptr) MSVCRT_strtof
@ cdecl _o_strtok(str str) MSVCRT_strtok
@ cdecl _o_strtok_s(ptr str ptr) MSVCRT_strtok_s
@ cdecl _o_strtol(str ptr long) MSVCRT_strtol
@ cdecl _o_strtold(str ptr) MSVCRT_strtod
@ cdecl -ret64 _o_strtoll(str ptr long) MSVCRT_strtoi64
@ cdecl _o_strtoul(str ptr long) MSVCRT_strtoul
@ cdecl -ret64 _o_strtoull(str ptr long) MSVCRT_strtoui64
@ cdecl _o_system(str) MSVCRT_system
@ cdecl _o_tan(double) MSVCRT_tan
@ cdecl -arch=!i386 _o_tanf(float) MSVCRT_tanf
@ cdecl _o_tanh(double) MSVCRT_tanh
@ cdecl -arch=!i386 _o_tanhf(float) MSVCRT_tanhf
@ cdecl _o_terminate() MSVCRT_terminate
@ cdecl _o_tgamma(double) MSVCR120_tgamma
@ cdecl _o_tgammaf(float) MSVCR120_tgammaf
@ cdecl _o_tgammal(double) MSVCR120_tgamma
@ cdecl _o_tmpfile_s(ptr) MSVCRT_tmpfile_s
@ cdecl _o_tmpnam_s(ptr long) MSVCRT_tmpnam_s
@ cdecl _o_tolower(long) MSVCRT_tolower
@ cdecl _o_toupper(long) MSVCRT_toupper
@ cdecl _o_towlower(long) MSVCRT_towlower
@ cdecl _o_towupper(long) MSVCRT_towupper
@ cdecl _o_ungetc(long ptr) MSVCRT_ungetc
@ cdecl _o_ungetwc(long ptr) MSVCRT_ungetwc
@ cdecl _o_wcrtomb(ptr long ptr) MSVCRT_wcrtomb
@ cdecl _o_wcrtomb_s(ptr ptr long long ptr) MSVCRT_wcrtomb_s
@ cdecl _o_wcscat_s(wstr long wstr) MSVCRT_wcscat_s
@ cdecl _o_wcscoll(wstr wstr) MSVCRT_wcscoll
@ cdecl _o_wcscpy(ptr wstr) MSVCRT_wcscpy
@ cdecl _o_wcscpy_s(ptr long wstr) MSVCRT_wcscpy_s
@ cdecl _o_wcsftime(ptr long wstr ptr) MSVCRT_wcsftime
@ cdecl _o_wcsncat_s(wstr long wstr long) MSVCRT_wcsncat_s
@ cdecl _o_wcsncpy_s(ptr long wstr long) MSVCRT_wcsncpy_s
@ cdecl _o_wcsrtombs(ptr ptr long ptr) MSVCRT_wcsrtombs
@ cdecl _o_wcsrtombs_s(ptr ptr long ptr long ptr) MSVCRT_wcsrtombs_s
@ cdecl _o_wcstod(wstr ptr) MSVCRT_wcstod
@ cdecl _o_wcstof(ptr ptr) MSVCRT_wcstof
@ cdecl _o_wcstok(wstr wstr ptr) MSVCRT_wcstok
@ cdecl _o_wcstok_s(ptr wstr ptr) MSVCRT_wcstok_s
@ cdecl _o_wcstol(wstr ptr long) MSVCRT_wcstol
@ cdecl _o_wcstold(wstr ptr ptr) MSVCRT_wcstod
@ cdecl -ret64 _o_wcstoll(wstr ptr long) MSVCRT__wcstoi64
@ cdecl _o_wcstombs(ptr ptr long) MSVCRT_wcstombs
@ cdecl _o_wcstombs_s(ptr ptr long wstr long) MSVCRT_wcstombs_s
@ cdecl _o_wcstoul(wstr ptr long) MSVCRT_wcstoul
@ cdecl -ret64 _o_wcstoull(wstr ptr long) MSVCRT__wcstoui64
@ cdecl _o_wctob(long) MSVCRT_wctob
@ cdecl _o_wctomb(ptr long) MSVCRT_wctomb
@ cdecl _o_wctomb_s(ptr ptr long long) MSVCRT_wctomb_s
@ cdecl _o_wmemcpy_s(ptr long ptr long) wmemcpy_s
@ cdecl _o_wmemmove_s(ptr long ptr long) wmemmove_s
@ varargs _open(str long) MSVCRT__open
@ cdecl _open_osfhandle(long long) MSVCRT__open_osfhandle
@ cdecl _pclose(ptr) MSVCRT__pclose
@ cdecl _pipe(ptr long long) MSVCRT__pipe
@ cdecl _popen(str str) MSVCRT__popen
@ cdecl _purecall()
@ cdecl _putc_nolock(long ptr) MSVCRT__fputc_nolock
@ cdecl _putch(long)
@ cdecl _putch_nolock(long)
@ cdecl _putenv(str)
@ cdecl _putenv_s(str str)
@ cdecl _putw(long ptr) MSVCRT__putw
@ cdecl _putwc_nolock(long ptr) MSVCRT__fputwc_nolock
@ cdecl _putwch(long)
@ cdecl _putwch_nolock(long)
@ cdecl _putws(wstr) MSVCRT__putws
@ stub _query_app_type
@ cdecl _query_new_handler() MSVCRT__query_new_handler
@ cdecl _query_new_mode() MSVCRT__query_new_mode
@ cdecl _read(long ptr long) MSVCRT__read
@ cdecl _realloc_base(ptr long)
@ cdecl _recalloc(ptr long long)
@ cdecl _register_onexit_function(ptr ptr) MSVCRT__register_onexit_function
@ cdecl _register_thread_local_exe_atexit_callback(ptr)
@ cdecl _resetstkoflw() MSVCRT__resetstkoflw
@ cdecl _rmdir(str) MSVCRT__rmdir
@ cdecl _rmtmp() MSVCRT__rmtmp
@ cdecl _rotl(long long)
@ cdecl -ret64 _rotl64(int64 long)
@ cdecl _rotr(long long)
@ cdecl -ret64 _rotr64(int64 long)
@ cdecl _scalb(double long) MSVCRT__scalb
@ cdecl -arch=x86_64 _scalbf(float long) MSVCRT__scalbf
@ cdecl _searchenv(str str ptr) MSVCRT__searchenv
@ cdecl _searchenv_s(str str ptr long) MSVCRT__searchenv_s
@ cdecl -arch=i386,x86_64,arm,arm64 _seh_filter_dll(long ptr) __CppXcptFilter
@ cdecl _seh_filter_exe(long ptr) _XcptFilter
@ cdecl -arch=win64 _set_FMA3_enable(long) MSVCRT__set_FMA3_enable
@ stdcall -arch=i386 _seh_longjmp_unwind4(ptr)
@ stdcall -arch=i386 _seh_longjmp_unwind(ptr)
@ cdecl -arch=i386 _set_SSE2_enable(long) MSVCRT__set_SSE2_enable
@ cdecl _set_abort_behavior(long long) MSVCRT__set_abort_behavior
@ cdecl _set_app_type(long) MSVCRT___set_app_type
@ cdecl _set_controlfp(long long)
@ cdecl _set_doserrno(long)
@ cdecl _set_errno(long)
@ cdecl _set_error_mode(long)
@ cdecl _set_fmode(long) MSVCRT__set_fmode
@ cdecl _set_invalid_parameter_handler(ptr)
@ cdecl _set_new_handler(ptr) MSVCRT_set_new_handler
@ cdecl _set_new_mode(long) MSVCRT__set_new_mode
@ cdecl _set_printf_count_output(long) MSVCRT__set_printf_count_output
@ cdecl _set_purecall_handler(ptr)
@ cdecl _set_se_translator(ptr) MSVCRT__set_se_translator
@ cdecl _set_thread_local_invalid_parameter_handler(ptr)
@ cdecl _seterrormode(long)
@ cdecl -arch=i386 -norelay _setjmp3(ptr long) MSVCRT__setjmp3
@ cdecl _setmaxstdio(long) MSVCRT__setmaxstdio
@ cdecl _setmbcp(long)
@ cdecl _setmode(long long) MSVCRT__setmode
@ stub _setsystime(ptr long)
@ cdecl _sleep(long) MSVCRT__sleep
@ varargs _sopen(str long long) MSVCRT__sopen
@ cdecl _sopen_dispatch(str long long long ptr long) MSVCRT__sopen_dispatch
@ cdecl _sopen_s(ptr str long long long) MSVCRT__sopen_s
@ varargs _spawnl(long str str) MSVCRT__spawnl
@ varargs _spawnle(long str str) MSVCRT__spawnle
@ varargs _spawnlp(long str str) MSVCRT__spawnlp
@ varargs _spawnlpe(long str str) MSVCRT__spawnlpe
@ cdecl _spawnv(long str ptr) MSVCRT__spawnv
@ cdecl _spawnve(long str ptr ptr) MSVCRT__spawnve
@ cdecl _spawnvp(long str ptr) MSVCRT__spawnvp
@ cdecl _spawnvpe(long str ptr ptr) MSVCRT__spawnvpe
@ cdecl _splitpath(str ptr ptr ptr ptr) MSVCRT__splitpath
@ cdecl _splitpath_s(str ptr long ptr long ptr long ptr long) MSVCRT__splitpath_s
@ cdecl _stat32(str ptr) MSVCRT__stat32
@ cdecl _stat32i64(str ptr) MSVCRT__stat32i64
@ cdecl _stat64(str ptr) MSVCRT_stat64
@ cdecl _stat64i32(str ptr) MSVCRT__stat64i32
@ cdecl _statusfp()
@ cdecl -arch=i386 _statusfp2(ptr ptr)
@ cdecl _strcoll_l(str str ptr) MSVCRT_strcoll_l
@ cdecl _strdate(ptr) MSVCRT__strdate
@ cdecl _strdate_s(ptr long)
@ cdecl _strdup(str) MSVCRT__strdup
@ cdecl _strerror(long) MSVCRT__strerror
@ stub _strerror_s
@ cdecl _strftime_l(ptr long str ptr ptr) MSVCRT__strftime_l
@ cdecl _stricmp(str str) MSVCRT__stricmp
@ cdecl _stricmp_l(str str ptr) MSVCRT__stricmp_l
@ cdecl _stricoll(str str) MSVCRT__stricoll
@ cdecl _stricoll_l(str str ptr) MSVCRT__stricoll_l
@ cdecl _strlwr(str) MSVCRT__strlwr
@ cdecl _strlwr_l(str ptr)
@ cdecl _strlwr_s(ptr long) MSVCRT__strlwr_s
@ cdecl _strlwr_s_l(ptr long ptr) MSVCRT__strlwr_s_l
@ cdecl _strncoll(str str long) MSVCRT__strncoll
@ cdecl _strncoll_l(str str long ptr) MSVCRT__strncoll_l
@ cdecl _strnicmp(str str long) MSVCRT__strnicmp
@ cdecl _strnicmp_l(str str long ptr) MSVCRT__strnicmp_l
@ cdecl _strnicoll(str str long) MSVCRT__strnicoll
@ cdecl _strnicoll_l(str str long ptr) MSVCRT__strnicoll_l
@ cdecl _strnset(str long long) MSVCRT__strnset
@ cdecl _strnset_s(str long long long) MSVCRT__strnset_s
@ cdecl _strrev(str) MSVCRT__strrev
@ cdecl _strset(str long)
@ stub _strset_s
@ cdecl _strtime(ptr) MSVCRT__strtime
@ cdecl _strtime_s(ptr long)
@ cdecl _strtod_l(str ptr ptr) MSVCRT_strtod_l
@ cdecl _strtof_l(str ptr ptr) MSVCRT__strtof_l
@ cdecl -ret64 _strtoi64(str ptr long) MSVCRT_strtoi64
@ cdecl -ret64 _strtoi64_l(str ptr long ptr) MSVCRT_strtoi64_l
@ cdecl -ret64 _strtoimax_l(str ptr long ptr) MSVCRT_strtoi64_l
@ cdecl _strtol_l(str ptr long ptr) MSVCRT__strtol_l
@ cdecl _strtold_l(str ptr ptr) MSVCRT_strtod_l
@ cdecl -ret64 _strtoll_l(str ptr long ptr) MSVCRT_strtoi64_l
@ cdecl -ret64 _strtoui64(str ptr long) MSVCRT_strtoui64
@ cdecl -ret64 _strtoui64_l(str ptr long ptr) MSVCRT_strtoui64_l
@ cdecl _strtoul_l(str ptr long ptr) MSVCRT_strtoul_l
@ cdecl -ret64 _strtoull_l(str ptr long ptr) MSVCRT_strtoui64_l
@ cdecl -ret64 _strtoumax_l(str ptr long ptr) MSVCRT_strtoui64_l
@ cdecl _strupr(str) MSVCRT__strupr
@ cdecl _strupr_l(str ptr) MSVCRT__strupr_l
@ cdecl _strupr_s(str long) MSVCRT__strupr_s
@ cdecl _strupr_s_l(str long ptr) MSVCRT__strupr_s_l
@ cdecl _strxfrm_l(ptr str long ptr) MSVCRT__strxfrm_l
@ cdecl _swab(str str long) MSVCRT__swab
@ cdecl _tell(long) MSVCRT__tell
@ cdecl -ret64 _telli64(long)
@ cdecl _tempnam(str str) MSVCRT__tempnam
@ cdecl _time32(ptr) MSVCRT__time32
@ cdecl _time64(ptr) MSVCRT__time64
@ cdecl _timespec32_get(ptr long)
@ cdecl _timespec64_get(ptr long)
@ cdecl _tolower(long) MSVCRT__tolower
@ cdecl _tolower_l(long ptr) MSVCRT__tolower_l
@ cdecl _toupper(long) MSVCRT__toupper
@ cdecl _toupper_l(long ptr) MSVCRT__toupper_l
@ cdecl _towlower_l(long ptr) MSVCRT__towlower_l
@ cdecl _towupper_l(long ptr) MSVCRT__towupper_l
@ cdecl _tzset() MSVCRT__tzset
@ cdecl _ui64toa(int64 ptr long) ntdll._ui64toa
@ cdecl _ui64toa_s(int64 ptr long long) MSVCRT__ui64toa_s
@ cdecl _ui64tow(int64 ptr long) ntdll._ui64tow
@ cdecl _ui64tow_s(int64 ptr long long) MSVCRT__ui64tow_s
@ cdecl _ultoa(long ptr long) ntdll._ultoa
@ cdecl _ultoa_s(long ptr long long) MSVCRT__ultoa_s
@ cdecl _ultow(long ptr long) ntdll._ultow
@ cdecl _ultow_s(long ptr long long) MSVCRT__ultow_s
@ cdecl _umask(long) MSVCRT__umask
@ stub _umask_s
@ cdecl _ungetc_nolock(long ptr) MSVCRT__ungetc_nolock
@ cdecl _ungetch(long)
@ cdecl _ungetch_nolock(long)
@ cdecl _ungetwc_nolock(long ptr) MSVCRT__ungetwc_nolock
@ cdecl _ungetwch(long)
@ cdecl _ungetwch_nolock(long)
@ cdecl _unlink(str) MSVCRT__unlink
@ cdecl _unloaddll(long)
@ cdecl _unlock_file(ptr) MSVCRT__unlock_file
@ cdecl _unlock_locales()
@ cdecl _utime32(str ptr)
@ cdecl _utime64(str ptr)
@ cdecl _waccess(wstr long) MSVCRT__waccess
@ cdecl _waccess_s(wstr long) MSVCRT__waccess_s
@ cdecl _wasctime(ptr) MSVCRT__wasctime
@ cdecl _wasctime_s(ptr long ptr) MSVCRT__wasctime_s
@ cdecl _wassert(wstr wstr long) MSVCRT__wassert
@ cdecl _wchdir(wstr) MSVCRT__wchdir
@ cdecl _wchmod(wstr long) MSVCRT__wchmod
@ cdecl _wcreat(wstr long) MSVCRT__wcreat
@ cdecl _wcreate_locale(long wstr) MSVCRT__wcreate_locale
@ cdecl _wcscoll_l(wstr wstr ptr) MSVCRT__wcscoll_l
@ cdecl _wcsdup(wstr) MSVCRT__wcsdup
@ cdecl _wcserror(long) MSVCRT__wcserror
@ cdecl _wcserror_s(ptr long long) MSVCRT__wcserror_s
@ cdecl _wcsftime_l(ptr long wstr ptr ptr) MSVCRT__wcsftime_l
@ cdecl _wcsicmp(wstr wstr) MSVCRT__wcsicmp
@ cdecl _wcsicmp_l(wstr wstr ptr) MSVCRT__wcsicmp_l
@ cdecl _wcsicoll(wstr wstr) MSVCRT__wcsicoll
@ cdecl _wcsicoll_l(wstr wstr ptr) MSVCRT__wcsicoll_l
@ cdecl _wcslwr(wstr) MSVCRT__wcslwr
@ cdecl _wcslwr_l(wstr ptr) MSVCRT__wcslwr_l
@ cdecl _wcslwr_s(wstr long) MSVCRT__wcslwr_s
@ cdecl _wcslwr_s_l(wstr long ptr) MSVCRT__wcslwr_s_l
@ cdecl _wcsncoll(wstr wstr long) MSVCRT__wcsncoll
@ cdecl _wcsncoll_l(wstr wstr long ptr) MSVCRT__wcsncoll_l
@ cdecl _wcsnicmp(wstr wstr long) MSVCRT__wcsnicmp
@ cdecl _wcsnicmp_l(wstr wstr long ptr) MSVCRT__wcsnicmp_l
@ cdecl _wcsnicoll(wstr wstr long) MSVCRT__wcsnicoll
@ cdecl _wcsnicoll_l(wstr wstr long ptr) MSVCRT__wcsnicoll_l
@ cdecl _wcsnset(wstr long long) MSVCRT__wcsnset
@ cdecl _wcsnset_s(wstr long long long) MSVCRT__wcsnset_s
@ cdecl _wcsrev(wstr) MSVCRT__wcsrev
@ cdecl _wcsset(wstr long) MSVCRT__wcsset
@ cdecl _wcsset_s(wstr long long) MSVCRT__wcsset_s
@ cdecl _wcstod_l(wstr ptr ptr) MSVCRT__wcstod_l
@ cdecl _wcstof_l(wstr ptr ptr) MSVCRT__wcstof_l
@ cdecl -ret64 _wcstoi64(wstr ptr long) MSVCRT__wcstoi64
@ cdecl -ret64 _wcstoi64_l(wstr ptr long ptr) MSVCRT__wcstoi64_l
@ stub _wcstoimax_l
@ cdecl _wcstol_l(wstr ptr long ptr) MSVCRT__wcstol_l
@ cdecl _wcstold_l(wstr ptr ptr) MSVCRT__wcstod_l
@ cdecl -ret64 _wcstoll_l(wstr ptr long ptr) MSVCRT__wcstoi64_l
@ cdecl _wcstombs_l(ptr ptr long ptr) MSVCRT__wcstombs_l
@ cdecl _wcstombs_s_l(ptr ptr long wstr long ptr) MSVCRT__wcstombs_s_l
@ cdecl -ret64 _wcstoui64(wstr ptr long) MSVCRT__wcstoui64
@ cdecl -ret64 _wcstoui64_l(wstr ptr long ptr) MSVCRT__wcstoui64_l
@ cdecl _wcstoul_l(wstr ptr long ptr) MSVCRT__wcstoul_l
@ cdecl -ret64 _wcstoull_l(wstr ptr long ptr) MSVCRT__wcstoui64_l
@ stub _wcstoumax_l
@ cdecl _wcsupr(wstr) MSVCRT__wcsupr
@ cdecl _wcsupr_l(wstr ptr) MSVCRT__wcsupr_l
@ cdecl _wcsupr_s(wstr long) MSVCRT__wcsupr_s
@ cdecl _wcsupr_s_l(wstr long ptr) MSVCRT__wcsupr_s_l
@ cdecl _wcsxfrm_l(ptr wstr long ptr) MSVCRT__wcsxfrm_l
@ cdecl _wctime32(ptr) MSVCRT__wctime32
@ cdecl _wctime32_s(ptr long ptr) MSVCRT__wctime32_s
@ cdecl _wctime64(ptr) MSVCRT__wctime64
@ cdecl _wctime64_s(ptr long ptr) MSVCRT__wctime64_s
@ cdecl _wctomb_l(ptr long ptr) MSVCRT__wctomb_l
@ cdecl _wctomb_s_l(ptr ptr long long ptr) MSVCRT__wctomb_s_l
@ extern _wctype MSVCRT__wctype
@ cdecl _wdupenv_s(ptr ptr wstr)
@ varargs _wexecl(wstr wstr)
@ varargs _wexecle(wstr wstr)
@ varargs _wexeclp(wstr wstr)
@ varargs _wexeclpe(wstr wstr)
@ cdecl _wexecv(wstr ptr)
@ cdecl _wexecve(wstr ptr ptr)
@ cdecl _wexecvp(wstr ptr)
@ cdecl _wexecvpe(wstr ptr ptr)
@ cdecl _wfdopen(long wstr) MSVCRT__wfdopen
@ cdecl _wfindfirst32(wstr ptr) MSVCRT__wfindfirst32
@ stub _wfindfirst32i64
@ cdecl _wfindfirst64(wstr ptr) MSVCRT__wfindfirst64
@ cdecl _wfindfirst64i32(wstr ptr) MSVCRT__wfindfirst64i32
@ cdecl _wfindnext32(long ptr) MSVCRT__wfindnext32
@ stub _wfindnext32i64
@ cdecl _wfindnext64(long ptr) MSVCRT__wfindnext64
@ cdecl _wfindnext64i32(long ptr) MSVCRT__wfindnext64i32
@ cdecl _wfopen(wstr wstr) MSVCRT__wfopen
@ cdecl _wfopen_s(ptr wstr wstr) MSVCRT__wfopen_s
@ cdecl _wfreopen(wstr wstr ptr) MSVCRT__wfreopen
@ cdecl _wfreopen_s(ptr wstr wstr ptr) MSVCRT__wfreopen_s
@ cdecl _wfsopen(wstr wstr long) MSVCRT__wfsopen
@ cdecl _wfullpath(ptr wstr long) MSVCRT__wfullpath
@ cdecl _wgetcwd(wstr long) MSVCRT__wgetcwd
@ cdecl _wgetdcwd(long wstr long) MSVCRT__wgetdcwd
@ cdecl _wgetenv(wstr) MSVCRT__wgetenv
@ cdecl _wgetenv_s(ptr ptr long wstr)
@ cdecl _wmakepath(ptr wstr wstr wstr wstr) MSVCRT__wmakepath
@ cdecl _wmakepath_s(ptr long wstr wstr wstr wstr) MSVCRT__wmakepath_s
@ cdecl _wmkdir(wstr) MSVCRT__wmkdir
@ cdecl _wmktemp(wstr) MSVCRT__wmktemp
@ cdecl _wmktemp_s(wstr long) MSVCRT__wmktemp_s
@ varargs _wopen(wstr long) MSVCRT__wopen
@ cdecl _wperror(wstr) MSVCRT__wperror
@ cdecl _wpopen(wstr wstr) MSVCRT__wpopen
@ cdecl _wputenv(wstr)
@ cdecl _wputenv_s(wstr wstr)
@ cdecl _wremove(wstr) MSVCRT__wremove
@ cdecl _wrename(wstr wstr) MSVCRT__wrename
@ cdecl _write(long ptr long) MSVCRT__write
@ cdecl _wrmdir(wstr) MSVCRT__wrmdir
@ cdecl _wsearchenv(wstr wstr ptr) MSVCRT__wsearchenv
@ cdecl _wsearchenv_s(wstr wstr ptr long) MSVCRT__wsearchenv_s
@ cdecl _wsetlocale(long wstr) MSVCRT__wsetlocale
@ varargs _wsopen(wstr long long) MSVCRT__wsopen
@ cdecl _wsopen_dispatch(wstr long long long ptr long) MSVCRT__wsopen_dispatch
@ cdecl _wsopen_s(ptr wstr long long long) MSVCRT__wsopen_s
@ varargs _wspawnl(long wstr wstr) MSVCRT__wspawnl
@ varargs _wspawnle(long wstr wstr) MSVCRT__wspawnle
@ varargs _wspawnlp(long wstr wstr) MSVCRT__wspawnlp
@ varargs _wspawnlpe(long wstr wstr) MSVCRT__wspawnlpe
@ cdecl _wspawnv(long wstr ptr) MSVCRT__wspawnv
@ cdecl _wspawnve(long wstr ptr ptr) MSVCRT__wspawnve
@ cdecl _wspawnvp(long wstr ptr) MSVCRT__wspawnvp
@ cdecl _wspawnvpe(long wstr ptr ptr) MSVCRT__wspawnvpe
@ cdecl _wsplitpath(wstr ptr ptr ptr ptr) MSVCRT__wsplitpath
@ cdecl _wsplitpath_s(wstr ptr long ptr long ptr long ptr long) MSVCRT__wsplitpath_s
@ cdecl _wstat32(wstr ptr) MSVCRT__wstat32
@ cdecl _wstat32i64(wstr ptr) MSVCRT__wstat32i64
@ cdecl _wstat64(wstr ptr) MSVCRT__wstat64
@ cdecl _wstat64i32(wstr ptr) MSVCRT__wstat64i32
@ cdecl _wstrdate(ptr) MSVCRT__wstrdate
@ cdecl _wstrdate_s(ptr long)
@ cdecl _wstrtime(ptr) MSVCRT__wstrtime
@ cdecl _wstrtime_s(ptr long)
@ cdecl _wsystem(wstr)
@ cdecl _wtempnam(wstr wstr) MSVCRT__wtempnam
@ cdecl _wtmpnam(ptr) MSVCRT__wtmpnam
@ cdecl _wtmpnam_s(ptr long) MSVCRT__wtmpnam_s
@ cdecl _wtof(wstr) MSVCRT__wtof
@ cdecl _wtof_l(wstr ptr) MSVCRT__wtof_l
@ cdecl _wtoi(wstr) MSVCRT__wtoi
@ cdecl -ret64 _wtoi64(wstr) MSVCRT__wtoi64
@ cdecl -ret64 _wtoi64_l(wstr ptr) MSVCRT__wtoi64_l
@ cdecl _wtoi_l(wstr ptr) MSVCRT__wtoi_l
@ cdecl _wtol(wstr) MSVCRT__wtol
@ cdecl _wtol_l(wstr ptr) MSVCRT__wtol_l
@ cdecl -ret64 _wtoll(wstr) MSVCRT__wtoll
@ cdecl -ret64 _wtoll_l(wstr ptr) MSVCRT__wtoll_l
@ cdecl _wunlink(wstr) MSVCRT__wunlink
@ cdecl _wutime32(wstr ptr)
@ cdecl _wutime64(wstr ptr)
@ cdecl _y0(double) MSVCRT__y0
@ cdecl _y1(double) MSVCRT__y1
@ cdecl _yn(long double) MSVCRT__yn
@ cdecl abort() MSVCRT_abort
@ cdecl abs(long) MSVCRT_abs
@ cdecl acos(double) MSVCRT_acos
@ cdecl -arch=!i386 acosf(float) MSVCRT_acosf
@ cdecl acosh(double) MSVCR120_acosh
@ cdecl acoshf(float) MSVCR120_acoshf
@ cdecl acoshl(double) MSVCR120_acoshl
@ cdecl asctime(ptr) MSVCRT_asctime
@ cdecl asctime_s(ptr long ptr) MSVCRT_asctime_s
@ cdecl asin(double) MSVCRT_asin
@ cdecl -arch=!i386 asinf(float) MSVCRT_asinf
@ cdecl asinh(double) MSVCR120_asinh
@ cdecl asinhf(float) MSVCR120_asinhf
@ cdecl asinhl(double) MSVCR120_asinhl
@ cdecl atan(double) MSVCRT_atan
@ cdecl atan2(double double) MSVCRT_atan2
@ cdecl -arch=!i386 atan2f(float float) MSVCRT_atan2f
@ cdecl -arch=!i386 atanf(float) MSVCRT_atanf
@ cdecl atanh(double) MSVCR120_atanh
@ cdecl atanhf(float) MSVCR120_atanhf
@ cdecl atanhl(double) MSVCR120_atanhl
@ cdecl atof(str) MSVCRT_atof
@ cdecl atoi(str) MSVCRT_atoi
@ cdecl atol(str) MSVCRT_atol
@ cdecl -ret64 atoll(str) MSVCRT_atoll
@ cdecl bsearch(ptr ptr long long ptr) MSVCRT_bsearch
@ cdecl bsearch_s(ptr ptr long long ptr ptr) MSVCRT_bsearch_s
@ cdecl btowc(long) MSVCRT_btowc
@ stub c16rtomb
@ stub c32rtomb
@ stub cabs
@ stub cabsf
@ stub cabsl
@ stub cacos
@ stub cacosf
@ stub cacosh
@ stub cacoshf
@ stub cacoshl
@ stub cacosl
@ cdecl calloc(long long) MSVCRT_calloc
@ stub carg
@ stub cargf
@ stub cargl
@ stub casin
@ stub casinf
@ stub casinh
@ stub casinhf
@ stub casinhl
@ stub casinl
@ stub catan
@ stub catanf
@ stub catanh
@ stub catanhf
@ stub catanhl
@ stub catanl
@ cdecl cbrt(double) MSVCR120_cbrt
@ cdecl cbrtf(float) MSVCR120_cbrtf
@ cdecl cbrtl(double) MSVCR120_cbrtl
@ stub ccos
@ stub ccosf
@ stub ccosh
@ stub ccoshf
@ stub ccoshl
@ stub ccosl
@ cdecl ceil(double) MSVCRT_ceil
@ cdecl -arch=!i386 ceilf(float) MSVCRT_ceilf
@ stub cexp
@ stub cexpf
@ stub cexpl
@ stub cimag
@ stub cimagf
@ stub cimagl
@ cdecl clearerr(ptr) MSVCRT_clearerr
@ cdecl clearerr_s(ptr) MSVCRT_clearerr_s
@ cdecl clock() MSVCRT_clock
@ stub clog
@ stub clog10
@ stub clog10f
@ stub clog10l
@ stub clogf
@ stub clogl
@ stub conj
@ stub conjf
@ stub conjl
@ cdecl copysign(double double) MSVCRT__copysign
@ cdecl copysignf(float float) MSVCRT__copysignf
@ cdecl copysignl(double double) MSVCRT__copysign
@ cdecl cos(double) MSVCRT_cos
@ cdecl -arch=!i386 cosf(float) MSVCRT_cosf
@ cdecl cosh(double) MSVCRT_cosh
@ cdecl -arch=!i386 coshf(float) MSVCRT_coshf
@ stub cpow
@ stub cpowf
@ stub cpowl
@ stub cproj
@ stub cprojf
@ stub cprojl
@ cdecl creal(int128) MSVCR120_creal
@ stub crealf
@ stub creall
@ stub csin
@ stub csinf
@ stub csinh
@ stub csinhf
@ stub csinhl
@ stub csinl
@ stub csqrt
@ stub csqrtf
@ stub csqrtl
@ stub ctan
@ stub ctanf
@ stub ctanh
@ stub ctanhf
@ stub ctanhl
@ stub ctanl
@ cdecl -ret64 div(long long) MSVCRT_div
@ cdecl erf(double) MSVCR120_erf
@ cdecl erfc(double) MSVCR120_erfc
@ cdecl erfcf(float) MSVCR120_erfcf
@ cdecl erfcl(double) MSVCR120_erfcl
@ cdecl erff(float) MSVCR120_erff
@ cdecl erfl(double) MSVCR120_erfl
@ cdecl exit(long) MSVCRT_exit
@ cdecl exp(double) MSVCRT_exp
@ cdecl exp2(double) MSVCR120_exp2
@ cdecl exp2f(float) MSVCR120_exp2f
@ cdecl exp2l(double) MSVCR120_exp2l
@ cdecl -arch=!i386 expf(float) MSVCRT_expf
@ cdecl expm1(double) MSVCR120_expm1
@ cdecl expm1f(float) MSVCR120_expm1f
@ cdecl expm1l(double) MSVCR120_expm1l
@ cdecl fabs(double) MSVCRT_fabs
@ cdecl -arch=arm,arm64 fabsf(float) MSVCRT_fabsf
@ cdecl fclose(ptr) MSVCRT_fclose
@ cdecl fdim(double double) MSVCR120_fdim
@ cdecl fdimf(float float) MSVCR120_fdimf
@ cdecl fdiml(double double) MSVCR120_fdim
@ stub feclearexcept
@ cdecl fegetenv(ptr) MSVCRT_fegetenv
@ stub fegetexceptflag
@ cdecl fegetround() MSVCRT_fegetround
@ stub feholdexcept
@ cdecl feof(ptr) MSVCRT_feof
@ cdecl ferror(ptr) MSVCRT_ferror
@ cdecl fesetenv(ptr) MSVCRT_fesetenv
@ stub fesetexceptflag
@ cdecl fesetround(long) MSVCRT_fesetround
@ stub fetestexcept
@ cdecl fflush(ptr) MSVCRT_fflush
@ cdecl fgetc(ptr) MSVCRT_fgetc
@ cdecl fgetpos(ptr ptr) MSVCRT_fgetpos
@ cdecl fgets(ptr long ptr) MSVCRT_fgets
@ cdecl fgetwc(ptr) MSVCRT_fgetwc
@ cdecl fgetws(ptr long ptr) MSVCRT_fgetws
@ cdecl floor(double) MSVCRT_floor
@ cdecl -arch=!i386 floorf(float) MSVCRT_floorf
@ cdecl fma(double double double) MSVCRT_fma
@ cdecl fmaf(float float float) MSVCRT_fmaf
@ cdecl fmal(double double double) MSVCRT_fma
@ cdecl fmax(double double) MSVCR120_fmax
@ cdecl fmaxf(float float) MSVCR120_fmaxf
@ cdecl fmaxl(double double) MSVCR120_fmax
@ cdecl fmin(double double) MSVCR120_fmin
@ cdecl fminf(float float) MSVCR120_fminf
@ cdecl fminl(double double) MSVCR120_fmin
@ cdecl fmod(double double) MSVCRT_fmod
@ cdecl -arch=!i386 fmodf(float float) MSVCRT_fmodf
@ cdecl fopen(str str) MSVCRT_fopen
@ cdecl fopen_s(ptr str str) MSVCRT_fopen_s
@ cdecl fputc(long ptr) MSVCRT_fputc
@ cdecl fputs(str ptr) MSVCRT_fputs
@ cdecl fputwc(long ptr) MSVCRT_fputwc
@ cdecl fputws(wstr ptr) MSVCRT_fputws
@ cdecl fread(ptr long long ptr) MSVCRT_fread
@ cdecl fread_s(ptr long long long ptr) MSVCRT_fread_s
@ cdecl free(ptr) MSVCRT_free
@ cdecl freopen(str str ptr) MSVCRT_freopen
@ cdecl freopen_s(ptr str str ptr) MSVCRT_freopen_s
@ cdecl frexp(double ptr) MSVCRT_frexp
@ cdecl fseek(ptr long long) MSVCRT_fseek
@ cdecl fsetpos(ptr ptr) MSVCRT_fsetpos
@ cdecl ftell(ptr) MSVCRT_ftell
@ cdecl fwrite(ptr long long ptr) MSVCRT_fwrite
@ cdecl getc(ptr) MSVCRT_getc
@ cdecl getchar() MSVCRT_getchar
@ cdecl getenv(str) MSVCRT_getenv
@ cdecl getenv_s(ptr ptr long str)
@ cdecl gets(str) MSVCRT_gets
@ cdecl gets_s(ptr long) MSVCRT_gets_s
@ cdecl getwc(ptr) MSVCRT_getwc
@ cdecl getwchar() MSVCRT_getwchar
@ cdecl hypot(double double) _hypot
@ cdecl ilogb(double) MSVCR120_ilogb
@ cdecl ilogbf(float) MSVCR120_ilogbf
@ cdecl ilogbl(double) MSVCR120_ilogbl
@ cdecl -ret64 imaxabs(int64) MSVCRT_imaxabs
@ stub imaxdiv
@ cdecl is_wctype(long long) MSVCRT_iswctype
@ cdecl isalnum(long) MSVCRT_isalnum
@ cdecl isalpha(long) MSVCRT_isalpha
@ cdecl isblank(long) MSVCRT_isblank
@ cdecl iscntrl(long) MSVCRT_iscntrl
@ cdecl isdigit(long) MSVCRT_isdigit
@ cdecl isgraph(long) MSVCRT_isgraph
@ cdecl isleadbyte(long) MSVCRT_isleadbyte
@ cdecl islower(long) MSVCRT_islower
@ cdecl isprint(long) MSVCRT_isprint
@ cdecl ispunct(long) MSVCRT_ispunct
@ cdecl isspace(long) MSVCRT_isspace
@ cdecl isupper(long) MSVCRT_isupper
@ cdecl iswalnum(long) MSVCRT_iswalnum
@ cdecl iswalpha(long) MSVCRT_iswalpha
@ cdecl iswascii(long) MSVCRT_iswascii
@ cdecl iswblank(long) MSVCRT_iswblank
@ cdecl iswcntrl(long) MSVCRT_iswcntrl
@ cdecl iswctype(long long) MSVCRT_iswctype
@ cdecl iswdigit(long) MSVCRT_iswdigit
@ cdecl iswgraph(long) MSVCRT_iswgraph
@ cdecl iswlower(long) MSVCRT_iswlower
@ cdecl iswprint(long) MSVCRT_iswprint
@ cdecl iswpunct(long) MSVCRT_iswpunct
@ cdecl iswspace(long) MSVCRT_iswspace
@ cdecl iswupper(long) MSVCRT_iswupper
@ cdecl iswxdigit(long) MSVCRT_iswxdigit
@ cdecl isxdigit(long) MSVCRT_isxdigit
@ cdecl labs(long) MSVCRT_labs
@ cdecl ldexp(double long) MSVCRT_ldexp
@ cdecl -ret64 ldiv(long long) MSVCRT_ldiv
@ cdecl lgamma(double) MSVCR120_lgamma
@ cdecl lgammaf(float) MSVCR120_lgammaf
@ cdecl lgammal(double) MSVCR120_lgammal
@ cdecl -ret64 llabs(int64) MSVCRT_llabs
@ cdecl lldiv(int64 int64) MSVCRT_lldiv
@ cdecl -ret64 llrint(double) MSVCR120_llrint
@ cdecl -ret64 llrintf(float) MSVCR120_llrintf
@ cdecl -ret64 llrintl(double) MSVCR120_llrintl
@ cdecl -ret64 llround(double) MSVCR120_llround
@ cdecl -ret64 llroundf(float) MSVCR120_llroundf
@ cdecl -ret64 llroundl(double) MSVCR120_llroundl
@ cdecl localeconv() MSVCRT_localeconv
@ cdecl log(double) MSVCRT_log
@ cdecl log10(double) MSVCRT_log10
@ cdecl -arch=!i386 log10f(float) MSVCRT_log10f
@ cdecl log1p(double) MSVCR120_log1p
@ cdecl log1pf(float) MSVCR120_log1pf
@ cdecl log1pl(double) MSVCR120_log1pl
@ cdecl log2(double) MSVCR120_log2
@ cdecl log2f(float) MSVCR120_log2f
@ cdecl log2l(double) MSVCR120_log2l
@ cdecl logb(double) MSVCRT__logb
@ cdecl logbf(float) MSVCRT__logbf
@ cdecl logbl(double) MSVCRT__logb
@ cdecl -arch=!i386 logf(float) MSVCRT_logf
@ cdecl -arch=i386,x86_64,arm,arm64 longjmp(ptr long) MSVCRT_longjmp
@ cdecl lrint(double) MSVCR120_lrint
@ cdecl lrintf(float) MSVCR120_lrintf
@ cdecl lrintl(double) MSVCR120_lrintl
@ cdecl lround(double) MSVCR120_lround
@ cdecl lroundf(float) MSVCR120_lroundf
@ cdecl lroundl(double) MSVCR120_lroundl
@ cdecl malloc(long) MSVCRT_malloc
@ cdecl mblen(ptr long) MSVCRT_mblen
@ cdecl mbrlen(ptr long ptr) MSVCRT_mbrlen
@ stub mbrtoc16
@ stub mbrtoc32
@ cdecl mbrtowc(ptr str long ptr) MSVCRT_mbrtowc
@ cdecl mbsrtowcs(ptr ptr long ptr) MSVCRT_mbsrtowcs
@ cdecl mbsrtowcs_s(ptr ptr long ptr long ptr) MSVCRT_mbsrtowcs_s
@ cdecl mbstowcs(ptr str long) MSVCRT_mbstowcs
@ cdecl mbstowcs_s(ptr ptr long str long) MSVCRT__mbstowcs_s
@ cdecl mbtowc(ptr str long) MSVCRT_mbtowc
@ cdecl memchr(ptr long long) MSVCRT_memchr
@ cdecl memcmp(ptr ptr long)
@ cdecl memcpy(ptr ptr long)
@ cdecl memcpy_s(ptr long ptr long) MSVCRT_memcpy_s
@ cdecl memmove(ptr ptr long)
@ cdecl memmove_s(ptr long ptr long) MSVCRT_memmove_s
@ cdecl memset(ptr long long)
@ cdecl modf(double ptr) MSVCRT_modf
@ cdecl -arch=!i386 modff(float ptr) MSVCRT_modff
@ cdecl nan(str) MSVCR120_nan
@ cdecl nanf(str) MSVCR120_nanf
@ cdecl nanl(str) MSVCR120_nan
@ cdecl nearbyint(double) MSVCRT_nearbyint
@ cdecl nearbyintf(float) MSVCRT_nearbyintf
@ cdecl nearbyintl(double) MSVCRT_nearbyint
@ cdecl nextafter(double double) MSVCRT__nextafter
@ cdecl nextafterf(float float) MSVCRT__nextafterf
@ cdecl nextafterl(double double) MSVCRT__nextafter
@ cdecl nexttoward(double double) MSVCRT_nexttoward
@ cdecl nexttowardf(float double) MSVCRT_nexttowardf
@ cdecl nexttowardl(double double) MSVCRT_nexttoward
@ stub norm
@ stub normf
@ stub norml
@ cdecl perror(str) MSVCRT_perror
@ cdecl pow(double double) MSVCRT_pow
@ cdecl -arch=!i386 powf(float float) MSVCRT_powf
@ cdecl putc(long ptr) MSVCRT_putc
@ cdecl putchar(long) MSVCRT_putchar
@ cdecl puts(str) MSVCRT_puts
@ cdecl putwc(long ptr) MSVCRT_fputwc
@ cdecl putwchar(long) MSVCRT__fputwchar
@ cdecl qsort(ptr long long ptr) MSVCRT_qsort
@ cdecl qsort_s(ptr long long ptr ptr) MSVCRT_qsort_s
@ cdecl quick_exit(long) MSVCRT_quick_exit
@ cdecl raise(long) MSVCRT_raise
@ cdecl rand() MSVCRT_rand
@ cdecl rand_s(ptr) MSVCRT_rand_s
@ cdecl realloc(ptr long) MSVCRT_realloc
@ cdecl remainder(double double) MSVCR120_remainder
@ cdecl remainderf(float float) MSVCR120_remainderf
@ cdecl remainderl(double double) MSVCR120_remainderl
@ cdecl remove(str) MSVCRT_remove
@ cdecl remquo(double double ptr) MSVCR120_remquo
@ cdecl remquof(float float ptr) MSVCR120_remquof
@ cdecl remquol(double double ptr) MSVCR120_remquol
@ cdecl rename(str str) MSVCRT_rename
@ cdecl rewind(ptr) MSVCRT_rewind
@ cdecl rint(double) MSVCR120_rint
@ cdecl rintf(float) MSVCR120_rintf
@ cdecl rintl(double) MSVCR120_rintl
@ cdecl round(double) MSVCR120_round
@ cdecl roundf(float) MSVCR120_roundf
@ cdecl roundl(double) MSVCR120_roundl
@ cdecl scalbln(double long) MSVCRT__scalb
@ cdecl scalblnf(float long) MSVCRT__scalbf
@ cdecl scalblnl(double long) MSVCR120_scalbnl
@ cdecl scalbn(double long) MSVCRT__scalb
@ cdecl scalbnf(float long) MSVCRT__scalbf
@ cdecl scalbnl(double long) MSVCR120_scalbnl
@ cdecl set_terminate(ptr) MSVCRT_set_terminate
@ cdecl set_unexpected(ptr) MSVCRT_set_unexpected
@ cdecl setbuf(ptr ptr) MSVCRT_setbuf
@ cdecl -arch=arm,x86_64 -norelay -private setjmp(ptr) MSVCRT__setjmp
@ cdecl setlocale(long str) MSVCRT_setlocale
@ cdecl setvbuf(ptr str long long) MSVCRT_setvbuf
@ cdecl signal(long long) MSVCRT_signal
@ cdecl sin(double) MSVCRT_sin
@ cdecl -arch=!i386 sinf(float) MSVCRT_sinf
@ cdecl sinh(double) MSVCRT_sinh
@ cdecl -arch=!i386 sinhf(float) MSVCRT_sinhf
@ cdecl sqrt(double) MSVCRT_sqrt
@ cdecl -arch=!i386 sqrtf(float) MSVCRT_sqrtf
@ cdecl srand(long) MSVCRT_srand
@ cdecl strcat(str str)
@ cdecl strcat_s(str long str) MSVCRT_strcat_s
@ cdecl strchr(str long)
@ cdecl strcmp(str str)
@ cdecl strcoll(str str) MSVCRT_strcoll
@ cdecl strcpy(ptr str)
@ cdecl strcpy_s(ptr long str) MSVCRT_strcpy_s
@ cdecl strcspn(str str) MSVCRT_strcspn
@ cdecl strerror(long) MSVCRT_strerror
@ cdecl strerror_s(ptr long long) MSVCRT_strerror_s
@ cdecl strftime(ptr long str ptr) MSVCRT_strftime
@ cdecl strlen(str)
@ cdecl strncat(str str long) MSVCRT_strncat
@ cdecl strncat_s(str long str long) MSVCRT_strncat_s
@ cdecl strncmp(str str long) MSVCRT_strncmp
@ cdecl strncpy(ptr str long) MSVCRT_strncpy
@ cdecl strncpy_s(ptr long str long) MSVCRT_strncpy_s
@ cdecl strnlen(str long) MSVCRT_strnlen
@ cdecl strpbrk(str str) MSVCRT_strpbrk
@ cdecl strrchr(str long) MSVCRT_strrchr
@ cdecl strspn(str str) ntdll.strspn
@ cdecl strstr(str str) MSVCRT_strstr
@ cdecl strtod(str ptr) MSVCRT_strtod
@ cdecl strtof(str ptr) MSVCRT_strtof
@ cdecl -ret64 strtoimax(str ptr long) MSVCRT_strtoi64
@ cdecl strtok(str str) MSVCRT_strtok
@ cdecl strtok_s(ptr str ptr) MSVCRT_strtok_s
@ cdecl strtol(str ptr long) MSVCRT_strtol
@ cdecl strtold(str ptr) MSVCRT_strtod
@ cdecl -ret64 strtoll(str ptr long) MSVCRT_strtoi64
@ cdecl strtoul(str ptr long) MSVCRT_strtoul
@ cdecl -ret64 strtoull(str ptr long) MSVCRT_strtoui64
@ cdecl -ret64 strtoumax(str ptr long) MSVCRT_strtoui64
@ cdecl strxfrm(ptr str long) MSVCRT_strxfrm
@ cdecl system(str) MSVCRT_system
@ cdecl tan(double) MSVCRT_tan
@ cdecl -arch=!i386 tanf(float) MSVCRT_tanf
@ cdecl tanh(double) MSVCRT_tanh
@ cdecl -arch=!i386 tanhf(float) MSVCRT_tanhf
@ cdecl terminate() MSVCRT_terminate
@ cdecl tgamma(double) MSVCR120_tgamma
@ cdecl tgammaf(float) MSVCR120_tgammaf
@ cdecl tgammal(double) MSVCR120_tgamma
@ cdecl tmpfile() MSVCRT_tmpfile
@ cdecl tmpfile_s(ptr) MSVCRT_tmpfile_s
@ cdecl tmpnam(ptr) MSVCRT_tmpnam
@ cdecl tmpnam_s(ptr long) MSVCRT_tmpnam_s
@ cdecl tolower(long) MSVCRT_tolower
@ cdecl toupper(long) MSVCRT_toupper
@ cdecl towctrans(long long) MSVCR120_towctrans
@ cdecl towlower(long) MSVCRT_towlower
@ cdecl towupper(long) MSVCRT_towupper
@ cdecl trunc(double) MSVCR120_trunc
@ cdecl truncf(float) MSVCR120_truncf
@ cdecl truncl(double) MSVCR120_truncl
@ stub unexpected
@ cdecl ungetc(long ptr) MSVCRT_ungetc
@ cdecl ungetwc(long ptr) MSVCRT_ungetwc
@ cdecl wcrtomb(ptr long ptr) MSVCRT_wcrtomb
@ cdecl wcrtomb_s(ptr ptr long long ptr) MSVCRT_wcrtomb_s
@ cdecl wcscat(wstr wstr) MSVCRT_wcscat
@ cdecl wcscat_s(wstr long wstr) MSVCRT_wcscat_s
@ cdecl wcschr(wstr long) MSVCRT_wcschr
@ cdecl wcscmp(wstr wstr) MSVCRT_wcscmp
@ cdecl wcscoll(wstr wstr) MSVCRT_wcscoll
@ cdecl wcscpy(ptr wstr) MSVCRT_wcscpy
@ cdecl wcscpy_s(ptr long wstr) MSVCRT_wcscpy_s
@ cdecl wcscspn(wstr wstr) ntdll.wcscspn
@ cdecl wcsftime(ptr long wstr ptr) MSVCRT_wcsftime
@ cdecl wcslen(wstr) MSVCRT_wcslen
@ cdecl wcsncat(wstr wstr long) ntdll.wcsncat
@ cdecl wcsncat_s(wstr long wstr long) MSVCRT_wcsncat_s
@ cdecl wcsncmp(wstr wstr long) MSVCRT_wcsncmp
@ cdecl wcsncpy(ptr wstr long) MSVCRT_wcsncpy
@ cdecl wcsncpy_s(ptr long wstr long) MSVCRT_wcsncpy_s
@ cdecl wcsnlen(wstr long) MSVCRT_wcsnlen
@ cdecl wcspbrk(wstr wstr) MSVCRT_wcspbrk
@ cdecl wcsrchr(wstr long) MSVCRT_wcsrchr
@ cdecl wcsrtombs(ptr ptr long ptr) MSVCRT_wcsrtombs
@ cdecl wcsrtombs_s(ptr ptr long ptr long ptr) MSVCRT_wcsrtombs_s
@ cdecl wcsspn(wstr wstr) ntdll.wcsspn
@ cdecl wcsstr(wstr wstr) MSVCRT_wcsstr
@ cdecl wcstod(wstr ptr) MSVCRT_wcstod
@ cdecl wcstof(ptr ptr) MSVCRT_wcstof
@ stub wcstoimax
@ cdecl wcstok(wstr wstr ptr) MSVCRT_wcstok
@ cdecl wcstok_s(ptr wstr ptr) MSVCRT_wcstok_s
@ cdecl wcstol(wstr ptr long) MSVCRT_wcstol
@ cdecl wcstold(wstr ptr) MSVCRT_wcstod
@ cdecl -ret64 wcstoll(wstr ptr long) MSVCRT__wcstoi64
@ cdecl wcstombs(ptr ptr long) MSVCRT_wcstombs
@ cdecl wcstombs_s(ptr ptr long wstr long) MSVCRT_wcstombs_s
@ cdecl wcstoul(wstr ptr long) MSVCRT_wcstoul
@ cdecl -ret64 wcstoull(wstr ptr long) MSVCRT__wcstoui64
@ stub wcstoumax
@ cdecl wcsxfrm(ptr wstr long) MSVCRT_wcsxfrm
@ cdecl wctob(long) MSVCRT_wctob
@ cdecl wctomb(ptr long) MSVCRT_wctomb
@ cdecl wctomb_s(ptr ptr long long) MSVCRT_wctomb_s
@ cdecl wctrans(str) MSVCR120_wctrans
@ cdecl wctype(str)
@ cdecl wmemcpy_s(ptr long ptr long)
@ cdecl wmemmove_s(ptr long ptr long)
