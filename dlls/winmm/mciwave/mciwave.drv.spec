name mciwave
file mciwave.drv
type win32

debug_channels (mciwave)

@ stdcall DriverProc(long long long long long) MCIWAVE_DriverProc
