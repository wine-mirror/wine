name	kernel
type	win16
file	krnl386.exe

# 1-207 are the basic functions, those are (with minor variations)
# present in win31, win95 and nt351

1   stub FatalExit
2   stub ExitKernel
3   pascal GetVersion() GetVersion16
4   pascal16 LocalInit(word word word) LocalInit16
5   register LocalAlloc(word word) WIN16_LocalAlloc
6   pascal16 LocalReAlloc(word word word) LocalReAlloc16
7   pascal16 LocalFree(word) LocalFree16
8   pascal LocalLock(word) LocalLock16
9   pascal16 LocalUnlock(word) LocalUnlock16
10  pascal16 LocalSize(word) LocalSize16
11  pascal16 LocalHandle(word) LocalHandle16
12  pascal16 LocalFlags(word) LocalFlags16
13  pascal16 LocalCompact(word) LocalCompact16
14  pascal LocalNotify(long) LocalNotify16
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
26  pascal16 GlobalFreeAll(word) GlobalFreeAll16
27  pascal16 GetModuleName(word ptr word) GetModuleName16
28  pascal   GlobalMasterHandle() GlobalMasterHandle16
29  pascal16 Yield() Yield16
30  pascal16 WaitEvent(word) WaitEvent16
31  pascal16 PostEvent(word) PostEvent16
32  pascal16 SetPriority(word s_word) SetPriority16
33  pascal16 LockCurrentTask(word) LockCurrentTask16
34  pascal16 SetTaskQueue(word word) SetTaskQueue16
35  pascal16 GetTaskQueue(word) GetTaskQueue16
36  pascal   GetCurrentTask() WIN16_GetCurrentTask
37  pascal GetCurrentPDB() GetCurrentPDB16
38  pascal   SetTaskSignalProc(word segptr) THUNK_SetTaskSignalProc
41  return EnableDos 0 0
42  return DisableDos 0 0
45  pascal16 LoadModule(str ptr) LoadModule16
46  pascal16 FreeModule(word) FreeModule16
47  pascal   GetModuleHandle(segstr) WIN16_GetModuleHandle
48  pascal16 GetModuleUsage(word) GetModuleUsage16
49  pascal16 GetModuleFileName(word ptr s_word) GetModuleFileName16
50  pascal GetProcAddress(word segstr) GetProcAddress16
51  pascal MakeProcInstance(segptr word) MakeProcInstance16
52  pascal16 FreeProcInstance(segptr) FreeProcInstance16
53  stub CallProcInstance
54  pascal16 GetInstanceData(word word word) GetInstanceData16
55  register Catch(segptr) Catch16 
56  register Throw(segptr word) Throw16
57  pascal16 GetProfileInt(str str s_word) GetProfileInt16
58  pascal16 GetProfileString(str str str ptr word) GetProfileString16
59  pascal16 WriteProfileString(str str str) WriteProfileString16
60  pascal16 FindResource(word segstr segstr) FindResource16
61  pascal16 LoadResource(word word) LoadResource16
62  pascal LockResource(word) WIN16_LockResource16
63  pascal16 FreeResource(word) FreeResource16
64  pascal16 AccessResource(word word) AccessResource16
65  pascal SizeofResource(word word) SizeofResource16
66  pascal16 AllocResource(word word long) AllocResource16
67  pascal SetResourceHandler(word segstr segptr) SetResourceHandler16
68  pascal16 InitAtomTable(word) InitAtomTable16
69  pascal16 FindAtom(segstr) FindAtom16
70  pascal16 AddAtom(segstr) AddAtom16
71  pascal16 DeleteAtom(word) DeleteAtom16
72  pascal16 GetAtomName(word ptr word) GetAtomName16
73  pascal16 GetAtomHandle(word) GetAtomHandle16
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
91  register InitTask() InitTask16
92  pascal   GetTempDrive(word) WIN16_GetTempDrive
93  pascal16 GetCodeHandle(segptr) GetCodeHandle16
94  pascal16 DefineHandleTable(word) DefineHandleTable16
95  pascal16 LoadLibrary(str) LoadLibrary16
96  pascal16 FreeLibrary(word) FreeLibrary16
97  pascal16 GetTempFileName(word str word ptr) GetTempFileName16
98  return GetLastDiskChange 0 0
99  stub GetLPErrMode
100 return ValidateCodeSegments 0 0
101 stub NoHookDosCall
102 register DOS3Call() DOS3Call
103 register NetBIOSCall() NetBIOSCall16
104 pascal16 GetCodeInfo(segptr ptr) GetCodeInfo16
105 pascal16 GetExeVersion() GetExeVersion16
106 pascal SetSwapAreaSize(word) SetSwapAreaSize16
107 pascal16 SetErrorMode(word) SetErrorMode16
108 pascal16 SwitchStackTo(word word word) SwitchStackTo16
109 register SwitchStackBack() SwitchStackBack16
110 pascal   PatchCodeHandle(word) PatchCodeHandle16
111 pascal   GlobalWire(word) GlobalWire16
112 pascal16 GlobalUnWire(word) GlobalUnWire16
113 equate __AHSHIFT 3
114 equate __AHINCR 8
115 pascal16 OutputDebugString(str) OutputDebugString16
116 stub InitLib
117 pascal16 OldYield() OldYield16
118 register GetTaskQueueDS() GetTaskQueueDS16
119 register GetTaskQueueES() GetTaskQueueES16
120 stub UndefDynLink
121 pascal16 LocalShrink(word word) LocalShrink16
122 pascal16 IsTaskLocked() IsTaskLocked16
123 return KbdRst 0 0
124 return EnableKernel 0 0
125 return DisableKernel 0 0
126 stub MemoryFreed
127 pascal16 GetPrivateProfileInt(str str s_word str) GetPrivateProfileInt16
128 pascal16 GetPrivateProfileString(str str str ptr word str)
             GetPrivateProfileString16
