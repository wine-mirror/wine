name midimap
file midimap.drv
type win32

import winmm.dll
import user32.dll
import advapi32.dll
import kernel32.dll
import	ntdll.dll

debug_channels (msacm)

@ stdcall DriverProc(long long long long long) MIDIMAP_DriverProc
@ stdcall modMessage(long long long long long) MIDIMAP_modMessage
