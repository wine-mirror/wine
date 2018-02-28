@ stdcall -norelay ExAcquireFastMutex(ptr)
@ stdcall -norelay ExReleaseFastMutex(ptr)
@ stdcall -norelay ExTryToAcquireFastMutex(ptr)
@ stub HalClearSoftwareInterrupt
@ stub HalRequestSoftwareInterrupt
@ stub HalSystemVectorDispatchEntry
@ stdcall -norelay KeAcquireInStackQueuedSpinLock(ptr ptr) ntoskrnl.exe.KeAcquireInStackQueuedSpinLock
@ stub KeAcquireInStackQueuedSpinLockRaiseToSynch
@ stub KeAcquireQueuedSpinLock
@ stub KeAcquireQueuedSpinLockRaiseToSynch
@ stub KeAcquireSpinLockRaiseToSynch
@ stdcall -norelay KeReleaseInStackQueuedSpinLock(ptr) ntoskrnl.exe.KeReleaseInStackQueuedSpinLock
@ stub KeReleaseQueuedSpinLock
@ stub KeTryToAcquireQueuedSpinLock
@ stub KeTryToAcquireQueuedSpinLockRaiseToSynch
@ stdcall -norelay KfAcquireSpinLock(ptr)
@ stdcall -norelay KfLowerIrql(long)
@ stdcall -norelay KfRaiseIrql(long)
@ stdcall -norelay KfReleaseSpinLock(ptr long)
@ stub HalAcquireDisplayOwnership
@ stub HalAdjustResourceList
@ stub HalAllProcessorsStarted
@ stub HalAllocateAdapterChannel
@ stub HalAllocateCommonBuffer
@ stub HalAllocateCrashDumpRegisters
@ stub HalAssignSlotResources
@ stub HalBeginSystemInterrupt
@ stub HalCalibratePerformanceCounter
@ stub HalDisableSystemInterrupt
@ stub HalDisplayString
@ stub HalEnableSystemInterrupt
@ stub HalEndSystemInterrupt
@ stub HalFlushCommonBuffer
@ stub HalFreeCommonBuffer
@ stub HalGetAdapter
@ stdcall HalGetBusData(long long long ptr long)
@ stdcall HalGetBusDataByOffset(long long long ptr long long)
@ stub HalGetEnvironmentVariable
@ stub HalGetInterruptVector
@ stub HalHandleNMI
@ stub HalInitSystem
@ stub HalInitializeProcessor
@ stub HalMakeBeep
@ stub HalProcessorIdle
@ stub HalQueryDisplayParameters
@ stub HalQueryRealTimeClock
@ stub HalReadDmaCounter
@ stub HalReportResourceUsage
@ stub HalRequestIpi
@ stub HalReturnToFirmware
@ stub HalSetBusData
@ stub HalSetBusDataByOffset
@ stub HalSetDisplayParameters
@ stub HalSetEnvironmentVariable
@ stub HalSetProfileInterval
@ stub HalSetRealTimeClock
@ stub HalSetTimeIncrement
@ stub HalStartNextProcessor
@ stub HalStartProfileInterrupt
@ stub HalStopProfileInterrupt
@ stdcall HalTranslateBusAddress(long long int64 ptr ptr)
@ stub IoAssignDriveLetters
@ stub IoFlushAdapterBuffers
@ stub IoFreeAdapterChannel
@ stub IoFreeMapRegisters
@ stub IoMapTransfer
@ stub IoReadPartitionTable
@ stub IoSetPartitionInformation
@ stub IoWritePartitionTable
@ stub KdComPortInUse
@ stub KeAcquireSpinLock
@ stub KeFlushWriteBuffer
@ stdcall KeGetCurrentIrql()
@ stub KeLowerIrql
@ stdcall -ret64 KeQueryPerformanceCounter(ptr)
@ stub KeRaiseIrql
@ stub KeRaiseIrqlToDpcLevel
@ stub KeRaiseIrqlToSynchLevel
@ stdcall KeReleaseSpinLock(ptr long) ntoskrnl.exe.KeReleaseSpinLock
@ stub KeStallExecutionProcessor
@ stub READ_PORT_BUFFER_UCHAR
@ stub READ_PORT_BUFFER_ULONG
@ stub READ_PORT_BUFFER_USHORT
@ stdcall READ_PORT_UCHAR(ptr)
@ stdcall READ_PORT_ULONG(ptr)
@ stub READ_PORT_USHORT
@ stub WRITE_PORT_BUFFER_UCHAR
@ stub WRITE_PORT_BUFFER_ULONG
@ stub WRITE_PORT_BUFFER_USHORT
@ stdcall WRITE_PORT_UCHAR(ptr long)
@ stdcall WRITE_PORT_ULONG(ptr long)
@ stub WRITE_PORT_USHORT
