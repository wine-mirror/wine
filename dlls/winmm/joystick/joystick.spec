name joystick
file joystick.drv
type win32

import winmm.dll

  1 stdcall DriverProc(long long long long long) JSTCK_DriverProc
