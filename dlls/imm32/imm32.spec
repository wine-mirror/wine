name	imm32
type	win32

@ stdcall ImmAssociateContext(long long) ImmAssociateContext
@ stdcall ImmConfigureIMEA(long long long ptr) ImmConfigureIMEA
@ stdcall ImmConfigureIMEW(long long long ptr) ImmConfigureIMEW
@ stdcall ImmCreateContext() ImmCreateContext
@ stub ImmCreateIMCC
@ stub ImmCreateSoftKeyboard
@ stdcall ImmDestroyContext(long) ImmDestroyContext
@ stub ImmDestroyIMCC
@ stub ImmDestroySoftKeyboard
@ stdcall ImmEnumRegisterWordA(long ptr str long str ptr) ImmEnumRegisterWordA
@ stdcall ImmEnumRegisterWordW(long ptr wstr long wstr ptr) ImmEnumRegisterWordW
@ stdcall ImmEscapeA(long long long ptr) ImmEscapeA
@ stdcall ImmEscapeW(long long long ptr) ImmEscapeW
@ stub ImmGenerateMessage
@ stdcall ImmGetCandidateListA(long long ptr long) ImmGetCandidateListA
@ stdcall ImmGetCandidateListCountA(long ptr) ImmGetCandidateListCountA
@ stdcall ImmGetCandidateListCountW(long ptr) ImmGetCandidateListCountW
@ stdcall ImmGetCandidateListW(long long ptr long) ImmGetCandidateListW
@ stdcall ImmGetCandidateWindow(long long ptr) ImmGetCandidateWindow
@ stdcall ImmGetCompositionFontA(long ptr) ImmGetCompositionFontA
@ stdcall ImmGetCompositionFontW(long ptr) ImmGetCompositionFontW
@ stdcall ImmGetCompositionStringA (long long ptr long) ImmGetCompositionStringA
@ stdcall ImmGetCompositionStringW (long long ptr long) ImmGetCompositionStringW
@ stdcall ImmGetCompositionWindow(long ptr) ImmGetCompositionWindow
@ stdcall ImmGetContext(long) ImmGetContext
@ stdcall ImmGetConversionListA(long long str ptr long long) ImmGetConversionListA
@ stdcall ImmGetConversionListW(long long wstr ptr long long) ImmGetConversionListW
@ stdcall ImmGetConversionStatus(long ptr ptr) ImmGetConversionStatus
@ stdcall ImmGetDefaultIMEWnd(long) ImmGetDefaultIMEWnd
@ stdcall ImmGetDescriptionA(long str long) ImmGetDescriptionA
@ stdcall ImmGetDescriptionW(long wstr long) ImmGetDescriptionW
@ stdcall ImmGetGuideLineA(long long str long) ImmGetGuideLineA
@ stdcall ImmGetGuideLineW(long long wstr long) ImmGetGuideLineW
@ stub ImmGetHotKey
@ stub ImmGetIMCCLockCount
@ stub ImmGetIMCCSize
@ stub ImmGetIMCLockCount
@ stdcall ImmGetIMEFileNameA(long str long) ImmGetIMEFileNameA
@ stdcall ImmGetIMEFileNameW(long wstr long) ImmGetIMEFileNameW
@ stdcall ImmGetOpenStatus(long) ImmGetOpenStatus
@ stdcall ImmGetProperty(long long) ImmGetProperty
@ stdcall ImmGetRegisterWordStyleA(long long ptr) ImmGetRegisterWordStyleA
@ stdcall ImmGetRegisterWordStyleW(long long ptr) ImmGetRegisterWordStyleW
@ stdcall ImmGetStatusWindowPos(long ptr) ImmGetStatusWindowPos
@ stdcall ImmGetVirtualKey(long) ImmGetVirtualKey
@ stdcall ImmInstallIMEA(str str) ImmInstallIMEA
@ stdcall ImmInstallIMEW(wstr wstr) ImmInstallIMEW
@ stdcall ImmIsIME(long) ImmIsIME
@ stdcall ImmIsUIMessageA(long long long long) ImmIsUIMessageA
@ stdcall ImmIsUIMessageW(long long long long) ImmIsUIMessageW
@ stub ImmLockIMC
@ stub ImmLockIMCC
@ stdcall ImmNotifyIME(long long long long) ImmNotifyIME
@ stub ImmReSizeIMCC
@ stdcall ImmRegisterWordA(long str long str) ImmRegisterWordA
@ stdcall ImmRegisterWordW(long wstr long wstr) ImmRegisterWordW
@ stdcall ImmReleaseContext(long long) ImmReleaseContext
@ stdcall ImmSetCandidateWindow(long ptr) ImmSetCandidateWindow
@ stdcall ImmSetCompositionFontA(long ptr) ImmSetCompositionFontA
@ stdcall ImmSetCompositionFontW(long ptr) ImmSetCompositionFontW
@ stdcall ImmSetCompositionStringA(long long ptr long ptr long) ImmSetCompositionStringA
@ stdcall ImmSetCompositionStringW(long long ptr long ptr long) ImmSetCompositionStringW
@ stdcall ImmSetCompositionWindow(long ptr) ImmSetCompositionWindow
@ stdcall ImmSetConversionStatus(long long long) ImmSetConversionStatus
@ stub ImmSetHotKey
@ stdcall ImmSetOpenStatus(long long) ImmSetOpenStatus
@ stdcall ImmSetStatusWindowPos(long ptr) ImmSetStatusWindowPos
@ stub ImmShowSoftKeyboard
@ stdcall ImmSimulateHotKey(long long) ImmSimulateHotKey
@ stub ImmUnlockIMC
@ stub ImmUnlockIMCC
@ stdcall ImmUnregisterWordA(long str long str) ImmUnregisterWordA
@ stdcall ImmUnregisterWordW(long wstr long wstr) ImmUnregisterWordW
