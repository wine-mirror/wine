name winearts
file winearts.drv
type win32

import winmm.dll
import user32.dll
import kernel32.dll
import ntdll.dll

debug_channels (wave)

@ stdcall DriverProc(long long long long long) ARTS_DriverProc
@ stdcall wodMessage(long long long long long) ARTS_wodMessage
