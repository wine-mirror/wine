name msrle32
type win32
init MSRLE32_DllMain

import winmm.dll
import user32.dll
import kernel32.dll
import ntdll.dll

debug_channels(msrle32)

@ stdcall DriverProc(long long long long long) MSRLE32_DriverProc

