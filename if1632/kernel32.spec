name	kernel32
type	win32
base	1

0000 stdcall AddAtomA(ptr) AddAtom32A
0001 stdcall AddAtomW(ptr) AddAtom32W
0002 stub AddConsoleAliasA
0003 stub AddConsoleAliasW
0004 stub AllocConsole
0005 stdcall AreFileApisANSI() AreFileApisANSI
0006 stub BackupRead
0007 stub BackupSeek
0008 stub BackupWrite
0009 stub BaseAttachCompleteThunk
0010 stub BasepDebugDump
0011 stdcall Beep(long long) Beep
0012 stub BeginUpdateResourceA
0013 stub BeginUpdateResourceW
0014 stdcall BuildCommDCBA(ptr ptr) BuildCommDCB32A
0015 stdcall BuildCommDCBAndTimeoutsA(ptr ptr ptr) BuildCommDCBAndTimeouts32A
0016 stdcall BuildCommDCBAndTimeoutsW(ptr ptr ptr) BuildCommDCBAndTimeouts32W
0017 stdcall BuildCommDCBW(ptr ptr) BuildCommDCB32W
0018 stub CallNamedPipeA
0019 stub CallNamedPipeW
0020 stdcall ClearCommBreak(long) ClearCommBreak32
0021 stdcall ClearCommError(long ptr ptr) ClearCommError
0022 stub CloseConsoleHandle
0023 stdcall CloseHandle(long) CloseHandle
0024 stub CloseProfileUserMapping
0025 stub CmdBatNotification
0026 stub CommConfigDialogA
0027 stub CommConfigDialogW
0028 stdcall CompareFileTime(ptr ptr) CompareFileTime
0029 stdcall CompareStringA(long long ptr long ptr long) CompareString32A
0030 stdcall CompareStringW(long long ptr long ptr long) CompareString32W
0031 stub ConnectNamedPipe
0032 stub ConsoleMenuControl
0033 stub ConsoleSubst
0034 stub ContinueDebugEvent
0035 stub ConvertDefaultLocale
0036 stdcall CopyFileA(ptr ptr long) CopyFile32A
0037 stdcall CopyFileW(ptr ptr long) CopyFile32W
0038 stub CreateConsoleScreenBuffer
0039 stdcall CreateDirectoryA(ptr ptr) CreateDirectory32A
0040 stdcall CreateDirectoryExA(ptr ptr ptr) CreateDirectoryEx32A
0041 stdcall CreateDirectoryExW(ptr ptr ptr) CreateDirectoryEx32W
0042 stdcall CreateDirectoryW(ptr ptr) CreateDirectory32W
0043 	stdcall CreateEventA(ptr long long ptr) CreateEventA
0044 stub CreateEventW
0045 stdcall CreateFileA(ptr long long ptr long long long) CreateFile32A
0046 stdcall CreateFileMappingA(long ptr long long long ptr) CreateFileMapping32A
0047 stdcall CreateFileMappingW(long ptr long long long ptr) CreateFileMapping32W
0048 stdcall CreateFileW(ptr long long ptr long long long) CreateFile32W
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
0063 stdcall CreateThread(ptr long ptr long long ptr) CreateThread
0064 stub CreateVirtualBuffer
0065 stub DebugActiveProcess
0066 stub DebugBreak
0067 stub DefineDosDeviceA
0068 stub DefineDosDeviceW
0069 stdcall DeleteAtom(long) DeleteAtom32
0070 stdcall DeleteCriticalSection(ptr)	DeleteCriticalSection
0071 stdcall DeleteFileA(ptr) DeleteFile32A
0072 stdcall DeleteFileW(ptr) DeleteFile32W
0073 stub DeviceIoControl
0074 stdcall DisableThreadLibraryCalls(long) DisableThreadLibraryCalls
0075 stub DisconnectNamedPipe
0076 stdcall DosDateTimeToFileTime(long long ptr) DosDateTimeToFileTime
0077 stub DuplicateConsoleHandle
0078 	stdcall DuplicateHandle(long long long ptr long long long) DuplicateHandle
0079 stub EndUpdateResourceA
0080 stub EndUpdateResourceW
0081 stdcall EnterCriticalSection(ptr)	EnterCriticalSection
0082 stub EnumCalendarInfoA
0083 stub EnumCalendarInfoW
0084 stub EnumDateFormatsA
0085 stub EnumDateFormatsW
0086 stdcall EnumResourceLanguagesA(long ptr ptr ptr long) THUNK_EnumResourceLanguages32A
0087 stdcall EnumResourceLanguagesW(long ptr ptr ptr long) THUNK_EnumResourceLanguages32W
0088 stdcall EnumResourceNamesA(long ptr ptr long) THUNK_EnumResourceNames32A
0089 stdcall EnumResourceNamesW(long ptr ptr long) THUNK_EnumResourceNames32W
0090 stdcall EnumResourceTypesA(long ptr long) THUNK_EnumResourceTypes32A
0091 stdcall EnumResourceTypesW(long ptr long) THUNK_EnumResourceTypes32W
0092 stdcall EnumSystemCodePagesA(ptr long) THUNK_EnumSystemCodePages32A
0093 stdcall EnumSystemCodePagesW(ptr long) THUNK_EnumSystemCodePages32W
0094 stdcall EnumSystemLocalesA(ptr long) THUNK_EnumSystemLocales32A
0095 stdcall EnumSystemLocalesW(ptr long) THUNK_EnumSystemLocales32W
0096 stub EnumTimeFormatsA
0097 stub EnumTimeFormatsW
0098 stub EraseTape
0099 stdcall EscapeCommFunction(long long) EscapeCommFunction32
0100 stdcall ExitProcess(long) ExitProcess
0101 stub ExitThread
0102 stub ExitVDM
0103 stdcall ExpandEnvironmentStringsA(ptr ptr long) ExpandEnvironmentStrings32A
0104 stdcall ExpandEnvironmentStringsW(ptr ptr long) ExpandEnvironmentStrings32W
0105 stub ExpungeConsoleCommandHistoryA
0106 stub ExpungeConsoleCommandHistoryW
0107 stub ExtendVirtualBuffer
0108 stdcall FatalAppExitA(long ptr) FatalAppExit32A
0109 stdcall FatalAppExitW(long ptr) FatalAppExit32W
0110 stub FatalExit
0111 stdcall FileTimeToDosDateTime(ptr ptr ptr) FileTimeToDosDateTime
0112 stdcall FileTimeToLocalFileTime(ptr ptr) FileTimeToLocalFileTime
0113 stdcall FileTimeToSystemTime(ptr ptr) FileTimeToSystemTime
0114 stub FillConsoleOutputAttribute
0115 stub FillConsoleOutputCharacterA
0116 stub FillConsoleOutputCharacterW
0117 stdcall FindAtomA(ptr) FindAtom32A
0118 stdcall FindAtomW(ptr) FindAtom32W
0119 stdcall FindClose(long) FindClose32
0120 stub FindCloseChangeNotification
0121 stub FindFirstChangeNotificationA
0122 stub FindFirstChangeNotificationW
0123 stdcall FindFirstFileA(ptr ptr) FindFirstFile32A
0124 stdcall FindFirstFileW(ptr ptr) FindFirstFile32W
0125 stub FindNextChangeNotification
0126 stdcall FindNextFileA(long ptr) FindNextFile32A
0127 stdcall FindNextFileW(long ptr) FindNextFile32W
0128 stdcall FindResourceA(long ptr ptr) FindResource32A
0129 stdcall FindResourceExA(long ptr ptr long) FindResourceEx32A
0130 stdcall FindResourceExW(long ptr ptr long) FindResourceEx32W
0131 stdcall FindResourceW(long ptr ptr) FindResource32W
0132 stub FlushConsoleInputBuffer
0133 stdcall FlushFileBuffers(long) FlushFileBuffers
0134 stub FlushInstructionCache
0135 stub FlushViewOfFile
0136 stub FoldStringA
0137 stub FoldStringW
0138 stdcall FormatMessageA() WIN32_FormatMessage32A
0139 stdcall FormatMessageW() WIN32_FormatMessage32W
0140 stub FreeConsole
0141 stdcall FreeEnvironmentStringsA(ptr) FreeEnvironmentStrings32A
0142 stdcall FreeEnvironmentStringsW(ptr) FreeEnvironmentStrings32W
0143 stdcall FreeLibrary(long) FreeLibrary32
0144 stub FreeLibraryAndExitThread
0145 stdcall FreeResource(long) FreeResource32
0146 stub FreeVirtualBuffer
0147 stub GenerateConsoleCtrlEvent
0148    stdcall GetACP() GetACP
0149 stdcall GetAtomNameA(long ptr long) GetAtomName32A
0150 stdcall GetAtomNameW(long ptr long) GetAtomName32W
0151 stub GetBinaryType
0152 stub GetBinaryTypeA
0153 stub GetBinaryTypeW
0154 stdcall GetCPInfo(long ptr) GetCPInfo
0155 stub GetCommConfig
0156 stdcall GetCommMask(long ptr) GetCommMask
0157 stub GetCommModemStatus
0158 stub GetCommProperties
0159 stdcall GetCommState(long ptr) GetCommState32
0160 stdcall GetCommTimeouts(long ptr) GetCommTimeouts
0161 stdcall GetCommandLineA() GetCommandLine32A
0162 stdcall GetCommandLineW() GetCommandLine32W
0163 stub GetCompressedFileSizeA
0164 stub GetCompressedFileSizeW
0165 stdcall GetComputerNameA(ptr ptr) GetComputerName32A
0166 stdcall GetComputerNameW(ptr ptr) GetComputerName32W
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
0177 stdcall GetConsoleCP() GetConsoleCP
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
0188 stdcall GetConsoleMode(long ptr) GetConsoleMode
0189 stdcall GetConsoleOutputCP() GetConsoleOutputCP
0190 stdcall GetConsoleScreenBufferInfo(long ptr) GetConsoleScreenBufferInfo
0191 stdcall GetConsoleTitleA(ptr long) GetConsoleTitle32A
0192 stdcall GetConsoleTitleW(ptr long) GetConsoleTitle32W
0193 stub GetCurrencyFormatA
0194 stub GetCurrencyFormatW
0195 stub GetCurrentConsoleFont
0196 stdcall GetCurrentDirectoryA(long ptr) GetCurrentDirectory32A
0197 stdcall GetCurrentDirectoryW(long ptr) GetCurrentDirectory32W
0198 stdcall GetCurrentProcess() GetCurrentProcess
0199 stdcall GetCurrentProcessId() GetCurrentProcessId
0200 stdcall GetCurrentThread() GetCurrentThread
0201 stdcall GetCurrentThreadId() GetCurrentThreadId
0202 stub GetDateFormatA
0203 stub GetDateFormatW
0204 stub GetDefaultCommConfigA
0205 stub GetDefaultCommConfigW
0206 	stdcall GetDiskFreeSpaceA(ptr ptr ptr ptr ptr) GetDiskFreeSpace32A
0207 	stdcall GetDiskFreeSpaceW(ptr ptr ptr ptr ptr) GetDiskFreeSpace32W
0208 stdcall GetDriveTypeA(ptr) GetDriveType32A
0209 stdcall GetDriveTypeW(ptr) GetDriveType32W
0210 stdcall GetEnvironmentStrings() GetEnvironmentStrings32A
0211 stdcall GetEnvironmentStringsA() GetEnvironmentStrings32A
0212 stdcall GetEnvironmentStringsW() GetEnvironmentStrings32W
0213 stdcall GetEnvironmentVariableA(ptr ptr long) GetEnvironmentVariable32A
0214 stdcall GetEnvironmentVariableW(ptr ptr long) GetEnvironmentVariable32W
0215 stub GetExitCodeProcess
0216 stub GetExitCodeThread
0217 stdcall GetFileAttributesA(ptr) GetFileAttributes32A
0218 stdcall GetFileAttributesW(ptr) GetFileAttributes32W
0219 stdcall GetFileInformationByHandle(long ptr) GetFileInformationByHandle
0220 stdcall GetFileSize(long ptr ptr) GetFileSize
0221 stdcall GetFileTime(long ptr ptr ptr) GetFileTime
0222 stdcall GetFileType(long) GetFileType
0223 stdcall GetFullPathNameA(ptr long ptr ptr) GetFullPathName32A
0224 stdcall GetFullPathNameW(ptr long ptr ptr) GetFullPathName32W
0225 stub GetHandleInformation
0226 stdcall GetLargestConsoleWindowSize(long) GetLargestConsoleWindowSize
0227 stdcall GetLastError() GetLastError
0228 stdcall GetLocalTime(ptr) GetLocalTime
0229 stdcall GetLocaleInfoA(long long ptr long) GetLocaleInfoA
0230 stdcall GetLocaleInfoW(long long ptr long) GetLocaleInfo32W
0231 stdcall GetLogicalDriveStringsA(long ptr) GetLogicalDriveStrings32A
0232 stdcall GetLogicalDriveStringsW(long ptr) GetLogicalDriveStrings32W
0233 stdcall GetLogicalDrives() GetLogicalDrives
0234 stub GetMailslotInfo
0235 stdcall GetModuleFileNameA(long ptr long) GetModuleFileName32A
0236 stdcall GetModuleFileNameW(long ptr long) GetModuleFileName32W
0237 stdcall GetModuleHandleA(ptr) WIN32_GetModuleHandleA
0238 stdcall GetModuleHandleW(ptr) WIN32_GetModuleHandleW
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
0250 stdcall GetPriorityClass(long) GetPriorityClass
0251 stdcall GetPrivateProfileIntA(ptr ptr long ptr) GetPrivateProfileInt32A
0252 stdcall GetPrivateProfileIntW(ptr ptr long ptr) GetPrivateProfileInt32W
0253 stub GetPrivateProfileSectionA
0254 stub GetPrivateProfileSectionW
0255 stdcall GetPrivateProfileStringA(ptr ptr ptr ptr long ptr) GetPrivateProfileString32A
0256 stdcall GetPrivateProfileStringW(ptr ptr ptr ptr long ptr) GetPrivateProfileString32W
0257 stdcall GetProcAddress(long ptr) GetProcAddress32
0258 stdcall GetProcessAffinityMask(long ptr ptr)	GetProcessAffinityMask
0259 stdcall GetProcessHeap() GetProcessHeap
0260 stub GetProcessHeaps
0261 stub GetProcessShutdownParameters
0262 stub GetProcessTimes
0263 stub GetProcessWorkingSetSize
0264 stdcall GetProfileIntA(ptr ptr long) GetProfileInt32A
0265 stdcall GetProfileIntW(ptr ptr long) GetProfileInt32W
0266 stub GetProfileSectionA
0267 stub GetProfileSectionW
0268 stdcall GetProfileStringA(ptr ptr ptr ptr long) GetProfileString32A
0269 stdcall GetProfileStringW(ptr ptr ptr ptr long) GetProfileString32W
0270 stub GetQueuedCompletionStatus
0271 stdcall GetShortPathNameA(ptr ptr long) GetShortPathName32A
0272 stdcall GetShortPathNameW(ptr ptr long) GetShortPathName32W
0273 stdcall GetStartupInfoA(ptr) GetStartupInfo32A
0274 stdcall GetStartupInfoW(ptr) GetStartupInfo32W
0275 stdcall GetStdHandle(long)	GetStdHandle
0276 stub GetStringTypeA
0277 stub GetStringTypeExA
0278 stub GetStringTypeExW
0279 stub GetStringTypeW
0280 stdcall GetSystemDefaultLCID() GetSystemDefaultLCID
0281 stdcall GetSystemDefaultLangID() GetSystemDefaultLangID
0282 stdcall GetSystemDirectoryA(ptr long) GetSystemDirectory32A
0283 stdcall GetSystemDirectoryW(ptr long) GetSystemDirectory32W
0284 stdcall GetSystemInfo(ptr) GetSystemInfo
0285 	stdcall GetSystemTime(ptr) GetSystemTime
0286 stub GetSystemTimeAdjustment
0287 stub GetTapeParameters
0288 stub GetTapePosition
0289 stub GetTapeStatus
0290 stdcall GetTempFileNameA(ptr ptr long ptr) GetTempFileName32A
0291 stdcall GetTempFileNameW(ptr ptr long ptr) GetTempFileName32W
0292 stdcall GetTempPathA(long ptr) GetTempPath32A
0293 stdcall GetTempPathW(long ptr) GetTempPath32W
0294 stdcall GetThreadContext(long ptr) GetThreadContext
0295 stdcall GetThreadLocale() GetThreadLocale
0296 stdcall GetThreadPriority(long) GetThreadPriority
0297 stub GetThreadSelectorEntry
0298 stub GetThreadTimes
0299 stdcall GetTickCount() GetTickCount
0300 stub GetTimeFormatA
0301 stub GetTimeFormatW
0302    stdcall GetTimeZoneInformation(ptr) GetTimeZoneInformation
0303 stdcall GetUserDefaultLCID() GetUserDefaultLCID
0304 stdcall GetUserDefaultLangID() GetUserDefaultLangID
0305 stub GetVDMCurrentDirectories
0306 stdcall GetVersion() GetVersion32
0307 stdcall GetVersionExA(ptr) GetVersionEx32A
0308 stdcall GetVersionExW(ptr) GetVersionEx32W
0309 stdcall GetVolumeInformationA(ptr ptr long ptr ptr ptr ptr long) GetVolumeInformation32A
0310 stdcall GetVolumeInformationW(ptr ptr long ptr ptr ptr ptr long) GetVolumeInformation32W
0311 stdcall GetWindowsDirectoryA(ptr long) GetWindowsDirectory32A
0312 stdcall GetWindowsDirectoryW(ptr long) GetWindowsDirectory32W
0313 stdcall GlobalAddAtomA(ptr) GlobalAddAtom32A
0314 stdcall GlobalAddAtomW(ptr) GlobalAddAtom32W
0315 stdcall GlobalAlloc(long long) GlobalAlloc32
0316 stdcall GlobalCompact(long) GlobalCompact32
0317 stdcall GlobalDeleteAtom(long) GlobalDeleteAtom
0318 stdcall GlobalFindAtomA(ptr) GlobalFindAtom32A
0319 stdcall GlobalFindAtomW(ptr) GlobalFindAtom32W
0320 stdcall GlobalFix(long) GlobalFix32
0321 stdcall GlobalFlags(long) GlobalFlags32
0322 stdcall GlobalFree(long) GlobalFree32
0323 stdcall GlobalGetAtomNameA(long ptr long) GlobalGetAtomName32A
0324 stdcall GlobalGetAtomNameW(long ptr long) GlobalGetAtomName32W
0325 stdcall GlobalHandle(ptr) GlobalHandle32
0326 stdcall GlobalLock(long) GlobalLock32
0327 stdcall GlobalMemoryStatus(ptr) GlobalMemoryStatus
0328 stdcall GlobalReAlloc(long long long) GlobalReAlloc32
0329 stdcall GlobalSize(long) GlobalSize32
0330 stdcall GlobalUnWire(long) GlobalUnWire32
0331 stdcall GlobalUnfix(long) GlobalUnfix32
0332 stdcall GlobalUnlock(long) GlobalUnlock32
0333 stdcall GlobalWire(long) GlobalWire32
0334 stdcall HeapAlloc(long long long) HeapAlloc
0335 stdcall HeapCompact(long long) HeapCompact
0336 stdcall HeapCreate(long long long)	HeapCreate
0337 stdcall HeapDestroy(long) HeapDestroy
0338 stdcall HeapFree(long long ptr) HeapFree
0339 stdcall HeapLock(long) HeapLock
0340 stdcall HeapReAlloc(long long ptr long) HeapReAlloc
0341 stdcall HeapSize(long long ptr) HeapSize
0342 stdcall HeapUnlock(long) HeapUnlock
0343 stdcall HeapValidate(long long ptr) HeapValidate
0344 stdcall HeapWalk(long ptr) HeapWalk
0345 stub InitAtomTable
0346 stdcall InitializeCriticalSection(ptr) InitializeCriticalSection
0347 stdcall InterlockedDecrement(ptr) InterlockedDecrement
0348 stdcall InterlockedExchange(ptr) InterlockedExchange
0349 stdcall InterlockedIncrement(ptr) InterlockedIncrement
0350 stub InvalidateConsoleDIBits
0351 stdcall IsBadCodePtr(ptr long)	WIN32_IsBadCodePtr
0352 stub IsBadHugeReadPtr
0353 stub IsBadHugeWritePtr
0354 stdcall IsBadReadPtr(ptr long)	WIN32_IsBadReadPtr
0355 stub IsBadStringPtrA
0356 stub IsBadStringPtrW
0357 stdcall IsBadWritePtr(ptr long)	WIN32_IsBadWritePtr
0358 stdcall IsDBCSLeadByte(long) IsDBCSLeadByte32
0359 stdcall IsDBCSLeadByteEx(long long) IsDBCSLeadByteEx
0360 stub IsValidCodePage
0361 stdcall IsValidLocale(long long) IsValidLocale
0362 stub LCMapStringA
0363 stub LCMapStringW
0364 stdcall LeaveCriticalSection(ptr)	LeaveCriticalSection
0365 stdcall LoadLibraryA(ptr) LoadLibrary32A
0366 stub LoadLibraryExA
0367 stub LoadLibraryExW
0368 stdcall LoadLibraryW(ptr) LoadLibrary32W
0369 stub LoadModule
0370 stdcall LoadResource(long long) LoadResource32
0371 stdcall LocalAlloc(long long) LocalAlloc32
0372 stdcall LocalCompact(long) LocalCompact32
0373 stdcall LocalFileTimeToFileTime(ptr ptr) LocalFileTimeToFileTime
0374 stdcall LocalFlags(long) LocalFlags32
0375 stdcall LocalFree(long) LocalFree32
0376 stdcall LocalHandle(ptr) LocalHandle32
0377 stdcall LocalLock(long) LocalLock32
0378 stdcall LocalReAlloc(long long long) LocalReAlloc32
0379 stdcall LocalShrink(long long) LocalShrink32
0380 stdcall LocalSize(long) LocalSize32
0381 stdcall LocalUnlock(long) LocalUnlock32
0382 stdcall LockFile(long long long long long) LockFile
0383 stub LockFileEx
0384 stdcall LockResource(long) LockResource32
0385 stdcall MapViewOfFile(long long long long long) MapViewOfFile
0386 stdcall MapViewOfFileEx(long long long long long long) MapViewOfFileEx
0387 stdcall MoveFileA(ptr ptr) MoveFile32A
0388 stub MoveFileExA
0389 stub MoveFileExW
0390 stdcall MoveFileW(ptr ptr) MoveFile32W
0391 stdcall MulDiv(long long long) MulDiv32
0392 stdcall MultiByteToWideChar(long long ptr long ptr long) MultiByteToWideChar
0393 stub OpenConsoleW
0394 stub OpenEventA
0395 stub OpenEventW
0396 stdcall OpenFile(ptr ptr long) OpenFile32
0397 stdcall OpenFileMappingA(long long ptr) OpenFileMapping
0398 stub OpenFileMappingW
0399 stub OpenMutexA
0400 stub OpenMutexW
0401 stub OpenProcess
0402 stub OpenProfileUserMapping
0403 stub OpenSemaphoreA
0404 stub OpenSemaphoreW
0405 stdcall OutputDebugStringA(ptr) OutputDebugString
0406 stub OutputDebugStringW
0407 stub PeekConsoleInputA
0408 stub PeekConsoleInputW
0409 stub PeekNamedPipe
0410 stub PrepareTape
0411 stub PulseEvent
0412 stub PurgeComm
0413 stdcall QueryDosDeviceA(ptr ptr long) QueryDosDevice32A
0414 stdcall QueryDosDeviceW(ptr ptr long) QueryDosDevice32W
0415 stdcall QueryPerformanceCounter(ptr) QueryPerformanceCounter
0416 stub QueryPerformanceFrequency
0417 stub QueryWin31IniFilesMappedToRegistry
0418 register RaiseException(long long long ptr) RaiseException
0419 stdcall ReadConsoleA(long ptr long ptr ptr) ReadConsole32A
0420 stub ReadConsoleInputA
0421 stub ReadConsoleInputW
0422 stub ReadConsoleOutputA
0423 stub ReadConsoleOutputAttribute
0424 stub ReadConsoleOutputCharacterA
0425 stub ReadConsoleOutputCharacterW
0426 stub ReadConsoleOutputW
0427 stdcall ReadConsoleW(long ptr long ptr ptr) ReadConsole32W
0428 stdcall ReadFile(long ptr long ptr ptr) ReadFile
0429 stub ReadFileEx
0430 stub ReadProcessMemory
0431 stub RegisterConsoleVDM
0432 stub RegisterWaitForInputIdle
0433 stub RegisterWowBaseHandlers
0434 stub RegisterWowExec
0435 	stdcall ReleaseMutex(long) ReleaseMutex
0436 stub ReleaseSemaphore
0437 stdcall RemoveDirectoryA(ptr) RemoveDirectory32A
0438 stdcall RemoveDirectoryW(ptr) RemoveDirectory32W
0439 	stdcall ResetEvent(long) ResetEvent
0440 stub ResumeThread
0441 stdcall RtlFillMemory(ptr long long) RtlFillMemory
0442 stdcall RtlMoveMemory(ptr ptr long) RtlMoveMemory
0443 register RtlUnwind(ptr long ptr long) RtlUnwind
0444 stdcall RtlZeroMemory(ptr long) RtlZeroMemory
0445 stub ScrollConsoleScreenBufferA
0446 stub ScrollConsoleScreenBufferW
0447 stdcall SearchPathA(ptr ptr ptr long ptr ptr) SearchPath32A
0448 stdcall SearchPathW(ptr ptr ptr long ptr ptr) SearchPath32W
0449 stdcall SetCommBreak(long) SetCommBreak32
0450 stub SetCommConfig
0451 stdcall SetCommMask(long ptr) SetCommMask
0452 stdcall SetCommState(long ptr) SetCommState32
0453 stdcall SetCommTimeouts(long ptr) SetCommTimeouts
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
0469 stdcall SetConsoleMode(long long) SetConsoleMode
0470 stub SetConsoleNumberOfCommandsA
0471 stub SetConsoleNumberOfCommandsW
0472 stub SetConsoleOutputCP
0473 stub SetConsolePalette
0474 stub SetConsoleScreenBufferSize
0475 stub SetConsoleTextAttribute
0476 stdcall SetConsoleTitleA(ptr) SetConsoleTitle32A
0477 stdcall SetConsoleTitleW(ptr) SetConsoleTitle32W
0478 stub SetConsoleWindowInfo
0479 stdcall SetCurrentDirectoryA(ptr) SetCurrentDirectory32A
0480 stdcall SetCurrentDirectoryW(ptr) SetCurrentDirectory32W
0481 stub SetDefaultCommConfigA
0482 stub SetDefaultCommConfigW
0483 stdcall SetEndOfFile(long) SetEndOfFile
0484 stdcall SetEnvironmentVariableA(ptr ptr) SetEnvironmentVariable32A
0485 stdcall SetEnvironmentVariableW(ptr ptr) SetEnvironmentVariable32W
0486 stdcall SetErrorMode(long) SetErrorMode32
0487 	stdcall	SetEvent(long) SetEvent
0488 stdcall SetFileApisToANSI() SetFileApisToANSI
0489 stdcall SetFileApisToOEM() SetFileApisToOEM
0490 stdcall SetFileAttributesA(ptr long) SetFileAttributes32A
0491 stdcall SetFileAttributesW(ptr long) SetFileAttributes32W
0492 stdcall SetFilePointer(long long ptr long) SetFilePointer
0493 stdcall SetFileTime(long ptr ptr ptr) SetFileTime
0494 stdcall SetHandleCount(long) SetHandleCount32
0495 stub SetHandleInformation
0496 stub SetLastConsoleEventActive
0497 stdcall SetLastError(long) SetLastError
0498 stub SetLocalTime
0499 stdcall SetLocaleInfoA(long long ptr) SetLocaleInfoA
0500 stub SetLocaleInfoW
0501 stub SetMailslotInfo
0502 stub SetNamedPipeHandleState
0503 stdcall SetPriorityClass(long long) SetPriorityClass
0504 stub SetProcessShutdownParameters
0505 stub SetProcessWorkingSetSize
0506 stdcall SetStdHandle(long long) SetStdHandle
0507 stdcall SetSystemTime(ptr) SetSystemTime
0508 stub SetSystemTimeAdjustment
0509 stub SetTapeParameters
0510 stub SetTapePosition
0511 stdcall SetThreadAffinityMask(long long)	SetThreadAffinityMask
0512 stub SetThreadContext
0513 stub SetThreadLocale
0514 stdcall SetThreadPriority(long long) SetThreadPriority
0515 stdcall SetTimeZoneInformation(ptr) SetTimeZoneInformation
0516 stdcall SetUnhandledExceptionFilter(ptr) THUNK_SetUnhandledExceptionFilter
0517 stub SetVDMCurrentDirectories
0518 stub SetVolumeLabelA
0519 stub SetVolumeLabelW
0520 stub SetupComm
0521 stub ShowConsoleCursor
0522 stdcall SizeofResource(long long) SizeofResource32
0523 	stdcall Sleep(long) Sleep
0524 stub SleepEx
0525 stub SuspendThread
0526 stdcall SystemTimeToFileTime(ptr ptr) SystemTimeToFileTime
0527 stub SystemTimeToTzSpecificLocalTime
0528 stub TerminateProcess
0529 stub TerminateThread
0530 stdcall TlsAlloc()	TlsAlloc
0531 stdcall TlsFree(long) TlsFree
0532 stdcall TlsGetValue(long) TlsGetValue
0533 stdcall TlsSetValue(long ptr) TlsSetValue
0534 stub TransactNamedPipe
0535 stdcall TransmitCommChar(long long) TransmitCommChar32
0536 stub TrimVirtualBuffer
0537 stdcall UnhandledExceptionFilter(ptr) UnhandledExceptionFilter
0538 stdcall UnlockFile(long long long long long) UnlockFile
0539 stub UnlockFileEx
0540 stdcall UnmapViewOfFile(ptr) UnmapViewOfFile
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
0552 stdcall VirtualProtect(ptr long long ptr) VirtualProtect
0553 stub VirtualProtectEx
0554 stdcall VirtualQuery(ptr ptr long) VirtualQuery
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
0565 stdcall WideCharToMultiByte(long long ptr long ptr long ptr ptr)	WideCharToMultiByte
0566 stdcall WinExec(ptr long) WinExec32
0567 stdcall WriteConsoleA(long ptr long ptr ptr) WriteConsole32A
0568 stub WriteConsoleInputA
0569 stub WriteConsoleInputVDMA
0570 stub WriteConsoleInputVDMW
0571 stub WriteConsoleInputW
0572 stub WriteConsoleOutputA
0573 stub WriteConsoleOutputAttribute
0574 stub WriteConsoleOutputCharacterA
0575 stub WriteConsoleOutputCharacterW
0576 stub WriteConsoleOutputW
0577 stdcall WriteConsoleW(long ptr long ptr ptr) WriteConsole32W
0578 stdcall WriteFile(long ptr long ptr ptr) WriteFile
0579 stub WriteFileEx
0580 stub WritePrivateProfileSectionA
0581 stub WritePrivateProfileSectionW
0582 stdcall WritePrivateProfileStringA(ptr ptr ptr ptr) WritePrivateProfileString32A
0583 stdcall WritePrivateProfileStringW(ptr ptr ptr ptr) WritePrivateProfileString32W
0584 stub WriteProcessMemory
0585 stub WriteProfileSectionA
0586 stub WriteProfileSectionW
0587 stdcall WriteProfileStringA(ptr ptr ptr) WriteProfileString32A
0588 stdcall WriteProfileStringW(ptr ptr ptr) WriteProfileString32W
0589 stub WriteTapemark
0590 stdcall _hread(long ptr long) _hread32
0591 stdcall _hwrite(long ptr long) _hwrite32
0592 stdcall _lclose(long) _lclose32
0593 stdcall _lcreat(ptr long) _lcreat32
0594 stdcall _llseek(long long long) _llseek32
0595 stdcall _lopen(ptr long) _lopen32
0596 stdcall _lread(long ptr long) _lread32
0597 stdcall _lwrite(long ptr long) _lwrite32
0598 stdcall lstrcat(ptr ptr) lstrcat32A
0599 stdcall lstrcatA(ptr ptr) lstrcat32A
0600 stdcall lstrcatW(ptr ptr) lstrcat32W
0601 stdcall lstrcmp(ptr ptr) lstrcmp32A
0602 stdcall lstrcmpA(ptr ptr) lstrcmp32A
0603 stdcall lstrcmpW(ptr ptr) lstrcmp32W
0604 stdcall lstrcmpi(ptr ptr) lstrcmpi32A
0605 stdcall lstrcmpiA(ptr ptr) lstrcmpi32A
0606 stdcall lstrcmpiW(ptr ptr) lstrcmpi32W
0607 stdcall lstrcpy(ptr ptr) lstrcpy32A
0608 stdcall lstrcpyA(ptr ptr) lstrcpy32A
0609 stdcall lstrcpyW(ptr ptr) lstrcpy32W
0610 stdcall lstrcpyn(ptr ptr long) lstrcpyn32A
0611 stdcall lstrcpynA(ptr ptr long) lstrcpyn32A
0612 stdcall lstrcpynW(ptr ptr long) lstrcpyn32W
0613 stdcall lstrlen(ptr) lstrlen32A
0614 stdcall lstrlenA(ptr) lstrlen32A
0615 stdcall lstrlenW(ptr) lstrlen32W
#late additions
0616 stub GetPrivateProfileSectionNamesA
0617 stub GetPrivateProfileSectionNamesW
0618 stub GetPrivateProfileStructA
0619 stub GetPrivateProfileStructW
0620 stub GetProcessVersion
0621    stdcall GetSystemPowerStatus(ptr) GetSystemPowerStatus
0622 stub GetSystemTimeAsFileTime
0623 stub HeapCreateTagsW
0624 stub HeapExtend
0625 stub HeapQueryTagW
0626 stub HeapSummary
0627 stub HeapUsage
0628 stub IsDebuggerPresent
0629 stub PostQueuedCompletionStatus
0630    stdcall SetSystemPowerState(long long) SetSystemPowerState
0631 stub WritePrivateProfileStructA
0632 stub WritePrivateProfileStructW
0633 stdcall MakeCriticalSectionGlobal(ptr) MakeCriticalSectionGlobal
#extra late additions
0634 stdcall ThunkConnect32(ptr ptr ptr ptr ptr ptr) ThunkConnect32
0636 stub SUnMapLS
0637 stub SMapLS
0638 stdcall ReinitializeCriticalSection(ptr) ReinitializeCriticalSection
0639 stub FT_Thunk
0640 stub FT_Exit20
0641 stub SMapLS_IP_EBP_12
0642 stub SUnMapLS_IP_EBP_12
0643 stub MapSLFix
0644 stub UnMapSLFixArray
0645 stub dprintf
0646 stub CreateToolhelp32Snapshot
0647 stub Module32First
0648 stub Module32Next
0649 stub Process32First
0650 stub Process32Next
0651 stub Thread32First
0652 stub Thread32Next
0653 stub RegisterServiceProcess
0654 stub QueueUserAPC
0655 stub ConvertToGlobalHandle
