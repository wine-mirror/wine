@ stdcall ImmAssociateContext(long long)
@ stdcall ImmConfigureIMEA(long long long ptr)
@ stdcall ImmConfigureIMEW(long long long ptr)
@ stdcall ImmCreateContext()
@ stub ImmCreateIMCC
@ stub ImmCreateSoftKeyboard
@ stdcall ImmDestroyContext(long)
@ stub ImmDestroyIMCC
@ stub ImmDestroySoftKeyboard
@ stdcall ImmDisableIME(long)
@ stdcall ImmEnumRegisterWordA(long ptr str long str ptr)
@ stdcall ImmEnumRegisterWordW(long ptr wstr long wstr ptr)
@ stdcall ImmEscapeA(long long long ptr)
@ stdcall ImmEscapeW(long long long ptr)
@ stub ImmGenerateMessage
@ stdcall ImmGetCandidateListA(long long ptr long)
@ stdcall ImmGetCandidateListCountA(long ptr)
@ stdcall ImmGetCandidateListCountW(long ptr)
@ stdcall ImmGetCandidateListW(long long ptr long)
@ stdcall ImmGetCandidateWindow(long long ptr)
@ stdcall ImmGetCompositionFontA(long ptr)
@ stdcall ImmGetCompositionFontW(long ptr)
@ stdcall ImmGetCompositionStringA (long long ptr long)
@ stdcall ImmGetCompositionStringW (long long ptr long)
@ stdcall ImmGetCompositionString (long long ptr long) ImmGetCompositionStringA
@ stdcall ImmGetCompositionWindow(long ptr)
@ stdcall ImmGetContext(long)
@ stdcall ImmGetConversionListA(long long str ptr long long)
@ stdcall ImmGetConversionListW(long long wstr ptr long long)
@ stdcall ImmGetConversionStatus(long ptr ptr)
@ stdcall ImmGetDefaultIMEWnd(long)
@ stdcall ImmGetDescriptionA(long ptr long)
@ stdcall ImmGetDescriptionW(long ptr long)
@ stdcall ImmGetGuideLineA(long long ptr long)
@ stdcall ImmGetGuideLineW(long long ptr long)
@ stub ImmGetHotKey
@ stub ImmGetIMCCLockCount
@ stub ImmGetIMCCSize
@ stub ImmGetIMCLockCount
@ stdcall ImmGetIMEFileNameA(long ptr long)
@ stdcall ImmGetIMEFileNameW(long ptr long)
@ stdcall ImmGetOpenStatus(long)
@ stdcall ImmGetProperty(long long)
@ stdcall ImmGetRegisterWordStyleA(long long ptr)
@ stdcall ImmGetRegisterWordStyleW(long long ptr)
@ stdcall ImmGetStatusWindowPos(long ptr)
@ stdcall ImmGetVirtualKey(long)
@ stdcall ImmInstallIMEA(str str)
@ stdcall ImmInstallIMEW(wstr wstr)
@ stdcall ImmIsIME(long)
@ stdcall ImmIsUIMessageA(long long long long)
@ stdcall ImmIsUIMessageW(long long long long)
@ stub ImmLockIMC
@ stub ImmLockIMCC
@ stdcall ImmNotifyIME(long long long long)
@ stub ImmReSizeIMCC
@ stdcall ImmRegisterWordA(long str long str)
@ stdcall ImmRegisterWordW(long wstr long wstr)
@ stdcall ImmReleaseContext(long long)
@ stdcall ImmSetCandidateWindow(long ptr)
@ stdcall ImmSetCompositionFontA(long ptr)
@ stdcall ImmSetCompositionFontW(long ptr)
@ stdcall ImmSetCompositionStringA(long long ptr long ptr long)
@ stdcall ImmSetCompositionStringW(long long ptr long ptr long)
@ stdcall ImmSetCompositionWindow(long ptr)
@ stdcall ImmSetConversionStatus(long long long)
@ stub ImmSetHotKey
@ stdcall ImmSetOpenStatus(long long)
@ stdcall ImmSetStatusWindowPos(long ptr)
@ stub ImmShowSoftKeyboard
@ stdcall ImmSimulateHotKey(long long)
@ stub ImmUnlockIMC
@ stub ImmUnlockIMCC
@ stdcall ImmUnregisterWordA(long str long str)
@ stdcall ImmUnregisterWordW(long wstr long wstr)
