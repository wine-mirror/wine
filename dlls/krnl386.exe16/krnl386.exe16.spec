# 1-207 are the basic functions, those are (with minor variations)
# present in win31, win95 and nt351

1   stub FatalExit
2   pascal -ret16 ExitKernel() ExitKernel16
3   pascal GetVersion() GetVersion16
4   pascal -ret16 LocalInit(word word word) LocalInit16
5   pascal -ret16 LocalAlloc(word word) LocalAlloc16
6   pascal -ret16 LocalReAlloc(word word word) LocalReAlloc16
7   pascal -ret16 LocalFree(word) LocalFree16
8   pascal LocalLock(word) LocalLock16
9   pascal -ret16 LocalUnlock(word) LocalUnlock16
10  pascal -ret16 LocalSize(word) LocalSize16
11  pascal -ret16 LocalHandle(word) LocalHandle16
12  pascal -ret16 LocalFlags(word) LocalFlags16
13  pascal -ret16 LocalCompact(word) LocalCompact16
14  pascal LocalNotify(long) LocalNotify16
15  pascal -ret16 GlobalAlloc(word long) GlobalAlloc16
16  pascal -ret16 GlobalReAlloc(word long word) GlobalReAlloc16
17  pascal -ret16 GlobalFree(word) GlobalFree16
18  pascal GlobalLock(word) WIN16_GlobalLock16
19  pascal -ret16 GlobalUnlock(word) GlobalUnlock16
20  pascal GlobalSize(word) GlobalSize16
21  pascal GlobalHandle(word) GlobalHandle16
22  pascal -ret16 GlobalFlags(word) GlobalFlags16
23  pascal -ret16 LockSegment(word) LockSegment16
24  pascal -ret16 UnlockSegment(word) UnlockSegment16
25  pascal GlobalCompact(long) GlobalCompact16
26  pascal -ret16 GlobalFreeAll(word) GlobalFreeAll16
27  pascal -ret16 GetModuleName(word ptr word) GetModuleName16 # W1.1: SETSWAPHOOK, W2.0: nothing !
28  pascal   GlobalMasterHandle() GlobalMasterHandle16
29  pascal -ret16 Yield() Yield16
30  pascal -ret16 WaitEvent(word) WaitEvent16
31  pascal -ret16 PostEvent(word) PostEvent16
32  pascal -ret16 SetPriority(word s_word) SetPriority16
33  pascal -ret16 LockCurrentTask(word) LockCurrentTask16
34  pascal -ret16 SetTaskQueue(word word) SetTaskQueue16
35  pascal -ret16 GetTaskQueue(word) GetTaskQueue16
36  pascal   GetCurrentTask() WIN16_GetCurrentTask
37  pascal GetCurrentPDB() GetCurrentPDB16
38  pascal   SetTaskSignalProc(word segptr) SetTaskSignalProc
39  stub     SetTaskSwitchProc      # W1.1, W2.0
40  stub     SetTaskInterchange     # W1.1, W2.0
41  pascal -ret16 EnableDos() KERNEL_nop
42  pascal -ret16 DisableDos() KERNEL_nop
43  stub     IsScreenGrab           # W1.1, W2.0
44  stub     BuildPDB               # W1.1, W2.0
45  pascal -ret16 LoadModule(str ptr) LoadModule16
46  pascal -ret16 FreeModule(word) FreeModule16
47  pascal   GetModuleHandle(segstr) WIN16_GetModuleHandle
48  pascal -ret16 GetModuleUsage(word) GetModuleUsage16
49  pascal -ret16 GetModuleFileName(word ptr s_word) GetModuleFileName16
50  pascal GetProcAddress(word str) GetProcAddress16
51  pascal MakeProcInstance(segptr word) MakeProcInstance16
52  pascal -ret16 FreeProcInstance(segptr) FreeProcInstance16
53  stub CallProcInstance
54  pascal -ret16 GetInstanceData(word word word) GetInstanceData16
55  pascal -register Catch(ptr) Catch16
56  pascal -register Throw(ptr word) Throw16
57  pascal -ret16 GetProfileInt(str str s_word) GetProfileInt16
58  pascal -ret16 GetProfileString(str str str ptr word) GetProfileString16
59  pascal -ret16 WriteProfileString(str str str) WriteProfileString16
60  pascal -ret16 FindResource(word str str) FindResource16
61  pascal -ret16 LoadResource(word word) LoadResource16
62  pascal LockResource(word) WIN16_LockResource16
63  pascal -ret16 FreeResource(word) FreeResource16
64  pascal -ret16 AccessResource(word word) AccessResource16
65  pascal SizeofResource(word word) SizeofResource16
66  pascal -ret16 AllocResource(word word long) AllocResource16
67  pascal SetResourceHandler(word str segptr) SetResourceHandler16
68  pascal -ret16 InitAtomTable(word) InitAtomTable16
69  pascal -ret16 FindAtom(str) FindAtom16
70  pascal -ret16 AddAtom(str) AddAtom16
71  pascal -ret16 DeleteAtom(word) DeleteAtom16
72  pascal -ret16 GetAtomName(word ptr word) GetAtomName16
73  pascal -ret16 GetAtomHandle(word) GetAtomHandle16
74  pascal -ret16 OpenFile(str ptr word) OpenFile16
75  stub OpenPathName
76  stub DeletePathName
# Reserved*: old Win 2.x functions now moved to USER (Win 3.0+)
77  pascal Reserved1(segptr) KERNEL_AnsiNext16
78  pascal Reserved2(segptr segptr) KERNEL_AnsiPrev16
79  pascal Reserved3(segstr) KERNEL_AnsiUpper16
80  pascal Reserved4(segstr) KERNEL_AnsiLower16
81  pascal -ret16 _lclose(word) _lclose16
82  pascal -ret16 _lread(word segptr word) WIN16_lread
83  pascal -ret16 _lcreat(str word) _lcreat16
84  pascal   _llseek(word long word) _llseek16
85  pascal -ret16 _lopen(str word) _lopen16
86  pascal -ret16 _lwrite(word ptr word) _lwrite16
87  pascal -ret16 Reserved5(str str) KERNEL_lstrcmp16
88  pascal   lstrcpy(segptr str) lstrcpy16
89  pascal   lstrcat(segstr str) lstrcat16
90  pascal -ret16 lstrlen(str) lstrlen16
91  pascal -register InitTask() InitTask16
92  pascal   GetTempDrive(word) GetTempDrive
93  pascal GetCodeHandle(segptr) GetCodeHandle16
94  pascal -ret16 DefineHandleTable(word) DefineHandleTable16
95  pascal -ret16 LoadLibrary(str) LoadLibrary16
96  pascal -ret16 FreeLibrary(word) FreeLibrary16
97  pascal -ret16 GetTempFileName(word str word ptr) GetTempFileName16
98  pascal -ret16 GetLastDiskChange() KERNEL_nop
99  pascal GetLPErrMode()
100 pascal -ret16 ValidateCodeSegments() KERNEL_nop
101 stub NoHookDosCall
102 pascal -register DOS3Call() DOS3Call
103 pascal -register NetBIOSCall() NetBIOSCall16
104 pascal -ret16 GetCodeInfo(segptr ptr) GetCodeInfo16
105 pascal -ret16 GetExeVersion() GetExeVersion16
106 pascal SetSwapAreaSize(word) SetSwapAreaSize16
107 pascal -ret16 SetErrorMode(word) SetErrorMode16
108 pascal -ret16 SwitchStackTo(word word word) SwitchStackTo16 # STO in W2.0
109 pascal -register SwitchStackBack() SwitchStackBack16 # SBACK in W2.0
110 pascal   PatchCodeHandle(word) PatchCodeHandle16
111 pascal   GlobalWire(word) GlobalWire16
112 pascal -ret16 GlobalUnWire(word) GlobalUnWire16
113 equate __AHSHIFT 3
114 equate __AHINCR 8
115 pascal -ret16 OutputDebugString(str) OutputDebugString16
116 stub InitLib
117 pascal -ret16 OldYield() OldYield16
118 pascal -ret16 GetTaskQueueDS() GetTaskQueueDS16
119 pascal -ret16 GetTaskQueueES() GetTaskQueueES16
120 stub UndefDynLink
121 pascal -ret16 LocalShrink(word word) LocalShrink16
122 pascal -ret16 IsTaskLocked() IsTaskLocked16
123 pascal -ret16 KbdRst() KERNEL_nop
124 pascal -ret16 EnableKernel() KERNEL_nop
125 pascal -ret16 DisableKernel() KERNEL_nop
126 stub MemoryFreed
127 pascal -ret16 GetPrivateProfileInt(str str s_word str) GetPrivateProfileInt16
128 pascal -ret16 GetPrivateProfileString(str str str ptr word str) GetPrivateProfileString16
129 pascal -ret16 WritePrivateProfileString(str str str str) WritePrivateProfileString16
130 pascal FileCDR(ptr) FileCDR16
131 pascal GetDOSEnvironment() GetDOSEnvironment16
132 pascal GetWinFlags() GetWinFlags16
133 pascal -ret16 GetExePtr(word) WIN16_GetExePtr
134 pascal -ret16 GetWindowsDirectory(ptr word) GetWindowsDirectory16
135 pascal -ret16 GetSystemDirectory(ptr word) GetSystemDirectory16
136 pascal -ret16 GetDriveType(word) GetDriveType16
137 pascal -ret16 FatalAppExit(word str) FatalAppExit16
138 pascal GetHeapSpaces(word) GetHeapSpaces16
139 stub DoSignal
140 pascal -ret16 SetSigHandler(segptr ptr ptr word word) SetSigHandler16
141 stub InitTask1
142 pascal -ret16 GetProfileSectionNames(ptr word) GetProfileSectionNames16
143 pascal -ret16 GetPrivateProfileSectionNames(ptr word str) GetPrivateProfileSectionNames16
144 pascal -ret16 CreateDirectory(ptr ptr) CreateDirectory16
145 pascal -ret16 RemoveDirectory(ptr) RemoveDirectory16
146 pascal -ret16 DeleteFile(ptr) DeleteFile16
147 pascal -ret16 SetLastError(long) SetLastError16
148 pascal GetLastError() GetLastError16
149 pascal -ret16 GetVersionEx(ptr) GetVersionEx16
150 pascal -ret16 DirectedYield(word) DirectedYield16
151 stub WinOldApCall
152 pascal -ret16 GetNumTasks() GetNumTasks16
154 pascal -ret16 GlobalNotify(segptr) GlobalNotify16
155 pascal -ret16 GetTaskDS() GetTaskDS16
156 pascal   LimitEMSPages(long) LimitEMSPages16
157 pascal   GetCurPID(long) GetCurPID16
158 pascal -ret16 IsWinOldApTask(word) IsWinOldApTask16
159 pascal GlobalHandleNoRIP(word) GlobalHandleNoRIP16
160 stub EMSCopy
161 pascal -ret16 LocalCountFree() LocalCountFree16
162 pascal -ret16 LocalHeapSize() LocalHeapSize16
163 pascal -ret16 GlobalLRUOldest(word) GlobalLRUOldest16
164 pascal -ret16 GlobalLRUNewest(word) GlobalLRUNewest16
165 pascal -ret16 A20Proc(word) A20Proc16
166 pascal -ret16 WinExec(str word) WinExec16
167 pascal -ret16 GetExpWinVer(word) GetExpWinVer16
168 pascal -ret16 DirectResAlloc(word word word) DirectResAlloc16
169 pascal GetFreeSpace(word) GetFreeSpace16
170 pascal -ret16 AllocCStoDSAlias(word) AllocCStoDSAlias16
171 pascal -ret16 AllocDStoCSAlias(word) AllocDStoCSAlias16
172 pascal -ret16 AllocAlias(word) AllocCStoDSAlias16
173 equate __ROMBIOS 0
174 equate __A000H 0
175 pascal -ret16 AllocSelector(word) AllocSelector16
176 pascal -ret16 FreeSelector(word) FreeSelector16
177 pascal -ret16 PrestoChangoSelector(word word) PrestoChangoSelector16
178 equate __WINFLAGS 0x413
179 equate __D000H 0
180 pascal -ret16 LongPtrAdd(long long) LongPtrAdd16
181 equate __B000H 0
182 equate __B800H 0
183 equate __0000H 0
184 pascal GlobalDOSAlloc(long) GlobalDOSAlloc16
185 pascal -ret16 GlobalDOSFree(word) GlobalDOSFree16
186 pascal GetSelectorBase(word) GetSelectorBase
187 pascal -ret16 SetSelectorBase(word long) SetSelectorBase
188 pascal GetSelectorLimit(word) GetSelectorLimit16
189 pascal -ret16 SetSelectorLimit(word long) SetSelectorLimit16
190 equate __E000H 0
191 pascal -ret16 GlobalPageLock(word) GlobalPageLock16
192 pascal -ret16 GlobalPageUnlock(word) GlobalPageUnlock16
193 equate __0040H 0
194 equate __F000H 0
195 equate __C000H 0
196 pascal -ret16 SelectorAccessRights(word word word) SelectorAccessRights16
197 pascal -ret16 GlobalFix(word) GlobalFix16
198 pascal -ret16 GlobalUnfix(word) GlobalUnfix16
199 pascal -ret16 SetHandleCount(word) SetHandleCount16
200 pascal -ret16 ValidateFreeSpaces() KERNEL_nop
201 stub ReplaceInst
202 stub RegisterPtrace
203 pascal -register DebugBreak() DebugBreak16
204 stub SwapRecording
205 stub CVWBreak
206 pascal -ret16 AllocSelectorArray(word) AllocSelectorArray16
207 pascal -ret16 IsDBCSLeadByte(word) IsDBCSLeadByte


