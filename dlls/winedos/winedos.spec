name winedos
type win32

import user32.dll
import kernel32.dll
import ntdll.dll

@ stdcall GetCurrent() MZ_Current
@ stdcall LoadDPMI() MZ_AllocDPMITask
@ stdcall LoadDosExe(str) MZ_LoadImage
@ stdcall Exec(ptr str long ptr) MZ_Exec
@ stdcall Exit(ptr long long) MZ_Exit

@ stdcall Enter(ptr) DOSVM_Enter
@ stdcall Wait(long long) DOSVM_Wait
@ stdcall QueueEvent(long long ptr ptr) DOSVM_QueueEvent
@ stdcall OutPIC(long long) DOSVM_PIC_ioport_out
@ stdcall SetTimer(long) DOSVM_SetTimer
@ stdcall GetTimer() DOSVM_GetTimer
