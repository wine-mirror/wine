@ stdcall wine_load_dos_exe(str str)

@ stdcall EmulateInterruptPM(ptr long) DOSVM_EmulateInterruptPM
@ stdcall CallBuiltinHandler(ptr long) DOSVM_CallBuiltinHandler

# I/O functions
@ stdcall inport(long long) DOSVM_inport
@ stdcall outport(long long long) DOSVM_outport