129 pascal16 WritePrivateProfileString(str str str str)
             WritePrivateProfileString16
130 pascal FileCDR(ptr) FileCDR16
131 pascal GetDOSEnvironment() GetDOSEnvironment16
132 pascal GetWinFlags() GetWinFlags16
133 register GetExePtr(word) WIN16_GetExePtr
134 pascal16 GetWindowsDirectory(ptr word) GetWindowsDirectory16
135 pascal16 GetSystemDirectory(ptr word) GetSystemDirectory16
136 pascal16 GetDriveType(word) GetDriveType16
137 pascal16 FatalAppExit(word str) FatalAppExit16
138 pascal GetHeapSpaces(word) GetHeapSpaces16
139 stub DoSignal
140 pascal16 SetSigHandler(segptr ptr ptr word word) SetSigHandler16
141 stub InitTask1
142 pascal16 GetProfileSectionNames(ptr word) GetProfileSectionNames16
143 pascal16 GetPrivateProfileSectionNames(ptr word str) GetPrivateProfileSectionNames16
144 pascal16 CreateDirectory(ptr ptr) CreateDirectory16
145 pascal16 RemoveDirectory(ptr) RemoveDirectory16
146 pascal16 DeleteFile(ptr) DeleteFile16
147 pascal16 SetLastError(long) SetLastError
148 pascal   GetLastError() GetLastError
149 pascal16 GetVersionEx(ptr) GetVersionEx16
150 pascal16 DirectedYield(word) DirectedYield16
151 stub WinOldApCall
152 pascal16 GetNumTasks() GetNumTasks16
154 pascal16 GlobalNotify(segptr) GlobalNotify16
155 pascal16 GetTaskDS() GetTaskDS16
156 return LimitEMSPages 4 0
157 return GetCurPID 4 0
158 return IsWinOldApTask 2 0
159 pascal GlobalHandleNoRIP(word) GlobalHandleNoRIP16
160 stub EMSCopy
161 pascal16 LocalCountFree() LocalCountFree16
162 pascal16 LocalHeapSize() LocalHeapSize16
163 pascal16 GlobalLRUOldest(word) GlobalLRUOldest16
164 pascal16 GlobalLRUNewest(word) GlobalLRUNewest16
165 return A20Proc 2 0
166 pascal16 WinExec(str word) WinExec16
167 pascal16 GetExpWinVer(word) GetExpWinVer16
168 pascal16 DirectResAlloc(word word word) DirectResAlloc16
169 pascal GetFreeSpace(word) GetFreeSpace16
170 pascal16 AllocCStoDSAlias(word) AllocCStoDSAlias16
171 pascal16 AllocDStoCSAlias(word) AllocDStoCSAlias16
172 pascal16 AllocAlias(word) AllocCStoDSAlias16
173 equate __ROMBIOS 0
174 equate __A000H 0
175 pascal16 AllocSelector(word) AllocSelector16
176 pascal16 FreeSelector(word) FreeSelector16
177 pascal16 PrestoChangoSelector(word word) PrestoChangoSelector16
178 equate __WINFLAGS 0x413
179 equate __D000H 0
180 pascal16 LongPtrAdd(long long) LongPtrAdd16
181 equate __B000H 0
182 equate __B800H 0
183 equate __0000H 0
184 pascal GlobalDOSAlloc(long) GlobalDOSAlloc16
185 pascal16 GlobalDOSFree(word) GlobalDOSFree16
186 pascal GetSelectorBase(word) WIN16_GetSelectorBase
187 pascal16 SetSelectorBase(word long) WIN16_SetSelectorBase
188 pascal GetSelectorLimit(word) GetSelectorLimit16
189 pascal16 SetSelectorLimit(word long) SetSelectorLimit16
190 equate __E000H 0
191 pascal16 GlobalPageLock(word) GlobalPageLock16
192 pascal16 GlobalPageUnlock(word) GlobalPageUnlock16
193 equate __0040H 0
194 equate __F000H 0
195 equate __C000H 0
196 pascal16 SelectorAccessRights(word word word) SelectorAccessRights16
197 pascal16 GlobalFix(word) GlobalFix16
198 pascal16 GlobalUnfix(word) GlobalUnfix16
199 pascal16 SetHandleCount(word) SetHandleCount16
200 return ValidateFreeSpaces 0 0
201 stub ReplaceInst
202 stub RegisterPtrace
203 register DebugBreak() DebugBreak16
204 stub SwapRecording
205 stub CVWBreak
206 pascal16 AllocSelectorArray(word) AllocSelectorArray16
207 pascal16 IsDBCSLeadByte(word) IsDBCSLeadByte16


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
220 pascal RegCloseKey(long) RegCloseKey                                  # Both 95/NT
221 pascal RegSetValue(long str long ptr long) RegSetValue16
222 pascal RegDeleteValue(long str) RegDeleteValue16
223 pascal RegEnumValue(long long ptr ptr ptr ptr ptr ptr) RegEnumValue16 # Both 95/NT
224 pascal RegQueryValue(long str ptr ptr) RegQueryValue16
225 pascal RegQueryValueEx(long str ptr ptr ptr ptr) RegQueryValueEx16
226 pascal RegSetValueEx(long str long long ptr long) RegSetValueEx16
227 pascal RegFlushKey(long) RegFlushKey
228 pascal16 K228(word) GetExePtr
229 pascal16 K229(long) Local32GetSegment16
230 pascal GlobalSmartPageLock(word) GlobalPageLock16 #?
231 stub GlobalSmartPageUnlock
232 stub RegLoadKey
233 stub RegUnloadKey
234 stub RegSaveKey
235 stub InvalidateNlsCache
236 stub GetProductName
237 return K237 0 0


