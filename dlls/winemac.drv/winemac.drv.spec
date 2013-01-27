# GDI driver

@ cdecl wine_get_gdi_driver(long) macdrv_get_gdi_driver

# USER driver

@ cdecl CreateDesktopWindow(long) macdrv_CreateDesktopWindow
@ cdecl CreateWindow(long) macdrv_CreateWindow
@ cdecl DestroyWindow(long) macdrv_DestroyWindow
@ cdecl EnumDisplayMonitors(long ptr ptr long) macdrv_EnumDisplayMonitors
@ cdecl GetMonitorInfo(long ptr) macdrv_GetMonitorInfo
@ cdecl MsgWaitForMultipleObjectsEx(long ptr long long long) macdrv_MsgWaitForMultipleObjectsEx
@ cdecl SetFocus(long) macdrv_SetFocus
@ cdecl SetLayeredWindowAttributes(long long long long) macdrv_SetLayeredWindowAttributes
@ cdecl SetParent(long long long) macdrv_SetParent
@ cdecl SetWindowRgn(long long long) macdrv_SetWindowRgn
@ cdecl SetWindowStyle(ptr long ptr) macdrv_SetWindowStyle
@ cdecl SetWindowText(long wstr) macdrv_SetWindowText
@ cdecl ShowWindow(long long ptr long) macdrv_ShowWindow
@ cdecl UpdateLayeredWindow(long ptr ptr) macdrv_UpdateLayeredWindow
@ cdecl WindowMessage(long long long long) macdrv_WindowMessage
@ cdecl WindowPosChanged(long long long ptr ptr ptr ptr ptr) macdrv_WindowPosChanged
@ cdecl WindowPosChanging(long long long ptr ptr ptr ptr) macdrv_WindowPosChanging