# 208-237 are Win95 extensions; a few of those are also present in WinNT

208 pascal K208(word long long long) Local32Init16
209 pascal K209(long long word long) Local32Alloc16
210 pascal K210(long long word long long) Local32ReAlloc16
211 pascal K211(long long word) Local32Free16
213 pascal K213(long long word word) Local32Translate16
214 pascal K214(long long word) Local32Size16
215 pascal K215(long word) Local32ValidHandle16  # Win95 only -- CONFLICT!
#215 stub WOWShouldWeSayWin95                  # WinNT only -- CONFLICT!
216 pascal RegEnumKey(long long ptr long) RegEnumKey16                    # Both 95/NT
217 pascal RegOpenKey(long str ptr) RegOpenKey16                          # Both 95/NT
218 pascal RegCreateKey(long str ptr) RegCreateKey16
219 pascal RegDeleteKey(long str) RegDeleteKey16
220 pascal RegCloseKey(long) RegCloseKey16                                # Both 95/NT
221 pascal RegSetValue(long str long ptr long) RegSetValue16
222 pascal RegDeleteValue(long str) RegDeleteValue16
223 pascal RegEnumValue(long long ptr ptr ptr ptr ptr ptr) RegEnumValue16 # Both 95/NT
224 pascal RegQueryValue(long str ptr ptr) RegQueryValue16
225 pascal RegQueryValueEx(long str ptr ptr ptr ptr) RegQueryValueEx16
226 pascal RegSetValueEx(long str long long ptr long) RegSetValueEx16
227 pascal RegFlushKey(long) RegFlushKey16
228 pascal -ret16 K228(word) GetExePtr
229 pascal -ret16 K229(long) Local32GetSegment16
230 pascal GlobalSmartPageLock(word) GlobalPageLock16 #?
231 pascal GlobalSmartPageUnlock(word) GlobalPageUnlock16 #?
232 stub RegLoadKey
233 stub RegUnloadKey
234 stub RegSaveKey
235 stub InvalidateNlsCache
236 stub GetProductName
237 pascal -ret16 K237() KERNEL_nop