# 262-274 are WinNT extensions; those are not present in Win95

262 stub WOWWaitForMsgAndEvent
263 stub WOWMsgBox
273 stub K273
274 stub GetShortPathName


# 310-356 are again shared between all versions

310 pascal16 LocalHandleDelta(word) LocalHandleDelta16
311 pascal GetSetKernelDOSProc(ptr) GetSetKernelDOSProc16
314 stub DebugDefineSegment
315 pascal16 WriteOutProfiles() WriteOutProfiles16
316 pascal GetFreeMemInfo() GetFreeMemInfo16
318 stub FatalExitHook
319 stub FlushCachedFileHandle
320 pascal16 IsTask(word) IsTask16
323 return IsRomModule 2 0
324 pascal16 LogError(word ptr) LogError16
325 pascal16 LogParamError(word ptr ptr) LogParamError16
326 return IsRomFile 2 0
327 register K327() HandleParamError
328 stub _DebugOutput
329 pascal16 K329(str word) DebugFillBuffer
332 long THHOOK(0 0 0 0 0 0 0 0)
334 pascal16 IsBadReadPtr(segptr word) IsBadReadPtr16
335 pascal16 IsBadWritePtr(segptr word) IsBadWritePtr16
336 pascal16 IsBadCodePtr(segptr) IsBadCodePtr16
337 pascal16 IsBadStringPtr(segptr word) IsBadStringPtr16
338 pascal16 HasGPHandler(segptr) HasGPHandler16
339 pascal16 DiagQuery() DiagQuery16
340 pascal16 DiagOutput(str) DiagOutput16
341 pascal ToolHelpHook(ptr) ToolHelpHook16
342 word __GP(0 0 0 0)
343 stub RegisterWinOldApHook
344 stub GetWinOldApHooks
345 pascal16 IsSharedSelector(word) IsSharedSelector16
346 pascal16 IsBadHugeReadPtr(segptr long) IsBadHugeReadPtr16
347 pascal16 IsBadHugeWritePtr(segptr long) IsBadHugeWritePtr16
348 pascal16 hmemcpy(ptr ptr long) hmemcpy16
349 pascal   _hread(word segptr long) WIN16_hread
350 pascal   _hwrite(word ptr long) _hwrite16
351 stub BUNNY_351
352 pascal   lstrcatn(segstr str word) lstrcatn16
353 pascal   lstrcpyn(segptr str word) lstrcpyn16
354 pascal   GetAppCompatFlags(word) GetAppCompatFlags16
355 pascal16 GetWinDebugInfo(ptr word) GetWinDebugInfo16
356 pascal16 SetWinDebugInfo(ptr) SetWinDebugInfo16


