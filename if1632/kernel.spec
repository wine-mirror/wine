name	kernel
type	win16
file	krnl386.exe

1   stub FatalExit
2   stub ExitKernel
3   pascal GetVersion() GetVersion16
4   pascal16 LocalInit(word word word) LocalInit
5   pascal16 LocalAlloc(word word) LocalAlloc16
6   pascal16 LocalReAlloc(word word word) LocalReAlloc16
7   pascal16 LocalFree(word) LocalFree16
8   pascal16 LocalLock(word) LocalLock16
9   pascal16 LocalUnlock(word) LocalUnlock16
10  pascal16 LocalSize(word) LocalSize16
11  pascal16 LocalHandle(word) LocalHandle16
12  pascal16 LocalFlags(word) LocalFlags16
13  pascal16 LocalCompact(word) LocalCompact16
14  pascal16 LocalNotify(long) LocalNotify
15  pascal16 GlobalAlloc(word long) GlobalAlloc16
16  pascal16 GlobalReAlloc(word long word) GlobalReAlloc16
17  pascal16 GlobalFree(word) GlobalFree16
18  pascal GlobalLock(word) WIN16_GlobalLock16
19  pascal16 GlobalUnlock(word) GlobalUnlock16
20  pascal GlobalSize(word) GlobalSize16
21  pascal GlobalHandle(word) GlobalHandle16
22  pascal16 GlobalFlags(word) GlobalFlags16
23  pascal16 LockSegment(word) LockSegment16
24  pascal16 UnlockSegment(word) UnlockSegment16
25  pascal GlobalCompact(long) GlobalCompact16
26  pascal16 GlobalFreeAll(word) GlobalFreeAll
27  pascal16 GetModuleName(word ptr word) GetModuleName
28  stub GlobalMasterHandle
29  pascal16 Yield() Yield16
30  pascal16 WaitEvent(word) WaitEvent
31  pascal16 PostEvent(word) PostEvent
32  pascal16 SetPriority(word s_word) SetPriority
33  pascal16 LockCurrentTask(word) LockCurrentTask
34  pascal SetTaskQueue(word word) SetTaskQueue
35  pascal16 GetTaskQueue(word) GetTaskQueue
36  pascal   GetCurrentTask() WIN16_GetCurrentTask
37  pascal GetCurrentPDB() GetCurrentPDB
38  pascal   SetTaskSignalProc(word segptr) THUNK_SetTaskSignalProc
41  return EnableDos 0 0
42  return DisableDos 0 0
45  pascal16 LoadModule(str ptr) LoadModule16
46  pascal16 FreeModule(word) FreeModule16
47  pascal   GetModuleHandle(segstr) WIN16_GetModuleHandle
48  pascal16 GetModuleUsage(word) GetModuleUsage
49  pascal16 GetModuleFileName(word ptr s_word) GetModuleFileName16
50  pascal GetProcAddress(word segstr) GetProcAddress16
51  pascal MakeProcInstance(segptr word) MakeProcInstance16
52  pascal16 FreeProcInstance(segptr) FreeProcInstance16
53  stub CallProcInstance
54  pascal16 GetInstanceData(word word word) GetInstanceData
55  register Catch(segptr) Catch 
56  register Throw(segptr word) Throw
57  pascal16 GetProfileInt(str str s_word) GetProfileInt16
58  pascal16 GetProfileString(str str str ptr word) GetProfileString16
59  pascal16 WriteProfileString(str str str) WriteProfileString16
60  pascal16 FindResource(word segstr segstr) FindResource16
61  pascal16 LoadResource(word word) LoadResource16
62  pascal LockResource(word) WIN16_LockResource16
63  pascal16 FreeResource(word) FreeResource16
64  pascal16 AccessResource(word word) AccessResource16
65  pascal SizeofResource(word word) SizeofResource16
66  pascal16 AllocResource(word word long) AllocResource
67  pascal SetResourceHandler(word segstr segptr) SetResourceHandler
68  pascal16 InitAtomTable(word) InitAtomTable16
69  pascal16 FindAtom(segstr) FindAtom16
70  pascal16 AddAtom(segstr) AddAtom16
71  pascal16 DeleteAtom(word) DeleteAtom16
72  pascal16 GetAtomName(word ptr word) GetAtomName16
73  pascal16 GetAtomHandle(word) GetAtomHandle
74  pascal16 OpenFile(str ptr word) OpenFile16
75  stub OpenPathName
76  stub DeletePathName
# Reserved*: old Win 2.x functions now moved to USER (Win 3.0+)
77  pascal Reserved1(segptr) AnsiNext16
78  pascal Reserved2(segptr segptr) AnsiPrev16
79  pascal Reserved3(segstr) AnsiUpper16
80  pascal Reserved4(segstr) AnsiLower16
81  pascal16 _lclose(word) _lclose16
82  pascal16 _lread(word segptr word) WIN16_lread
83  pascal16 _lcreat(str word) _lcreat16
84  pascal   _llseek(word long word) _llseek16
85  pascal16 _lopen(str word) _lopen16
86  pascal16 _lwrite(word ptr word) _lwrite16
87  pascal16 Reserved5(str str) lstrcmp16
88  pascal   lstrcpy(segptr str) lstrcpy16
89  pascal   lstrcat(segstr str) lstrcat16
90  pascal16 lstrlen(str) lstrlen16
91  register InitTask() InitTask
92  pascal   GetTempDrive(word) WIN16_GetTempDrive
93  pascal16 GetCodeHandle(segptr) GetCodeHandle
94  pascal16 DefineHandleTable(word) DefineHandleTable16
95  pascal16 LoadLibrary(str) LoadLibrary16
96  pascal16 FreeLibrary(word) FreeLibrary16
97  pascal16 GetTempFileName(word str word ptr) GetTempFileName16
98  return GetLastDiskChange 0 0
99  stub GetLPErrMode
100 return ValidateCodeSegments 0 0
101 stub NoHookDosCall
102 register DOS3Call() DOS3Call
103 register NetBIOSCall() NetBIOSCall
104 stub GetCodeInfo
105 pascal16 GetExeVersion() GetExeVersion
106 pascal SetSwapAreaSize(word) SetSwapAreaSize16
107 pascal16 SetErrorMode(word) SetErrorMode16
108 pascal16 SwitchStackTo(word word word) SwitchStackTo
109 register SwitchStackBack() SwitchStackBack
110 pascal16 PatchCodeHandle(word) PatchCodeHandle
111 pascal   GlobalWire(word) GlobalWire16
112 pascal16 GlobalUnWire(word) GlobalUnWire16
113 equate __AHSHIFT 3
114 equate __AHINCR 8
115 pascal16 OutputDebugString(str) OutputDebugString16
116 stub InitLib
117 pascal16 OldYield() OldYield
118 register GetTaskQueueDS() GetTaskQueueDS
119 register GetTaskQueueES() GetTaskQueueES
120 stub UndefDynLink
121 pascal16 LocalShrink(word word) LocalShrink16
122 pascal16 IsTaskLocked() IsTaskLocked
123 return KbdRst 0 0
124 return EnableKernel 0 0
125 return DisableKernel 0 0
126 stub MemoryFreed
127 pascal16 GetPrivateProfileInt(str str s_word str) GetPrivateProfileInt16
128 pascal16 GetPrivateProfileString(str str str ptr word str)
             GetPrivateProfileString16
