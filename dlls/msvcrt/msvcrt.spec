# msvcrt.dll - MS VC++ Run Time Library

@ cdecl $I10_OUTPUT() MSVCRT_I10_OUTPUT
@ cdecl ??0__non_rtti_object@@QAE@ABV0@@Z(ptr ptr) MSVCRT___non_rtti_object_copy_ctor
@ cdecl ??0__non_rtti_object@@QAE@PBD@Z(ptr ptr) MSVCRT___non_rtti_object_ctor
@ cdecl ??0bad_cast@@QAE@ABQBD@Z(ptr ptr) MSVCRT_bad_cast_ctor
@ cdecl ??0bad_cast@@QAE@ABV0@@Z(ptr ptr) MSVCRT_bad_cast_copy_ctor
@ cdecl ??0bad_typeid@@QAE@ABV0@@Z(ptr ptr) MSVCRT_bad_typeid_copy_ctor
@ cdecl ??0bad_typeid@@QAE@PBD@Z(ptr ptr) MSVCRT_bad_typeid_ctor
@ cdecl ??0exception@@QAE@ABQBD@Z(ptr ptr) MSVCRT_exception_ctor
@ cdecl ??0exception@@QAE@ABV0@@Z(ptr ptr) MSVCRT_exception_copy_ctor
@ cdecl ??0exception@@QAE@XZ(ptr) MSVCRT_exception_default_ctor
@ cdecl ??1__non_rtti_object@@UAE@XZ(ptr) MSVCRT___non_rtti_object_dtor
@ cdecl ??1bad_cast@@UAE@XZ(ptr) MSVCRT_bad_cast_dtor
@ cdecl ??1bad_typeid@@UAE@XZ(ptr) MSVCRT_bad_typeid_dtor
@ cdecl ??1exception@@UAE@XZ(ptr) MSVCRT_exception_dtor
@ cdecl ??1type_info@@UAE@XZ(ptr) MSVCRT_type_info_dtor
@ cdecl ??2@YAPAXI@Z(long) MSVCRT_operator_new
@ cdecl ??_U@YAPAXI@Z(long) MSVCRT_operator_new
@ cdecl ??3@YAXPAX@Z(ptr) MSVCRT_operator_delete
@ cdecl ??_V@YAXPAX@Z(ptr) MSVCRT_operator_delete
@ cdecl ??4__non_rtti_object@@QAEAAV0@ABV0@@Z(ptr ptr) MSVCRT___non_rtti_object_opequals
@ cdecl ??4bad_cast@@QAEAAV0@ABV0@@Z(ptr ptr) MSVCRT_bad_cast_opequals
@ cdecl ??4bad_typeid@@QAEAAV0@ABV0@@Z(ptr ptr)MSVCRT_bad_typeid_opequals
@ cdecl ??4exception@@QAEAAV0@ABV0@@Z(ptr ptr) MSVCRT_exception_opequals
@ stdcall ??8type_info@@QBEHABV0@@Z(ptr ptr) MSVCRT_type_info_opequals_equals
@ stdcall ??9type_info@@QBEHABV0@@Z(ptr ptr) MSVCRT_type_info_opnot_equals
@ stub ??_7__non_rtti_object@@6B@
@ stub ??_7bad_cast@@6B@
@ stub ??_7bad_typeid@@6B@
@ stub ??_7exception@@6B@
@ cdecl ??_E__non_rtti_object@@UAEPAXI@Z(ptr long) MSVCRT___non_rtti_object__unknown_E
@ stub ??_Ebad_cast@@UAEPAXI@Z #(ptr long)
@ stub ??_Ebad_typeid@@UAEPAXI@Z #(ptr long)
@ cdecl ??_Eexception@@UAEPAXI@Z(ptr long) MSVCRT_exception__unknown_E
@ cdecl ??_G__non_rtti_object@@UAEPAXI@Z(ptr long) MSVCRT___non_rtti_object__unknown_G
@ stub ??_Gbad_cast@@UAEPAXI@Z #(ptr long)
@ stub ??_Gbad_typeid@@UAEPAXI@Z #(ptr long)
@ cdecl ??_Gexception@@UAEPAXI@Z(ptr long) MSVCRT_exception__unknown_G
@ cdecl ?_query_new_handler@@YAP6AHI@ZXZ() MSVCRT__query_new_handler
@ cdecl ?_query_new_mode@@YAHXZ() MSVCRT__query_new_mode
@ cdecl ?_set_new_handler@@YAP6AHI@ZP6AHI@Z@Z(ptr) MSVCRT__set_new_handler
@ cdecl ?_set_new_mode@@YAHH@Z(long) MSVCRT__set_new_mode
@ cdecl ?_set_se_translator@@YAP6AXIPAU_EXCEPTION_POINTERS@@@ZP6AXI0@Z@Z(ptr) MSVCRT__set_se_translator
@ stub ?before@type_info@@QBEHABV1@@Z #(ptr ptr) stdcall
@ stdcall ?name@type_info@@QBEPBDXZ(ptr) MSVCRT_type_info_name
@ stdcall ?raw_name@type_info@@QBEPBDXZ(ptr) MSVCRT_type_info_raw_name
@ cdecl ?set_new_handler@@YAP6AXXZP6AXXZ@Z(ptr) MSVCRT__set_new_handler
@ cdecl ?set_terminate@@YAP6AXXZP6AXXZ@Z(ptr) MSVCRT_set_terminate
@ cdecl ?set_unexpected@@YAP6AXXZP6AXXZ@Z(ptr) MSVCRT_set_unexpected
@ cdecl ?terminate@@YAXXZ() MSVCRT_terminate
@ cdecl ?unexpected@@YAXXZ() MSVCRT_unexpected
@ cdecl ?what@exception@@UBEPBDXZ(ptr) MSVCRT_what_exception
@ cdecl _CIacos()
@ cdecl _CIasin()
@ cdecl _CIatan()
@ cdecl _CIatan2()
@ cdecl _CIcos()
@ cdecl _CIcosh()
@ cdecl _CIexp()
@ cdecl _CIfmod()
@ cdecl _CIlog()
@ cdecl _CIlog10()
@ cdecl _CIpow()
@ cdecl _CIsin()
@ cdecl _CIsinh()
@ cdecl _CIsqrt()
@ cdecl _CItan()
@ cdecl _CItanh()
@ cdecl _CxxThrowException(long long)
@ cdecl -i386 -norelay _EH_prolog()
@ cdecl _Getdays()
@ cdecl _Getmonths()
@ cdecl _Getnames()
@ extern _HUGE MSVCRT__HUGE
@ cdecl _Strftime(str long str ptr ptr)
@ cdecl _XcptFilter(long ptr)
@ cdecl -register -i386 __CxxFrameHandler(ptr ptr ptr ptr)
@ stub __CxxLongjmpUnwind #(ptr) stdcall
@ cdecl __RTCastToVoid(ptr) MSVCRT___RTCastToVoid
@ cdecl __RTDynamicCast(ptr long ptr ptr long) MSVCRT___RTDynamicCast
@ cdecl __RTtypeid(ptr) MSVCRT___RTtypeid
@ stub __STRINGTOLD #(ptr ptr str long)
@ extern __argc MSVCRT___argc
@ extern __argv MSVCRT___argv
@ stub __badioinfo
@ stub __crtCompareStringA
@ stub __crtGetLocaleInfoW
@ cdecl __crtLCMapStringA(long long str long ptr long long long)
@ cdecl __dllonexit(ptr ptr ptr)
@ cdecl __doserrno()
@ stub __fpecode #()
@ cdecl __getmainargs(ptr ptr ptr long ptr)
@ extern __initenv MSVCRT___initenv
@ cdecl __isascii(long) MSVCRT___isascii
@ cdecl __iscsym(long) MSVCRT___iscsym
@ cdecl __iscsymf(long) MSVCRT___iscsymf
@ stub __lc_codepage
@ stub __lc_collate
@ stub __lc_handle
@ cdecl __lconv_init()
@ extern __mb_cur_max MSVCRT___mb_cur_max
@ cdecl __p___argc()
@ cdecl __p___argv()
@ cdecl __p___initenv()
@ cdecl __p___mb_cur_max()
@ cdecl __p___wargv()
@ cdecl __p___winitenv()
@ cdecl __p__acmdln()
@ stub __p__amblksiz #()
@ cdecl __p__commode()
@ cdecl __p__daylight() MSVCRT___p__daylight
@ stub __p__dstbias #()
@ cdecl __p__environ()
@ stub __p__fileinfo #()
@ cdecl __p__fmode()
@ cdecl __p__iob()
@ stub __p__mbcasemap #()
@ cdecl __p__mbctype()
@ cdecl __p__osver()
@ cdecl __p__pctype()
@ cdecl __p__pgmptr()
@ stub __p__pwctype #()
@ cdecl __p__timezone()
@ stub __p__tzname #()
@ cdecl __p__wcmdln()
@ cdecl __p__wenviron()
@ cdecl __p__winmajor()
@ cdecl __p__winminor()
@ cdecl __p__winver()
@ stub __p__wpgmptr #()
@ stub __pioinfo #()
@ stub __pxcptinfoptrs #()
@ cdecl __set_app_type(long) MSVCRT___set_app_type
@ extern __setlc_active MSVCRT___setlc_active
@ cdecl __setusermatherr(ptr) MSVCRT___setusermatherr
@ cdecl __threadhandle() kernel32.GetCurrentThread
@ cdecl __threadid() kernel32.GetCurrentThreadId
@ cdecl __toascii(long) MSVCRT___toascii
@ cdecl __unDName(long str ptr ptr long) MSVCRT___unDName
@ cdecl __unDNameEx() MSVCRT___unDNameEx #FIXME
@ extern __unguarded_readlc_active MSVCRT___unguarded_readlc_active
@ extern __wargv MSVCRT___wargv
@ cdecl __wgetmainargs(ptr ptr ptr long ptr)
@ extern __winitenv MSVCRT___winitenv
@ cdecl _abnormal_termination()
@ cdecl _access(str long)
@ extern _acmdln MSVCRT__acmdln
@ cdecl _adj_fdiv_m16i()
@ cdecl _adj_fdiv_m32()
@ cdecl _adj_fdiv_m32i()
@ cdecl _adj_fdiv_m64()
@ cdecl _adj_fdiv_r()
@ cdecl _adj_fdivr_m16i()
@ cdecl _adj_fdivr_m32()
@ cdecl _adj_fdivr_m32i()
@ cdecl _adj_fdivr_m64()
@ cdecl _adj_fpatan()
@ cdecl _adj_fprem()
@ cdecl _adj_fprem1()
@ cdecl _adj_fptan()
@ cdecl _adjust_fdiv()
@ stub _aexit_rtn
@ cdecl _amsg_exit(long) MSVCRT__amsg_exit
@ cdecl _assert(str str long) MSVCRT__assert
@ stub _atodbl #(ptr str)
@ cdecl -ret64 _atoi64(str) ntdll._atoi64
@ stub _atoldbl #(ptr str)
@ cdecl _beep(long long)
@ cdecl _beginthread (ptr long ptr)
@ cdecl _beginthreadex (ptr long ptr ptr long ptr)
@ cdecl _c_exit() MSVCRT__c_exit
@ cdecl _cabs(long)
@ cdecl _callnewh(long)
@ cdecl _cexit() MSVCRT__cexit
@ cdecl _cgets(str)
@ cdecl _chdir(str)
@ cdecl _chdrive(long)
@ cdecl _chgsign( double )
@ cdecl -i386 _chkesp()
@ cdecl _chmod(str long)
@ cdecl _chsize (long long)
@ cdecl _clearfp()
@ cdecl _close(long)
@ cdecl _commit(long)
@ extern _commode MSVCRT__commode
@ cdecl _control87(long long)
@ cdecl _controlfp(long long)
@ cdecl _copysign( double double )
@ varargs _cprintf(str)
@ cdecl _cputs(str)
@ cdecl _creat(str long)
@ varargs _cscanf(str)
@ extern _ctype MSVCRT__ctype
@ cdecl _cwait(ptr long long)
@ extern _daylight MSVCRT___daylight
@ stub _dstbias
@ cdecl _dup (long)
@ cdecl _dup2 (long long)
@ cdecl _ecvt(double long ptr ptr)
@ cdecl _endthread ()
@ cdecl _endthreadex(long)
@ extern _environ MSVCRT__environ
@ cdecl _eof(long)
@ cdecl _errno() MSVCRT__errno
@ cdecl _except_handler2(ptr ptr ptr ptr)
@ cdecl _except_handler3(ptr ptr ptr ptr)
@ varargs _execl(str str)
@ stub _execle #(str str) varargs
@ varargs _execlp(str str)
@ stub _execlpe #(str str) varargs
@ cdecl _execv(str str)
@ cdecl _execve(str str str)
@ cdecl _execvp(str str)
@ cdecl _execvpe(str str str)
@ cdecl _exit(long) MSVCRT__exit
@ cdecl _expand(ptr long)
@ cdecl _fcloseall()
@ cdecl _fcvt(double long ptr ptr)
@ cdecl _fdopen(long str)
@ cdecl _fgetchar()
@ cdecl _fgetwchar()
@ cdecl _filbuf(ptr)
@ stub _fileinfo
@ cdecl _filelength(long)
@ stub _filelengthi64 #(long)
@ cdecl _fileno(ptr)
@ cdecl _findclose(long)
@ cdecl _findfirst(str ptr)
@ stub _findfirsti64 #(str ptr)
@ cdecl _findnext(long ptr)
@ stub _findnexti64 #(long ptr)
@ cdecl _finite( double )
@ cdecl _flsbuf(long ptr)
@ cdecl _flushall()
@ extern _fmode MSVCRT__fmode
@ cdecl _fpclass(double)
@ stub _fpieee_flt #(long ptr ptr)
@ cdecl _fpreset()
@ cdecl _fputchar(long)
@ cdecl _fputwchar(long)
@ cdecl _fsopen(str str long)
@ cdecl _fstat(long ptr) MSVCRT__fstat
@ cdecl _fstati64(long ptr)
@ cdecl _ftime(ptr)
@ cdecl _ftol() ntdll._ftol
@ cdecl _fullpath(ptr str long)
@ cdecl _futime(long ptr)
@ cdecl _gcvt(double long str)
@ cdecl _get_osfhandle(long)
@ stub _get_sbh_threshold #()
@ cdecl _getch()
@ cdecl _getche()
@ cdecl _getcwd(str long)
@ cdecl _getdcwd(long str long)
@ cdecl _getdiskfree(long ptr)
@ cdecl _getdllprocaddr(long str long)
@ cdecl _getdrive()
@ cdecl _getdrives() kernel32.GetLogicalDrives
@ stub _getmaxstdio #()
@ cdecl _getmbcp()
@ cdecl _getpid() kernel32.GetCurrentProcessId
@ stub _getsystime #(ptr)
@ cdecl _getw(ptr)
@ cdecl _getws(ptr) MSVCRT__getws
@ cdecl _global_unwind2(ptr)
@ cdecl _heapadd (ptr long)
@ cdecl _heapchk()
@ cdecl _heapmin()
@ cdecl _heapset(long)
@ stub _heapused #(ptr ptr)
@ cdecl _heapwalk(ptr)
@ cdecl _hypot(double double) hypot
@ cdecl _i64toa(long long ptr long) ntdll._i64toa
@ cdecl _i64tow(long long ptr long) ntdll._i64tow
@ cdecl _initterm(ptr ptr)
@ stub _inp #(long) -i386
@ stub _inpd #(long) -i386
@ stub _inpw #(long) -i386
@ extern _iob MSVCRT__iob
@ cdecl _isatty(long)
@ cdecl _isctype(long long)
@ stub _ismbbalnum #(long)
@ stub _ismbbalpha #(long)
@ stub _ismbbgraph #(long)
@ stub _ismbbkalnum #(long)
@ cdecl _ismbbkana(long)
@ stub _ismbbkprint #(long)
@ stub _ismbbkpunct #(long)
@ cdecl _ismbblead(long)
@ stub _ismbbprint #(long)
@ stub _ismbbpunct #(long)
@ cdecl _ismbbtrail(long)
@ cdecl _ismbcalnum(long)
@ cdecl _ismbcalpha(long)
@ cdecl _ismbcdigit(long)
@ cdecl _ismbcgraph(long)
@ cdecl _ismbchira(long)
@ cdecl _ismbckata(long)
@ stub _ismbcl0 #(long)
@ stub _ismbcl1 #(long)
@ stub _ismbcl2 #(long)
@ stub _ismbclegal #(long)
@ cdecl _ismbclower(long)
@ cdecl _ismbcprint(long)
@ cdecl _ismbcpunct(long)
@ cdecl _ismbcspace(long)
@ cdecl _ismbcsymbol(long)
@ cdecl _ismbcupper(long)
@ cdecl _ismbslead(ptr ptr)
@ cdecl _ismbstrail(ptr ptr)
@ cdecl _isnan( double )
@ cdecl _itoa(long ptr long) ntdll._itoa
@ cdecl _itow(long ptr long) ntdll._itow
@ cdecl _j0(double) j0
@ cdecl _j1(double) j1
@ cdecl _jn(long double) jn
@ cdecl _kbhit()
@ cdecl _lfind(ptr ptr ptr long ptr)
@ cdecl _loaddll(str)
@ cdecl _local_unwind2(ptr long)
@ cdecl _lock(long)
@ cdecl _locking(long long long)
@ cdecl _logb( double )
@ stub _longjmpex
@ cdecl _lrotl(long long)
@ cdecl _lrotr(long long)
@ cdecl _lsearch(ptr ptr long long ptr)
@ cdecl _lseek(long long long)
@ cdecl -ret64 _lseeki64(long long long long)
@ cdecl _ltoa(long ptr long) ntdll._ltoa
@ cdecl _ltow(long ptr long) ntdll._ltow
@ cdecl _makepath(str str str str str)
@ cdecl _matherr(ptr)
@ cdecl _mbbtombc(long)
@ stub _mbbtype #(long long)
@ stub _mbcasemap
@ cdecl _mbccpy (str str) strcpy
@ stub _mbcjistojms #(long)
@ stub _mbcjmstojis #(long)
@ cdecl _mbclen(ptr)
@ stub _mbctohira #(long)
@ stub _mbctokata #(long)
@ cdecl _mbctolower(long)
@ stub _mbctombb #(long)
@ cdecl _mbctoupper(long)
@ stub _mbctype
@ stub _mbsbtype #(str long)
@ cdecl _mbscat(str str) strcat
@ cdecl _mbschr(str long)
@ cdecl _mbscmp(str str)
@ stub _mbscoll #(str str)
@ cdecl _mbscpy(ptr str) strcpy
@ cdecl _mbscspn (str str)
@ cdecl _mbsdec(ptr ptr)
@ cdecl _mbsdup(str) _strdup
@ cdecl _mbsicmp(str str)
@ cdecl _mbsicoll(str str)
@ cdecl _mbsinc(str)
@ cdecl _mbslen(str)
@ cdecl _mbslwr(str)
@ cdecl _mbsnbcat (str str long)
@ cdecl _mbsnbcmp(str str long)
@ cdecl _mbsnbcnt(ptr long)
@ stub _mbsnbcoll #(str str long)
@ cdecl _mbsnbcpy(ptr str long)
@ cdecl _mbsnbicmp(str str long)
@ stub _mbsnbicoll #(str str long)
@ cdecl _mbsnbset(str long long)
@ cdecl _mbsncat(str str long)
@ cdecl _mbsnccnt(str long)
@ cdecl _mbsncmp(str str long)
@ stub _mbsncoll #(str str long)
@ cdecl _mbsncpy(str str long)
@ cdecl _mbsnextc(str)
@ cdecl _mbsnicmp(str str long)
@ stub _mbsnicoll #(str str long)
@ cdecl _mbsninc(str long)
@ cdecl _mbsnset(str long long)
@ cdecl _mbspbrk(str str)
@ cdecl _mbsrchr(str long)
@ cdecl _mbsrev(str)
@ cdecl _mbsset(str long)
@ cdecl _mbsspn(str str)
@ stub _mbsspnp #(str str)
@ cdecl _mbsstr(str str) strstr
@ cdecl _mbstok(str str)
@ cdecl _mbstrlen(str)
@ cdecl _mbsupr(str)
@ cdecl _memccpy(ptr ptr long long) memccpy
@ cdecl _memicmp(str str long) ntdll._memicmp
@ cdecl _mkdir(str)
@ cdecl _mktemp(str)
@ cdecl _msize(ptr)
@ cdecl _nextafter(double double)
@ cdecl _onexit(ptr)
@ varargs _open(str long)
@ cdecl _open_osfhandle(long long)
@ stub _osver
@ stub _outp #(long long)
@ stub _outpd #(long long)
@ stub _outpw #(long long)
@ stub _pclose #(ptr)
@ extern _pctype MSVCRT__pctype
@ extern _pgmptr MSVCRT__pgmptr
@ stub _pipe #(ptr long long)
@ stub _popen #(str str)
@ cdecl _purecall()
@ cdecl _putch(long)
@ cdecl _putenv(str)
@ cdecl _putw(long ptr)
@ cdecl _putws(wstr)
@ stub _pwctype
@ cdecl _read(long ptr long)
@ cdecl _rmdir(str)
@ cdecl _rmtmp()
@ cdecl _rotl(long long)
@ cdecl _rotr(long long)
@ cdecl _safe_fdiv()
@ cdecl _safe_fdivr()
@ cdecl _safe_fprem()
@ cdecl _safe_fprem1()
@ cdecl _scalb( double long)
@ cdecl _searchenv(str str ptr)
@ stdcall -i386 _seh_longjmp_unwind(ptr)
@ stub _set_error_mode #(long)
@ stub _set_sbh_threshold #(long)
@ stub _seterrormode #(long)
@ cdecl -register -i386 _setjmp(ptr) _MSVCRT__setjmp
@ cdecl -register -i386 _setjmp3(ptr long) _MSVCRT__setjmp3
@ stub _setmaxstdio #(long)
@ cdecl _setmbcp(long)
@ cdecl _setmode(long long)
@ stub _setsystime #(ptr long)
@ cdecl _sleep(long)
@ varargs _snprintf(str long str) snprintf
@ varargs _snwprintf(wstr long wstr) ntdll._snwprintf
@ varargs _sopen(str long long) MSVCRT__sopen
@ varargs _spawnl(long str str)
@ stub _spawnle #(long str str) varargs
@ varargs _spawnlp(long str str)
@ stub _spawnlpe #(long str str) varargs
@ cdecl _spawnv(long str ptr)
@ cdecl _spawnve(long str ptr ptr)
@ cdecl _spawnvp(long str ptr)
@ cdecl _spawnvpe(long str ptr ptr)
@ cdecl _splitpath(str ptr ptr ptr ptr) ntdll._splitpath
@ cdecl _stat(str ptr) MSVCRT__stat
@ cdecl _stati64(str ptr)
@ cdecl _statusfp()
@ cdecl _strcmpi(str str) strcasecmp
@ cdecl _strdate(ptr)
@ cdecl _strdup(str)
@ cdecl _strerror(long)
@ cdecl _stricmp(str str) strcasecmp
@ stub _stricoll #(str str)
@ cdecl _strlwr(str) ntdll._strlwr
@ stub _strncoll #(str str long)
@ cdecl _strnicmp(str str long) strncasecmp
@ stub _strnicoll #(str str long)
@ cdecl _strnset(str long long)
@ cdecl _strrev(str)
@ cdecl _strset(str long)
@ cdecl _strtime(ptr)
@ cdecl _strupr(str) ntdll._strupr
@ cdecl _swab(str str long)
@ extern _sys_errlist MSVCRT__sys_errlist
@ extern _sys_nerr MSVCRT__sys_nerr
@ cdecl _tell(long)
@ stub _telli64 #(long)
@ cdecl _tempnam(str str)
@ stub _timezone # extern
@ cdecl _tolower(long) MSVCRT__tolower
@ cdecl _toupper(long) MSVCRT__toupper
@ stub _tzname # extern
@ cdecl _tzset() tzset
@ cdecl _ui64toa(long long ptr long) ntdll._ui64toa
@ cdecl _ui64tow(long long ptr long) ntdll._ui64tow
@ cdecl _ultoa(long ptr long) ntdll._ultoa
@ cdecl _ultow(long ptr long) ntdll._ultow
@ cdecl _umask(long)
@ cdecl _ungetch(long)
@ cdecl _unlink(str)
@ cdecl _unloaddll(long)
@ cdecl _unlock(long)
@ cdecl _utime(str ptr)
@ cdecl _vsnprintf(ptr long ptr ptr) vsnprintf
@ cdecl _vsnwprintf(ptr long wstr long)
@ cdecl _waccess(wstr long)
@ stub _wasctime #(ptr)
@ cdecl _wchdir(wstr)
@ cdecl _wchmod(wstr long)
@ extern _wcmdln MSVCRT__wcmdln
@ cdecl _wcreat(wstr long)
@ cdecl _wcsdup(wstr)
@ cdecl _wcsicmp(wstr wstr) ntdll._wcsicmp
@ cdecl _wcsicoll(wstr wstr)
@ cdecl _wcslwr(wstr) ntdll._wcslwr
@ stub _wcsncoll #(wstr wstr long)
@ cdecl _wcsnicmp(wstr wstr long) ntdll._wcsnicmp
@ stub _wcsnicoll #(wstr wstr long)
@ cdecl _wcsnset(wstr long long)
@ cdecl _wcsrev(wstr)
@ cdecl _wcsset(wstr long)
@ cdecl _wcsupr(wstr) ntdll._wcsupr
@ stub _wctime #(ptr)
@ extern _wenviron MSVCRT__wenviron
@ stub _wexecl #(wstr wstr) varargs
@ stub _wexecle #(wstr wstr) varargs
@ stub _wexeclp #(wstr wstr) varargs
@ stub _wexeclpe #(wstr wstr) varargs
@ stub _wexecv #(wstr ptr)
@ stub _wexecve #(wstr ptr ptr)
@ stub _wexecvp #(wstr ptr)
@ stub _wexecvpe #(wstr ptr ptr)
@ cdecl _wfdopen(long wstr)
@ cdecl _wfindfirst(wstr ptr)
@ stub _wfindfirsti64 #(wstr ptr)
@ cdecl _wfindnext(long ptr)
@ stub _wfindnexti64 #(long ptr)
@ cdecl _wfopen(wstr wstr)
@ stub _wfreopen #(wstr wstr ptr)
@ cdecl _wfsopen(wstr wstr long)
@ stub _wfullpath #(ptr wstr long)
@ cdecl _wgetcwd(wstr long)
@ cdecl _wgetdcwd(long wstr long)
@ cdecl _wgetenv(wstr)
@ extern _winmajor MSVCRT__winmajor
@ extern _winminor MSVCRT__winminor
@ extern _winver MSVCRT__winver
@ cdecl _wmakepath(wstr wstr wstr wstr wstr)
@ cdecl _wmkdir(wstr)
@ cdecl _wmktemp(wstr)
@ varargs _wopen(wstr long)
@ stub _wperror #(wstr)
@ stub _wpgmptr # extern
@ stub _wpopen #(wstr wstr)
@ cdecl _wputenv(wstr)
@ cdecl _wremove(wstr)
@ cdecl _wrename(wstr wstr)
@ cdecl _write(long ptr long)
@ cdecl _wrmdir(wstr)
@ stub _wsearchenv #(wstr wstr ptr)
@ stub _wsetlocale #(long wstr)
@ varargs _wsopen (wstr long long) MSVCRT__wsopen
@ stub _wspawnl #(long wstr wstr) varargs
@ stub _wspawnle #(long wstr wstr) varargs
@ stub _wspawnlp #(long wstr wstr) varargs
@ stub _wspawnlpe #(long wstr wstr) varargs
@ stub _wspawnv #(long wstr ptr)
@ stub _wspawnve #(long wstr ptr ptr)
@ stub _wspawnvp #(long wstr ptr)
@ stub _wspawnvpe #(long wstr ptr ptr)
@ cdecl _wsplitpath(wstr wstr wstr wstr wstr)
@ cdecl _wstat(wstr ptr)
@ stub _wstati64 #(wstr ptr)
@ stub _wstrdate #(ptr)
@ stub _wstrtime #(ptr)
@ stub _wsystem #(wstr)
@ cdecl _wtempnam(wstr wstr)
@ stub _wtmpnam #(ptr)
@ cdecl _wtoi(wstr) ntdll._wtoi
@ cdecl _wtoi64(wstr) ntdll._wtoi64
@ cdecl _wtol(wstr) ntdll._wtol
@ cdecl _wunlink(wstr)
@ cdecl _wutime(wstr ptr)
@ cdecl _y0(double)
@ cdecl _y1(double)
@ cdecl _yn(long double )
@ cdecl abort() MSVCRT_abort
@ cdecl abs(long)
@ cdecl acos(double)
@ cdecl asctime(ptr)
@ cdecl asin(double)
@ cdecl atan(double)
@ cdecl atan2(double double)
@ cdecl atexit(ptr) MSVCRT_atexit
@ cdecl atof(str)
@ cdecl atoi(str)
@ cdecl atol(str)
@ cdecl bsearch(ptr ptr long long ptr)
@ cdecl calloc(long long) MSVCRT_calloc
@ cdecl ceil(double)
@ cdecl clearerr(ptr) MSVCRT_clearerr
@ cdecl clock() MSVCRT_clock
@ cdecl cos(double)
@ cdecl cosh(double)
@ cdecl ctime(ptr)
@ cdecl difftime(long long) MSVCRT_difftime
@ cdecl div(long long) MSVCRT_div
@ cdecl exit(long) MSVCRT_exit
@ cdecl exp(double)
@ cdecl fabs(double)
@ cdecl fclose(ptr) MSVCRT_fclose
@ cdecl feof(ptr) MSVCRT_feof
@ cdecl ferror(ptr) MSVCRT_ferror
@ cdecl fflush(ptr) MSVCRT_fflush
@ cdecl fgetc(ptr) MSVCRT_fgetc
@ cdecl fgetpos(ptr ptr) MSVCRT_fgetpos
@ cdecl fgets(ptr long ptr) MSVCRT_fgets
@ cdecl fgetwc(ptr) MSVCRT_fgetwc
@ cdecl fgetws(ptr long ptr) MSVCRT_fgetws
@ cdecl floor(double)
@ cdecl fmod(double double)
@ cdecl fopen(str str) MSVCRT_fopen
@ varargs fprintf(ptr str) MSVCRT_fprintf
@ cdecl fputc(long ptr) MSVCRT_fputc
@ cdecl fputs(str ptr) MSVCRT_fputs
@ cdecl fputwc(long ptr) MSVCRT_fputwc
@ cdecl fputws(wstr ptr) MSVCRT_fputws
@ cdecl fread(ptr long long ptr) MSVCRT_fread
@ cdecl free(ptr) MSVCRT_free
@ cdecl freopen(str str ptr) MSVCRT_freopen
@ cdecl frexp(double ptr)
@ varargs fscanf(ptr str) MSVCRT_fscanf
@ cdecl fseek(ptr long long) MSVCRT_fseek
@ cdecl fsetpos(ptr ptr) MSVCRT_fsetpos
@ cdecl ftell(ptr) MSVCRT_ftell
@ varargs fwprintf(ptr wstr) MSVCRT_fwprintf
@ cdecl fwrite(ptr long long ptr) MSVCRT_fwrite
@ varargs fwscanf(ptr wstr) MSVCRT_fwscanf
@ cdecl getc(ptr) MSVCRT_getc
@ cdecl getchar() MSVCRT_getchar
@ cdecl getenv(str) MSVCRT_getenv
@ cdecl gets(str) MSVCRT_gets
@ cdecl getwc(ptr) MSVCRT_getwc
@ cdecl getwchar() MSVCRT_getwchar
@ cdecl gmtime(ptr)
@ cdecl is_wctype(long long) ntdll.iswctype
@ cdecl isalnum(long) MSVCRT_isalnum
@ cdecl isalpha(long) MSVCRT_isalpha
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
@ cdecl labs(long)
@ cdecl ldexp( double long) MSVCRT_ldexp
@ cdecl ldiv(long long) MSVCRT_ldiv
@ stub localeconv #()
@ cdecl localtime(ptr)
@ cdecl log(double)
@ cdecl log10(double)
@ cdecl -register -i386 longjmp(ptr long) _MSVCRT_longjmp
@ cdecl malloc(long) MSVCRT_malloc
@ cdecl mblen(ptr long) MSVCRT_mblen
@ cdecl mbstowcs(ptr str long) ntdll.mbstowcs
@ cdecl mbtowc(wstr str long) MSVCRT_mbtowc
@ cdecl memchr(ptr long long)
@ cdecl memcmp(ptr ptr long)
@ cdecl memcpy(ptr ptr long)
@ cdecl memmove(ptr ptr long)
@ cdecl memset(ptr long long)
@ cdecl mktime(ptr) MSVCRT_mktime
@ cdecl modf(double ptr)
@ cdecl perror(str) MSVCRT_perror
@ cdecl pow(double double)
@ varargs printf(str) MSVCRT_printf
@ cdecl putc(long ptr) MSVCRT_putc
@ cdecl putchar(long) MSVCRT_putchar
@ cdecl puts(str) MSVCRT_puts
@ cdecl putwc(long ptr) MSVCRT_fputwc
@ cdecl putwchar(long) _fputwchar
@ cdecl qsort(ptr long long ptr)
@ stub raise #(long)
@ cdecl rand() MSVCRT_rand
@ cdecl realloc(ptr long) MSVCRT_realloc
@ cdecl remove(str) MSVCRT_remove
@ cdecl rename(str str) MSVCRT_rename
@ cdecl rewind(ptr) MSVCRT_rewind
@ varargs scanf(str) MSVCRT_scanf
@ cdecl setbuf(ptr ptr) MSVCRT_setbuf
@ cdecl setlocale(long str) MSVCRT_setlocale
@ cdecl setvbuf(ptr str long long) MSVCRT_setvbuf
@ cdecl signal(long long) MSVCRT_signal
@ cdecl sin(double)
@ cdecl sinh(double)
@ varargs sprintf(ptr str)
@ cdecl sqrt(double)
@ cdecl srand(long)
@ varargs sscanf(str str) MSVCRT_sscanf
@ cdecl strcat(str str)
@ cdecl strchr(str long)
@ cdecl strcmp(str str)
@ cdecl strcoll(str str)
@ cdecl strcpy(ptr str)
@ cdecl strcspn(str str)
@ cdecl strerror(long) MSVCRT_strerror
@ cdecl strftime(str long str ptr)
@ cdecl strlen(str)
@ cdecl strncat(str str long)
@ cdecl strncmp(str str long)
@ cdecl strncpy(ptr str long)
@ cdecl strpbrk(str str)
@ cdecl strrchr(str long)
@ cdecl strspn(str str)
@ cdecl strstr(str str)
@ cdecl strtod(str ptr)
@ cdecl strtok(str str)
@ cdecl strtol(str ptr long)
@ cdecl strtoul(str ptr long)
@ cdecl strxfrm(ptr str long)
@ varargs swprintf(wstr wstr) ntdll.swprintf
@ varargs swscanf(wstr wstr) MSVCRT_swscanf
@ cdecl system(str) MSVCRT_system
@ cdecl tan(double)
@ cdecl tanh(double)
@ cdecl time(ptr) MSVCRT_time
@ cdecl tmpfile() MSVCRT_tmpfile
@ cdecl tmpnam(ptr) MSVCRT_tmpnam
@ cdecl tolower(long)
@ cdecl toupper(long)
@ cdecl towlower(long) ntdll.towlower
@ cdecl towupper(long) ntdll.towupper
@ cdecl ungetc(long ptr) MSVCRT_ungetc
@ cdecl ungetwc(long ptr) MSVCRT_ungetwc
@ cdecl vfprintf(ptr str long) MSVCRT_vfprintf
@ cdecl vfwprintf(ptr wstr long) MSVCRT_vfwprintf
@ cdecl vprintf(str long) MSVCRT_vprintf
@ cdecl vsprintf(ptr str ptr)
@ cdecl vswprintf(ptr wstr long) MSVCRT_vswprintf
@ cdecl vwprintf(wstr long) MSVCRT_vwprintf
@ cdecl wcscat(wstr wstr) ntdll.wcscat
@ cdecl wcschr(wstr long) ntdll.wcschr
@ cdecl wcscmp(wstr wstr) ntdll.wcscmp
@ cdecl wcscoll(wstr wstr) MSVCRT_wcscoll
@ cdecl wcscpy(ptr wstr) ntdll.wcscpy
@ cdecl wcscspn(wstr wstr) ntdll.wcscspn
@ stub wcsftime #(ptr long wstr ptr)
@ cdecl wcslen(wstr) ntdll.wcslen
@ cdecl wcsncat(wstr wstr long) ntdll.wcsncat
@ cdecl wcsncmp(wstr wstr long) ntdll.wcsncmp
@ cdecl wcsncpy(ptr wstr long) ntdll.wcsncpy
@ cdecl wcspbrk(wstr wstr) MSVCRT_wcspbrk
@ cdecl wcsrchr(wstr long) ntdll.wcsrchr
@ cdecl wcsspn(wstr wstr) ntdll.wcsspn
@ cdecl wcsstr(wstr wstr) ntdll.wcsstr
@ stub wcstod #(wstr ptr)
@ cdecl wcstok(wstr wstr) ntdll.wcstok
@ cdecl wcstol(wstr ptr long) ntdll.wcstol
@ cdecl wcstombs(ptr ptr long) ntdll.wcstombs
@ cdecl wcstoul(wstr ptr long) ntdll.wcstoul
@ stub wcsxfrm #(ptr wstr long)
@ cdecl wctomb(ptr long) MSVCRT_wctomb
@ varargs wprintf(wstr) MSVCRT_wprintf
@ varargs wscanf(wstr) MSVCRT_wscanf
@ stub _Gettnames
@ stub __lc_collate_cp