# 357-365 are present in Win95 only
# Note that from here on most of the Win95-only functions are exported 
# ordinal-only; the names given here are mostly guesses :-)

357 pascal MapSL(segptr) MapSL
358 pascal MapLS(long) MapLS
359 pascal UnMapLS(segptr) UnMapLS
360 pascal16 OpenFileEx(str ptr word) OpenFile16
361 return PIGLET_361 0 0
362 stub ThunkTerminateProcess
365 register GlobalChangeLockCount(word word) GlobalChangeLockCount16


# 403-404 are common to all versions

403 pascal16 FarSetOwner(word word) FarSetOwner16
404 pascal16 FarGetOwner(word) FarGetOwner16


# 406-494 are present only in Win95

406 pascal16 WritePrivateProfileStruct(str str ptr word str) WritePrivateProfileStruct16
407 pascal16 GetPrivateProfileStruct(str str ptr word str) GetPrivateProfileStruct16
408 stub KERNEL_408
409 stub KERNEL_409
410 stub CreateProcessFromWinExec
411 pascal   GetCurrentDirectory(long ptr) GetCurrentDirectory16
412 pascal16 SetCurrentDirectory(ptr) SetCurrentDirectory16
413 pascal16 FindFirstFile(ptr ptr) FindFirstFile16
414 pascal16 FindNextFile(word ptr) FindNextFile16
415 pascal16 FindClose(word) FindClose16
416 pascal16 WritePrivateProfileSection(str str str) WritePrivateProfileSection16
417 pascal16 WriteProfileSection(str str) WriteProfileSection16
418 pascal16 GetPrivateProfileSection(str ptr word str) GetPrivateProfileSection16
419 pascal16 GetProfileSection(str ptr word) GetProfileSection16
420 pascal   GetFileAttributes(ptr) GetFileAttributes16
421 pascal16 SetFileAttributes(ptr long) SetFileAttributes16
422 pascal16 GetDiskFreeSpace(ptr ptr ptr ptr ptr) GetDiskFreeSpace16 
423 stub LogApiThk
431 pascal16 IsPeFormat(str word) IsPeFormat16
432 stub FileTimeToLocalFileTime
434 pascal16 UnicodeToAnsi(ptr ptr word) UnicodeToAnsi16
435 stub GetTaskFlags
436 stub _ConfirmSysLevel
437 stub _CheckNotSysLevel
438 stub _CreateSysLevel
439 stub _EnterSysLevel
440 stub _LeaveSysLevel
441 pascal CreateThread16(ptr long segptr segptr long ptr) THUNK_CreateThread16
442 pascal VWin32_EventCreate() VWin32_EventCreate
443 pascal VWin32_EventDestroy(long) VWin32_EventDestroy
444 pascal16 Local32Info(ptr word) Local32Info16
445 pascal16 Local32First(ptr word) Local32First16
446 pascal16 Local32Next(ptr) Local32Next16
447 return KERNEL_447 0 0
448 stub KERNEL_448
449 pascal GetpWin16Lock() GetpWin16Lock16
450 pascal VWin32_EventWait(long) VWin32_EventWait
451 pascal VWin32_EventSet(long) VWin32_EventSet
452 pascal LoadLibrary32(str) LoadLibraryA
453 pascal GetProcAddress32(long str) GetProcAddress32_16
454 equate __FLATCS 0   # initialized by BUILTIN_Init()
455 equate __FLATDS 0   # initialized by BUILTIN_Init()
456 pascal DefResourceHandler(word word word) NE_DefResourceHandler
457 pascal CreateW32Event(long long) WIN16_CreateEvent
458 pascal SetW32Event(long) SetEvent
459 pascal ResetW32Event(long) ResetEvent
460 pascal WaitForSingleObject(long long) WIN16_WaitForSingleObject
461 pascal WaitForMultipleObjects(long ptr long long) WIN16_WaitForMultipleObjects
462 pascal GetCurrentThreadId() GetCurrentThreadId
463 pascal SetThreadQueue(long word) SetThreadQueue16
464 pascal GetThreadQueue(long) GetThreadQueue16
465 stub NukeProcess
466 stub ExitProcess
467 stub WOACreateConsole
468 stub WOASpawnConApp
469 stub WOAGimmeTitle
470 stub WOADestroyConsole
471 pascal GetCurrentProcessId() GetCurrentProcessId
472 register MapHInstLS() WIN16_MapHInstLS
473 register MapHInstSL() WIN16_MapHInstSL
474 pascal CloseW32Handle(long) CloseHandle
475 register GetTEBSelectorFS() GetTEBSelectorFS16
476 pascal ConvertToGlobalHandle(long) ConvertToGlobalHandle
477 stub WOAFullScreen
478 stub WOATerminateProcess
479 pascal KERNEL_479(long) VWin32_EventSet  # ???
480 pascal16 _EnterWin16Lock() SYSLEVEL_EnterWin16Lock
481 pascal16 _LeaveWin16Lock() SYSLEVEL_LeaveWin16Lock
482 pascal LoadSystemLibrary32(str) LoadLibraryA   # FIXME!
483 stub MapProcessHandle
484 pascal SetProcessDWORD(long s_word long) SetProcessDword
485 pascal GetProcessDWORD(long s_word) GetProcessDword
486 pascal FreeLibrary32(long) FreeLibrary
487 pascal GetModuleFileName32(long str word) GetModuleFileNameA
488 pascal GetModuleHandle32(str) GetModuleHandleA
489 stub KERNEL_489  # VWin32_BoostWithDecay
490 pascal16 KERNEL_490(word) KERNEL_490
491 pascal RegisterServiceProcess(long long) RegisterServiceProcess
492 stub WOAAbort
493 pascal16 UTInit(long long long long) UTInit16
494 stub KERNEL_494