129 pascal16 WritePrivateProfileString(str str str str)
             WritePrivateProfileString16
130 pascal FileCDR(ptr) FileCDR
131 pascal GetDOSEnvironment() GetDOSEnvironment
132 pascal GetWinFlags() GetWinFlags
133 register GetExePtr(word) WIN16_GetExePtr
134 pascal16 GetWindowsDirectory(ptr word) GetWindowsDirectory16
135 pascal16 GetSystemDirectory(ptr word) GetSystemDirectory16
136 pascal16 GetDriveType(word) GetDriveType16
137 pascal16 FatalAppExit(word str) FatalAppExit16
138 pascal GetHeapSpaces(word) GetHeapSpaces
139 stub DoSignal
140 pascal16 SetSigHandler(segptr ptr ptr word word) SetSigHandler
141 stub InitTask1
142 pascal16 GetProfileSectionNames(ptr word) GetProfileSectionNames16
143 pascal16 GetPrivateProfileSectionNames(ptr word str) GetPrivateProfileSectionNames16
144 pascal16 CreateDirectory(ptr ptr) CreateDirectory16
145 pascal16 RemoveDirectory(ptr) RemoveDirectory16
146 pascal16 DeleteFile(ptr) DeleteFile16
147 pascal16 SetLastError(long) SetLastError
148 pascal   GetLastError() GetLastError
149 pascal16 GetVersionEx(ptr) GetVersionEx16
150 pascal16 DirectedYield(word) DirectedYield
151 stub WinOldApCall
152 pascal16 GetNumTasks() GetNumTasks
154 pascal16 GlobalNotify(segptr) GlobalNotify
155 pascal16 GetTaskDS() GetTaskDS
156 return LimitEMSPages 4 0
157 return GetCurPID 4 0
158 return IsWinOldApTask 2 0
159 pascal GlobalHandleNoRIP(word) GlobalHandleNoRIP
160 stub EMSCopy
161 pascal16 LocalCountFree() LocalCountFree
162 pascal16 LocalHeapSize() LocalHeapSize
163 pascal16 GlobalLRUOldest(word) GlobalLRUOldest
164 pascal16 GlobalLRUNewest(word) GlobalLRUNewest
165 return A20Proc 2 0
166 pascal16 WinExec(str word) WinExec16
167 pascal16 GetExpWinVer(word) GetExpWinVer
168 pascal16 DirectResAlloc(word word word) DirectResAlloc
169 pascal GetFreeSpace(word) GetFreeSpace16
170 pascal16 AllocCStoDSAlias(word) AllocCStoDSAlias
171 pascal16 AllocDStoCSAlias(word) AllocDStoCSAlias
172 pascal16 AllocAlias(word) AllocCStoDSAlias
173 equate __ROMBIOS 0
174 equate __A000H 0
175 pascal16 AllocSelector(word) AllocSelector
176 pascal16 FreeSelector(word) FreeSelector
177 pascal16 PrestoChangoSelector(word word) PrestoChangoSelector
178 equate __WINFLAGS 0x413
179 equate __D000H 0
180 pascal16 LongPtrAdd(long long) LongPtrAdd
181 equate __B000H 0
182 equate __B800H 0
183 equate __0000H 0
184 pascal GlobalDOSAlloc(long) GlobalDOSAlloc
185 pascal16 GlobalDOSFree(word) GlobalDOSFree
186 pascal GetSelectorBase(word) GetSelectorBase
187 pascal16 SetSelectorBase(word long) SetSelectorBase
188 pascal GetSelectorLimit(word) GetSelectorLimit
189 pascal16 SetSelectorLimit(word long) SetSelectorLimit
190 equate __E000H 0
191 pascal16 GlobalPageLock(word) GlobalPageLock
192 pascal16 GlobalPageUnlock(word) GlobalPageUnlock
193 equate __0040H 0
194 equate __F000H 0
195 equate __C000H 0
196 pascal16 SelectorAccessRights(word word word) SelectorAccessRights
197 pascal16 GlobalFix(word) GlobalFix16
198 pascal16 GlobalUnfix(word) GlobalUnfix16
199 pascal16 SetHandleCount(word) SetHandleCount16
200 return ValidateFreeSpaces 0 0
201 stub ReplaceInst
202 stub RegisterPtrace
203 register DebugBreak() DebugBreak
204 stub SwapRecording
205 stub CVWBreak
206 pascal16 AllocSelectorArray(word) AllocSelectorArray
207 pascal16 IsDBCSLeadByte(word) IsDBCSLeadByte16
208 stub KERNEL_208
209 stub KERNEL_209
210 stub KERNEL_210
211 stub KERNEL_211
213 stub KERNEL_213
214 stub KERNEL_214
216 pascal   RegEnumKey(long long ptr long) RegEnumKey16
217 pascal   RegOpenKey(long str ptr) RegOpenKey16
218 pascal   RegCreateKey(long str ptr) RegCreateKey16
219 pascal   RegDeleteKey(long str) RegDeleteKey16
220 pascal   RegCloseKey(long) RegCloseKey
221 pascal   RegSetValue(long str long ptr long) RegSetValue16
222 pascal   RegDeleteValue(long str) RegDeleteValue16
223 pascal   RegEnumValue(long long ptr ptr ptr ptr ptr ptr) RegEnumValue16
224 pascal   RegQueryValue(long str ptr ptr) RegQueryValue16
225 pascal   RegQueryValueEx(long str ptr ptr ptr ptr) RegQueryValueEx16
226 pascal   RegSetValueEx(long str long long ptr long) RegSetValueEx16
227 pascal   RegFlushKey(long) RegFlushKey
228 stub K228
229 stub K229
230 pascal GlobalSmartPageLock(word) GlobalPageLock #?
231 stub GlobalSmartPageUnlock
232 stub RegLoadKey
233 stub RegUnloadKey
234 stub RegSaveKey
235 stub InvalidateNlsCache
236 stub GetProductName
237 stub KERNEL_237
262 stub KERNEL_262
263 stub KERNEL_263
310 pascal16 LocalHandleDelta(word) LocalHandleDelta
311 pascal GetSetKernelDOSProc(ptr) GetSetKernelDOSProc
314 stub DebugDefineSegment
315 pascal16 WriteOutProfiles() WriteOutProfiles
316 pascal GetFreeMemInfo() GetFreeMemInfo
318 stub FatalExitHook
319 stub FlushCachedFileHandle
320 pascal16 IsTask(word) IsTask
321 stub KERNEL_321
323 return IsRomModule 2 0
324 pascal16 LogError(word ptr) LogError
325 pascal16 LogParamError(word ptr ptr) LogParamError
326 return IsRomFile 2 0
327 stub KERNEL_327
328 stub _DebugOutput
329 pascal16 K329(str word) DebugFillBuffer
332 long THHOOK(0 0 0 0 0 0 0 0)
334 pascal16 IsBadReadPtr(segptr word) IsBadReadPtr16
335 pascal16 IsBadWritePtr(segptr word) IsBadWritePtr16
336 pascal16 IsBadCodePtr(segptr) IsBadCodePtr16
337 pascal16 IsBadStringPtr(segptr word) IsBadStringPtr16
338 stub HasGPHandler
339 pascal16 DiagQuery() DiagQuery
340 pascal16 DiagOutput(str) DiagOutput
341 pascal ToolHelpHook(ptr) ToolHelpHook
342 stub __GP
343 stub RegisterWinOldApHook
344 stub GetWinOldApHooks
345 pascal16 IsSharedSelector(word) IsSharedSelector
346 pascal16 IsBadHugeReadPtr(segptr long) IsBadHugeReadPtr16
347 pascal16 IsBadHugeWritePtr(segptr long) IsBadHugeWritePtr16
348 pascal16 hmemcpy(ptr ptr long) hmemcpy
349 pascal   _hread(word segptr long) WIN16_hread
350 pascal   _hwrite(word ptr long) _hwrite16
#351 BUNNY_351
352 pascal   lstrcatn(segstr str word) lstrcatn16
353 pascal   lstrcpyn(segptr str word) lstrcpyn16
354 pascal   GetAppCompatFlags(word) GetAppCompatFlags16
355 pascal16 GetWinDebugInfo(ptr word) GetWinDebugInfo
356 pascal16 SetWinDebugInfo(ptr) SetWinDebugInfo
357 pascal MapSL(segptr) MapSL
358 pascal MapLS(long) MapLS
359 pascal UnMapLS(segptr) UnMapLS
360 pascal16 OpenFileEx(str ptr word) OpenFile16
#361 PIGLET_361
365 stub KERNEL_365
403 pascal16 FarSetOwner(word word) FarSetOwner
404 pascal16 FarGetOwner(word) FarGetOwner
406 stub WritePrivateProfileStruct
407 stub GetPrivateProfileStruct
411 pascal   GetCurrentDirectory(long ptr) GetCurrentDirectory16
412 pascal16 SetCurrentDirectory(ptr) SetCurrentDirectory16
413 pascal16 FindFirstFile(ptr ptr) FindFirstFile16
414 pascal16 FindNextFile(word ptr) FindNextFile16
415 pascal16 FindClose(word) FindClose16
416 stub WritePrivateProfileSection
417 stub WriteProfileSection
418 stub GetPrivateProfileSection
419 stub GetProfileSection
420 pascal   GetFileAttributes(ptr) GetFileAttributes16
421 pascal16 SetFileAttributes(ptr long) SetFileAttributes16
422 pascal16 GetDiskFreeSpace(ptr ptr ptr ptr ptr) GetDiskFreeSpace16 
431 pascal16 IsPeFormat(str word) IsPeFormat
432 stub FileTimeToLocalFileTime
434 stub KERNEL_434
435 stub KERNEL_435
439 stub KERNEL_439
440 stub KERNEL_440
444 stub KERNEL_444
445 stub KERNEL_445
446 stub KERNEL_446
447 stub KERNEL_447
449 pascal GetpWin16Lock() GetpWin16Lock16
450 pascal16 KERNEL_450() stub_KERNEL_450
452 stub KERNEL_452
453 stub KERNEL_453
454 equate __FLATCS 0   # initialized by BUILTIN_Init()
455 equate __FLATDS 0   # initialized by BUILTIN_Init()
471 pascal KERNEL_471() _KERNEL_471
472 register KERNEL_472() _KERNEL_472
473 stub KERNEL_473
475 register KERNEL_475() _KERNEL_475
480 stub KERNEL_480
481 stub KERNEL_481
482 pascal LoadLibrary32(str) LoadLibrary32A
485 pascal GetProcessDWORD(long s_word) GetProcessDword
486 stub KERNEL_486
491 stub RegisterServiceProcess

