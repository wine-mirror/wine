@ stdcall LoadDosExe(str long) MZ_LoadImage
@ stdcall EmulateInterruptPM(ptr long) DOSVM_EmulateInterruptPM

# DPMI functions
@ stdcall CallRMInt(ptr) DOSVM_CallRMInt
@ stdcall CallRMProc(ptr long) DOSVM_CallRMProc
@ stdcall AllocRMCB(ptr) DOSVM_AllocRMCB
@ stdcall FreeRMCB(ptr) DOSVM_FreeRMCB
@ stdcall RawModeSwitch(ptr) DOSVM_RawModeSwitch

# I/O functions
@ stdcall SetTimer(long) DOSVM_SetTimer
@ stdcall GetTimer() DOSVM_GetTimer
@ stdcall inport(long long ptr) DOSVM_inport
@ stdcall outport(long long long) DOSVM_outport
@ stdcall ASPIHandler(ptr) DOSVM_ASPIHandler
