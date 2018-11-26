# GDI driver

@ cdecl wine_get_gdi_driver(long) ANDROID_get_gdi_driver

# USER driver

@ cdecl GetKeyNameText(long ptr long) ANDROID_GetKeyNameText
@ cdecl GetKeyboardLayout(long) ANDROID_GetKeyboardLayout
@ cdecl MapVirtualKeyEx(long long long) ANDROID_MapVirtualKeyEx
@ cdecl ToUnicodeEx(long long ptr ptr long long long) ANDROID_ToUnicodeEx
@ cdecl VkKeyScanEx(long long) ANDROID_VkKeyScanEx
@ cdecl SetCursor(long) ANDROID_SetCursor
@ cdecl ChangeDisplaySettingsEx(ptr ptr long long long) ANDROID_ChangeDisplaySettingsEx
@ cdecl EnumDisplayMonitors(long ptr ptr long) ANDROID_EnumDisplayMonitors
@ cdecl EnumDisplaySettingsEx(ptr long ptr long) ANDROID_EnumDisplaySettingsEx
@ cdecl GetMonitorInfo(long ptr) ANDROID_GetMonitorInfo
@ cdecl CreateWindow(long) ANDROID_CreateWindow
@ cdecl DestroyWindow(long) ANDROID_DestroyWindow
@ cdecl MsgWaitForMultipleObjectsEx(long ptr long long long) ANDROID_MsgWaitForMultipleObjectsEx
@ cdecl SetCapture(long long) ANDROID_SetCapture
@ cdecl SetLayeredWindowAttributes(long long long long) ANDROID_SetLayeredWindowAttributes
@ cdecl SetParent(long long long) ANDROID_SetParent
@ cdecl SetWindowRgn(long long long) ANDROID_SetWindowRgn
@ cdecl SetWindowStyle(ptr long ptr) ANDROID_SetWindowStyle
@ cdecl ShowWindow(long long ptr long) ANDROID_ShowWindow
@ cdecl UpdateLayeredWindow(long ptr ptr) ANDROID_UpdateLayeredWindow
@ cdecl WindowMessage(long long long long) ANDROID_WindowMessage
@ cdecl WindowPosChanging(long long long ptr ptr ptr ptr) ANDROID_WindowPosChanging
@ cdecl WindowPosChanged(long long long ptr ptr ptr ptr ptr) ANDROID_WindowPosChanged

# Desktop
@ cdecl wine_create_desktop(long long) ANDROID_create_desktop

# MMDevAPI driver functions
@ stdcall -private GetPriority() AUDDRV_GetPriority
@ stdcall -private GetEndpointIDs(long ptr ptr ptr ptr) AUDDRV_GetEndpointIDs
@ stdcall -private GetAudioEndpoint(ptr ptr ptr) AUDDRV_GetAudioEndpoint
@ stdcall -private GetAudioSessionManager(ptr ptr) AUDDRV_GetAudioSessionManager