# Those stubs can be found in WindowNT3.51 krnl386.exe. Some are used by Win95
# too, some seem to specify different functions in Win95... Ugh.
500 pascal WOW16Call(word word word) WOW16Call
501 stub KDDBGOUT
502 stub WOWGETNEXTVDMCOMMAND
503 stub WOWREGISTERSHELLWINDOWHANDLE
504 stub WOWLOADMODULE
505 stub WOWQUERYPERFORMANCECOUNTER
506 stub WOWCURSORICONOP
507 stub WOWFAILEDEXEC
508 stub WOWCLOSECOMPORT
509 stub WOWKILLREMOTETASK
511 stub WOWKILLREMOTETASK
512 stub WOWQUERYDEBUG
513 pascal   LoadLibraryEx32W(ptr long long) LoadLibraryEx32W16
514 pascal16 FreeLibrary32W(long) FreeLibrary32
515 pascal   GetProcAddress32W(long str) GetProcAddress32
516 pascal GetVDMPointer32W(segptr long) GetVDMPointer32W
517 pascal CallProc32W() WIN16_CallProc32W
518 pascal CallProcEx32W() WIN16_CallProcEx32W 
519 stub EXITKERNELTHUNK
# the __MOD_ variables are WORD datareferences.
520 equate __MOD_KERNEL 4200
521 equate __MOD_DKERNEL 4201
522 equate __MOD_USER 4203
523 equate __MOD_DUSER 4204
524 equate __MOD_GDI 4205
525 equate __MOD_DGDI 4206
526 equate __MOD_KEYBOARD 4207
527 equate __MOD_SOUND 4208
528 equate __MOD_SHELL 4209
529 equate __MOD_WINSOCK 4210
530 equate __MOD_TOOLHELP 4211
531 equate __MOD_MMEDIA 4212
532 equate __MOD_COMMDLG 4213
540 stub KERNEL_540
541 stub WOWSETEXITONLASTAPP
544 stub WOWSETCOMPATHANDLE

600 stub KERNEL_600
601 stub KERNEL_601
604 stub KERNEL_604
605 stub KERNEL_605
606 stub KERNEL_606
607 pascal KERNEL_607(long long long) _KERNEL_607
608 pascal KERNEL_608(long long long) _KERNEL_608
611 pascal KERNEL_611(long long) _KERNEL_611
612 stub KERNEL_612
613 stub KERNEL_613
614 stub KERNEL_614
619 pascal KERNEL_619(word long long) _KERNEL_619
621 stub KERNEL_621
627 stub IsBadFlatReadWritePtr
630 register C16ThkSL() C16ThkSL
631 register C16ThkSL01() C16ThkSL01
651 pascal ThunkConnect16(str str word long ptr str word) ThunkConnect16
700 pascal KERNEL_700() stub_KERNEL_700
