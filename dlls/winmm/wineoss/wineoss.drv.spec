name wineoss
file wineoss.drv
type win32

import winmm.dll
import user32.dll
import kernel32.dll
import	ntdll.dll

debug_channels (midi mmaux wave)

  1 stdcall DriverProc(long long long long long) OSS_DriverProc
  2 stdcall auxMessage(long long long long long) OSS_auxMessage
  3 stdcall mixMessage(long long long long long) OSS_mixMessage
  4 stdcall midMessage(long long long long long) OSS_midMessage
  5 stdcall modMessage(long long long long long) OSS_modMessage
  6 stdcall widMessage(long long long long long) OSS_widMessage
  7 stdcall wodMessage(long long long long long) OSS_wodMessage