# 262-274 are WinNT extensions; those are not present in Win95

262 stub WOWWaitForMsgAndEvent
263 stub WOWMsgBox
273 stub K273
274 pascal -ret16 GetShortPathName(str ptr word) GetShortPathName16


# 310-356 are again shared between all versions

310 pascal -ret16 LocalHandleDelta(word) LocalHandleDelta16
311 pascal GetSetKernelDOSProc(ptr) GetSetKernelDOSProc16
314 stub DebugDefineSegment
315 pascal -ret16 WriteOutProfiles() WriteOutProfiles16
316 pascal GetFreeMemInfo() GetFreeMemInfo16
318 stub FatalExitHook
319 stub FlushCachedFileHandle
320 pascal -ret16 IsTask(word) IsTask16
323 pascal -ret16 IsRomModule(word) IsRomModule16
324 pascal -ret16 LogError(word ptr) LogError16
325 pascal -ret16 LogParamError(word ptr ptr) LogParamError16
326 pascal -ret16 IsRomFile(word) IsRomFile16
327 pascal -register K327() HandleParamError
328 varargs -ret16 _DebugOutput(word str) _DebugOutput
329 pascal -ret16 K329(str word) DebugFillBuffer
332 variable THHOOK(0 0 0 0 0 0 0 0)
334 pascal -ret16 IsBadReadPtr(segptr word) IsBadReadPtr16
335 pascal -ret16 IsBadWritePtr(segptr word) IsBadWritePtr16
336 pascal -ret16 IsBadCodePtr(segptr) IsBadCodePtr16
337 pascal -ret16 IsBadStringPtr(segptr word) IsBadStringPtr16
338 pascal -ret16 HasGPHandler(segptr) HasGPHandler16
339 pascal -ret16 DiagQuery() DiagQuery16
340 pascal -ret16 DiagOutput(str) DiagOutput16
341 pascal ToolHelpHook(ptr) ToolHelpHook16
342 variable __GP(0 0)
343 stub RegisterWinOldApHook
344 stub GetWinOldApHooks
345 pascal -ret16 IsSharedSelector(word) IsSharedSelector16
346 pascal -ret16 IsBadHugeReadPtr(segptr long) IsBadHugeReadPtr16
347 pascal -ret16 IsBadHugeWritePtr(segptr long) IsBadHugeWritePtr16
348 pascal -ret16 hmemcpy(ptr ptr long) hmemcpy16
349 pascal   _hread(word segptr long) WIN16_hread
350 pascal   _hwrite(word ptr long) _hwrite16
351 pascal -ret16 BUNNY_351() KERNEL_nop
352 pascal   lstrcatn(segstr str word) lstrcatn16
353 pascal   lstrcpyn(segptr str word) lstrcpyn16
354 pascal   GetAppCompatFlags(word) GetAppCompatFlags16
355 pascal -ret16 GetWinDebugInfo(ptr word) GetWinDebugInfo16
356 pascal -ret16 SetWinDebugInfo(ptr) SetWinDebugInfo16


