# GDI driver

@ cdecl wine_get_gdi_driver(long) X11DRV_get_gdi_driver

# USER driver

@ cdecl ActivateKeyboardLayout(long long) X11DRV_ActivateKeyboardLayout
@ cdecl Beep() X11DRV_Beep
@ cdecl GetKeyNameText(long ptr long) X11DRV_GetKeyNameText
@ cdecl GetKeyboardLayout(long) X11DRV_GetKeyboardLayout
@ cdecl GetKeyboardLayoutName(ptr) X11DRV_GetKeyboardLayoutName
@ cdecl LoadKeyboardLayout(wstr long) X11DRV_LoadKeyboardLayout
@ cdecl MapVirtualKeyEx(long long long) X11DRV_MapVirtualKeyEx
@ cdecl ToUnicodeEx(long long ptr ptr long long long) X11DRV_ToUnicodeEx
@ cdecl UnloadKeyboardLayout(long) X11DRV_UnloadKeyboardLayout
@ cdecl VkKeyScanEx(long long) X11DRV_VkKeyScanEx
@ cdecl DestroyCursorIcon(long) X11DRV_DestroyCursorIcon
@ cdecl SetCursor(long) X11DRV_SetCursor
@ cdecl GetCursorPos(ptr) X11DRV_GetCursorPos
@ cdecl SetCursorPos(long long) X11DRV_SetCursorPos
@ cdecl ClipCursor(ptr) X11DRV_ClipCursor
@ cdecl ChangeDisplaySettingsEx(ptr ptr long long long) X11DRV_ChangeDisplaySettingsEx
@ cdecl EnumDisplaySettingsEx(ptr long ptr long) X11DRV_EnumDisplaySettingsEx
@ cdecl CreateDesktopWindow(long) X11DRV_CreateDesktopWindow
@ cdecl CreateWindow(long) X11DRV_CreateWindow
@ cdecl DestroyWindow(long) X11DRV_DestroyWindow
@ cdecl FlashWindowEx(ptr) X11DRV_FlashWindowEx
@ cdecl GetDC(long long long ptr ptr long) X11DRV_GetDC
@ cdecl MsgWaitForMultipleObjectsEx(long ptr long long long) X11DRV_MsgWaitForMultipleObjectsEx
@ cdecl ReleaseDC(long long) X11DRV_ReleaseDC
@ cdecl ScrollDC(long long long long) X11DRV_ScrollDC
@ cdecl SetCapture(long long) X11DRV_SetCapture
@ cdecl SetFocus(long) X11DRV_SetFocus
@ cdecl SetLayeredWindowAttributes(long long long long) X11DRV_SetLayeredWindowAttributes
@ cdecl SetParent(long long long) X11DRV_SetParent
@ cdecl SetWindowIcon(long long long) X11DRV_SetWindowIcon
@ cdecl SetWindowRgn(long long long) X11DRV_SetWindowRgn
@ cdecl SetWindowStyle(ptr long ptr) X11DRV_SetWindowStyle
@ cdecl SetWindowText(long wstr) X11DRV_SetWindowText
@ cdecl ShowWindow(long long ptr long) X11DRV_ShowWindow
@ cdecl SysCommand(long long long) X11DRV_SysCommand
@ cdecl UpdateClipboard() X11DRV_UpdateClipboard
@ cdecl UpdateLayeredWindow(long ptr ptr) X11DRV_UpdateLayeredWindow
@ cdecl WindowMessage(long long long long) X11DRV_WindowMessage
@ cdecl WindowPosChanging(long long long ptr ptr ptr ptr) X11DRV_WindowPosChanging
@ cdecl WindowPosChanged(long long long ptr ptr ptr ptr ptr) X11DRV_WindowPosChanged
@ cdecl SystemParametersInfo(long long ptr long) X11DRV_SystemParametersInfo
@ cdecl ThreadDetach() X11DRV_ThreadDetach

# WinTab32
@ cdecl AttachEventQueueToTablet(long) X11DRV_AttachEventQueueToTablet
@ cdecl GetCurrentPacket(ptr) X11DRV_GetCurrentPacket
@ cdecl LoadTabletInfo(long) X11DRV_LoadTabletInfo
@ cdecl WTInfoW(long long ptr) X11DRV_WTInfoW

# Desktop
@ cdecl wine_create_desktop(long long) X11DRV_create_desktop

# System tray
@ cdecl wine_notify_icon(long ptr)

#IME Interface
@ stdcall ImeInquire(ptr ptr wstr)
@ stdcall ImeConfigure(long long long ptr)
@ stdcall ImeDestroy(long)
@ stdcall ImeEscape(long long ptr)
@ stdcall ImeSelect(long long)
@ stdcall ImeSetActiveContext(long long)
@ stdcall ImeToAsciiEx(long long ptr ptr long long)
@ stdcall NotifyIME(long long long long)
@ stdcall ImeRegisterWord(wstr long wstr)
@ stdcall ImeUnregisterWord(wstr long wstr)
@ stdcall ImeEnumRegisterWord(ptr wstr long wstr ptr)
@ stdcall ImeSetCompositionString(long long ptr long ptr long)
@ stdcall ImeConversionList(long wstr ptr long long)
@ stdcall ImeProcessKey(long long long ptr)
@ stdcall ImeGetRegisterWordStyle(long ptr)
@ stdcall ImeGetImeMenuItems(long long long ptr ptr long)
