@ stdcall wine_load_dos_exe(str str)

@ stdcall EmulateInterruptPM(ptr long) DOSVM_EmulateInterruptPM
@ stdcall CallBuiltinHandler(ptr long) DOSVM_CallBuiltinHandler

# I/O functions
@ stdcall inport(long long) DOSVM_inport
@ stdcall outport(long long long) DOSVM_outport

# DOS memory functions
@ cdecl FreeDosBlock(ptr) DOSMEM_FreeBlock
@ cdecl AllocDosBlock(long ptr) DOSMEM_AllocBlock
@ cdecl ResizeDosBlock(ptr long long) DOSMEM_ResizeBlock
