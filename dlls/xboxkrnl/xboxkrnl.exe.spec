# xboxkrnl.exe - original Xbox kernel export table.
#
# All 378 ordinals below are real: the ordinal numbers, export names and
# argument counts/calling conventions come from Cxbx-Reloaded's
# src/core/kernel/exports/KernelThunk.cpp (the authoritative ordinal->name
# table; the real xboxkrnl.exe exports by ordinal only, no name table, hence
# -noname throughout) cross-referenced against the XBSYSAPI EXPORTNUM()
# declarations spread across src/core/kernel/common/*.h for each function's
# actual parameter list. See dlls/xboxkrnl/main.c for which ordinals are
# real forwards to equivalent ntdll/kernel32 functionality versus
# FIXME-logging stubs (dlls/xboxkrnl/generated_stubs.c).

  1 stdcall -noname AvGetSavedDataAddress() XBOXKRNL_AvGetSavedDataAddress
  2 stdcall -noname AvSendTVEncoderOption(ptr long long ptr) XBOXKRNL_AvSendTVEncoderOption
  3 stdcall -noname AvSetDisplayMode(ptr long long long long long) XBOXKRNL_AvSetDisplayMode
  4 stdcall -noname AvSetSavedDataAddress(ptr) XBOXKRNL_AvSetSavedDataAddress
  5 stdcall -noname DbgBreakPoint() XBOXKRNL_DbgBreakPoint
  6 stdcall -noname DbgBreakPointWithStatus(long) XBOXKRNL_DbgBreakPointWithStatus
  7 stdcall -noname DbgLoadImageSymbols(ptr ptr long) XBOXKRNL_DbgLoadImageSymbols
  8 varargs -noname DbgPrint(str) XBOXKRNL_DbgPrint
  9 stdcall -noname HalReadSMCTrayState(ptr ptr) XBOXKRNL_HalReadSMCTrayState
 10 stdcall -noname DbgPrompt(ptr ptr long) XBOXKRNL_DbgPrompt
 11 stdcall -noname DbgUnLoadImageSymbols(ptr ptr long) XBOXKRNL_DbgUnLoadImageSymbols
 12 stdcall -noname ExAcquireReadWriteLockExclusive(ptr) XBOXKRNL_ExAcquireReadWriteLockExclusive
 13 stdcall -noname ExAcquireReadWriteLockShared(ptr) XBOXKRNL_ExAcquireReadWriteLockShared
 14 stdcall -noname ExAllocatePool(long) XBOXKRNL_ExAllocatePool
 15 stdcall -noname ExAllocatePoolWithTag(long long) XBOXKRNL_ExAllocatePoolWithTag
 16 extern -noname ExEventObjectType XBOXKRNL_ExEventObjectType
 17 stdcall -noname ExFreePool(ptr) XBOXKRNL_ExFreePool
 18 stdcall -noname ExInitializeReadWriteLock(ptr) XBOXKRNL_ExInitializeReadWriteLock
 19 stdcall -noname ExInterlockedAddLargeInteger(ptr int64 ptr) XBOXKRNL_ExInterlockedAddLargeInteger
 20 stdcall -fastcall -noname ExInterlockedAddLargeStatistic(ptr long) XBOXKRNL_ExInterlockedAddLargeStatistic
 21 stdcall -fastcall -noname ExInterlockedCompareExchange64(ptr ptr ptr) XBOXKRNL_ExInterlockedCompareExchange64
 22 extern -noname ExMutantObjectType XBOXKRNL_ExMutantObjectType
 23 stdcall -noname ExQueryPoolBlockSize(ptr) XBOXKRNL_ExQueryPoolBlockSize
 24 stdcall -noname ExQueryNonVolatileSetting(long ptr ptr long ptr) XBOXKRNL_ExQueryNonVolatileSetting
 25 stdcall -noname ExReadWriteRefurbInfo(ptr long long) XBOXKRNL_ExReadWriteRefurbInfo
 26 stdcall -noname ExRaiseException(ptr) XBOXKRNL_ExRaiseException
 27 stdcall -noname ExRaiseStatus(long) XBOXKRNL_ExRaiseStatus
 28 stdcall -noname ExReleaseReadWriteLock(ptr) XBOXKRNL_ExReleaseReadWriteLock
 29 stdcall -noname ExSaveNonVolatileSetting(long long ptr long) XBOXKRNL_ExSaveNonVolatileSetting
 30 extern -noname ExSemaphoreObjectType XBOXKRNL_ExSemaphoreObjectType
 31 extern -noname ExTimerObjectType XBOXKRNL_ExTimerObjectType
 32 stdcall -fastcall -noname ExfInterlockedInsertHeadList(ptr ptr) XBOXKRNL_ExfInterlockedInsertHeadList
 33 stdcall -fastcall -noname ExfInterlockedInsertTailList(ptr ptr) XBOXKRNL_ExfInterlockedInsertTailList
 34 stdcall -fastcall -noname ExfInterlockedRemoveHeadList(ptr) XBOXKRNL_ExfInterlockedRemoveHeadList
 35 stdcall -noname FscGetCacheSize() XBOXKRNL_FscGetCacheSize
 36 stdcall -noname FscInvalidateIdleBlocks() XBOXKRNL_FscInvalidateIdleBlocks
 37 stdcall -noname FscSetCacheSize(long) XBOXKRNL_FscSetCacheSize
 38 stdcall -fastcall -noname HalClearSoftwareInterrupt(long) XBOXKRNL_HalClearSoftwareInterrupt
 39 stdcall -noname HalDisableSystemInterrupt(long) XBOXKRNL_HalDisableSystemInterrupt
 40 extern -noname HalDiskCachePartitionCount XBOXKRNL_HalDiskCachePartitionCount
 41 extern -noname HalDiskModelNumber XBOXKRNL_HalDiskModelNumber
 42 extern -noname HalDiskSerialNumber XBOXKRNL_HalDiskSerialNumber
 43 stdcall -noname HalEnableSystemInterrupt(long long) XBOXKRNL_HalEnableSystemInterrupt
 44 stdcall -noname HalGetInterruptVector(long ptr) XBOXKRNL_HalGetInterruptVector
 45 stdcall -noname HalReadSMBusValue(long long long ptr) XBOXKRNL_HalReadSMBusValue
 46 stdcall -noname HalReadWritePCISpace(long long long ptr long long) XBOXKRNL_HalReadWritePCISpace
 47 stdcall -noname HalRegisterShutdownNotification(ptr long) XBOXKRNL_HalRegisterShutdownNotification
 48 stdcall -fastcall -noname HalRequestSoftwareInterrupt(long) XBOXKRNL_HalRequestSoftwareInterrupt
 49 stdcall -noname HalReturnToFirmware(long) XBOXKRNL_HalReturnToFirmware
 50 stdcall -noname HalWriteSMBusValue(long long long long) XBOXKRNL_HalWriteSMBusValue
 51 stdcall -fastcall -noname InterlockedCompareExchange(ptr long long) XBOXKRNL_InterlockedCompareExchange
 52 stdcall -fastcall -noname InterlockedDecrement(ptr) XBOXKRNL_InterlockedDecrement
 53 stdcall -fastcall -noname InterlockedIncrement(ptr) XBOXKRNL_InterlockedIncrement
 54 stdcall -fastcall -noname InterlockedExchange(ptr long) XBOXKRNL_InterlockedExchange
 55 stdcall -fastcall -noname InterlockedExchangeAdd(ptr long) XBOXKRNL_InterlockedExchangeAdd
 56 stdcall -fastcall -noname InterlockedFlushSList(ptr) XBOXKRNL_InterlockedFlushSList
 57 stdcall -fastcall -noname InterlockedPopEntrySList(ptr) XBOXKRNL_InterlockedPopEntrySList
 58 stdcall -fastcall -noname InterlockedPushEntrySList(ptr ptr) XBOXKRNL_InterlockedPushEntrySList
 59 stdcall -noname IoAllocateIrp(long) XBOXKRNL_IoAllocateIrp
 60 stdcall -noname IoBuildAsynchronousFsdRequest(long ptr ptr long ptr ptr) XBOXKRNL_IoBuildAsynchronousFsdRequest
 61 stdcall -noname IoBuildDeviceIoControlRequest(long ptr ptr long ptr long long ptr ptr) XBOXKRNL_IoBuildDeviceIoControlRequest
 62 stdcall -noname IoBuildSynchronousFsdRequest(long ptr ptr long ptr ptr ptr) XBOXKRNL_IoBuildSynchronousFsdRequest
 63 stdcall -noname IoCheckShareAccess(long long ptr ptr long) XBOXKRNL_IoCheckShareAccess
 64 extern -noname IoCompletionObjectType XBOXKRNL_IoCompletionObjectType
 65 stdcall -noname IoCreateDevice(ptr long ptr long long ptr) XBOXKRNL_IoCreateDevice
 66 stdcall -noname IoCreateFile(ptr long ptr ptr ptr long long long long long) XBOXKRNL_IoCreateFile
 67 stdcall -noname IoCreateSymbolicLink(ptr ptr) XBOXKRNL_IoCreateSymbolicLink
 68 stdcall -noname IoDeleteDevice(ptr) XBOXKRNL_IoDeleteDevice
 69 stdcall -noname IoDeleteSymbolicLink(ptr) XBOXKRNL_IoDeleteSymbolicLink
 70 extern -noname IoDeviceObjectType XBOXKRNL_IoDeviceObjectType
 71 extern -noname IoFileObjectType XBOXKRNL_IoFileObjectType
 72 stdcall -noname IoFreeIrp(ptr) XBOXKRNL_IoFreeIrp
 73 stdcall -noname IoInitializeIrp(ptr long long) XBOXKRNL_IoInitializeIrp
 74 stdcall -noname IoInvalidDeviceRequest(ptr ptr) XBOXKRNL_IoInvalidDeviceRequest
 75 stdcall -noname IoQueryFileInformation(ptr long long ptr ptr) XBOXKRNL_IoQueryFileInformation
 76 stdcall -noname IoQueryVolumeInformation(ptr long long ptr ptr) XBOXKRNL_IoQueryVolumeInformation
 77 stdcall -noname IoQueueThreadIrp(ptr) XBOXKRNL_IoQueueThreadIrp
 78 stdcall -noname IoRemoveShareAccess(ptr ptr) XBOXKRNL_IoRemoveShareAccess
 79 stdcall -noname IoSetIoCompletion(ptr ptr ptr long long) XBOXKRNL_IoSetIoCompletion
 80 stdcall -noname IoSetShareAccess(long long ptr ptr) XBOXKRNL_IoSetShareAccess
 81 stdcall -noname IoStartNextPacket(ptr) XBOXKRNL_IoStartNextPacket
 82 stdcall -noname IoStartNextPacketByKey(ptr long) XBOXKRNL_IoStartNextPacketByKey
 83 stdcall -noname IoStartPacket(ptr ptr ptr) XBOXKRNL_IoStartPacket
 84 stdcall -noname IoSynchronousDeviceIoControlRequest(long ptr ptr long ptr long ptr long) XBOXKRNL_IoSynchronousDeviceIoControlRequest
 85 stdcall -noname IoSynchronousFsdRequest(long ptr ptr long ptr) XBOXKRNL_IoSynchronousFsdRequest
 86 stdcall -fastcall -noname IofCallDriver(ptr ptr) XBOXKRNL_IofCallDriver
 87 stdcall -fastcall -noname IofCompleteRequest(ptr long) XBOXKRNL_IofCompleteRequest
 88 extern -noname KdDebuggerEnabled XBOXKRNL_KdDebuggerEnabled
 89 extern -noname KdDebuggerNotPresent XBOXKRNL_KdDebuggerNotPresent
 90 stdcall -noname IoDismountVolume(ptr) XBOXKRNL_IoDismountVolume
 91 stdcall -noname IoDismountVolumeByName(ptr) XBOXKRNL_IoDismountVolumeByName
 92 stdcall -noname KeAlertResumeThread(ptr ptr) XBOXKRNL_KeAlertResumeThread
 93 stdcall -noname KeAlertThread(ptr) XBOXKRNL_KeAlertThread
 94 stdcall -noname KeBoostPriorityThread(ptr long) XBOXKRNL_KeBoostPriorityThread
 95 stdcall -noname KeBugCheck(long) XBOXKRNL_KeBugCheck
 96 stdcall -noname KeBugCheckEx(long ptr ptr ptr ptr) XBOXKRNL_KeBugCheckEx
 97 stdcall -noname KeCancelTimer(ptr) XBOXKRNL_KeCancelTimer
 98 stdcall -noname KeConnectInterrupt(ptr) XBOXKRNL_KeConnectInterrupt
 99 stdcall -noname KeDelayExecutionThread(long long ptr) XBOXKRNL_KeDelayExecutionThread