# 357-365 are present in Win95 only
# Note that from here on most of the Win95-only functions are exported
# ordinal-only; the names given here are mostly guesses :-)

357 pascal MapSL(segptr) MapSL
358 pascal MapLS(long) MapLS
359 pascal UnMapLS(segptr) UnMapLS
360 pascal -ret16 OpenFileEx(str ptr word) OpenFile16
361 pascal -ret16 PIGLET_361() KERNEL_nop
362 stub ThunkTerminateProcess
365 pascal -register GlobalChangeLockCount(word word) GlobalChangeLockCount16


# 403-404 are common to all versions

403 pascal -ret16 FarSetOwner(word word) FarSetOwner16 # aka K403
404 pascal -ret16 FarGetOwner(word) FarGetOwner16 # aka K404


# 406-494 are present only in Win95

406 pascal -ret16 WritePrivateProfileStruct(str str ptr word str) WritePrivateProfileStruct16
407 pascal -ret16 GetPrivateProfileStruct(str str ptr word str) GetPrivateProfileStruct16
408 stub KERNEL_408
409 stub KERNEL_409
410 stub CreateProcessFromWinExec
411 pascal   GetCurrentDirectory(long ptr) GetCurrentDirectory16
412 pascal -ret16 SetCurrentDirectory(ptr) SetCurrentDirectory16
413 pascal -ret16 FindFirstFile(ptr ptr) FindFirstFile16
414 pascal -ret16 FindNextFile(word ptr) FindNextFile16
415 pascal -ret16 FindClose(word) FindClose16
416 pascal -ret16 WritePrivateProfileSection(str str str) WritePrivateProfileSection16
417 pascal -ret16 WriteProfileSection(str str) WriteProfileSection16
418 pascal -ret16 GetPrivateProfileSection(str ptr word str) GetPrivateProfileSection16
419 pascal -ret16 GetProfileSection(str ptr word) GetProfileSection16
420 pascal   GetFileAttributes(ptr) GetFileAttributes16
421 pascal -ret16 SetFileAttributes(ptr long) SetFileAttributes16
422 pascal -ret16 GetDiskFreeSpace(ptr ptr ptr ptr ptr) GetDiskFreeSpace16
423 pascal -ret16 LogApiThk(str) LogApiThk
431 pascal -ret16 IsPeFormat(str word) IsPeFormat16
432 stub FileTimeToLocalFileTime
434 pascal -ret16 UnicodeToAnsi(ptr ptr word) UnicodeToAnsi16
435 stub GetTaskFlags
436 pascal -ret16 _ConfirmSysLevel(ptr) _ConfirmSysLevel
437 pascal -ret16 _CheckNotSysLevel(ptr) _CheckNotSysLevel
438 pascal -ret16 _CreateSysLevel(ptr long) _CreateSysLevel
439 pascal -ret16 _EnterSysLevel(ptr) _EnterSysLevel
440 pascal -ret16 _LeaveSysLevel(ptr) _LeaveSysLevel
441 pascal CreateThread16(ptr long segptr segptr long ptr) CreateThread16
442 pascal VWin32_EventCreate() VWin32_EventCreate
443 pascal VWin32_EventDestroy(long) VWin32_EventDestroy
444 pascal -ret16 Local32Info(ptr word) Local32Info16
445 pascal -ret16 Local32First(ptr word) Local32First16
446 pascal -ret16 Local32Next(ptr) Local32Next16
447 pascal -ret16 WIN32_OldYield() WIN32_OldYield16
448 stub KERNEL_448
449 pascal GetpWin16Lock() GetpWin16Lock16
450 pascal VWin32_EventWait(long) VWin32_EventWait
451 pascal VWin32_EventSet(long) VWin32_EventSet
452 pascal LoadLibrary32(str) LoadLibrary32_16
453 pascal GetProcAddress32(long str) GetProcAddress32_16
454 equate __FLATCS 0   # initialized by BUILTIN_Init()
455 equate __FLATDS 0   # initialized by BUILTIN_Init()
456 pascal DefResourceHandler(word word word) NE_DefResourceHandler
457 pascal CreateW32Event(long long) CreateW32Event
458 pascal SetW32Event(long) SetW32Event
459 pascal ResetW32Event(long) ResetW32Event
460 pascal WaitForSingleObject(long long) WaitForSingleObject16
461 pascal WaitForMultipleObjects(long ptr long long) WaitForMultipleObjects16
462 pascal GetCurrentThreadId() GetCurrentThreadId16
463 pascal SetThreadQueue(long word) SetThreadQueue16
464 pascal GetThreadQueue(long) GetThreadQueue16
465 stub NukeProcess
466 pascal -ret16 ExitProcess(word) ExitProcess16
467 stub WOACreateConsole
468 stub WOASpawnConApp
469 stub WOAGimmeTitle
470 stub WOADestroyConsole
471 pascal GetCurrentProcessId() GetCurrentProcessId16
472 pascal -register MapHInstLS() MapHInstLS16
473 pascal -register MapHInstSL() MapHInstSL16
474 pascal CloseW32Handle(long) CloseW32Handle
475 pascal -ret16 GetTEBSelectorFS() GetTEBSelectorFS16
476 pascal ConvertToGlobalHandle(long) ConvertToGlobalHandle16
477 stub WOAFullScreen
478 stub WOATerminateProcess
479 pascal KERNEL_479(long) VWin32_EventSet  # ???
480 pascal -ret16 _EnterWin16Lock() _EnterWin16Lock
481 pascal -ret16 _LeaveWin16Lock() _LeaveWin16Lock
482 pascal LoadSystemLibrary32(str) LoadLibrary32_16   # FIXME!
483 pascal MapProcessHandle(long) MapProcessHandle
484 pascal SetProcessDword(long s_word long) SetProcessDword
485 pascal GetProcessDword(long s_word) GetProcessDword
486 pascal FreeLibrary32(long) FreeLibrary32_16
487 pascal GetModuleFileName32(long str word) GetModuleFileName32_16
488 pascal GetModuleHandle32(str) GetModuleHandle32_16
489 stub KERNEL_489  # VWin32_BoostWithDecay
490 pascal -ret16 KERNEL_490(word) KERNEL_490
491 pascal RegisterServiceProcess(long long) RegisterServiceProcess16
492 stub WOAAbort
493 pascal -ret16 UTInit(long long long long) UTInit16
494 stub KERNEL_494

