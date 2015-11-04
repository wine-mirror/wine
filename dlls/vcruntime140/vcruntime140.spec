@ stub _CreateFrameInfo
@ stdcall _CxxThrowException(long long) ucrtbase._CxxThrowException
@ cdecl -arch=i386 -norelay _EH_prolog() ucrtbase._EH_prolog
@ stub _FindAndUnlinkFrame
@ stub _IsExceptionObjectToBeDestroyed
@ stub _NLG_Dispatch2
@ stub _NLG_Return
@ stub _NLG_Return2
@ stub _SetWinRTOutOfMemoryExceptionCallback
@ cdecl __AdjustPointer(ptr ptr) ucrtbase.__AdjustPointer
@ stub __BuildCatchObject
@ stub __BuildCatchObjectHelper
@ stdcall -arch=x86_64 __C_specific_handler(ptr long ptr ptr) ucrtbase.__C_specific_handler
@ stub __C_specific_handler_noexcept
@ cdecl -arch=i386,x86_64,arm __CxxDetectRethrow(ptr) ucrtbase.__CxxDetectRethrow
@ cdecl -arch=i386,x86_64,arm __CxxExceptionFilter(ptr ptr long ptr) ucrtbase.__CxxExceptionFilter
@ cdecl -arch=i386,x86_64,arm -norelay __CxxFrameHandler(ptr ptr ptr ptr) ucrtbase.__CxxFrameHandler
@ cdecl -arch=i386,x86_64,arm -norelay __CxxFrameHandler2(ptr ptr ptr ptr) ucrtbase.__CxxFrameHandler2
@ cdecl -arch=i386,x86_64,arm -norelay __CxxFrameHandler3(ptr ptr ptr ptr) ucrtbase.__CxxFrameHandler3
@ stdcall -arch=i386 __CxxLongjmpUnwind(ptr) ucrtbase.__CxxLongjmpUnwind
@ cdecl -arch=i386,x86_64,arm __CxxQueryExceptionSize() ucrtbase.__CxxQueryExceptionSize
@ stub __CxxRegisterExceptionObject
@ stub __CxxUnregisterExceptionObject
@ stub __DestructExceptionObject
@ stub __FrameUnwindFilter
@ stub __GetPlatformExceptionInfo
@ stub __NLG_Dispatch2
@ stub __NLG_Return2
@ cdecl __RTCastToVoid(ptr) ucrtbase.__RTCastToVoid
@ cdecl __RTDynamicCast(ptr long ptr ptr long) ucrtbase.__RTDynamicCast
@ cdecl __RTtypeid(ptr) ucrtbase.__RTtypeid
@ stub __TypeMatch
@ stub __current_exception
@ stub __current_exception_context
@ stub __intrinsic_setjmp
@ stub __intrinsic_setjmpex
@ stub __processing_throw
@ stub __report_gsfailure
@ stub __std_exception_copy
@ stub __std_exception_destroy
@ stub __std_terminate
@ stub __std_type_info_compare
@ stub __std_type_info_destroy_list
@ stub __std_type_info_hash
@ stub __std_type_info_name
@ cdecl __telemetry_main_invoke_trigger(ptr)
@ cdecl __telemetry_main_return_trigger(ptr)
@ cdecl __unDName(ptr str long ptr ptr long) ucrtbase.__unDName
@ cdecl __unDNameEx(ptr str long ptr ptr ptr long) ucrtbase.__unDNameEx
@ cdecl __uncaught_exception() ucrtbase.__uncaught_exception
@ stub __uncaught_exceptions
@ stub __vcrt_GetModuleFileNameW
@ stub __vcrt_GetModuleHandleW
@ cdecl __vcrt_InitializeCriticalSectionEx(ptr long long)
@ stub __vcrt_LoadLibraryExW
@ cdecl -arch=i386 -norelay _chkesp() ucrtbase._chkesp
@ cdecl -arch=i386 _except_handler2(ptr ptr ptr ptr) ucrtbase._except_handler2
@ cdecl -arch=i386 _except_handler3(ptr ptr ptr ptr) ucrtbase._except_handler3
@ cdecl -arch=i386 _except_handler4_common(ptr ptr ptr ptr ptr ptr) ucrtbase._except_handler4_common
@ stub _get_purecall_handler
@ cdecl _get_unexpected() ucrtbase._get_unexpected
@ cdecl -arch=i386 _global_unwind2(ptr) ucrtbase._global_unwind2
@ stub _is_exception_typeof
@ cdecl -arch=i386 _local_unwind2(ptr long) ucrtbase._local_unwind2
@ cdecl -arch=i386 _local_unwind4(ptr ptr long) ucrtbase._local_unwind4
@ cdecl -arch=i386 _longjmpex(ptr long) ucrtbase._longjmpex
@ cdecl -arch=x86_64 _local_unwind(ptr ptr) ucrtbase._local_unwind
@ cdecl _purecall() ucrtbase._purecall
@ stdcall -arch=i386 _seh_longjmp_unwind4(ptr) ucrtbase._seh_longjmp_unwind4
@ stdcall -arch=i386 _seh_longjmp_unwind(ptr) ucrtbase._seh_longjmp_unwind
@ cdecl _set_purecall_handler(ptr) ucrtbase._set_purecall_handler
@ stub _set_se_translator
@ cdecl -arch=i386 -norelay _setjmp3(ptr long) ucrtbase._setjmp3
@ cdecl -arch=i386,x86_64,arm longjmp(ptr long) ucrtbase.longjmp
@ cdecl memchr(ptr long long) ucrtbase.memchr
@ cdecl memcmp(ptr ptr long) ucrtbase.memcmp
@ cdecl memcpy(ptr ptr long) ucrtbase.memcpy
@ cdecl memmove(ptr ptr long) ucrtbase.memmove
@ cdecl memset(ptr long long) ucrtbase.memset
@ stub set_unexpected
@ cdecl strchr(str long) ucrtbase.strchr
@ cdecl strrchr(str long) ucrtbase.strrchr
@ cdecl strstr(str str) ucrtbase.strstr
@ stub unexpected
@ cdecl wcschr(wstr long) ucrtbase.wcschr
@ cdecl wcsrchr(wstr long) ucrtbase.wcsrchr
@ cdecl wcsstr(wstr wstr) ucrtbase.wcsstr