100 stdcall -noname KeDisconnectInterrupt(ptr) XBOXKRNL_KeDisconnectInterrupt
101 stdcall -noname KeEnterCriticalRegion(long) XBOXKRNL_KeEnterCriticalRegion
102 extern -noname MmGlobalData XBOXKRNL_MmGlobalData
103 stdcall -noname KeGetCurrentIrql() XBOXKRNL_KeGetCurrentIrql
104 stdcall -noname KeGetCurrentThread() XBOXKRNL_KeGetCurrentThread
105 stdcall -noname KeInitializeApc(ptr ptr ptr ptr ptr long ptr) XBOXKRNL_KeInitializeApc
106 stdcall -noname KeInitializeDeviceQueue(ptr) XBOXKRNL_KeInitializeDeviceQueue
107 stdcall -noname KeInitializeDpc(ptr ptr ptr) XBOXKRNL_KeInitializeDpc
108 stdcall -noname KeInitializeEvent(ptr long long) XBOXKRNL_KeInitializeEvent
109 stdcall -noname KeInitializeInterrupt(ptr ptr ptr long long long long) XBOXKRNL_KeInitializeInterrupt
110 stdcall -noname KeInitializeMutant(ptr long) XBOXKRNL_KeInitializeMutant
111 stdcall -noname KeInitializeQueue(ptr long) XBOXKRNL_KeInitializeQueue
112 stdcall -noname KeInitializeSemaphore(ptr long long) XBOXKRNL_KeInitializeSemaphore
113 stdcall -noname KeInitializeTimerEx(ptr long) XBOXKRNL_KeInitializeTimerEx
114 stdcall -noname KeInsertByKeyDeviceQueue(ptr ptr long) XBOXKRNL_KeInsertByKeyDeviceQueue
115 stdcall -noname KeInsertDeviceQueue(ptr ptr) XBOXKRNL_KeInsertDeviceQueue
116 stdcall -noname KeInsertHeadQueue(ptr ptr) XBOXKRNL_KeInsertHeadQueue
117 stdcall -noname KeInsertQueue(ptr ptr) XBOXKRNL_KeInsertQueue
118 stdcall -noname KeInsertQueueApc(ptr ptr ptr long) XBOXKRNL_KeInsertQueueApc
119 stdcall -noname KeInsertQueueDpc(ptr ptr ptr) XBOXKRNL_KeInsertQueueDpc
120 extern -noname KeInterruptTime XBOXKRNL_KeInterruptTime
121 stdcall -noname KeIsExecutingDpc() XBOXKRNL_KeIsExecutingDpc
122 stdcall -noname KeLeaveCriticalRegion(long) XBOXKRNL_KeLeaveCriticalRegion
123 stdcall -noname KePulseEvent(ptr long long) XBOXKRNL_KePulseEvent
124 stdcall -noname KeQueryBasePriorityThread(ptr) XBOXKRNL_KeQueryBasePriorityThread
125 stdcall -noname KeQueryInterruptTime() XBOXKRNL_KeQueryInterruptTime
126 stdcall -noname KeQueryPerformanceCounter() XBOXKRNL_KeQueryPerformanceCounter
127 stdcall -noname KeQueryPerformanceFrequency() XBOXKRNL_KeQueryPerformanceFrequency
128 stdcall -noname KeQuerySystemTime(ptr) XBOXKRNL_KeQuerySystemTime
129 stdcall -noname KeRaiseIrqlToDpcLevel() XBOXKRNL_KeRaiseIrqlToDpcLevel
130 stdcall -noname KeRaiseIrqlToSynchLevel() XBOXKRNL_KeRaiseIrqlToSynchLevel
131 stdcall -noname KeReleaseMutant(ptr long long long) XBOXKRNL_KeReleaseMutant
132 stdcall -noname KeReleaseSemaphore(ptr long long long) XBOXKRNL_KeReleaseSemaphore
133 stdcall -noname KeRemoveByKeyDeviceQueue(ptr long) XBOXKRNL_KeRemoveByKeyDeviceQueue
134 stdcall -noname KeRemoveDeviceQueue(ptr) XBOXKRNL_KeRemoveDeviceQueue
135 stdcall -noname KeRemoveEntryDeviceQueue(ptr ptr) XBOXKRNL_KeRemoveEntryDeviceQueue
136 stdcall -noname KeRemoveQueue(ptr long ptr) XBOXKRNL_KeRemoveQueue
137 stdcall -noname KeRemoveQueueDpc(ptr) XBOXKRNL_KeRemoveQueueDpc
138 stdcall -noname KeResetEvent(ptr) XBOXKRNL_KeResetEvent
139 stdcall -noname KeRestoreFloatingPointState(ptr) XBOXKRNL_KeRestoreFloatingPointState
140 stdcall -noname KeResumeThread(ptr) XBOXKRNL_KeResumeThread
141 stdcall -noname KeRundownQueue(ptr) XBOXKRNL_KeRundownQueue
142 stdcall -noname KeSaveFloatingPointState(ptr) XBOXKRNL_KeSaveFloatingPointState
143 stdcall -noname KeSetBasePriorityThread(ptr long) XBOXKRNL_KeSetBasePriorityThread
144 stdcall -noname KeSetDisableBoostThread(ptr long) XBOXKRNL_KeSetDisableBoostThread
145 stdcall -noname KeSetEvent(ptr long long) XBOXKRNL_KeSetEvent
146 stdcall -noname KeSetEventBoostPriority(ptr ptr) XBOXKRNL_KeSetEventBoostPriority
147 stdcall -noname KeSetPriorityProcess(ptr long) XBOXKRNL_KeSetPriorityProcess
148 stdcall -noname KeSetPriorityThread(ptr long) XBOXKRNL_KeSetPriorityThread
149 stdcall -noname KeSetTimer(ptr int64 ptr) XBOXKRNL_KeSetTimer
150 stdcall -noname KeSetTimerEx(ptr int64 long ptr) XBOXKRNL_KeSetTimerEx
151 stdcall -noname KeStallExecutionProcessor(long) XBOXKRNL_KeStallExecutionProcessor
152 stdcall -noname KeSuspendThread(ptr) XBOXKRNL_KeSuspendThread
153 stdcall -noname KeSynchronizeExecution(ptr ptr ptr) XBOXKRNL_KeSynchronizeExecution
154 extern -noname KeSystemTime XBOXKRNL_KeSystemTime
155 stdcall -noname KeTestAlertThread(long) XBOXKRNL_KeTestAlertThread
156 extern -noname KeTickCount XBOXKRNL_KeTickCount
157 extern -noname KeTimeIncrement XBOXKRNL_KeTimeIncrement
158 stdcall -noname KeWaitForMultipleObjects(long ptr long long long long ptr ptr) XBOXKRNL_KeWaitForMultipleObjects
159 stdcall -noname KeWaitForSingleObject(ptr long long long ptr) XBOXKRNL_KeWaitForSingleObject
160 stdcall -fastcall -noname KfRaiseIrql(long) XBOXKRNL_KfRaiseIrql
161 stdcall -fastcall -noname KfLowerIrql(long) XBOXKRNL_KfLowerIrql
162 extern -noname KiBugCheckData XBOXKRNL_KiBugCheckData
163 stdcall -fastcall -noname KiUnlockDispatcherDatabase(long) XBOXKRNL_KiUnlockDispatcherDatabase
164 extern -noname LaunchDataPage XBOXKRNL_LaunchDataPage
165 stdcall -noname MmAllocateContiguousMemory(long) XBOXKRNL_MmAllocateContiguousMemory
166 stdcall -noname MmAllocateContiguousMemoryEx(long long long long long) XBOXKRNL_MmAllocateContiguousMemoryEx
167 stdcall -noname MmAllocateSystemMemory(long long) XBOXKRNL_MmAllocateSystemMemory
168 stdcall -noname MmClaimGpuInstanceMemory(long ptr) XBOXKRNL_MmClaimGpuInstanceMemory
169 stdcall -noname MmCreateKernelStack(long long) XBOXKRNL_MmCreateKernelStack
170 stdcall -noname MmDeleteKernelStack(ptr ptr) XBOXKRNL_MmDeleteKernelStack
171 stdcall -noname MmFreeContiguousMemory(ptr) XBOXKRNL_MmFreeContiguousMemory
172 stdcall -noname MmFreeSystemMemory(ptr long) XBOXKRNL_MmFreeSystemMemory
173 stdcall -noname MmGetPhysicalAddress(ptr) XBOXKRNL_MmGetPhysicalAddress
174 stdcall -noname MmIsAddressValid(ptr) XBOXKRNL_MmIsAddressValid
175 stdcall -noname MmLockUnlockBufferPages(ptr long long) XBOXKRNL_MmLockUnlockBufferPages
176 stdcall -noname MmLockUnlockPhysicalPage(long long) XBOXKRNL_MmLockUnlockPhysicalPage
177 stdcall -noname MmMapIoSpace(long long long) XBOXKRNL_MmMapIoSpace
178 stdcall -noname MmPersistContiguousMemory(ptr long long) XBOXKRNL_MmPersistContiguousMemory
179 stdcall -noname MmQueryAddressProtect(ptr) XBOXKRNL_MmQueryAddressProtect
180 stdcall -noname MmQueryAllocationSize(ptr) XBOXKRNL_MmQueryAllocationSize
181 stdcall -noname MmQueryStatistics(ptr) XBOXKRNL_MmQueryStatistics
182 stdcall -noname MmSetAddressProtect(ptr long long) XBOXKRNL_MmSetAddressProtect
183 stdcall -noname MmUnmapIoSpace(ptr long) XBOXKRNL_MmUnmapIoSpace
184 stdcall -noname NtAllocateVirtualMemory(ptr long ptr long long) XBOXKRNL_NtAllocateVirtualMemory
185 stdcall -noname NtCancelTimer(ptr ptr) XBOXKRNL_NtCancelTimer
186 stdcall -noname NtClearEvent(ptr) XBOXKRNL_NtClearEvent
187 stdcall -noname NtClose(ptr) XBOXKRNL_NtClose
188 stdcall -noname NtCreateDirectoryObject(ptr ptr) XBOXKRNL_NtCreateDirectoryObject
189 stdcall -noname NtCreateEvent(ptr ptr long long) XBOXKRNL_NtCreateEvent
190 stdcall -noname NtCreateFile(ptr long ptr ptr ptr long long long long) XBOXKRNL_NtCreateFile
191 stdcall -noname NtCreateIoCompletion(ptr long ptr long) XBOXKRNL_NtCreateIoCompletion
192 stdcall -noname NtCreateMutant(ptr ptr long) XBOXKRNL_NtCreateMutant
193 stdcall -noname NtCreateSemaphore(ptr ptr long long) XBOXKRNL_NtCreateSemaphore
194 stdcall -noname NtCreateTimer(ptr ptr long) XBOXKRNL_NtCreateTimer
195 stdcall -noname NtDeleteFile(ptr) XBOXKRNL_NtDeleteFile
196 stdcall -noname NtDeviceIoControlFile(ptr ptr ptr ptr ptr long ptr long ptr long) XBOXKRNL_NtDeviceIoControlFile
197 stdcall -noname NtDuplicateObject(ptr ptr long) XBOXKRNL_NtDuplicateObject
198 stdcall -noname NtFlushBuffersFile(ptr ptr) XBOXKRNL_NtFlushBuffersFile
199 stdcall -noname NtFreeVirtualMemory(ptr ptr long) XBOXKRNL_NtFreeVirtualMemory
200 stdcall -noname NtFsControlFile(ptr ptr ptr ptr ptr long ptr long ptr long) XBOXKRNL_NtFsControlFile
201 stdcall -noname NtOpenDirectoryObject(ptr ptr) XBOXKRNL_NtOpenDirectoryObject
202 stdcall -noname NtOpenFile(ptr long ptr ptr long long) XBOXKRNL_NtOpenFile
203 stdcall -noname NtOpenSymbolicLinkObject(ptr ptr) XBOXKRNL_NtOpenSymbolicLinkObject
204 stdcall -noname NtProtectVirtualMemory(ptr ptr long ptr) XBOXKRNL_NtProtectVirtualMemory
205 stdcall -noname NtPulseEvent(ptr ptr) XBOXKRNL_NtPulseEvent
206 stdcall -noname NtQueueApcThread(ptr ptr ptr ptr ptr) XBOXKRNL_NtQueueApcThread
207 stdcall -noname NtQueryDirectoryFile(ptr ptr ptr ptr ptr ptr long long ptr long) XBOXKRNL_NtQueryDirectoryFile
208 stdcall -noname NtQueryDirectoryObject(ptr ptr long long ptr ptr) XBOXKRNL_NtQueryDirectoryObject
209 stdcall -noname NtQueryEvent(ptr ptr) XBOXKRNL_NtQueryEvent
210 stdcall -noname NtQueryFullAttributesFile(ptr ptr) XBOXKRNL_NtQueryFullAttributesFile
211 stdcall -noname NtQueryInformationFile(ptr ptr ptr long long) XBOXKRNL_NtQueryInformationFile
212 stdcall -noname NtQueryIoCompletion(ptr ptr) XBOXKRNL_NtQueryIoCompletion
213 stdcall -noname NtQueryMutant(ptr ptr) XBOXKRNL_NtQueryMutant
214 stdcall -noname NtQuerySemaphore(ptr ptr) XBOXKRNL_NtQuerySemaphore
215 stdcall -noname NtQuerySymbolicLinkObject(ptr ptr ptr) XBOXKRNL_NtQuerySymbolicLinkObject
216 stdcall -noname NtQueryTimer(ptr ptr) XBOXKRNL_NtQueryTimer
217 stdcall -noname NtQueryVirtualMemory(ptr ptr) XBOXKRNL_NtQueryVirtualMemory
218 stdcall -noname NtQueryVolumeInformationFile(ptr ptr ptr long long) XBOXKRNL_NtQueryVolumeInformationFile
219 stdcall -noname NtReadFile(ptr ptr ptr ptr ptr ptr long ptr) XBOXKRNL_NtReadFile
220 stdcall -noname NtReadFileScatter(ptr ptr ptr ptr ptr ptr long ptr) XBOXKRNL_NtReadFileScatter
221 stdcall -noname NtReleaseMutant(ptr ptr) XBOXKRNL_NtReleaseMutant
222 stdcall -noname NtReleaseSemaphore(ptr long ptr) XBOXKRNL_NtReleaseSemaphore
223 stdcall -noname NtRemoveIoCompletion(ptr ptr ptr ptr ptr) XBOXKRNL_NtRemoveIoCompletion
224 stdcall -noname NtResumeThread(ptr ptr) XBOXKRNL_NtResumeThread
225 stdcall -noname NtSetEvent(ptr ptr) XBOXKRNL_NtSetEvent
226 stdcall -noname NtSetInformationFile(ptr ptr ptr long long) XBOXKRNL_NtSetInformationFile
227 stdcall -noname NtSetIoCompletion(ptr ptr ptr long long) XBOXKRNL_NtSetIoCompletion
228 stdcall -noname NtSetSystemTime(ptr ptr) XBOXKRNL_NtSetSystemTime
229 stdcall -noname NtSetTimerEx(ptr ptr ptr long ptr long long ptr) XBOXKRNL_NtSetTimerEx
230 stdcall -noname NtSignalAndWaitForSingleObjectEx(ptr ptr long long ptr) XBOXKRNL_NtSignalAndWaitForSingleObjectEx
231 stdcall -noname NtSuspendThread(ptr ptr) XBOXKRNL_NtSuspendThread
232 stdcall -noname NtUserIoApcDispatcher(ptr ptr long) XBOXKRNL_NtUserIoApcDispatcher
233 stdcall -noname NtWaitForSingleObject(ptr long ptr) XBOXKRNL_NtWaitForSingleObject
234 stdcall -noname NtWaitForSingleObjectEx(ptr long long ptr) XBOXKRNL_NtWaitForSingleObjectEx
235 stdcall -noname NtWaitForMultipleObjectsEx(long ptr long long long ptr) XBOXKRNL_NtWaitForMultipleObjectsEx
236 stdcall -noname NtWriteFile(ptr ptr ptr ptr ptr ptr long ptr) XBOXKRNL_NtWriteFile
237 stdcall -noname NtWriteFileGather(ptr ptr ptr ptr ptr ptr long ptr) XBOXKRNL_NtWriteFileGather
238 stdcall -noname NtYieldExecution() XBOXKRNL_NtYieldExecution
239 stdcall -noname ObCreateObject(ptr ptr long ptr) XBOXKRNL_ObCreateObject
240 extern -noname ObDirectoryObjectType XBOXKRNL_ObDirectoryObjectType
241 stdcall -noname ObInsertObject(ptr ptr long ptr) XBOXKRNL_ObInsertObject
242 stdcall -noname ObMakeTemporaryObject(ptr) XBOXKRNL_ObMakeTemporaryObject
243 stdcall -noname ObOpenObjectByName(ptr ptr ptr ptr) XBOXKRNL_ObOpenObjectByName
244 stdcall -noname ObOpenObjectByPointer(ptr ptr ptr) XBOXKRNL_ObOpenObjectByPointer
245 extern -noname ObpObjectHandleTable XBOXKRNL_ObpObjectHandleTable
246 stdcall -noname ObReferenceObjectByHandle(ptr ptr ptr) XBOXKRNL_ObReferenceObjectByHandle
247 stdcall -noname ObReferenceObjectByName(ptr long ptr ptr ptr) XBOXKRNL_ObReferenceObjectByName
248 stdcall -noname ObReferenceObjectByPointer(ptr ptr) XBOXKRNL_ObReferenceObjectByPointer
249 extern -noname ObSymbolicLinkObjectType XBOXKRNL_ObSymbolicLinkObjectType
250 stdcall -fastcall -noname ObfDereferenceObject(ptr) XBOXKRNL_ObfDereferenceObject
251 stdcall -fastcall -noname ObfReferenceObject(ptr) XBOXKRNL_ObfReferenceObject
252 stdcall -noname PhyGetLinkState(long) XBOXKRNL_PhyGetLinkState
253 stdcall -noname PhyInitialize(long ptr) XBOXKRNL_PhyInitialize
254 stdcall -noname PsCreateSystemThread(ptr ptr ptr ptr long) XBOXKRNL_PsCreateSystemThread
255 stdcall -noname PsCreateSystemThreadEx(ptr long long long ptr ptr ptr long long ptr) XBOXKRNL_PsCreateSystemThreadEx
256 stdcall -noname PsQueryStatistics(ptr) XBOXKRNL_PsQueryStatistics
257 stdcall -noname PsSetCreateThreadNotifyRoutine(ptr) XBOXKRNL_PsSetCreateThreadNotifyRoutine
258 stdcall -noname PsTerminateSystemThread(long) XBOXKRNL_PsTerminateSystemThread
259 extern -noname PsThreadObjectType XBOXKRNL_PsThreadObjectType
260 stdcall -noname RtlAnsiStringToUnicodeString(ptr ptr long) XBOXKRNL_RtlAnsiStringToUnicodeString
261 stdcall -noname RtlAppendStringToString(ptr ptr) XBOXKRNL_RtlAppendStringToString
262 stdcall -noname RtlAppendUnicodeStringToString(ptr ptr) XBOXKRNL_RtlAppendUnicodeStringToString
263 stdcall -noname RtlAppendUnicodeToString(ptr ptr) XBOXKRNL_RtlAppendUnicodeToString
264 stdcall -noname RtlAssert(ptr ptr long ptr) XBOXKRNL_RtlAssert
265 stdcall -noname RtlCaptureContext(ptr) XBOXKRNL_RtlCaptureContext
266 stdcall -noname RtlCaptureStackBackTrace(long long ptr ptr) XBOXKRNL_RtlCaptureStackBackTrace
267 stdcall -noname RtlCharToInteger(ptr long ptr) XBOXKRNL_RtlCharToInteger
268 stdcall -noname RtlCompareMemory(ptr ptr long) XBOXKRNL_RtlCompareMemory
269 stdcall -noname RtlCompareMemoryUlong(ptr long long) XBOXKRNL_RtlCompareMemoryUlong
270 stdcall -noname RtlCompareString(ptr ptr long) XBOXKRNL_RtlCompareString
271 stdcall -noname RtlCompareUnicodeString(ptr ptr long) XBOXKRNL_RtlCompareUnicodeString
272 stdcall -noname RtlCopyString(ptr ptr) XBOXKRNL_RtlCopyString
273 stdcall -noname RtlCopyUnicodeString(ptr ptr) XBOXKRNL_RtlCopyUnicodeString
274 stdcall -noname RtlCreateUnicodeString(ptr ptr) XBOXKRNL_RtlCreateUnicodeString
275 stdcall -noname RtlDowncaseUnicodeChar(long) XBOXKRNL_RtlDowncaseUnicodeChar
276 stdcall -noname RtlDowncaseUnicodeString(ptr ptr long) XBOXKRNL_RtlDowncaseUnicodeString
277 stdcall -noname RtlEnterCriticalSection(ptr) XBOXKRNL_RtlEnterCriticalSection
278 stdcall -noname RtlEnterCriticalSectionAndRegion(ptr) XBOXKRNL_RtlEnterCriticalSectionAndRegion
279 stdcall -noname RtlEqualString(ptr ptr long) XBOXKRNL_RtlEqualString
280 stdcall -noname RtlEqualUnicodeString(ptr ptr long) XBOXKRNL_RtlEqualUnicodeString
281 stdcall -noname RtlExtendedIntegerMultiply(int64 long) XBOXKRNL_RtlExtendedIntegerMultiply
282 stdcall -noname RtlExtendedLargeIntegerDivide(int64 long ptr) XBOXKRNL_RtlExtendedLargeIntegerDivide
283 stdcall -noname RtlExtendedMagicDivide(int64 int64 long) XBOXKRNL_RtlExtendedMagicDivide
284 stdcall -noname RtlFillMemory(ptr long long) XBOXKRNL_RtlFillMemory
285 stdcall -noname RtlFillMemoryUlong(ptr long long) XBOXKRNL_RtlFillMemoryUlong
286 stdcall -noname RtlFreeAnsiString(ptr) XBOXKRNL_RtlFreeAnsiString
287 stdcall -noname RtlFreeUnicodeString(ptr) XBOXKRNL_RtlFreeUnicodeString
288 stdcall -noname RtlGetCallersAddress(ptr ptr) XBOXKRNL_RtlGetCallersAddress
289 stdcall -noname RtlInitAnsiString(ptr ptr) XBOXKRNL_RtlInitAnsiString
290 stdcall -noname RtlInitUnicodeString(ptr ptr) XBOXKRNL_RtlInitUnicodeString
291 stdcall -noname RtlInitializeCriticalSection(ptr) XBOXKRNL_RtlInitializeCriticalSection
292 stdcall -noname RtlIntegerToChar(long long long ptr) XBOXKRNL_RtlIntegerToChar
293 stdcall -noname RtlIntegerToUnicodeString(long long ptr) XBOXKRNL_RtlIntegerToUnicodeString
294 stdcall -noname RtlLeaveCriticalSection(ptr) XBOXKRNL_RtlLeaveCriticalSection
295 stdcall -noname RtlLeaveCriticalSectionAndRegion(ptr) XBOXKRNL_RtlLeaveCriticalSectionAndRegion
296 stdcall -noname RtlLowerChar(long) XBOXKRNL_RtlLowerChar
297 stdcall -noname RtlMapGenericMask(ptr ptr) XBOXKRNL_RtlMapGenericMask
298 stdcall -noname RtlMoveMemory(ptr ptr long) XBOXKRNL_RtlMoveMemory
299 stdcall -noname RtlMultiByteToUnicodeN(ptr long ptr ptr long) XBOXKRNL_RtlMultiByteToUnicodeN
300 stdcall -noname RtlMultiByteToUnicodeSize(ptr ptr long) XBOXKRNL_RtlMultiByteToUnicodeSize
301 stdcall -noname RtlNtStatusToDosError(long) XBOXKRNL_RtlNtStatusToDosError
302 stdcall -noname RtlRaiseException(ptr) XBOXKRNL_RtlRaiseException
303 stdcall -noname RtlRaiseStatus(long) XBOXKRNL_RtlRaiseStatus
304 stdcall -noname RtlTimeFieldsToTime(ptr ptr) XBOXKRNL_RtlTimeFieldsToTime
305 stdcall -noname RtlTimeToTimeFields(ptr ptr) XBOXKRNL_RtlTimeToTimeFields
306 stdcall -noname RtlTryEnterCriticalSection(ptr) XBOXKRNL_RtlTryEnterCriticalSection
307 stdcall -fastcall -noname RtlUlongByteSwap(long) XBOXKRNL_RtlUlongByteSwap
308 stdcall -noname RtlUnicodeStringToAnsiString(ptr ptr long) XBOXKRNL_RtlUnicodeStringToAnsiString
309 stdcall -noname RtlUnicodeStringToInteger(ptr long ptr) XBOXKRNL_RtlUnicodeStringToInteger
310 stdcall -noname RtlUnicodeToMultiByteN(ptr long ptr ptr long) XBOXKRNL_RtlUnicodeToMultiByteN
311 stdcall -noname RtlUnicodeToMultiByteSize(ptr ptr long) XBOXKRNL_RtlUnicodeToMultiByteSize
312 stdcall -noname RtlUnwind(ptr ptr ptr ptr) XBOXKRNL_RtlUnwind
313 stdcall -noname RtlUpcaseUnicodeChar(long) XBOXKRNL_RtlUpcaseUnicodeChar
314 stdcall -noname RtlUpcaseUnicodeString(ptr ptr long) XBOXKRNL_RtlUpcaseUnicodeString
315 stdcall -noname RtlUpcaseUnicodeToMultiByteN(ptr long ptr ptr long) XBOXKRNL_RtlUpcaseUnicodeToMultiByteN
316 stdcall -noname RtlUpperChar(long) XBOXKRNL_RtlUpperChar
317 stdcall -noname RtlUpperString(ptr ptr) XBOXKRNL_RtlUpperString
318 stdcall -fastcall -noname RtlUshortByteSwap(long) XBOXKRNL_RtlUshortByteSwap
319 stdcall -noname RtlWalkFrameChain(ptr long long) XBOXKRNL_RtlWalkFrameChain
320 stdcall -noname RtlZeroMemory(ptr long) XBOXKRNL_RtlZeroMemory
321 extern -noname XboxEEPROMKey XBOXKRNL_XboxEEPROMKey
322 extern -noname XboxHardwareInfo XBOXKRNL_XboxHardwareInfo
323 extern -noname XboxHDKey XBOXKRNL_XboxHDKey
324 extern -noname XboxKrnlVersion XBOXKRNL_XboxKrnlVersion
325 extern -noname XboxSignatureKey XBOXKRNL_XboxSignatureKey
326 extern -noname XeImageFileName XBOXKRNL_XeImageFileName
327 stdcall -noname XeLoadSection(ptr) XBOXKRNL_XeLoadSection
328 stdcall -noname XeUnloadSection(ptr) XBOXKRNL_XeUnloadSection
329 stdcall -noname READ_PORT_BUFFER_UCHAR(long ptr long) XBOXKRNL_READ_PORT_BUFFER_UCHAR
330 stdcall -noname READ_PORT_BUFFER_USHORT(long ptr long) XBOXKRNL_READ_PORT_BUFFER_USHORT
331 stdcall -noname READ_PORT_BUFFER_ULONG(long ptr long) XBOXKRNL_READ_PORT_BUFFER_ULONG
332 stdcall -noname WRITE_PORT_BUFFER_UCHAR(long ptr long) XBOXKRNL_WRITE_PORT_BUFFER_UCHAR
333 stdcall -noname WRITE_PORT_BUFFER_USHORT(long ptr long) XBOXKRNL_WRITE_PORT_BUFFER_USHORT
334 stdcall -noname WRITE_PORT_BUFFER_ULONG(long ptr long) XBOXKRNL_WRITE_PORT_BUFFER_ULONG
335 stdcall -noname XcSHAInit(ptr) XBOXKRNL_XcSHAInit
336 stdcall -noname XcSHAUpdate(ptr ptr long) XBOXKRNL_XcSHAUpdate
337 stdcall -noname XcSHAFinal(ptr ptr) XBOXKRNL_XcSHAFinal
338 stdcall -noname XcRC4Key(ptr long ptr) XBOXKRNL_XcRC4Key
339 stdcall -noname XcRC4Crypt(ptr long ptr) XBOXKRNL_XcRC4Crypt
340 stdcall -noname XcHMAC(ptr long ptr long ptr long ptr) XBOXKRNL_XcHMAC
341 stdcall -noname XcPKEncPublic(ptr ptr ptr) XBOXKRNL_XcPKEncPublic
342 stdcall -noname XcPKDecPrivate(ptr ptr ptr) XBOXKRNL_XcPKDecPrivate
343 stdcall -noname XcPKGetKeyLen(ptr) XBOXKRNL_XcPKGetKeyLen
344 stdcall -noname XcVerifyPKCS1Signature(ptr ptr ptr) XBOXKRNL_XcVerifyPKCS1Signature
345 stdcall -noname XcModExp(ptr ptr ptr ptr long) XBOXKRNL_XcModExp
346 stdcall -noname XcDESKeyParity(ptr long) XBOXKRNL_XcDESKeyParity
347 stdcall -noname XcKeyTable(long ptr ptr) XBOXKRNL_XcKeyTable
348 stdcall -noname XcBlockCrypt(long ptr ptr ptr long) XBOXKRNL_XcBlockCrypt
349 stdcall -noname XcBlockCryptCBC(long long ptr ptr ptr long ptr) XBOXKRNL_XcBlockCryptCBC
350 stdcall -noname XcCryptService(long ptr) XBOXKRNL_XcCryptService
351 stdcall -noname XcUpdateCrypto(ptr ptr) XBOXKRNL_XcUpdateCrypto
352 stdcall -noname RtlRip(ptr ptr ptr) XBOXKRNL_RtlRip
353 extern -noname XboxLANKey XBOXKRNL_XboxLANKey
354 extern -noname XboxAlternateSignatureKeys XBOXKRNL_XboxAlternateSignatureKeys
355 extern -noname XePublicKeyData XBOXKRNL_XePublicKeyData
356 extern -noname HalBootSMCVideoMode XBOXKRNL_HalBootSMCVideoMode
357 extern -noname IdexChannelObject XBOXKRNL_IdexChannelObject
358 stdcall -noname HalIsResetOrShutdownPending() XBOXKRNL_HalIsResetOrShutdownPending
359 stdcall -noname IoMarkIrpMustComplete(ptr) XBOXKRNL_IoMarkIrpMustComplete
360 stdcall -noname HalInitiateShutdown() XBOXKRNL_HalInitiateShutdown
361 cdecl -noname RtlSnprintf(ptr long ptr) XBOXKRNL_RtlSnprintf
362 cdecl -noname RtlSprintf(ptr ptr) XBOXKRNL_RtlSprintf
363 stdcall -noname RtlVsnprintf(ptr long ptr long) XBOXKRNL_RtlVsnprintf
364 stdcall -noname RtlVsprintf(ptr ptr long) XBOXKRNL_RtlVsprintf
365 stdcall -noname HalEnableSecureTrayEject() XBOXKRNL_HalEnableSecureTrayEject
366 stdcall -noname HalWriteSMCScratchRegister(long) XBOXKRNL_HalWriteSMCScratchRegister
367 stdcall -noname UnknownAPI367() XBOXKRNL_UnknownAPI367
368 stdcall -noname UnknownAPI368() XBOXKRNL_UnknownAPI368
369 stdcall -noname UnknownAPI369() XBOXKRNL_UnknownAPI369
370 stdcall -noname XProfpControl(long long) XBOXKRNL_XProfpControl
371 stdcall -noname XProfpGetData() XBOXKRNL_XProfpGetData
372 stdcall -noname IrtClientInitFast() XBOXKRNL_IrtClientInitFast
373 stdcall -noname IrtSweep() XBOXKRNL_IrtSweep
374 stdcall -noname MmDbgAllocateMemory(long long) XBOXKRNL_MmDbgAllocateMemory
375 stdcall -noname MmDbgFreeMemory(ptr long) XBOXKRNL_MmDbgFreeMemory
376 stdcall -noname MmDbgQueryAvailablePages() XBOXKRNL_MmDbgQueryAvailablePages
377 stdcall -noname MmDbgReleaseAddress(ptr ptr) XBOXKRNL_MmDbgReleaseAddress
378 stdcall -noname MmDbgWriteCheck(ptr ptr) XBOXKRNL_MmDbgWriteCheck
