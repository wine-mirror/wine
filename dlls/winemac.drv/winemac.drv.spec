# GDI driver

@ cdecl wine_get_gdi_driver(long) macdrv_get_gdi_driver

# USER driver

@ cdecl ActivateKeyboardLayout(long long) macdrv_ActivateKeyboardLayout
@ cdecl Beep() macdrv_Beep
@ cdecl CreateDesktopWindow(long) macdrv_CreateDesktopWindow
@ cdecl CreateWindow(long) macdrv_CreateWindow
@ cdecl DestroyWindow(long) macdrv_DestroyWindow
@ cdecl EnumDisplayMonitors(long ptr ptr long) macdrv_EnumDisplayMonitors
@ cdecl GetKeyboardLayout(long) macdrv_GetKeyboardLayout
@ cdecl GetKeyboardLayoutName(ptr) macdrv_GetKeyboardLayoutName
@ cdecl GetKeyNameText(long ptr long) macdrv_GetKeyNameText
@ cdecl GetMonitorInfo(long ptr) macdrv_GetMonitorInfo
@ cdecl MapVirtualKeyEx(long long long) macdrv_MapVirtualKeyEx
@ cdecl MsgWaitForMultipleObjectsEx(long ptr long long long) macdrv_MsgWaitForMultipleObjectsEx
@ cdecl ScrollDC(long long long ptr ptr long ptr) macdrv_ScrollDC
@ cdecl SetFocus(long) macdrv_SetFocus
@ cdecl SetLayeredWindowAttributes(long long long long) macdrv_SetLayeredWindowAttributes
@ cdecl SetParent(long long long) macdrv_SetParent
@ cdecl SetWindowRgn(long long long) macdrv_SetWindowRgn
@ cdecl SetWindowStyle(ptr long ptr) macdrv_SetWindowStyle
@ cdecl SetWindowText(long wstr) macdrv_SetWindowText
@ cdecl ShowWindow(long long ptr long) macdrv_ShowWindow
@ cdecl SysCommand(long long long) macdrv_SysCommand
@ cdecl ToUnicodeEx(long long ptr ptr long long long) macdrv_ToUnicodeEx
@ cdecl UpdateLayeredWindow(long ptr ptr) macdrv_UpdateLayeredWindow
@ cdecl VkKeyScanEx(long long) macdrv_VkKeyScanEx
@ cdecl WindowMessage(long long long long) macdrv_WindowMessage
@ cdecl WindowPosChanged(long long long ptr ptr ptr ptr ptr) macdrv_WindowPosChanged
@ cdecl WindowPosChanging(long long long ptr ptr ptr ptr) macdrv_WindowPosChanging
