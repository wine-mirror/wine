@ stdcall CallbackMayRunLong(ptr) kernel32.CallbackMayRunLong
@ stdcall CancelThreadpoolIo(ptr) kernel32.CancelThreadpoolIo
@ stdcall ChangeTimerQueueTimer(ptr ptr long long) kernel32.ChangeTimerQueueTimer
@ stdcall CloseThreadpool(ptr) kernel32.CloseThreadpool
@ stdcall CloseThreadpoolCleanupGroup(ptr) kernel32.CloseThreadpoolCleanupGroup
@ stdcall CloseThreadpoolCleanupGroupMembers(ptr long ptr) kernel32.CloseThreadpoolCleanupGroupMembers
@ stdcall CloseThreadpoolIo(ptr) kernel32.CloseThreadpoolIo
@ stdcall CloseThreadpoolTimer(ptr) kernel32.CloseThreadpoolTimer
@ stdcall CloseThreadpoolWait(ptr) kernel32.CloseThreadpoolWait
@ stdcall CloseThreadpoolWork(ptr) kernel32.CloseThreadpoolWork
@ stdcall CreateThreadpool(ptr) kernel32.CreateThreadpool
@ stdcall CreateThreadpoolCleanupGroup() kernel32.CreateThreadpoolCleanupGroup
@ stdcall CreateThreadpoolIo(ptr ptr ptr ptr) kernel32.CreateThreadpoolIo
@ stdcall CreateThreadpoolTimer(ptr ptr ptr) kernel32.CreateThreadpoolTimer
@ stdcall CreateThreadpoolWait(ptr ptr ptr) kernel32.CreateThreadpoolWait
@ stdcall CreateThreadpoolWork(ptr ptr ptr) kernel32.CreateThreadpoolWork
@ stdcall CreateTimerQueue() kernel32.CreateTimerQueue
@ stdcall CreateTimerQueueTimer(ptr long ptr ptr long long long) kernel32.CreateTimerQueueTimer
@ stdcall DeleteTimerQueueEx(long long) kernel32.DeleteTimerQueueEx
@ stdcall DeleteTimerQueueTimer(long long long) kernel32.DeleteTimerQueueTimer
@ stdcall DisassociateCurrentThreadFromCallback(ptr) kernel32.DisassociateCurrentThreadFromCallback
@ stdcall FreeLibraryWhenCallbackReturns(ptr ptr) kernel32.FreeLibraryWhenCallbackReturns
@ stdcall IsThreadpoolTimerSet(ptr) kernel32.IsThreadpoolTimerSet
@ stdcall LeaveCriticalSectionWhenCallbackReturns(ptr ptr) kernel32.LeaveCriticalSectionWhenCallbackReturns
@ stdcall QueryThreadpoolStackInformation(ptr ptr) kernel32.QueryThreadpoolStackInformation
@ stdcall RegisterWaitForSingleObjectEx(long ptr ptr long long) kernel32.RegisterWaitForSingleObjectEx
@ stdcall ReleaseMutexWhenCallbackReturns(ptr long) kernel32.ReleaseMutexWhenCallbackReturns
@ stdcall ReleaseSemaphoreWhenCallbackReturns(ptr long long) kernel32.ReleaseSemaphoreWhenCallbackReturns
@ stdcall SetEventWhenCallbackReturns(ptr long) kernel32.SetEventWhenCallbackReturns
@ stdcall SetThreadpoolStackInformation(ptr ptr) kernel32.SetThreadpoolStackInformation
@ stdcall SetThreadpoolThreadMaximum(ptr long) kernel32.SetThreadpoolThreadMaximum
@ stdcall SetThreadpoolThreadMinimum(ptr long) kernel32.SetThreadpoolThreadMinimum
@ stdcall SetThreadpoolTimer(ptr ptr long long) kernel32.SetThreadpoolTimer
@ stdcall SetThreadpoolWait(ptr long ptr) kernel32.SetThreadpoolWait
@ stdcall StartThreadpoolIo(ptr) kernel32.StartThreadpoolIo
@ stdcall SubmitThreadpoolWork(ptr) kernel32.SubmitThreadpoolWork
@ stdcall TrySubmitThreadpoolCallback(ptr ptr ptr) kernel32.TrySubmitThreadpoolCallback
@ stdcall UnregisterWaitEx(long long) kernel32.UnregisterWaitEx
@ stdcall WaitForThreadpoolIoCallbacks(ptr) kernel32.WaitForThreadpoolIoCallbacks
@ stdcall WaitForThreadpoolTimerCallbacks(ptr long) kernel32.WaitForThreadpoolTimerCallbacks
@ stdcall WaitForThreadpoolWaitCallbacks(ptr long) kernel32.WaitForThreadpoolWaitCallbacks
@ stdcall WaitForThreadpoolWorkCallbacks(ptr long) kernel32.WaitForThreadpoolWorkCallbacks
