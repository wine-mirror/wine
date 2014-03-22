@ stdcall AcquireSRWLockExclusive(ptr) kernel32.AcquireSRWLockExclusive
@ stdcall AcquireSRWLockShared(ptr) kernel32.AcquireSRWLockShared
@ stdcall CancelWaitableTimer(long) kernel32.CancelWaitableTimer
@ stdcall CreateEventA(ptr long long str) kernel32.CreateEventA
@ stdcall CreateEventExA(ptr str long long) kernel32.CreateEventExA
@ stdcall CreateEventExW(ptr wstr long long) kernel32.CreateEventExW
@ stdcall CreateEventW(ptr long long wstr) kernel32.CreateEventW
@ stdcall CreateMutexA(ptr long str) kernel32.CreateMutexA
@ stdcall CreateMutexExA(ptr str long long) kernel32.CreateMutexExA
@ stdcall CreateMutexExW(ptr wstr long long) kernel32.CreateMutexExW
@ stdcall CreateMutexW(ptr long wstr) kernel32.CreateMutexW
@ stdcall CreateSemaphoreExW(ptr long long wstr long long) kernel32.CreateSemaphoreExW
@ stdcall CreateWaitableTimerExW(ptr wstr long long) kernel32.CreateWaitableTimerExW
@ stdcall DeleteCriticalSection(ptr) kernel32.DeleteCriticalSection
@ stub DeleteSynchronizationBarrier
@ stdcall EnterCriticalSection(ptr) kernel32.EnterCriticalSection
@ stub EnterSynchronizationBarrier
@ stdcall InitOnceBeginInitialize(ptr long ptr ptr) kernel32.InitOnceBeginInitialize
@ stdcall InitOnceComplete(ptr long ptr) kernel32.InitOnceComplete
@ stdcall InitOnceExecuteOnce(ptr ptr ptr ptr) kernel32.InitOnceExecuteOnce
@ stdcall InitOnceInitialize(ptr) kernel32.InitOnceInitialize
@ stdcall InitializeConditionVariable(ptr) kernel32.InitializeConditionVariable
@ stdcall InitializeCriticalSection(ptr) kernel32.InitializeCriticalSection
@ stdcall InitializeCriticalSectionAndSpinCount(ptr long) kernel32.InitializeCriticalSectionAndSpinCount
@ stdcall InitializeCriticalSectionEx(ptr long long) kernel32.InitializeCriticalSectionEx
@ stdcall InitializeSRWLock(ptr) kernel32.InitializeSRWLock
@ stub InitializeSynchronizationBarrier
@ stdcall LeaveCriticalSection(ptr) kernel32.LeaveCriticalSection
@ stdcall OpenEventA(long long str) kernel32.OpenEventA
@ stdcall OpenEventW(long long wstr) kernel32.OpenEventW
@ stdcall OpenMutexW(long long wstr) kernel32.OpenMutexW
@ stdcall OpenSemaphoreW(long long wstr) kernel32.OpenSemaphoreW
@ stdcall OpenWaitableTimerW(long long wstr) kernel32.OpenWaitableTimerW
@ stdcall ReleaseMutex(long) kernel32.ReleaseMutex
@ stdcall ReleaseSRWLockExclusive(ptr) kernel32.ReleaseSRWLockExclusive
@ stdcall ReleaseSRWLockShared(ptr) kernel32.ReleaseSRWLockShared
@ stdcall ReleaseSemaphore(long long ptr) kernel32.ReleaseSemaphore
@ stdcall ResetEvent(long) kernel32.ResetEvent
@ stdcall SetCriticalSectionSpinCount(ptr long) kernel32.SetCriticalSectionSpinCount
@ stdcall SetEvent(long) kernel32.SetEvent
@ stdcall SetWaitableTimer(long ptr long ptr ptr long) kernel32.SetWaitableTimer
@ stdcall SetWaitableTimerEx(long ptr long ptr ptr ptr long) kernel32.SetWaitableTimerEx
@ stdcall SignalObjectAndWait(long long long long) kernel32.SignalObjectAndWait
@ stdcall Sleep(long) kernel32.Sleep
@ stdcall SleepConditionVariableCS(ptr ptr long) kernel32.SleepConditionVariableCS
@ stdcall SleepConditionVariableSRW(ptr ptr long long) kernel32.SleepConditionVariableSRW
@ stdcall SleepEx(long long) kernel32.SleepEx
@ stdcall TryAcquireSRWLockExclusive(ptr) kernel32.TryAcquireSRWLockExclusive
@ stdcall TryAcquireSRWLockShared(ptr) kernel32.TryAcquireSRWLockShared
@ stdcall TryEnterCriticalSection(ptr) kernel32.TryEnterCriticalSection
@ stdcall WaitForMultipleObjectsEx(long ptr long long long) kernel32.WaitForMultipleObjectsEx
@ stdcall WaitForSingleObject(long long) kernel32.WaitForSingleObject
@ stdcall WaitForSingleObjectEx(long long long) kernel32.WaitForSingleObjectEx
@ stub WaitOnAddress
@ stdcall WakeAllConditionVariable(ptr) kernel32.WakeAllConditionVariable
@ stub WakeByAddressAll
@ stub WakeByAddressSingle
@ stdcall WakeConditionVariable(ptr) kernel32.WakeConditionVariable
