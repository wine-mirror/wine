name	twain_32
type	win32
init	TWAIN_LibMain

import  user32.dll
import  gdi32.dll
import  kernel32.dll
import  ntdll.dll

debug_channels (twain)

@ stdcall DSM_Entry(ptr ptr long long long ptr) DSM_Entry
