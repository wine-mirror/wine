name	imm32
type	win32

  1 stdcall ImmAssociateContext(long long) ImmAssociateContext
  2 stdcall ImmConfigureIMEA(long long long ptr) ImmConfigureIMEA
  3 stdcall ImmConfigureIMEW(long long long ptr) ImmConfigureIMEW
  4 stdcall ImmCreateContext() ImmCreateContext
  5 stub ImmCreateIMCC
  6 stub ImmCreateSoftKeyboard
  7 stdcall ImmDestroyContext(long) ImmDestroyContext
  8 stub ImmDestroyIMCC
  9 stub ImmDestroySoftKeyboard
 10 stdcall ImmEnumRegisterWordA(long ptr str long str ptr) ImmEnumRegisterWordA
 11 stdcall ImmEnumRegisterWordW(long ptr wstr long wstr ptr) ImmEnumRegisterWordW
 12 stdcall ImmEscapeA(long long long ptr) ImmEscapeA
 13 stdcall ImmEscapeW(long long long ptr) ImmEscapeW
 14 stub ImmGenerateMessage
 15 stdcall ImmGetCandidateListA(long long ptr long) ImmGetCandidateListA
 16 stdcall ImmGetCandidateListCountA(long ptr) ImmGetCandidateListCountA
 17 stdcall ImmGetCandidateListCountW(long ptr) ImmGetCandidateListCountW
 18 stdcall ImmGetCandidateListW(long long ptr long) ImmGetCandidateListW
 19 stdcall ImmGetCandidateWindow(long long ptr) ImmGetCandidateWindow
 20 stdcall ImmGetCompositionFontA(long ptr) ImmGetCompositionFontA
 21 stdcall ImmGetCompositionFontW(long ptr) ImmGetCompositionFontW
 22 stdcall ImmGetCompositionStringA (long long ptr long) ImmGetCompositionStringA
 23 stdcall ImmGetCompositionStringW (long long ptr long) ImmGetCompositionStringW
 24 stdcall ImmGetCompositionWindow(long ptr) ImmGetCompositionWindow
 25 stdcall ImmGetContext(long) ImmGetContext
 26 stdcall ImmGetConversionListA(long long str ptr long long) ImmGetConversionListA
 27 stdcall ImmGetConversionListW(long long wstr ptr long long) ImmGetConversionListW
 28 stdcall ImmGetConversionStatus(long ptr ptr) ImmGetConversionStatus
 29 stdcall ImmGetDefaultIMEWnd(long) ImmGetDefaultIMEWnd
 30 stdcall ImmGetDescriptionA(long str long) ImmGetDescriptionA
 31 stdcall ImmGetDescriptionW(long wstr long) ImmGetDescriptionW
 32 stdcall ImmGetGuideLineA(long long str long) ImmGetGuideLineA
 33 stdcall ImmGetGuideLineW(long long wstr long) ImmGetGuideLineW
 34 stub ImmGetHotKey
 35 stub ImmGetIMCCLockCount
 36 stub ImmGetIMCCSize
 37 stub ImmGetIMCLockCount
 38 stdcall ImmGetIMEFileNameA(long str long) ImmGetIMEFileNameA
 39 stdcall ImmGetIMEFileNameW(long wstr long) ImmGetIMEFileNameW
 40 stdcall ImmGetOpenStatus(long) ImmGetOpenStatus
 41 stdcall ImmGetProperty(long long) ImmGetProperty
 42 stdcall ImmGetRegisterWordStyleA(long long ptr) ImmGetRegisterWordStyleA
 43 stdcall ImmGetRegisterWordStyleW(long long ptr) ImmGetRegisterWordStyleW
 44 stdcall ImmGetStatusWindowPos(long ptr) ImmGetStatusWindowPos
 45 stdcall ImmGetVirtualKey(long) ImmGetVirtualKey
 46 stdcall ImmInstallIMEA(str str) ImmInstallIMEA
 47 stdcall ImmInstallIMEW(wstr wstr) ImmInstallIMEW
 48 stdcall ImmIsIME(long) ImmIsIME
 49 stdcall ImmIsUIMessageA(long long long long) ImmIsUIMessageA
 50 stdcall ImmIsUIMessageW(long long long long) ImmIsUIMessageW
 51 stub ImmLockIMC
 52 stub ImmLockIMCC
 53 stdcall ImmNotifyIME(long long long long) ImmNotifyIME
 54 stub ImmReSizeIMCC
 55 stdcall ImmRegisterWordA(long str long str) ImmRegisterWordA
 56 stdcall ImmRegisterWordW(long wstr long wstr) ImmRegisterWordW
 57 stdcall ImmReleaseContext(long long) ImmReleaseContext
 58 stdcall ImmSetCandidateWindow(long ptr) ImmSetCandidateWindow
 59 stdcall ImmSetCompositionFontA(long ptr) ImmSetCompositionFontA
 60 stdcall ImmSetCompositionFontW(long ptr) ImmSetCompositionFontW
 61 stdcall ImmSetCompositionStringA(long long ptr long ptr long) ImmSetCompositionStringA
 62 stdcall ImmSetCompositionStringW(long long ptr long ptr long) ImmSetCompositionStringW
 63 stdcall ImmSetCompositionWindow(long ptr) ImmSetCompositionWindow
 64 stdcall ImmSetConversionStatus(long long long) ImmSetConversionStatus
 65 stub ImmSetHotKey
 66 stdcall ImmSetOpenStatus(long long) ImmSetOpenStatus
 67 stdcall ImmSetStatusWindowPos(long ptr) ImmSetStatusWindowPos
 68 stub ImmShowSoftKeyboard
 69 stdcall ImmSimulateHotKey(long long) ImmSimulateHotKey
 70 stub ImmUnlockIMC
 71 stub ImmUnlockIMCC
 72 stdcall ImmUnregisterWordA(long str long str) ImmUnregisterWordA
