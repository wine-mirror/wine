name	ntdll
type	win32

#note that the Zw... functions are alternate names for the 
#Nt... functions.  (see www.sysinternals.com for details)
#if you change a Nt.. function DON'T FORGET to change the
#Zw one too.

001 stub CsrAllocateCaptureBuffer
002 stub CsrAllocateCapturePointer
003 stub CsrAllocateMessagePointer
004 stub CsrCaptureMessageBuffer
005 stub CsrCaptureMessageString
006 stub CsrCaptureTimeout
007 stub CsrClientCallServer
008 stub CsrClientConnectToServer
009 stub CsrClientMaxMessage
010 stub CsrClientSendMessage
011 stub CsrClientThreadConnect
012 stub CsrFreeCaptureBuffer
013 stub CsrIdentifyAlertableThread
014 stub CsrNewThread
015 stub CsrProbeForRead
016 stub CsrProbeForWrite
017 stub CsrSetPriorityClass
018 stub CsrpProcessCallbackRequest
019 stub DbgBreakPoint
020 cdecl DbgPrint(str long) DbgPrint
021 stub DbgPrompt
022 stub DbgSsHandleKmApiMsg
023 stub DbgSsInitialize
024 stub DbgUiConnectToDbg
025 stub DbgUiContinue
026 stub DbgUiWaitStateChange
027 stub DbgUserBreakPoint
028 stub KiUserApcDispatcher
029 stub KiUserCallbackDispatcher
030 stub KiUserExceptionDispatcher
031 stub LdrAccessResource
032 stub LdrDisableThreadCalloutsForDll
033 stub LdrEnumResources
034 stub LdrFindEntryForAddress
035 stub LdrFindResourceDirectory_U
036 stub LdrFindResource_U
037 stub LdrGetDllHandle
038 stub LdrGetProcedureAddress
039 stub LdrInitializeThunk
040 stub LdrLoadDll
041 stub LdrProcessRelocationBlock
042 stub LdrQueryImageFileExecutionOptions
043 stub LdrQueryProcessModuleInformation
044 stub LdrShutdownProcess
045 stub LdrShutdownThread
046 stub LdrUnloadDll
047 stub LdrVerifyImageMatchesChecksum
048 stub NPXEMULATORTABLE
049 stub NlsMbCodePageTag
050 stub NlsMbOemCodePageTag
051 stdcall NtAcceptConnectPort(long long long long long long) NtAcceptConnectPort
052 stub NtAccessCheck
053 stub NtAccessCheckAndAuditAlarm
054 stub NtAdjustGroupsToken
055 stdcall NtAdjustPrivilegesToken(long long long long long long) NtAdjustPrivilegesToken
056 stub NtAlertResumeThread
057 stub NtAlertThread
058 stub NtAllocateLocallyUniqueId
059 stub NtAllocateUuids
060 stub NtAllocateVirtualMemory
061 stub NtCallbackReturn
062 stub NtCancelIoFile
063 stub NtCancelTimer
064 stub NtClearEvent
065 stdcall NtClose(long) NtClose
066 stub NtCloseObjectAuditAlarm
067 stdcall NtCompleteConnectPort(long) NtCompleteConnectPort
068 stdcall NtConnectPort(long long long long long long long long) NtConnectPort
069 stub NtContinue
070 stdcall NtCreateDirectoryObject(long long long) NtCreateDirectoryObject
071 stdcall NtCreateEvent(long long long long long) NtCreateEvent
072 stub NtCreateEventPair
073 stdcall NtCreateFile(ptr long ptr ptr long long long ptr long long ptr) NtCreateFile
074 stub NtCreateIoCompletion
075 stdcall NtCreateKey(long long long long long long long) NtCreateKey
076 stdcall NtCreateMailslotFile(long long long long long long long long) NtCreateMailslotFile
077 stub NtCreateMutant
078 stub NtCreateNamedPipeFile
079 stdcall NtCreatePagingFile(long long long long) NtCreatePagingFile
080 stdcall NtCreatePort(long long long long long) NtCreatePort
081 stub NtCreateProcess
082 stub NtCreateProfile
083 stdcall NtCreateSection(long long long long long long long) NtCreateSection
084 stub NtCreateSemaphore
085 stub NtCreateSymbolicLinkObject
086 stub NtCreateThread
087 stdcall NtCreateTimer(long long long) NtCreateTimer
088 stub NtCreateToken
089 stdcall NtCurrentTeb() NtCurrentTeb
090 stub NtDelayExecution
091 stub NtDeleteFile
092 stub NtDeleteKey
093 stub NtDeleteValueKey
094 stdcall NtDeviceIoControlFile(long long long long long long long long long long) NtDeviceIoControlFile
095 stub NtDisplayString
096 stdcall NtDuplicateObject(long long long long long long long) NtDuplicateObject
097 stdcall NtDuplicateToken(long long long long long long) NtDuplicateToken
098 stub NtEnumerateBus
099 stdcall NtEnumerateKey (long long long long long long) NtEnumerateKey
100 stdcall NtEnumerateValueKey (long long long long long long) NtEnumerateValueKey
101 stub NtExtendSection
102 stub NtFlushBuffersFile
103 stub NtFlushInstructionCache
104 stub NtFlushKey
105 stub NtFlushVirtualMemory
106 stub NtFlushWriteBuffer
107 stub NtFreeVirtualMemory
108 stdcall NtFsControlFile(long long long long long long long long long long) NtFsControlFile
109 stub NtGetContextThread
110 stub NtGetPlugPlayEvent
111 stub NtGetTickCount
112 stub NtImpersonateClientOfPort
113 stub NtImpersonateThread
114 stub NtInitializeRegistry
115 stdcall NtListenPort(long long) NtListenPort
116 stub NtLoadDriver
117 stub NtLoadKey
118 stub NtLockFile
119 stub NtLockVirtualMemory
120 stub NtMakeTemporaryObject
121 stdcall NtMapViewOfSection(long long long long long long long long long long) NtMapViewOfSection
122 stub NtNotifyChangeDirectoryFile
123 stub NtNotifyChangeKey
124 stdcall NtOpenDirectoryObject(long long long) NtOpenDirectoryObject
125 stdcall NtOpenEvent(long long long) NtOpenEvent
126 stub NtOpenEventPair
127 stdcall NtOpenFile(ptr long ptr ptr long long) NtOpenFile
128 stub NtOpenIoCompletion
129 stdcall NtOpenKey(ptr long ptr) NtOpenKey
130 stub NtOpenMutant
131 stub NtOpenObjectAuditAlarm
132 stub NtOpenProcess
133 stdcall NtOpenProcessToken(long long long) NtOpenProcessToken
134 stdcall NtOpenSection(long long long) NtOpenSection
135 stub NtOpenSemaphore
136 stdcall NtOpenSymbolicLinkObject (long long long) NtOpenSymbolicLinkObject
137 stub NtOpenThread
138 stdcall NtOpenThreadToken(long long long long) NtOpenThreadToken
139 stub NtOpenTimer
140 stub NtPlugPlayControl
141 stub NtPrivilegeCheck
142 stub NtPrivilegeObjectAuditAlarm
143 stub NtPrivilegedServiceAuditAlarm
144 stub NtProtectVirtualMemory
145 stub NtPulseEvent
146 stub NtQueryAttributesFile
147 stub NtQueryDefaultLocale
148 stub NtQueryDirectoryFile
149 stdcall NtQueryDirectoryObject(long long long long long long long) NtQueryDirectoryObject
150 stub NtQueryEaFile
151 stub NtQueryEvent
152 stdcall NtQueryInformationFile(long long long long long) NtQueryInformationFile
153 stub NtQueryInformationPort
154 stdcall NtQueryInformationProcess(long long long long long) NtQueryInformationProcess
155 stdcall NtQueryInformationThread (long long long long long) NtQueryInformationThread
156 stdcall NtQueryInformationToken (long long long long long) NtQueryInformationToken
157 stub NtQueryIntervalProfile
158 stub NtQueryIoCompletion
159 stdcall NtQueryKey (long long long long) NtQueryKey
160 stub NtQueryMutant
161 stdcall NtQueryObject(long long long long long) NtQueryObject
162 stdcall NtQueryPerformanceCounter (long long) NtQueryPerformanceCounter
163 stdcall NtQuerySection (long long long long long) NtQuerySection
164 stdcall NtQuerySecurityObject (long long long long long) NtQuerySecurityObject
165 stdcall NtQuerySemaphore (long long long long long) NtQuerySemaphore
166 stub NtQuerySymbolicLinkObject
167 stub NtQuerySystemEnvironmentValue
168 stdcall NtQuerySystemInformation(long long long long) NtQuerySystemInformation
169 stdcall NtQuerySystemTime(ptr) GetSystemTimeAsFileTime
170 stub NtQueryTimer
171 stdcall NtQueryTimerResolution(long long long) NtQueryTimerResolution
172 stdcall NtQueryValueKey(long long long long long long) NtQueryValueKey
173 stub NtQueryVirtualMemory
174 stub NtQueryVolumeInformationFile
175 stdcall NtRaiseException(long long long ptr) NtRaiseException
176 stub NtRaiseHardError
177 stdcall NtReadFile(long long long long long long long long long) NtReadFile
178 stub NtReadRequestData
179 stub NtReadVirtualMemory
180 stub NtRegisterNewDevice
181 stdcall NtRegisterThreadTerminatePort(long) NtRegisterThreadTerminatePort
182 stub NtReleaseMutant
183 stub NtReleaseProcessMutant
184 stub NtReleaseSemaphore
185 stub NtRemoveIoCompletion
186 stub NtReplaceKey
187 stub NtReplyPort
188 stdcall NtReplyWaitReceivePort(long long long long) NtReplyWaitReceivePort
189 stub NtReplyWaitReplyPort
190 stub NtRequestPort
191 stdcall NtRequestWaitReplyPort(long long long) NtRequestWaitReplyPort
192 stub NtResetEvent
193 stub NtRestoreKey
194 stdcall NtResumeThread(long long) NtResumeThread
195 stub NtSaveKey
196 stub NtSetContextThread
197 stub NtSetDefaultHardErrorPort
198 stub NtSetDefaultLocale
199 stub NtSetEaFile
200 stdcall NtSetEvent(long long) NtSetEvent
201 stub NtSetHighEventPair
202 stub NtSetHighWaitLowEventPair
203 stub NtSetHighWaitLowThread
204 stdcall NtSetInformationFile(long long long long long) NtSetInformationFile
205 stub NtSetInformationKey
206 stub NtSetInformationObject
207 stdcall NtSetInformationProcess(long long long long) NtSetInformationProcess
208 stdcall NtSetInformationThread(long long long long) NtSetInformationThread
209 stub NtSetInformationToken
210 stdcall NtSetIntervalProfile(long long) NtSetIntervalProfile
211 stub NtSetIoCompletion
212 stub NtSetLdtEntries
213 stub NtSetLowEventPair
214 stub NtSetLowWaitHighEventPair
215 stub NtSetLowWaitHighThread
216 stub NtSetSecurityObject
217 stub NtSetSystemEnvironmentValue
218 stub NtSetSystemInformation
219 stub NtSetSystemPowerState
220 stub NtSetSystemTime
221 stdcall NtSetTimer(long long long long long long) NtSetTimer
222 stub NtSetTimerResolution
223 stdcall NtSetValueKey(long long long long long long) NtSetValueKey
224 stdcall NtSetVolumeInformationFile(long long long long long) NtSetVolumeInformationFile
225 stub NtShutdownSystem
226 stub NtStartProfile
227 stub NtStopProfile
228 stub NtSuspendThread
229 stub NtSystemDebugControl
230 stub NtTerminateProcess
231 stdcall NtTerminateThread(long long) NtTerminateThread
232 stub NtTestAlert
233 stub NtUnloadDriver
234 stub NtUnloadKey
235 stub NtUnlockFile
236 stub NtUnlockVirtualMemory
237 stub NtUnmapViewOfSection
238 stub NtVdmControl
239 stub NtW32Call
240 stub NtWaitForMultipleObjects
241 stub NtWaitForProcessMutant
242 stdcall NtWaitForSingleObject(long long long) NtWaitForSingleObject
243 stub NtWaitHighEventPair
244 stub NtWaitLowEventPair
245 stub NtWriteFile
246 stub NtWriteRequestData
247 stub NtWriteVirtualMemory
248 stub PfxFindPrefix
249 stub PfxInitialize
250 stub PfxInsertPrefix
251 stub PfxRemovePrefix
252 stub RestoreEm87Context
253 stub RtlAbortRXact
254 stub RtlAbsoluteToSelfRelativeSD
255 stdcall RtlAcquirePebLock() RtlAcquirePebLock
256 stdcall RtlAcquireResourceExclusive(ptr long) RtlAcquireResourceExclusive
257 stdcall RtlAcquireResourceShared(ptr long) RtlAcquireResourceShared
258 stdcall RtlAddAccessAllowedAce(long long long long) RtlAddAccessAllowedAce
259 stub RtlAddAccessDeniedAce
260 stdcall RtlAddAce(ptr long long ptr long) RtlAddAce
261 stub RtlAddActionToRXact
262 stub RtlAddAttributeActionToRXact
263 stub RtlAddAuditAccessAce
264 stdcall RtlAdjustPrivilege(long long long long) RtlAdjustPrivilege
265 stdcall RtlAllocateAndInitializeSid (ptr long long long long long long long long long ptr) RtlAllocateAndInitializeSid
266 stdcall RtlAllocateHeap(long long long) HeapAlloc
267 stub RtlAnsiCharToUnicodeChar
268 stub RtlAnsiStringToUnicodeSize
269 stdcall RtlAnsiStringToUnicodeString(ptr ptr long) RtlAnsiStringToUnicodeString
270 stub RtlAppendAsciizToString
271 stub RtlAppendStringToString
272 stub RtlAppendUnicodeStringToString
273 stub RtlAppendUnicodeToString
274 stub RtlApplyRXact
275 stub RtlApplyRXactNoFlush
276 stub RtlAreAllAccessesGranted
277 stub RtlAreAnyAccessesGranted
278 stub RtlAreBitsClear
279 stub RtlAreBitsSet
280 stub RtlAssert
281 stub RtlCaptureStackBackTrace
282 stub RtlCharToInteger
283 stub RtlCheckRegistryKey
284 stub RtlClearAllBits
285 stub RtlClearBits
286 stub RtlCompactHeap
287 stub RtlCompareMemory
288 stub RtlCompareMemoryUlong
289 stub RtlCompareString
290 stdcall RtlCompareUnicodeString (ptr ptr long) RtlCompareUnicodeString
291 stub RtlCompressBuffer
292 stub RtlConsoleMultiByteToUnicodeN
293 stub RtlConvertExclusiveToShared
294 stub RtlConvertLongToLargeInteger
295 stub RtlConvertSharedToExclusive
296 stub RtlConvertSidToUnicodeString
297 stub RtlConvertUiListToApiList
298 stub RtlConvertUlongToLargeInteger
299 stub RtlCopyLuid
300 stub RtlCopyLuidAndAttributesArray
301 stub RtlCopySecurityDescriptor
302 stdcall RtlCopySid(long ptr ptr) RtlCopySid
303 stub RtlCopySidAndAttributesArray
304 stub RtlCopyString
305 stub RtlCopyUnicodeString
306 stdcall RtlCreateAcl(ptr long long) RtlCreateAcl
307 stub RtlCreateAndSetSD
308 stdcall RtlCreateEnvironment(long long) RtlCreateEnvironment
309 stdcall RtlCreateHeap(long long long) HeapCreate
310 stub RtlCreateProcessParameters
311 stub RtlCreateQueryDebugBuffer
312 stub RtlCreateRegistryKey
313 stdcall RtlCreateSecurityDescriptor(ptr long) RtlCreateSecurityDescriptor
314 stub RtlCreateTagHeap
315 stub RtlCreateUnicodeString
316 stub RtlCreateUnicodeStringFromAsciiz
317 stub RtlCreateUserProcess
318 stub RtlCreateUserSecurityObject
319 stub RtlCreateUserThread
320 stub RtlCustomCPToUnicodeN
321 stub RtlCutoverTimeToSystemTime
322 stub RtlDeNormalizeProcessParams
323 stub RtlDecompressBuffer
324 stub RtlDecompressFragment
325 stub RtlDelete
326 stub RtlDeleteAce
327 stdcall RtlDeleteCriticalSection(ptr) DeleteCriticalSection
328 stub RtlDeleteElementGenericTable
329 stub RtlDeleteRegistryValue
330 stdcall RtlDeleteResource(ptr) RtlDeleteResource
331 stdcall RtlDeleteSecurityObject(long) RtlDeleteSecurityObject
332 stdcall RtlDestroyEnvironment(long) RtlDestroyEnvironment
333 stdcall RtlDestroyHeap(long) HeapDestroy
334 stub RtlDestroyProcessParameters
335 stub RtlDestroyQueryDebugBuffer
336 stub RtlDetermineDosPathNameType_U
337 stub RtlDoesFileExists_U
338 stdcall RtlDosPathNameToNtPathName_U(ptr ptr long long) RtlDosPathNameToNtPathName_U
339 stub RtlDosSearchPath_U
340 stdcall RtlDumpResource(ptr) RtlDumpResource
341 stub RtlEnlargedIntegerMultiply
342 stub RtlEnlargedUnsignedDivide
343 stub RtlEnlargedUnsignedMultiply
344 stdcall RtlEnterCriticalSection(ptr) EnterCriticalSection
345 stub RtlEnumProcessHeaps
346 stub RtlEnumerateGenericTable
347 stub RtlEnumerateGenericTableWithoutSplaying
348 stub RtlEqualComputerName
349 stub RtlEqualDomainName
350 stub RtlEqualLuid
351 stub RtlEqualPrefixSid
352 stdcall RtlEqualSid (long long) RtlEqualSid
353 stub RtlEqualString
354 stdcall RtlEqualUnicodeString(long long long) RtlEqualUnicodeString
355 stub RtlEraseUnicodeString
356 stub RtlExpandEnvironmentStrings_U
357 stub RtlExtendHeap
358 stdcall RtlExtendedIntegerMultiply(long long long) RtlExtendedIntegerMultiply
359 stdcall RtlExtendedLargeIntegerDivide(long long long ptr) RtlExtendedLargeIntegerDivide
360 stub RtlExtendedMagicDivide
361 stdcall RtlFillMemory(ptr long long) RtlFillMemory
362 stub RtlFillMemoryUlong
363 stub RtlFindClearBits
364 stub RtlFindClearBitsAndSet
365 stub RtlFindLongestRunClear
366 stub RtlFindLongestRunSet
367 stub RtlFindMessage
368 stub RtlFindSetBits
369 stub RtlFindSetBitsAndClear
370 stdcall RtlFirstFreeAce(ptr ptr) RtlFirstFreeAce
371 stdcall RtlFormatCurrentUserKeyPath() RtlFormatCurrentUserKeyPath
372 stub RtlFormatMessage
373 stdcall RtlFreeAnsiString(long) RtlFreeAnsiString
374 stdcall RtlFreeHeap(long long long) HeapFree
375 stub RtlFreeOemString
376 stdcall RtlFreeSid (long) RtlFreeSid
377 stdcall RtlFreeUnicodeString(ptr) RtlFreeUnicodeString
378 stub RtlGenerate8dot3Name
379 stdcall RtlGetAce(ptr long ptr) RtlGetAce
380 stub RtlGetCallersAddress
381 stub RtlGetCompressionWorkSpaceSize
382 stub RtlGetControlSecurityDescriptor
383 stub RtlGetCurrentDirectory_U
384 stdcall RtlGetDaclSecurityDescriptor(long long long long) RtlGetDaclSecurityDescriptor
385 stub RtlGetElementGenericTable
386 stub RtlGetFullPathName_U
387 stub RtlGetGroupSecurityDescriptor
388 stub RtlGetLongestNtPathLength
389 stub RtlGetNtGlobalFlags
390 stdcall RtlGetNtProductType(ptr) RtlGetNtProductType
391 stub RtlGetOwnerSecurityDescriptor
392 stub RtlGetProcessHeaps
393 stub RtlGetSaclSecurityDescriptor
394 stub RtlGetUserInfoHeap
395 stub RtlIdentifierAuthoritySid
396 stub RtlImageDirectoryEntryToData
397 stdcall RtlImageNtHeader(long) RtlImageNtHeader
398 stub RtlImpersonateSelf
399 stdcall RtlInitAnsiString(ptr str) RtlInitAnsiString
400 stub RtlInitCodePageTable
401 stub RtlInitNlsTables
402 stdcall RtlInitString(ptr str) RtlInitString
403 stdcall RtlInitUnicodeString(ptr wstr) RtlInitUnicodeString
404 stub RtlInitializeBitMap
405 stub RtlInitializeContext
406 stdcall RtlInitializeCriticalSection(ptr) InitializeCriticalSection
407 stub RtlInitializeGenericTable
408 stub RtlInitializeRXact
409 stdcall RtlInitializeResource(ptr) RtlInitializeResource
410 stdcall RtlInitializeSid(ptr ptr long) RtlInitializeSid
411 stub RtlInsertElementGenericTable
412 stdcall RtlIntegerToChar(long long long long) RtlIntegerToChar
413 stub RtlIntegerToUnicodeString
414 stub RtlIsDosDeviceName_U
415 stub RtlIsGenericTableEmpty
416 stub RtlIsNameLegalDOS8Dot3
417 stdcall RtlIsTextUnicode(ptr long ptr) RtlIsTextUnicode
418 stub RtlLargeIntegerAdd
419 stub RtlLargeIntegerArithmeticShift
420 stub RtlLargeIntegerDivide
421 stub RtlLargeIntegerNegate
422 stub RtlLargeIntegerShiftLeft
423 stub RtlLargeIntegerShiftRight
424 stub RtlLargeIntegerSubtract
425 stub RtlLargeIntegerToChar
426 stdcall RtlLeaveCriticalSection(ptr) LeaveCriticalSection
427 stdcall RtlLengthRequiredSid(long) RtlLengthRequiredSid
428 stub RtlLengthSecurityDescriptor
429 stdcall RtlLengthSid(ptr) RtlLengthSid
430 stub RtlLocalTimeToSystemTime
431 stub RtlLockHeap
432 stub RtlLookupElementGenericTable
433 stub RtlMakeSelfRelativeSD
434 stub RtlMapGenericMask
435 stdcall RtlMoveMemory(ptr ptr long) RtlMoveMemory
436 stdcall RtlMultiByteToUnicodeN(ptr long ptr ptr long) RtlMultiByteToUnicodeN
437 stub RtlMultiByteToUnicodeSize
438 stub RtlNewInstanceSecurityObject
439 stub RtlNewSecurityGrantedAccess
440 stdcall RtlNewSecurityObject(long long long long long long) RtlNewSecurityObject
441 stdcall RtlNormalizeProcessParams(ptr) RtlNormalizeProcessParams
442 stdcall RtlNtStatusToDosError(long) RtlNtStatusToDosError
443 stub RtlNumberGenericTableElements
444 stub RtlNumberOfClearBits
445 stub RtlNumberOfSetBits
446 stub RtlOemStringToUnicodeSize
447 stdcall RtlOemStringToUnicodeString(ptr ptr long) RtlOemStringToUnicodeString
448 stdcall RtlOemToUnicodeN(ptr long ptr ptr long) RtlOemToUnicodeN
449 stdcall RtlOpenCurrentUser(long ptr) RtlOpenCurrentUser
450 stub RtlPcToFileHeader
451 stub RtlPrefixString
452 stub RtlPrefixUnicodeString
453 stub RtlProtectHeap
454 stdcall RtlQueryEnvironmentVariable_U(long long long) RtlQueryEnvironmentVariable_U
455 stub RtlQueryInformationAcl
456 stub RtlQueryProcessBackTraceInformation
457 stub RtlQueryProcessDebugInformation
458 stub RtlQueryProcessHeapInformation
459 stub RtlQueryProcessLockInformation
460 stub RtlQueryRegistryValues
461 stub RtlQuerySecurityObject
462 stub RtlQueryTagHeap
463 stub RtlQueryTimeZoneInformation
464 stdcall RtlRaiseException (long) RtlRaiseException
465 stub RtlRaiseStatus
466 stub RtlRandom
467 stub RtlReAllocateHeap
468 stub RtlRealPredecessor
469 stub RtlRealSuccessor
470 stdcall RtlReleasePebLock() RtlReleasePebLock
471 stdcall RtlReleaseResource(ptr) RtlReleaseResource
472 stub RtlRemoteCall
473 stub RtlResetRtlTranslations
474 stub RtlRunDecodeUnicodeString
475 stub RtlRunEncodeUnicodeString
476 stub RtlSecondsSince1970ToTime
477 stub RtlSecondsSince1980ToTime
478 stub RtlSelfRelativeToAbsoluteSD
479 stub RtlSetAllBits
480 stub RtlSetBits
481 stub RtlSetCurrentDirectory_U
482 stub RtlSetCurrentEnvironment
483 stdcall RtlSetDaclSecurityDescriptor(ptr long ptr long) RtlSetDaclSecurityDescriptor
484 stdcall RtlSetEnvironmentVariable(long long long) RtlSetEnvironmentVariable
485 stdcall RtlSetGroupSecurityDescriptor(ptr ptr long) RtlSetGroupSecurityDescriptor
486 stub RtlSetInformationAcl
487 stdcall RtlSetOwnerSecurityDescriptor(ptr ptr long) RtlSetOwnerSecurityDescriptor
488 stdcall RtlSetSaclSecurityDescriptor(ptr long ptr long) RtlSetSaclSecurityDescriptor
489 stub RtlSetSecurityObject
490 stub RtlSetTimeZoneInformation
491 stub RtlSetUserFlagsHeap
492 stub RtlSetUserValueHeap
493 stdcall RtlSizeHeap(long long long) HeapSize
494 stub RtlSplay
495 stub RtlStartRXact
496 stdcall RtlSubAuthorityCountSid(ptr) RtlSubAuthorityCountSid
497 stdcall RtlSubAuthoritySid(ptr long) RtlSubAuthoritySid
498 stub RtlSubtreePredecessor
499 stub RtlSubtreeSuccessor
500 stdcall RtlSystemTimeToLocalTime (long long) RtlSystemTimeToLocalTime
501 stub RtlTimeFieldsToTime
502 stdcall RtlTimeToElapsedTimeFields(long long) RtlTimeToElapsedTimeFields
503 stdcall RtlTimeToSecondsSince1970(ptr ptr) RtlTimeToSecondsSince1970
504 stdcall RtlTimeToSecondsSince1980(ptr ptr) RtlTimeToSecondsSince1980
505 stdcall RtlTimeToTimeFields (long long) RtlTimeToTimeFields
506 stub RtlUnicodeStringToAnsiSize
507 stdcall RtlUnicodeStringToAnsiString(ptr ptr long) RtlUnicodeStringToAnsiString
508 stub RtlUnicodeStringToCountedOemString
509 stub RtlUnicodeStringToInteger
510 stub RtlUnicodeStringToOemSize
511 stdcall RtlUnicodeStringToOemString(ptr ptr long) RtlUnicodeStringToOemString
512 stub RtlUnicodeToCustomCPN
513 stub RtlUnicodeToMultiByteN
514 stub RtlUnicodeToMultiByteSize
515 stdcall RtlUnicodeToOemN(ptr long ptr ptr long) RtlUnicodeToOemN
516 stub RtlUniform
517 stub RtlUnlockHeap
518 stub RtlUnwind
519 stub RtlUpcaseUnicodeChar
520 stdcall RtlUpcaseUnicodeString(ptr ptr long) RtlUpcaseUnicodeString
521 stub RtlUpcaseUnicodeStringToAnsiString
522 stub RtlUpcaseUnicodeStringToCountedOemString
523 stub RtlUpcaseUnicodeStringToOemString
524 stub RtlUpcaseUnicodeToCustomCPN
525 stub RtlUpcaseUnicodeToMultiByteN
526 stub RtlUpcaseUnicodeToOemN
527 stub RtlUpperChar
528 stub RtlUpperString
529 stub RtlUsageHeap
530 stub RtlValidAcl
531 stub RtlValidSecurityDescriptor
532 stub RtlValidSid
533 stub RtlValidateHeap
534 stub RtlValidateProcessHeaps
535 stub RtlWalkHeap
536 stub RtlWriteRegistryValue
537 stub RtlZeroHeap
538 stdcall RtlZeroMemory(ptr long) RtlZeroMemory
539 stub RtlpInitializeRtl
540 stub RtlpNtCreateKey
541 stub RtlpNtEnumerateSubKey
542 stub RtlpNtMakeTemporaryKey
543 stub RtlpNtOpenKey
544 stub RtlpNtQueryValueKey
545 stub RtlpNtSetValueKey
546 stub RtlpUnWaitCriticalSection
547 stub RtlpWaitForCriticalSection
548 stdcall RtlxAnsiStringToUnicodeSize(ptr) RtlxAnsiStringToUnicodeSize
549 stdcall RtlxOemStringToUnicodeSize(ptr) RtlxOemStringToUnicodeSize
550 stub RtlxUnicodeStringToAnsiSize
551 stub RtlxUnicodeStringToOemSize
552 stub SaveEm87Context
553 stdcall ZwAcceptConnectPort(long long long long long long) NtAcceptConnectPort
554 stub ZwAccessCheck
555 stub ZwAccessCheckAndAuditAlarm
556 stdcall ZwAdjustGroupsToken(long long long long long long) NtAdjustPrivilegesToken
557 stub ZwAdjustPrivilegesToken
558 stub ZwAlertResumeThread
559 stub ZwAlertThread
560 stub ZwAllocateLocallyUniqueId
561 stub ZwAllocateUuids
562 stub ZwAllocateVirtualMemory
563 stub ZwCallbackReturn
564 stub ZwCancelIoFile
565 stub ZwCancelTimer
566 stub ZwClearEvent
567 stub ZwClose
568 stub ZwCloseObjectAuditAlarm
569 stdcall ZwCompleteConnectPort(long) NtCompleteConnectPort
570 stdcall ZwConnectPort(long long long long long long long long) NtConnectPort
571 stub ZwContinue
572 stdcall ZwCreateDirectoryObject(long long long) NtCreateDirectoryObject
573 stdcall ZwCreateEvent(long long long long long) NtCreateEvent
574 stub ZwCreateEventPair
575 stdcall ZwCreateFile(ptr long ptr ptr long long long ptr long long ptr) NtCreateFile
576 stub ZwCreateIoCompletion
577 stdcall ZwCreateKey(long long long long long long long) NtCreateKey
578 stdcall ZwCreateMailslotFile(long long long long long long long long) NtCreateMailslotFile
579 stub ZwCreateMutant
580 stub ZwCreateNamedPipeFile
581 stdcall ZwCreatePagingFile(long long long long) NtCreatePagingFile
582 stdcall ZwCreatePort(long long long long long) NtCreatePort
583 stub ZwCreateProcess
584 stub ZwCreateProfile
585 stdcall ZwCreateSection(long long long long long long long) NtCreateSection
586 stub ZwCreateSemaphore
587 stub ZwCreateSymbolicLinkObject
588 stub ZwCreateThread
589 stdcall ZwCreateTimer(long long long) NtCreateTimer
590 stub ZwCreateToken
591 stub ZwDelayExecution
592 stub ZwDeleteFile
593 stub ZwDeleteKey
594 stub ZwDeleteValueKey
595 stdcall ZwDeviceIoControlFile(long long long long long long long long long long) NtDeviceIoControlFile
596 stub ZwDisplayString
597 stdcall ZwDuplicateObject(long long long long long long long) NtDuplicateObject
598 stdcall ZwDuplicateToken(long long long long long long) NtDuplicateToken
599 stub ZwEnumerateBus
600 stub ZwEnumerateKey
601 stub ZwEnumerateValueKey
602 stub ZwExtendSection
603 stub ZwFlushBuffersFile
604 stub ZwFlushInstructionCache
605 stub ZwFlushKey
606 stub ZwFlushVirtualMemory
607 stub ZwFlushWriteBuffer
608 stub ZwFreeVirtualMemory
609 stdcall ZwFsControlFile(long long long long long long long long long long) NtFsControlFile
610 stub ZwGetContextThread
611 stub ZwGetPlugPlayEvent
612 stub ZwGetTickCount
613 stub ZwImpersonateClientOfPort
614 stub ZwImpersonateThread
615 stub ZwInitializeRegistry
616 stdcall ZwListenPort(long long) NtListenPort
617 stub ZwLoadDriver
618 stub ZwLoadKey
619 stub ZwLockFile
620 stub ZwLockVirtualMemory
621 stub ZwMakeTemporaryObject
622 stdcall ZwMapViewOfSection(long long long long long long long long long long) NtMapViewOfSection
623 stub ZwNotifyChangeDirectoryFile
624 stub ZwNotifyChangeKey
625 stdcall ZwOpenDirectoryObject(long long long) NtOpenDirectoryObject
626 stdcall ZwOpenEvent(long long long) NtOpenEvent
627 stub ZwOpenEventPair
628 stdcall ZwOpenFile(ptr long ptr ptr long long) NtOpenFile
629 stub ZwOpenIoCompletion
630 stdcall ZwOpenKey(ptr long ptr) NtOpenKey
631 stub ZwOpenMutant
632 stub ZwOpenObjectAuditAlarm
633 stub ZwOpenProcess
634 stdcall ZwOpenProcessToken(long long long) NtOpenProcessToken
635 stdcall ZwOpenSection(long long long) NtOpenSection
636 stub ZwOpenSemaphore
637 stub ZwOpenSymbolicLinkObject
638 stub ZwOpenThread
639 stdcall ZwOpenThreadToken(long long long long) NtOpenThreadToken
640 stub ZwOpenTimer
641 stub ZwPlugPlayControl
642 stub ZwPrivilegeCheck
643 stub ZwPrivilegeObjectAuditAlarm
644 stub ZwPrivilegedServiceAuditAlarm
645 stub ZwProtectVirtualMemory
646 stub ZwPulseEvent
647 stub ZwQueryAttributesFile
648 stub ZwQueryDefaultLocale
649 stub ZwQueryDirectoryFile
650 stdcall ZwQueryDirectoryObject(long long long long long long long) NtQueryDirectoryObject
651 stub ZwQueryEaFile
652 stub ZwQueryEvent
653 stdcall ZwQueryInformationFile(long long long long long) NtQueryInformationFile
654 stub ZwQueryInformationPort
655 stdcall ZwQueryInformationProcess(long long long long long) NtQueryInformationProcess
656 stdcall ZwQueryInformationThread(long long long long long) NtQueryInformationThread
657 stdcall ZwQueryInformationToken(long long long long long) NtQueryInformationToken
658 stub ZwQueryIntervalProfile
659 stub ZwQueryIoCompletion
660 stub ZwQueryKey
661 stub ZwQueryMutant
662 stdcall ZwQueryObject(long long long long long) NtQueryObject
663 stub ZwQueryPerformanceCounter
664 stub ZwQuerySection
665 stub ZwQuerySecurityObject
666 stub ZwQuerySemaphore
667 stub ZwQuerySymbolicLinkObject
668 stub ZwQuerySystemEnvironmentValue
669 stdcall ZwQuerySystemInformation(long long long long) NtQuerySystemInformation
670 stdcall ZwQuerySystemTime(ptr) GetSystemTimeAsFileTime
671 stub ZwQueryTimer
672 stub ZwQueryTimerResolution
673 stub ZwQueryValueKey
674 stub ZwQueryVirtualMemory
675 stub ZwQueryVolumeInformationFile
676 stub ZwRaiseException
677 stub ZwRaiseHardError
678 stdcall ZwReadFile(long long long long long long long long long) NtReadFile
679 stub ZwReadRequestData
680 stub ZwReadVirtualMemory
681 stub ZwRegisterNewDevice
682 stdcall ZwRegisterThreadTerminatePort(long) NtRegisterThreadTerminatePort
683 stub ZwReleaseMutant
684 stub ZwReleaseProcessMutant
685 stub ZwReleaseSemaphore
686 stub ZwRemoveIoCompletion
687 stub ZwReplaceKey
688 stub ZwReplyPort
689 stdcall ZwReplyWaitReceivePort(long long long long) NtReplyWaitReceivePort
690 stub ZwReplyWaitReplyPort
691 stub ZwRequestPort
692 stdcall ZwRequestWaitReplyPort(long long long) NtRequestWaitReplyPort
693 stub ZwResetEvent
694 stub ZwRestoreKey
695 stdcall ZwResumeThread(long long) NtResumeThread
696 stub ZwSaveKey
697 stub ZwSetContextThread
698 stub ZwSetDefaultHardErrorPort
699 stub ZwSetDefaultLocale
700 stub ZwSetEaFile
701 stdcall ZwSetEvent(long long) NtSetEvent
702 stub ZwSetHighEventPair
703 stub ZwSetHighWaitLowEventPair
704 stub ZwSetHighWaitLowThread
705 stdcall ZwSetInformationFile(long long long long long) NtSetInformationFile
706 stub ZwSetInformationKey
707 stub ZwSetInformationObject
708 stdcall ZwSetInformationProcess(long long long long) NtSetInformationProcess
709 stdcall ZwSetInformationThread(long long long long) NtSetInformationThread
710 stub ZwSetInformationToken
711 stdcall ZwSetIntervalProfile(long long) NtSetIntervalProfile
712 stub ZwSetIoCompletion
713 stub ZwSetLdtEntries
714 stub ZwSetLowEventPair
715 stub ZwSetLowWaitHighEventPair
716 stub ZwSetLowWaitHighThread
717 stub ZwSetSecurityObject
718 stub ZwSetSystemEnvironmentValue
719 stub ZwSetSystemInformation
720 stub ZwSetSystemPowerState
721 stub ZwSetSystemTime
722 stdcall ZwSetTimer(long long long long long long) NtSetTimer
723 stub ZwSetTimerResolution
724 stdcall ZwSetValueKey(long long long long long long) NtSetValueKey
725 stdcall ZwSetVolumeInformationFile(long long long long long) NtSetVolumeInformationFile
726 stub ZwShutdownSystem
727 stub ZwStartProfile
728 stub ZwStopProfile
729 stub ZwSuspendThread
730 stub ZwSystemDebugControl
731 stub ZwTerminateProcess
732 stdcall ZwTerminateThread(long long) NtTerminateThread
733 stub ZwTestAlert
734 stub ZwUnloadDriver
735 stub ZwUnloadKey
736 stub ZwUnlockFile
737 stub ZwUnlockVirtualMemory
738 stub ZwUnmapViewOfSection
739 stub ZwVdmControl
740 stub ZwW32Call
741 stub ZwWaitForMultipleObjects
742 stub ZwWaitForProcessMutant
743 stdcall ZwWaitForSingleObject(long long long) NtWaitForSingleObject
744 stub ZwWaitHighEventPair
745 stub ZwWaitLowEventPair
746 stub ZwWriteFile
747 stub ZwWriteRequestData
748 stub ZwWriteVirtualMemory
749 stub _CIpow
750 stub __eCommonExceptions
751 stub __eEmulatorInit
752 stub __eF2XM1
753 stub __eFABS
754 stub __eFADD32
755 stub __eFADD64
756 stub __eFADDPreg
757 stub __eFADDreg
758 stub __eFADDtop
759 stub __eFCHS
760 stub __eFCOM
761 stub __eFCOM32
762 stub __eFCOM64
763 stub __eFCOMP
764 stub __eFCOMP32
765 stub __eFCOMP64
766 stub __eFCOMPP
767 stub __eFCOS
768 stub __eFDECSTP
769 stub __eFDIV32
770 stub __eFDIV64
771 stub __eFDIVPreg
772 stub __eFDIVR32
773 stub __eFDIVR64
774 stub __eFDIVRPreg
775 stub __eFDIVRreg
776 stub __eFDIVRtop
777 stub __eFDIVreg
778 stub __eFDIVtop
779 stub __eFFREE
780 stub __eFIADD16
781 stub __eFIADD32
782 stub __eFICOM16
783 stub __eFICOM32
784 stub __eFICOMP16
785 stub __eFICOMP32
786 stub __eFIDIV16
787 stub __eFIDIV32
788 stub __eFIDIVR16
789 stub __eFIDIVR32
790 stub __eFILD16
791 stub __eFILD32
792 stub __eFILD64
793 stub __eFIMUL16
794 stub __eFIMUL32
795 stub __eFINCSTP
796 stub __eFINIT
797 stub __eFIST16
798 stub __eFIST32
799 stub __eFISTP16
800 stub __eFISTP32
801 stub __eFISTP64
802 stub __eFISUB16
803 stub __eFISUB32
804 stub __eFISUBR16
805 stub __eFISUBR32
806 stub __eFLD1
807 stub __eFLD32
808 stub __eFLD64
809 stub __eFLD80
810 stub __eFLDCW
811 stub __eFLDENV
812 stub __eFLDL2E
813 stub __eFLDLN2
814 stub __eFLDPI
815 stub __eFLDZ
816 stub __eFMUL32
817 stub __eFMUL64
818 stub __eFMULPreg
819 stub __eFMULreg
820 stub __eFMULtop
821 stub __eFPATAN
822 stub __eFPREM
823 stub __eFPREM1
824 stub __eFPTAN
825 stub __eFRNDINT
826 stub __eFRSTOR
827 stub __eFSAVE
828 stub __eFSCALE
829 stub __eFSIN
830 stub __eFSQRT
831 stub __eFST
832 stub __eFST32
833 stub __eFST64
834 stub __eFSTCW
835 stub __eFSTENV
836 stub __eFSTP
837 stub __eFSTP32
838 stub __eFSTP64
839 stub __eFSTP80
840 stub __eFSTSW
841 stub __eFSUB32
842 stub __eFSUB64
843 stub __eFSUBPreg
844 stub __eFSUBR32
845 stub __eFSUBR64
846 stub __eFSUBRPreg
847 stub __eFSUBRreg
848 stub __eFSUBRtop
849 stub __eFSUBreg
850 stub __eFSUBtop
851 stub __eFTST
852 stub __eFUCOM
853 stub __eFUCOMP
854 stub __eFUCOMPP
855 stub __eFXAM
856 stub __eFXCH
857 stub __eFXTRACT
858 stub __eFYL2X
859 stub __eFYL2XP1
860 stub __eGetStatusWord
861 register _alloca_probe() NTDLL_alloca_probe
862 register _chkstk() NTDLL_chkstk
863 stub _fltused
864 cdecl _ftol(double) CRTDLL__ftol
865 stub _itoa
866 stub _ltoa
867 stub _memccpy
868 cdecl _memicmp(str str long) CRTDLL__memicmp
869 stub _snprintf
870 stub _snwprintf
871 stub _splitpath
872 cdecl _strcmpi(str str) CRTDLL__strcmpi
873 cdecl _stricmp(str str) CRTDLL__strcmpi
874 stub _strlwr
875 cdecl _strnicmp(str str long) CRTDLL__strnicmp
876 cdecl _strupr(str) CRTDLL__strupr
877 cdecl _ultoa(long ptr long) CRTDLL__ultoa
878 stub _vsnprintf
879 cdecl _wcsicmp(wstr wstr) CRTDLL__wcsicmp
880 cdecl _wcslwr(wstr) CRTDLL__wcslwr
881 cdecl _wcsnicmp(wstr wstr long) CRTDLL__wcsnicmp
882 cdecl _wcsupr(wstr) CRTDLL__wcsupr
883 stub abs
884 stub atan
885 cdecl atoi(str) atoi
886 cdecl atol(str) atol
887 stub ceil
888 stub cos
889 stub fabs
890 stub floor
891 cdecl isalpha(long) isalpha
892 cdecl isdigit(long) isdigit
893 cdecl islower(long) islower
894 cdecl isprint(long) isprint
895 cdecl isspace(long) isspace
896 cdecl isupper(long) isupper
897 stub iswalpha
898 stub iswctype
899 cdecl isxdigit(long) isxdigit
900 stub labs
901 stub log
902 stub mbstowcs
903 cdecl memchr(ptr long long) memchr
904 cdecl memcmp(ptr ptr long) memcmp
905 cdecl memcpy(ptr ptr long) memcpy
906 cdecl memmove(ptr ptr long) memmove
907 cdecl memset(ptr long long) memset
908 stub pow
909 stub qsort
910 stub sin
911 varargs sprintf() wsprintf32A
912 stub sqrt
913 varargs sscanf() sscanf
914 cdecl strcat(str str) strcat
915 cdecl strchr(str long) strchr
916 cdecl strcmp(str str) strcmp
917 cdecl strcpy(ptr str) strcpy
918 cdecl strcspn(str str) strcspn
919 cdecl strlen(str) strlen
920 cdecl strncat(str str long) strncat
921 cdecl strncmp(str str long) strncmp
922 cdecl strncpy(ptr str long) strncpy
923 cdecl strpbrk(str str long) strpbrk
924 cdecl strrchr(str long) strrchr
925 cdecl strspn(str str) strspn
926 cdecl strstr(str str) strstr
927 varargs swprintf() wsprintf32W
928 stub tan
929 cdecl tolower(long) tolower
930 cdecl toupper(long) toupper
931 stub towlower
932 stub towupper
933 cdecl vsprintf(ptr str ptr) CRTDLL_vsprintf
934 cdecl wcscat(wstr wstr) CRTDLL_wcscat
935 cdecl wcschr(wstr long) CRTDLL_wcschr
936 cdecl wcscmp(wstr wstr) CRTDLL_wcscmp
937 cdecl wcscpy(ptr wstr) CRTDLL_wcscpy
938 stub wcscspn
939 cdecl wcslen(wstr) CRTDLL_wcslen
940 stub wcsncat
941 cdecl wcsncmp(wstr wstr long) CRTDLL_wcsncmp
942 cdecl wcsncpy(ptr wstr long) CRTDLL_wcsncpy
943 stub wcspbrk
944 cdecl wcsrchr(wstr long) CRTDLL_wcsrchr
945 cdecl wcsspn(wstr wstr) CRTDLL_wcsspn
946 cdecl wcsstr(wstr wstr) CRTDLL_wcsstr
947 cdecl wcstok(wstr wstr) CRTDLL_wcstok
948 cdecl wcstol(wstr ptr long) CRTDLL_wcstol
949 cdecl wcstombs(ptr ptr long) CRTDLL_wcstombs
950 stub wcstoul

