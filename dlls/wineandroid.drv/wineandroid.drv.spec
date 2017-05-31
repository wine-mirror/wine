# GDI driver

@ cdecl wine_get_gdi_driver(long) ANDROID_get_gdi_driver

# USER driver

@ cdecl EnumDisplayMonitors(long ptr ptr long) ANDROID_EnumDisplayMonitors
@ cdecl GetMonitorInfo(long ptr) ANDROID_GetMonitorInfo
@ cdecl DestroyWindow(long) ANDROID_DestroyWindow
@ cdecl WindowPosChanging(long long long ptr ptr ptr ptr) ANDROID_WindowPosChanging
@ cdecl WindowPosChanged(long long long ptr ptr ptr ptr ptr) ANDROID_WindowPosChanged
