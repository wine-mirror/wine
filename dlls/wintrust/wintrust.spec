name	wintrust
type	win32

import ntdll.dll

debug_channels (win32)

@ stdcall WinVerifyTrust(long ptr ptr) WinVerifyTrust
