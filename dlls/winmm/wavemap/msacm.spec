name msacmmap
file msacm.drv
type win32

  1 stdcall DriverProc(long long long long long) WAVEMAP_DriverProc
  2 stdcall widMessage(long long long long long) WAVEMAP_widMessage
  3 stdcall wodMessage(long long long long long) WAVEMAP_wodMessage
