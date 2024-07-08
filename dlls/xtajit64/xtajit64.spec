@ stdcall BTCpu64FlushInstructionCache(ptr long)
@ stdcall BTCpu64IsProcessorFeaturePresent(long)
@ stdcall BTCpu64NotifyMemoryDirty(ptr long)
@ stdcall BTCpu64NotifyReadFile(ptr ptr long long long)
@ extern DispatchJump
@ extern RetToEntryThunk
@ extern ExitToX64
@ extern BeginSimulation
@ stdcall FlushInstructionCacheHeavy(ptr long)
@ stdcall NotifyMapViewOfSection(ptr ptr ptr long long long)
@ stdcall NotifyMemoryAlloc(ptr long long long long long)
@ stdcall NotifyMemoryFree(ptr long long long long)
@ stdcall NotifyMemoryProtect(ptr long long long long)
@ stdcall NotifyUnmapViewOfSection(ptr long long)
@ stdcall ProcessInit()
#@ stub ProcessTerm
#@ stub ResetToConsistentState
@ stdcall ThreadInit()
#@ stub ThreadTerm
@ stdcall UpdateProcessorInformation(ptr)
