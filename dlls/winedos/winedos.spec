@ stdcall LoadDosExe(str long) MZ_LoadImage
@ stdcall EmulateInterruptPM(ptr long) DOSVM_EmulateInterruptPM
@ stdcall CallBuiltinHandler(ptr long) DOSVM_CallBuiltinHandler

# I/O functions
@ stdcall SetTimer(long) DOSVM_SetTimer
@ stdcall GetTimer() DOSVM_GetTimer
@ stdcall inport(long long ptr) DOSVM_inport
@ stdcall outport(long long long) DOSVM_outport
