name joystick
file joystick.drv
type win32

import winmm.dll
import user32.dll
import ntdll.dll

debug_channels (joystick)

@ stdcall DriverProc(long long long long long) JSTCK_DriverProc
