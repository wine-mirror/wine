name mciavi
file mciavi.drv
type win32
init MCIAVI_LibMain
rsrc mciavi_res.res

import msvfw32.dll
import winmm.dll
import user32.dll
import gdi32.dll
import kernel32.dll
import ntdll.dll

debug_channels (mciavi)

@ stdcall DriverProc(long long long long long) MCIAVI_DriverProc
