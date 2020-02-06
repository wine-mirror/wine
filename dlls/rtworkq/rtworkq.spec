@ stdcall RtwqAddPeriodicCallback(ptr ptr ptr)
@ stub RtwqAllocateSerialWorkQueue
@ stdcall RtwqAllocateWorkQueue(long ptr)
@ stub RtwqBeginRegisterWorkQueueWithMMCSS
@ stub RtwqBeginUnregisterWorkQueueWithMMCSS
@ stdcall RtwqCancelDeadline(long)
@ stub RtwqCancelMultipleWaitingWorkItem
@ stdcall RtwqCancelWorkItem(int64)
@ stdcall RtwqCreateAsyncResult(ptr ptr ptr ptr)
@ stub RtwqEndRegisterWorkQueueWithMMCSS
@ stub RtwqEndUnregisterWorkQueueWithMMCSS
@ stub RtwqGetPlatform
@ stub RtwqGetWorkQueueMMCSSClass
@ stub RtwqGetWorkQueueMMCSSPriority
@ stub RtwqGetWorkQueueMMCSSTaskId
@ stdcall RtwqInvokeCallback(ptr)
@ stub RtwqJoinWorkQueue
@ stdcall RtwqLockPlatform()
@ stdcall RtwqLockSharedWorkQueue(wstr long ptr ptr)
@ stdcall RtwqLockWorkQueue(long)
@ stub RtwqPutMultipleWaitingWorkItem
@ stdcall RtwqPutWaitingWorkItem(long long ptr ptr)
@ stdcall RtwqPutWorkItem(long long ptr)
@ stub RtwqRegisterPlatformEvents
@ stub RtwqRegisterPlatformWithMMCSS
@ stdcall RtwqRemovePeriodicCallback(long)
@ stdcall RtwqScheduleWorkItem(ptr int64 ptr)
@ stdcall RtwqSetDeadline(long int64 ptr)
@ stdcall RtwqSetDeadline2(long int64 int64 ptr)
@ stdcall RtwqSetLongRunning(long long)
@ stdcall RtwqShutdown()
@ stdcall RtwqStartup()
@ stub RtwqUnjoinWorkQueue
@ stdcall RtwqUnlockPlatform()
@ stdcall RtwqUnlockWorkQueue(long)
@ stub RtwqUnregisterPlatformEvents
@ stub RtwqUnregisterPlatformFromMMCSS