# 495 is present only in Win98
495 pascal WaitForMultipleObjectsEx(long ptr long long long) WaitForMultipleObjectsEx16

# 500-544 are WinNT extensions; some are also available in Win95

500 varargs WOW16Call(word word word) WOW16Call
501 stub KDDBGOUT                                               # Both NT/95 (?)
502 stub WOWGETNEXTVDMCOMMAND
503 stub WOWREGISTERSHELLWINDOWHANDLE
504 stub WOWLOADMODULE
505 stub WOWQUERYPERFORMANCECOUNTER
506 stub WOWCURSORICONOP
#507 stub WOWCURSORICONOP # conflict with 506 !
507 stub WOWFAILEDEXEC
#508 stub WOWFAILEDEXEC # conflict with 507 ! (something broken here ?)
508 stub WOWCLOSECOMPORT
#509 stub WOWCLOSECOMPORT # conflict with 508 !
#509 stub WOWKILLREMOTETASK
511 stub WOWKILLREMOTETASK
512 stub WOWQUERYDEBUG
513 pascal LoadLibraryEx32W(ptr long long) LoadLibraryEx32W16   # Both NT/95
514 pascal FreeLibrary32W(long) FreeLibrary32W16                # Both NT/95
515 pascal GetProcAddress32W(long str) GetProcAddress32W16      # Both NT/95
516 pascal GetVDMPointer32W(segptr word) GetVDMPointer32W16     # Both NT/95
517 varargs CallProc32W(long long long) CallProc32W16           # Both NT/95
518 varargs _CallProcEx32W(long long long) CallProcEx32W16      # Both NT/95
519 stub EXITKERNELTHUNK
# the __MOD_ variables are WORD datareferences, the current values are invented.
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
541 stub WOWSETEXITONLASTAPP
544 stub WOWSETCOMPATHANDLE


# 531-568 are Win95-only extensions.
# NOTE: Ordinals 531,532,541 clash with the WinNT extensions given above! Argh!

#531 stub ConvertClipboardHandleLS
#532 stub ConvertClipboardHandleSL
533 stub ConvertDDEHandleLS
534 stub ConvertDDEHandleSL
535 pascal VWin32_BoostThreadGroup(long long) VWin32_BoostThreadGroup
536 pascal VWin32_BoostThreadStatic(long long) VWin32_BoostThreadStatic
537 stub KERNEL_537
538 stub ThunkTheTemplateHandle
540 stub KERNEL_540
#541 stub KERNEL_541
542 stub KERNEL_542
543 stub KERNEL_543
560 pascal SetThunkletCallbackGlue(long segptr) SetThunkletCallbackGlue16
561 pascal AllocLSThunkletCallback(segptr long) AllocLSThunkletCallback16
562 pascal AllocSLThunkletCallback(long long) AllocSLThunkletCallback16
563 pascal FindLSThunkletCallback(segptr long) FindLSThunkletCallback
564 pascal FindSLThunkletCallback(long long) FindSLThunkletCallback
566 stub KERNEL_566  # (thunklet) FIXME!!!
567 pascal AllocLSThunkletCallbackEx(segptr long word) AllocLSThunkletCallbackEx16
568 pascal AllocSLThunkletCallbackEx(long long word) AllocSLThunkletCallbackEx16


# 600-653 are Win95 only

