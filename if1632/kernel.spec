name	kernel
type	win16

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
29  pascal16 Yield() Yield
30  pascal16 WaitEvent(word) WaitEvent
31  pascal16 PostEvent(word) PostEvent
32  pascal16 SetPriority(word s_word) SetPriority
33  pascal16 LockCurrentTask(word) LockCurrentTask
34  pascal SetTaskQueue(word word) SetTaskQueue
35  pascal16 GetTaskQueue(word) GetTaskQueue
36  pascal   GetCurrentTask() WIN16_GetCurrentTask
37  pascal GetCurrentPDB() GetCurrentPDB
38  pascal   SetTaskSignalProc(word segptr) SetTaskSignalProc
41  return EnableDos 0 0
42  return DisableDos 0 0
45  pascal16 LoadModule(ptr ptr) LoadModule16
46  pascal16 FreeModule(word) FreeModule16
47  pascal16 GetModuleHandle(segptr) WIN16_GetModuleHandle
48  pascal16 GetModuleUsage(word) GetModuleUsage
49  pascal16 GetModuleFileName(word ptr s_word) GetModuleFileName16
50  pascal GetProcAddress(word segptr) GetProcAddress16
51  pascal MakeProcInstance(segptr word) MakeProcInstance16
52  pascal16 FreeProcInstance(segptr) FreeProcInstance16
53  stub CallProcInstance
54  pascal16 GetInstanceData(word word word) GetInstanceData
55  pascal16 Catch(ptr) Catch 
56  pascal16 Throw(ptr word) Throw
57  pascal16 GetProfileInt(ptr ptr s_word) GetProfileInt16
58  pascal16 GetProfileString(ptr ptr ptr ptr word) GetProfileString16
59  pascal16 WriteProfileString(ptr ptr ptr) WriteProfileString16
60  pascal16 FindResource(word segptr segptr) FindResource16
61  pascal16 LoadResource(word word) LoadResource16
62  pascal LockResource(word) WIN16_LockResource16
63  pascal16 FreeResource(word) FreeResource16
64  pascal16 AccessResource(word word) AccessResource16
65  pascal SizeofResource(word word) SizeofResource16
66  pascal16 AllocResource(word word long) AllocResource16
67  pascal SetResourceHandler(word ptr ptr) SetResourceHandler
68  pascal16 InitAtomTable(word) InitAtomTable16
69  pascal16 FindAtom(segptr) FindAtom16
70  pascal16 AddAtom(segptr) AddAtom16
71  pascal16 DeleteAtom(word) DeleteAtom16
72  pascal16 GetAtomName(word ptr word) GetAtomName16
73  pascal16 GetAtomHandle(word) GetAtomHandle
74  pascal16 OpenFile(ptr ptr word) OpenFile16
75  stub OpenPathName
76  stub DeletePathName
77 stub KERNEL_77		#RESERVED1
78 stub KERNEL_78 		#RESERVED2
#79 RESERVED3
#80 RESERVED4
81  pascal16 _lclose(word) _lclose16
82  pascal16 _lread(word segptr word) WIN16_lread
83  pascal16 _lcreat(ptr word) _lcreat16
84  pascal   _llseek(word long word) _llseek16
85  pascal16 _lopen(ptr word) _lopen16
86  pascal16 _lwrite(word ptr word) _lwrite16
87  pascal16 RESERVED5(ptr ptr) lstrcmp16
88  pascal   lstrcpy(segptr segptr) lstrcpy16
89  pascal   lstrcat(segptr segptr) lstrcat16
90  pascal16 lstrlen(ptr) lstrlen16
91  register InitTask() InitTask
92  pascal   GetTempDrive(byte) WIN16_GetTempDrive
93  pascal16 GetCodeHandle(segptr) GetCodeHandle
94  stub DefineHandleTable
95  pascal16 LoadLibrary(ptr) LoadLibrary16
96  pascal16 FreeLibrary(word) FreeLibrary16
97  pascal16 GetTempFileName(byte ptr word ptr) GetTempFileName16
98  return GetLastDiskChange 0 0
99  stub GetLPErrMode
100 return ValidateCodeSegments 0 0
101 stub NoHookDosCall
102 register DOS3Call() DOS3Call
103 register NetBIOSCall() NetBIOSCall
104 stub GetCodeInfo
105 pascal16 GetExeVersion() GetExeVersion
106 pascal SetSwapAreaSize(word) SetSwapAreaSize
107 pascal16 SetErrorMode(word) SetErrorMode16
108 pascal16 SwitchStackTo(word word word) SwitchStackTo
109 register SwitchStackBack() SwitchStackBack
110 pascal16 PatchCodeHandle(word) PatchCodeHandle
111 pascal   GlobalWire(word) GlobalWire16
112 pascal16 GlobalUnWire(word) GlobalUnWire16
113 equate __AHSHIFT 3
114 equate __AHINCR 8
115 pascal16 OutputDebugString(ptr) OutputDebugString16
116 stub InitLib
117 pascal16 OldYield() OldYield
118 register GetTaskQueueDS() GetTaskQueueDS
119 register GetTaskQueueES() GetTaskQueueES
120 stub UndefDynLink
121 pascal16 LocalShrink(word word) LocalShrink16
122 pascal16 IsTaskLocked() IsTaskLocked
123 stub KbdRst
124 return EnableKernel 0 0
125 return DisableKernel 0 0
126 stub MemoryFreed
127 pascal16 GetPrivateProfileInt(ptr ptr s_word ptr) GetPrivateProfileInt16
128 pascal16 GetPrivateProfileString(ptr ptr ptr ptr word ptr)
             GetPrivateProfileString16
