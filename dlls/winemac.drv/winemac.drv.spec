# GDI driver

@ cdecl wine_get_gdi_driver(long) macdrv_get_gdi_driver

# USER driver

@ cdecl ActivateKeyboardLayout(long long) macdrv_ActivateKeyboardLayout
@ cdecl Beep() macdrv_Beep
@ cdecl ChangeDisplaySettingsEx(ptr ptr long long long) macdrv_ChangeDisplaySettingsEx
@ cdecl ClipCursor(ptr) macdrv_ClipCursor
@ cdecl CreateDesktopWindow(long) macdrv_CreateDesktopWindow
@ cdecl CreateWindow(long) macdrv_CreateWindow
@ cdecl DestroyCursorIcon(long) macdrv_DestroyCursorIcon
@ cdecl DestroyWindow(long) macdrv_DestroyWindow
@ cdecl EnumDisplaySettingsEx(ptr long ptr long) macdrv_EnumDisplaySettingsEx
@ cdecl GetCursorPos(ptr) macdrv_GetCursorPos
@ cdecl GetKeyboardLayout(long) macdrv_GetKeyboardLayout
@ cdecl GetKeyboardLayoutList(long ptr) macdrv_GetKeyboardLayoutList
@ cdecl GetKeyboardLayoutName(ptr) macdrv_GetKeyboardLayoutName
@ cdecl GetKeyNameText(long ptr long) macdrv_GetKeyNameText
@ cdecl MapVirtualKeyEx(long long long) macdrv_MapVirtualKeyEx
@ cdecl MsgWaitForMultipleObjectsEx(long ptr long long long) macdrv_MsgWaitForMultipleObjectsEx
@ cdecl RegisterHotKey(long long long) macdrv_RegisterHotKey
@ cdecl SetCapture(long long) macdrv_SetCapture
@ cdecl SetCursor(long) macdrv_SetCursor
@ cdecl SetCursorPos(long long) macdrv_SetCursorPos
@ cdecl SetFocus(long) macdrv_SetFocus
@ cdecl SetLayeredWindowAttributes(long long long long) macdrv_SetLayeredWindowAttributes
@ cdecl SetParent(long long long) macdrv_SetParent
@ cdecl SetWindowRgn(long long long) macdrv_SetWindowRgn
@ cdecl SetWindowStyle(ptr long ptr) macdrv_SetWindowStyle
@ cdecl SetWindowText(long wstr) macdrv_SetWindowText
@ cdecl ShowWindow(long long ptr long) macdrv_ShowWindow
@ cdecl SysCommand(long long long) macdrv_SysCommand
@ cdecl SystemParametersInfo(long long ptr long) macdrv_SystemParametersInfo
@ cdecl ThreadDetach() macdrv_ThreadDetach
@ cdecl ToUnicodeEx(long long ptr ptr long long long) macdrv_ToUnicodeEx
@ cdecl UnregisterHotKey(long long long) macdrv_UnregisterHotKey
@ cdecl UpdateClipboard() macdrv_UpdateClipboard
@ cdecl UpdateLayeredWindow(long ptr ptr) macdrv_UpdateLayeredWindow
@ cdecl VkKeyScanEx(long long) macdrv_VkKeyScanEx
@ cdecl WindowMessage(long long long long) macdrv_WindowMessage
@ cdecl WindowPosChanged(long long long ptr ptr ptr ptr ptr) macdrv_WindowPosChanged
@ cdecl WindowPosChanging(long long long ptr ptr ptr ptr) macdrv_WindowPosChanging

# System tray
@ cdecl wine_notify_icon(long ptr)

# IME
@ stdcall ImeConfigure(long long long ptr)
@ stdcall ImeConversionList(long wstr ptr long long)
@ stdcall ImeDestroy(long)
@ stdcall ImeEnumRegisterWord(ptr wstr long wstr ptr)
@ stdcall ImeEscape(long long ptr)
@ stdcall ImeGetImeMenuItems(long long long ptr ptr long)
@ stdcall ImeGetRegisterWordStyle(long ptr)
@ stdcall ImeInquire(ptr wstr wstr)
@ stdcall ImeProcessKey(long long long ptr)
@ stdcall ImeRegisterWord(wstr long wstr)
@ stdcall ImeSelect(long long)
@ stdcall ImeSetActiveContext(long long)
@ stdcall ImeSetCompositionString(long long ptr long ptr long)
@ stdcall ImeToAsciiEx(long long ptr ptr long long)
@ stdcall ImeUnregisterWord(wstr long wstr)
@ stdcall NotifyIME(long long long long)