# NT 4 additions
951 stub NtAddAtom
952 stub NtDeleteAtom
953 stub NtFindAtom
954 stub NtQueryFullAttributesFile
955 stub NtQueueApcThread
956 stub NtReadFileScatter
957 stub NtSignalAndWaitForSingleObject
958 stub NtWriteFileGather
959 stub NtYieldExecution
960 stub RtlAddAtomToAtomTable
961 stub RtlAllocateHandle
962 stub RtlCreateAtomTable
963 stub RtlDeleteAtomFromAtomTable
964 stub RtlFreeHandle
965 stub RtlInitializeHandleTable
966 stub RtlIsValidHandle
967 stub RtlLookupAtomInAtomTable
968 stub RtlQueryAtomInAtomTable
969 stdcall RtlTryEnterCriticalSection(ptr) TryEnterCriticalSection
970 stub RtlEnumerateProperties
971 stub RtlSetPropertyClassId
972 stub RtlSetPropertyNames
973 stub RtlQueryPropertyNames
974 stub RtlFlushPropertySet
975 stub RtlSetProperties
976 stub RtlQueryProperties
977 stub RtlQueryPropertySet
978 stub RtlSetUnicodeCallouts
979 stub RtlPropertySetNameToGuid
980 stub RtlGuidToPropertySetName
981 stub RtlClosePropertySet
982 stub RtlCreatePropertySet
983 stub _wtoi
984 stub _wtol
985 stub RtlSetPropertySetClassId
