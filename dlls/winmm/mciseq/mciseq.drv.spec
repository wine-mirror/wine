name mciseq
file mciseq.drv
type win32

import winmm.dll
import -delay user32.dll
import kernel32.dll
import ntdll.dll

debug_channels (mcimidi)

@ stdcall DriverProc(long long long long long) MCIMIDI_DriverProc