# 495 is present only in Win98
495 pascal WaitForMultipleObjectsEx(long ptr long long long) WIN16_WaitForMultipleObjectsEx

# 500-544 are WinNT extensions; some are also available in Win95

500 pascal WOW16Call(word word word) WOW16Call
501 stub KDDBGOUT                                               # Both NT/95 (?)
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
513 pascal   LoadLibraryEx32W(ptr long long) LoadLibraryEx32W16 # Both NT/95
514 pascal16 FreeLibrary32W(long) FreeLibrary                 # Both NT/95
515 pascal   GetProcAddress32W(long str) GetProcAddress       # Both NT/95
516 pascal   GetVDMPointer32W(segptr word) GetVDMPointer32W     # Both NT/95
517 pascal   CallProc32W() CallProc32W_16                    # Both NT/95
518 pascal   CallProcEx32W() CallProcEx32W_16                # Both NT/95
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
602 register GetDummyModuleHandleDS() GetDummyModuleHandleDS16
603 stub KERNEL_603  # OutputDebugString (?)
604 register CBClientGlueSL() CBClientGlueSL
605 pascal AllocSLThunkletCallback(long long) AllocSLThunkletCallback16
606 pascal AllocLSThunkletCallback(segptr long) AllocLSThunkletCallback16
607 pascal AllocLSThunkletSysthunk(segptr long long) AllocLSThunkletSysthunk16
608 pascal AllocSLThunkletSysthunk(long segptr long) AllocSLThunkletSysthunk16
609 pascal FindLSThunkletCallback(segptr long) FindLSThunkletCallback
610 pascal FindSLThunkletCallback(long long) FindSLThunkletCallback
611 return FreeThunklet 8 0
612 pascal16 IsSLThunklet(ptr) IsSLThunklet16
613 stub HugeMapLS
614 stub HugeUnMapLS
615 pascal16 ConvertDialog32To16(long long long) ConvertDialog32To16
616 pascal16 ConvertMenu32To16(long long long) ConvertMenu32To16
617 pascal16 GetMenu32Size(ptr) GetMenu32Size16
618 pascal16 GetDialog32Size(ptr) GetDialog32Size16
619 pascal16 RegisterCBClient(word ptr long) RegisterCBClient16
620 register CBClientThunkSL() CBClientThunkSL
621 register CBClientThunkSLEx() CBClientThunkSLEx
622 pascal16 UnRegisterCBClient(word ptr long) UnRegisterCBClient16
623 pascal16 InitCBClient(long) InitCBClient16
624 pascal SetFastQueue(long long) SetFastQueue16
625 pascal GetFastQueue() GetFastQueue16
626 stub SmashEnvironment
627 stub IsBadFlatReadWritePtr
630 register C16ThkSL() C16ThkSL
631 register C16ThkSL01() C16ThkSL01
651 pascal ThunkConnect16(str str word long ptr str word) ThunkConnect16
652 stub IsThreadId
653 stub OkWithKernelToChangeUsers


# 700-704 are Win95 only

700 pascal SSInit() SSInit16
701 stub SSOnBigStack
702 stub SSCall
703 stub CallProc32WFix
704 stub SSConfirmSmallStack


# Win95 krnl386.exe also exports ordinals 802-864,
# however, those seem to be only callback stubs that are
# never called directly by other modules ...