600 stub AllocCodeAlias
601 stub FreeCodeAlias
602 pascal -ret16 GetDummyModuleHandleDS() GetDummyModuleHandleDS16
603 stub KERNEL_603  # OutputDebugString (?)
604 pascal -register CBClientGlueSL() CBClientGlueSL
# FIXME: 605 is duplicate of 562
605 pascal AllocSLThunkletCallback_dup(long long) AllocSLThunkletCallback16
# FIXME: 606 is duplicate of 561
606 pascal AllocLSThunkletCallback_dup(segptr long) AllocLSThunkletCallback16
607 pascal AllocLSThunkletSysthunk(segptr long long) AllocLSThunkletSysthunk16
608 pascal AllocSLThunkletSysthunk(long segptr long) AllocSLThunkletSysthunk16
# FIXME: 609 is duplicate of 563
609 pascal FindLSThunkletCallback_dup(segptr long) FindLSThunkletCallback
# FIXME: 610 is duplicate of 562
610 pascal FindSLThunkletCallback_dup(long long) FindSLThunkletCallback
611 pascal -ret16 FreeThunklet(long long) FreeThunklet16
612 pascal -ret16 IsSLThunklet(ptr) IsSLThunklet16
613 stub HugeMapLS
614 stub HugeUnMapLS
615 pascal -ret16 ConvertDialog32To16(long long long) ConvertDialog32To16
616 pascal -ret16 ConvertMenu32To16(long long long) ConvertMenu32To16
617 pascal -ret16 GetMenu32Size(ptr) GetMenu32Size16
618 pascal -ret16 GetDialog32Size(ptr) GetDialog32Size16
619 pascal -ret16 RegisterCBClient(word segptr long) RegisterCBClient16
620 pascal -register CBClientThunkSL() CBClientThunkSL
621 pascal -register CBClientThunkSLEx() CBClientThunkSLEx
622 pascal -ret16 UnRegisterCBClient(word segptr long) UnRegisterCBClient16
623 pascal -ret16 InitCBClient(long) InitCBClient16
624 pascal SetFastQueue(long long) SetFastQueue16
625 pascal GetFastQueue() GetFastQueue16
626 stub SmashEnvironment
627 pascal -ret16 IsBadFlatReadWritePtr(segptr long word) IsBadFlatReadWritePtr16
630 pascal -register C16ThkSL() C16ThkSL
631 pascal -register C16ThkSL01() C16ThkSL01
651 pascal ThunkConnect16(str str word long ptr str word) ThunkConnect16
652 stub IsThreadId
653 stub OkWithKernelToChangeUsers

# Extra Wine internal functions for thunking and self-loader

666 pascal UTGlue16(ptr long ptr long) UTGlue16
667 pascal EntryAddrProc(word word) EntryAddrProc16
668 pascal MyAlloc(word word word) MyAlloc16
669 pascal -ret16 DllEntryPoint(long word word word long word) KERNEL_DllEntryPoint

# 700-704 are Win95 only

700 pascal SSInit() SSInit16
701 stub SSOnBigStack
702 stub SSCall
703 stub CallProc32WFix
704 pascal -register SSConfirmSmallStack() SSConfirmSmallStack

# Win95 krnl386.exe also exports ordinals 802-864,
# however, those seem to be only callback stubs that are
# never called directly by other modules ...

################################################################
# VxD entry points
#
 901 pascal -register __wine_vxd_vmm() __wine_vxd_vmm
 905 pascal -register __wine_vxd_timer() __wine_vxd_timer
 909 pascal -register __wine_vxd_reboot() __wine_vxd_reboot
 910 pascal -register __wine_vxd_vdd() __wine_vxd_vdd
 912 pascal -register __wine_vxd_vmd() __wine_vxd_vmd
 914 pascal -register __wine_vxd_comm() __wine_vxd_comm
#915 pascal -register __wine_vxd_printer() __wine_vxd_printer
 923 pascal -register __wine_vxd_shell() __wine_vxd_shell
 933 pascal -register __wine_vxd_pagefile() __wine_vxd_pagefile
 938 pascal -register __wine_vxd_apm() __wine_vxd_apm
 939 pascal -register __wine_vxd_vxdloader() __wine_vxd_vxdloader
 945 pascal -register __wine_vxd_win32s() __wine_vxd_win32s
 951 pascal -register __wine_vxd_configmg() __wine_vxd_configmg
 955 pascal -register __wine_vxd_enable() __wine_vxd_enable
1990 pascal -register __wine_vxd_timerapi() __wine_vxd_timerapi

