name msdmo
type win32

import ole32.dll
import user32.dll
import advapi32.dll
import kernel32.dll
import ntdll.dll

debug_channels (msdmo)

@ stub DMOEnum
@ stub DMOGetName
@ stub DMOGetTypes
@ stub DMOGuidToStrA
@ stub DMOGuidToStrW
@ stub DMORegister
@ stub DMOStrToGuidA
@ stub DMOStrToGuidW
@ stub DMOUnregister
@ stub MoCopyMediaType
@ stub MoCreateMediaType
@ stub MoDeleteMediaType
@ stub MoDuplicateMediaType
@ stub MoFreeMediaType
@ stub MoInitMediaType

