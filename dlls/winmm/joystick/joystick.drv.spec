name joystick
file joystick.drv
type win32

debug_channels (joystick)

@ stdcall DriverProc(long long long long long) JSTCK_DriverProc
