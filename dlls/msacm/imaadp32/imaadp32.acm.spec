name imaadp32
file imaadp32.acm
type win32
init IMAADP32_DllMain

import winmm.dll
import user32.dll
import kernel32.dll
import ntdll.dll

debug_channels (imaadp32)

@ stdcall DriverProc(long long long long long) IMAADP32_DriverProc
