@ stub CtfImmAppCompatEnableIMEonProtectedCode
@ stub CtfImmCoUninitialize
@ stub CtfImmDispatchDefImeMessage
@ stub CtfImmEnterCoInitCountSkipMode
@ stub CtfImmGenerateMessage
@ stub CtfImmGetCompatibleKeyboardLayout
@ stub CtfImmGetGuidAtom
@ stub CtfImmGetIMEFileName
@ stub CtfImmGetTMAEFlags
@ stdcall CtfImmHideToolbarWnd()
@ stdcall CtfImmIsCiceroEnabled()
@ stub CtfImmIsCiceroStartedInThread
@ stub CtfImmIsGuidMapEnable
@ stub CtfImmIsTextFrameServiceDisabled
@ stub CtfImmLastEnabledWndDestroy
@ stub CtfImmLeaveCoInitCountSkipMode
@ stub CtfImmNotify
@ stdcall CtfImmRestoreToolbarWnd(long)
@ stub CtfImmSetAppCompatFlags
@ stub CtfImmSetCiceroStartInThread
@ stub CtfImmSetDefaultRemoteKeyboardLayout
@ stub CtfImmTIMActivate
@ stub GetKeyboardLayoutCP
@ stdcall ImmActivateLayout(long)
@ stdcall ImmAssociateContext(long long)
@ stdcall ImmAssociateContextEx(long long long)
@ stdcall ImmConfigureIMEA(long long long ptr)
@ stdcall ImmConfigureIMEW(long long long ptr)
@ stdcall ImmCreateContext()
@ stdcall ImmCreateIMCC(long)
@ stdcall ImmCreateSoftKeyboard(long long long long)
@ stdcall ImmDestroyContext(long)
@ stdcall ImmDestroyIMCC(long)
@ stdcall ImmDestroySoftKeyboard(long)
@ stdcall ImmDisableIME(long) NtUserDisableThreadIme
@ stdcall ImmDisableIme(long) NtUserDisableThreadIme
@ stdcall ImmDisableLegacyIME()
@ stdcall ImmDisableTextFrameService(long)
@ stdcall ImmEnumInputContext(long ptr long)
@ stdcall ImmEnumRegisterWordA(long ptr str long str ptr)
@ stdcall ImmEnumRegisterWordW(long ptr wstr long wstr ptr)
@ stdcall ImmEscapeA(long long long ptr)
@ stdcall ImmEscapeW(long long long ptr)
@ stdcall ImmFreeLayout(long)
@ stdcall ImmGenerateMessage(ptr)
@ stdcall ImmGetCandidateListA(long long ptr long)
@ stdcall ImmGetCandidateListCountA(long ptr)
@ stdcall ImmGetCandidateListCountW(long ptr)
@ stdcall ImmGetCandidateListW(long long ptr long)
@ stdcall ImmGetCandidateWindow(long long ptr)
@ stdcall ImmGetCompositionFontA(long ptr)
@ stdcall ImmGetCompositionFontW(long ptr)
@ stdcall ImmGetCompositionString (long long ptr long) ImmGetCompositionStringA
@ stdcall ImmGetCompositionStringA (long long ptr long)
@ stdcall ImmGetCompositionStringW (long long ptr long)
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
@ stdcall ImmGetHotKey(long ptr ptr ptr)
@ stdcall ImmGetIMCCLockCount(long)
@ stdcall ImmGetIMCCSize(long)
@ stdcall ImmGetIMCLockCount(long)
@ stdcall ImmGetIMEFileNameA(long ptr long)
@ stdcall ImmGetIMEFileNameW(long ptr long)
@ stub ImmGetImeInfoEx
@ stdcall ImmGetImeMenuItemsA(long long long ptr ptr long)
@ stdcall ImmGetImeMenuItemsW(long long long ptr ptr long)
@ stdcall ImmGetOpenStatus(long)
@ stdcall ImmGetProperty(long long)
@ stdcall ImmGetRegisterWordStyleA(long long ptr)
@ stdcall ImmGetRegisterWordStyleW(long long ptr)
@ stdcall ImmGetStatusWindowPos(long ptr)
@ stdcall ImmGetVirtualKey(long)
@ stub ImmIMPGetIMEA
@ stub ImmIMPGetIMEW
@ stub ImmIMPQueryIMEA
@ stub ImmIMPQueryIMEW
@ stub ImmIMPSetIMEA
@ stub ImmIMPSetIMEW
@ stdcall ImmInstallIMEA(str str)
@ stdcall ImmInstallIMEW(wstr wstr)
@ stdcall ImmIsIME(long)
@ stdcall ImmIsUIMessageA(long long long long)
@ stdcall ImmIsUIMessageW(long long long long)
@ stdcall ImmLoadIME(long)
@ stub ImmLoadLayout
@ stub ImmLockClientImc
@ stdcall ImmLockIMC(long)
@ stdcall ImmLockIMCC(long)
@ stub ImmLockImeDpi
@ stdcall ImmNotifyIME(long long long long)
@ stub ImmPenAuxInput
@ stdcall ImmProcessKey(long long long long long)
@ stub ImmPutImeMenuItemsIntoMappedFile
@ stdcall ImmReSizeIMCC(long long)
@ stub ImmRegisterClient
@ stdcall ImmRegisterWordA(long str long str)
@ stdcall ImmRegisterWordW(long wstr long wstr)
@ stdcall ImmReleaseContext(long long)
@ stdcall ImmRequestMessageA(ptr long long)
@ stdcall ImmRequestMessageW(ptr long long)
@ stub ImmSendIMEMessageExA
@ stub ImmSendIMEMessageExW
@ stub ImmSendMessageToActiveDefImeWndW
@ stdcall ImmSetActiveContext(long long long)
@ stub ImmSetActiveContextConsoleIME
@ stdcall ImmSetCandidateWindow(long ptr)
@ stdcall ImmSetCompositionFontA(long ptr)
@ stdcall ImmSetCompositionFontW(long ptr)
@ stdcall ImmSetCompositionStringA(long long ptr long ptr long)
@ stdcall ImmSetCompositionStringW(long long ptr long ptr long)
@ stdcall ImmSetCompositionWindow(long ptr)
@ stdcall ImmSetConversionStatus(long long long)
#@ stdcall ImmSetHotKey(long long long ptr) user32.CliImmSetHotKey
@ stdcall ImmSetOpenStatus(long long)
@ stdcall ImmSetStatusWindowPos(long ptr)
@ stdcall ImmShowSoftKeyboard(long long)
@ stdcall ImmSimulateHotKey(long long)
@ stub ImmSystemHandler
@ stdcall ImmTranslateMessage(long long long long)
@ stub ImmUnlockClientImc
@ stdcall ImmUnlockIMC(long)
@ stdcall ImmUnlockIMCC(long)
@ stub ImmUnlockImeDpi
@ stdcall ImmUnregisterWordA(long str long str)
@ stdcall ImmUnregisterWordW(long wstr long wstr)
@ stub ImmWINNLSEnableIME
@ stub ImmWINNLSGetEnableStatus
@ stub ImmWINNLSGetIMEHotkey

################################################################
# Wine internal extensions
@ stdcall __wine_ime_wnd_proc(long long long long long)