################################################################
# 32-bit version of the various 16-bit functions exported by kernel32
#
@ stdcall -arch=win32 -norelay VxDCall0() VxDCall
@ stdcall -arch=win32 -norelay VxDCall1() VxDCall
@ stdcall -arch=win32 -norelay VxDCall2() VxDCall
@ stdcall -arch=win32 -norelay VxDCall3() VxDCall
@ stdcall -arch=win32 -norelay VxDCall4() VxDCall
@ stdcall -arch=win32 -norelay VxDCall5() VxDCall
@ stdcall -arch=win32 -norelay VxDCall6() VxDCall
@ stdcall -arch=win32 -norelay VxDCall7() VxDCall
@ stdcall -arch=win32 -norelay VxDCall8() VxDCall
@ stdcall -arch=win32 k32CharToOemA(str ptr)
@ stdcall -arch=win32 k32CharToOemBuffA(str ptr long)
@ stdcall -arch=win32 k32OemToCharA(ptr ptr)
@ stdcall -arch=win32 k32OemToCharBuffA(ptr ptr long)
@ stdcall -arch=win32 k32LoadStringA(long long ptr long)
@ varargs -arch=win32 k32wsprintfA(str str)
@ stdcall -arch=win32 k32wvsprintfA(ptr str ptr)
@ stdcall -arch=win32 -norelay CommonUnimpStub()
@ stdcall -arch=win32 GetProcessDword(long long)
@ stdcall -arch=win32 DosFileHandleToWin32Handle(long)
@ stdcall -arch=win32 Win32HandleToDosFileHandle(long)
@ stdcall -arch=win32 DisposeLZ32Handle(long)
@ stdcall -arch=win32 GlobalAlloc16(long long)
@ stdcall -arch=win32 GlobalLock16(long)
@ stdcall -arch=win32 GlobalUnlock16(long)
@ stdcall -arch=win32 GlobalFix16(long)
@ stdcall -arch=win32 GlobalUnfix16(long)
@ stdcall -arch=win32 GlobalWire16(long)
@ stdcall -arch=win32 GlobalUnWire16(long)
@ stdcall -arch=win32 GlobalFree16(long)
@ stdcall -arch=win32 GlobalSize16(long)
@ stdcall -arch=win32 HouseCleanLogicallyDeadHandles()
@ stdcall -arch=win32 GetWin16DOSEnv()
@ stdcall -arch=win32 LoadLibrary16(str)
@ stdcall -arch=win32 FreeLibrary16(long)
@ stdcall -arch=win32 GetProcAddress16(long str) WIN32_GetProcAddress16
@ stdcall -arch=win32 -norelay AllocMappedBuffer()
@ stdcall -arch=win32 -norelay FreeMappedBuffer()
@ stdcall -arch=win32 -norelay OT_32ThkLSF()
@ stdcall -arch=win32 ThunkInitLSF(ptr str long str str)
@ stdcall -arch=win32 -norelay LogApiThkLSF(str)
@ stdcall -arch=win32 ThunkInitLS(ptr str long str str)
@ stdcall -arch=win32 -norelay LogApiThkSL(str)
@ stdcall -arch=win32 -norelay Common32ThkLS()
@ stdcall -arch=win32 ThunkInitSL(ptr str long str str)
@ stdcall -arch=win32 -norelay LogCBThkSL(str)
@ stdcall -arch=win32 ReleaseThunkLock(ptr)
@ stdcall -arch=win32 RestoreThunkLock(long)
@ stdcall -arch=win32 -norelay W32S_BackTo32()
@ stdcall -arch=win32 GetThunkBuff()
@ stdcall -arch=win32 GetThunkStuff(str str)
@ stdcall -arch=win32 K32WOWCallback16(long long)
@ stdcall -arch=win32 K32WOWCallback16Ex(long long long ptr ptr)
@ stdcall -arch=win32 K32WOWGetVDMPointer(long long long)
@ stdcall -arch=win32 K32WOWHandle32(long long)
@ stdcall -arch=win32 K32WOWHandle16(long long)
@ stdcall -arch=win32 K32WOWGlobalAlloc16(long long)
@ stdcall -arch=win32 K32WOWGlobalLock16(long)
@ stdcall -arch=win32 K32WOWGlobalUnlock16(long)
@ stdcall -arch=win32 K32WOWGlobalFree16(long)
@ stdcall -arch=win32 K32WOWGlobalAllocLock16(long long ptr)
@ stdcall -arch=win32 K32WOWGlobalUnlockFree16(long)
@ stdcall -arch=win32 K32WOWGlobalLockSize16(long ptr)
@ stdcall -arch=win32 K32WOWYield16()
@ stdcall -arch=win32 K32WOWDirectedYield16(long)
@ stdcall -arch=win32 K32WOWGetVDMPointerFix(long long long)
@ stdcall -arch=win32 K32WOWGetVDMPointerUnfix(long)
@ stdcall -arch=win32 K32WOWGetDescriptor(long ptr)
@ stdcall -arch=win32 _KERNEL32_86(ptr)
@ stdcall -arch=win32 SSOnBigStack()
@ varargs -arch=win32 SSCall(long long ptr)
@ stdcall -arch=win32 -norelay FT_PrologPrime()
@ stdcall -arch=win32 -norelay QT_ThunkPrime()
@ stdcall -arch=win32 PK16FNF(ptr)
@ stdcall -arch=win32 GetPK16SysVar()
@ stdcall -arch=win32 GetpWin16Lock(ptr)
@ stdcall -arch=win32 _CheckNotSysLevel(ptr)
@ stdcall -arch=win32 _ConfirmSysLevel(ptr)
@ stdcall -arch=win32 _ConfirmWin16Lock()
@ stdcall -arch=win32 _EnterSysLevel(ptr)
@ stdcall -arch=win32 _LeaveSysLevel(ptr)
@ stdcall -arch=win32 _KERNEL32_99(long)
@ stdcall -arch=win32 _KERNEL32_100(long long long)

