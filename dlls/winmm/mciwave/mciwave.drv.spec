name mciwave
file mciwave.drv
type win32

import winmm.dll

  1 stdcall DriverProc(long long long long long) MCIWAVE_DriverProc
