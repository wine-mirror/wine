name	kernel32
base	1

0000 stub AddAtomA
0001 stub AddAtomW
0002 stub AddConsoleAliasA
0003 stub AddConsoleAliasW
0004 stub AllocConsole
0005 stub AreFileApisANSI
0006 stub BackupRead
0007 stub BackupSeek
0008 stub BackupWrite
0009 stub BaseAttachCompleteThunk
0010 stub BasepDebugDump
0011 stub Beep
0012 stub BeginUpdateResourceA
0013 stub BeginUpdateResourceW
0014 stub BuildCommDCBA
0015 stub BuildCommDCBAndTimeoutsA
0016 stub BuildCommDCBAndTimeoutsW
0017 stub BuildCommDCBW
0018 stub CallNamedPipeA
0019 stub CallNamedPipeW
0020 stub ClearCommBreak
0021 stub ClearCommError
0022 stub CloseConsoleHandle
0023    stdcall CloseHandle(long) CloseHandle
0024 stub CloseProfileUserMapping
0025 stub CmdBatNotification
0026 stub CommConfigDialogA
0027 stub CommConfigDialogW
0028 stub CompareFileTime
0029 stub CompareStringA
0030 stub CompareStringW
0031 stub ConnectNamedPipe
0032 stub ConsoleMenuControl
0033 stub ConsoleSubst
0034 stub ContinueDebugEvent
0035 stub ConvertDefaultLocale
0036 stub CopyFileA
0037 stub CopyFileW
0038 stub CreateConsoleScreenBuffer
0039 stub CreateDirectoryA
0040 stub CreateDirectoryExA
0041 stub CreateDirectoryExW
0042 stub CreateDirectoryW
0043 	stdcall CreateEventA(ptr long long ptr) CreateEventA
0044 stub CreateEventW
0045   stdcall CreateFileA(ptr long long ptr long long long) CreateFileA
0046 	stdcall CreateFileMappingA(long ptr long long long ptr) CreateFileMapping
0047 stub CreateFileMappingW
0048 stub CreateFileW
0049 stub CreateIoCompletionPort
0050 stub CreateMailslotA
0051 stub CreateMailslotW
0052 	stdcall CreateMutexA(ptr long ptr) CreateMutexA
0053 stub CreateMutexW
0054 stub CreateNamedPipeA
0055 stub CreateNamedPipeW
0056 stub CreatePipe
0057 stub CreateProcessA
0058 stub CreateProcessW
0059 stub CreateRemoteThread
0060 stub CreateSemaphoreA
0061 stub CreateSemaphoreW
0062 stub CreateTapePartition
0063 stub CreateThread
0064 stub CreateVirtualBuffer
0065 stub DebugActiveProcess
0066 stub DebugBreak
0067 stub DefineDosDeviceA
0068 stub DefineDosDeviceW
0069 stub DeleteAtom
0070 stub DeleteCriticalSection
0071 stub DeleteFileA
0072 stub DeleteFileW
0073 stub DeviceIoControl
0074 stub DisableThreadLibraryCalls
0075 stub DisconnectNamedPipe
0076 stub DosDateTimeToFileTime
0077 stub DuplicateConsoleHandle
0078 	stdcall DuplicateHandle(long long long ptr long long long) DuplicateHandle
0079 stub EndUpdateResourceA
0080 stub EndUpdateResourceW
0081 stub EnterCriticalSection
0082 stub EnumCalendarInfoA
0083 stub EnumCalendarInfoW
0084 stub EnumDateFormatsA
0085 stub EnumDateFormatsW
0086 stub EnumResourceLanguagesA
0087 stub EnumResourceLanguagesW
0088 stub EnumResourceNamesA
0089 stub EnumResourceNamesW
0090 stub EnumResourceTypesA
0091 stub EnumResourceTypesW
0092 stub EnumSystemCodePagesA
0093 stub EnumSystemCodePagesW
0094 stub EnumSystemLocalesA
0095 stub EnumSystemLocalesW
0096 stub EnumTimeFormatsA
0097 stub EnumTimeFormatsW
0098 stub EraseTape
0099 stub EscapeCommFunction
0100   stdcall ExitProcess(long) ExitProcess
0101 stub ExitThread
0102 stub ExitVDM
0103 stub ExpandEnvironmentStringsA
0104 stub ExpandEnvironmentStringsW
0105 stub ExpungeConsoleCommandHistoryA
0106 stub ExpungeConsoleCommandHistoryW
0107 stub ExtendVirtualBuffer
0108 stub FatalAppExitA
0109 stub FatalAppExitW
0110 stub FatalExit
0111 stub FileTimeToDosDateTime
0112 stub FileTimeToLocalFileTime
0113 stub FileTimeToSystemTime
0114 stub FillConsoleOutputAttribute
0115 stub FillConsoleOutputCharacterA
0116 stub FillConsoleOutputCharacterW
0117 stub FindAtomA
0118 stub FindAtomW
0119 stub FindClose
0120 stub FindCloseChangeNotification
0121 stub FindFirstChangeNotificationA
0122 stub FindFirstChangeNotificationW
0123 stub FindFirstFileA
0124 stub FindFirstFileW
0125 stub FindNextChangeNotification
0126 stub FindNextFileA
0127 stub FindNextFileW
0128 stdcall FindResourceA(long ptr ptr) FindResource32
0129 stub FindResourceExA
0130 stub FindResourceExW
0131 stub FindResourceW
0132 stub FlushConsoleInputBuffer
0133 stub FlushFileBuffers
0134 stub FlushInstructionCache
0135 stub FlushViewOfFile
0136 stub FoldStringA
0137 stub FoldStringW
0138 stub FormatMessageA
0139 stub FormatMessageW
0140 stub FreeConsole
0141 stub FreeEnvironmentStringsA
0142 stub FreeEnvironmentStringsW
0143 stub FreeLibrary
0144 stub FreeLibraryAndExitThread
0145 stdcall FreeResource(long) FreeResource32
0146 stub FreeVirtualBuffer
0147 stub GenerateConsoleCtrlEvent
0148    stdcall GetACP() GetACP
0149 stub GetAtomNameA
0150 stub GetAtomNameW
0151 stub GetBinaryType
0152 stub GetBinaryTypeA
0153 stub GetBinaryTypeW
0154    stdcall GetCPInfo(long ptr) GetCPInfo
0155 stub GetCommConfig
0156 stub GetCommMask
0157 stub GetCommModemStatus
0158 stub GetCommProperties
0159 stub GetCommState
0160 stub GetCommTimeouts
0161	stdcall GetCommandLineA()	GetCommandLineA
0162 stub GetCommandLineW
0163 stub GetCompressedFileSizeA
0164 stub GetCompressedFileSizeW
0165 stub GetComputerNameA
0166 stub GetComputerNameW
0167 stub GetConsoleAliasA
0168 stub GetConsoleAliasExesA
0169 stub GetConsoleAliasExesLengthA
0170 stub GetConsoleAliasExesLengthW
0171 stub GetConsoleAliasExesW
0172 stub GetConsoleAliasW
0173 stub GetConsoleAliasesA
0174 stub GetConsoleAliasesLengthA
0175 stub GetConsoleAliasesLengthW
0176 stub GetConsoleAliasesW
0177 stub GetConsoleCP
0178 stub GetConsoleCommandHistoryA
0179 stub GetConsoleCommandHistoryLengthA
0180 stub GetConsoleCommandHistoryLengthW
0181 stub GetConsoleCommandHistoryW
0182 stub GetConsoleCursorInfo
0183 stub GetConsoleDisplayMode
0184 stub GetConsoleFontInfo
0185 stub GetConsoleFontSize
0186 stub GetConsoleHardwareState
0187 stub GetConsoleInputWaitHandle
0188 stub GetConsoleMode
0189 stub GetConsoleOutputCP
0190 stub GetConsoleScreenBufferInfo
0191 stub GetConsoleTitleA
0192 stub GetConsoleTitleW
0193 stub GetCurrencyFormatA
0194 stub GetCurrencyFormatW
0195 stub GetCurrentConsoleFont
0196 stub GetCurrentDirectoryA
0197 stub GetCurrentDirectoryW
0198 	stdcall GetCurrentProcess() GetCurrentProcess
0199 stdcall GetCurrentProcessId() GetCurrentThreadId
0200 	stdcall GetCurrentThread() GetCurrentThread
0201	stdcall GetCurrentThreadId()	GetCurrentThreadId
0202 stub GetDateFormatA
0203 stub GetDateFormatW
0204 stub GetDefaultCommConfigA
0205 stub GetDefaultCommConfigW
0206 stub GetDiskFreeSpaceA
0207 stub GetDiskFreeSpaceW
0208 stub GetDriveTypeA
0209 stub GetDriveTypeW
0210	stdcall GetEnvironmentStrings()	GetEnvironmentStrings
0211 stub GetEnvironmentStringsA
0212 stub GetEnvironmentStringsW
0213    stdcall GetEnvironmentVariableA(ptr ptr long) GetEnvironmentVariableA
0214 stub GetEnvironmentVariableW
0215 stub GetExitCodeProcess
0216 stub GetExitCodeThread
0217 stub GetFileAttributesA
0218 stub GetFileAttributesW
0219   stdcall GetFileInformationByHandle(long ptr) GetFileInformationByHandle
0220 stub GetFileSize
0221 stub GetFileTime
0222    stdcall GetFileType(long) GetFileType
0223 stub GetFullPathNameA
0224 stub GetFullPathNameW
0225 stub GetHandleInformation
0226 stub GetLargestConsoleWindowSize
0227    stdcall GetLastError() GetLastError
0228    stdcall GetLocalTime(ptr) GetLocalTime
0229 stub GetLocaleInfoA
0230 stub GetLocaleInfoW
0231 stub GetLogicalDriveStringsA
0232 stub GetLogicalDriveStringsW
0233 stub GetLogicalDrives
0234 stub GetMailslotInfo
0235	stdcall GetModuleFileNameA(long ptr long) GetModuleFileNameA
0236 stub GetModuleFileNameW
0237	stdcall GetModuleHandleA(ptr)	WIN32_GetModuleHandle
0238 stub GetModuleHandleW
0239 stub GetNamedPipeHandleStateA
0240 stub GetNamedPipeHandleStateW
0241 stub GetNamedPipeInfo
0242 stub GetNextVDMCommand
0243 stub GetNumberFormatA
0244 stub GetNumberFormatW
0245 stub GetNumberOfConsoleFonts
0246 stub GetNumberOfConsoleInputEvents
0247 stub GetNumberOfConsoleMouseButtons
0248    stdcall GetOEMCP() GetOEMCP
0249 stub GetOverlappedResult
0250 stub GetPriorityClass
0251 stdcall GetPrivateProfileIntA(ptr ptr long ptr) GetPrivateProfileInt
0252 stub GetPrivateProfileIntW
0253 stub GetPrivateProfileSectionA
0254 stub GetPrivateProfileSectionW
0255 stub GetPrivateProfileStringA
0256 stub GetPrivateProfileStringW
0257	stdcall GetProcAddress(long long)	WIN32_GetProcAddress
0258 stub GetProcessAffinityMask
0259	return GetProcessHeap 0 0
0260 stub GetProcessHeaps
0261 stub GetProcessShutdownParameters
0262 stub GetProcessTimes
0263 stub GetProcessWorkingSetSize
0264 	stdcall GetProfileIntA(ptr ptr long) GetProfileInt
0265 stub GetProfileIntW
0266 stub GetProfileSectionA
0267 stub GetProfileSectionW
0268 stub GetProfileStringA
0269 stub GetProfileStringW
0270 stub GetQueuedCompletionStatus
0271 stub GetShortPathNameA
0272 stub GetShortPathNameW
0273	stdcall GetStartupInfoA(ptr) GetStartupInfoA
0274 stub GetStartupInfoW
0275	stdcall GetStdHandle(long)	GetStdHandle
0276 stub GetStringTypeA
0277 stub GetStringTypeExA
0278 stub GetStringTypeExW
0279 stub GetStringTypeW
0280 stub GetSystemDefaultLCID
0281 stub GetSystemDefaultLangID
0282 stub GetSystemDirectoryA
0283 stub GetSystemDirectoryW
0284 stub GetSystemInfo
0285 	stdcall GetSystemTime(ptr) GetSystemTime
0286 stub GetSystemTimeAdjustment
0287 stub GetTapeParameters
0288 stub GetTapePosition
0289 stub GetTapeStatus
0290 stub GetTempFileNameA
0291 stub GetTempFileNameW
0292 stub GetTempPathA
0293 stub GetTempPathW
0294	stdcall GetThreadContext(long ptr)	GetThreadContext
0295 stub GetThreadLocale
0296 stub GetThreadPriority
0297 stub GetThreadSelectorEntry
0298 stub GetThreadTimes
0299    stdcall GetTickCount() GetTickCount
0300 stub GetTimeFormatA
0301 stub GetTimeFormatW
0302    stdcall GetTimeZoneInformation(ptr) GetTimeZoneInformation
0303 stub GetUserDefaultLCID
0304 stub GetUserDefaultLangID
0305 stub GetVDMCurrentDirectories
#Use Win 3.1 GetVersion for now
0306	stdcall GetVersion()	GetVersion
0307 stub GetVersionExA
0308 stub GetVersionExW
0309 stub GetVolumeInformationA
0310 stub GetVolumeInformationW
0311 stub GetWindowsDirectoryA
0312 stub GetWindowsDirectoryW
0313 stub GlobalAddAtomA
0314 stub GlobalAddAtomW
0315	stdcall GlobalAlloc(long long)	GlobalAlloc32
0316 stub GlobalCompact
0317 stub GlobalDeleteAtom
0318 stub GlobalFindAtomA
0319 stub GlobalFindAtomW
0320 stub GlobalFix
0321 stub GlobalFlags
0322 stub GlobalFree
0323 stub GlobalGetAtomNameA
0324 stub GlobalGetAtomNameW
0325 stub GlobalHandle
0326 stub GlobalLock
0327 stub GlobalMemoryStatus
0328 stub GlobalReAlloc
0329 stub GlobalSize
0330 stub GlobalUnWire
0331 stub GlobalUnfix
0332 stub GlobalUnlock
0333 stub GlobalWire
0334 stdcall HeapAlloc(long long long) HeapAlloc
0335 stub HeapCompact
0336 stdcall HeapCreate(long long long)	HeapCreate
0337 stub HeapDestroy
0338 stub HeapFree
0339 stub HeapLock
0340 stub HeapReAlloc
0341 stub HeapSize
0342 stub HeapUnlock
0343 stub HeapValidate
0344 stub HeapWalk
0345 stub InitAtomTable
0346 stub InitializeCriticalSection
0347 stub InterlockedDecrement
0348 stub InterlockedExchange
0349 stub InterlockedIncrement
0350 stub InvalidateConsoleDIBits
0351 stub IsBadCodePtr
0352 stub IsBadHugeReadPtr
0353 stub IsBadHugeWritePtr
0354 stub IsBadReadPtr
0355 stub IsBadStringPtrA
0356 stub IsBadStringPtrW
0357 stub IsBadWritePtr
0358 stub IsDBCSLeadByte
0359 stub IsDBCSLeadByteEx
0360 stub IsValidCodePage
0361 stub IsValidLocale
0362 stub LCMapStringA
0363 stub LCMapStringW
0364 stub LeaveCriticalSection
0365	stdcall LoadLibraryA(long)		LoadLibraryA
0366 stub LoadLibraryExA
0367 stub LoadLibraryExW
0368 stub LoadLibraryW
0369 stub LoadModule
0370 stdcall LoadResource(long long) LoadResource32
0371	stdcall LocalAlloc(long long)	GlobalAlloc32
0372 stub LocalCompact
0373 stub LocalFileTimeToFileTime
0374 stub LocalFlags
0375 stub LocalFree
0376 stub LocalHandle
0377 stub LocalLock
0378 stub LocalReAlloc
0379 stub LocalShrink
0380 stub LocalSize
0381 stub LocalUnlock
0382 stub LockFile
0383 stub LockFileEx
0384 stub LockResource
0385 stub MapViewOfFile
0386 	stdcall MapViewOfFileEx(long long long long long long) MapViewOfFileEx
0387 stub MoveFileA
0388 stub MoveFileExA
0389 stub MoveFileExW
0390 stub MoveFileW
0391 stub MulDiv
0392 stdcall MultiByteToWideChar(long long ptr long ptr long) MultiByteToWideChar
0393 stub OpenConsoleW
0394 stub OpenEventA
0395 stub OpenEventW
0396 stub OpenFile
0397 	stdcall OpenFileMappingA(long long ptr) OpenFileMapping
0398 stub OpenFileMappingW
0399 stub OpenMutexA
0400 stub OpenMutexW
0401 stub OpenProcess
0402 stub OpenProfileUserMapping
0403 stub OpenSemaphoreA
0404 stub OpenSemaphoreW
0405 stub OutputDebugStringA
0406 stub OutputDebugStringW
0407 stub PeekConsoleInputA
0408 stub PeekConsoleInputW
0409 stub PeekNamedPipe
0410 stub PrepareTape
0411 stub PulseEvent
0412 stub PurgeComm
0413 stub QueryDosDeviceA
0414 stub QueryDosDeviceW
0415 stub QueryPerformanceCounter
0416 stub QueryPerformanceFrequency
0417 stub QueryWin31IniFilesMappedToRegistry
0418 stdcall RaiseException(long long long ptr) RaiseException
0419 stub ReadConsoleA
0420 stub ReadConsoleInputA
0421 stub ReadConsoleInputW
0422 stub ReadConsoleOutputA
0423 stub ReadConsoleOutputAttribute
0424 stub ReadConsoleOutputCharacterA
0425 stub ReadConsoleOutputCharacterW
0426 stub ReadConsoleOutputW
0427 stub ReadConsoleW
0428 stdcall ReadFile(long ptr long ptr ptr) ReadFile
0429 stub ReadFileEx
0430 stub ReadProcessMemory
0431 stub RegisterConsoleVDM
0432 stub RegisterWaitForInputIdle
0433 stub RegisterWowBaseHandlers
0434 stub RegisterWowExec
0435 	stdcall ReleaseMutex(long) ReleaseMutex
0436 stub ReleaseSemaphore
0437 stub RemoveDirectoryA
0438 stub RemoveDirectoryW
0439 	stdcall ResetEvent(long) ResetEvent
0440 stub ResumeThread
0441 stub RtlFillMemory
0442 stub RtlMoveMemory
0443 stub RtlUnwind
0444 stub RtlZeroMemory
0445 stub ScrollConsoleScreenBufferA
0446 stub ScrollConsoleScreenBufferW
0447 stub SearchPathA
0448 stub SearchPathW
0449 stub SetCommBreak
0450 stub SetCommConfig
0451 stub SetCommMask
0452 stub SetCommState
0453 stub SetCommTimeouts
0454 stub SetComputerNameA
0455 stub SetComputerNameW
0456 stub SetConsoleActiveScreenBuffer
0457 stub SetConsoleCP
0458 stub SetConsoleCommandHistoryMode
0459 stdcall SetConsoleCtrlHandler(ptr long) SetConsoleCtrlHandler
0460 stub SetConsoleCursor
0461 stub SetConsoleCursorInfo
0462 stub SetConsoleCursorPosition
0463 stub SetConsoleDisplayMode
0464 stub SetConsoleFont
0465 stub SetConsoleHardwareState
0466 stub SetConsoleKeyShortcuts
0467 stub SetConsoleMaximumWindowSize
0468 stub SetConsoleMenuClose
0469 stub SetConsoleMode
0470 stub SetConsoleNumberOfCommandsA
0471 stub SetConsoleNumberOfCommandsW
0472 stub SetConsoleOutputCP
0473 stub SetConsolePalette
0474 stub SetConsoleScreenBufferSize
0475 stub SetConsoleTextAttribute
0476 stub SetConsoleTitleA
0477 stub SetConsoleTitleW
0478 stub SetConsoleWindowInfo
0479 stub SetCurrentDirectoryA
0480 stub SetCurrentDirectoryW
0481 stub SetDefaultCommConfigA
0482 stub SetDefaultCommConfigW
0483 stub SetEndOfFile
0484    stdcall SetEnvironmentVariableA(ptr ptr) SetEnvironmentVariableA
0485 stub SetEnvironmentVariableW
0486 stub SetErrorMode
0487 	stdcall	SetEvent(long) SetEvent
0488 stub SetFileApisToANSI
0489 stub SetFileApisToOEM
0490 stub SetFileAttributesA
0491 stub SetFileAttributesW
0492    stdcall SetFilePointer(long long ptr long) SetFilePointer
0493 stub SetFileTime
0494    stdcall SetHandleCount(long) W32_SetHandleCount
0495 stub SetHandleInformation
0496 stub SetLastConsoleEventActive
0497    stdcall SetLastError(long) SetLastError
0498 stub SetLocalTime
0499 stub SetLocaleInfoA
0500 stub SetLocaleInfoW
0501 stub SetMailslotInfo
0502 stub SetNamedPipeHandleState
0503 stub SetPriorityClass
0504 stub SetProcessShutdownParameters
0505 stub SetProcessWorkingSetSize
0506 stub SetStdHandle
0507 stub SetSystemTime
0508 stub SetSystemTimeAdjustment
0509 stub SetTapeParameters
0510 stub SetTapePosition
0511 stub SetThreadAffinityMask
0512 stub SetThreadContext
0513 stub SetThreadLocale
0514 stub SetThreadPriority
0515 stub SetTimeZoneInformation
0516 stub SetUnhandledExceptionFilter
0517 stub SetVDMCurrentDirectories
0518 stub SetVolumeLabelA
0519 stub SetVolumeLabelW
0520 stub SetupComm
0521 stub ShowConsoleCursor
0522 stub SizeofResource
0523 	stdcall Sleep(long) Sleep
0524 stub SleepEx
0525 stub SuspendThread
0526 stub SystemTimeToFileTime
0527 stub SystemTimeToTzSpecificLocalTime
0528 stub TerminateProcess
0529 stub TerminateThread
0530 stub TlsAlloc
0531 stub TlsFree
0532 stub TlsGetValue
0533 stub TlsSetValue
0534 stub TransactNamedPipe
0535 stub TransmitCommChar
0536 stub TrimVirtualBuffer
0537 stub UnhandledExceptionFilter
0538 stub UnlockFile
0539 stub UnlockFileEx
0540 stub UnmapViewOfFile
0541 stub UpdateResourceA
0542 stub UpdateResourceW
0543 stub VDMConsoleOperation
0544 stub VDMOperationStarted
0545 stub VerLanguageNameA
0546 stub VerLanguageNameW
0547 stub VerifyConsoleIoHandle
0548    stdcall VirtualAlloc(ptr long long long) VirtualAlloc
0549 stub VirtualBufferExceptionHandler
0550    stdcall VirtualFree(ptr long long) VirtualFree
0551 stub VirtualLock
0552 stub VirtualProtect
0553 stub VirtualProtectEx
0554 stub VirtualQuery
0555 stub VirtualQueryEx
0556 stub VirtualUnlock
0557 stub WaitCommEvent
0558 stub WaitForDebugEvent
0559 stub WaitForMultipleObjects
0560 stub WaitForMultipleObjectsEx
0561 	stdcall WaitForSingleObject(long long) WaitForSingleObject
0562 stub WaitForSingleObjectEx
0563 stub WaitNamedPipeA
0564 stub WaitNamedPipeW
0565 stub WideCharToMultiByte
0566 stub WinExec
0567 stub WriteConsoleA
0568 stub WriteConsoleInputA
0569 stub WriteConsoleInputVDMA
0570 stub WriteConsoleInputVDMW
0571 stub WriteConsoleInputW
0572 stub WriteConsoleOutputA
0573 stub WriteConsoleOutputAttribute
0574 stub WriteConsoleOutputCharacterA
0575 stub WriteConsoleOutputCharacterW
0576 stub WriteConsoleOutputW
0577 stub WriteConsoleW
0578    stdcall WriteFile(long ptr long ptr ptr) WriteFile
0579 stub WriteFileEx
0580 stub WritePrivateProfileSectionA
0581 stub WritePrivateProfileSectionW
0582 stdcall WritePrivateProfileStringA(ptr ptr ptr ptr)	WritePrivateProfileString
0583 stub WritePrivateProfileStringW
0584 stub WriteProcessMemory
0585 stub WriteProfileSectionA
0586 stub WriteProfileSectionW
0587 stdcall WriteProfileStringA(ptr ptr ptr)	WriteProfileString
0588 stub WriteProfileStringW
0589 stub WriteTapemark
0590 stub _hread
0591 stub _hwrite
0592 stub _lclose
0593 stub _lcreat
0594 stub _llseek
0595 stub _lopen
0596 stub _lread
0597 stub _lwrite
0598 stub lstrcat
0599 stub lstrcatA
0600 stub lstrcatW
0601 stub lstrcmp
0602 stub lstrcmpA
0603 stub lstrcmpW
0604 stub lstrcmpi
0605 stub lstrcmpiA
0606 stub lstrcmpiW
0607 stub lstrcpy
0608 	stdcall lstrcpyA(ptr ptr) strcpy
0609 stub lstrcpyW
0610 stub lstrcpyn
0611 stub lstrcpynA
0612 stub lstrcpynW
0613 stub lstrlen
0614 	stdcall lstrlenA(ptr) strlen
0615 stub lstrlenW
#late additions
0616 stub GetPrivateProfileSectionNamesA
0617 stub GetPrivateProfileSectionNamesW
0618 stub GetPrivateProfileStructA
0619 stub GetPrivateProfileStructW
0620 stub GetProcessVersion
0621 stub GetSystemPowerStatus
0622 stub GetSystemTimeAsFileTime
0623 stub HeapCreateTagsW
0624 stub HeapExtend
0625 stub HeapQueryTagW
0626 stub HeapSummary
0627 stub HeapUsage
0628 stub IsDebuggerPresent
0629 stub PostQueuedCompletionStatus
0630 stub SetSystemPowerState
0631 stub WritePrivateProfileStructA
0632 stub WritePrivateProfileStructW
0633 stub MakeCriticalSectionGlobal
