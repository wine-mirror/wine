name mciavi
file mciavi.drv
type win32
init MCIAVI_LibMain

debug_channels (mciavi)

@ stdcall DriverProc(long long long long long) MCIAVI_DriverProc
