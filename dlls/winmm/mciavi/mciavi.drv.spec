name mciavi
file mciavi.drv
type win32
init MCIAVI_LibMain
rsrc mciavi_res.res

debug_channels (mciavi)

@ stdcall DriverProc(long long long long long) MCIAVI_DriverProc
