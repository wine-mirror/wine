name mcianim
file mcianim.drv
type win32

import winmm.dll
import user32.dll
import kernel32.dll
import	ntdll.dll

debug_channels (mcianim)

@ stdcall DriverProc(long long long long long) MCIANIM_DriverProc
