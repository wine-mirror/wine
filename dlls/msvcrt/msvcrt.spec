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
@ cdecl _CIacos() _CIacos
@ cdecl _CIasin() _CIasin
@ cdecl _CIatan() _CIatan
@ cdecl _CIatan2() _CIatan2
@ cdecl _CIcos() _CIcos
@ cdecl _CIcosh() _CIcosh
@ cdecl _CIexp() _CIexp
@ cdecl _CIfmod() _CIfmod
@ cdecl _CIlog() _CIlog
@ cdecl _CIlog10() _CIlog10
@ cdecl _CIpow() _CIpow
@ cdecl _CIsin() _CIsin
@ cdecl _CIsinh() _CIsinh
@ cdecl _CIsqrt() _CIsqrt
@ cdecl _CItan() _CItan
@ cdecl _CItanh() _CItanh
@ cdecl _CxxThrowException(long long) _CxxThrowException
@ cdecl -i386 -norelay _EH_prolog() _EH_prolog
@ cdecl _Getdays() _Getdays
@ cdecl _Getmonths() _Getmonths
@ cdecl _Getnames() _Getnames
@ extern _HUGE MSVCRT__HUGE
@ cdecl _Strftime(str long str ptr ptr) _Strftime
@ cdecl _XcptFilter(long ptr) _XcptFilter
@ cdecl -register -i386 __CxxFrameHandler(ptr ptr ptr ptr) __CxxFrameHandler
@ stub __CxxLongjmpUnwind
@ cdecl __RTCastToVoid(ptr) MSVCRT___RTCastToVoid
@ cdecl __RTDynamicCast(ptr long ptr ptr long) MSVCRT___RTDynamicCast
@ cdecl __RTtypeid(ptr) MSVCRT___RTtypeid
@ stub __STRINGTOLD
@ extern __argc MSVCRT___argc
@ extern __argv MSVCRT___argv
@ stub __badioinfo
@ stub __crtCompareStringA
@ stub __crtGetLocaleInfoW
@ stub __crtLCMapStringA
@ cdecl __dllonexit(ptr ptr ptr) __dllonexit
@ cdecl __doserrno() __doserrno
@ stub __fpecode #()
@ cdecl __getmainargs(ptr ptr ptr long ptr) __getmainargs
@ extern __initenv MSVCRT___initenv
@ cdecl __isascii(long) MSVCRT___isascii
@ cdecl __iscsym(long) MSVCRT___iscsym
@ cdecl __iscsymf(long) MSVCRT___iscsymf
@ stub __lc_codepage
@ stub __lc_collate
@ stub __lc_handle
@ cdecl __lconv_init() __lconv_init
@ extern __mb_cur_max MSVCRT___mb_cur_max
@ cdecl __p___argc() __p___argc
@ cdecl __p___argv() __p___argv
@ cdecl __p___initenv() __p___initenv
@ cdecl __p___mb_cur_max() __p___mb_cur_max
@ cdecl __p___wargv() __p___wargv
@ cdecl __p___winitenv() __p___winitenv
@ cdecl __p__acmdln()  __p__acmdln
@ stub __p__amblksiz #()
@ cdecl __p__commode()  __p__commode
@ stub __p__daylight #()
@ stub __p__dstbias #()
@ cdecl __p__environ() __p__environ
@ stub __p__fileinfo #()
@ cdecl __p__fmode() __p__fmode
@ cdecl __p__iob() __p__iob
@ stub __p__mbcasemap #()
@ cdecl __p__mbctype() __p__mbctype
@ cdecl __p__osver()  __p__osver
@ cdecl __p__pctype() __p__pctype
@ stub __p__pgmptr #()
@ stub __p__pwctype #()
@ cdecl __p__timezone() __p__timezone
@ stub __p__tzname #()
@ cdecl __p__wcmdln() __p__wcmdln
@ cdecl __p__wenviron() __p__wenviron
@ cdecl __p__winmajor() __p__winmajor
@ cdecl __p__winminor() __p__winminor
@ cdecl __p__winver() __p__winver
@ stub __p__wpgmptr #()
@ stub __pioinfo #()
@ stub __pxcptinfoptrs #()
@ cdecl __set_app_type(long) MSVCRT___set_app_type
@ extern __setlc_active MSVCRT___setlc_active
@ cdecl __setusermatherr(ptr) MSVCRT___setusermatherr
@ forward __threadhandle kernel32.GetCurrentThread
@ forward __threadid kernel32.GetCurrentThreadId
@ cdecl __toascii(long) MSVCRT___toascii
@ cdecl __unDName(long str ptr ptr long) MSVCRT___unDName
@ cdecl __unDNameEx() MSVCRT___unDNameEx #FIXME
@ extern __unguarded_readlc_active MSVCRT___unguarded_readlc_active
@ extern __wargv MSVCRT___wargv
@ cdecl __wgetmainargs(ptr ptr ptr long ptr) __wgetmainargs
@ extern __winitenv MSVCRT___winitenv
@ cdecl _abnormal_termination() _abnormal_termination
@ cdecl _access(str long) _access
@ extern _acmdln MSVCRT__acmdln
@ cdecl _adj_fdiv_m16i() _adj_fdiv_m16i
@ cdecl _adj_fdiv_m32() _adj_fdiv_m32
@ cdecl _adj_fdiv_m32i() _adj_fdiv_m32i
@ cdecl _adj_fdiv_m64() _adj_fdiv_m64
@ cdecl _adj_fdiv_r() _adj_fdiv_r
@ cdecl _adj_fdivr_m16i() _adj_fdivr_m16i
@ cdecl _adj_fdivr_m32() _adj_fdivr_m32
@ cdecl _adj_fdivr_m32i() _adj_fdivr_m32i
@ cdecl _adj_fdivr_m64() _adj_fdivr_m64
@ cdecl _adj_fpatan() _adj_fpatan
@ cdecl _adj_fprem() _adj_fprem
@ cdecl _adj_fprem1() _adj_fprem1
@ cdecl _adj_fptan() _adj_fptan
@ cdecl _adjust_fdiv() _adjust_fdiv
@ stub _aexit_rtn
@ cdecl _amsg_exit(long) MSVCRT__amsg_exit
@ cdecl _assert(str str long) MSVCRT__assert
@ stub _atodbl
@ stub _atoi64 #(str)
@ stub _atoldbl
@ cdecl _beep(long long) _beep
@ cdecl _beginthread (ptr long ptr) _beginthread
@ cdecl _beginthreadex (ptr long ptr ptr long ptr) _beginthreadex
@ cdecl _c_exit() MSVCRT__c_exit
@ cdecl _cabs(long) _cabs
@ cdecl _callnewh(long) _callnewh
@ cdecl _cexit() MSVCRT__cexit
@ cdecl _cgets(str) _cgets
@ cdecl _chdir(str) _chdir
@ cdecl _chdrive(long) _chdrive
@ cdecl _chgsign( double ) _chgsign
@ cdecl -i386 _chkesp() _chkesp
@ cdecl _chmod(str long) _chmod
@ cdecl _chsize (long long) _chsize
@ cdecl _clearfp() _clearfp
@ cdecl _close(long) _close
@ cdecl _commit(long) _commit
@ extern _commode MSVCRT__commode
@ cdecl _control87(long long) _control87
@ cdecl _controlfp(long long) _controlfp
@ cdecl _copysign( double double ) _copysign
@ varargs _cprintf(str) _cprintf
@ cdecl _cputs(str) _cputs
@ cdecl _creat(str long) _creat
@ varargs _cscanf(str) _cscanf
@ extern _ctype MSVCRT__ctype
@ cdecl _cwait(ptr long long) _cwait
@ stub _daylight
@ stub _dstbias
@ cdecl _dup (long) _dup
@ cdecl _dup2 (long long) _dup2
@ cdecl _ecvt( double long ptr ptr) ecvt
@ cdecl _endthread () _endthread
@ cdecl _endthreadex(long) _endthreadex
@ extern _environ MSVCRT__environ
@ cdecl _eof(long) _eof
@ cdecl _errno() MSVCRT__errno
@ cdecl _except_handler2(ptr ptr ptr ptr) _except_handler2
@ cdecl _except_handler3(ptr ptr ptr ptr) _except_handler3
@ varargs _execl(str str) _execl
@ stub _execle #(str str) varargs
@ varargs _execlp(str str) _execlp
@ stub _execlpe #(str str) varargs
@ cdecl _execv(str str) _execv
@ cdecl _execve(str str str) _execve
@ cdecl _execvp(str str) _execvp
@ cdecl _execvpe(str str str) _execvpe
@ cdecl _exit(long) MSVCRT__exit
@ cdecl _expand(ptr long) _expand
@ cdecl _fcloseall() _fcloseall
@ cdecl _fcvt( double long ptr ptr) fcvt
@ cdecl _fdopen(long str) _fdopen
@ cdecl _fgetchar() _fgetchar
@ cdecl _fgetwchar() _fgetwchar
@ cdecl _filbuf(ptr) _filbuf
@ stub _fileinfo
@ cdecl _filelength(long) _filelength
@ stub _filelengthi64 #(long)
@ cdecl _fileno(ptr) _fileno
@ cdecl _findclose(long) _findclose
@ cdecl _findfirst(str ptr) _findfirst
@ stub _findfirsti64 #(str ptr)
@ cdecl _findnext(long ptr) _findnext
@ stub _findnexti64 #(long ptr)
@ cdecl _finite( double ) _finite
@ cdecl _flsbuf(long ptr) _flsbuf
@ cdecl _flushall() _flushall
@ extern _fmode MSVCRT__fmode
@ cdecl _fpclass(double) _fpclass
@ stub _fpieee_flt
@ cdecl _fpreset() _fpreset
@ cdecl _fputchar(long) _fputchar
@ cdecl _fputwchar(long) _fputwchar
@ cdecl _fsopen(str str long) _fsopen
@ cdecl _fstat(long ptr) MSVCRT__fstat
@ cdecl _fstati64(long ptr) _fstati64
@ cdecl _ftime(ptr) _ftime
@ forward _ftol ntdll._ftol
@ cdecl _fullpath(ptr str long) _fullpath
@ cdecl _futime(long ptr) _futime
@ cdecl _gcvt( double long str) gcvt
@ cdecl _get_osfhandle(long) _get_osfhandle
@ stub _get_sbh_threshold #()
@ cdecl _getch() _getch
@ cdecl _getche() _getche
@ cdecl _getcwd(str long) _getcwd
@ cdecl _getdcwd(long str long) _getdcwd
@ cdecl _getdiskfree(long ptr) _getdiskfree
@ forward _getdllprocaddr kernel32.GetProcAddress
@ cdecl _getdrive() _getdrive
@ forward _getdrives kernel32.GetLogicalDrives
@ stub _getmaxstdio #()
@ cdecl _getmbcp() _getmbcp
@ forward _getpid kernel32.GetCurrentProcessId
@ stub _getsystime #(ptr)
@ cdecl _getw(ptr) _getw
@ cdecl _getws(ptr) MSVCRT__getws
@ cdecl _global_unwind2(ptr) _global_unwind2
@ cdecl _heapadd (ptr long) _heapadd
@ cdecl _heapchk() _heapchk
@ cdecl _heapmin() _heapmin
@ cdecl _heapset(long) _heapset
@ stub _heapused #(ptr ptr)
@ cdecl _heapwalk(ptr) _heapwalk
@ cdecl _hypot(double double) hypot
@ stub _i64toa #(long str long)
@ stub _i64tow #(long wstr long)
@ cdecl _initterm(ptr ptr) _initterm
@ stub _inp #(long) -i386
@ stub _inpd #(long) -i386
@ stub _inpw #(long) -i386
@ extern _iob MSVCRT__iob
@ cdecl _isatty(long) _isatty
@ cdecl _isctype(long long) _isctype
@ stub _ismbbalnum #(long)
@ stub _ismbbalpha #(long)
@ stub _ismbbgraph #(long)
@ stub _ismbbkalnum #(long)
@ cdecl _ismbbkana(long) _ismbbkana
@ stub _ismbbkprint #(long)
@ stub _ismbbkpunct #(long)
@ cdecl _ismbblead(long) _ismbblead
@ stub _ismbbprint #(long)
@ stub _ismbbpunct #(long)
@ cdecl _ismbbtrail(long) _ismbbtrail
@ cdecl _ismbcalnum(long) _ismbcalnum
@ cdecl _ismbcalpha(long) _ismbcalpha
@ cdecl _ismbcdigit(long) _ismbcdigit
@ cdecl _ismbcgraph(long) _ismbcgraph
@ cdecl _ismbchira(long) _ismbchira
@ cdecl _ismbckata(long) _ismbckata
@ stub _ismbcl0 #(long)
@ stub _ismbcl1 #(long)
@ stub _ismbcl2 #(long)
@ stub _ismbclegal #(long)
@ cdecl _ismbclower(long) _ismbclower
@ cdecl _ismbcprint(long) _ismbcprint
@ cdecl _ismbcpunct(long) _ismbcpunct
@ cdecl _ismbcspace(long) _ismbcspace
@ cdecl _ismbcsymbol(long) _ismbcsymbol
@ cdecl _ismbcupper(long) _ismbcupper
@ cdecl _ismbslead(ptr ptr) _ismbslead
@ cdecl _ismbstrail(ptr ptr) _ismbstrail
@ cdecl _isnan( double ) _isnan
@ forward _itoa ntdll._itoa
@ cdecl _itow(long wstr long) _itow
@ cdecl _j0(double) j0
@ cdecl _j1(double) j1
@ cdecl _jn(long double) jn
@ cdecl _kbhit() _kbhit
@ cdecl _lfind(ptr ptr ptr long ptr) _lfind
@ cdecl _loaddll(str) _loaddll
@ cdecl _local_unwind2(ptr long) _local_unwind2
@ cdecl _lock(long) _lock
@ cdecl _locking(long long long) _locking
@ cdecl _logb( double ) _logb
@ stub _longjmpex
@ cdecl _lrotl(long long) _lrotl
@ cdecl _lrotr(long long) _lrotr
@ cdecl _lsearch(ptr ptr long long ptr) _lsearch
@ cdecl _lseek(long long long) _lseek
@ cdecl -ret64 _lseeki64(long long long long) _lseeki64
@ forward _ltoa ntdll._ltoa
@ cdecl _ltow(long ptr long) _ltow
@ cdecl _makepath(str str str str str) _makepath
@ cdecl _matherr(ptr) _matherr
@ cdecl _mbbtombc(long) _mbbtombc
@ stub _mbbtype #(long long)
@ stub _mbcasemap
@ cdecl _mbccpy (str str) strcpy
@ stub _mbcjistojms #(long)
@ stub _mbcjmstojis #(long)
@ cdecl _mbclen(ptr) _mbclen
@ stub _mbctohira #(long)
@ stub _mbctokata #(long)
@ cdecl _mbctolower(long) _mbctolower
@ stub _mbctombb #(long)
@ cdecl _mbctoupper(long) _mbctoupper
@ stub _mbctype
@ stub _mbsbtype #(ptr long)
@ cdecl _mbscat(str str) strcat
@ cdecl _mbschr(str long) _mbschr
@ cdecl _mbscmp(str str) _mbscmp
@ stub _mbscoll #(str str)
@ cdecl _mbscpy(ptr str) strcpy
@ cdecl _mbscspn (str str) _mbscspn
@ cdecl _mbsdec(ptr ptr) _mbsdec
@ cdecl _mbsdup(str) _strdup
@ cdecl _mbsicmp(str str) _mbsicmp
@ cdecl _mbsicoll(str str) _mbsicoll
@ cdecl _mbsinc(str) _mbsinc
@ cdecl _mbslen(str) _mbslen
@ cdecl _mbslwr(str) _mbslwr
@ stub _mbsnbcat #(str str long)
@ cdecl _mbsnbcmp(str str long) _mbsnbcmp
@ cdecl _mbsnbcnt(ptr long) _mbsnbcnt
@ stub _mbsnbcoll #(str str long)
@ cdecl _mbsnbcpy(ptr str long) _mbsnbcpy
@ cdecl _mbsnbicmp(str str long) _mbsnbicmp
@ stub _mbsnbicoll #(str str long)
@ cdecl _mbsnbset(str long long) _mbsnbset
@ cdecl _mbsncat(str str long) _mbsncat
@ cdecl _mbsnccnt(str long) _mbsnccnt
@ cdecl _mbsncmp(str str long) _mbsncmp
@ stub _mbsncoll #(ptr str long)
@ cdecl _mbsncpy(str str long) _mbsncpy
@ cdecl _mbsnextc(str) _mbsnextc
@ cdecl _mbsnicmp(str str long) _mbsnicmp
@ stub _mbsnicoll #(str str long)
@ cdecl _mbsninc(str long) _mbsninc
@ cdecl _mbsnset(str long long) _mbsnset
@ cdecl _mbspbrk(str str) _mbspbrk
@ cdecl _mbsrchr(str long) _mbsrchr
@ cdecl _mbsrev(str) _mbsrev
@ cdecl _mbsset(str long) _mbsset
@ cdecl _mbsspn(str str) _mbsspn
@ stub _mbsspnp #(str str)
@ cdecl _mbsstr(str str) strstr
@ cdecl _mbstok(str str) _mbstok
@ cdecl _mbstrlen(str) _mbstrlen
@ cdecl _mbsupr(str) _mbsupr
@ cdecl _memccpy(ptr ptr long long) memccpy
@ forward _memicmp ntdll._memicmp
@ cdecl _mkdir(str) _mkdir
@ cdecl _mktemp(str) _mktemp
@ cdecl _msize(ptr) _msize
@ cdecl _nextafter(double double) _nextafter
@ cdecl _onexit(ptr) _onexit
@ varargs _open(str long) _open
@ cdecl _open_osfhandle(long long) _open_osfhandle
@ stub _osver
@ stub _outp #(long long)
@ stub _outpd #(long long)
@ stub _outpw #(long long)
@ stub _pclose #(ptr)
@ extern _pctype MSVCRT__pctype
@ stub _pgmptr
@ stub _pipe #(ptr long long)
@ stub _popen #(str str)
@ cdecl _purecall() _purecall
@ cdecl _putch(long) _putch
@ cdecl _putenv(str) _putenv
@ cdecl _putw(long ptr) _putw
@ cdecl _putws(wstr) _putws
@ stub _pwctype
@ cdecl _read(long ptr long) _read
@ cdecl _rmdir(str) _rmdir
@ cdecl _rmtmp() _rmtmp
@ cdecl _rotl(long long) _rotl
@ cdecl _rotr(long long) _rotr
@ cdecl _safe_fdiv() _safe_fdiv
@ cdecl _safe_fdivr() _safe_fdivr
@ cdecl _safe_fprem() _safe_fprem
@ cdecl _safe_fprem1() _safe_fprem1
@ cdecl _scalb( double long) _scalb
@ cdecl _searchenv(str str str) _searchenv
@ stdcall -i386 _seh_longjmp_unwind(ptr) _seh_longjmp_unwind
@ stub _set_error_mode #(long)
@ stub _set_sbh_threshold #(long)
@ stub _seterrormode #(long)
@ cdecl -register -i386 _setjmp(ptr) _MSVCRT__setjmp
@ cdecl -register -i386 _setjmp3(ptr long) _MSVCRT__setjmp3
@ stub _setmaxstdio #(long)
@ cdecl _setmbcp(long) _setmbcp
@ cdecl _setmode(long long) _setmode
@ stub _setsystime #(ptr long)
@ cdecl _sleep(long) _sleep
@ varargs _snprintf(str long str) snprintf
@ forward _snwprintf ntdll._snwprintf
@ varargs _sopen(str long long) MSVCRT__sopen
@ varargs _spawnl(long str str) _spawnl
@ stub _spawnle #(str str) varargs
@ varargs _spawnlp(long str str) _spawnlp
@ stub _spawnlpe #(str str) varargs
@ cdecl _spawnv(long str ptr) _spawnv
@ cdecl _spawnve(long str ptr ptr) _spawnve
@ cdecl _spawnvp(long str ptr) _spawnvp
@ cdecl _spawnvpe(long str ptr ptr) _spawnvpe
@ forward _splitpath ntdll._splitpath
@ cdecl _stat(str ptr) MSVCRT__stat
@ cdecl _stati64(str ptr) _stati64
@ cdecl _statusfp() _statusfp
@ cdecl _strcmpi(str str) strcasecmp
@ cdecl _strdate(str) _strdate
@ cdecl _strdup(str) _strdup
@ cdecl _strerror(long) _strerror
@ cdecl _stricmp(str str) strcasecmp
@ stub _stricoll #(str str)
@ forward _strlwr ntdll._strlwr
@ stub _strncoll #(str str long)
@ cdecl _strnicmp(str str long) strncasecmp
@ stub _strnicoll #(str str long)
@ cdecl _strnset(str long long) _strnset
@ cdecl _strrev(str) _strrev
@ cdecl _strset(str long) _strset
@ cdecl _strtime(str) _strtime
@ forward _strupr ntdll._strupr
@ cdecl _swab(str str long) _swab
@ extern _sys_errlist MSVCRT__sys_errlist
@ extern _sys_nerr MSVCRT__sys_nerr
@ cdecl _tell(long) _tell
@ stub _telli64 #(long)
@ cdecl _tempnam(str str) _tempnam
@ stub _timezone #()
@ cdecl _tolower(long) MSVCRT__tolower
@ cdecl _toupper(long) MSVCRT__toupper
@ stub _tzname
@ cdecl _tzset() tzset
@ stub _ui64toa #(long str long)
@ stub _ui64tow #(long wstr long)
@ forward _ultoa ntdll._ultoa
@ forward _ultow ntdll._ultow
@ cdecl _umask(long) _umask
@ cdecl _ungetch(long) _ungetch
@ cdecl _unlink(str) _unlink
@ cdecl _unloaddll(long) _unloaddll
@ cdecl _unlock(long) _unlock
@ cdecl _utime(str ptr) _utime
@ cdecl _vsnprintf(ptr long ptr ptr) vsnprintf
@ cdecl _vsnwprintf(ptr long wstr long) _vsnwprintf
@ cdecl _waccess(wstr long) _waccess
@ stub _wasctime #(ptr)
@ cdecl _wchdir(wstr) _wchdir
@ cdecl _wchmod(wstr long) _wchmod
@ extern _wcmdln MSVCRT__wcmdln
@ cdecl _wcreat(wstr long) _wcreat
@ cdecl _wcsdup(wstr) _wcsdup
@ forward _wcsicmp ntdll._wcsicmp
@ cdecl _wcsicoll(wstr wstr) _wcsicoll
@ forward _wcslwr ntdll._wcslwr
@ stub _wcsncoll #(wstr wstr long)
@ forward _wcsnicmp ntdll._wcsnicmp
@ stub _wcsnicoll #(wstr wstr long)
@ cdecl _wcsnset(wstr long long) _wcsnset
@ cdecl _wcsrev(wstr) _wcsrev
@ cdecl _wcsset(wstr long) _wcsset
@ forward _wcsupr ntdll._wcsupr
@ stub _wctime #(ptr)
@ extern _wenviron MSVCRT__wenviron
@ stub _wexecl #(wstr wstr) varargs
@ stub _wexecle #(wstr wstr) varargs
@ stub _wexeclp #(wstr wstr) varargs
@ stub _wexeclpe #(wstr wstr) varargs
@ stub _wexecv #(wstr wstr)
@ stub _wexecve #(wstr wstr wstr)
@ stub _wexecvp #(wstr wstr)
@ stub _wexecvpe #(wstr wstr wstr)
@ cdecl _wfdopen(long wstr) _wfdopen
@ cdecl _wfindfirst(wstr ptr) _wfindfirst
@ stub _wfindfirsti64 #(wstr ptr)
@ cdecl _wfindnext(long ptr)  _wfindnext
@ stub _wfindnexti64 #(long ptr)
@ cdecl _wfopen(wstr wstr) _wfopen
@ stub _wfreopen #(wstr wstr ptr)
@ cdecl _wfsopen(wstr wstr long) _wfsopen
@ stub _wfullpath #(wstr wstr long)
@ cdecl _wgetcwd(wstr long) _wgetcwd
@ cdecl _wgetdcwd(long wstr long) _wgetdcwd
@ cdecl _wgetenv(wstr) _wgetenv
@ extern _winmajor MSVCRT__winmajor
@ extern _winminor MSVCRT__winminor
@ extern _winver MSVCRT__winver
@ cdecl _wmakepath(wstr wstr wstr wstr wstr) _wmakepath
@ cdecl _wmkdir(wstr) _wmkdir
@ cdecl _wmktemp(wstr) _wmktemp
@ varargs _wopen(wstr long) _wopen
@ stub _wperror #(wstr)
@ stub _wpgmptr
@ stub _wpopen #(wstr wstr)
@ cdecl _wputenv(wstr) _wputenv
@ cdecl _wremove(wstr) _wremove
@ cdecl _wrename(wstr wstr) _wrename
@ cdecl _write(long ptr long) _write
@ cdecl _wrmdir(wstr) _wrmdir
@ stub _wsearchenv #(wstr wstr wstr)
@ stub _wsetlocale #(long wstr)
@ varargs _wsopen (wstr long long) MSVCRT__wsopen
@ stub _wspawnl #(long wstr wstr) varargs
@ stub _wspawnle #(long wstr wstr) varargs
@ stub _wspawnlp #(long wstr wstr) varargs
@ stub _wspawnlpe #(long wstr wstr) varargs
@ stub _wspawnv #(long wstr wstr)
@ stub _wspawnve #(long wstr wstr wstr)
@ stub _wspawnvp #(long wstr wstr)
@ stub _wspawnvpe #(long wstr wstr wstr)
@ cdecl _wsplitpath(wstr wstr wstr wstr wstr) _wsplitpath
@ cdecl _wstat(wstr ptr) _wstat
@ stub _wstati64 #(wstr ptr)
@ stub _wstrdate #(wstr)
@ stub _wstrtime #(wstr)
@ stub _wsystem #(wstr)
@ cdecl _wtempnam(wstr wstr) _wtempnam
@ stub _wtmpnam #(wstr)
@ forward _wtoi NTDLL._wtoi
@ stub _wtoi64 #(wstr)
@ forward _wtol NTDLL._wtol
@ cdecl _wunlink(wstr) _wunlink
@ cdecl _wutime(wstr ptr) _wutime
@ cdecl _y0(double) _y0
@ cdecl _y1(double) _y1
@ cdecl _yn(long double ) _yn
@ cdecl abort() MSVCRT_abort
@ cdecl abs(long) abs
@ cdecl acos(double) acos
@ cdecl asctime(ptr) asctime
@ cdecl asin(double) asin
@ cdecl atan(double) atan
@ cdecl atan2(double double) atan2
@ cdecl atexit(ptr) MSVCRT_atexit
@ cdecl atof(str) atof
@ cdecl atoi(str) atoi
@ cdecl atol(str) atol
@ cdecl bsearch(ptr ptr long long ptr) bsearch
@ cdecl calloc(long long) MSVCRT_calloc
@ cdecl ceil(double) ceil
@ cdecl clearerr(ptr) MSVCRT_clearerr
@ cdecl clock() MSVCRT_clock
@ cdecl cos(double) cos
@ cdecl cosh(double) cosh
@ cdecl ctime(ptr) ctime
@ cdecl difftime(long long) MSVCRT_difftime
@ cdecl div(long long) MSVCRT_div
@ cdecl exit(long) MSVCRT_exit
@ cdecl exp(double) exp
@ cdecl fabs(double) fabs
@ cdecl fclose(ptr) MSVCRT_fclose
@ cdecl feof(ptr) MSVCRT_feof
@ cdecl ferror(ptr) MSVCRT_ferror
@ cdecl fflush(ptr) MSVCRT_fflush
@ cdecl fgetc(ptr) MSVCRT_fgetc
@ cdecl fgetpos(ptr ptr) MSVCRT_fgetpos
@ cdecl fgets(ptr long ptr) MSVCRT_fgets
@ cdecl fgetwc(ptr) MSVCRT_fgetwc
@ cdecl fgetws(ptr long ptr) MSVCRT_fgetws
@ cdecl floor(double) floor
@ cdecl fmod(double double) fmod
@ cdecl fopen(str str) MSVCRT_fopen
@ varargs fprintf(ptr str) MSVCRT_fprintf
@ cdecl fputc(long ptr) MSVCRT_fputc
@ cdecl fputs(str ptr) MSVCRT_fputs
@ cdecl fputwc(long ptr) MSVCRT_fputwc
@ cdecl fputws(wstr ptr) MSVCRT_fputws
@ cdecl fread(ptr long long ptr) MSVCRT_fread
@ cdecl free(ptr) MSVCRT_free
@ cdecl freopen(str str ptr) MSVCRT_freopen
@ cdecl frexp(double ptr) frexp
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
@ cdecl gmtime(ptr) gmtime
@ forward is_wctype ntdll.iswctype
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
@ forward iswalpha ntdll.iswalpha
@ cdecl iswascii(long) MSVCRT_iswascii
@ cdecl iswcntrl(long) MSVCRT_iswcntrl
@ forward iswctype ntdll.iswctype
@ cdecl iswdigit(long) MSVCRT_iswdigit
@ cdecl iswgraph(long) MSVCRT_iswgraph
@ cdecl iswlower(long) MSVCRT_iswlower
@ cdecl iswprint(long) MSVCRT_iswprint
@ cdecl iswpunct(long) MSVCRT_iswpunct
@ cdecl iswspace(long) MSVCRT_iswspace
@ cdecl iswupper(long) MSVCRT_iswupper
@ cdecl iswxdigit(long) MSVCRT_iswxdigit
@ cdecl isxdigit(long) MSVCRT_isxdigit
@ cdecl labs(long) labs
@ cdecl ldexp( double long) MSVCRT_ldexp
@ cdecl ldiv(long long) MSVCRT_ldiv
@ stub localeconv #()
@ cdecl localtime(ptr) localtime
@ cdecl log(double) log
@ cdecl log10(double) log10
@ cdecl -register -i386 longjmp(ptr long) _MSVCRT_longjmp
@ cdecl malloc(long) MSVCRT_malloc
@ cdecl mblen(ptr long) MSVCRT_mblen
@ forward mbstowcs ntdll.mbstowcs
@ cdecl mbtowc(wstr str long) MSVCRT_mbtowc
@ cdecl memchr(ptr long long) memchr
@ cdecl memcmp(ptr ptr long) memcmp
@ cdecl memcpy(ptr ptr long) memcpy
@ cdecl memmove(ptr ptr long) memmove
@ cdecl memset(ptr long long) memset
@ cdecl mktime(ptr) MSVCRT_mktime
@ cdecl modf(double ptr) modf
@ cdecl perror(str) MSVCRT_perror
@ cdecl pow(double double) pow
@ varargs printf(str) MSVCRT_printf
@ cdecl putc(long ptr) MSVCRT_putc
@ cdecl putchar(long) MSVCRT_putchar
@ cdecl puts(str) MSVCRT_puts
@ cdecl putwc(long ptr) MSVCRT_fputwc
@ cdecl putwchar(long) _fputwchar
@ cdecl qsort(ptr long long ptr) qsort
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
@ cdecl sin(double) sin
@ cdecl sinh(double) sinh
@ varargs sprintf(ptr str) sprintf
@ cdecl sqrt(double) sqrt
@ cdecl srand(long) srand
@ varargs sscanf(str str) MSVCRT_sscanf
@ cdecl strcat(str str) strcat
@ cdecl strchr(str long) strchr
@ cdecl strcmp(str str) strcmp
@ cdecl strcoll(str str) strcoll
@ cdecl strcpy(ptr str) strcpy
@ cdecl strcspn(str str) strcspn
@ cdecl strerror(long) MSVCRT_strerror
@ cdecl strftime(str long str ptr) strftime
@ cdecl strlen(str) strlen
@ cdecl strncat(str str long) strncat
@ cdecl strncmp(str str long) strncmp
@ cdecl strncpy(ptr str long) strncpy
@ cdecl strpbrk(str str) strpbrk
@ cdecl strrchr(str long) strrchr
@ cdecl strspn(str str) strspn
@ cdecl strstr(str str) strstr
@ cdecl strtod(str ptr) strtod
@ cdecl strtok(str str) strtok
@ cdecl strtol(str ptr long) strtol
@ cdecl strtoul(str ptr long) strtoul
@ cdecl strxfrm(ptr str long) strxfrm
@ forward swprintf ntdll.swprintf
@ varargs swscanf(wstr wstr) MSVCRT_swscanf
@ cdecl system(str) MSVCRT_system
@ cdecl tan(double) tan
@ cdecl tanh(double) tanh
@ cdecl time(ptr) MSVCRT_time
@ cdecl tmpfile() MSVCRT_tmpfile
@ cdecl tmpnam(str) MSVCRT_tmpnam
@ cdecl tolower(long) tolower
@ cdecl toupper(long) toupper
@ forward towlower ntdll.towlower
@ forward towupper ntdll.towupper
@ cdecl ungetc(long ptr) MSVCRT_ungetc
@ cdecl ungetwc(long ptr) MSVCRT_ungetwc
@ cdecl vfprintf(ptr str long) MSVCRT_vfprintf
@ cdecl vfwprintf(ptr wstr long) MSVCRT_vfwprintf
@ cdecl vprintf(str long) MSVCRT_vprintf
@ cdecl vsprintf(ptr str ptr) vsprintf
@ cdecl vswprintf(ptr wstr long) MSVCRT_vswprintf
@ cdecl vwprintf(wstr long) MSVCRT_vwprintf
@ forward wcscat ntdll.wcscat
@ forward wcschr ntdll.wcschr
@ forward wcscmp ntdll.wcscmp
@ cdecl wcscoll(wstr wstr) MSVCRT_wcscoll
@ forward wcscpy ntdll.wcscpy
@ forward wcscspn ntdll.wcscspn
@ stub wcsftime #(wstr long wstr ptr)
@ forward wcslen ntdll.wcslen
@ forward wcsncat ntdll.wcsncat
@ forward wcsncmp ntdll.wcsncmp
@ forward wcsncpy ntdll.wcsncpy
@ cdecl wcspbrk(wstr wstr) MSVCRT_wcspbrk
@ forward wcsrchr ntdll.wcsrchr
@ forward wcsspn ntdll.wcsspn
@ forward wcsstr ntdll.wcsstr
@ stub wcstod #(wstr ptr)
@ forward wcstok ntdll.wcstok
@ forward wcstol ntdll.wcstol
@ forward wcstombs ntdll.wcstombs
@ forward wcstoul ntdll.wcstoul
@ stub wcsxfrm #(wstr wstr long)
@ cdecl wctomb(ptr long) MSVCRT_wctomb
@ varargs wprintf(wstr) MSVCRT_wprintf
@ varargs wscanf(wstr) MSVCRT_wscanf
@ stub _Gettnames
@ stub __lc_collate_cp
