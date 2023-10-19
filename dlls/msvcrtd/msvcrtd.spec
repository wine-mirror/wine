# msvcrtd.dll - MS VC++ Run Time Library

@ cdecl -norelay $I10_OUTPUT(double long long ptr) I10_OUTPUT
@ thiscall -arch=i386 ??0__non_rtti_object@@QAE@ABV0@@Z(ptr ptr) __non_rtti_object_copy_ctor
@ cdecl -arch=win64 ??0__non_rtti_object@@QEAA@AEBV0@@Z(ptr ptr) __non_rtti_object_copy_ctor
@ thiscall -arch=i386 ??0__non_rtti_object@@QAE@PBD@Z(ptr ptr) __non_rtti_object_ctor
@ cdecl -arch=win64 ??0__non_rtti_object@@QEAA@PEBD@Z(ptr ptr) __non_rtti_object_ctor
@ thiscall -arch=win32 ??0bad_cast@@QAE@ABQBD@Z(ptr ptr) bad_cast_ctor
@ cdecl -arch=win64 ??0bad_cast@@AEAA@PEBQEBD@Z(ptr ptr) bad_cast_ctor
@ thiscall -arch=i386 ??0bad_cast@@QAE@ABV0@@Z(ptr ptr) bad_cast_copy_ctor
@ cdecl -arch=win64 ??0bad_cast@@QEAA@AEBQEBD@Z(ptr ptr) bad_cast_ctor
@ thiscall -arch=i386 ??0bad_typeid@@QAE@ABV0@@Z(ptr ptr) bad_typeid_copy_ctor
@ cdecl -arch=win64 ??0bad_typeid@@QEAA@AEBV0@@Z(ptr ptr) bad_typeid_copy_ctor
@ thiscall -arch=i386 ??0bad_typeid@@QAE@PBD@Z(ptr str) bad_typeid_ctor
@ cdecl -arch=win64 ??0bad_typeid@@QEAA@PEBD@Z(ptr str) bad_typeid_ctor
@ thiscall -arch=i386 ??0exception@@QAE@ABQBD@Z(ptr ptr) exception_ctor
@ cdecl -arch=win64 ??0exception@@QEAA@AEBQEBD@Z(ptr ptr) exception_ctor
@ thiscall -arch=i386 ??0exception@@QAE@ABV0@@Z(ptr ptr) exception_copy_ctor
@ cdecl -arch=win64 ??0exception@@QEAA@AEBV0@@Z(ptr ptr) exception_copy_ctor
@ thiscall -arch=i386 ??0exception@@QAE@XZ(ptr) exception_default_ctor
@ cdecl -arch=win64 ??0exception@@QEAA@XZ(ptr) exception_default_ctor
@ thiscall -arch=i386 ??1__non_rtti_object@@UAE@XZ(ptr) __non_rtti_object_dtor
@ cdecl -arch=win64 ??1__non_rtti_object@@UEAA@XZ(ptr) __non_rtti_object_dtor
@ thiscall -arch=i386 ??1bad_cast@@UAE@XZ(ptr) bad_cast_dtor
@ cdecl -arch=win64 ??1bad_cast@@UEAA@XZ(ptr) bad_cast_dtor
@ thiscall -arch=i386 ??1bad_typeid@@UAE@XZ(ptr) bad_typeid_dtor
@ cdecl -arch=win64 ??1bad_typeid@@UEAA@XZ(ptr) bad_typeid_dtor
@ thiscall -arch=i386 ??1exception@@UAE@XZ(ptr) exception_dtor
@ cdecl -arch=win64 ??1exception@@UEAA@XZ(ptr) exception_dtor
@ thiscall -arch=i386 ??1type_info@@UAE@XZ(ptr) type_info_dtor
@ cdecl -arch=win64 ??1type_info@@UEAA@XZ(ptr) type_info_dtor
@ cdecl -arch=win32 ??2@YAPAXI@Z(long) operator_new
@ cdecl -arch=win64 ??2@YAPEAX_K@Z(long) operator_new
@ cdecl -arch=win32 ??2@YAPAXIHPBDH@Z(long long str long) operator_new_dbg
@ cdecl -arch=win64 ??2@YAPEAX_KHPEBDH@Z(long long str long) operator_new_dbg
@ cdecl -arch=win32 ??3@YAXPAX@Z(ptr) operator_delete
@ cdecl -arch=win64 ??3@YAXPEAX@Z(ptr) operator_delete
@ thiscall -arch=i386 ??4__non_rtti_object@@QAEAAV0@ABV0@@Z(ptr ptr) __non_rtti_object_opequals
@ cdecl -arch=win64 ??4__non_rtti_object@@QEAAAEAV0@AEBV0@@Z(ptr ptr) __non_rtti_object_opequals
@ thiscall -arch=i386 ??4bad_cast@@QAEAAV0@ABV0@@Z(ptr ptr) bad_cast_opequals
@ cdecl -arch=win64 ??4bad_cast@@QEAAAEAV0@AEBV0@@Z(ptr ptr) bad_cast_opequals
@ thiscall -arch=i386 ??4bad_typeid@@QAEAAV0@ABV0@@Z(ptr ptr) bad_typeid_opequals
@ cdecl -arch=win64 ??4bad_typeid@@QEAAAEAV0@AEBV0@@Z(ptr ptr) bad_typeid_opequals
@ thiscall -arch=i386 ??4exception@@QAEAAV0@ABV0@@Z(ptr ptr) exception_opequals
@ cdecl -arch=win64 ??4exception@@QEAAAEAV0@AEBV0@@Z(ptr ptr) exception_opequals
@ thiscall -arch=i386 ??8type_info@@QBEHABV0@@Z(ptr ptr) type_info_opequals_equals
@ cdecl -arch=win64 ??8type_info@@QEBAHAEBV0@@Z(ptr ptr) type_info_opequals_equals
@ thiscall -arch=i386 ??9type_info@@QBEHABV0@@Z(ptr ptr) type_info_opnot_equals
@ cdecl -arch=win64 ??9type_info@@QEBAHAEBV0@@Z(ptr ptr) type_info_opnot_equals
@ extern ??_7__non_rtti_object@@6B@ __non_rtti_object_vtable
@ extern ??_7bad_cast@@6B@ bad_cast_vtable
@ extern ??_7bad_typeid@@6B@ bad_typeid_vtable
@ extern ??_7exception@@6B@ exception_vtable
@ thiscall -arch=win32 ??_E__non_rtti_object@@UAEPAXI@Z(ptr long) __non_rtti_object_vector_dtor
@ thiscall -arch=win32 ??_Ebad_cast@@UAEPAXI@Z(ptr long) bad_cast_vector_dtor
@ thiscall -arch=win32 ??_Ebad_typeid@@UAEPAXI@Z(ptr long) bad_typeid_vector_dtor
@ thiscall -arch=win32 ??_Eexception@@UAEPAXI@Z(ptr long) exception_vector_dtor
@ thiscall -arch=win32 ??_G__non_rtti_object@@UAEPAXI@Z(ptr long) __non_rtti_object_scalar_dtor
@ thiscall -arch=win32 ??_Gbad_cast@@UAEPAXI@Z(ptr long) bad_cast_scalar_dtor
@ thiscall -arch=win32 ??_Gbad_typeid@@UAEPAXI@Z(ptr long) bad_typeid_scalar_dtor
@ thiscall -arch=win32 ??_Gexception@@UAEPAXI@Z(ptr long) exception_scalar_dtor
@ cdecl -arch=win32 ?_query_new_handler@@YAP6AHI@ZXZ() _query_new_handler
@ cdecl -arch=win64 ?_query_new_handler@@YAP6AH_K@ZXZ() _query_new_handler
@ cdecl ?_query_new_mode@@YAHXZ() _query_new_mode
@ cdecl -arch=win32 ?_set_new_handler@@YAP6AHI@ZP6AHI@Z@Z(ptr) _set_new_handler
@ cdecl -arch=win64 ?_set_new_handler@@YAP6AH_K@ZP6AH0@Z@Z(ptr) _set_new_handler
@ cdecl ?_set_new_mode@@YAHH@Z(long) _set_new_mode
@ cdecl -arch=win32 ?_set_se_translator@@YAP6AXIPAU_EXCEPTION_POINTERS@@@ZP6AXI0@Z@Z(ptr) _set_se_translator
@ cdecl -arch=win64 ?_set_se_translator@@YAP6AXIPEAU_EXCEPTION_POINTERS@@@ZP6AXI0@Z@Z(ptr) _set_se_translator
@ thiscall -arch=i386 ?before@type_info@@QBEHABV1@@Z(ptr ptr) type_info_before
@ cdecl -arch=win64 ?before@type_info@@QEBAHAEBV1@@Z(ptr ptr) type_info_before
@ thiscall -arch=win32 ?name@type_info@@QBEPBDXZ(ptr) type_info_name
@ cdecl -arch=win64 ?name@type_info@@QEBAPEBDXZ(ptr) type_info_name
@ thiscall -arch=i386 ?raw_name@type_info@@QBEPBDXZ(ptr) type_info_raw_name
@ cdecl -arch=win64 ?raw_name@type_info@@QEBAPEBDXZ(ptr) type_info_raw_name
@ cdecl ?set_new_handler@@YAP6AXXZP6AXXZ@Z(ptr) set_new_handler
@ cdecl ?set_terminate@@YAP6AXXZP6AXXZ@Z(ptr) set_terminate
@ cdecl ?set_unexpected@@YAP6AXXZP6AXXZ@Z(ptr) set_unexpected
@ cdecl ?terminate@@YAXXZ() terminate
@ cdecl ?unexpected@@YAXXZ() unexpected
@ thiscall -arch=i386 ?what@exception@@UBEPBDXZ(ptr) exception_what
@ cdecl -arch=win64 ?what@exception@@UEBAPEBDXZ(ptr) exception_what
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
@ cdecl _CrtCheckMemory()
@ stub _CrtDbgBreak
@ varargs _CrtDbgReport(long ptr long ptr ptr)
@ stub _CrtDoForAllClientObjects
@ cdecl _CrtDumpMemoryLeaks()
@ stub _CrtIsMemoryBlock
@ stub _CrtIsValidHeapPointer
@ stub _CrtIsValidPointer
@ stub _CrtMemCheckpoint
@ stub _CrtMemDifference
@ stub _CrtMemDumpAllObjectsSince
@ stub _CrtMemDumpStatistics
@ stub _CrtSetAllocHook
@ cdecl _CrtSetBreakAlloc(long)
@ stub _CrtSetDbgBlockType
@ cdecl _CrtSetDbgFlag(long)
@ cdecl _CrtSetDumpClient(ptr)
@ stub _CrtSetReportFile
@ cdecl _CrtSetReportHook(ptr)
@ cdecl _CrtSetReportMode(long long)
@ stdcall _CxxThrowException(ptr ptr)
@ cdecl -arch=i386 -norelay _EH_prolog()
@ cdecl _Getdays()
@ cdecl _Getmonths()
@ cdecl _Gettnames()
@ extern _HUGE MSVCRT__HUGE
@ cdecl _Strftime(ptr long str ptr ptr)
@ cdecl _XcptFilter(long ptr)
@ cdecl -norelay __CxxFrameHandler(ptr ptr ptr ptr)
@ stdcall -arch=i386 __CxxLongjmpUnwind(ptr)
@ cdecl __RTCastToVoid(ptr)
@ cdecl __RTDynamicCast(ptr long ptr ptr long)
@ cdecl __RTtypeid(ptr)
@ cdecl __STRINGTOLD(ptr ptr str long)
@ cdecl ___mb_cur_max_func()
@ extern __argc MSVCRT___argc
@ extern __argv MSVCRT___argv
@ extern __badioinfo MSVCRT___badioinfo
@ cdecl __crtCompareStringA(long long str long str long)
@ cdecl __crtGetLocaleInfoW(long long ptr long)
@ cdecl __crtLCMapStringA(long long str long ptr long long long)
@ cdecl __dllonexit(ptr ptr ptr)
@ cdecl __doserrno()
@ cdecl __fpecode()
@ cdecl __getmainargs(ptr ptr ptr long ptr)
@ extern __initenv MSVCRT___initenv
@ cdecl __isascii(long)
@ cdecl __iscsym(long)
@ cdecl __iscsymf(long)
@ extern __lc_codepage MSVCRT___lc_codepage
@ extern __lc_collate_cp MSVCRT___lc_collate_cp
@ extern __lc_handle MSVCRT___lc_handle
@ cdecl __lconv_init()
@ extern __mb_cur_max MSVCRT___mb_cur_max
@ cdecl __p___argc()
@ cdecl __p___argv()
@ cdecl __p___initenv()
@ cdecl __p___mb_cur_max()
@ cdecl __p___wargv()
@ cdecl __p___winitenv()
@ cdecl __p__acmdln()
@ cdecl __p__amblksiz()
@ cdecl __p__commode()
@ cdecl __p__crtAssertBusy()
@ cdecl __p__crtBreakAlloc()
@ cdecl __p__crtDbgFlag()
@ cdecl __p__daylight()
@ cdecl __p__dstbias()
@ cdecl __p__environ()
@ stub __p__fileinfo()
@ cdecl __p__fmode()
@ cdecl __p__iob() __iob_func
@ stub __p__mbcasemap()
@ cdecl __p__mbctype()
@ cdecl __p__osver()
@ cdecl __p__pctype()
@ cdecl __p__pgmptr()
@ cdecl __p__pwctype()
@ cdecl __p__timezone()
@ cdecl __p__tzname()
@ cdecl __p__wcmdln()
@ cdecl __p__wenviron()
@ cdecl __p__winmajor()
@ cdecl __p__winminor()
@ cdecl __p__winver()
@ cdecl __p__wpgmptr()
@ cdecl __pctype_func()
@ extern __pioinfo MSVCRT___pioinfo
@ cdecl __pwctype_func()
@ cdecl __pxcptinfoptrs()
@ cdecl __set_app_type(long)
@ extern __setlc_active MSVCRT___setlc_active
@ cdecl __setusermatherr(ptr)
@ cdecl __threadhandle() kernel32.GetCurrentThread
@ cdecl __threadid() kernel32.GetCurrentThreadId
@ cdecl __toascii(long)
@ cdecl __unDName(ptr str long ptr ptr long)
@ cdecl __unDNameEx(ptr str long ptr ptr ptr long)
@ extern __unguarded_readlc_active MSVCRT___unguarded_readlc_active
@ extern __wargv MSVCRT___wargv
@ cdecl __wgetmainargs(ptr ptr ptr long ptr)
@ extern __winitenv MSVCRT___winitenv
@ cdecl _abnormal_termination() __intrinsic_abnormal_termination
@ cdecl _access(str long)
@ extern _acmdln MSVCRT__acmdln
@ stdcall -arch=i386 _adj_fdiv_m16i(long)
@ stdcall -arch=i386 _adj_fdiv_m32(long)
@ stdcall -arch=i386 _adj_fdiv_m32i(long)
@ stdcall -arch=i386 _adj_fdiv_m64(int64)
@ cdecl -arch=i386 _adj_fdiv_r()
@ stdcall -arch=i386 _adj_fdivr_m16i(long)
@ stdcall -arch=i386 _adj_fdivr_m32(long)
@ stdcall -arch=i386 _adj_fdivr_m32i(long)
@ stdcall -arch=i386 _adj_fdivr_m64(int64)
@ cdecl -arch=i386 _adj_fpatan()
@ cdecl -arch=i386 _adj_fprem()
@ cdecl -arch=i386 _adj_fprem1()
@ cdecl -arch=i386 _adj_fptan()
@ extern -arch=i386 _adjust_fdiv MSVCRT__adjust_fdiv
@ extern _aexit_rtn
@ cdecl _amsg_exit(long)
@ cdecl _assert(str str long)
@ cdecl _atodbl(ptr str)
@ cdecl -ret64 _atoi64(str)
@ cdecl _atoldbl(ptr str)
@ cdecl _beep(long long)
@ cdecl _beginthread(ptr long ptr)
@ cdecl _beginthreadex(ptr long ptr ptr long ptr)
@ cdecl _c_exit()
@ cdecl _cabs(long)
@ cdecl _callnewh(long)
@ cdecl _calloc_dbg(long long) calloc
@ cdecl _cexit()
@ cdecl _cgets(ptr)
@ cdecl _chdir(str)
@ cdecl _chdrive(long)
@ cdecl _chgsign(double)
@ cdecl -arch=i386 -norelay _chkesp()
@ cdecl _chmod(str long)
@ cdecl _chsize(long long)
@ cdecl _clearfp()
@ cdecl _close(long)
@ cdecl _commit(long)
@ extern _commode MSVCRT__commode
@ cdecl _control87(long long)
@ cdecl _controlfp(long long)
@ cdecl _copysign(double double) copysign
@ varargs _cprintf(str)
@ cdecl _cputs(str)
@ cdecl _creat(str long)
@ extern _crtAssertBusy
@ extern _crtBreakAlloc
@ extern _crtDbgFlag
@ varargs _cscanf(str)
@ extern _ctype MSVCRT__ctype
@ cdecl _cwait(ptr long long)
@ extern _daylight MSVCRT___daylight
@ extern _dstbias MSVCRT__dstbias
@ cdecl _dup(long)
@ cdecl _dup2(long long)
@ cdecl _ecvt(double long ptr ptr)
@ cdecl _endthread()
@ cdecl _endthreadex(long)
@ extern _environ MSVCRT__environ
@ cdecl _eof(long)
@ cdecl _errno()
@ cdecl -arch=i386 _except_handler2(ptr ptr ptr ptr)
@ cdecl -arch=i386 _except_handler3(ptr ptr ptr ptr)
@ varargs _execl(str str)
@ varargs _execle(str str)
@ varargs _execlp(str str)
@ varargs _execlpe(str str)
@ cdecl _execv(str ptr)
@ cdecl _execve(str ptr ptr)
@ cdecl _execvp(str ptr)
@ cdecl _execvpe(str ptr ptr)
@ cdecl _exit(long)
@ cdecl _expand(ptr long)
@ cdecl _expand_dbg(ptr long) _expand
@ cdecl _fcloseall()
@ cdecl _fcvt(double long ptr ptr)
@ cdecl _fdopen(long str)
@ cdecl _fgetchar()
@ cdecl _fgetwchar()
@ cdecl _filbuf(ptr)
# extern _fileinfo
@ cdecl _filelength(long)
@ cdecl -ret64 _filelengthi64(long)
@ cdecl _fileno(ptr)
@ cdecl _findclose(long)
@ cdecl _findfirst(str ptr)
@ cdecl _findfirsti64(str ptr)
@ cdecl _findnext(long ptr)
@ cdecl _findnexti64(long ptr)
@ cdecl _finite(double)
@ cdecl _flsbuf(long ptr)
@ cdecl _flushall()
@ extern _fmode MSVCRT__fmode
@ cdecl _fpclass(double)
@ cdecl _fpieee_flt(long ptr ptr)
@ cdecl _fpreset()
@ cdecl _fputchar(long)
@ cdecl _fputwchar(long)
@ cdecl _free_dbg(ptr) free
@ cdecl _fsopen(str str long)
@ cdecl _fstat(long ptr)
@ cdecl _fstati64(long ptr)
@ cdecl -arch=win32 _ftime(ptr) _ftime32
@ cdecl -arch=win64 _ftime(ptr) _ftime64
@ cdecl -arch=i386 -ret64 _ftol()
@ cdecl _fullpath(ptr str long)
@ cdecl -arch=win32 _futime(long ptr) _futime32
@ cdecl -arch=win64 _futime(long ptr) _futime64
@ cdecl _gcvt(double long str)
@ cdecl _get_osfhandle(long)
@ cdecl _get_sbh_threshold()
@ cdecl _getch()
@ cdecl _getche()
@ cdecl _getcwd(str long)
@ cdecl _getdcwd(long str long)
@ cdecl _getdiskfree(long ptr)
@ cdecl _getdllprocaddr(long str long)
@ cdecl _getdrive()
@ cdecl _getdrives() kernel32.GetLogicalDrives
@ cdecl _getmaxstdio()
@ cdecl _getmbcp()
@ cdecl _getpid()
@ stub _getsystime(ptr)
@ cdecl _getw(ptr)
@ cdecl _getws(ptr)
@ cdecl -arch=i386 _global_unwind2(ptr)
@ cdecl _heapadd(ptr long)
@ cdecl _heapchk()
@ cdecl _heapmin()
@ cdecl _heapset(long)
@ stub _heapused(ptr ptr)
@ cdecl _heapwalk(ptr)
@ cdecl _hypot(double double)
@ cdecl _i64toa(int64 ptr long)
@ cdecl _i64tow(int64 ptr long)
@ cdecl _initterm(ptr ptr)
@ stub -arch=i386 _inp(long)
@ stub -arch=i386 _inpd(long)
@ stub -arch=i386 _inpw(long)
@ extern _iob MSVCRT__iob
@ cdecl _isatty(long)
@ cdecl _isctype(long long)
@ stub _ismbbalnum(long)
@ stub _ismbbalpha(long)
@ stub _ismbbgraph(long)
@ stub _ismbbkalnum(long)
@ cdecl _ismbbkana(long)
@ stub _ismbbkprint(long)
@ stub _ismbbkpunct(long)
@ cdecl _ismbblead(long)
@ stub _ismbbprint(long)
@ stub _ismbbpunct(long)
@ cdecl _ismbbtrail(long)
@ cdecl _ismbcalnum(long)
@ cdecl _ismbcalpha(long)
@ cdecl _ismbcdigit(long)
@ cdecl _ismbcgraph(long)
@ cdecl _ismbchira(long)
@ cdecl _ismbckata(long)
@ cdecl _ismbcl0(long)
@ cdecl _ismbcl1(long)
@ cdecl _ismbcl2(long)
@ cdecl _ismbclegal(long)
@ cdecl _ismbclower(long)
@ cdecl _ismbcprint(long)
@ cdecl _ismbcpunct(long)
@ cdecl _ismbcspace(long)
@ cdecl _ismbcsymbol(long)
@ cdecl _ismbcupper(long)
@ cdecl _ismbslead(ptr ptr)
@ cdecl _ismbstrail(ptr ptr)
@ cdecl _isnan(double)
@ cdecl _itoa(long ptr long)
@ cdecl _itow(long ptr long)
@ cdecl _j0(double)
@ cdecl _j1(double)
@ cdecl _jn(long double)
@ cdecl _kbhit()
@ cdecl _lfind(ptr ptr ptr long ptr)
@ cdecl _loaddll(str)
@ cdecl -arch=win64 _local_unwind(ptr ptr)
@ cdecl -arch=i386 _local_unwind2(ptr long)
@ cdecl _lock(long)
@ cdecl _locking(long long long)
@ cdecl _logb(double) logb
@ cdecl -arch=i386 _longjmpex(ptr long) MSVCRT_longjmp
@ cdecl _lrotl(long long) MSVCRT__lrotl
@ cdecl _lrotr(long long) MSVCRT__lrotr
@ cdecl _lsearch(ptr ptr ptr long ptr)
@ cdecl _lseek(long long long)
@ cdecl -ret64 _lseeki64(long int64 long)
@ cdecl _ltoa(long ptr long)
@ cdecl _ltow(long ptr long)
@ cdecl _makepath(ptr str str str str)
@ cdecl _malloc_dbg(long) malloc
@ cdecl _mbbtombc(long)
@ cdecl _mbbtype(long long)
# extern _mbcasemap
@ cdecl _mbccpy(ptr ptr)
@ cdecl _mbcjistojms(long)
@ cdecl _mbcjmstojis(long)
@ cdecl _mbclen(ptr)
@ cdecl _mbctohira(long)
@ cdecl _mbctokata(long)
@ cdecl _mbctolower(long)
@ cdecl _mbctombb(long)
@ cdecl _mbctoupper(long)
@ extern _mbctype MSVCRT_mbctype
@ cdecl _mbsbtype(str long)
@ cdecl _mbscat(str str)
@ cdecl _mbschr(str long)
@ cdecl _mbscmp(str str)
@ cdecl _mbscoll(str str)
@ cdecl _mbscpy(ptr str)
@ cdecl _mbscspn(str str)
@ cdecl _mbsdec(ptr ptr)
@ cdecl _mbsdup(str) _strdup
@ cdecl _mbsicmp(str str)
@ cdecl _mbsicoll(str str)
@ cdecl _mbsinc(str)
@ cdecl _mbslen(str)
@ cdecl _mbslwr(str)
@ cdecl _mbsnbcat(str str long)
@ cdecl _mbsnbcmp(str str long)
@ cdecl _mbsnbcnt(ptr long)
@ cdecl _mbsnbcoll(str str long)
@ cdecl _mbsnbcpy(ptr str long)
@ cdecl _mbsnbicmp(str str long)
@ cdecl _mbsnbicoll(str str long)
@ cdecl _mbsnbset(ptr long long)
@ cdecl _mbsncat(str str long)
@ cdecl _mbsnccnt(str long)
@ cdecl _mbsncmp(str str long)
@ stub _mbsncoll(str str long)
@ cdecl _mbsncpy(ptr str long)
@ cdecl _mbsnextc(str)
@ cdecl _mbsnicmp(str str long)
@ stub _mbsnicoll(str str long)
@ cdecl _mbsninc(str long)
@ cdecl _mbsnset(ptr long long)
@ cdecl _mbspbrk(str str)
@ cdecl _mbsrchr(str long)
@ cdecl _mbsrev(str)
@ cdecl _mbsset(ptr long)
@ cdecl _mbsspn(str str)
@ cdecl _mbsspnp(str str)
@ cdecl _mbsstr(str str)
@ cdecl _mbstok(str str)
@ cdecl _mbstrlen(str)
@ cdecl _mbsupr(str)
@ cdecl _memccpy(ptr ptr long long)
@ cdecl _memicmp(str str long)
@ cdecl _mkdir(str)
@ cdecl _mktemp(str)
@ cdecl _msize(ptr)
@ cdecl _msize_dbg(ptr) _msize
@ cdecl _nextafter(double double) nextafter
@ cdecl _onexit(ptr)
@ varargs _open(str long)
@ cdecl _open_osfhandle(long long)
@ extern _osver MSVCRT__osver
@ stub -arch=i386 _outp(long long)
@ stub -arch=i386 _outpd(long long)
@ stub -arch=i386 _outpw(long long)
@ cdecl _pclose(ptr)
@ extern _pctype MSVCRT__pctype
@ extern _pgmptr MSVCRT__pgmptr
@ cdecl _pipe(ptr long long)
@ cdecl _popen(str str)
@ cdecl _purecall()
@ cdecl _putch(long)
@ cdecl _putenv(str)
@ cdecl _putw(long ptr)
@ cdecl _putws(wstr)
@ extern _pwctype MSVCRT__pwctype
@ cdecl _read(long ptr long)
@ cdecl _realloc_dbg(ptr long) realloc
@ cdecl _rmdir(str)
@ cdecl _rmtmp()
@ cdecl _rotl(long long) MSVCRT__rotl
@ cdecl _rotr(long long) MSVCRT__rotr
@ cdecl -arch=i386 _safe_fdiv()
@ cdecl -arch=i386 _safe_fdivr()
@ cdecl -arch=i386 _safe_fprem()
@ cdecl -arch=i386 _safe_fprem1()
@ cdecl _scalb(double long)
@ cdecl _searchenv(str str ptr)
@ stdcall -arch=i386 _seh_longjmp_unwind4(ptr)
@ stdcall -arch=i386 _seh_longjmp_unwind(ptr)
@ cdecl _set_error_mode(long)
@ cdecl _set_sbh_threshold(long)
@ cdecl _seterrormode(long)
@ cdecl -norelay _setjmp(ptr) MSVCRT__setjmp
@ cdecl -arch=i386 -norelay _setjmp3(ptr long) MSVCRT__setjmp3
@ cdecl _setmaxstdio(long)
@ cdecl _setmbcp(long)
@ cdecl _setmode(long long)
@ stub _setsystime(ptr long)
@ cdecl _sleep(long)
@ varargs _snprintf(ptr long str)
@ varargs _snwprintf(ptr long wstr)
@ varargs _sopen(str long long)
@ varargs _spawnl(long str str)
@ varargs _spawnle(long str str)
@ varargs _spawnlp(long str str)
@ varargs _spawnlpe(long str str)
@ cdecl _spawnv(long str ptr)
@ cdecl _spawnve(long str ptr ptr)
@ cdecl _spawnvp(long str ptr)
@ cdecl _spawnvpe(long str ptr ptr)
@ cdecl _splitpath(str ptr ptr ptr ptr)
@ cdecl _stat(str ptr)
@ cdecl _stati64(str ptr)
@ cdecl _statusfp()
@ cdecl _strcmpi(str str) _stricmp
@ cdecl _strdate(ptr)
@ cdecl _strdup(str)
@ cdecl _strerror(long)
@ cdecl _stricmp(str str)
@ cdecl _stricoll(str str)
@ cdecl _strlwr(str)
@ cdecl _strncoll(str str long)
@ cdecl _strnicmp(str str long)
@ cdecl _strnicoll(str str long)
@ cdecl _strnset(str long long)
@ cdecl _strrev(str)
@ cdecl _strset(str long)
@ cdecl _strtime(ptr)
@ cdecl _strupr(str)
@ cdecl _swab(str str long)
@ extern _sys_errlist MSVCRT__sys_errlist
@ extern _sys_nerr MSVCRT__sys_nerr
@ cdecl _tell(long)
@ cdecl -ret64 _telli64(long)
@ cdecl _tempnam(str str)
@ extern _timezone MSVCRT___timezone
@ cdecl _tolower(long)
@ cdecl _toupper(long)
@ extern _tzname MSVCRT__tzname
@ cdecl _tzset()
@ cdecl _ui64toa(int64 ptr long)
@ cdecl _ui64tow(int64 ptr long)
@ cdecl _ultoa(long ptr long)
@ cdecl _ultow(long ptr long)
@ cdecl _umask(long)
@ cdecl _ungetch(long)
@ cdecl _unlink(str)
@ cdecl _unloaddll(long)
@ cdecl _unlock(long)
@ cdecl -arch=win32 _utime(str ptr) _utime32
@ cdecl -arch=win64 _utime(str ptr) _utime64
@ cdecl -norelay _vsnprintf(ptr long str ptr)
@ cdecl _vsnwprintf(ptr long wstr ptr)
@ cdecl _waccess(wstr long)
@ cdecl _wasctime(ptr)
@ cdecl _wchdir(wstr)
@ cdecl _wchmod(wstr long)
@ extern _wcmdln MSVCRT__wcmdln
@ cdecl _wcreat(wstr long)
@ cdecl _wcsdup(wstr)
@ cdecl _wcsicmp(wstr wstr)
@ cdecl _wcsicoll(wstr wstr)
@ cdecl _wcslwr(wstr)
@ cdecl _wcsncoll(wstr wstr long)
@ cdecl _wcsnicmp(wstr wstr long)
@ cdecl _wcsnicoll(wstr wstr long)
@ cdecl _wcsnset(wstr long long)
@ cdecl _wcsrev(wstr)
@ cdecl _wcsset(wstr long)
@ cdecl _wcsupr(wstr)
@ cdecl -arch=win32 _wctime(ptr) _wctime32
@ cdecl -arch=win64 _wctime(ptr) _wctime64
@ extern _wenviron MSVCRT__wenviron
@ varargs _wexecl(wstr wstr)
@ varargs _wexecle(wstr wstr)
@ varargs _wexeclp(wstr wstr)
@ varargs _wexeclpe(wstr wstr)
@ cdecl _wexecv(wstr ptr)
@ cdecl _wexecve(wstr ptr ptr)
@ cdecl _wexecvp(wstr ptr)
@ cdecl _wexecvpe(wstr ptr ptr)
@ cdecl _wfdopen(long wstr)
@ cdecl _wfindfirst(wstr ptr)
@ cdecl _wfindfirsti64(wstr ptr)
@ cdecl _wfindnext(long ptr)
@ cdecl _wfindnexti64(long ptr)
@ cdecl _wfopen(wstr wstr)
@ cdecl _wfreopen(wstr wstr ptr)
@ cdecl _wfsopen(wstr wstr long)
@ cdecl _wfullpath(ptr wstr long)
@ cdecl _wgetcwd(wstr long)
@ cdecl _wgetdcwd(long wstr long)
@ cdecl _wgetenv(wstr)
@ extern _winmajor MSVCRT__winmajor
@ extern _winminor MSVCRT__winminor
@ extern _winver MSVCRT__winver
@ cdecl _wmakepath(ptr wstr wstr wstr wstr)
@ cdecl _wmkdir(wstr)
@ cdecl _wmktemp(wstr)
@ varargs _wopen(wstr long)
@ cdecl _wperror(wstr)
@ extern _wpgmptr MSVCRT__wpgmptr
@ cdecl _wpopen(wstr wstr)
@ cdecl _wputenv(wstr)
@ cdecl _wremove(wstr)
@ cdecl _wrename(wstr wstr)
@ cdecl _write(long ptr long)
@ cdecl _wrmdir(wstr)
@ cdecl _wsearchenv(wstr wstr ptr)
@ cdecl _wsetlocale(long wstr)
@ varargs _wsopen(wstr long long)
@ varargs _wspawnl(long wstr wstr)
@ varargs _wspawnle(long wstr wstr)
@ varargs _wspawnlp(long wstr wstr)
@ varargs _wspawnlpe(long wstr wstr)
@ cdecl _wspawnv(long wstr ptr)
@ cdecl _wspawnve(long wstr ptr ptr)
@ cdecl _wspawnvp(long wstr ptr)
@ cdecl _wspawnvpe(long wstr ptr ptr)
@ cdecl _wsplitpath(wstr ptr ptr ptr ptr)
@ cdecl _wstat(wstr ptr)
@ cdecl _wstati64(wstr ptr)
@ cdecl _wstrdate(ptr)
@ cdecl _wstrtime(ptr)
@ cdecl _wsystem(wstr)
@ cdecl _wtempnam(wstr wstr)
@ cdecl _wtmpnam(ptr)
@ cdecl _wtoi(wstr)
@ cdecl -ret64 _wtoi64(wstr)
@ cdecl _wtol(wstr)
@ cdecl _wunlink(wstr)
@ cdecl -arch=win32 _wutime(wstr ptr) _wutime32
@ cdecl -arch=win64 _wutime(wstr ptr) _wutime64
@ cdecl _y0(double)
@ cdecl _y1(double)
@ cdecl _yn(long double)
@ cdecl abort()
@ cdecl abs(long)
@ cdecl acos(double)
@ cdecl asctime(ptr)
@ cdecl asin(double) MSVCRT_asin
@ cdecl atan(double)
@ cdecl atan2(double double)
@ cdecl -private atexit(ptr) MSVCRT_atexit
@ cdecl atof(str)
@ cdecl atoi(str)
@ cdecl atol(str)
@ cdecl bsearch(ptr ptr long long ptr)
@ cdecl calloc(long long)
@ cdecl ceil(double)
@ cdecl clearerr(ptr)
@ cdecl clock()
@ cdecl cos(double)
@ cdecl cosh(double)
@ cdecl -arch=win32 ctime(ptr) _ctime32
@ cdecl -arch=win64 ctime(ptr) _ctime64
@ cdecl -arch=win32 difftime(long long) _difftime32
@ cdecl -arch=win64 difftime(long long) _difftime64
@ cdecl -ret64 div(long long)
@ cdecl exit(long)
@ cdecl exp(double)
@ cdecl fabs(double)
@ cdecl fclose(ptr)
@ cdecl feof(ptr)
@ cdecl ferror(ptr)
@ cdecl fflush(ptr)
@ cdecl fgetc(ptr)
@ cdecl fgetpos(ptr ptr)
@ cdecl fgets(ptr long ptr)
@ cdecl fgetwc(ptr)
@ cdecl fgetws(ptr long ptr)
@ cdecl floor(double)
@ cdecl fmod(double double)
@ cdecl fopen(str str)
@ varargs fprintf(ptr str)
@ cdecl fputc(long ptr)
@ cdecl fputs(str ptr)
@ cdecl fputwc(long ptr)
@ cdecl fputws(wstr ptr)
@ cdecl fread(ptr long long ptr)
@ cdecl free(ptr)
@ cdecl freopen(str str ptr)
@ cdecl frexp(double ptr)
@ varargs fscanf(ptr str)
@ cdecl fseek(ptr long long)
@ cdecl fsetpos(ptr ptr)
@ cdecl ftell(ptr)
@ varargs fwprintf(ptr wstr)
@ cdecl fwrite(ptr long long ptr)
@ varargs fwscanf(ptr wstr)
@ cdecl getc(ptr)
@ cdecl getchar()
@ cdecl getenv(str)
@ cdecl gets(str)
@ cdecl getwc(ptr)
@ cdecl getwchar()
@ cdecl -arch=win32 gmtime(ptr) _gmtime32
@ cdecl -arch=win64 gmtime(ptr) _gmtime64
@ cdecl is_wctype(long long) iswctype
@ cdecl isalnum(long)
@ cdecl isalpha(long)
@ cdecl iscntrl(long)
@ cdecl isdigit(long)
@ cdecl isgraph(long)
@ cdecl isleadbyte(long)
@ cdecl islower(long)
@ cdecl isprint(long)
@ cdecl ispunct(long)
@ cdecl isspace(long)
@ cdecl isupper(long)
@ cdecl iswalnum(long)
@ cdecl iswalpha(long)
@ cdecl iswascii(long)
@ cdecl iswcntrl(long)
@ cdecl iswctype(long long)
@ cdecl iswdigit(long)
@ cdecl iswgraph(long)
@ cdecl iswlower(long)
@ cdecl iswprint(long)
@ cdecl iswpunct(long)
@ cdecl iswspace(long)
@ cdecl iswupper(long)
@ cdecl iswxdigit(long)
@ cdecl isxdigit(long)
@ cdecl labs(long)
@ cdecl ldexp(double long)
@ cdecl -ret64 ldiv(long long)
@ cdecl localeconv()
@ cdecl -arch=win32 localtime(ptr) _localtime32
@ cdecl -arch=win64 localtime(ptr) _localtime64
@ cdecl log(double)
@ cdecl log10(double)
@ cdecl longjmp(ptr long) MSVCRT_longjmp
@ cdecl malloc(long)
@ cdecl mblen(ptr long)
@ cdecl mbstowcs(ptr str long)
@ cdecl mbtowc(ptr str long)
@ cdecl memchr(ptr long long)
@ cdecl memcmp(ptr ptr long)
@ cdecl memcpy(ptr ptr long)
@ cdecl memmove(ptr ptr long)
@ cdecl memset(ptr long long)
@ cdecl -arch=win32 mktime(ptr) _mktime32
@ cdecl -arch=win64 mktime(ptr) _mktime64
@ cdecl modf(double ptr)
@ cdecl perror(str)
@ cdecl pow(double double)
@ varargs printf(str)
@ cdecl putc(long ptr)
@ cdecl putchar(long)
@ cdecl puts(str)
@ cdecl putwc(long ptr) fputwc
@ cdecl putwchar(long) _fputwchar
@ cdecl qsort(ptr long long ptr)
@ cdecl raise(long)
@ cdecl rand()
@ cdecl realloc(ptr long)
@ cdecl remove(str)
@ cdecl rename(str str)
@ cdecl rewind(ptr)
@ varargs scanf(str)
@ cdecl setbuf(ptr ptr)
@ cdecl setlocale(long str)
@ cdecl setvbuf(ptr str long long)
@ cdecl signal(long long)
@ cdecl sin(double)
@ cdecl sinh(double)
@ varargs sprintf(ptr str)
@ cdecl sqrt(double) MSVCRT_sqrt
@ cdecl srand(long)
@ varargs sscanf(str str)
@ cdecl strcat(str str)
@ cdecl strchr(str long)
@ cdecl strcmp(str str)
@ cdecl strcoll(str str)
@ cdecl strcpy(ptr str)
@ cdecl strcspn(str str)
@ cdecl strerror(long)
@ cdecl strftime(ptr long str ptr)
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
@ varargs swprintf(ptr wstr) _swprintf
@ varargs swscanf(wstr wstr)
@ cdecl system(str)
@ cdecl tan(double)
@ cdecl tanh(double) MSVCRT_tanh
@ cdecl -arch=win32 time(ptr) _time32
@ cdecl -arch=win64 time(ptr) _time64
@ cdecl tmpfile()
@ cdecl tmpnam(ptr)
@ cdecl tolower(long)
@ cdecl toupper(long)
@ cdecl towlower(long)
@ cdecl towupper(long)
@ cdecl ungetc(long ptr)
@ cdecl ungetwc(long ptr)
@ cdecl vfprintf(ptr str ptr)
@ cdecl vfwprintf(ptr wstr ptr)
@ cdecl vprintf(str ptr)
@ cdecl vsprintf(ptr str ptr)
@ cdecl vswprintf(ptr wstr ptr) _vswprintf
@ cdecl vwprintf(wstr ptr)
@ cdecl wcscat(wstr wstr)
@ cdecl wcschr(wstr long)
@ cdecl wcscmp(wstr wstr)
@ cdecl wcscoll(wstr wstr)
@ cdecl wcscpy(ptr wstr)
@ cdecl wcscspn(wstr wstr)
@ cdecl wcsftime(ptr long wstr ptr)
@ cdecl wcslen(wstr)
@ cdecl wcsncat(wstr wstr long)
@ cdecl wcsncmp(wstr wstr long)
@ cdecl wcsncpy(ptr wstr long)
@ cdecl wcspbrk(wstr wstr)
@ cdecl wcsrchr(wstr long)
@ cdecl wcsspn(wstr wstr)
@ cdecl wcsstr(wstr wstr)
@ cdecl wcstod(wstr ptr)
@ cdecl wcstok(wstr wstr)
@ cdecl wcstol(wstr ptr long)
@ cdecl wcstombs(ptr ptr long)
@ cdecl wcstoul(wstr ptr long)
@ cdecl wcsxfrm(ptr wstr long)
@ cdecl wctomb(ptr long)
@ varargs wprintf(wstr)
@ varargs wscanf(wstr)
