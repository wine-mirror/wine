name msimg32
type win32

import ntdll.dll

debug_channels (msimg32)

@ stdcall AlphaBlend(long long long long long long long long long long long) AlphaBlend
@ stub DllInitialize
@ stdcall GradientFill(long ptr long ptr long long) GradientFill
@ stdcall TransparentBlt(long long long long long long long long long long long) TransparentBlt
@ stdcall vSetDdrawflag() vSetDdrawflag
