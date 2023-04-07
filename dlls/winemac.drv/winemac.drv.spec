# System tray
@ cdecl wine_notify_icon(long ptr)

# IME
@ stdcall ImeProcessKey(long long long ptr)
@ stdcall ImeSelect(long long)
@ stdcall ImeSetCompositionString(long long ptr long ptr long)
@ stdcall ImeToAsciiEx(long long ptr ptr long long)
@ stdcall NotifyIME(long long long long)
