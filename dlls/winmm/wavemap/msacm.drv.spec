name msacmmap
file msacm.drv
type win32

debug_channels (msacm)

@ stdcall DriverProc(long long long long long) WAVEMAP_DriverProc
@ stdcall widMessage(long long long long long) WAVEMAP_widMessage
@ stdcall wodMessage(long long long long long) WAVEMAP_wodMessage
