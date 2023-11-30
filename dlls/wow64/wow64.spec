@ stub Wow64AllocThreadHeap
@ stub Wow64AllocateHeap
@ stdcall Wow64AllocateTemp(long)
@ stdcall Wow64ApcRoutine(long long long ptr)
@ stub Wow64CheckIfNXEnabled
@ stub Wow64EmulateAtlThunk
@ stub Wow64FreeHeap
@ stub Wow64FreeThreadHeap
@ stub Wow64GetWow64ImageOption
@ stub Wow64IsControlFlowGuardEnforced
@ stub Wow64IsStackExtentsCheckEnforced
@ stdcall Wow64KiUserCallbackDispatcher(long ptr long ptr ptr)
@ stdcall Wow64LdrpInitialize(ptr)
@ stub Wow64LogPrint
@ stub Wow64NotifyUnsimulateComplete
@ stdcall Wow64PassExceptionToGuest(ptr)
@ stub Wow64PrepareForDebuggerAttach
@ stdcall -norelay Wow64PrepareForException(ptr ptr)
@ stdcall Wow64ProcessPendingCrossProcessItems()
@ stdcall Wow64RaiseException(long ptr)
@ stub Wow64ShallowThunkAllocObjectAttributes32TO64_FNC
@ stub Wow64ShallowThunkAllocSecurityQualityOfService32TO64_FNC
@ stub Wow64ShallowThunkSIZE_T32TO64
@ stub Wow64ShallowThunkSIZE_T64TO32
@ stub Wow64SuspendLocalThread
@ stdcall -norelay Wow64SystemServiceEx(long ptr)
@ stub Wow64ValidateUserCallTarget
@ stub Wow64ValidateUserCallTargetFilter
