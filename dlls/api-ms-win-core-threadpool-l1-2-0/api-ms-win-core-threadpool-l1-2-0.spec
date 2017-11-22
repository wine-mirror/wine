@ stdcall CallbackMayRunLong(ptr) kernel32.CallbackMayRunLong
@ stub CancelThreadpoolIo
@ stdcall CloseThreadpool(ptr) kernel32.CloseThreadpool
@ stdcall CloseThreadpoolCleanupGroup(ptr) kernel32.CloseThreadpoolCleanupGroup
@ stdcall CloseThreadpoolCleanupGroupMembers(ptr long ptr) kernel32.CloseThreadpoolCleanupGroupMembers
@ stub CloseThreadpoolIo
@ stdcall CloseThreadpoolTimer(ptr) kernel32.CloseThreadpoolTimer
@ stdcall CloseThreadpoolWait(ptr) kernel32.CloseThreadpoolWait
@ stdcall CloseThreadpoolWork(ptr) kernel32.CloseThreadpoolWork
@ stdcall CreateThreadpool(ptr) kernel32.CreateThreadpool
@ stdcall CreateThreadpoolCleanupGroup() kernel32.CreateThreadpoolCleanupGroup
@ stdcall CreateThreadpoolIo(ptr ptr ptr ptr) kernel32.CreateThreadpoolIo
@ stdcall CreateThreadpoolTimer(ptr ptr ptr) kernel32.CreateThreadpoolTimer
@ stdcall CreateThreadpoolWait(ptr ptr ptr) kernel32.CreateThreadpoolWait
@ stdcall CreateThreadpoolWork(ptr ptr ptr) kernel32.CreateThreadpoolWork
@ stdcall DisassociateCurrentThreadFromCallback(ptr) kernel32.DisassociateCurrentThreadFromCallback
@ stdcall FreeLibraryWhenCallbackReturns(ptr ptr) kernel32.FreeLibraryWhenCallbackReturns
@ stdcall IsThreadpoolTimerSet(ptr) kernel32.IsThreadpoolTimerSet
@ stdcall LeaveCriticalSectionWhenCallbackReturns(ptr ptr) kernel32.LeaveCriticalSectionWhenCallbackReturns
@ stub QueryThreadpoolStackInformation
@ stdcall ReleaseMutexWhenCallbackReturns(ptr long) kernel32.ReleaseMutexWhenCallbackReturns
@ stdcall ReleaseSemaphoreWhenCallbackReturns(ptr long long) kernel32.ReleaseSemaphoreWhenCallbackReturns
@ stdcall SetEventWhenCallbackReturns(ptr long) kernel32.SetEventWhenCallbackReturns
@ stub SetThreadpoolStackInformation
@ stdcall SetThreadpoolThreadMaximum(ptr long) kernel32.SetThreadpoolThreadMaximum
@ stdcall SetThreadpoolThreadMinimum(ptr long) kernel32.SetThreadpoolThreadMinimum
@ stdcall SetThreadpoolTimer(ptr ptr long long) kernel32.SetThreadpoolTimer
@ stub SetThreadpoolTimerEx
@ stdcall SetThreadpoolWait(ptr long ptr) kernel32.SetThreadpoolWait
@ stub SetThreadpoolWaitEx
@ stub StartThreadpoolIo
@ stdcall SubmitThreadpoolWork(ptr) kernel32.SubmitThreadpoolWork
@ stdcall TrySubmitThreadpoolCallback(ptr ptr ptr) kernel32.TrySubmitThreadpoolCallback
@ stub WaitForThreadpoolIoCallbacks
@ stdcall WaitForThreadpoolTimerCallbacks(ptr long) kernel32.WaitForThreadpoolTimerCallbacks
@ stdcall WaitForThreadpoolWaitCallbacks(ptr long) kernel32.WaitForThreadpoolWaitCallbacks
@ stdcall WaitForThreadpoolWorkCallbacks(ptr long) kernel32.WaitForThreadpoolWorkCallbacks
