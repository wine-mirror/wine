# msvcrt.dll - MS VC++ Run Time Library
name    msvcrt
type    win32
init    MSVCRT_Init

import kernel32.dll
import ntdll.dll

debug_channels (msvcrt)

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
@ cdecl ??3@YAXPAX@Z(ptr) MSVCRT_operator_delete
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
@ stub ?_set_se_translator@@YAP6AXIPAU_EXCEPTION_POINTERS@@@ZP6AXI0@Z@Z
@ stub ?before@type_info@@QBEHABV1@@Z #(ptr ptr) stdcall
@ stdcall ?name@type_info@@QBEPBDXZ(ptr) MSVCRT_type_info_name
@ stdcall ?raw_name@type_info@@QBEPBDXZ(ptr) MSVCRT_type_info_raw_name
@ stub ?set_new_handler@@YAP6AXXZP6AXXZ@Z 
@ stub ?set_terminate@@YAP6AXXZP6AXXZ@Z
@ stub ?set_unexpected@@YAP6AXXZP6AXXZ@Z
@ stub ?terminate@@YAXXZ #()
@ stub ?unexpected@@YAXXZ #()
@ stdcall ?what@exception@@UBEPBDXZ(ptr) MSVCRT_exception_what
@ cdecl -noimport _CIacos() MSVCRT__CIacos
@ cdecl -noimport _CIasin() MSVCRT__CIasin
@ cdecl -noimport _CIatan() MSVCRT__CIatan
@ cdecl -noimport _CIatan2() MSVCRT__CIatan2
@ cdecl -noimport _CIcos() MSVCRT__CIcos
@ cdecl -noimport _CIcosh() MSVCRT__CIcosh
@ cdecl -noimport _CIexp() MSVCRT__CIexp
@ cdecl -noimport _CIfmod() MSVCRT__CIfmod
@ cdecl -noimport _CIlog() MSVCRT__CIlog
@ cdecl -noimport _CIlog10() MSVCRT__CIlog10
@ cdecl -noimport _CIpow() MSVCRT__CIpow
@ cdecl -noimport _CIsin() MSVCRT__CIsin
@ cdecl -noimport _CIsinh() MSVCRT__CIsinh
@ cdecl -noimport _CIsqrt() MSVCRT__CIsqrt
@ cdecl -noimport _CItan() MSVCRT__CItan
@ cdecl -noimport _CItanh() MSVCRT__CItanh
@ stub _CxxThrowException
@ cdecl -i386 -norelay _EH_prolog() MSVCRT__EH_prolog
@ cdecl _Getdays() MSVCRT__Getdays
@ cdecl _Getmonths() MSVCRT__Getmonths
@ cdecl _Getnames() MSVCRT__Getnames
@ extern _HUGE MSVCRT__HUGE
@ cdecl _Strftime(str long str ptr ptr) MSVCRT__Strftime
@ cdecl _XcptFilter(long ptr) MSVCRT__XcptFilter
@ stub __CxxFrameHandler
@ stub __CxxLongjmpUnwind
@ stub __RTCastToVoid
@ stub __RTDynamicCast
@ stub __RTtypeid
@ stub __STRINGTOLD
@ extern __argc MSVCRT___argc
@ extern __argv MSVCRT___argv
@ stub __badioinfo
@ stub __crtCompareStringA
@ stub __crtGetLocaleInfoW
@ stub __crtLCMapStringA
@ cdecl __dllonexit() MSVCRT___dllonexit
@ cdecl __doserrno() MSVCRT___doserrno
@ stub __fpecode #()
@ cdecl __getmainargs(ptr ptr ptr long) MSVCRT___getmainargs
@ extern __initenv MSVCRT___initenv
@ cdecl __isascii(long) MSVCRT___isascii
@ cdecl __iscsym(long) MSVCRT___iscsym
@ cdecl __iscsymf(long) MSVCRT___iscsymf
@ stub __lc_codepage
@ stub __lc_collate
@ stub __lc_handle
@ stub __lconv_init
@ extern __mb_cur_max MSVCRT___mb_cur_max
@ cdecl __p___argc() MSVCRT___p___argc
@ cdecl __p___argv() MSVCRT___p___argv
@ cdecl __p___initenv() MSVCRT___p___initenv
@ cdecl __p___mb_cur_max() MSVCRT___p___mb_cur_max
@ cdecl __p___wargv() MSVCRT___p___wargv
@ cdecl __p___winitenv() MSVCRT___p___winitenv
@ cdecl __p__acmdln()  MSVCRT___p__acmdln
@ stub __p__amblksiz #()
@ cdecl __p__commode()  MSVCRT___p__commode
@ stub __p__daylight #()
@ stub __p__dstbias #()
@ cdecl __p__environ() MSVCRT___p__environ
@ stub __p__fileinfo #()
@ cdecl __p__fmode() MSVCRT___p__fmode
@ cdecl __p__iob() MSVCRT___p__iob
@ stub __p__mbcasemap #()
@ cdecl __p__mbctype() MSVCRT___p__mbctype
@ cdecl __p__osver()  MSVCRT___p__osver
@ cdecl __p__pctype() MSVCRT___p__pctype
@ stub __p__pgmptr #()
@ stub __p__pwctype #()
@ cdecl __p__timezone() MSVCRT___p__timezone
@ stub __p__tzname #()
@ cdecl __p__wcmdln() MSVCRT___p__wcmdln
@ cdecl __p__wenviron() MSVCRT___p__wenviron
@ cdecl __p__winmajor() MSVCRT___p__winmajor
@ cdecl __p__winminor() MSVCRT___p__winminor
@ cdecl __p__winver() MSVCRT___p__winver
@ stub __p__wpgmptr #()
@ stub __pioinfo #()
@ stub __pxcptinfoptrs #()
@ cdecl __set_app_type(long) MSVCRT___set_app_type
@ extern __setlc_active MSVCRT___setlc_active
@ cdecl __setusermatherr(ptr) MSVCRT___setusermatherr
@ forward __threadhandle kernel32.GetCurrentThread
@ forward __threadid kernel32.GetCurrentThreadId
@ cdecl __toascii(long) MSVCRT___toascii
@ cdecl __unDName() MSVCRT___unDName #FIXME
@ cdecl __unDNameEx() MSVCRT___unDNameEx #FIXME
@ extern __unguarded_readlc_active MSVCRT___unguarded_readlc_active
@ extern __wargv MSVCRT___wargv
@ cdecl __wgetmainargs(ptr ptr ptr long) MSVCRT___wgetmainargs
@ extern __winitenv MSVCRT___winitenv
@ cdecl _abnormal_termination() MSVCRT__abnormal_termination
@ cdecl _access(str long) MSVCRT__access
@ extern _acmdln MSVCRT__acmdln
@ cdecl _adj_fdiv_m16i() MSVCRT__adj_fdiv_m16i
@ cdecl _adj_fdiv_m32() MSVCRT__adj_fdiv_m32
@ cdecl _adj_fdiv_m32i() MSVCRT__adj_fdiv_m32i
@ cdecl _adj_fdiv_m64() MSVCRT__adj_fdiv_m64
@ cdecl _adj_fdiv_r() MSVCRT__adj_fdiv_r
@ cdecl _adj_fdivr_m16i() MSVCRT__adj_fdivr_m16i
@ cdecl _adj_fdivr_m32() MSVCRT__adj_fdivr_m32
@ cdecl _adj_fdivr_m32i() MSVCRT__adj_fdivr_m32i
@ cdecl _adj_fdivr_m64() MSVCRT__adj_fdivr_m64
@ cdecl _adj_fpatan() MSVCRT__adj_fpatan
@ cdecl _adj_fprem() MSVCRT__adj_fprem
@ cdecl _adj_fprem1() MSVCRT__adj_fprem1
@ cdecl _adj_fptan() MSVCRT__adj_fptan
@ cdecl _adjust_fdiv() MSVCRT__adjust_fdiv
@ stub _aexit_rtn
@ cdecl _amsg_exit(long) MSVCRT__amsg_exit
@ cdecl _assert(str str long) MSVCRT__assert
@ stub _atodbl
@ stub _atoi64 #(str)
@ stub _atoldbl 
@ cdecl _beep(long long) MSVCRT__beep
@ stub _beginthread #(ptr long ptr)
@ cdecl _beginthreadex (ptr long ptr ptr long ptr) MSVCRT__beginthreadex
@ cdecl _c_exit() MSVCRT__c_exit
@ cdecl _cabs(long) MSVCRT__cabs
@ stub _callnewh
@ cdecl _cexit() MSVCRT__cexit
@ cdecl _cgets(str) MSVCRT__cgets
@ cdecl _chdir(str) MSVCRT__chdir
@ cdecl _chdrive(long) MSVCRT__chdrive
@ cdecl _chgsign( double ) MSVCRT__chgsign
@ cdecl -i386 _chkesp() MSVCRT__chkesp
@ cdecl _chmod(str long) MSVCRT__chmod
@ stub _chsize #(long long)
@ cdecl _clearfp() MSVCRT__clearfp
@ cdecl _close(long) MSVCRT__close
@ cdecl _commit(long) MSVCRT__commit
@ extern _commode MSVCRT__commode
@ cdecl _control87(long long) MSVCRT__control87
@ cdecl _controlfp(long long) MSVCRT__controlfp
@ cdecl _copysign( double double ) MSVCRT__copysign
@ varargs _cprintf(str) MSVCRT__cprintf
@ cdecl _cputs(str) MSVCRT__cputs
@ cdecl _creat(str long) MSVCRT__creat
@ varargs _cscanf(str) MSVCRT__cscanf
@ extern _ctype MSVCRT__ctype
@ cdecl _cwait(ptr long long) MSVCRT__cwait
@ stub _daylight
@ stub _dstbias
@ stub _dup #(long)
@ stub _dup2 #(long long)
@ cdecl _ecvt( double long ptr ptr) ecvt
@ stub _endthread #()
@ cdecl _endthreadex(long) MSVCRT__endthreadex
@ extern _environ MSVCRT__environ
@ cdecl _eof(long) MSVCRT__eof
@ cdecl _errno() MSVCRT__errno
@ cdecl _except_handler2(ptr ptr ptr ptr) MSVCRT__except_handler2
@ cdecl _except_handler3(ptr ptr ptr ptr) MSVCRT__except_handler3
@ stub _execl #(str str) varargs
@ stub _execle #(str str) varargs
@ stub _execlp #(str str) varargs
@ stub _execlpe #(str str) varargs
@ stub _execv #(str str)
@ stub _execve #(str str str)
@ stub _execvp #(str str)
@ stub _execvpe #(str str str)
@ cdecl _exit(long) MSVCRT__exit
@ cdecl _expand(ptr long) MSVCRT__expand
@ cdecl _fcloseall() MSVCRT__fcloseall
@ cdecl _fcvt( double long ptr ptr) fcvt
@ cdecl _fdopen(long str) MSVCRT__fdopen
@ cdecl _fgetchar() MSVCRT__fgetchar
@ cdecl _fgetwchar() MSVCRT__fgetwchar
@ cdecl _filbuf(ptr) MSVCRT__filbuf
@ stub _fileinfo
@ cdecl _filelength(long) MSVCRT__filelength
@ stub _filelengthi64 #(long)
@ cdecl _fileno(ptr) MSVCRT__fileno
@ cdecl _findclose(long) MSVCRT__findclose
@ cdecl _findfirst(str ptr) MSVCRT__findfirst
@ stub _findfirsti64 #(str ptr)
@ cdecl _findnext(long ptr) MSVCRT__findnext
@ stub _findnexti64 #(long ptr)
@ cdecl _finite( double ) MSVCRT__finite
@ cdecl _flsbuf(long ptr) MSVCRT__flsbuf
@ cdecl _flushall() MSVCRT__flushall
@ extern _fmode MSVCRT__fmode
@ cdecl _fpclass(double) MSVCRT__fpclass
@ stub _fpieee_flt
@ cdecl _fpreset() MSVCRT__fpreset
@ cdecl _fputchar(long) MSVCRT__fputchar
@ cdecl _fputwchar(long) MSVCRT__fputwchar
@ cdecl _fsopen(str str) MSVCRT__fsopen
@ cdecl _fstat(long ptr) MSVCRT__fstat
@ stub _fstati64 #(long ptr)
@ cdecl _ftime(ptr) MSVCRT__ftime
@ forward _ftol ntdll._ftol
@ cdecl _fullpath(str str long) MSVCRT__fullpath
@ cdecl _futime() MSVCRT__futime
@ cdecl _gcvt( double long str) gcvt
@ cdecl _get_osfhandle(long) MSVCRT__get_osfhandle
@ stub _get_sbh_threshold #()
@ cdecl _getch() MSVCRT__getch
@ cdecl _getche() MSVCRT__getche
@ cdecl _getcwd(str long) MSVCRT__getcwd
@ cdecl _getdcwd(long str long) MSVCRT__getdcwd
@ cdecl _getdiskfree(long ptr) MSVCRT__getdiskfree
@ forward _getdllprocaddr kernel32.GetProcAddress
@ cdecl _getdrive() MSVCRT__getdrive
@ forward _getdrives kernel32.GetLogicalDrives
@ stub _getmaxstdio #()
@ cdecl _getmbcp() MSVCRT__getmbcp
@ forward _getpid kernel32.GetCurrentProcessId
@ stub _getsystime #(ptr)
@ cdecl _getw(ptr) MSVCRT__getw
@ stub _getws #(wstr)
@ cdecl _global_unwind2(ptr) MSVCRT__global_unwind2
@ stub _heapadd #()
@ cdecl _heapchk() MSVCRT__heapchk
@ cdecl _heapmin() MSVCRT__heapmin
@ cdecl _heapset(long) MSVCRT__heapset
@ stub _heapused #(ptr ptr)
@ cdecl _heapwalk(ptr) MSVCRT__heapwalk
@ cdecl _hypot(double double) hypot
@ stub _i64toa #(long str long)
@ stub _i64tow #(long wstr long)
@ cdecl _initterm(ptr ptr) MSVCRT__initterm
@ stub -i386 _inp #(long)
@ stub -i386 _inpd #(long)
@ stub -i386 _inpw #(long)
@ extern _iob MSVCRT__iob
@ cdecl _isatty(long) MSVCRT__isatty
@ cdecl _isctype(long long) MSVCRT__isctype
@ stub _ismbbalnum #(long)
@ stub _ismbbalpha #(long)
@ stub _ismbbgraph #(long)
@ stub _ismbbkalnum #(long)
@ cdecl _ismbbkana(long) MSVCRT__ismbbkana
@ stub _ismbbkprint #(long)
@ stub _ismbbkpunct #(long)
@ cdecl _ismbblead(long) MSVCRT__ismbblead
@ stub _ismbbprint #(long)
@ stub _ismbbpunct #(long)
@ cdecl _ismbbtrail(long) MSVCRT__ismbbtrail
@ stub _ismbcalnum #(long)
@ stub _ismbcalpha #(long)
@ stub _ismbcdigit #(long)
@ stub _ismbcgraph #(long)
@ cdecl _ismbchira(long) MSVCRT__ismbchira
@ cdecl _ismbckata(long) MSVCRT__ismbckata
@ stub _ismbcl0 #(long)
@ stub _ismbcl1 #(long)
@ stub _ismbcl2 #(long)
@ stub _ismbclegal #(long)
@ stub _ismbclower #(long)
@ stub _ismbcprint #(long)
@ stub _ismbcpunct #(long)
@ stub _ismbcspace #(long)
@ stub _ismbcsymbol #(long)
@ stub _ismbcupper #(long)
@ cdecl _ismbslead(ptr ptr) MSVCRT__ismbslead
@ cdecl _ismbstrail(ptr ptr) MSVCRT__ismbstrail
@ cdecl _isnan( double ) MSVCRT__isnan
@ forward _itoa ntdll._itoa
@ cdecl _itow(long wstr long) MSVCRT__itow
@ cdecl _j0(double) j0
@ cdecl _j1(double) j1
@ cdecl _jn(long double) jn
@ cdecl _kbhit() MSVCRT__kbhit
@ stub _lfind
@ cdecl _loaddll(str) MSVCRT__loaddll
@ cdecl _local_unwind2(ptr long) MSVCRT__local_unwind2
@ stub _lock
@ stub _locking #(long long long)
@ cdecl _logb( double ) MSVCRT__logb
@ stub _longjmpex
@ cdecl _lrotl(long long) MSVCRT__lrotl
@ cdecl _lrotr(long long) MSVCRT__lrotr
@ cdecl _lsearch(ptr ptr long long ptr) MSVCRT__lsearch
@ cdecl _lseek(long long long) MSVCRT__lseek
@ stub _lseeki64 #(long long long)
@ forward _ltoa ntdll._ltoa
@ forward _ltow ntdll._ltow
@ cdecl _makepath(str str str str str) MSVCRT__makepath
@ cdecl _mbbtombc(long) MSVCRT__mbbtombc
@ stub _mbbtype #(long long)
@ stub _mbcasemap
@ cdecl _mbccpy (str str) strcpy
@ stub _mbcjistojms #(long)
@ stub _mbcjmstojis #(long)
@ cdecl _mbclen(ptr) MSVCRT__mbclen
@ stub _mbctohira #(long)
@ stub _mbctokata #(long)
@ stub _mbctolower #(long)
@ stub _mbctombb #(long)
@ stub _mbctoupper #(long)
@ stub _mbctype
@ stub _mbsbtype #(ptr long)
@ cdecl _mbscat(str str) strcat
@ cdecl _mbschr(str long) MSVCRT__mbschr
@ cdecl _mbscmp(str str) MSVCRT__mbscmp
@ stub _mbscoll #(str str)
@ cdecl _mbscpy(ptr str) strcpy
@ stub _mbscspn #(str str)
@ cdecl _mbsdec(ptr ptr) MSVCRT__mbsdec
@ cdecl _mbsdup(str) MSVCRT__strdup
@ cdecl _mbsicmp(str str) MSVCRT__mbsicmp
@ stub _mbsicoll #(str str)
@ cdecl _mbsinc(str) MSVCRT__mbsinc
@ cdecl _mbslen(str) MSVCRT__mbslen
@ stub _mbslwr #(str)
@ stub _mbsnbcat #(str str long)
@ stub _mbsnbcmp #(str str long)
@ stub _mbsnbcnt #(ptr long)
@ stub _mbsnbcoll #(str str long)
@ stub _mbsnbcpy #(ptr str long)
@ stub _mbsnbicmp #(str str long)
@ stub _mbsnbicoll #(str str long)
@ stub _mbsnbset #(str long long)
@ cdecl _mbsncat(str str long) MSVCRT__mbsncat
@ cdecl _mbsnccnt(str long) MSVCRT__mbsnccnt
@ cdecl _mbsncmp(str str long) MSVCRT__mbsncmp
@ stub _mbsncoll #(ptr str long)
@ cdecl _mbsncpy(str str long) MSVCRT__mbsncpy
@ cdecl _mbsnextc(str) MSVCRT__mbsnextc
@ cdecl _mbsnicmp(str str long) MSVCRT__mbsnicmp
@ stub _mbsnicoll #(str str long)
@ cdecl _mbsninc(str long) MSVCRT__mbsninc
@ cdecl _mbsnset(str long long) MSVCRT__mbsnset
@ stub _mbspbrk #(str str)
@ cdecl _mbsrchr(str long) MSVCRT__mbsrchr
@ stub _mbsrev #(str)
@ cdecl _mbsset(str long) MSVCRT__mbsset
@ stub _mbsspn #(str str)
@ stub _mbsspnp #(str str)
@ cdecl _mbsstr(str str) strstr
@ stub _mbstok #(str str)
@ cdecl _mbstrlen(str) MSVCRT__mbstrlen
@ stub _mbsupr #(str)
@ cdecl _memccpy(ptr ptr long long) memccpy
@ forward _memicmp ntdll._memicmp
@ cdecl _mkdir(str) MSVCRT__mkdir
@ cdecl _mktemp(str) MSVCRT__mktemp
@ cdecl _msize(ptr) MSVCRT__msize
@ cdecl _nextafter(double double) MSVCRT__nextafter
@ cdecl _onexit(ptr) MSVCRT__onexit
@ cdecl _open(str long) MSVCRT__open
@ cdecl _open_osfhandle(long long) MSVCRT__open_osfhandle
@ stub _osver
@ stub _outp #(long long)
@ stub _outpd #(long long)
@ stub _outpw #(long long)
@ stub _pclose #(ptr)
@ extern _pctype MSVCRT__pctype
@ stub _pgmptr
@ stub _pipe #(ptr long long)
@ stub _popen #(str str)
@ cdecl _purecall() MSVCRT__purecall
@ cdecl _putch(long) MSVCRT__putch
@ stub _putenv #(str)
@ cdecl _putw(long ptr) MSVCRT__putw
@ stub _putws #(wstr)
@ stub _pwctype
@ cdecl _read(long ptr long) MSVCRT__read
@ cdecl _rmdir(str) MSVCRT__rmdir
@ cdecl _rmtmp() MSVCRT__rmtmp
@ cdecl _rotl(long long) MSVCRT__rotl
@ cdecl _rotr(long long) MSVCRT__rotr
@ cdecl _safe_fdiv() MSVCRT__safe_fdiv
@ cdecl _safe_fdivr() MSVCRT__safe_fdivr
@ cdecl _safe_fprem() MSVCRT__safe_fprem
@ cdecl _safe_fprem1() MSVCRT__safe_fprem1
@ cdecl _scalb( double long) MSVCRT__scalb
@ cdecl _searchenv(str str str) MSVCRT__searchenv
@ stub _seh_longjmp_unwind
@ stub _set_error_mode #(long)
@ stub _set_sbh_threshold #(long)
@ stub _seterrormode #(long)
@ cdecl _setjmp(ptr) MSVCRT__setjmp
@ stub _setjmp3
@ stub _setmaxstdio #(long)
@ cdecl _setmbcp(long) MSVCRT__setmbcp
@ cdecl _setmode(long long) MSVCRT__setmode
@ stub _setsystime #(ptr long)
@ cdecl _sleep(long) MSVCRT__sleep
@ varargs _snprintf(str long str) snprintf
@ stub _snwprintf #(wstr long wstr) varargs
@ stub _sopen
@ stub _spawnl #(str str) varargs
@ stub _spawnle #(str str) varargs
@ stub _spawnlp #(str str) varargs
@ stub _spawnlpe #(str str) varargs
@ cdecl _spawnv(str str) MSVCRT__spawnv
@ cdecl _spawnve(str str str) MSVCRT__spawnve
@ cdecl _spawnvp(str str) MSVCRT__spawnvp
@ cdecl _spawnvpe(str str str) MSVCRT__spawnvpe
@ cdecl _splitpath(str str str str str) MSVCRT__splitpath
@ cdecl _stat(str ptr) MSVCRT__stat
@ stub _stati64 #(str ptr)
@ cdecl _statusfp() MSVCRT__statusfp
@ cdecl _strcmpi(str str) strcasecmp
@ cdecl _strdate(str) MSVCRT__strdate
@ cdecl _strdup(str) MSVCRT__strdup
@ cdecl _strerror(long) MSVCRT__strerror
@ cdecl _stricmp(str str) strcasecmp
@ stub _stricoll #(str str)
@ forward _strlwr ntdll._strlwr
@ stub _strncoll #(str str long)
@ cdecl _strnicmp(str str long) strncasecmp
@ stub _strnicoll #(str str long)
@ cdecl _strnset(str long long) MSVCRT__strnset
@ cdecl _strrev(str) MSVCRT__strrev
@ cdecl _strset(str long) MSVCRT__strset
@ cdecl _strtime(str) MSVCRT__strtime
@ forward _strupr ntdll._strupr
@ cdecl _swab(str str long) MSVCRT__swab
@ stub _sys_errlist #()
@ stub _sys_nerr #()
@ cdecl _tell(long) MSVCRT__tell
@ stub _telli64 #(long)
@ cdecl _tempnam(str str) MSVCRT__tempnam
@ stub _timezone #()
@ cdecl _tolower(long) MSVCRT__tolower
@ cdecl _toupper(long) MSVCRT__toupper
@ stub _tzname
@ stub _tzset #()
@ stub _ui64toa #(long str long)
@ stub _ui64tow #(long wstr long)
@ forward _ultoa ntdll._ultoa
@ forward _ultow ntdll._ultow
@ cdecl _umask(long) MSVCRT__umask
@ cdecl _ungetch(long) MSVCRT__ungetch
@ cdecl _unlink(str) MSVCRT__unlink
@ cdecl _unloaddll(long) MSVCRT__unloaddll
@ stub _unlock
@ cdecl _utime(str ptr) MSVCRT__utime
@ cdecl _vsnprintf(ptr long ptr ptr) vsnprintf
@ stub _vsnwprintf #(wstr long wstr long)
@ stub _waccess #(wstr long)
@ stub _wasctime #(ptr)
@ cdecl _wchdir(wstr) MSVCRT__wchdir
@ stub _wchmod #(wstr long)
@ extern _wcmdln MSVCRT__wcmdln
@ stub _wcreat #(wstr long)
@ cdecl _wcsdup(wstr) MSVCRT__wcsdup
@ forward _wcsicmp ntdll._wcsicmp
@ cdecl _wcsicoll(wstr wstr) MSVCRT__wcsicoll
@ stub _wcslwr #(wstr)
@ stub _wcsncoll #(wstr wstr long)
@ forward _wcsnicmp ntdll._wcsnicmp
@ stub _wcsnicoll #(wstr wstr long)
@ cdecl _wcsnset(wstr long long) MSVCRT__wcsnset
@ cdecl _wcsrev(wstr) MSVCRT__wcsrev
@ cdecl _wcsset(wstr long) MSVCRT__wcsset
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
@ stub _wfdopen #(long wstr)
@ cdecl _wfindfirst(wstr ptr) MSVCRT__wfindfirst
@ stub _wfindfirsti64 #(wstr ptr)
@ cdecl _wfindnext(long ptr)  MSVCRT__wfindnext
@ stub _wfindnexti64 #(long ptr)
@ stub _wfopen #(wstr wstr)
@ stub _wfreopen #(wstr wstr ptr)
@ stub _wfsopen #(wstr wstr)
@ stub _wfullpath #(wstr wstr long)
@ cdecl _wgetcwd(wstr long) MSVCRT__wgetcwd
@ cdecl _wgetdcwd(long wstr long) MSVCRT__wgetdcwd
@ cdecl _wgetenv(wstr) MSVCRT__wgetenv
@ extern _winmajor MSVCRT__winmajor
@ extern _winminor MSVCRT__winminor
@ extern _winver MSVCRT__winver
@ stub _wmakepath #(wstr wstr wstr wstr wstr)
@ cdecl _wmkdir(wstr) MSVCRT__wmkdir
@ stub _wmktemp #(wstr)
@ stub _wopen #(wstr long) varargs
@ stub _wperror #(wstr)
@ stub _wpgmptr
@ stub _wpopen #(wstr wstr)
@ stub _wputenv #(wstr)
@ stub _wremove #(wstr)
@ stub _wrename #(wstr wstr)
@ cdecl _write(long ptr long) MSVCRT__write
@ cdecl _wrmdir(wstr) MSVCRT__wrmdir
@ stub _wsearchenv #(wstr wstr wstr)
@ stub _wsetlocale #(long wstr)
@ stub _wsopen #(wstr long long) varargs
@ stub _wspawnl #(long wstr wstr) varargs
@ stub _wspawnle #(long wstr wstr) varargs
@ stub _wspawnlp #(long wstr wstr) varargs
@ stub _wspawnlpe #(long wstr wstr) varargs
@ stub _wspawnv #(long wstr wstr)
@ stub _wspawnve #(long wstr wstr wstr)
@ stub _wspawnvp #(long wstr wstr)
@ stub _wspawnvpe #(long wstr wstr wstr)
@ stub _wsplitpath #(wstr wstr wstr wstr wstr)
@ stub _wstat #(wstr ptr)
@ stub _wstati64 #(wstr ptr)
@ stub _wstrdate #(wstr)
@ stub _wstrtime #(wstr)
@ stub _wsystem #(wstr)
@ stub _wtempnam #(wstr wstr)
@ stub _wtmpnam #(wstr)
@ forward _wtoi NTDLL._wtoi
@ stub _wtoi64 #(wstr)
@ forward _wtol NTDLL._wtol
@ stub _wunlink #(wstr)
@ stub _wutime
@ cdecl _y0(double) MSVCRT__y0
@ cdecl _y1(double) MSVCRT__y1
@ cdecl _yn(long double ) MSVCRT__yn
@ cdecl abort() MSVCRT_abort
@ cdecl abs(long) abs
@ cdecl acos( double ) acos
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
@ cdecl -noimport div(long long) MSVCRT_div
@ cdecl exit(long) MSVCRT_exit
@ cdecl exp(double) exp
@ cdecl fabs(double) fabs
@ cdecl fclose(ptr) MSVCRT_fclose
@ cdecl feof(ptr) MSVCRT_feof
@ cdecl ferror(ptr) MSVCRT_ferror
@ cdecl fflush(ptr) MSVCRT_fflush
@ cdecl fgetc(ptr) MSVCRT_fgetc
@ cdecl fgetpos(ptr ptr) MSVCRT_fgetpos
@ cdecl fgets(str long ptr) MSVCRT_fgets
@ cdecl fgetwc(ptr) MSVCRT_fgetwc
@ stub fgetws #(wstr long ptr)
@ cdecl floor(double) floor
@ cdecl fmod(double double) fmod
@ cdecl fopen(str str) MSVCRT_fopen
@ varargs fprintf(ptr str) MSVCRT_fprintf
@ cdecl fputc(long ptr) MSVCRT_fputc
@ cdecl fputs(str ptr) MSVCRT_fputs
@ cdecl fputwc(long ptr) MSVCRT_fputwc
@ stub fputws #(wstr ptr)
@ cdecl fread() MSVCRT_fread
@ cdecl free() MSVCRT_free
@ cdecl freopen(str str ptr) MSVCRT_freopen
@ cdecl frexp(double ptr) frexp
@ varargs fscanf(ptr str) MSVCRT_fscanf
@ cdecl fseek(ptr long long) MSVCRT_fseek
@ cdecl fsetpos(ptr ptr) MSVCRT_fsetpos
@ cdecl ftell(ptr) MSVCRT_ftell
@ stub fwprintf #(ptr wstr) varargs
@ cdecl fwrite(ptr long long ptr) MSVCRT_fwrite
@ stub fwscanf #(ptr wstr) varargs
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
@ forward iswalpha ntdll._iswalpha
@ cdecl iswascii(long) MSVCRT_iswascii
@ cdecl iswcntrl(long) MSVCRT_iswcntrl
@ forward iswctype ntdll._iswctype
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
@ cdecl -noimport ldiv(long long) MSVCRT_ldiv
@ stub localeconv #()
@ cdecl localtime(ptr) localtime
@ cdecl log(double) log
@ cdecl log10(double) log10
@ cdecl longjmp(long long) MSVCRT_longjmp
@ cdecl malloc(long) MSVCRT_malloc
@ stub mblen #(str long)
@ forward mbstowcs ntdll.mbstowcs
@ cdecl mbtowc(wstr str long) MSVCRT_mbtowc
@ cdecl memchr(ptr long long) memchr
@ cdecl memcmp(ptr ptr long) memcmp
@ cdecl memcpy(ptr ptr long) memcpy
@ cdecl memmove(ptr ptr long) memmove
@ cdecl memset(ptr long long) memset
@ cdecl mktime(ptr) mktime
@ cdecl modf(double ptr) modf
@ cdecl perror(str) MSVCRT_perror
@ cdecl pow(double double) pow
@ varargs printf(str) MSVCRT_printf
@ cdecl putc(long ptr) MSVCRT_putc
@ cdecl putchar(long) MSVCRT_putchar
@ cdecl puts(str) MSVCRT_puts
@ stub putwc #(long ptr)
@ stub putwchar #(long)
@ cdecl qsort(ptr long long ptr) qsort
@ stub raise #(long)
@ cdecl rand() MSVCRT_rand
@ cdecl realloc() MSVCRT_realloc
@ cdecl remove(str) MSVCRT_remove
@ cdecl rename(str str) MSVCRT_rename
@ cdecl rewind(ptr) MSVCRT_rewind
@ varargs scanf(str) MSVCRT_scanf
@ cdecl setbuf(ptr ptr) MSVCRT_setbuf
@ cdecl setlocale(long str) MSVCRT_setlocale
@ stub setvbuf #(ptr str long long)
@ cdecl signal(long long) MSVCRT_signal
@ cdecl sin(double) sin
@ cdecl sinh(double) sinh
@ varargs sprintf(str str) sprintf
@ cdecl sqrt(double) sqrt
@ cdecl srand(long) srand
@ varargs sscanf(str str) sscanf
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
@ stub swprintf #(wstr wstr) varargs
@ stub swscanf #(wstr wstr) varargs
@ cdecl system(str) MSVCRT_system
@ cdecl tan(double) tan
@ cdecl tanh(double) tanh
@ cdecl time(ptr) MSVCRT_time
@ cdecl tmpfile() MSVCRT_tmpfile
@ cdecl tmpnam(str) MSVCRT_tmpnam
@ forward towlower ntdll.towlower
@ forward towupper ntdll.towupper
@ stub ungetc #(long ptr)
@ stub ungetwc #(long ptr)
@ cdecl vfprintf(ptr str long) MSVCRT_vfprintf
@ stub vfwprintf #(ptr wstr long)
@ stub vprintf #(str long)
@ cdecl vsprintf(ptr str ptr) vsprintf
@ stub vswprintf #(wstr wstr long)
@ stub vwprintf #(wstr long)
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
@ forward  wcsncpy ntdll.wcsncpy
@ cdecl wcspbrk(wstr wstr) MSVCRT_wcspbrk
@ forward wcsrchr ntdll.wcsrchr
@ forward wcsspn ntdll.wcsspn
@ forward wcsstr ntdll.wcsstr
@ stub wcstod #(wstr ptr)
@ forward wcstok ntdll.wcstok
@ forward wcstol ntdll.wcstol
@ forward wcstombs ntdll.wcstombs
@ stub wcstoul #(wstr ptr long)
@ stub wcsxfrm #(wstr wstr long)
@ cdecl wctomb(ptr long) MSVCRT_wctomb
@ stub wprintf #(wstr) varargs
@ stub wscanf #(wstr) varargs
