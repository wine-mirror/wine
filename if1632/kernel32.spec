name	kernel32
type	win32
base	1

# Functions exported by the Win95 kernel32.dll 
# (these need to have these exact ordinals, for some win95 dlls 
#  import kernel32.dll by ordinal)
# the base is NOT included in these ordinals

# undocumented ordinal only calls (names taken from k32exp.h by Andrew
# Schulman.
0  stub VxDCall0
1  stub VxDCall0
2  stub VxDCall0
3  stub VxDCall0
4  stub VxDCall0
5  stub VxDCall0
6  stub VxDCall0
7  stub VxDCall0
8  stub VxDCall0
 
9  stub _KERNEL32_stringconv1 #ansi2oem or reverse?
 
17  stdcall _KERNEL32_18(long long) _KERNEL32_18
18  stub _KERNEL32_getheapsegment
 
30  stub _KERNEL32_31
 
34  stub LoadLibrary16
35  stub FreeLibrary16
36  stub GetProcAddress16
 
39  stub _KERNEL32_40

42  return _KERNEL32_43 20 0
44  stub _KERNEL32_45
 
49  stdcall AddAtomA(ptr) AddAtom32A

51  register _KERNEL32_52(long) _KERNEL32_52

# WOW calls
53  stub WOWCallback16
54  stub WOWCallback16Ex
55  stub WOWGetVDMPointer
56  stub WOWHandle32
57  stub WOWHandle16
58  stub WOWGlobalAlloc16
59  stub WOWGlobalLock16
60  stub WOWGlobalUnlock16
61  stub WOWGlobalFree16
62  stub WOWGlobalAllocLock16
63  stub WOWGlobalUnlockFree16
64  stub WOWGlobalLockSize16
65  stub WOWYield16
66  stub WOWDirectedYield16
67  stub WOWGetVDMPointerFix
68  stub WOWGetVDMPointerUnfix
69  stub WOW32_1
 
71  stub RtlLargeIntegerAdd
72  stub RtlEnlargedIntegerMultiply
73  stub RtlEnlargedUnsignedMultiply
74  stub RtlEnlargedUnsignedDivide
75  stub RtlExtendedLargeIntegerDivide
76  stub RtlExtendedMagicDivide
77  stub RtlExtendedIntegerMultiply
78  stub RtlLargeIntegerShiftLeft
79  stub RtlLargeIntegerShiftRight
80  stub RtlLargeIntegerArithmeticShift
81  stub RtlLargeIntegerNegate
82  stub RtlLargeIntegerSubtract
83  stub RtlConvertLongToLargeInteger
84  stub RtlConvertUlongToLargeInteger

86  stub _KERNEL32_87
87  stub _KERNEL32_88

90  stub _KERNEL32_90
91  stub _KERNEL32_91
92  stdcall GETPWIN16LOCK(ptr) GetPWinLock
96  stub ENTERSYSLEVEL
97  stub LEAVESYSLEVEL
98  stub _KERNEL32_98
99  stub _KERNEL32_99
100 stub _KERNEL32_100


