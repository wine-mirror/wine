name msg711
file msg711.drv
type win32
init MSG711_DllMain

import winmm.dll
import user32.dll
import kernel32.dll
import ntdll.dll

debug_channels (msg711)

@ stdcall DriverProc(long long long long long) MSG711_DriverProc
