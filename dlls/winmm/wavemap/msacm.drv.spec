name msacmmap
file msacm.drv
type win32

import winmm.dll
import user32.dll
import kernel32.dll
import	ntdll.dll

debug_channels (msacm)

@ stdcall DriverProc(long long long long long) WAVEMAP_DriverProc
@ stdcall widMessage(long long long long long) WAVEMAP_widMessage
@ stdcall wodMessage(long long long long long) WAVEMAP_wodMessage
