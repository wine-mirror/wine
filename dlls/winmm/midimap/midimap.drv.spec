name midimap
file midimap.drv
type win32

import winmm.dll
import user32.dll
import kernel32.dll

@ stdcall DriverProc(long long long long long) MIDIMAP_DriverProc
@ stdcall midMessage(long long long long long) MIDIMAP_midMessage
@ stdcall modMessage(long long long long long) MIDIMAP_modMessage
