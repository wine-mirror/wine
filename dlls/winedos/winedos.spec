name winedos
type win32
init DOSVM_Init

import user32.dll
import kernel32.dll
import ntdll.dll

debug_channels (aspi console ddraw int int21 int31 module relay)

@ stdcall LoadDosExe(str long) MZ_LoadImage

# DPMI functions
@ stdcall CallRMInt(ptr) DOSVM_CallRMInt
@ stdcall CallRMProc(ptr long) DOSVM_CallRMProc
@ stdcall AllocRMCB(ptr) DOSVM_AllocRMCB
@ stdcall FreeRMCB(ptr) DOSVM_FreeRMCB

# I/O functions
@ stdcall SetTimer(long) DOSVM_SetTimer
@ stdcall GetTimer() DOSVM_GetTimer
@ stdcall inport(long long ptr) DOSVM_inport
@ stdcall outport(long long long) DOSVM_outport
@ stdcall ASPIHandler(ptr) DOSVM_ASPIHandler
