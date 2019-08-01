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
@ stdcall -arch=x86_64 __C_specific_handler(ptr long ptr ptr) ntdll.__C_specific_handler
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
@ cdecl __stdio_common_vsprintf(int64 ptr long str ptr ptr)
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
@ cdecl _assert(str str long) MSVCRT__assert
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
@ stub _atoldbl_l
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
@ cdecl -arch=arm,x86_64,arm64 _finitef(float) MSVCRT__finitef
@ cdecl _flushall() MSVCRT__flushall
@ cdecl _fpclass(double) MSVCRT__fpclass
@ stub _fpclassf
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
@ cdecl _gcvt_s(ptr long  double long) MSVCRT__gcvt_s
@ stub _get_FMA3_enable
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
@ stub _ismbbkana_l
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
@ stub _ismbcalnum_l
@ cdecl _ismbcalpha(long)
@ stub _ismbcalpha_l
@ stub _ismbcblank
@ stub _ismbcblank_l
@ cdecl _ismbcdigit(long)
@ stub _ismbcdigit_l
@ cdecl _ismbcgraph(long)
@ stub _ismbcgraph_l
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
@ stub _ismbclower_l
@ cdecl _ismbcprint(long)
@ stub _ismbcprint_l
@ cdecl _ismbcpunct(long)
@ stub _ismbcpunct_l
@ cdecl _ismbcspace(long)
@ stub _ismbcspace_l
@ cdecl _ismbcsymbol(long)
@ stub _ismbcsymbol_l
@ cdecl _ismbcupper(long)
@ stub _ismbcupper_l
@ cdecl _ismbslead(ptr ptr)
@ stub _ismbslead_l
@ cdecl _ismbstrail(ptr ptr)
@ stub _ismbstrail_l
@ cdecl _isnan(double) MSVCRT__isnan
@ cdecl -arch=x86_64 _isnanf(float) MSVCRT__isnanf
@ cdecl _isprint_l(long ptr) MSVCRT__isprint_l
@ stub _ispunct_l
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
@ cdecl -arch=x86_64 _local_unwind(ptr ptr)
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
@ cdecl -arch=arm,x86_64,arm64 _logbf(float) MSVCRT__logbf
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
@ stub _mbbtype_l
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
@ stub _mbscmp_l
@ cdecl _mbscoll(str str)
@ cdecl _mbscoll_l(str str ptr)
@ cdecl _mbscpy_s(ptr long str)
@ cdecl _mbscpy_s_l(ptr long str ptr)
@ cdecl _mbscspn(str str)
@ stub _mbscspn_l
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
@ stub _mbsnextc_l
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
@ stub _mbsspn_l
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
@ cdecl _o___acrt_iob_func(long) MSVCRT___acrt_iob_func
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
@ cdecl _o___p___argc() MSVCRT___p___argc
@ stub _o___p___argv
@ cdecl _o___p___wargv() MSVCRT___p___wargv
@ stub _o___p__acmdln
@ cdecl _o___p__commode() __p__commode
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
@ cdecl _o___std_exception_destroy(ptr) MSVCRT___std_exception_destroy
@ cdecl _o___std_type_info_destroy_list(ptr) MSVCRT_type_info_destroy_list
@ stub _o___std_type_info_name
@ cdecl _o___stdio_common_vfprintf(int64 ptr str ptr ptr) MSVCRT__stdio_common_vfprintf
@ stub _o___stdio_common_vfprintf_p
@ stub _o___stdio_common_vfprintf_s
@ stub _o___stdio_common_vfscanf
@ stub _o___stdio_common_vfwprintf
@ stub _o___stdio_common_vfwprintf_p
@ stub _o___stdio_common_vfwprintf_s
@ stub _o___stdio_common_vfwscanf
@ cdecl _o___stdio_common_vsnprintf_s(int64 ptr long long str ptr ptr) MSVCRT__stdio_common_vsnprintf_s
@ stub _o___stdio_common_vsnwprintf_s
@ stub _o___stdio_common_vsprintf
@ stub _o___stdio_common_vsprintf_p
@ cdecl _o___stdio_common_vsprintf_s(int64 ptr long str ptr ptr) MSVCRT__stdio_common_vsprintf_s
@ stub _o___stdio_common_vsscanf
@ stub _o___stdio_common_vswprintf
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
@ cdecl _o__configthreadlocale(long) _configthreadlocale
@ stub _o__configure_narrow_argv
@ cdecl _o__configure_wide_argv(long) _configure_wide_argv
@ cdecl _o__controlfp_s(ptr long long) _controlfp_s
@ stub _o__cputs
@ stub _o__cputws
@ stub _o__creat
@ stub _o__create_locale
@ cdecl _o__crt_atexit(ptr) MSVCRT__crt_atexit
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
@ cdecl _o__errno() MSVCRT__errno
@ stub _o__except1
@ cdecl _o__execute_onexit_table(ptr) MSVCRT__execute_onexit_table
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
@ stub _o__fileno
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
@ cdecl _o__fpclass(double) MSVCRT__fpclass
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
@ cdecl _o__get_initial_wide_environment() _get_initial_wide_environment
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
@ cdecl _o__initialize_onexit_table(ptr) MSVCRT__initialize_onexit_table
@ cdecl _o__initialize_wide_environment() _initialize_wide_environment
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
@ stub _o__mbstrlen
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
@ cdecl _o__register_onexit_function(ptr ptr) MSVCRT__register_onexit_function
@ stub _o__resetstkoflw
@ stub _o__rmdir
@ stub _o__rmtmp
@ stub _o__scalb
@ stub _o__scalbf
@ stub _o__searchenv
@ stub _o__searchenv_s
@ cdecl _o__seh_filter_dll(long ptr) __CppXcptFilter
@ cdecl _o__seh_filter_exe(long ptr) _XcptFilter
@ stub _o__set_abort_behavior
@ cdecl _o__set_app_type(long) MSVCRT___set_app_type
@ stub _o__set_doserrno
@ stub _o__set_errno
@ cdecl _o__set_fmode(long) MSVCRT__set_fmode
@ stub _o__set_invalid_parameter_handler
@ stub _o__set_new_handler
@ cdecl _o__set_new_mode(long) MSVCRT__set_new_mode
@ stub _o__set_thread_local_invalid_parameter_handler
@ stub _o__seterrormode
@ stub _o__setmbcp
@ stub _o__setmode
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
@ stub _o__strdup
@ stub _o__strerror
@ stub _o__strerror_s
@ stub _o__strftime_l
@ cdecl _o__stricmp(str str) MSVCRT__stricmp
@ stub _o__stricmp_l
@ stub _o__stricoll
@ stub _o__stricoll_l
@ stub _o__strlwr
@ stub _o__strlwr_l
@ stub _o__strlwr_s
@ stub _o__strlwr_s_l
@ stub _o__strncoll
@ stub _o__strncoll_l
@ stub _o__strnicmp
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
@ stub _o__wfopen
@ stub _o__wfopen_s
@ stub _o__wfreopen
@ stub _o__wfreopen_s
@ stub _o__wfsopen
@ stub _o__wfullpath
@ stub _o__wgetcwd
@ stub _o__wgetdcwd
@ stub _o__wgetenv
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
@ stub _o__wsetlocale
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
@ stub _o_abort
@ stub _o_acos
@ stub _o_acosf
@ stub _o_acosh
@ stub _o_acoshf
@ stub _o_acoshl
@ stub _o_asctime
@ stub _o_asctime_s
@ stub _o_asin
@ stub _o_asinf
@ stub _o_asinh
@ stub _o_asinhf
@ stub _o_asinhl
@ stub _o_atan
@ stub _o_atan2
@ stub _o_atan2f
@ stub _o_atanf
@ stub _o_atanh
@ stub _o_atanhf
@ stub _o_atanhl
@ stub _o_atof
@ stub _o_atoi
@ stub _o_atol
@ stub _o_atoll
@ stub _o_bsearch
@ stub _o_bsearch_s
@ stub _o_btowc
@ cdecl _o_calloc(long long) MSVCRT_calloc
@ stub _o_cbrt
@ stub _o_cbrtf
@ stub _o_ceil
@ stub _o_ceilf
@ stub _o_clearerr
@ stub _o_clearerr_s
@ stub _o_cos
@ stub _o_cosf
@ stub _o_cosh
@ stub _o_coshf
@ stub _o_erf
@ stub _o_erfc
@ stub _o_erfcf
@ stub _o_erfcl
@ stub _o_erff
@ stub _o_erfl
@ cdecl _o_exit(long) MSVCRT_exit
@ stub _o_exp
@ stub _o_exp2
@ stub _o_exp2f
@ stub _o_exp2l
@ stub _o_expf
@ stub _o_fabs
@ stub _o_fclose
@ stub _o_feof
@ stub _o_ferror
@ stub _o_fflush
@ stub _o_fgetc
@ stub _o_fgetpos
@ stub _o_fgets
@ stub _o_fgetwc
@ stub _o_fgetws
@ stub _o_floor
@ stub _o_floorf
@ stub _o_fma
@ stub _o_fmaf
@ stub _o_fmal
@ stub _o_fmod
@ stub _o_fmodf
@ stub _o_fopen
@ stub _o_fopen_s
@ stub _o_fputc
@ stub _o_fputs
@ stub _o_fputwc
@ stub _o_fputws
@ stub _o_fread
@ stub _o_fread_s
@ cdecl _o_free(ptr) MSVCRT_free
@ stub _o_freopen
@ stub _o_freopen_s
@ stub _o_frexp
@ stub _o_fseek
@ stub _o_fsetpos
@ stub _o_ftell
@ stub _o_fwrite
@ stub _o_getc
@ stub _o_getchar
@ stub _o_getenv
@ stub _o_getenv_s
@ stub _o_gets
@ stub _o_gets_s
@ stub _o_getwc
@ stub _o_getwchar
@ stub _o_hypot
@ stub _o_is_wctype
@ cdecl _o_isalnum(long) MSVCRT_isalnum
@ cdecl _o_isalpha(long) MSVCRT_isalpha
@ stub _o_isblank
@ stub _o_iscntrl
@ cdecl _o_isdigit(long) MSVCRT_isdigit
@ stub _o_isgraph
@ stub _o_isleadbyte
@ stub _o_islower
@ cdecl _o_isprint(long) MSVCRT_isprint
@ stub _o_ispunct
@ stub _o_isspace
@ stub _o_isupper
@ stub _o_iswalnum
@ stub _o_iswalpha
@ stub _o_iswascii
@ stub _o_iswblank
@ stub _o_iswcntrl
@ stub _o_iswctype
@ stub _o_iswdigit
@ stub _o_iswgraph
@ stub _o_iswlower
@ stub _o_iswprint
@ stub _o_iswpunct
@ stub _o_iswspace
@ stub _o_iswupper
@ stub _o_iswxdigit
@ stub _o_isxdigit
@ stub _o_ldexp
@ stub _o_lgamma
@ stub _o_lgammaf
@ stub _o_lgammal
@ stub _o_llrint
@ stub _o_llrintf
@ stub _o_llrintl
@ stub _o_llround
@ stub _o_llroundf
@ stub _o_llroundl
@ stub _o_localeconv
@ stub _o_log
@ stub _o_log10
@ stub _o_log10f
@ stub _o_log1p
@ stub _o_log1pf
@ stub _o_log1pl
@ stub _o_log2
@ stub _o_log2f
@ stub _o_log2l
@ stub _o_logb
@ stub _o_logbf
@ stub _o_logbl
@ stub _o_logf
@ stub _o_lrint
@ stub _o_lrintf
@ stub _o_lrintl
@ stub _o_lround
@ stub _o_lroundf
@ stub _o_lroundl
@ cdecl _o_malloc(long) MSVCRT_malloc
@ stub _o_mblen
@ stub _o_mbrlen
@ stub _o_mbrtoc16
@ stub _o_mbrtoc32
@ stub _o_mbrtowc
@ stub _o_mbsrtowcs
@ stub _o_mbsrtowcs_s
@ stub _o_mbstowcs
@ stub _o_mbstowcs_s
@ stub _o_mbtowc
@ stub _o_memcpy_s
@ stub _o_memset
@ stub _o_modf
@ stub _o_modff
@ stub _o_nan
@ stub _o_nanf
@ stub _o_nanl
@ stub _o_nearbyint
@ stub _o_nearbyintf
@ stub _o_nearbyintl
@ stub _o_nextafter
@ stub _o_nextafterf
@ stub _o_nextafterl
@ stub _o_nexttoward
@ stub _o_nexttowardf
@ stub _o_nexttowardl
@ stub _o_pow
@ stub _o_powf
@ stub _o_putc
@ stub _o_putchar
@ stub _o_puts
@ stub _o_putwc
@ stub _o_putwchar
@ cdecl _o_qsort(ptr long long ptr) MSVCRT_qsort
@ stub _o_qsort_s
@ stub _o_raise
@ stub _o_rand
@ stub _o_rand_s
@ cdecl _o_realloc(ptr long) MSVCRT_realloc
@ stub _o_remainder
@ stub _o_remainderf
@ stub _o_remainderl
@ stub _o_remove
@ stub _o_remquo
@ stub _o_remquof
@ stub _o_remquol
@ stub _o_rename
@ stub _o_rewind
@ stub _o_rint
@ stub _o_rintf
@ stub _o_rintl
@ stub _o_round
@ stub _o_roundf
@ stub _o_roundl
@ stub _o_scalbln
@ stub _o_scalblnf
@ stub _o_scalblnl
@ stub _o_scalbn
@ stub _o_scalbnf
@ stub _o_scalbnl
@ stub _o_set_terminate
@ stub _o_setbuf
@ stub _o_setlocale
@ stub _o_setvbuf
@ stub _o_sin
@ stub _o_sinf
@ stub _o_sinh
@ stub _o_sinhf
@ stub _o_sqrt
@ stub _o_sqrtf
@ stub _o_srand
@ cdecl _o_strcat_s(str long str) MSVCRT_strcat_s
@ stub _o_strcoll
@ stub _o_strcpy_s
@ stub _o_strerror
@ stub _o_strerror_s
@ stub _o_strftime
@ stub _o_strncat_s
@ stub _o_strncpy_s
@ stub _o_strtod
@ stub _o_strtof
@ stub _o_strtok
@ stub _o_strtok_s
@ stub _o_strtol
@ stub _o_strtold
@ stub _o_strtoll
@ cdecl _o_strtoul(str ptr long) MSVCRT_strtoul
@ stub _o_strtoull
@ stub _o_system
@ stub _o_tan
@ stub _o_tanf
@ stub _o_tanh
@ stub _o_tanhf
@ stub _o_terminate
@ stub _o_tgamma
@ stub _o_tgammaf
@ stub _o_tgammal
@ stub _o_tmpfile_s
@ stub _o_tmpnam_s
@ cdecl _o_tolower(long) MSVCRT__tolower
@ cdecl _o_toupper(long) MSVCRT__toupper
@ stub _o_towlower
@ stub _o_towupper
@ stub _o_ungetc
@ stub _o_ungetwc
@ stub _o_wcrtomb
@ stub _o_wcrtomb_s
@ stub _o_wcscat_s
@ stub _o_wcscoll
@ stub _o_wcscpy
@ stub _o_wcscpy_s
@ stub _o_wcsftime
@ stub _o_wcsncat_s
@ stub _o_wcsncpy_s
@ stub _o_wcsrtombs
@ stub _o_wcsrtombs_s
@ stub _o_wcstod
@ stub _o_wcstof
@ stub _o_wcstok
@ stub _o_wcstok_s
@ stub _o_wcstol
@ stub _o_wcstold
@ stub _o_wcstoll
@ stub _o_wcstombs
@ stub _o_wcstombs_s
@ stub _o_wcstoul
@ stub _o_wcstoull
@ stub _o_wctob
@ stub _o_wctomb
@ stub _o_wctomb_s
@ stub _o_wmemcpy_s
@ stub _o_wmemmove_s
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
@ stub _query_new_handler
@ stub _query_new_mode
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
@ stub _strtoimax_l
@ cdecl _strtol_l(str ptr long ptr) MSVCRT__strtol_l
@ stub _strtold_l
@ cdecl -ret64 _strtoll_l(str ptr long ptr) MSVCRT_strtoi64_l
@ cdecl -ret64 _strtoui64(str ptr long) MSVCRT_strtoui64
@ cdecl -ret64 _strtoui64_l(str ptr long ptr) MSVCRT_strtoui64_l
@ cdecl _strtoul_l(str ptr long ptr) MSVCRT_strtoul_l
@ cdecl -ret64 _strtoull_l(str ptr long ptr) MSVCRT_strtoui64_l
@ stub _strtoumax_l
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
@ cdecl _wcstod_l(wstr ptr long) MSVCRT__wcstod_l
@ cdecl _wcstof_l(wstr ptr ptr) MSVCRT__wcstof_l
@ cdecl -ret64 _wcstoi64(wstr ptr long) MSVCRT__wcstoi64
@ cdecl -ret64 _wcstoi64_l(wstr ptr long ptr) MSVCRT__wcstoi64_l
@ stub _wcstoimax_l
@ cdecl _wcstol_l(wstr ptr long ptr) MSVCRT__wcstol_l
@ stub _wcstold_l
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
@ stub _wctype
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
@ cdecl -arch=arm,x86_64,arm64 acosf(float) MSVCRT_acosf
@ cdecl acosh(double) MSVCR120_acosh
@ cdecl acoshf(float) MSVCR120_acoshf
@ cdecl acoshl(double) MSVCR120_acoshl
@ cdecl asctime(ptr) MSVCRT_asctime
@ cdecl asctime_s(ptr long ptr) MSVCRT_asctime_s
@ cdecl asin(double) MSVCRT_asin
@ cdecl -arch=arm,x86_64,arm64 asinf(float) MSVCRT_asinf
@ cdecl asinh(double) MSVCR120_asinh
@ cdecl asinhf(float) MSVCR120_asinhf
@ cdecl asinhl(double) MSVCR120_asinhl
@ cdecl atan(double) MSVCRT_atan
@ cdecl atan2(double double) MSVCRT_atan2
@ cdecl -arch=arm,x86_64,arm64 atan2f(float float) MSVCRT_atan2f
@ cdecl -arch=arm,x86_64,arm64 atanf(float) MSVCRT_atanf
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
@ cdecl -arch=arm,x86_64,arm64 ceilf(float) MSVCRT_ceilf
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
@ cdecl -arch=arm,x86_64,arm64 cosf(float) MSVCRT_cosf
@ cdecl cosh(double) MSVCRT_cosh
@ cdecl -arch=arm,x86_64,arm64 coshf(float) MSVCRT_coshf
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
@ cdecl -arch=arm,x86_64,arm64 expf(float) MSVCRT_expf
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
@ cdecl -arch=arm,x86_64,arm64 floorf(float) MSVCRT_floorf
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
@ cdecl -arch=arm,x86_64,arm64 fmodf(float float) MSVCRT_fmodf
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
@ stub imaxabs
@ stub imaxdiv
@ cdecl is_wctype(long long) ntdll.iswctype
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
@ cdecl iswalpha(long) ntdll.iswalpha
@ cdecl iswascii(long) MSVCRT_iswascii
@ cdecl iswblank(long) MSVCRT_iswblank
@ cdecl iswcntrl(long) MSVCRT_iswcntrl
@ cdecl iswctype(long long) ntdll.iswctype
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
@ cdecl ldiv(long long) MSVCRT_ldiv
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
@ cdecl -arch=arm,x86_64,arm64 log10f(float) MSVCRT_log10f
@ cdecl log1p(double) MSVCR120_log1p
@ cdecl log1pf(float) MSVCR120_log1pf
@ cdecl log1pl(double) MSVCR120_log1pl
@ cdecl log2(double) MSVCR120_log2
@ cdecl log2f(float) MSVCR120_log2f
@ cdecl log2l(double) MSVCR120_log2l
@ cdecl logb(double) MSVCRT__logb
@ cdecl logbf(float) MSVCRT__logbf
@ cdecl logbl(double) MSVCRT__logb
@ cdecl -arch=arm,x86_64,arm64 logf(float) MSVCRT_logf
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
@ cdecl memcmp(ptr ptr long) MSVCRT_memcmp
@ cdecl memcpy(ptr ptr long) MSVCRT_memcpy
@ cdecl memcpy_s(ptr long ptr long) MSVCRT_memcpy_s
@ cdecl memmove(ptr ptr long) MSVCRT_memmove
@ cdecl memmove_s(ptr long ptr long) MSVCRT_memmove_s
@ cdecl memset(ptr long long) MSVCRT_memset
@ cdecl modf(double ptr) MSVCRT_modf
@ cdecl -arch=arm,x86_64,arm64 modff(float ptr) MSVCRT_modff
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
@ cdecl -arch=arm,x86_64,arm64 powf(float float) MSVCRT_powf
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
@ cdecl -arch=arm,x86_64,arm64 sinf(float) MSVCRT_sinf
@ cdecl sinh(double) MSVCRT_sinh
@ cdecl -arch=arm,x86_64,arm64 sinhf(float) MSVCRT_sinhf
@ cdecl sqrt(double) MSVCRT_sqrt
@ cdecl -arch=arm,x86_64,arm64 sqrtf(float) MSVCRT_sqrtf
@ cdecl srand(long) MSVCRT_srand
@ cdecl strcat(str str) ntdll.strcat
@ cdecl strcat_s(str long str) MSVCRT_strcat_s
@ cdecl strchr(str long) MSVCRT_strchr
@ cdecl strcmp(str str) MSVCRT_strcmp
@ cdecl strcoll(str str) MSVCRT_strcoll
@ cdecl strcpy(ptr str) MSVCRT_strcpy
@ cdecl strcpy_s(ptr long str) MSVCRT_strcpy_s
@ cdecl strcspn(str str) MSVCRT_strcspn
@ cdecl strerror(long) MSVCRT_strerror
@ cdecl strerror_s(ptr long long) MSVCRT_strerror_s
@ cdecl strftime(ptr long str ptr) MSVCRT_strftime
@ cdecl strlen(str) MSVCRT_strlen
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
@ stub strtoimax
@ cdecl strtok(str str) MSVCRT_strtok
@ cdecl strtok_s(ptr str ptr) MSVCRT_strtok_s
@ cdecl strtol(str ptr long) MSVCRT_strtol
@ stub strtold
@ cdecl -ret64 strtoll(str ptr long) MSVCRT_strtoi64
@ cdecl strtoul(str ptr long) MSVCRT_strtoul
@ cdecl -ret64 strtoull(str ptr long) MSVCRT_strtoui64
@ stub strtoumax
@ cdecl strxfrm(ptr str long) MSVCRT_strxfrm
@ cdecl system(str) MSVCRT_system
@ cdecl tan(double) MSVCRT_tan
@ cdecl -arch=arm,x86_64,arm64 tanf(float) MSVCRT_tanf
@ cdecl tanh(double) MSVCRT_tanh
@ cdecl -arch=arm,x86_64,arm64 tanhf(float) MSVCRT_tanhf
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
@ stub wcrtomb_s
@ cdecl wcscat(wstr wstr) ntdll.wcscat
@ cdecl wcscat_s(wstr long wstr) MSVCRT_wcscat_s
@ cdecl wcschr(wstr long) MSVCRT_wcschr
@ cdecl wcscmp(wstr wstr) MSVCRT_wcscmp
@ cdecl wcscoll(wstr wstr) MSVCRT_wcscoll
@ cdecl wcscpy(ptr wstr) ntdll.wcscpy
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
@ cdecl wcstok(wstr wstr) MSVCRT_wcstok
@ cdecl wcstok_s(ptr wstr ptr) MSVCRT_wcstok_s
@ cdecl wcstol(wstr ptr long) MSVCRT_wcstol
@ stub wcstold
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
