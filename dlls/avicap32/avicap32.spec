name avicap32
type win32

import ntdll.dll

debug_channels (avicap32)

@ stdcall capCreateCaptureWindowA(str long long long long long long long) capCreateCaptureWindowA
@ stdcall capCreateCaptureWindowW(wstr long long long long long long long) capCreateCaptureWindowW
@ stdcall capGetDriverDescriptionA(long ptr long ptr long) capGetDriverDescriptionA
@ stdcall capGetDriverDescriptionW(long ptr long ptr long) capGetDriverDescriptionW
