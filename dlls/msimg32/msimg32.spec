name msimg32
type win32

import ntdll.dll

debug_channels (msimg32)

@ stub AlphaBlend
@ stub DllInitialize
@ stdcall GradientFill (long ptr long ptr long long ) GradientFill
@ stub TransparentBlt
@ stub vSetDdrawflag
