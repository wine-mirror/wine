name midimap
file midimap.drv
type win32

import winmm.dll

  1 stdcall DriverProc(long long long long long) MIDIMAP_DriverProc
  2 stdcall midMessage(long long long long long) MIDIMAP_midMessage
  3 stdcall modMessage(long long long long long) MIDIMAP_modMessage
