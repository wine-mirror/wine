# GDI driver

@ cdecl wine_get_gdi_driver(long) macdrv_get_gdi_driver

# USER driver

@ cdecl CreateDesktopWindow(long) macdrv_CreateDesktopWindow
@ cdecl CreateWindow(long) macdrv_CreateWindow
@ cdecl DestroyWindow(long) macdrv_DestroyWindow
@ cdecl EnumDisplayMonitors(long ptr ptr long) macdrv_EnumDisplayMonitors
@ cdecl GetMonitorInfo(long ptr) macdrv_GetMonitorInfo
@ cdecl SetParent(long long long) macdrv_SetParent
@ cdecl SetWindowStyle(ptr long ptr) macdrv_SetWindowStyle
@ cdecl SetWindowText(long wstr) macdrv_SetWindowText
@ cdecl WindowPosChanged(long long long ptr ptr ptr ptr ptr) macdrv_WindowPosChanged
@ cdecl WindowPosChanging(long long long ptr ptr ptr ptr) macdrv_WindowPosChanging