@ stdcall -arch=win32 AllocSLCallback(long long)
@ stdcall -arch=win32 -norelay FT_Exit0()
@ stdcall -arch=win32 -norelay FT_Exit12()
@ stdcall -arch=win32 -norelay FT_Exit16()
@ stdcall -arch=win32 -norelay FT_Exit20()
@ stdcall -arch=win32 -norelay FT_Exit24()
@ stdcall -arch=win32 -norelay FT_Exit28()
@ stdcall -arch=win32 -norelay FT_Exit32()
@ stdcall -arch=win32 -norelay FT_Exit36()
@ stdcall -arch=win32 -norelay FT_Exit40()
@ stdcall -arch=win32 -norelay FT_Exit44()
@ stdcall -arch=win32 -norelay FT_Exit48()
@ stdcall -arch=win32 -norelay FT_Exit4()
@ stdcall -arch=win32 -norelay FT_Exit52()
@ stdcall -arch=win32 -norelay FT_Exit56()
@ stdcall -arch=win32 -norelay FT_Exit8()
@ stdcall -arch=win32 -norelay FT_Prolog()
@ stdcall -arch=win32 -norelay FT_Thunk()
@ stdcall -arch=win32 FreeSLCallback(long)
@ stdcall -arch=win32 Get16DLLAddress(long str)
@ stdcall -arch=win32 -norelay K32Thk1632Epilog()
@ stdcall -arch=win32 -norelay K32Thk1632Prolog()
@ stdcall -arch=win32 -norelay MapHInstLS()
@ stdcall -arch=win32 -norelay MapHInstLS_PN()
@ stdcall -arch=win32 -norelay MapHInstSL()
@ stdcall -arch=win32 -norelay MapHInstSL_PN()
@ stdcall -arch=win32 MapHModuleLS(long)
@ stdcall -arch=win32 MapHModuleSL(long)
@ stdcall -arch=win32 MapLS(ptr)
@ stdcall -arch=win32 MapSL(long)
@ stdcall -arch=win32 MapSLFix(long)
@ stdcall -arch=win32 PrivateFreeLibrary(long)
@ stdcall -arch=win32 PrivateLoadLibrary(str)
@ stdcall -arch=win32 -norelay QT_Thunk()
@ stdcall -arch=win32 -norelay SMapLS()
@ stdcall -arch=win32 -norelay SMapLS_IP_EBP_12()
@ stdcall -arch=win32 -norelay SMapLS_IP_EBP_16()
@ stdcall -arch=win32 -norelay SMapLS_IP_EBP_20()
@ stdcall -arch=win32 -norelay SMapLS_IP_EBP_24()
@ stdcall -arch=win32 -norelay SMapLS_IP_EBP_28()
@ stdcall -arch=win32 -norelay SMapLS_IP_EBP_32()
@ stdcall -arch=win32 -norelay SMapLS_IP_EBP_36()
@ stdcall -arch=win32 -norelay SMapLS_IP_EBP_40()
@ stdcall -arch=win32 -norelay SMapLS_IP_EBP_8()
@ stdcall -arch=win32 -norelay SUnMapLS()
@ stdcall -arch=win32 -norelay SUnMapLS_IP_EBP_12()
@ stdcall -arch=win32 -norelay SUnMapLS_IP_EBP_16()
@ stdcall -arch=win32 -norelay SUnMapLS_IP_EBP_20()
@ stdcall -arch=win32 -norelay SUnMapLS_IP_EBP_24()
@ stdcall -arch=win32 -norelay SUnMapLS_IP_EBP_28()
@ stdcall -arch=win32 -norelay SUnMapLS_IP_EBP_32()
@ stdcall -arch=win32 -norelay SUnMapLS_IP_EBP_36()
@ stdcall -arch=win32 -norelay SUnMapLS_IP_EBP_40()
@ stdcall -arch=win32 -norelay SUnMapLS_IP_EBP_8()
@ stdcall -arch=win32 ThunkConnect32(ptr str str str ptr long)
@ stdcall -arch=win32 UTRegister(long str str str ptr ptr ptr)
@ stdcall -arch=win32 UTUnRegister(long)
@ stdcall -arch=win32 UnMapLS(long)
@ stdcall -arch=win32 -norelay UnMapSLFixArray(long long)
@ stdcall -arch=win32 _lclose16(long)

################################################################
# 16-bit symbols not available from kernel32 but used by other 16-bit dlls
#
@ stdcall -arch=win32 AllocSelectorArray16(long)
@ stdcall -arch=win32 FarGetOwner16(long)
@ stdcall -arch=win32 FarSetOwner16(long long)
@ stdcall -arch=win32 FindResource16(long str str)
@ stdcall -arch=win32 FreeResource16(long)
@ stdcall -arch=win32 FreeSelector16(long)
@ stdcall -arch=win32 GetCurrentPDB16()
@ stdcall -arch=win32 GetCurrentTask()
@ stdcall -arch=win32 GetDOSEnvironment16()
@ stdcall -arch=win32 GetExePtr(long)
@ stdcall -arch=win32 GetExeVersion16()
@ stdcall -arch=win32 GetExpWinVer16(long)
@ stdcall -arch=win32 GetModuleHandle16(str)
@ stdcall -arch=win32 GetSelectorBase(long)
@ stdcall -arch=win32 GetSelectorLimit16(long)
@ stdcall -arch=win32 GlobalReAlloc16(long long long)
@ stdcall -arch=win32 InitTask16(ptr)
@ stdcall -arch=win32 IsBadReadPtr16(long long)
@ stdcall -arch=win32 IsTask16(long)
@ stdcall -arch=win32 LoadModule16(str ptr)
@ stdcall -arch=win32 LoadResource16(long long)
@ stdcall -arch=win32 LocalAlloc16(long long)
@ stdcall -arch=win32 LocalInit16(long long long)
@ stdcall -arch=win32 LocalLock16(long)
@ stdcall -arch=win32 LocalUnlock16(long)
@ stdcall -arch=win32 LocalReAlloc16(long long long)
@ stdcall -arch=win32 LocalFree16(long)
@ stdcall -arch=win32 LocalSize16(long)
@ stdcall -arch=win32 LocalCompact16(long)
@ stdcall -arch=win32 LocalCountFree16()
@ stdcall -arch=win32 LocalHeapSize16()
@ stdcall -arch=win32 LockResource16(long)
@ stdcall -arch=win32 PrestoChangoSelector16(long long)
@ stdcall -arch=win32 SetSelectorBase(long long)
@ stdcall -arch=win32 SetSelectorLimit16(long long)
@ stdcall -arch=win32 SizeofResource16(long long)
@ stdcall -arch=win32 WinExec16(str long)

################################################################
# Wine internal extensions
#
# All functions must be prefixed with '__wine_' (for internal functions)
# or 'wine_' (for user-visible functions) to avoid namespace conflicts.

# DOS support
2000 pascal -register __wine_call_int_handler(word) __wine_call_int_handler16
@ stdcall -arch=win32 __wine_call_int_handler16(long ptr)

# VxDs
@ cdecl -arch=win32 -private __wine_vxd_open(wstr long ptr)
@ cdecl -arch=win32 -private __wine_vxd_get_proc(long)

# Snoop support
2001 pascal -register __wine_snoop_entry()
2002 pascal -register __wine_snoop_return()
