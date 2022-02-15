@ stdcall CallbackMayRunLong(ptr) kernelbase.CallbackMayRunLong
@ stdcall CancelThreadpoolIo(ptr) kernelbase.CancelThreadpoolIo
@ stdcall ChangeTimerQueueTimer(ptr ptr long long) kernelbase.ChangeTimerQueueTimer
@ stdcall CloseThreadpool(ptr) kernelbase.CloseThreadpool
@ stdcall CloseThreadpoolCleanupGroup(ptr) kernelbase.CloseThreadpoolCleanupGroup
@ stdcall CloseThreadpoolCleanupGroupMembers(ptr long ptr) kernelbase.CloseThreadpoolCleanupGroupMembers
@ stdcall CloseThreadpoolIo(ptr) kernelbase.CloseThreadpoolIo
@ stdcall CloseThreadpoolTimer(ptr) kernelbase.CloseThreadpoolTimer
@ stdcall CloseThreadpoolWait(ptr) kernelbase.CloseThreadpoolWait
@ stdcall CloseThreadpoolWork(ptr) kernelbase.CloseThreadpoolWork
@ stdcall CreateThreadpool(ptr) kernelbase.CreateThreadpool
@ stdcall CreateThreadpoolCleanupGroup() kernelbase.CreateThreadpoolCleanupGroup
@ stdcall CreateThreadpoolIo(ptr ptr ptr ptr) kernelbase.CreateThreadpoolIo
@ stdcall CreateThreadpoolTimer(ptr ptr ptr) kernelbase.CreateThreadpoolTimer
@ stdcall CreateThreadpoolWait(ptr ptr ptr) kernelbase.CreateThreadpoolWait
@ stdcall CreateThreadpoolWork(ptr ptr ptr) kernelbase.CreateThreadpoolWork
@ stdcall CreateTimerQueue() kernelbase.CreateTimerQueue
@ stdcall CreateTimerQueueTimer(ptr long ptr ptr long long long) kernelbase.CreateTimerQueueTimer
@ stdcall DeleteTimerQueueEx(long long) kernelbase.DeleteTimerQueueEx
@ stdcall DeleteTimerQueueTimer(long long long) kernelbase.DeleteTimerQueueTimer
@ stdcall DisassociateCurrentThreadFromCallback(ptr) kernelbase.DisassociateCurrentThreadFromCallback
@ stdcall FreeLibraryWhenCallbackReturns(ptr ptr) kernelbase.FreeLibraryWhenCallbackReturns
@ stdcall IsThreadpoolTimerSet(ptr) kernelbase.IsThreadpoolTimerSet
@ stdcall LeaveCriticalSectionWhenCallbackReturns(ptr ptr) kernelbase.LeaveCriticalSectionWhenCallbackReturns
@ stdcall QueryThreadpoolStackInformation(ptr ptr) kernelbase.QueryThreadpoolStackInformation
@ stdcall RegisterWaitForSingleObjectEx(long ptr ptr long long) kernelbase.RegisterWaitForSingleObjectEx
@ stdcall ReleaseMutexWhenCallbackReturns(ptr long) kernelbase.ReleaseMutexWhenCallbackReturns
@ stdcall ReleaseSemaphoreWhenCallbackReturns(ptr long long) kernelbase.ReleaseSemaphoreWhenCallbackReturns
@ stdcall SetEventWhenCallbackReturns(ptr long) kernelbase.SetEventWhenCallbackReturns
@ stdcall SetThreadpoolStackInformation(ptr ptr) kernelbase.SetThreadpoolStackInformation
@ stdcall SetThreadpoolThreadMaximum(ptr long) kernelbase.SetThreadpoolThreadMaximum
@ stdcall SetThreadpoolThreadMinimum(ptr long) kernelbase.SetThreadpoolThreadMinimum
@ stdcall SetThreadpoolTimer(ptr ptr long long) kernelbase.SetThreadpoolTimer
@ stdcall SetThreadpoolWait(ptr long ptr) kernelbase.SetThreadpoolWait
@ stdcall StartThreadpoolIo(ptr) kernelbase.StartThreadpoolIo
@ stdcall SubmitThreadpoolWork(ptr) kernelbase.SubmitThreadpoolWork
@ stdcall TrySubmitThreadpoolCallback(ptr ptr ptr) kernelbase.TrySubmitThreadpoolCallback
@ stdcall UnregisterWaitEx(long long) kernelbase.UnregisterWaitEx
@ stdcall WaitForThreadpoolIoCallbacks(ptr) kernelbase.WaitForThreadpoolIoCallbacks
@ stdcall WaitForThreadpoolTimerCallbacks(ptr long) kernelbase.WaitForThreadpoolTimerCallbacks
@ stdcall WaitForThreadpoolWaitCallbacks(ptr long) kernelbase.WaitForThreadpoolWaitCallbacks
@ stdcall WaitForThreadpoolWorkCallbacks(ptr long) kernelbase.WaitForThreadpoolWorkCallbacks
