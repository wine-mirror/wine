# msvcrt.dll - MS VC++ Run Time Library

@ cdecl -norelay $I10_OUTPUT(double long long long ptr) MSVCRT_I10_OUTPUT
@ thiscall -arch=win32 ??0__non_rtti_object@@QAE@ABV0@@Z(ptr ptr) MSVCRT___non_rtti_object_copy_ctor
@ cdecl -arch=win64 ??0__non_rtti_object@@QEAA@AEBV0@@Z(ptr ptr) MSVCRT___non_rtti_object_copy_ctor
@ thiscall -arch=win32 ??0__non_rtti_object@@QAE@PBD@Z(ptr ptr) MSVCRT___non_rtti_object_ctor
@ cdecl -arch=win64 ??0__non_rtti_object@@QEAA@PEBD@Z(ptr ptr) MSVCRT___non_rtti_object_ctor
@ thiscall -arch=win32 ??0bad_cast@@AAE@PBQBD@Z(ptr ptr) MSVCRT_bad_cast_ctor
@ cdecl -arch=win64 ??0bad_cast@@AEAA@PEBQEBD@Z(ptr ptr) MSVCRT_bad_cast_ctor
@ thiscall -arch=win32 ??0bad_cast@@QAE@ABQBD@Z(ptr ptr) MSVCRT_bad_cast_ctor
@ cdecl -arch=win64 ??0bad_cast@@QEAA@AEBQEBD@Z(ptr ptr) MSVCRT_bad_cast_ctor
@ thiscall -arch=win32 ??0bad_cast@@QAE@ABV0@@Z(ptr ptr) MSVCRT_bad_cast_copy_ctor
@ cdecl -arch=win64 ??0bad_cast@@QEAA@AEBV0@@Z(ptr ptr) MSVCRT_bad_cast_copy_ctor
@ thiscall -arch=win32 ??0bad_cast@@QAE@PBD@Z(ptr str) MSVCRT_bad_cast_ctor_charptr
@ cdecl -arch=win64 ??0bad_cast@@QEAA@PEBD@Z(ptr str) MSVCRT_bad_cast_ctor_charptr
@ thiscall -arch=win32 ??0bad_typeid@@QAE@ABV0@@Z(ptr ptr) MSVCRT_bad_typeid_copy_ctor
@ cdecl -arch=win64 ??0bad_typeid@@QEAA@AEBV0@@Z(ptr ptr) MSVCRT_bad_typeid_copy_ctor
@ thiscall -arch=win32 ??0bad_typeid@@QAE@PBD@Z(ptr str) MSVCRT_bad_typeid_ctor
@ cdecl -arch=win64 ??0bad_typeid@@QEAA@PEBD@Z(ptr str) MSVCRT_bad_typeid_ctor
@ thiscall -arch=win32 ??0exception@@QAE@ABQBD@Z(ptr ptr) MSVCRT_exception_ctor
@ cdecl -arch=win64 ??0exception@@QEAA@AEBQEBD@Z(ptr ptr) MSVCRT_exception_ctor
@ thiscall -arch=win32 ??0exception@@QAE@ABQBDH@Z(ptr ptr long) MSVCRT_exception_ctor_noalloc
@ cdecl -arch=win64 ??0exception@@QEAA@AEBQEBDH@Z(ptr ptr long) MSVCRT_exception_ctor_noalloc
@ thiscall -arch=win32 ??0exception@@QAE@ABV0@@Z(ptr ptr) MSVCRT_exception_copy_ctor
@ cdecl -arch=win64 ??0exception@@QEAA@AEBV0@@Z(ptr ptr) MSVCRT_exception_copy_ctor
@ thiscall -arch=win32 ??0exception@@QAE@XZ(ptr) MSVCRT_exception_default_ctor
@ cdecl -arch=win64 ??0exception@@QEAA@XZ(ptr) MSVCRT_exception_default_ctor
@ thiscall -arch=win32 ??1__non_rtti_object@@UAE@XZ(ptr) MSVCRT___non_rtti_object_dtor
@ cdecl -arch=win64 ??1__non_rtti_object@@UEAA@XZ(ptr) MSVCRT___non_rtti_object_dtor
@ thiscall -arch=win32 ??1bad_cast@@UAE@XZ(ptr) MSVCRT_bad_cast_dtor
@ cdecl -arch=win64 ??1bad_cast@@UEAA@XZ(ptr) MSVCRT_bad_cast_dtor
@ thiscall -arch=win32 ??1bad_typeid@@UAE@XZ(ptr) MSVCRT_bad_typeid_dtor
@ cdecl -arch=win64 ??1bad_typeid@@UEAA@XZ(ptr) MSVCRT_bad_typeid_dtor
@ thiscall -arch=win32 ??1exception@@UAE@XZ(ptr) MSVCRT_exception_dtor
@ cdecl -arch=win64 ??1exception@@UEAA@XZ(ptr) MSVCRT_exception_dtor
@ thiscall -arch=win32 ??1type_info@@UAE@XZ(ptr) MSVCRT_type_info_dtor
@ cdecl -arch=win64 ??1type_info@@UEAA@XZ(ptr) MSVCRT_type_info_dtor
@ cdecl -arch=win32 ??2@YAPAXI@Z(long) MSVCRT_operator_new
@ cdecl -arch=win64 ??2@YAPEAX_K@Z(long) MSVCRT_operator_new
@ cdecl -arch=win32 ??2@YAPAXIHPBDH@Z(long long str long) MSVCRT_operator_new_dbg
@ cdecl -arch=win64 ??2@YAPEAX_KHPEBDH@Z(long long str long) MSVCRT_operator_new_dbg
@ cdecl -arch=win32 ??3@YAXPAX@Z(ptr) MSVCRT_operator_delete
@ cdecl -arch=win64 ??3@YAXPEAX@Z(ptr) MSVCRT_operator_delete
@ thiscall -arch=win32 ??4__non_rtti_object@@QAEAAV0@ABV0@@Z(ptr ptr) MSVCRT___non_rtti_object_opequals
@ cdecl -arch=win64 ??4__non_rtti_object@@QEAAAEAV0@AEBV0@@Z(ptr ptr) MSVCRT___non_rtti_object_opequals
@ thiscall -arch=win32 ??4bad_cast@@QAEAAV0@ABV0@@Z(ptr ptr) MSVCRT_bad_cast_opequals
@ cdecl -arch=win64 ??4bad_cast@@QEAAAEAV0@AEBV0@@Z(ptr ptr) MSVCRT_bad_cast_opequals
@ thiscall -arch=win32 ??4bad_typeid@@QAEAAV0@ABV0@@Z(ptr ptr) MSVCRT_bad_typeid_opequals
@ cdecl -arch=win64 ??4bad_typeid@@QEAAAEAV0@AEBV0@@Z(ptr ptr) MSVCRT_bad_typeid_opequals
@ thiscall -arch=win32 ??4exception@@QAEAAV0@ABV0@@Z(ptr ptr) MSVCRT_exception_opequals
@ cdecl -arch=win64 ??4exception@@QEAAAEAV0@AEBV0@@Z(ptr ptr) MSVCRT_exception_opequals
@ thiscall -arch=win32 ??8type_info@@QBEHABV0@@Z(ptr ptr) MSVCRT_type_info_opequals_equals
@ cdecl -arch=win64 ??8type_info@@QEBAHAEBV0@@Z(ptr ptr) MSVCRT_type_info_opequals_equals
@ thiscall -arch=win32 ??9type_info@@QBEHABV0@@Z(ptr ptr) MSVCRT_type_info_opnot_equals
@ cdecl -arch=win64 ??9type_info@@QEBAHAEBV0@@Z(ptr ptr) MSVCRT_type_info_opnot_equals
@ extern ??_7__non_rtti_object@@6B@ MSVCRT___non_rtti_object_vtable
@ extern ??_7bad_cast@@6B@ MSVCRT_bad_cast_vtable
@ extern ??_7bad_typeid@@6B@ MSVCRT_bad_typeid_vtable
@ extern ??_7exception@@6B@ MSVCRT_exception_vtable
@ thiscall -arch=win32 ??_E__non_rtti_object@@UAEPAXI@Z(ptr long) MSVCRT___non_rtti_object_vector_dtor
@ thiscall -arch=win32 ??_Ebad_cast@@UAEPAXI@Z(ptr long) MSVCRT_bad_cast_vector_dtor
@ thiscall -arch=win32 ??_Ebad_typeid@@UAEPAXI@Z(ptr long) MSVCRT_bad_typeid_vector_dtor
@ thiscall -arch=win32 ??_Eexception@@UAEPAXI@Z(ptr long) MSVCRT_exception_vector_dtor
@ thiscall -arch=win32 ??_Fbad_cast@@QAEXXZ(ptr) MSVCRT_bad_cast_default_ctor
@ cdecl -arch=win64 ??_Fbad_cast@@QEAAXXZ(ptr) MSVCRT_bad_cast_default_ctor
@ thiscall -arch=win32 ??_Fbad_typeid@@QAEXXZ(ptr) MSVCRT_bad_typeid_default_ctor
@ cdecl -arch=win64 ??_Fbad_typeid@@QEAAXXZ(ptr) MSVCRT_bad_typeid_default_ctor
@ thiscall -arch=win32 ??_G__non_rtti_object@@UAEPAXI@Z(ptr long) MSVCRT___non_rtti_object_scalar_dtor
@ thiscall -arch=win32 ??_Gbad_cast@@UAEPAXI@Z(ptr long) MSVCRT_bad_cast_scalar_dtor
@ thiscall -arch=win32 ??_Gbad_typeid@@UAEPAXI@Z(ptr long) MSVCRT_bad_typeid_scalar_dtor
@ thiscall -arch=win32 ??_Gexception@@UAEPAXI@Z(ptr long) MSVCRT_exception_scalar_dtor
@ cdecl -arch=win32 ??_U@YAPAXI@Z(long) MSVCRT_operator_new
@ cdecl -arch=win64 ??_U@YAPEAX_K@Z(long) MSVCRT_operator_new
@ cdecl -arch=win32 ??_U@YAPAXIHPBDH@Z(long long str long) MSVCRT_operator_new_dbg
@ cdecl -arch=win64 ??_U@YAPEAX_KHPEBDH@Z(long long str long) MSVCRT_operator_new_dbg
@ cdecl -arch=win32 ??_V@YAXPAX@Z(ptr) MSVCRT_operator_delete
@ cdecl -arch=win64 ??_V@YAXPEAX@Z(ptr) MSVCRT_operator_delete
@ cdecl -arch=win32 ?_query_new_handler@@YAP6AHI@ZXZ() MSVCRT__query_new_handler
@ cdecl -arch=win64 ?_query_new_handler@@YAP6AH_K@ZXZ() MSVCRT__query_new_handler
@ cdecl ?_query_new_mode@@YAHXZ() MSVCRT__query_new_mode
@ cdecl -arch=win32 ?_set_new_handler@@YAP6AHI@ZP6AHI@Z@Z(ptr) MSVCRT__set_new_handler
@ cdecl -arch=win64 ?_set_new_handler@@YAP6AH_K@ZP6AH0@Z@Z(ptr) MSVCRT__set_new_handler
@ cdecl ?_set_new_mode@@YAHH@Z(long) MSVCRT__set_new_mode
@ cdecl -arch=win32 ?_set_se_translator@@YAP6AXIPAU_EXCEPTION_POINTERS@@@ZP6AXI0@Z@Z(ptr) MSVCRT__set_se_translator
@ cdecl -arch=win64 ?_set_se_translator@@YAP6AXIPEAU_EXCEPTION_POINTERS@@@ZP6AXI0@Z@Z(ptr) MSVCRT__set_se_translator
@ thiscall -arch=win32 ?before@type_info@@QBEHABV1@@Z(ptr ptr) MSVCRT_type_info_before
@ cdecl -arch=win64 ?before@type_info@@QEBAHAEBV1@@Z(ptr ptr) MSVCRT_type_info_before
@ thiscall -arch=win32 ?name@type_info@@QBEPBDXZ(ptr) MSVCRT_type_info_name
@ cdecl -arch=win64 ?name@type_info@@QEBAPEBDXZ(ptr) MSVCRT_type_info_name
@ thiscall -arch=win32 ?raw_name@type_info@@QBEPBDXZ(ptr) MSVCRT_type_info_raw_name
@ cdecl -arch=win64 ?raw_name@type_info@@QEBAPEBDXZ(ptr) MSVCRT_type_info_raw_name
@ cdecl ?set_new_handler@@YAP6AXXZP6AXXZ@Z(ptr) MSVCRT_set_new_handler
@ cdecl ?set_terminate@@YAP6AXXZP6AXXZ@Z(ptr) MSVCRT_set_terminate
@ cdecl ?set_unexpected@@YAP6AXXZP6AXXZ@Z(ptr) MSVCRT_set_unexpected
@ cdecl ?terminate@@YAXXZ() MSVCRT_terminate
@ cdecl ?unexpected@@YAXXZ() MSVCRT_unexpected
@ thiscall -arch=win32 ?what@exception@@UBEPBDXZ(ptr) MSVCRT_what_exception
@ cdecl -arch=win64 ?what@exception@@UEBAPEBDXZ(ptr) MSVCRT_what_exception
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
# stub _CrtCheckMemory
# stub _CrtDbgBreak
# stub _CrtDbgReport
# stub _CrtDbgReportV
# stub _CrtDbgReportW
# stub _CrtDbgReportWV
# stub _CrtDoForAllClientObjects
# stub _CrtDumpMemoryLeaks
# stub _CrtIsMemoryBlock
# stub _CrtIsValidHeapPointer
# stub _CrtIsValidPointer
# stub _CrtMemCheckpoint
# stub _CrtMemDifference
# stub _CrtMemDumpAllObjectsSince
# stub _CrtMemDumpStatistics
# stub _CrtReportBlockType
# stub _CrtSetAllocHook
# stub _CrtSetBreakAlloc
# stub _CrtSetDbgBlockType
# stub _CrtSetDbgFlag
# stub _CrtSetDumpClient
# stub _CrtSetReportFile
# stub _CrtSetReportHook
# stub _CrtSetReportHook2
# stub _CrtSetReportMode
@ cdecl _CxxThrowException(long long)
@ cdecl -i386 -norelay _EH_prolog()
@ cdecl _Getdays()
@ cdecl _Getmonths()
@ cdecl _Gettnames()
@ extern _HUGE MSVCRT__HUGE
@ cdecl _Strftime(str long str ptr ptr)
@ cdecl _XcptFilter(long ptr)
@ cdecl __CppXcptFilter(long ptr)
# stub __CxxCallUnwindDelDtor
# stub __CxxCallUnwindDtor
# stub __CxxCallUnwindVecDtor
@ cdecl __CxxDetectRethrow(ptr)
# stub __CxxExceptionFilter
@ cdecl -i386 -norelay __CxxFrameHandler(ptr ptr ptr ptr)
@ cdecl -i386 -norelay __CxxFrameHandler2(ptr ptr ptr ptr) __CxxFrameHandler
@ cdecl -i386 -norelay __CxxFrameHandler3(ptr ptr ptr ptr) __CxxFrameHandler
@ stdcall -arch=x86_64 __C_specific_handler(ptr long ptr ptr) ntdll.__C_specific_handler
@ stdcall -i386 __CxxLongjmpUnwind(ptr)
@ cdecl __CxxQueryExceptionSize()
# stub __CxxRegisterExceptionObject
# stub __CxxUnregisterExceptionObject
# stub __DestructExceptionObject
@ cdecl __RTCastToVoid(ptr) MSVCRT___RTCastToVoid
@ cdecl __RTDynamicCast(ptr long ptr ptr long) MSVCRT___RTDynamicCast
@ cdecl __RTtypeid(ptr) MSVCRT___RTtypeid
@ cdecl __STRINGTOLD(ptr ptr str long)
@ cdecl ___lc_codepage_func()
@ cdecl ___lc_collate_cp_func()
@ cdecl ___lc_handle_func()
@ cdecl ___mb_cur_max_func() MSVCRT____mb_cur_max_func
@ cdecl ___setlc_active_func() MSVCRT____setlc_active_func
@ cdecl ___unguarded_readlc_active_add_func() MSVCRT____unguarded_readlc_active_add_func
@ extern __argc MSVCRT___argc
@ extern __argv MSVCRT___argv
@ extern __badioinfo MSVCRT___badioinfo
@ cdecl __crtCompareStringA(long long str long str long)
@ cdecl __crtCompareStringW(long long wstr long wstr long)
@ cdecl __crtGetLocaleInfoW(long long ptr long)
@ cdecl __crtGetStringTypeW(long long wstr long ptr)
@ cdecl __crtLCMapStringA(long long str long ptr long long long)
@ cdecl __crtLCMapStringW(long long wstr long ptr long long long)
# stub __daylight
@ cdecl __dllonexit(ptr ptr ptr)
@ cdecl __doserrno() MSVCRT___doserrno
# stub __dstbias
@ cdecl __fpecode()
@ stub __get_app_type
@ cdecl __getmainargs(ptr ptr ptr long ptr)
@ extern __initenv MSVCRT___initenv
@ cdecl __iob_func() MSVCRT___iob_func
@ cdecl __isascii(long) MSVCRT___isascii
@ cdecl __iscsym(long) MSVCRT___iscsym
@ cdecl __iscsymf(long) MSVCRT___iscsymf
@ extern __lc_codepage MSVCRT___lc_codepage
@ stub __lc_collate
@ extern __lc_collate_cp MSVCRT___lc_collate_cp
@ extern __lc_handle MSVCRT___lc_handle
@ cdecl __lconv_init()
# stub __libm_sse2_acos
# stub __libm_sse2_acosf
# stub __libm_sse2_asin
# stub __libm_sse2_asinf
# stub __libm_sse2_atan
# stub __libm_sse2_atan2
# stub __libm_sse2_atanf
# stub __libm_sse2_cos
# stub __libm_sse2_cosf
# stub __libm_sse2_exp
# stub __libm_sse2_expf
# stub __libm_sse2_log
# stub __libm_sse2_log10
# stub __libm_sse2_log10f
# stub __libm_sse2_logf
# stub __libm_sse2_pow
# stub __libm_sse2_powf
# stub __libm_sse2_sin
# stub __libm_sse2_sinf
# stub __libm_sse2_tan
# stub __libm_sse2_tanf
@ extern __mb_cur_max MSVCRT___mb_cur_max
@ cdecl -arch=i386 __p___argc()
@ cdecl -arch=i386 __p___argv()
@ cdecl -arch=i386 __p___initenv()
@ cdecl -arch=i386 __p___mb_cur_max() MSVCRT____mb_cur_max_func
@ cdecl -arch=i386 __p___wargv()
@ cdecl -arch=i386 __p___winitenv()
@ cdecl -arch=i386 __p__acmdln()
@ cdecl -arch=i386 __p__amblksiz()
@ cdecl -arch=i386 __p__commode()
@ cdecl -arch=i386 __p__daylight() MSVCRT___p__daylight
@ cdecl -arch=i386 __p__dstbias()
@ cdecl -arch=i386 __p__environ()
@ stub -arch=i386 __p__fileinfo #()
@ cdecl -arch=i386 __p__fmode()
@ cdecl -arch=i386 __p__iob() MSVCRT___iob_func
@ stub -arch=i386 __p__mbcasemap #()
@ cdecl -arch=i386 __p__mbctype()
@ cdecl -arch=i386 __p__osver()
@ cdecl -arch=i386 __p__pctype() MSVCRT___pctype_func
@ cdecl -arch=i386 __p__pgmptr()
@ stub -arch=i386 __p__pwctype #()
@ cdecl -arch=i386 __p__timezone() MSVCRT___p__timezone
@ cdecl -arch=i386 __p__tzname()
@ cdecl -arch=i386 __p__wcmdln()
@ cdecl -arch=i386 __p__wenviron()
@ cdecl -arch=i386 __p__winmajor()
@ cdecl -arch=i386 __p__winminor()
@ cdecl -arch=i386 __p__winver()
@ cdecl -arch=i386 __p__wpgmptr()
@ cdecl __pctype_func() MSVCRT___pctype_func
@ extern __pioinfo MSVCRT___pioinfo
# stub __pwctype_func
@ stub __pxcptinfoptrs #()
@ cdecl __set_app_type(long) MSVCRT___set_app_type
@ extern __setlc_active MSVCRT___setlc_active
@ cdecl __setusermatherr(ptr) MSVCRT___setusermatherr
# stub __strncnt
@ cdecl __threadhandle() kernel32.GetCurrentThread
@ cdecl __threadid() kernel32.GetCurrentThreadId
@ cdecl __toascii(long) MSVCRT___toascii
@ cdecl __uncaught_exception() MSVCRT___uncaught_exception
@ cdecl __unDName(ptr str long ptr ptr long)
@ cdecl __unDNameEx(ptr str long ptr ptr ptr long)
@ extern __unguarded_readlc_active MSVCRT___unguarded_readlc_active
@ extern __wargv MSVCRT___wargv
# stub __wcserror
# stub __wcserror_s
# stub __wcsncnt
@ cdecl __wgetmainargs(ptr ptr ptr long ptr)
@ extern __winitenv MSVCRT___winitenv
@ cdecl _abnormal_termination()
# stub _abs64
@ cdecl _access(str long) MSVCRT__access
# stub _access_s
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
# extern _aexit_rtn
@ cdecl _aligned_free(ptr)
# stub _aligned_free_dbg
@ cdecl _aligned_malloc(long long)
# stub _aligned_malloc_dbg
@ cdecl _aligned_offset_malloc(long long long)
# stub _aligned_offset_malloc_dbg
@ cdecl _aligned_offset_realloc(ptr long long long)
# stub _aligned_offset_realloc_dbg
@ cdecl _aligned_realloc(ptr long long)
# stub _aligned_realloc_dbg
@ cdecl _amsg_exit(long)
@ cdecl _assert(str str long) MSVCRT__assert
@ stub _atodbl #(ptr str)
# stub _atodbl_l
@ cdecl _atof_l(str ptr) MSVCRT__atof_l
# stub _atoflt_l
@ cdecl -ret64 _atoi64(str) ntdll._atoi64
# stub _atoi64_l
# stub _atoi_l
# stub _atol_l
@ cdecl _atoldbl(ptr str) MSVCRT__atoldbl
# stub _atoldbl_l
@ cdecl _beep(long long)
@ cdecl _beginthread (ptr long ptr)
@ cdecl _beginthreadex (ptr long ptr ptr long ptr)
@ cdecl _c_exit() MSVCRT__c_exit
@ cdecl _cabs(long) MSVCRT__cabs
@ cdecl _callnewh(long)
# stub _calloc_dbg
@ cdecl _cexit() MSVCRT__cexit
@ cdecl _cgets(str)
# stub _cgets_s
# stub _cgetws
# stub _cgetws_s
@ cdecl _chdir(str) MSVCRT__chdir
@ cdecl _chdrive(long)
@ cdecl _chgsign( double )
@ cdecl -i386 -norelay _chkesp()
@ cdecl _chmod(str long) MSVCRT__chmod
@ cdecl _chsize(long long) MSVCRT__chsize
# stub _chsize_s
# stub _chvalidator
# stub _chvalidator_l
@ cdecl _clearfp()
@ cdecl _close(long) MSVCRT__close
@ cdecl _commit(long)
@ extern _commode MSVCRT__commode
@ cdecl _control87(long long)
@ cdecl _controlfp(long long)
@ cdecl _controlfp_s(ptr long long)
@ cdecl _copysign( double double )
@ varargs _cprintf(str)
# stub _cprintf_l
# stub _cprintf_p
# stub _cprintf_p_l
# stub _cprintf_s
# stub _cprintf_s_l
@ cdecl _cputs(str)
# stub _cputws
@ cdecl _creat(str long) MSVCRT__creat
# stub _crtAssertBusy
# stub _crtBreakAlloc
# stub _crtDbgFlag
@ varargs _cscanf(str)
@ varargs _cscanf_l(str ptr)
@ varargs _cscanf_s(str)
@ varargs _cscanf_s_l(str ptr)
@ cdecl _ctime32(ptr) MSVCRT__ctime32
# stub _ctime32_s
@ cdecl _ctime64(ptr) MSVCRT__ctime64
# stub _ctime64_s
@ extern _ctype MSVCRT__ctype
@ cdecl _cwait(ptr long long)
# stub _cwprintf
# stub _cwprintf_l
# stub _cwprintf_p
# stub _cwprintf_p_l
# stub _cwprintf_s
# stub _cwprintf_s_l
@ varargs _cwscanf(wstr)
@ varargs _cwscanf_l(wstr ptr)
@ varargs _cwscanf_s(wstr)
@ varargs _cwscanf_s_l(wstr ptr)
@ extern _daylight MSVCRT___daylight
@ cdecl _difftime32(long long) MSVCRT__difftime32
@ cdecl _difftime64(long long) MSVCRT__difftime64
@ extern _dstbias MSVCRT__dstbias
@ cdecl _dup (long) MSVCRT__dup
@ cdecl _dup2 (long long) MSVCRT__dup2
@ cdecl _ecvt(double long ptr ptr)
# stub _ecvt_s
@ cdecl _endthread ()
@ cdecl _endthreadex(long)
@ extern _environ MSVCRT__environ
@ cdecl _eof(long)
@ cdecl _errno() MSVCRT__errno
@ cdecl -i386 _except_handler2(ptr ptr ptr ptr)
@ cdecl -i386 _except_handler3(ptr ptr ptr ptr)
@ cdecl -i386 _except_handler4_common(ptr ptr ptr ptr ptr ptr)
@ varargs _execl(str str)
@ varargs _execle(str str)
@ varargs _execlp(str str)
@ varargs _execlpe(str str)
@ cdecl _execv(str ptr)
@ cdecl _execve(str ptr ptr) MSVCRT__execve
@ cdecl _execvp(str ptr)
@ cdecl _execvpe(str ptr ptr)
@ cdecl _exit(long) MSVCRT__exit
@ cdecl _expand(ptr long)
# stub _expand_dbg
@ cdecl _fcloseall() MSVCRT__fcloseall
@ cdecl _fcvt(double long ptr ptr)
# stub _fcvt_s
@ cdecl _fdopen(long str) MSVCRT__fdopen
@ cdecl _fgetchar()
@ cdecl _fgetwchar()
@ cdecl _filbuf(ptr) MSVCRT__filbuf
# extern _fileinfo
@ cdecl _filelength(long) MSVCRT__filelength
@ cdecl -ret64 _filelengthi64(long) MSVCRT__filelengthi64
@ cdecl _fileno(ptr) MSVCRT__fileno
@ cdecl _findclose(long) MSVCRT__findclose
@ cdecl _findfirst(str ptr) MSVCRT__findfirst
@ cdecl _findfirst64(str ptr) MSVCRT__findfirst64
@ cdecl _findfirsti64(str ptr) MSVCRT__findfirsti64
@ cdecl _findnext(long ptr) MSVCRT__findnext
@ cdecl _findnext64(long ptr) MSVCRT__findnext64
@ cdecl _findnexti64(long ptr) MSVCRT__findnexti64
@ cdecl _finite( double )
@ cdecl _flsbuf(long ptr) MSVCRT__flsbuf
@ cdecl _flushall()
@ extern _fmode MSVCRT__fmode
@ cdecl _fpclass(double)
@ stub _fpieee_flt #(long ptr ptr)
@ cdecl _fpreset()
# stub _fprintf_l
# stub _fprintf_p
# stub _fprintf_p_l
# stub _fprintf_s_l
@ cdecl _fputchar(long)
@ cdecl _fputwchar(long)
# stub _free_dbg
# stub _freea
# stub _freea_s
# stub _fscanf_l
@ varargs _fscanf_l(ptr str ptr) MSVCRT__fscanf_l
@ varargs _fscanf_s_l(ptr str ptr) MSVCRT__fscanf_s_l
# stub _fseeki64
@ cdecl _fsopen(str str long) MSVCRT__fsopen
@ cdecl _fstat(long ptr) MSVCRT__fstat
@ cdecl _fstat64(long ptr) MSVCRT__fstat64
@ cdecl _fstati64(long ptr) MSVCRT__fstati64
@ cdecl _ftime(ptr) MSVCRT__ftime
@ cdecl _ftime32(ptr) MSVCRT__ftime32
# stub _ftime32_s
@ cdecl _ftime64(ptr) MSVCRT__ftime64
# stub _ftime64_s
@ cdecl -ret64 _ftol() ntdll._ftol
@ cdecl -ret64 _ftol2() ntdll._ftol
@ cdecl -ret64 _ftol2_sse() ntdll._ftol #FIXME: SSE variant should be implemented
# stub _ftol2_sse_excpt
@ cdecl _fullpath(ptr str long)
# stub _fullpath_dbg
@ cdecl _futime(long ptr)
@ cdecl _futime32(long ptr)
@ cdecl _futime64(long ptr)
# stub _fwprintf_l
# stub _fwprintf_p
# stub _fwprintf_p_l
# stub _fwprintf_s_l
@ varargs _fwscanf_l(ptr wstr ptr) MSVCRT__fwscanf_l
@ varargs _fwscanf_s_l(ptr wstr ptr) MSVCRT__fwscanf_s_l
@ cdecl _gcvt(double long str)
@ cdecl _gcvt_s(ptr long  double long)
# stub _get_doserrno
# stub _get_environ
# stub _get_errno
# stub _get_fileinfo
# stub _get_fmode
@ cdecl _get_heap_handle()
@ cdecl _get_osfhandle(long)
@ cdecl _get_osplatform(ptr) MSVCRT__get_osplatform
# stub _get_osver
# stub _get_output_format
# stub _get_pgmptr
@ cdecl _get_sbh_threshold()
# stub _get_wenviron
# stub _get_winmajor
# stub _get_winminor
# stub _get_winver
# stub _get_wpgmptr
@ stub _get_terminate
@ stub _get_unexpected
@ cdecl _getch()
@ cdecl _getche()
@ cdecl _getcwd(str long)
@ cdecl _getdcwd(long str long)
@ cdecl _getdiskfree(long ptr) MSVCRT__getdiskfree
@ cdecl _getdllprocaddr(long str long)
@ cdecl _getdrive()
@ cdecl _getdrives() kernel32.GetLogicalDrives
@ cdecl _getmaxstdio()
@ cdecl _getmbcp()
@ cdecl _getpid() kernel32.GetCurrentProcessId
@ stub _getsystime #(ptr)
@ cdecl _getw(ptr) MSVCRT__getw
# stub _getwch
# stub _getwche
@ cdecl _getws(ptr) MSVCRT__getws
@ cdecl -i386 _global_unwind2(ptr)
@ cdecl _gmtime32(ptr) MSVCRT__gmtime32
@ cdecl _gmtime32_s(ptr ptr) MSVCRT__gmtime32_s
@ cdecl _gmtime64(ptr) MSVCRT__gmtime64
@ cdecl _gmtime64_s(ptr ptr) MSVCRT__gmtime64_s
@ cdecl _heapadd (ptr long)
@ cdecl _heapchk()
@ cdecl _heapmin()
@ cdecl _heapset(long)
@ stub _heapused #(ptr ptr)
@ cdecl _heapwalk(ptr)
@ cdecl _hypot(double double)
@ cdecl _hypotf(float float)
@ cdecl _i64toa(int64 ptr long) ntdll._i64toa
# stub _i64toa_s
@ cdecl _i64tow(int64 ptr long) ntdll._i64tow
# stub _i64tow_s
@ cdecl _initterm(ptr ptr)
# stub _initterm_e
@ stub _inp #(long) -i386
@ stub _inpd #(long) -i386
@ stub _inpw #(long) -i386
@ cdecl _invalid_parameter(wstr wstr wstr long long) MSVCRT__invalid_parameter
@ extern _iob MSVCRT__iob
# stub _isalnum_l
# stub _isalpha_l
@ cdecl _isatty(long)
# stub _iscntrl_l
@ cdecl _isctype(long long)
# stub _isctype_l
# stub _isdigit_l
# stub _isgraph_l
# stub _isleadbyte_l
# stub _islower_l
@ stub _ismbbalnum #(long)
# stub _ismbbalnum_l
@ stub _ismbbalpha #(long)
# stub _ismbbalpha_l
@ stub _ismbbgraph #(long)
# stub _ismbbgraph_l
@ stub _ismbbkalnum #(long)
# stub _ismbbkalnum_l
@ cdecl _ismbbkana(long)
# stub _ismbbkana_l
@ stub _ismbbkprint #(long)
# stub _ismbbkprint_l
@ stub _ismbbkpunct #(long)
# stub _ismbbkpunct_l
@ cdecl _ismbblead(long)
# stub _ismbblead_l
@ stub _ismbbprint #(long)
# stub _ismbbprint_l
@ stub _ismbbpunct #(long)
# stub _ismbbpunct_l
@ cdecl _ismbbtrail(long)
# stub _ismbbtrail_l
@ cdecl _ismbcalnum(long)
# stub _ismbcalnum_l
@ cdecl _ismbcalpha(long)
# stub _ismbcalpha_l
@ cdecl _ismbcdigit(long)
# stub _ismbcdigit_l
@ cdecl _ismbcgraph(long)
# stub _ismbcgraph_l
@ cdecl _ismbchira(long)
# stub _ismbchira_l
@ cdecl _ismbckata(long)
# stub _ismbckata_l
@ stub _ismbcl0 #(long)
# stub _ismbcl0_l
@ stub _ismbcl1 #(long)
# stub _ismbcl1_l
@ stub _ismbcl2 #(long)
# stub _ismbcl2_l
@ cdecl _ismbclegal(long)
# stub _ismbclegal_l
@ cdecl _ismbclower(long)
# stub _ismbclower_l
@ cdecl _ismbcprint(long)
# stub _ismbcprint_l
@ cdecl _ismbcpunct(long)
# stub _ismbcpunct_l
@ cdecl _ismbcspace(long)
# stub _ismbcspace_l
@ cdecl _ismbcsymbol(long)
# stub _ismbcsymbol_l
@ cdecl _ismbcupper(long)
# stub _ismbcupper_l
@ cdecl _ismbslead(ptr ptr)
# stub _ismbslead_l
@ cdecl _ismbstrail(ptr ptr)
# stub _ismbstrail_l
@ cdecl _isnan( double )
# stub _isprint_l
# stub _isspace_l
# stub _isupper_l
# stub _iswalnum_l
@ cdecl _iswalpha_l(long ptr) MSVCRT__iswalpha_l
# stub _iswcntrl_l
# stub _iswctype_l
# stub _iswdigit_l
# stub _iswgraph_l
# stub _iswlower_l
# stub _iswprint_l
# stub _iswpunct_l
# stub _iswspace_l
# stub _iswupper_l
# stub _iswxdigit_l
# stub _isxdigit_l
@ cdecl _itoa(long ptr long) ntdll._itoa
# stub _itoa_s
@ cdecl _itow(long ptr long) ntdll._itow
# stub _itow_s
@ cdecl _j0(double)
@ cdecl _j1(double)
@ cdecl _jn(long double)
@ cdecl _kbhit()
@ cdecl _lfind(ptr ptr ptr long ptr)
# stub _lfind_s
@ cdecl _loaddll(str)
@ cdecl -i386 _local_unwind2(ptr long)
@ cdecl -i386 _local_unwind4(ptr ptr long)
@ cdecl _localtime32(ptr) MSVCRT__localtime32
# stub _localtime32_s
@ cdecl _localtime64(ptr) MSVCRT__localtime64
# stub _localtime64_s
@ cdecl _lock(long)
@ cdecl _locking(long long long) MSVCRT__locking
@ cdecl _logb( double )
@ cdecl -i386 _longjmpex(ptr long) MSVCRT_longjmp
@ cdecl _lrotl(long long) MSVCRT__lrotl
@ cdecl _lrotr(long long) MSVCRT__lrotr
@ cdecl _lsearch(ptr ptr long long ptr)
# stub _lsearch_s
@ cdecl _lseek(long long long) MSVCRT__lseek
@ cdecl -ret64 _lseeki64(long int64 long) MSVCRT__lseeki64
@ cdecl _ltoa(long ptr long) ntdll._ltoa
# stub _ltoa_s
@ cdecl _ltow(long ptr long) ntdll._ltow
# stub _ltow_s
@ cdecl _makepath(ptr str str str str)
@ cdecl _makepath_s(ptr long str str str str)
# stub _malloc_dbg
@ cdecl _matherr(ptr) MSVCRT__matherr
@ cdecl _mbbtombc(long)
# stub _mbbtombc_l
@ cdecl _mbbtype(long long)
# extern _mbcasemap
@ cdecl _mbccpy (str str)
# stub _mbccpy_l
# stub _mbccpy_s
# stub _mbccpy_s_l
@ cdecl _mbcjistojms (long)
# stub _mbcjistojms_l
@ stub _mbcjmstojis #(long)
# stub _mbcjmstojis_l
@ cdecl _mbclen(ptr)
# stub _mbclen_l
@ stub _mbctohira #(long)
# stub _mbctohira_l
@ stub _mbctokata #(long)
# stub _mbctokata_l
@ cdecl _mbctolower(long)
# stub _mbctolower_l
@ cdecl _mbctombb(long)
# stub _mbctombb_l
@ cdecl _mbctoupper(long)
# stub _mbctoupper_l
@ extern _mbctype MSVCRT_mbctype
# stub _mblen_l
@ cdecl _mbsbtype(str long)
# stub _mbsbtype_l
@ cdecl _mbscat(str str)
# stub _mbscat_s
# stub _mbscat_s_l
@ cdecl _mbschr(str long)
# stub _mbschr_l
@ cdecl _mbscmp(str str)
# stub _mbscmp_l
@ cdecl _mbscoll(str str)
# stub _mbscoll_l
@ cdecl _mbscpy(ptr str)
# stub _mbscpy_s
# stub _mbscpy_s_l
@ cdecl _mbscspn (str str)
# stub _mbscspn_l
@ cdecl _mbsdec(ptr ptr)
# stub _mbsdec_l
@ cdecl _mbsdup(str) _strdup
# stub _strdup_dbg
@ cdecl _mbsicmp(str str)
# stub _mbsicmp_l
@ cdecl _mbsicoll(str str)
# stub _mbsicoll_l
@ cdecl _mbsinc(str)
# stub _mbsinc_l
@ cdecl _mbslen(str)
# stub _mbslen_l
@ cdecl _mbslwr(str)
# stub _mbslwr_l
# stub _mbslwr_s
# stub _mbslwr_s_l
@ cdecl _mbsnbcat (str str long)
# stub _mbsnbcat_l
# stub _mbsnbcat_s
# stub _mbsnbcat_s_l
@ cdecl _mbsnbcmp(str str long)
# stub _mbsnbcmp_l
@ cdecl _mbsnbcnt(ptr long)
# stub _mbsnbcnt_l
@ stub _mbsnbcoll #(str str long)
# stub _mbsnbcoll_l
@ cdecl _mbsnbcpy(ptr str long)
# stub _mbsnbcpy_l
@ cdecl _mbsnbcpy_s(ptr long str long)
# stub _mbsnbcpy_s_l
@ cdecl _mbsnbicmp(str str long)
# stub _mbsnbicmp_l
@ stub _mbsnbicoll #(str str long)
# stub _mbsnbicoll_l
@ cdecl _mbsnbset(str long long)
# stub _mbsnbset_l
# stub _mbsnbset_s
# stub _mbsnbset_s_l
@ cdecl _mbsncat(str str long)
# stub _mbsncat_l
# stub _mbsncat_s
# stub _mbsncat_s_l
@ cdecl _mbsnccnt(str long)
# stub _mbsnccnt_l
@ cdecl _mbsncmp(str str long)
# stub _mbsncmp_l
@ stub _mbsncoll #(str str long)
# stub _mbsncoll_l
@ cdecl _mbsncpy(str str long)
# stub _mbsncpy_l
# stub _mbsncpy_s
# stub _mbsncpy_s_l
@ cdecl _mbsnextc(str)
# stub _mbsnextc_l
@ cdecl _mbsnicmp(str str long)
# stub _mbsnicmp_l
@ stub _mbsnicoll #(str str long)
# stub _mbsnicoll_l
@ cdecl _mbsninc(str long)
# stub _mbsninc_l
# stub _mbsnlen
# stub _mbsnlen_l
@ cdecl _mbsnset(str long long)
# stub _mbsnset_l
# stub _mbsnset_s
# stub _mbsnset_s_l
@ cdecl _mbspbrk(str str)
# stub _mbspbrk_l
@ cdecl _mbsrchr(str long)
# stub _mbsrchr_l
@ cdecl _mbsrev(str)
# stub _mbsrev_l
@ cdecl _mbsset(str long)
# stub _mbsset_l
# stub _mbsset_s
# stub _mbsset_s_l
@ cdecl _mbsspn(str str)
# stub _mbsspn_l
@ cdecl _mbsspnp(str str)
# stub _mbsspnp_l
@ cdecl _mbsstr(str str)
# stub _mbsstr_l
@ cdecl _mbstok(str str)
# stub _mbstok_l
# stub _mbstok_s
# stub _mbstok_s_l
@ cdecl _mbstowcs_l(ptr str long ptr) MSVCRT__mbstowcs_l
@ cdecl _mbstowcs_s_l(ptr ptr long str long ptr) MSVCRT__mbstowcs_s_l
@ cdecl _mbstrlen(str)
@ cdecl _mbstrlen_l(str ptr)
# stub _mbstrnlen
# stub _mbstrnlen_l
@ cdecl _mbsupr(str)
# stub _mbsupr_l
# stub _mbsupr_s
# stub _mbsupr_s_l
# stub _mbtowc_l
@ cdecl _memccpy(ptr ptr long long) ntdll._memccpy
@ cdecl _memicmp(str str long) ntdll._memicmp
# stub _memicmp_l
@ cdecl _mkdir(str) MSVCRT__mkdir
@ cdecl _mkgmtime(ptr) MSVCRT__mkgmtime
@ cdecl _mkgmtime32(ptr) MSVCRT__mkgmtime32
@ cdecl _mkgmtime64(ptr) MSVCRT__mkgmtime64
@ cdecl _mktemp(str)
# stub _mktemp_s
@ cdecl _mktime32(ptr) MSVCRT__mktime32
@ cdecl _mktime64(ptr) MSVCRT__mktime64
@ cdecl _msize(ptr)
# stub _msize_debug
@ cdecl _nextafter(double double)
@ cdecl _onexit(ptr) MSVCRT__onexit
@ varargs _open(str long) MSVCRT__open
@ cdecl _open_osfhandle(long long)
@ extern _osplatform MSVCRT__osplatform
@ extern _osver MSVCRT__osver
@ stub _outp #(long long)
@ stub _outpd #(long long)
@ stub _outpw #(long long)
@ cdecl _pclose (ptr) MSVCRT__pclose
@ extern _pctype MSVCRT__pctype
@ extern _pgmptr MSVCRT__pgmptr
@ cdecl _pipe (ptr long long) MSVCRT__pipe
@ cdecl _popen (str str) MSVCRT__popen
# stub _printf_l
# stub _printf_p
# stub _printf_p_l
# stub _printf_s_l
@ cdecl _purecall()
@ cdecl _putch(long)
@ cdecl _putenv(str)
# stub _putenv_s
@ cdecl _putw(long ptr) MSVCRT__putw
# stub _putwch
@ cdecl _putws(wstr)
# extern _pwctype
@ cdecl _read(long ptr long) MSVCRT__read
# stub _realloc_dbg
# stub _resetstkoflw
@ cdecl _rmdir(str) MSVCRT__rmdir
@ cdecl _rmtmp()
@ cdecl _rotl(long long)
# stub _rotl64
@ cdecl _rotr(long long)
# stub _rotr64
@ cdecl -arch=i386 _safe_fdiv()
@ cdecl -arch=i386 _safe_fdivr()
@ cdecl -arch=i386 _safe_fprem()
@ cdecl -arch=i386 _safe_fprem1()
@ cdecl _scalb(double long) MSVCRT__scalb
@ cdecl -arch=x86_64 _scalbf(float long) MSVCRT__scalbf
@ varargs _scanf_l(str ptr) MSVCRT__scanf_l
@ varargs _scanf_s_l(str ptr) MSVCRT__scanf_s_l
@ varargs _scprintf(str) MSVCRT__scprintf
# stub _scprintf_l
# stub _scprintf_p_l
@ varargs _scwprintf(wstr) MSVCRT__scwprintf
# stub _scwprintf_l
# stub _scwprintf_p_l
@ cdecl _searchenv(str str ptr)
# stub _searchenv_s
# stub _seh_longjmp_unwind4
@ stdcall -i386 _seh_longjmp_unwind(ptr)
@ cdecl _set_SSE2_enable(long) MSVCRT__set_SSE2_enable
# stub _set_controlfp
# stub _set_doserrno
# stub _set_errno
@ cdecl _set_error_mode(long)
# stub _set_fileinfo
# stub _set_fmode
# stub _set_output_format
@ cdecl _set_sbh_threshold(long)
@ cdecl _seterrormode(long)
@ cdecl -arch=i386,x86_64 -norelay _setjmp(ptr) MSVCRT__setjmp
@ cdecl -arch=i386 -norelay _setjmp3(ptr long) MSVCRT__setjmp3
@ cdecl -arch=x86_64 -norelay _setjmpex(ptr ptr) MSVCRT__setjmpex
@ cdecl _setmaxstdio(long)
@ cdecl _setmbcp(long)
@ cdecl _setmode(long long)
@ stub _setsystime #(ptr long)
@ cdecl _sleep(long) MSVCRT__sleep
@ varargs _snprintf(ptr long str) MSVCRT__snprintf
# stub _snprintf_c
# stub _snprintf_c_l
# stub _snprintf_l
@ varargs _snprintf_s(ptr long long str) MSVCRT__snprintf_s
# stub _snprintf_s_l
# stub _snscanf
# stub _snscanf_l
# stub _snscanf_s
# stub _snscanf_s_l
@ varargs _snwprintf(ptr long wstr) MSVCRT__snwprintf
# stub _snwprintf_l
@ varargs _snwprintf_s(ptr long long wstr) MSVCRT__snwprintf_s
# stub _snwprintf_s_l
# stub _snwscanf
# stub _snwscanf_l
# stub _snwscanf_s
# stub _snwscanf_s_l
@ varargs _sopen(str long long) MSVCRT__sopen
# stub _sopen_s
@ varargs _spawnl(long str str)
@ varargs _spawnle(long str str)
@ varargs _spawnlp(long str str)
@ varargs _spawnlpe(long str str)
@ cdecl _spawnv(long str ptr)
@ cdecl _spawnve(long str ptr ptr) MSVCRT__spawnve
@ cdecl _spawnvp(long str ptr)
@ cdecl _spawnvpe(long str ptr ptr) MSVCRT__spawnvpe
@ cdecl _splitpath(str ptr ptr ptr ptr)
@ cdecl _splitpath_s(str ptr long ptr long ptr long ptr long)
# stub _sprintf_l
# stub _sprintf_p_l
# stub _sprintf_s_l
@ varargs _sscanf_l(str str ptr) MSVCRT__sscanf_l
@ varargs _sscanf_s_l(str str ptr) MSVCRT__sscanf_s_l
@ cdecl _stat(str ptr) MSVCRT_stat
@ cdecl _stat64(str ptr) MSVCRT_stat64
@ cdecl _stati64(str ptr) MSVCRT_stati64
@ cdecl _statusfp()
@ cdecl _strcmpi(str str) ntdll._strcmpi
# stub _strcoll_l
@ cdecl _strdate(ptr)
@ cdecl _strdate_s(ptr long)
@ cdecl _strdup(str)
# stub _strdup_dbg
@ cdecl _strerror(long)
# stub _strerror_s
@ cdecl _stricmp(str str) ntdll._stricmp
# stub _stricmp_l
@ cdecl _stricoll(str str) MSVCRT__stricoll
# stub _stricoll_l
@ cdecl _strlwr(str) ntdll._strlwr
# stub _strlwr_l
# stub _strlwr_s
# stub _strlwr_s_l
@ stub _strncoll #(str str long)
# stub _strncoll_l
@ cdecl _strnicmp(str str long) ntdll._strnicmp
# stub _strnicmp_l
@ stub _strnicoll #(str str long)
# stub _strnicoll_l
@ cdecl _strnset(str long long) MSVCRT__strnset
# stub _strnset_s
@ cdecl _strrev(str)
@ cdecl _strset(str long)
# stub _strset_s
@ cdecl _strtime(ptr)
@ cdecl _strtime_s(ptr long)
@ cdecl _strtod_l(str ptr ptr) MSVCRT_strtod_l
@ cdecl -ret64 _strtoi64(str ptr long) MSVCRT_strtoi64
@ cdecl -ret64 _strtoi64_l(str ptr long ptr) MSVCRT_strtoi64_l
# stub _strtol_l
@ cdecl -ret64 _strtoui64(str ptr long) MSVCRT_strtoui64
@ cdecl -ret64 _strtoui64_l(str ptr long ptr) MSVCRT_strtoui64_l
# stub _strtoul_l
@ cdecl _strupr(str) ntdll._strupr
# stub _strupr_l
# stub _strupr_s
# stub _strupr_s_l
# stub _strxfrm_l
@ cdecl _swab(str str long) MSVCRT__swab
# stub _swprintf
# stub _swprintf_c
# stub _swprintf_c_l
# stub _swprintf_p_l
# stub _swprintf_s_l
@ varargs _swscanf_l(wstr wstr ptr) MSVCRT__swscanf_l
@ varargs _swscanf_s_l(wstr wstr ptr) MSVCRT__swscanf_s_l
@ extern _sys_errlist MSVCRT__sys_errlist
@ extern _sys_nerr MSVCRT__sys_nerr
@ cdecl _tell(long) MSVCRT__tell
@ cdecl -ret64 _telli64(long)
@ cdecl _tempnam(str str)
# stub _tempnam_dbg
@ cdecl _time32(ptr) MSVCRT__time32
@ cdecl _time64(ptr) MSVCRT__time64
@ extern _timezone MSVCRT___timezone
@ cdecl _tolower(long) MSVCRT__tolower
# stub _tolower_l
@ cdecl _toupper(long) MSVCRT__toupper
# stub _toupper_l
# stub _towlower_l
# stub _towupper_l
@ extern _tzname MSVCRT__tzname
@ cdecl _tzset() MSVCRT__tzset
@ cdecl _ui64toa(int64 ptr long) ntdll._ui64toa
@ cdecl _ui64toa_s(int64 ptr long long) MSVCRT__ui64toa_s
@ cdecl _ui64tow(int64 ptr long) ntdll._ui64tow
# stub _ui64tow_s
@ cdecl _ultoa(long ptr long) ntdll._ultoa
# stub _ultoa_s
@ cdecl _ultow(long ptr long) ntdll._ultow
# stub _ultow_s
@ cdecl _umask(long) MSVCRT__umask
# stub _umask_s
@ cdecl _ungetch(long)
# stub _ungetwch
@ cdecl _unlink(str) MSVCRT__unlink
@ cdecl _unloaddll(long)
@ cdecl _unlock(long)
@ cdecl _utime32(str ptr)
@ cdecl _utime64(str ptr)
# stub _vcprintf
# stub _vcprintf_l
# stub _vcprintf_p
# stub _vcprintf_p_l
# stub _vcprintf_s
# stub _vcprintf_s_l
# stub _vcwprintf
# stub _vcwprintf_l
# stub _vcwprintf_p
# stub _vcwprintf_p_l
# stub _vcwprintf_s
# stub _vcwprintf_s_l
# stub _vfprintf_l
# stub _vfprintf_p
# stub _vfprintf_p_l
# stub _vfprintf_s_l
# stub _vfwprintf_l
# stub _vfwprintf_p
# stub _vfwprintf_p_l
# stub _vfwprintf_s_l
# stub _vprintf_l
# stub _vprintf_p
# stub _vprintf_p_l
# stub _vprintf_s_l
@ cdecl _utime(str ptr)
@ cdecl _vscprintf(str ptr)
# stub _vscprintf_l
# stub _vscprintf_p_l
@ cdecl _vscwprintf(wstr ptr)
# stub _vscwprintf_l
# stub _vscwprintf_p_l
@ cdecl _vsnprintf(ptr long str ptr) MSVCRT_vsnprintf
@ cdecl _vsnprintf_c(ptr long str ptr) MSVCRT_vsnprintf
@ cdecl _vsnprintf_c_l(ptr long str ptr ptr) MSVCRT_vsnprintf_l
@ cdecl _vsnprintf_l(ptr long str ptr ptr) MSVCRT_vsnprintf_l
@ cdecl _vsnprintf_s(ptr long long str ptr) MSVCRT_vsnprintf_s
@ cdecl _vsnprintf_s_l(ptr long long str ptr ptr) MSVCRT_vsnprintf_s_l
@ cdecl _vsnwprintf(ptr long wstr ptr) MSVCRT_vsnwprintf
@ cdecl _vsnwprintf_l(ptr long wstr ptr ptr) MSVCRT_vsnwprintf_l
@ cdecl _vsnwprintf_s(ptr long long wstr ptr) MSVCRT_vsnwprintf_s
@ cdecl _vsnwprintf_s_l(ptr long long wstr ptr ptr) MSVCRT_vsnwprintf_s_l
# stub _vsprintf_l
# stub _vsprintf_p
# stub _vsprintf_p_l
# stub _vsprintf_s_l
@ cdecl _vswprintf(ptr long wstr ptr) MSVCRT_vsnwprintf
@ cdecl _vswprintf_c(ptr long wstr ptr) MSVCRT_vsnwprintf
@ cdecl _vswprintf_c_l(ptr long wstr ptr ptr) MSVCRT_vsnwprintf_l
@ cdecl _vswprintf_l(ptr long wstr ptr ptr) MSVCRT_vsnwprintf_l
@ cdecl _vswprintf_p_l(ptr long wstr ptr ptr) MSVCRT_vsnwprintf_l
@ cdecl _vswprintf_s_l(ptr long wstr ptr ptr) MSVCRT_vswprintf_s_l
# stub _vwprintf_l
# stub _vwprintf_p
# stub _vwprintf_p_l
# stub _vwprintf_s_l
@ cdecl _waccess(wstr long)
# stub _waccess_s
@ cdecl _wasctime(ptr) MSVCRT__wasctime
# stub _wasctime_s
# stub _wassert
@ cdecl _wchdir(wstr)
@ cdecl _wchmod(wstr long)
@ extern _wcmdln MSVCRT__wcmdln
@ cdecl _wcreat(wstr long)
# stub _wcscoll_l
@ cdecl _wcsdup(wstr)
# stub _wcsdup_dbg
# stub _wcserror
# stub _wcserror_s
# stub _wcsftime_l
@ cdecl _wcsicmp(wstr wstr) ntdll._wcsicmp
# stub _wcsicmp_l
@ cdecl _wcsicoll(wstr wstr)
# stub _wcsicoll_l
@ cdecl _wcslwr(wstr) ntdll._wcslwr
# stub _wcslwr_l
# stub _wcslwr_s
# stub _wcslwr_s_l
@ stub _wcsncoll #(wstr wstr long)
# stub _wcsncoll_l
@ cdecl _wcsnicmp(wstr wstr long) ntdll._wcsnicmp
# stub _wcsnicmp_l
@ stub _wcsnicoll #(wstr wstr long)
# stub _wcsnicoll_l
@ cdecl _wcsnset(wstr long long) MSVCRT__wcsnset
# stub _wcsnset_s
@ cdecl _wcsrev(wstr)
@ cdecl _wcsset(wstr long)
# stub _wcsset_s
@ cdecl -ret64 _wcstoi64(wstr ptr long) MSVCRT__wcstoi64
@ cdecl -ret64 _wcstoi64_l(wstr ptr long ptr) MSVCRT__wcstoi64_l
# stub _wcstol_l
@ cdecl _wcstombs_l(ptr ptr long ptr) MSVCRT__wcstombs_l
@ cdecl _wcstombs_s_l(ptr ptr long wstr long ptr) MSVCRT__wcstombs_s_l
@ cdecl -ret64 _wcstoui64(wstr ptr long) MSVCRT__wcstoui64
@ cdecl -ret64 _wcstoui64_l(wstr ptr long ptr) MSVCRT__wcstoui64_l
# stub _wcstoul_l
@ cdecl _wcsupr(wstr) ntdll._wcsupr
# stub _wcsupr_l
@ cdecl _wcsupr_s(wstr long) MSVCRT__wcsupr_s
# stub _wcsupr_s_l
# stub _wcsxfrm_l
@ cdecl _wctime(ptr) MSVCRT__wctime
@ cdecl _wctime32(ptr) MSVCRT__wctime32
# stub _wctime32_s
@ cdecl _wctime64(ptr) MSVCRT__wctime64
# stub _wctime64_s
# stub _wctomb_l
# stub _wctomb_s_l
# stub _wctype
@ extern _wenviron MSVCRT__wenviron
@ varargs _wexecl(wstr wstr)
@ varargs _wexecle(wstr wstr)
@ varargs _wexeclp(wstr wstr)
@ varargs _wexeclpe(wstr wstr)
@ cdecl _wexecv(wstr ptr)
@ cdecl _wexecve(wstr ptr ptr)
@ cdecl _wexecvp(wstr ptr)
@ cdecl _wexecvpe(wstr ptr ptr)
@ cdecl _wfdopen(long wstr) MSVCRT__wfdopen
@ cdecl _wfindfirst(wstr ptr) MSVCRT__wfindfirst
# stub _wfindfirst64
@ cdecl _wfindfirsti64(wstr ptr) MSVCRT__wfindfirsti64
@ cdecl _wfindnext(long ptr) MSVCRT__wfindnext
# stub _wfindnext64
@ cdecl _wfindnexti64(long ptr) MSVCRT__wfindnexti64
@ cdecl _wfopen(wstr wstr) MSVCRT__wfopen
@ cdecl _wfopen_s(ptr wstr wstr) MSVCRT__wfopen_s
@ cdecl _wfreopen(wstr wstr ptr) MSVCRT__wfreopen
# stub _wfreopen_s
@ cdecl _wfsopen(wstr wstr long) MSVCRT__wfsopen
@ cdecl _wfullpath(ptr wstr long)
# stub _wfullpath_dbg
@ cdecl _wgetcwd(wstr long)
@ cdecl _wgetdcwd(long wstr long)
@ cdecl _wgetenv(wstr)
# stub _wgetenv_s
@ extern _winmajor MSVCRT__winmajor
@ extern _winminor MSVCRT__winminor
# stub _winput_s
@ extern _winver MSVCRT__winver
@ cdecl _wmakepath(ptr wstr wstr wstr wstr)
@ cdecl _wmakepath_s(ptr long wstr wstr wstr wstr)
@ cdecl _wmkdir(wstr)
@ cdecl _wmktemp(wstr)
# stub _wmktemp_s
@ varargs _wopen(wstr long)
# stub _woutput_s
@ stub _wperror #(wstr)
@ extern _wpgmptr MSVCRT__wpgmptr
@ cdecl _wpopen (wstr wstr) MSVCRT__wpopen
# stub _wprintf_l
# stub _wprintf_p
# stub _wprintf_p_l
# stub _wprintf_s_l
@ cdecl _wputenv(wstr)
# stub _wputenv_s
@ cdecl _wremove(wstr)
@ cdecl _wrename(wstr wstr)
@ cdecl _write(long ptr long) MSVCRT__write
@ cdecl _wrmdir(wstr)
@ varargs _wscanf_l(wstr ptr) MSVCRT__wscanf_l
@ varargs _wscanf_s_l(wstr ptr) MSVCRT__wscanf_s_l
@ cdecl _wsearchenv(wstr wstr ptr)
# stub _wsearchenv_s
@ cdecl _wsetlocale(long wstr) MSVCRT__wsetlocale
@ varargs _wsopen (wstr long long) MSVCRT__wsopen
# stub _wsopen_s
@ varargs _wspawnl(long wstr wstr)
@ varargs _wspawnle(long wstr wstr)
@ varargs _wspawnlp(long wstr wstr)
@ varargs _wspawnlpe(long wstr wstr)
@ cdecl _wspawnv(long wstr ptr)
@ cdecl _wspawnve(long wstr ptr ptr)
@ cdecl _wspawnvp(long wstr ptr)
@ cdecl _wspawnvpe(long wstr ptr ptr)
@ cdecl _wsplitpath(wstr ptr ptr ptr ptr)
@ cdecl _wsplitpath_s(wstr ptr long ptr long ptr long ptr long) _wsplitpath_s
@ cdecl _wstat(wstr ptr) MSVCRT__wstat
@ cdecl _wstati64(wstr ptr) MSVCRT__wstati64
@ cdecl _wstat64(wstr ptr) MSVCRT__wstat64
@ cdecl _wstrdate(ptr)
@ cdecl _wstrdate_s(ptr long)
@ cdecl _wstrtime(ptr)
@ cdecl _wstrtime_s(ptr long)
@ cdecl _wsystem(wstr)
@ cdecl _wtempnam(wstr wstr)
# stub _wtempnam_dbg
@ stub _wtmpnam #(ptr)
# stub _wtmpnam_s
@ cdecl _wtof(wstr) MSVCRT__wtof
@ cdecl _wtof_l(wstr ptr) MSVCRT__wtof_l
@ cdecl _wtoi(wstr) ntdll._wtoi
@ cdecl -ret64 _wtoi64(wstr) ntdll._wtoi64
# stub _wtoi64_l
# stub _wtoi_l
@ cdecl _wtol(wstr) ntdll._wtol
# stub _wtol_l
@ cdecl _wunlink(wstr)
@ cdecl _wutime(wstr ptr)
@ cdecl _wutime32(wstr ptr)
@ cdecl _wutime64(wstr ptr)
@ cdecl _y0(double)
@ cdecl _y1(double)
@ cdecl _yn(long double )
@ cdecl abort() MSVCRT_abort
@ cdecl abs(long) ntdll.abs
@ cdecl acos(double) MSVCRT_acos
@ cdecl -arch=x86_64 acosf(float) MSVCRT_acosf
@ cdecl asctime(ptr) MSVCRT_asctime
# stub asctime_s
@ cdecl asin(double) MSVCRT_asin
@ cdecl atan(double) MSVCRT_atan
@ cdecl atan2(double double) MSVCRT_atan2
@ cdecl -arch=x86_64 asinf(float) MSVCRT_asinf
@ cdecl -arch=x86_64 atanf(float) MSVCRT_atanf
@ cdecl -arch=x86_64 atan2f(float float) MSVCRT_atan2f
@ cdecl atexit(ptr) MSVCRT_atexit
@ cdecl atof(str) MSVCRT_atof
@ cdecl atoi(str) ntdll.atoi
@ cdecl atol(str) ntdll.atol
@ cdecl bsearch(ptr ptr long long ptr) ntdll.bsearch
# stub bsearch_s
@ cdecl btowc(long) MSVCRT_btowc
@ cdecl calloc(long long) MSVCRT_calloc
@ cdecl ceil(double) MSVCRT_ceil
@ cdecl -arch=x86_64 ceilf(float) MSVCRT_ceilf
@ cdecl clearerr(ptr) MSVCRT_clearerr
# stub clearerr_s
@ cdecl clock() MSVCRT_clock
@ cdecl cos(double) MSVCRT_cos
@ cdecl cosh(double) MSVCRT_cosh
@ cdecl -arch=x86_64 cosf(float) MSVCRT_cosf
@ cdecl -arch=x86_64 coshf(float) MSVCRT_coshf
@ cdecl ctime(ptr) MSVCRT_ctime
@ cdecl difftime(long long) MSVCRT_difftime
@ cdecl -ret64 div(long long) MSVCRT_div
@ cdecl exit(long) MSVCRT_exit
@ cdecl exp(double) MSVCRT_exp
@ cdecl -arch=x86_64 expf(float) MSVCRT_expf
@ cdecl fabs(double) MSVCRT_fabs
@ cdecl fclose(ptr) MSVCRT_fclose
@ cdecl feof(ptr) MSVCRT_feof
@ cdecl ferror(ptr) MSVCRT_ferror
@ cdecl fflush(ptr) MSVCRT_fflush
@ cdecl fgetc(ptr) MSVCRT_fgetc
@ cdecl fgetpos(ptr ptr) MSVCRT_fgetpos
@ cdecl fgets(ptr long ptr) MSVCRT_fgets
@ cdecl fgetwc(ptr) MSVCRT_fgetwc
@ cdecl fgetws(ptr long ptr) MSVCRT_fgetws
@ cdecl floor(double) MSVCRT_floor
@ cdecl -arch=x86_64 floorf(float) MSVCRT_floorf
@ cdecl fmod(double double) MSVCRT_fmod
@ cdecl -arch=x86_64 fmodf(float float) MSVCRT_fmodf
@ cdecl fopen(str str) MSVCRT_fopen
@ cdecl fopen_s(ptr str str) MSVCRT_fopen_s
@ varargs fprintf(ptr str) MSVCRT_fprintf
# stub fprintf_s
@ cdecl fputc(long ptr) MSVCRT_fputc
@ cdecl fputs(str ptr) MSVCRT_fputs
@ cdecl fputwc(long ptr) MSVCRT_fputwc
@ cdecl fputws(wstr ptr) MSVCRT_fputws
@ cdecl fread(ptr long long ptr) MSVCRT_fread
@ cdecl free(ptr) MSVCRT_free
@ cdecl freopen(str str ptr) MSVCRT_freopen
# stub freopen_s
@ cdecl frexp(double ptr) MSVCRT_frexp
@ cdecl -arch=x86_64 frexpf(float ptr) MSVCRT_frexpf
@ varargs fscanf(ptr str) MSVCRT_fscanf
@ varargs fscanf_s(ptr str) MSVCRT_fscanf_s
@ cdecl fseek(ptr long long) MSVCRT_fseek
@ cdecl fsetpos(ptr ptr) MSVCRT_fsetpos
@ cdecl ftell(ptr) MSVCRT_ftell
@ varargs fwprintf(ptr wstr) MSVCRT_fwprintf
# stub fwprintf_s
@ cdecl fwrite(ptr long long ptr) MSVCRT_fwrite
@ varargs fwscanf(ptr wstr) MSVCRT_fwscanf
@ varargs fwscanf_s(ptr wstr) MSVCRT_fwscanf_s
@ cdecl getc(ptr) MSVCRT_getc
@ cdecl getchar() MSVCRT_getchar
@ cdecl getenv(str) MSVCRT_getenv
# stub getenv_s
@ cdecl gets(str) MSVCRT_gets
@ cdecl getwc(ptr) MSVCRT_getwc
@ cdecl getwchar() MSVCRT_getwchar
@ cdecl gmtime(ptr) MSVCRT_gmtime
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
@ cdecl labs(long) ntdll.labs
@ cdecl ldexp( double long) MSVCRT_ldexp
@ cdecl ldiv(long long) MSVCRT_ldiv
@ cdecl localeconv() MSVCRT_localeconv
@ cdecl localtime(ptr) MSVCRT_localtime
@ cdecl log(double) MSVCRT_log
@ cdecl log10(double) MSVCRT_log10
@ cdecl -arch=x86_64 logf(float) MSVCRT_logf
@ cdecl -arch=x86_64 log10f(float) MSVCRT_log10f
@ cdecl -i386 longjmp(ptr long) MSVCRT_longjmp
@ cdecl malloc(long) MSVCRT_malloc
@ cdecl mblen(ptr long) MSVCRT_mblen
# stub mbrlen
# stub mbrtowc
# stub mbsdup_dbg
# stub mbsrtowcs
# stub mbsrtowcs_s
@ cdecl mbstowcs(ptr str long) MSVCRT_mbstowcs
@ cdecl mbstowcs_s(ptr ptr long str long) MSVCRT__mbstowcs_s
@ cdecl mbtowc(wstr str long) MSVCRT_mbtowc
@ cdecl memchr(ptr long long) ntdll.memchr
@ cdecl memcmp(ptr ptr long) ntdll.memcmp
@ cdecl memcpy(ptr ptr long) ntdll.memcpy
@ cdecl memcpy_s(ptr long ptr long) memmove_s
@ cdecl memmove(ptr ptr long) ntdll.memmove
@ cdecl memmove_s(ptr long ptr long)
@ cdecl memset(ptr long long) ntdll.memset
@ cdecl mktime(ptr) MSVCRT_mktime
@ cdecl modf(double ptr) MSVCRT_modf
@ cdecl -arch=x86_64 modff(float ptr) MSVCRT_modff
@ cdecl perror(str) MSVCRT_perror
@ cdecl pow(double double) MSVCRT_pow
@ cdecl -arch=x86_64 powf(float float) MSVCRT_powf
@ varargs printf(str) MSVCRT_printf
# stub printf_s
@ cdecl putc(long ptr) MSVCRT_putc
@ cdecl putchar(long) MSVCRT_putchar
@ cdecl puts(str) MSVCRT_puts
@ cdecl putwc(long ptr) MSVCRT_fputwc
@ cdecl putwchar(long) _fputwchar
@ cdecl qsort(ptr long long ptr) ntdll.qsort
# stub qsort_s
@ cdecl raise(long) MSVCRT_raise
@ cdecl rand() MSVCRT_rand
@ cdecl rand_s(ptr) MSVCRT_rand_s
@ cdecl realloc(ptr long) MSVCRT_realloc
@ cdecl remove(str) MSVCRT_remove
@ cdecl rename(str str) MSVCRT_rename
@ cdecl rewind(ptr) MSVCRT_rewind
@ varargs scanf(str) MSVCRT_scanf
@ varargs scanf_s(str) MSVCRT_scanf_s
@ cdecl setbuf(ptr ptr) MSVCRT_setbuf
@ cdecl -arch=x86_64 -norelay -private setjmp(ptr) MSVCRT__setjmp
@ cdecl setlocale(long str) MSVCRT_setlocale
@ cdecl setvbuf(ptr str long long) MSVCRT_setvbuf
@ cdecl signal(long long) MSVCRT_signal
@ cdecl sin(double) MSVCRT_sin
@ cdecl sinh(double) MSVCRT_sinh
@ cdecl -arch=x86_64 sinf(float) MSVCRT_sinf
@ cdecl -arch=x86_64 sinhf(float) MSVCRT_sinhf
@ varargs sprintf(ptr str) MSVCRT_sprintf
@ varargs sprintf_s(ptr long str) MSVCRT_sprintf_s
@ cdecl sqrt(double) MSVCRT_sqrt
@ cdecl -arch=x86_64 sqrtf(float) MSVCRT_sqrtf
@ cdecl srand(long) MSVCRT_srand
@ varargs sscanf(str str) MSVCRT_sscanf
@ varargs sscanf_s(str str) MSVCRT_sscanf_s
@ cdecl strcat(str str) ntdll.strcat
@ cdecl strcat_s(str long str) MSVCRT_strcat_s
@ cdecl strchr(str long) ntdll.strchr
@ cdecl strcmp(str str) ntdll.strcmp
@ cdecl strcoll(str str) MSVCRT_strcoll
@ cdecl strcpy(ptr str) ntdll.strcpy
@ cdecl strcpy_s(ptr long str) MSVCRT_strcpy_s
@ cdecl strcspn(str str) ntdll.strcspn
@ cdecl strerror(long) MSVCRT_strerror
# stub strerror_s
@ cdecl strftime(str long str ptr) MSVCRT_strftime
@ cdecl strlen(str) ntdll.strlen
@ cdecl strncat(str str long) ntdll.strncat
# stub strncat_s
@ cdecl strncmp(str str long) ntdll.strncmp
@ cdecl strncpy(ptr str long) ntdll.strncpy
@ cdecl strncpy_s(ptr long str long)
@ cdecl strnlen(str long) MSVCRT_strnlen
@ cdecl strpbrk(str str) ntdll.strpbrk
@ cdecl strrchr(str long) ntdll.strrchr
@ cdecl strspn(str str) ntdll.strspn
@ cdecl strstr(str str) ntdll.strstr
@ cdecl strtod(str ptr) MSVCRT_strtod
@ cdecl strtok(str str) MSVCRT_strtok
@ cdecl strtok_s(ptr str ptr) MSVCRT_strtok_s
@ cdecl strtol(str ptr long) MSVCRT_strtol
@ cdecl strtoul(str ptr long) MSVCRT_strtoul
@ cdecl strxfrm(ptr str long) MSVCRT_strxfrm
@ varargs swprintf(ptr wstr) MSVCRT_swprintf
@ varargs swprintf_s(ptr long wstr) MSVCRT_swprintf_s
@ varargs swscanf(wstr wstr) MSVCRT_swscanf
@ varargs swscanf_s(wstr wstr) MSVCRT_swscanf_s
@ cdecl system(str) MSVCRT_system
@ cdecl tan(double) MSVCRT_tan
@ cdecl tanh(double) MSVCRT_tanh
@ cdecl -arch=x86_64 tanf(float) MSVCRT_tanf
@ cdecl -arch=x86_64 tanhf(float) MSVCRT_tanhf
@ cdecl time(ptr) MSVCRT_time
@ cdecl tmpfile() MSVCRT_tmpfile
# stub tmpfile_s
@ cdecl tmpnam(ptr) MSVCRT_tmpnam
# stub tmpnam_s
@ cdecl tolower(long) ntdll.tolower
@ cdecl toupper(long) ntdll.toupper
@ cdecl towlower(long) ntdll.towlower
@ cdecl towupper(long) ntdll.towupper
@ cdecl ungetc(long ptr) MSVCRT_ungetc
@ cdecl ungetwc(long ptr) MSVCRT_ungetwc
# stub utime
@ cdecl vfprintf(ptr str ptr) MSVCRT_vfprintf
# stub vfprintf_s
@ cdecl vfwprintf(ptr wstr ptr) MSVCRT_vfwprintf
# stub vfwprintf_s
@ cdecl vprintf(str ptr) MSVCRT_vprintf
# stub vprintf_s
# stub vsnprintf
@ cdecl vsprintf(ptr str ptr) MSVCRT_vsprintf
@ cdecl vsprintf_s(ptr long str ptr) MSVCRT_vsprintf_s
@ cdecl vswprintf(ptr wstr ptr) MSVCRT_vswprintf
@ cdecl vswprintf_s(ptr long wstr ptr) MSVCRT_vswprintf_s
@ cdecl vwprintf(wstr ptr) MSVCRT_vwprintf
# stub vwprintf_s
# stub wcrtomb
# stub wcrtomb_s
@ cdecl wcscat(wstr wstr) ntdll.wcscat
@ cdecl wcscat_s(wstr long wstr) MSVCRT_wcscat_s
@ cdecl wcschr(wstr long) ntdll.wcschr
@ cdecl wcscmp(wstr wstr) ntdll.wcscmp
@ cdecl wcscoll(wstr wstr) MSVCRT_wcscoll
@ cdecl wcscpy(ptr wstr) ntdll.wcscpy
@ cdecl wcscpy_s(ptr long wstr) MSVCRT_wcscpy_s
@ cdecl wcscspn(wstr wstr) ntdll.wcscspn
@ cdecl wcsftime(ptr long wstr ptr) MSVCRT_wcsftime
@ cdecl wcslen(wstr) ntdll.wcslen
@ cdecl wcsncat(wstr wstr long) ntdll.wcsncat
# stub wcsncat_s
@ cdecl wcsncmp(wstr wstr long) ntdll.wcsncmp
@ cdecl wcsncpy(ptr wstr long) ntdll.wcsncpy
@ cdecl wcsncpy_s(ptr long wstr long) MSVCRT_wcsncpy_s
# stub wcsnlen
@ cdecl wcspbrk(wstr wstr) MSVCRT_wcspbrk
@ cdecl wcsrchr(wstr long) ntdll.wcsrchr
# stub wcsrtombs
# stub wcsrtombs_s
@ cdecl wcsspn(wstr wstr) ntdll.wcsspn
@ cdecl wcsstr(wstr wstr) ntdll.wcsstr
@ cdecl wcstod(wstr ptr) MSVCRT_wcstod
@ cdecl wcstok(wstr wstr) MSVCRT_wcstok
# stub wcstok_s
@ cdecl wcstol(wstr ptr long) ntdll.wcstol
@ cdecl wcstombs(ptr ptr long) MSVCRT_wcstombs
@ cdecl wcstombs_s(ptr ptr long wstr long) MSVCRT_wcstombs_s
@ cdecl wcstoul(wstr ptr long) ntdll.wcstoul
@ stub wcsxfrm #(ptr wstr long) MSVCRT_wcsxfrm
# stub wctob
@ cdecl wctomb(ptr long) MSVCRT_wctomb
# stub wctomb_s
@ varargs wprintf(wstr) MSVCRT_wprintf
# stub wprintf_s
@ varargs wscanf(wstr) MSVCRT_wscanf
@ varargs wscanf_s(wstr) MSVCRT_wscanf_s

# Functions not exported in native dll:
@ cdecl _get_invalid_parameter_handler()
@ cdecl _set_invalid_parameter_handler(ptr)
@ cdecl _create_locale(long str) MSVCRT__create_locale
@ cdecl _free_locale(ptr) MSVCRT__free_locale
@ cdecl _configthreadlocale(long)
@ cdecl _wcstod_l(wstr ptr) MSVCRT__wcstod_l
@ cdecl ___mb_cur_max_l_func(ptr)
@ cdecl _set_purecall_handler(ptr)
