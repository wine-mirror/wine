name	imm32
type	win32

import	kernel32.dll
import	ntdll.dll

debug_channels (imm)

@ stdcall ImmAssociateContext(long long) ImmAssociateContext
@ stub ImmAssociateContextEx
@ stub ImmConfigureIME
@ stdcall ImmConfigureIMEA(long long long ptr) ImmConfigureIMEA
@ stdcall ImmConfigureIMEW(long long long ptr) ImmConfigureIMEW
@ stdcall ImmCreateContext() ImmCreateContext
@ stdcall ImmCreateIMCC(long) ImmCreateIMCC
@ stdcall ImmCreateSoftKeyboard(long long long long) ImmCreateSoftKeyboard
@ stdcall ImmDestroyContext(long) ImmDestroyContext
@ stdcall ImmDestroyIMCC(long) ImmDestroyIMCC
@ stdcall ImmDestroySoftKeyboard(long) ImmDestroySoftKeyboard
@ stub ImmDisableIME
@ stub ImmEnumInputContext
@ stdcall ImmEnumRegisterWordA(long ptr str long str ptr) ImmEnumRegisterWordA
@ stdcall ImmEnumRegisterWordW(long ptr wstr long wstr ptr) ImmEnumRegisterWordW
@ stdcall ImmEscapeA(long long long ptr) ImmEscapeA
@ stdcall ImmEscapeW(long long long ptr) ImmEscapeW
@ stdcall ImmGenerateMessage(long) ImmGenerateMessage
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
@ stdcall ImmGetHotKey(long ptr ptr ptr) ImmGetHotKey
@ stdcall ImmGetIMCCLockCount(long) ImmGetIMCCLockCount
@ stdcall ImmGetIMCCSize(long) ImmGetIMCCSize
@ stdcall ImmGetIMCLockCount(long) ImmGetIMCLockCount
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
@ stdcall ImmLockIMC(long) ImmLockIMC
@ stdcall ImmLockIMCC(long) ImmLockIMCC
@ stdcall ImmNotifyIME(long long long long) ImmNotifyIME
@ stdcall ImmReSizeIMCC(long long) ImmReSizeIMCC
@ stdcall ImmRegisterWordA(long str long str) ImmRegisterWordA
@ stdcall ImmRegisterWordW(long wstr long wstr) ImmRegisterWordW
@ stdcall ImmReleaseContext(long long) ImmReleaseContext
@ stub ImmRequestMessageA
@ stub ImmRequestMessageW
@ stdcall ImmSetCandidateWindow(long ptr) ImmSetCandidateWindow
@ stdcall ImmSetCompositionFontA(long ptr) ImmSetCompositionFontA
@ stdcall ImmSetCompositionFontW(long ptr) ImmSetCompositionFontW
@ stdcall ImmSetCompositionStringA(long long ptr long ptr long) ImmSetCompositionStringA
@ stdcall ImmSetCompositionStringW(long long ptr long ptr long) ImmSetCompositionStringW
@ stdcall ImmSetCompositionWindow(long ptr) ImmSetCompositionWindow
@ stdcall ImmSetConversionStatus(long long long) ImmSetConversionStatus
@ stdcall ImmSetHotKey(long long long long) ImmSetHotKey
@ stdcall ImmSetOpenStatus(long long) ImmSetOpenStatus
@ stdcall ImmSetStatusWindowPos(long ptr) ImmSetStatusWindowPos
@ stdcall ImmShowSoftKeyboard(long long) ImmShowSoftKeyboard
@ stdcall ImmSimulateHotKey(long long) ImmSimulateHotKey
@ stdcall ImmUnlockIMC(long) ImmUnlockIMC
@ stdcall ImmUnlockIMCC(long) ImmUnlockIMCC
@ stdcall ImmUnregisterWordA(long str long str) ImmUnregisterWordA
@ stdcall ImmUnregisterWordW(long wstr long wstr) ImmUnregisterWordW
@ stub SKWndProcT1
