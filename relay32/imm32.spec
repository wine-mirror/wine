name	imm32
type	win32

  1 stdcall ImmAssociateContext(long long) ImmAssociateContext32
  2 stdcall ImmConfigureIMEA(long long long ptr) ImmConfigureIME32A
  3 stdcall ImmConfigureIMEW(long long long ptr) ImmConfigureIME32W
  4 stdcall ImmCreateContext() ImmCreateContext32
  5 stub ImmCreateIMCC
  6 stub ImmCreateSoftKeyboard
  7 stdcall ImmDestroyContext(long) ImmDestroyContext32
  8 stub ImmDestroyIMCC
  9 stub ImmDestroySoftKeyboard
 10 stdcall ImmEnumRegisterWordA(long ptr str long str ptr) ImmEnumRegisterWord32A
 11 stdcall ImmEnumRegisterWordW(long ptr wstr long wstr ptr) ImmEnumRegisterWord32W
 12 stdcall ImmEscapeA(long long long ptr) ImmEscape32A
 13 stdcall ImmEscapeW(long long long ptr) ImmEscape32W
 14 stub ImmGenerateMessage
 15 stdcall ImmGetCandidateListA(long long ptr long) ImmGetCandidateList32A
 16 stdcall ImmGetCandidateListCountA(long ptr) ImmGetCandidateListCount32A
 17 stdcall ImmGetCandidateListCountW(long ptr) ImmGetCandidateListCount32W
 18 stdcall ImmGetCandidateListW(long long ptr long) ImmGetCandidateList32W
 19 stdcall ImmGetCandidateWindow(long long ptr) ImmGetCandidateWindow32
 20 stdcall ImmGetCompositionFontA(long ptr) ImmGetCompositionFont32A
 21 stdcall ImmGetCompositionFontW(long ptr) ImmGetCompositionFont32W
 22 stdcall ImmGetCompositionStringA (long long ptr long) ImmGetCompositionString32A
 23 stdcall ImmGetCompositionStringW (long long ptr long) ImmGetCompositionString32W
 24 stdcall ImmGetCompositionWindow(long ptr) ImmGetCompositionWindow32
 25 stdcall ImmGetContext(long) ImmGetContext32
 26 stdcall ImmGetConversionListA(long long str ptr long long) ImmGetConversionList32A
 27 stdcall ImmGetConversionListW(long long wstr ptr long long) ImmGetConversionList32W
 28 stdcall ImmGetConversionStatus(long ptr ptr) ImmGetConversionStatus32
 29 stdcall ImmGetDefaultIMEWnd(long) ImmGetDefaultIMEWnd32
 30 stdcall ImmGetDescriptionA(long str long) ImmGetDescription32A
 31 stdcall ImmGetDescriptionW(long wstr long) ImmGetDescription32W
 32 stdcall ImmGetGuideLineA(long long str long) ImmGetGuideLine32A
 33 stdcall ImmGetGuideLineW(long long wstr long) ImmGetGuideLine32W
 34 stub ImmGetHotKey
 35 stub ImmGetIMCCLockCount
 36 stub ImmGetIMCCSize
 37 stub ImmGetIMCLockCount
 38 stdcall ImmGetIMEFileNameA(long str long) ImmGetIMEFileName32A
 39 stdcall ImmGetIMEFileNameW(long wstr long) ImmGetIMEFileName32W
 40 stdcall ImmGetOpenStatus(long) ImmGetOpenStatus32
 41 stdcall ImmGetProperty(long long) ImmGetProperty32
 42 stdcall ImmGetRegisterWordStyleA(long long ptr) ImmGetRegisterWordStyle32A
 43 stdcall ImmGetRegisterWordStyleW(long long ptr) ImmGetRegisterWordStyle32W
 44 stdcall ImmGetStatusWindowPos(long ptr) ImmGetStatusWindowPos32
 45 stdcall ImmGetVirtualKey(long) ImmGetVirtualKey32
 46 stdcall ImmInstallIMEA(str str) ImmInstallIME32A
 47 stdcall ImmInstallIMEW(wstr wstr) ImmInstallIME32W
 48 stdcall ImmIsIME(long) ImmIsIME32
 49 stdcall ImmIsUIMessageA(long long long long) ImmIsUIMessage32A
 50 stdcall ImmIsUIMessageW(long long long long) ImmIsUIMessage32W
 51 stub ImmLockIMC
 52 stub ImmLockIMCC
 53 stdcall ImmNotifyIME(long long long long) ImmNotifyIME32
 54 stub ImmReSizeIMCC
 55 stdcall ImmRegisterWordA(long str long str) ImmRegisterWord32A
 56 stdcall ImmRegisterWordW(long wstr long wstr) ImmRegisterWord32W
 57 stdcall ImmReleaseContext(long long) ImmReleaseContext32
 58 stdcall ImmSetCandidateWindow(long ptr) ImmSetCandidateWindow32
 59 stdcall ImmSetCompositionFontA(long ptr) ImmSetCompositionFont32A
 60 stdcall ImmSetCompositionFontW(long ptr) ImmSetCompositionFont32W
 61 stdcall ImmSetCompositionStringA(long long ptr long ptr long) ImmSetCompositionString32A
 62 stdcall ImmSetCompositionStringW(long long ptr long ptr long) ImmSetCompositionString32W
 63 stdcall ImmSetCompositionWindow(long ptr) ImmSetCompositionWindow32
 64 stdcall ImmSetConversionStatus(long long long) ImmSetConversionStatus32
 65 stub ImmSetHotKey
 66 stdcall ImmSetOpenStatus(long long) ImmSetOpenStatus32
 67 stdcall ImmSetStatusWindowPos(long ptr) ImmSetStatusWindowPos32
 68 stub ImmShowSoftKeyboard
 69 stdcall ImmSimulateHotKey(long long) ImmSimulateHotKey32
 70 stub ImmUnlockIMC
 71 stub ImmUnlockIMCC
 72 stdcall ImmUnregisterWordA(long str long str) ImmUnregisterWord32A
