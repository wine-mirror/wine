name mciavi
file mciavi.drv
type win32

import winmm.dll

  1 stdcall DriverProc(long long long long long) MCIAVI_DriverProc
