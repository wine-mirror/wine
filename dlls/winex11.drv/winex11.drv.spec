# WinTab32
@ cdecl AttachEventQueueToTablet(long) X11DRV_AttachEventQueueToTablet
@ cdecl GetCurrentPacket(ptr) X11DRV_GetCurrentPacket
@ cdecl LoadTabletInfo(long) X11DRV_LoadTabletInfo
@ cdecl WTInfoW(long long ptr) X11DRV_WTInfoW

# Desktop
@ cdecl wine_create_desktop(long long)

# System tray
@ cdecl wine_notify_icon(long ptr)

#IME Interface
@ stdcall ImeSelect(long long)
@ stdcall ImeToAsciiEx(long long ptr ptr long long)
@ stdcall NotifyIME(long long long long)
@ stdcall ImeSetCompositionString(long long ptr long ptr long)
@ stdcall ImeProcessKey(long long long ptr)