101   stdcall AddAtomW(ptr) AddAtom32W
102   stub AllocConsole
103   stub AllocLSCallback
104   stub AllocSLCallback
105   stdcall AreFileApisANSI() AreFileApisANSI
106   stub BackupRead
107   stub BackupSeek
108   stub BackupWrite
109   stdcall Beep(long long) Beep
110   stub BeginUpdateResourceA
111   stub BeginUpdateResourceW
112   stdcall BuildCommDCBA(ptr ptr) BuildCommDCB32A
113   stdcall BuildCommDCBAndTimeoutsA(ptr ptr ptr) BuildCommDCBAndTimeouts32A
114   stdcall BuildCommDCBAndTimeoutsW(ptr ptr ptr) BuildCommDCBAndTimeouts32W
115   stdcall BuildCommDCBW(ptr ptr) BuildCommDCB32W
116   stub CallNamedPipeA
117   stub CallNamedPipeW
118   stub Callback12
119   stub Callback16
120   stub Callback20
121   stub Callback24
122   stub Callback28
123   stub Callback32
124   stub Callback36
125   stub Callback40
126   stub Callback44
127   stub Callback48
128   stub Callback4
129   stub Callback52
130   stub Callback56
131   stub Callback60
132   stub Callback64
133   stub Callback8
134   stdcall ClearCommBreak(long) ClearCommBreak32
135   stdcall ClearCommError(long ptr ptr) ClearCommError
136   stdcall CloseHandle(long) CloseHandle
137   stub CloseProfileUserMapping
138   stub CloseSystemHandle
139   stub CommConfigDialogA
140   stub CommConfigDialogW
141   stdcall CompareFileTime(ptr ptr) CompareFileTime
142   stdcall CompareStringA(long long ptr long ptr long) CompareString32A
143   stdcall CompareStringW(long long ptr long ptr long) CompareString32W
144   stub ConnectNamedPipe
145   stdcall ContinueDebugEvent(long long long) ContinueDebugEvent
146   stub ConvertDefaultLocale
147   stub ConvertToGlobalHandle
148   stdcall CopyFileA(ptr ptr long) CopyFile32A
149   stdcall CopyFileW(ptr ptr long) CopyFile32W
150   stub CreateConsoleScreenBuffer
151   stdcall CreateDirectoryA(ptr ptr) CreateDirectory32A
152   stdcall CreateDirectoryExA(ptr ptr ptr) CreateDirectoryEx32A
153   stdcall CreateDirectoryExW(ptr ptr ptr) CreateDirectoryEx32W
154   stdcall CreateDirectoryW(ptr ptr) CreateDirectory32W
155   stdcall CreateEventA(ptr long long ptr) CreateEvent32A
156   stdcall CreateEventW(ptr long long ptr) CreateEvent32W
157   stdcall CreateFileA(ptr long long ptr long long long) CreateFile32A
158   stdcall CreateFileMappingA(long ptr long long long ptr) CreateFileMapping32A
159   stdcall CreateFileMappingW(long ptr long long long ptr) CreateFileMapping32W
160   stdcall CreateFileW(ptr long long ptr long long long) CreateFile32W
161   stub CreateIoCompletionPort
162   stub CreateKernelThread
163   stub CreateMailslotA
164   stub CreateMailslotW
165   stdcall CreateMutexA(ptr long ptr) CreateMutex32A
166   stdcall CreateMutexW(ptr long ptr) CreateMutex32W
167   stub CreateNamedPipeA
168   stub CreateNamedPipeW
169   stub CreatePipe
170   stdcall CreateProcessA(ptr ptr ptr ptr long long ptr ptr ptr ptr) CreateProcess32A
171   stub CreateProcessW
172   stub CreateRemoteThread
173   stdcall CreateSemaphoreA(ptr long long ptr) CreateSemaphore32A
174   stdcall CreateSemaphoreW(ptr long long ptr) CreateSemaphore32W
175   stub CreateSocketHandle
176   stub CreateTapePartition
177   stdcall CreateThread(ptr long ptr long long ptr) CreateThread
178   stub CreateToolhelp32Snapshot
179   stub DebugActiveProcess
180   stdcall DebugBreak() DebugBreak32
181   stub DefineDosDeviceA
182   stub DefineDosDeviceW
183   stdcall DeleteAtom(long) DeleteAtom32
184   stdcall DeleteCriticalSection(ptr)	DeleteCriticalSection
185   stdcall DeleteFileA(ptr) DeleteFile32A
186   stdcall DeleteFileW(ptr) DeleteFile32W
187   stub DeviceIoControl
188   stdcall DisableThreadLibraryCalls(long) DisableThreadLibraryCalls
189   stub DisconnectNamedPipe
190   stdcall DosDateTimeToFileTime(long long ptr) DosDateTimeToFileTime
191   stdcall DuplicateHandle(long long long ptr long long long) DuplicateHandle
192   stub EndUpdateResourceA
193   stub EndUpdateResourceW
194   stdcall EnterCriticalSection(ptr)	EnterCriticalSection
195   stub EnumCalendarInfoA
196   stub EnumCalendarInfoW
197   stub EnumDateFormatsA
198   stub EnumDateFormatsW
199   stdcall EnumResourceLanguagesA(long ptr ptr ptr long) THUNK_EnumResourceLanguages32A
200   stdcall EnumResourceLanguagesW(long ptr ptr ptr long) THUNK_EnumResourceLanguages32W
201   stdcall EnumResourceNamesA(long ptr ptr long) THUNK_EnumResourceNames32A
202   stdcall EnumResourceNamesW(long ptr ptr long) THUNK_EnumResourceNames32W
203   stdcall EnumResourceTypesA(long ptr long) THUNK_EnumResourceTypes32A
204   stdcall EnumResourceTypesW(long ptr long) THUNK_EnumResourceTypes32W
205   stdcall EnumSystemCodePagesA(ptr long) THUNK_EnumSystemCodePages32A
206   stdcall EnumSystemCodePagesW(ptr long) THUNK_EnumSystemCodePages32W
207   stdcall EnumSystemLocalesA(ptr long) THUNK_EnumSystemLocales32A
208   stdcall EnumSystemLocalesW(ptr long) THUNK_EnumSystemLocales32W
209   stub EnumTimeFormatsA
210   stub EnumTimeFormatsW
211   stub EraseTape
212   stdcall EscapeCommFunction(long long) EscapeCommFunction32
213   stdcall ExitProcess(long) ExitProcess
214   stub ExitThread
215   stdcall ExpandEnvironmentStringsA(ptr ptr long) ExpandEnvironmentStrings32A
216   stdcall ExpandEnvironmentStringsW(ptr ptr long) ExpandEnvironmentStrings32W
217   stub FT_Exit0
218   stub FT_Exit12
219   stub FT_Exit16
220   stub FT_Exit20
221   stub FT_Exit24
222   stub FT_Exit28
223   stub FT_Exit32
224   stub FT_Exit36
226   stub FT_Exit40
227   stub FT_Exit44
228   stub FT_Exit48
225   stub FT_Exit4
229   stub FT_Exit52
230   stub FT_Exit56
231   stub FT_Exit8
232   stub FT_Prolog
233   stub FT_Thunk
234   stdcall FatalAppExitA(long ptr) FatalAppExit32A
235   stdcall FatalAppExitW(long ptr) FatalAppExit32W
236   stub FatalExit
237   stdcall FileTimeToDosDateTime(ptr ptr ptr) FileTimeToDosDateTime
238   stdcall FileTimeToLocalFileTime(ptr ptr) FileTimeToLocalFileTime
239   stdcall FileTimeToSystemTime(ptr ptr) FileTimeToSystemTime
240   stub FillConsoleOutputAttribute
241   stub FillConsoleOutputCharacterA
242   stub FillConsoleOutputCharacterW
243   stdcall FindAtomA(ptr) FindAtom32A
244   stdcall FindAtomW(ptr) FindAtom32W
246   stub FindCloseChangeNotification
245   stdcall FindClose(long) FindClose32
247   stub FindFirstChangeNotificationA
248   stub FindFirstChangeNotificationW
249   stdcall FindFirstFileA(ptr ptr) FindFirstFile32A
250   stdcall FindFirstFileW(ptr ptr) FindFirstFile32W
251   stub FindNextChangeNotification
252   stdcall FindNextFileA(long ptr) FindNextFile32A
253   stdcall FindNextFileW(long ptr) FindNextFile32W
254   stdcall FindResourceA(long ptr ptr) FindResource32A
255   stdcall FindResourceExA(long ptr ptr long) FindResourceEx32A
256   stdcall FindResourceExW(long ptr ptr long) FindResourceEx32W
257   stdcall FindResourceW(long ptr ptr) FindResource32W
258   stdcall FlushConsoleInputBuffer(long) FlushConsoleInputBuffer
259   stdcall FlushFileBuffers(long) FlushFileBuffers
260   stub FlushInstructionCache
261   stub FlushViewOfFile
262   stub FoldStringA
263   stub FoldStringW
264   stdcall FormatMessageA() WIN32_FormatMessage32A
265   stdcall FormatMessageW() WIN32_FormatMessage32W
266   stub FreeConsole
267   stdcall FreeEnvironmentStringsA(ptr) FreeEnvironmentStrings32A
268   stdcall FreeEnvironmentStringsW(ptr) FreeEnvironmentStrings32W
269   stub FreeLSCallback
271   stub FreeLibraryAndExitThread
270   stdcall FreeLibrary(long) FreeLibrary32
272   stdcall FreeResource(long) FreeResource32
273   stub FreeSLCallback
274   stub GenerateConsoleCtrlEvent
275   stdcall GetACP() GetACP
276   stdcall GetAtomNameA(long ptr long) GetAtomName32A
277   stdcall GetAtomNameW(long ptr long) GetAtomName32W
278   stub GetBinaryType
279   stub GetBinaryTypeA
280   stub GetBinaryTypeW
281   stdcall GetCPInfo(long ptr) GetCPInfo
282   stub GetCommConfig
283   stdcall GetCommMask(long ptr) GetCommMask
284   stub GetCommModemStatus
285   stub GetCommProperties
286   stdcall GetCommState(long ptr) GetCommState32
287   stdcall GetCommTimeouts(long ptr) GetCommTimeouts
288   stdcall GetCommandLineA() GetCommandLine32A
289   stdcall GetCommandLineW() GetCommandLine32W
290   stub GetCompressedFileSizeA
291   stub GetCompressedFileSizeW
292   stdcall GetComputerNameA(ptr ptr) GetComputerName32A
293   stdcall GetComputerNameW(ptr ptr) GetComputerName32W
294   stdcall GetConsoleCP() GetConsoleCP
296   stdcall GetConsoleMode(long ptr) GetConsoleMode
297   stdcall GetConsoleOutputCP() GetConsoleOutputCP
298   stdcall GetConsoleScreenBufferInfo(long ptr) GetConsoleScreenBufferInfo
299   stdcall GetConsoleTitleA(ptr long) GetConsoleTitle32A
300   stdcall GetConsoleTitleW(ptr long) GetConsoleTitle32W
301   stub GetCurrencyFormatA
302   stub GetCurrencyFormatW
303   stdcall GetCurrentDirectoryA(long ptr) GetCurrentDirectory32A
304   stdcall GetCurrentDirectoryW(long ptr) GetCurrentDirectory32W
305   stdcall GetCurrentProcess() GetCurrentProcess
306   stdcall GetCurrentProcessId() GetCurrentProcessId
307   stdcall GetCurrentThread() GetCurrentThread
308   stdcall GetCurrentThreadId() GetCurrentThreadId
309   stub GetDateFormatA
310   stub GetDateFormatW
311   stub GetDaylightFlag
312   stub GetDefaultCommConfigA
313   stub GetDefaultCommConfigW
314   stdcall GetDiskFreeSpaceA(ptr ptr ptr ptr ptr) GetDiskFreeSpace32A
315   stdcall GetDiskFreeSpaceW(ptr ptr ptr ptr ptr) GetDiskFreeSpace32W
316   stdcall GetDriveTypeA(ptr) GetDriveType32A
317   stdcall GetDriveTypeW(ptr) GetDriveType32W
319   stdcall GetEnvironmentStringsA() GetEnvironmentStrings32A
320   stdcall GetEnvironmentStringsW() GetEnvironmentStrings32W
318   stdcall GetEnvironmentStrings() GetEnvironmentStrings32A
321   stdcall GetEnvironmentVariableA(ptr ptr long) GetEnvironmentVariable32A
322   stdcall GetEnvironmentVariableW(ptr ptr long) GetEnvironmentVariable32W
323   stub GetErrorMode
324   stub GetExitCodeProcess
325   stub GetExitCodeThread
326   stdcall GetFileAttributesA(ptr) GetFileAttributes32A
327   stdcall GetFileAttributesW(ptr) GetFileAttributes32W
328   stdcall GetFileInformationByHandle(long ptr) GetFileInformationByHandle
329   stdcall GetFileSize(long ptr) GetFileSize
330   stdcall GetFileTime(long ptr ptr ptr) GetFileTime
331   stdcall GetFileType(long) GetFileType
332   stdcall GetFullPathNameA(ptr long ptr ptr) GetFullPathName32A
333   stdcall GetFullPathNameW(ptr long ptr ptr) GetFullPathName32W
334   stub GetHandleContext
335   stub GetHandleInformation
336   stub GetLSCallbackTarget
337   stub GetLSCallbackTemplate
338   stdcall GetLargestConsoleWindowSize(long) GetLargestConsoleWindowSize
339   stdcall GetLastError() GetLastError
340   stdcall GetLocalTime(ptr) GetLocalTime
341   stdcall GetLocaleInfoA(long long ptr long) GetLocaleInfo32A
342   stdcall GetLocaleInfoW(long long ptr long) GetLocaleInfo32W
343   stdcall GetLogicalDriveStringsA(long ptr) GetLogicalDriveStrings32A
344   stdcall GetLogicalDriveStringsW(long ptr) GetLogicalDriveStrings32W
345   stdcall GetLogicalDrives() GetLogicalDrives
346   stub GetMailslotInfo
347   stdcall GetModuleFileNameA(long ptr long) GetModuleFileName32A
348   stdcall GetModuleFileNameW(long ptr long) GetModuleFileName32W
349   stdcall GetModuleHandleA(ptr) WIN32_GetModuleHandleA
350   stdcall GetModuleHandleW(ptr) WIN32_GetModuleHandleW
351   stub GetNamedPipeHandleStateA
352   stub GetNamedPipeHandleStateW
353   stub GetNamedPipeInfo
354   stub GetNumberFormatA
355   stub GetNumberFormatW
356   stdcall GetNumberOfConsoleInputEvents(long ptr) GetNumberOfConsoleInputEvents
357   stub GetNumberOfConsoleMouseButtons
358   stdcall GetOEMCP() GetOEMCP
359   stub GetOverlappedResult
360   stdcall GetPriorityClass(long) GetPriorityClass
361   stdcall GetPrivateProfileIntA(ptr ptr long ptr) GetPrivateProfileInt32A
362   stdcall GetPrivateProfileIntW(ptr ptr long ptr) GetPrivateProfileInt32W
363   stub GetPrivateProfileSectionA
364   stub GetPrivateProfileSectionNamesA
365   stub GetPrivateProfileSectionNamesW
366   stub GetPrivateProfileSectionW
367   stdcall GetPrivateProfileStringA(ptr ptr ptr ptr long ptr) GetPrivateProfileString32A
368   stdcall GetPrivateProfileStringW(ptr ptr ptr ptr long ptr) GetPrivateProfileString32W
369   stub GetPrivateProfileStructA
370   stub GetPrivateProfileStructW
371   stdcall GetProcAddress(long ptr) GetProcAddress32
372   stdcall GetProcessAffinityMask(long ptr ptr)	GetProcessAffinityMask
373   stub GetProcessFlags
374   stdcall GetProcessHeap() GetProcessHeap
375   stub GetProcessHeaps
376   stub GetProcessShutdownParameters
377   stdcall GetProcessTimes(long ptr ptr ptr ptr) GetProcessTimes
378   stdcall GetProcessVersion(long) GetProcessVersion
379   stub GetProcessWorkingSetSize
380   stub GetProductName
381   stdcall GetProfileIntA(ptr ptr long) GetProfileInt32A
382   stdcall GetProfileIntW(ptr ptr long) GetProfileInt32W
383   stub GetProfileSectionA
384   stub GetProfileSectionW
385   stdcall GetProfileStringA(ptr ptr ptr ptr long) GetProfileString32A
386   stdcall GetProfileStringW(ptr ptr ptr ptr long) GetProfileString32W
387   stub GetQueuedCompletionStatus
388   stub GetSLCallbackTarget
389   stub GetSLCallbackTemplate
390   stdcall GetShortPathNameA(ptr ptr long) GetShortPathName32A
391   stdcall GetShortPathNameW(ptr ptr long) GetShortPathName32W
392   stdcall GetStartupInfoA(ptr) GetStartupInfo32A
393   stdcall GetStartupInfoW(ptr) GetStartupInfo32W
394   stdcall GetStdHandle(long)	GetStdHandle
395   stdcall GetStringTypeA(long long ptr long ptr) GetStringType32A
396   stdcall GetStringTypeExA(long long ptr long ptr) GetStringTypeEx32A
397   stdcall GetStringTypeExW(long long ptr long ptr) GetStringTypeEx32W
398   stdcall GetStringTypeW(long ptr long ptr) GetStringType32W
399   stdcall GetSystemDefaultLCID() GetSystemDefaultLCID
400   stdcall GetSystemDefaultLangID() GetSystemDefaultLangID
401   stdcall GetSystemDirectoryA(ptr long) GetSystemDirectory32A
402   stdcall GetSystemDirectoryW(ptr long) GetSystemDirectory32W
403   stdcall GetSystemInfo(ptr) GetSystemInfo
404   stdcall GetSystemPowerStatus(ptr) GetSystemPowerStatus
405   stdcall GetSystemTime(ptr) GetSystemTime
406   stub GetSystemTimeAdjustment
407   stub GetSystemTimeAsFileTime
408   stub GetTapeParameters
409   stub GetTapePosition
410   stub GetTapeStatus
411   stdcall GetTempFileNameA(ptr ptr long ptr) GetTempFileName32A
412   stdcall GetTempFileNameW(ptr ptr long ptr) GetTempFileName32W
413   stdcall GetTempPathA(long ptr) GetTempPath32A
414   stdcall GetTempPathW(long ptr) GetTempPath32W
415   stdcall GetThreadContext(long ptr) GetThreadContext
416   stdcall GetThreadLocale() GetThreadLocale
417   stdcall GetThreadPriority(long) GetThreadPriority
418   stub GetThreadSelectorEntry
419   stub GetThreadTimes
420   stdcall GetTickCount() GetTickCount
421   stub GetTimeFormatA
422   stub GetTimeFormatW
423   stdcall GetTimeZoneInformation(ptr) GetTimeZoneInformation
424   stdcall GetUserDefaultLCID() GetUserDefaultLCID
425   stdcall GetUserDefaultLangID() GetUserDefaultLangID
426   stdcall GetVersion() GetVersion32
427   stdcall GetVersionExA(ptr) GetVersionEx32A
428   stdcall GetVersionExW(ptr) GetVersionEx32W
429   stdcall GetVolumeInformationA(ptr ptr long ptr ptr ptr ptr long) GetVolumeInformation32A
430   stdcall GetVolumeInformationW(ptr ptr long ptr ptr ptr ptr long) GetVolumeInformation32W
431   stdcall GetWindowsDirectoryA(ptr long) GetWindowsDirectory32A
432   stdcall GetWindowsDirectoryW(ptr long) GetWindowsDirectory32W
433   stdcall GlobalAddAtomA(ptr) GlobalAddAtom32A
434   stdcall GlobalAddAtomW(ptr) GlobalAddAtom32W
435   stdcall GlobalAlloc(long long) GlobalAlloc32
436   stdcall GlobalCompact(long) GlobalCompact32
437   stdcall GlobalDeleteAtom(long) GlobalDeleteAtom
438   stdcall GlobalFindAtomA(ptr) GlobalFindAtom32A
439   stdcall GlobalFindAtomW(ptr) GlobalFindAtom32W
440   stdcall GlobalFix(long) GlobalFix32
441   stdcall GlobalFlags(long) GlobalFlags32
442   stdcall GlobalFree(long) GlobalFree32
443   stdcall GlobalGetAtomNameA(long ptr long) GlobalGetAtomName32A
444   stdcall GlobalGetAtomNameW(long ptr long) GlobalGetAtomName32W
445   stdcall GlobalHandle(ptr) GlobalHandle32
446   stdcall GlobalLock(long) GlobalLock32
447   stdcall GlobalMemoryStatus(ptr) GlobalMemoryStatus
448   stdcall GlobalReAlloc(long long long) GlobalReAlloc32
449   stdcall GlobalSize(long) GlobalSize32
450   stdcall GlobalUnWire(long) GlobalUnWire32
451   stdcall GlobalUnfix(long) GlobalUnfix32
452   stdcall GlobalUnlock(long) GlobalUnlock32
453   stdcall GlobalWire(long) GlobalWire32
454   stub Heap32First
455   stub Heap32ListFirst
456   stub Heap32ListNext
457   stub Heap32Next
458   stdcall HeapAlloc(long long long) HeapAlloc
459   stdcall HeapCompact(long long) HeapCompact
460   stdcall HeapCreate(long long long)	HeapCreate
461   stdcall HeapDestroy(long) HeapDestroy
462   stdcall HeapFree(long long ptr) HeapFree
463   stdcall HeapLock(long) HeapLock
464   stdcall HeapReAlloc(long long ptr long) HeapReAlloc
466   stdcall HeapSize(long long ptr) HeapSize
467   stdcall HeapUnlock(long) HeapUnlock
468   stdcall HeapValidate(long long ptr) HeapValidate
469   stdcall HeapWalk(long ptr) HeapWalk
470   stub InitAtomTable
471   stdcall InitializeCriticalSection(ptr) InitializeCriticalSection
472   stdcall InterlockedDecrement(ptr) InterlockedDecrement
473   stdcall InterlockedExchange(ptr) InterlockedExchange
474   stdcall InterlockedIncrement(ptr) InterlockedIncrement
475   stub InvalidateNLSCache
476   stdcall IsBadCodePtr(ptr long) IsBadCodePtr32
477   stdcall IsBadHugeReadPtr(ptr long) IsBadHugeReadPtr32
478   stdcall IsBadHugeWritePtr(ptr long) IsBadHugeWritePtr32
479   stdcall IsBadReadPtr(ptr long) IsBadReadPtr32
480   stdcall IsBadStringPtrA(ptr long) IsBadStringPtr32A
481   stdcall IsBadStringPtrW(ptr long) IsBadStringPtr32W
482   stdcall IsBadWritePtr(ptr long) IsBadWritePtr32
483   stdcall IsDBCSLeadByte(long) IsDBCSLeadByte32
484   stdcall IsDBCSLeadByteEx(long long) IsDBCSLeadByteEx
485   stub IsLSCallback
486   stub IsSLCallback
487   stdcall IsValidCodePage(long) IsValidCodePage
488   stdcall IsValidLocale(long long) IsValidLocale
489   stub K32Thk1632Epilog
490   stub K32Thk1632Prolog
491   stub LCMapStringA
492   stub LCMapStringW
493   stdcall LeaveCriticalSection(ptr)	LeaveCriticalSection
494   stdcall LoadLibraryA(ptr) LoadLibrary32A
495   stub LoadLibraryExA
496   stub LoadLibraryExW
497   stdcall LoadLibraryW(ptr) LoadLibrary32W
498   stub LoadModule
499   stdcall LoadResource(long long) LoadResource32
500   stdcall LocalAlloc(long long) LocalAlloc32
501   stdcall LocalCompact(long) LocalCompact32
502   stdcall LocalFileTimeToFileTime(ptr ptr) LocalFileTimeToFileTime
503   stdcall LocalFlags(long) LocalFlags32
504   stdcall LocalFree(long) LocalFree32
505   stdcall LocalHandle(ptr) LocalHandle32
506   stdcall LocalLock(long) LocalLock32
507   stdcall LocalReAlloc(long long long) LocalReAlloc32
508   stdcall LocalShrink(long long) LocalShrink32
509   stdcall LocalSize(long) LocalSize32
510   stdcall LocalUnlock(long) LocalUnlock32
511   stdcall LockFile(long long long long long) LockFile
512   stub LockFileEx
513   stdcall LockResource(long) LockResource32
514   stdcall MakeCriticalSectionGlobal(ptr) MakeCriticalSectionGlobal
515   stub MapHInstLS
516   stub MapHInstLS_PN
517   stub MapHInstSL
518   stub MapHInstSL_PN
519   stub MapHModuleLS
520   stub MapHModuleSL
521   stdcall MapLS(ptr) MapLS
643   stdcall MapSL(long) MapSL
523   stub MapSLFix
522   stub MapSL
524   stdcall MapViewOfFile(long long long long long) MapViewOfFile
525   stdcall MapViewOfFileEx(long long long long long ptr) MapViewOfFileEx
526   stub Module32First
527   stub Module32Next
528   stdcall MoveFileA(ptr ptr) MoveFile32A
529   stdcall MoveFileExA(ptr ptr long) MoveFileEx32A
530   stdcall MoveFileExW(ptr ptr long) MoveFileEx32W
531   stdcall MoveFileW(ptr ptr) MoveFile32W
532   stdcall MulDiv(long long long) MulDiv32
533   stdcall MultiByteToWideChar(long long ptr long ptr long) MultiByteToWideChar
535   stdcall OpenEventA(long long ptr) OpenEvent32A
536   stdcall OpenEventW(long long ptr) OpenEvent32W
537   stdcall OpenFile(ptr ptr long) OpenFile32
538   stdcall OpenFileMappingA(long long ptr) OpenFileMapping32A
539   stdcall OpenFileMappingW(long long ptr) OpenFileMapping32W
540   stdcall OpenMutexA(long long ptr) OpenMutex32A
541   stdcall OpenMutexW(long long ptr) OpenMutex32W
542   stub OpenProcess
543   stub OpenProfileUserMapping
544   stdcall OpenSemaphoreA(long long ptr) OpenSemaphore32A
545   stdcall OpenSemaphoreW(long long ptr) OpenSemaphore32W
546   stub OpenVxDHandle
547   stdcall OutputDebugStringA(ptr) OutputDebugString32A
548   stdcall OutputDebugStringW(ptr) OutputDebugString32W
549   stub PeekConsoleInputA
550   stub PeekConsoleInputW
551   stub PeekNamedPipe
552   stub PostQueuedCompletionStatus
553   stub PrepareTape
554   stub Process32First
555   stub Process32Next
556   stub PulseEvent
557   stub PurgeComm
558   stub QT_Thunk
559   stdcall QueryDosDeviceA(ptr ptr long) QueryDosDevice32A
560   stdcall QueryDosDeviceW(ptr ptr long) QueryDosDevice32W
561   stub QueryNumberOfEventLogRecords
562   stub QueryOldestEventLogRecord
563   stdcall QueryPerformanceCounter(ptr) QueryPerformanceCounter
564   stub QueryPerformanceFrequency
565   stub QueueUserAPC
566   register RaiseException(long long long ptr) RaiseException
567   stdcall ReadConsoleA(long ptr long ptr ptr) ReadConsole32A
568   stub ReadConsoleInputA
569   stub ReadConsoleInputW
570   stub ReadConsoleOutputA
571   stub ReadConsoleOutputAttribute
572   stub ReadConsoleOutputCharacterA
573   stub ReadConsoleOutputCharacterW
574   stub ReadConsoleOutputW
575   stdcall ReadConsoleW(long ptr long ptr ptr) ReadConsole32W
576   stdcall ReadFile(long ptr long ptr ptr) ReadFile
577   stub ReadFileEx
578   stub ReadProcessMemory
579   stub RegisterServiceProcess
580   stdcall ReinitializeCriticalSection(ptr) ReinitializeCriticalSection
581   stdcall ReleaseMutex(long) ReleaseMutex
582   stdcall ReleaseSemaphore(long long ptr) ReleaseSemaphore
583   stdcall RemoveDirectoryA(ptr) RemoveDirectory32A
584   stdcall RemoveDirectoryW(ptr) RemoveDirectory32W
585   stdcall ResetEvent(long) ResetEvent
586   stub ResumeThread
587   stdcall RtlFillMemory(ptr long long) RtlFillMemory
588   stdcall RtlMoveMemory(ptr ptr long) RtlMoveMemory
589   register RtlUnwind(ptr long ptr long) RtlUnwind
590   stdcall RtlZeroMemory(ptr long) RtlZeroMemory
591   stub SMapLS
592   stub SMapLS_IP_EBP_12
593   stub SMapLS_IP_EBP_16
594   stub SMapLS_IP_EBP_20
595   stub SMapLS_IP_EBP_24
596   stub SMapLS_IP_EBP_28
597   stub SMapLS_IP_EBP_32
598   stub SMapLS_IP_EBP_36
599   stub SMapLS_IP_EBP_40
600   stub SMapLS_IP_EBP_8
601   stub SUnMapLS
602   stub SUnMapLS_IP_EBP_12
603   stub SUnMapLS_IP_EBP_16
604   stub SUnMapLS_IP_EBP_20
605   stub SUnMapLS_IP_EBP_24
606   stub SUnMapLS_IP_EBP_28
607   stub SUnMapLS_IP_EBP_32
608   stub SUnMapLS_IP_EBP_36
609   stub SUnMapLS_IP_EBP_40
610   stub SUnMapLS_IP_EBP_8
611   stub ScrollConsoleScreenBufferA
612   stub ScrollConsoleScreenBufferW
613   stdcall SearchPathA(ptr ptr ptr long ptr ptr) SearchPath32A
614   stdcall SearchPathW(ptr ptr ptr long ptr ptr) SearchPath32W
615   stdcall SetCommBreak(long) SetCommBreak32
616   stub SetCommConfig
617   stdcall SetCommMask(long ptr) SetCommMask
618   stdcall SetCommState(long ptr) SetCommState32
619   stdcall SetCommTimeouts(long ptr) SetCommTimeouts
620   stub SetComputerNameA
621   stub SetComputerNameW
622   stub SetConsoleActiveScreenBuffer
623   stub SetConsoleCP
624   stdcall SetConsoleCtrlHandler(ptr long) SetConsoleCtrlHandler
625   stub SetConsoleCursorInfo
626   stdcall SetConsoleCursorPosition(long long) SetConsoleCursorPosition
627   stdcall SetConsoleMode(long long) SetConsoleMode
628   stub SetConsoleOutputCP
629   stub SetConsoleScreenBufferSize
630   stub SetConsoleTextAttribute
631   stdcall SetConsoleTitleA(ptr) SetConsoleTitle32A
632   stdcall SetConsoleTitleW(ptr) SetConsoleTitle32W
633   stub SetConsoleWindowInfo
634   stdcall SetCurrentDirectoryA(ptr) SetCurrentDirectory32A
635   stdcall SetCurrentDirectoryW(ptr) SetCurrentDirectory32W
636   stub SetDaylightFlag
637   stub SetDefaultCommConfigA
638   stub SetDefaultCommConfigW
639   stdcall SetEndOfFile(long) SetEndOfFile
640   stdcall SetEnvironmentVariableA(ptr ptr) SetEnvironmentVariable32A
641   stdcall SetEnvironmentVariableW(ptr ptr) SetEnvironmentVariable32W
642   stdcall SetErrorMode(long) SetErrorMode32
643   stdcall	SetEvent(long) SetEvent
644   stdcall SetFileApisToANSI() SetFileApisToANSI
645   stdcall SetFileApisToOEM() SetFileApisToOEM
646   stdcall SetFileAttributesA(ptr long) SetFileAttributes32A
647   stdcall SetFileAttributesW(ptr long) SetFileAttributes32W
648   stdcall SetFilePointer(long long ptr long) SetFilePointer
649   stdcall SetFileTime(long ptr ptr ptr) SetFileTime
650   stub SetHandleContext
651   stdcall SetHandleCount(long) SetHandleCount32
652   stub SetHandleInformation
653   stdcall SetLastError(long) SetLastError
654   stub SetLocalTime
655   stdcall SetLocaleInfoA(long long ptr) SetLocaleInfoA
656   stub SetLocaleInfoW
657   stub SetMailslotInfo
658   stub SetNamedPipeHandleState
659   stdcall SetPriorityClass(long long) SetPriorityClass
660   stub SetProcessShutdownParameters
661   stub SetProcessWorkingSetSize
662   stdcall SetStdHandle(long long) SetStdHandle
663   stdcall SetSystemPowerState(long long) SetSystemPowerState
664   stdcall SetSystemTime(ptr) SetSystemTime
665   stub SetSystemTimeAdjustment
666   stub SetTapeParameters
667   stub SetTapePosition
668   stdcall SetThreadAffinityMask(long long)	SetThreadAffinityMask
669   stub SetThreadContext
670   stub SetThreadLocale
671   stdcall SetThreadPriority(long long) SetThreadPriority
672   stdcall SetTimeZoneInformation(ptr) SetTimeZoneInformation
673   stdcall SetUnhandledExceptionFilter(ptr) THUNK_SetUnhandledExceptionFilter
674   stub SetVolumeLabelA
675   stub SetVolumeLabelW
676   stub SetupComm
677   stdcall SizeofResource(long long) SizeofResource32
678   stdcall Sleep(long) Sleep
679   stub SleepEx
680   stub SuspendThread
681   stdcall SystemTimeToFileTime(ptr ptr) SystemTimeToFileTime
682   stub SystemTimeToTzSpecificLocalTime
683   stub TerminateProcess
684   stub TerminateThread
685   stub Thread32First
686   stub Thread32Next
687   stdcall ThunkConnect32(ptr ptr ptr ptr ptr ptr) ThunkConnect32
688   stdcall TlsAlloc()	TlsAlloc
690   stdcall TlsFree(long) TlsFree
691   stub TlsFreeInternal
692   stdcall TlsGetValue(long) TlsGetValue
693   stdcall TlsSetValue(long ptr) TlsSetValue
694   stub Toolhelp32ReadProcessMemory
695   stub TransactNamedPipe
696   stdcall TransmitCommChar(long long) TransmitCommChar32
697   stub UTRegister
698   stub UTUnRegister
699   stdcall UnMapLS(long) UnMapLS
700   stub UnMapSLFixArray
701   stdcall UnhandledExceptionFilter(ptr) UnhandledExceptionFilter
702   stub UninitializeCriticalSection
703   stdcall UnlockFile(long long long long long) UnlockFile
704   stub UnlockFileEx
705   stdcall UnmapViewOfFile(ptr) UnmapViewOfFile
706   stub UpdateResourceA
707   stub UpdateResourceW
708   stub VerLanguageNameA
709   stub VerLanguageNameW
710   stdcall VirtualAlloc(ptr long long long) VirtualAlloc
711   stdcall VirtualFree(ptr long long) VirtualFree
712   stdcall VirtualLock(ptr long) VirtualLock
713   stdcall VirtualProtect(ptr long long ptr) VirtualProtect
714   stdcall VirtualProtectEx(long ptr long long ptr) VirtualProtectEx
715   stdcall VirtualQuery(ptr ptr long) VirtualQuery
716   stdcall VirtualQueryEx(long ptr ptr long) VirtualQueryEx
717   stdcall VirtualUnlock(ptr long) VirtualUnlock
718   stub WaitCommEvent
719   stub WaitForDebugEvent
720   stub WaitForMultipleObjects
721   stub WaitForMultipleObjectsEx
722   stdcall WaitForSingleObject(long long) WaitForSingleObject
723   stub WaitForSingleObjectEx
724   stub WaitNamedPipeA
725   stub WaitNamedPipeW
726   stdcall WideCharToMultiByte(long long ptr long ptr long ptr ptr)	WideCharToMultiByte
727   stdcall WinExec(ptr long) WinExec32
728   stdcall WriteConsoleA(long ptr long ptr ptr) WriteConsole32A
729   stub WriteConsoleInputA
730   stub WriteConsoleInputW
731   stub WriteConsoleOutputA
732   stub WriteConsoleOutputAttribute
733   stub WriteConsoleOutputCharacterA
734   stub WriteConsoleOutputCharacterW
735   stub WriteConsoleOutputW
736   stdcall WriteConsoleW(long ptr long ptr ptr) WriteConsole32W
737   stdcall WriteFile(long ptr long ptr ptr) WriteFile
738   stub WriteFileEx
739   stub WritePrivateProfileSectionA
740   stub WritePrivateProfileSectionW
741   stdcall WritePrivateProfileStringA(ptr ptr ptr ptr) WritePrivateProfileString32A
742   stdcall WritePrivateProfileStringW(ptr ptr ptr ptr) WritePrivateProfileString32W
743   stub WritePrivateProfileStructA
744   stub WritePrivateProfileStructW
745   stub WriteProcessMemory
746   stub WriteProfileSectionA
747   stub WriteProfileSectionW
748   stdcall WriteProfileStringA(ptr ptr ptr) WriteProfileString32A
749   stdcall WriteProfileStringW(ptr ptr ptr) WriteProfileString32W
750   stub WriteTapemark
751   stub _DebugOut
752   stub _DebugPrintf
753   stdcall _hread(long ptr long) _hread32
754   stdcall _hwrite(long ptr long) _hwrite32
755   stdcall _lclose(long) _lclose32
756   stdcall _lcreat(ptr long) _lcreat32
757   stdcall _llseek(long long long) _llseek32
758   stdcall _lopen(ptr long) _lopen32
759   stdcall _lread(long ptr long) _lread32
760   stdcall _lwrite(long ptr long) _lwrite32
761   stub dprintf
762   stdcall lstrcat(ptr ptr) lstrcat32A
763   stdcall lstrcatA(ptr ptr) lstrcat32A
764   stdcall lstrcatW(ptr ptr) lstrcat32W
765   stdcall lstrcmp(ptr ptr) lstrcmp32A
766   stdcall lstrcmpA(ptr ptr) lstrcmp32A
767   stdcall lstrcmpW(ptr ptr) lstrcmp32W
768   stdcall lstrcmpi(ptr ptr) lstrcmpi32A
769   stdcall lstrcmpiA(ptr ptr) lstrcmpi32A
770   stdcall lstrcmpiW(ptr ptr) lstrcmpi32W
771   stdcall lstrcpy(ptr ptr) lstrcpy32A
772   stdcall lstrcpyA(ptr ptr) lstrcpy32A
773   stdcall lstrcpyW(ptr ptr) lstrcpy32W
774   stdcall lstrcpyn(ptr ptr long) lstrcpyn32A
775   stdcall lstrcpynA(ptr ptr long) lstrcpyn32A
776   stdcall lstrcpynW(ptr ptr long) lstrcpyn32W
777   stdcall lstrlen(ptr) lstrlen32A
778   stdcall lstrlenA(ptr) lstrlen32A
779   stdcall lstrlenW(ptr) lstrlen32W
# 
# Functions exported by kernel32.dll in NT 3.51
# 
780   stub AddConsoleAliasA
781   stub AddConsoleAliasW
782   stub BaseAttachCompleteThunk
783   stub BasepDebugDump
784   stub CloseConsoleHandle
785   stub CmdBatNotification
786   stub ConsoleMenuControl
787   stub ConsoleSubst
788   stub CreateVirtualBuffer
789   stub ExitVDM
790   stub ExpungeConsoleCommandHistoryA
791   stub ExpungeConsoleCommandHistoryW
792   stub ExtendVirtualBuffer
793   stub FreeVirtualBuffer
794   stub GetConsoleAliasA
795   stub GetConsoleAliasExesA
796   stub GetConsoleAliasExesLengthA
797   stub GetConsoleAliasExesLengthW
798   stub GetConsoleAliasExesW
799   stub GetConsoleAliasW
800   stub GetConsoleAliasesA
801   stub GetConsoleAliasesLengthA
802   stub GetConsoleAliasesLengthW
803   stub GetConsoleAliasesW
804   stub GetConsoleCommandHistoryA
805   stub GetConsoleCommandHistoryLengthA
806   stub GetConsoleCommandHistoryLengthW
807   stub GetConsoleCommandHistoryW
808   stub GetConsoleCursorInfo
809   stub GetConsoleCursorInfo
810   stub GetConsoleDisplayMode
811   stub GetConsoleFontInfo
812   stub GetConsoleFontSize
813   stub GetConsoleHardwareState
814   stub GetConsoleInputWaitHandle
815   stub GetCurrentConsoleFont
816   stub GetNextVDMCommand
817   stub GetNumberOfConsoleFonts
818   stub GetVDMCurrentDirectories
819   stub HeapCreateTagsW
820   stub HeapExtend
821   stub HeapQueryTagW
822   stub HeapSetFlags
823   stub HeapSummary
824   stub HeapUsage
825   stub InvalidateConsoleDIBits
826   stub IsDebuggerPresent
827   stub NotifyNLSUserCache
828   stub OpenConsoleW
829   stub QueryWin31IniFilesMappedToRegistry
830   stub RegisterConsoleVDM
831   stub RegisterWaitForInputIdle
832   stub RegisterWowBaseHandlers
833   stub RegisterWowExec
834   stub SetConsoleCommandHistoryMode
835   stub SetConsoleCursor
836   stub SetConsoleDisplayMode
837   stub SetConsoleFont
838   stub SetConsoleHardwareState
839   stub SetConsoleKeyShortcuts
840   stub SetConsoleMaximumWindowSize
841   stub SetConsoleMenuClose
842   stub SetConsoleNumberOfCommandsA
843   stub SetConsoleNumberOfCommandsW
844   stub SetConsolePalette
845   stub SetLastConsoleEventActive
846   stub SetVDMCurrentDirectories
847   stub ShowConsoleCursor
848   stub TrimVirtualBuffer
849   stub VDMConsoleOperation
850   stub VDMOperationStarted
851   stub VerifyConsoleIoHandle
852   stub VirtualBufferExceptionHandler
853   stub WriteConsoleInputVDMA
854   stub WriteConsoleInputVDMW
