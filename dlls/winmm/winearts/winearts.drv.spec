name winearts
file winearts.drv
type win32

debug_channels (wave)

@ stdcall DriverProc(long long long long long) ARTS_DriverProc
@ stdcall wodMessage(long long long long long) ARTS_wodMessage