129 pascal16 WritePrivateProfileString(ptr ptr ptr ptr)
             WritePrivateProfileString16
130 pascal FileCDR(ptr) FileCDR
131 pascal GetDOSEnvironment() GetDOSEnvironment
132 pascal GetWinFlags() GetWinFlags
133 pascal16 GetExePtr(word) GetExePtr
134 pascal16 GetWindowsDirectory(ptr word) GetWindowsDirectory16
135 pascal16 GetSystemDirectory(ptr word) GetSystemDirectory16
136 pascal16 GetDriveType(byte) GetDriveType16
137 pascal16 FatalAppExit(word ptr) FatalAppExit16
138 pascal GetHeapSpaces(word) GetHeapSpaces
139 stub DoSignal
140 pascal16 SetSigHandler(segptr ptr ptr word word) SetSigHandler
141 stub InitTask1
142 stub GetProfileSectionNames
143 stub GetPrivateProfileSectionNames
144 pascal16 CreateDirectory(ptr ptr) CreateDirectory16
145 pascal16 RemoveDirectory(ptr) RemoveDirectory16
146 pascal16 DeleteFile(ptr) DeleteFile16
147 pascal16 SetLastError(long) SetLastError
148 pascal   GetLastError() GetLastError
149 stub GetVersionEx
150 pascal16 DirectedYield(word) DirectedYield
151 stub WinOldApCall
152 pascal16 GetNumTasks() GetNumTasks
154 pascal16 GlobalNotify(segptr) GlobalNotify
155 pascal16 GetTaskDS() GetTaskDS
156 return LimitEMSPages 4 0
157 return GetCurPID 4 0
158 return IsWinOldApTask 2 0
159 stub GlobalHandleNoRIP
160 stub EMSCopy
161 pascal16 LocalCountFree() LocalCountFree
162 pascal16 LocalHeapSize() LocalHeapSize
163 pascal16 GlobalLRUOldest(word) GlobalLRUOldest
164 pascal16 GlobalLRUNewest(word) GlobalLRUNewest
165 return A20Proc 2 0
166 pascal16 WinExec(ptr word) WinExec16
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
203 register DebugBreak() DebugBreak16
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
217 pascal   RegOpenKey(long ptr ptr) RegOpenKey16
218 pascal   RegCreateKey(long ptr ptr) RegCreateKey16
219 pascal   RegDeleteKey(long ptr) RegDeleteKey16
220 pascal   RegCloseKey(long) RegCloseKey
221 pascal   RegSetValue(long ptr long ptr long) RegSetValue16
222 pascal   RegDeleteValue(long ptr) RegDeleteValue16
223 pascal   RegEnumValue(long long ptr ptr ptr ptr ptr ptr) RegEnumValue16
224 pascal   RegQueryValue(long ptr ptr ptr) RegQueryValue16
225 pascal   RegQueryValueEx(long ptr ptr ptr ptr ptr) RegQueryValueEx16
226 pascal   RegSetValueEx(long ptr long long ptr long) RegSetValueEx16
227 pascal   RegFlushKey(long) RegFlushKey
228 stub K228
229 stub K229
230 pascal GlobalSmartPageLock(word) GlobalPageLock #?
231 stub GlobalSmartPageUnlock
232 stub RegLoadKey
233 stub RegUnloadKey
234 stub RegSaveKey
235 stub InvalidateNlsCache
237 stub KERNEL_237
262 stub KERNEL_262
263 stub KERNEL_263
310 pascal16 LocalHandleDelta(word) LocalHandleDelta
311 stub GetSetKernelDosProc
314 stub DebugDefineSegment
315 pascal16 WriteOutProfiles() WriteOutProfiles
316 stub GetFreeMemInfo
318 stub FatalExitHook
319 stub FlushCachedFileHandle
320 pascal16 IsTask(word) IsTask
323 return IsRomModule 2 0
324 pascal16 LogError(word ptr) LogError
325 pascal16 LogParamError(word ptr ptr) LogParamError
326 return IsRomFile 2 0
327 stub KERNEL_327
328 stub _DebugOutput
#329 K329
#332 stub THHOOK
334 pascal16 IsBadReadPtr(segptr word) IsBadReadPtr16
335 pascal16 IsBadWritePtr(segptr word) IsBadWritePtr16
336 pascal16 IsBadCodePtr(segptr) IsBadCodePtr16
337 pascal16 IsBadStringPtr(segptr word) IsBadStringPtr16
338 stub HasGPHandler
339 stub DiagQuery
340 stub DiagOutput
341 stub ToolHelpHook
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
352 pascal   lstrcatn(segptr segptr word) lstrcatn16
353 pascal   lstrcpyn(segptr segptr word) lstrcpyn16
354 pascal   GetAppCompatFlags(word) GetAppCompatFlags16
355 pascal16 GetWinDebugInfo(ptr word) GetWinDebugInfo
356 pascal16 SetWinDebugInfo(ptr) SetWinDebugInfo
357 stub KERNEL_357
358 stub KERNEL_358
359 stub KERNEL_359
360 stub OpenFileEx
#361 PIGLET_361
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
431 stub KERNEL_431
432 stub FileTimeToLocalFileTime
435 stub KERNEL_435
439 stub KERNEL_439
440 stub KERNEL_440
444 stub KERNEL_444
445 stub KERNEL_445
446 stub KERNEL_446
447 stub KERNEL_447
449 stub KERNEL_449
450 pascal16 KERNEL_450() stub_KERNEL_450
454 stub KERNEL_454
455 stub KERNEL_455
471 stub KERNEL_471
472 stub KERNEL_472
473 stub KERNEL_473
482 stub KERNEL_482
491 stub RegisterServiceProcess
500 stub KERNEL_500
502 stub KERNEL_502
503 stub KERNEL_503
511 stub KERNEL_511
513 pascal LoadLibraryEx32W(ptr long long) LoadLibraryEx32W16
514 stub FreeLibrary32W
515 pascal GetProcAddress32W(long ptr) GetProcAddress32
516 stub GetVDMPointer32W
517 pascal CallProc32W() WIN16_CallProc32W
518 stub CallProcEx32W
519 stub KERNEL_519
522 stub KERNEL_522
525 stub KERNEL_525
600 stub KERNEL_600
601 stub KERNEL_601
604 stub KERNEL_604
605 stub KERNEL_605
612 stub KERNEL_612
613 stub KERNEL_613
614 stub KERNEL_614
619 stub KERNEL_619
621 stub KERNEL_621
627 stub IsBadFlatReadWritePtr
630 stub KERNEL_630
631 stub FUNC004	# shell hook
651 stub KERNEL_651
700 return KERNEL_700 0 1
