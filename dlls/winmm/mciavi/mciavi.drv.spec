name mciavi
file mciavi.drv
init MCIAVI_LibMain

@ stdcall DriverProc(long long long long long) MCIAVI_DriverProc
