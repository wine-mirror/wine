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
@ stdcall ProcessTerm(ptr long long)
@ stdcall ResetToConsistentState(ptr ptr ptr)
@ stdcall ThreadInit()
@ stdcall ThreadTerm(ptr long)
@ stdcall UpdateProcessorInformation(ptr)
