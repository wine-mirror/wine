name	kernel
id	1

1   stub FatalExit
2   stub ExitKernel
3   pascal GetVersion() GetVersion
4   pascal16 LocalInit(word word word) LocalInit
5   pascal16 LocalAlloc(word word) LocalAlloc
6   pascal16 LocalReAlloc(word word word) LocalReAlloc
7   pascal16 LocalFree(word) LocalFree
8   pascal16 LocalLock(word) LocalLock
9   pascal16 LocalUnlock(word) LocalUnlock
10  pascal16 LocalSize(word) LocalSize
11  pascal16 LocalHandle(word) LocalHandle
12  pascal16 LocalFlags(word) LocalFlags
13  pascal16 LocalCompact(word) LocalCompact
14  pascal16 LocalNotify(long) LocalNotify
15  pascal16 GlobalAlloc(word long) GlobalAlloc
16  pascal16 GlobalReAlloc(word long word) GlobalReAlloc
17  pascal16 GlobalFree(word) GlobalFree
18  pascal GlobalLock(word) WIN16_GlobalLock
19  pascal16 GlobalUnlock(word) GlobalUnlock
20  pascal GlobalSize(word) GlobalSize
21  pascal GlobalHandle(word) GlobalHandle
22  pascal16 GlobalFlags(word) GlobalFlags
23  pascal16 LockSegment(word) LockSegment
24  pascal16 UnlockSegment(word) UnlockSegment
25  pascal GlobalCompact(long) GlobalCompact
26  pascal16 GlobalFreeAll(word) GlobalFreeAll
27  stub GetModuleName
28  stub GlobalMasterHandle
29  pascal16 Yield() Yield
30  pascal16 WaitEvent(word) WaitEvent
31  pascal16 PostEvent(word) PostEvent
32  pascal16 SetPriority(word s_word) SetPriority
33  pascal16 LockCurrentTask(word) LockCurrentTask
34  pascal SetTaskQueue(word word) SetTaskQueue
35  pascal16 GetTaskQueue(word) GetTaskQueue
36  pascal GetCurrentTask() GetCurrentTask
37  pascal GetCurrentPDB() GetCurrentPDB
38  stub SetTaskSignalProc
41  return EnableDos 0 0
42  return DisableDos 0 0
45  pascal16 LoadModule(ptr ptr) LoadModule
46  pascal16 FreeModule(word) FreeModule
47  pascal16 GetModuleHandle(segptr) WIN16_GetModuleHandle
48  pascal16 GetModuleUsage(word) GetModuleUsage
49  pascal16 GetModuleFileName(word ptr s_word) GetModuleFileName
50  pascal GetProcAddress(word segptr) GetProcAddress
51  pascal MakeProcInstance(segptr word) MakeProcInstance
52  pascal16 FreeProcInstance(segptr) FreeProcInstance
53  stub CallProcInstance
54  pascal16 GetInstanceData(word word word) GetInstanceData
55  pascal16 Catch(ptr) Catch 
56  pascal16 Throw(ptr word) Throw
57  pascal16 GetProfileInt(ptr ptr s_word) GetProfileInt
58  pascal16 GetProfileString(ptr ptr ptr ptr word) GetProfileString
59  pascal16 WriteProfileString(ptr ptr ptr) WriteProfileString
60  pascal16 FindResource(word segptr segptr) FindResource
61  pascal16 LoadResource(word word) LoadResource
62  pascal LockResource(word) WIN16_LockResource
63  pascal16 FreeResource(word) FreeResource
64  pascal16 AccessResource(word word) AccessResource
65  pascal SizeofResource(word word) SizeofResource
66  pascal16 AllocResource(word word long) AllocResource
67  stub SetResourceHandler
68  pascal16 InitAtomTable(word) InitAtomTable
69  pascal16 FindAtom(segptr) FindAtom
70  pascal16 AddAtom(segptr) AddAtom
71  pascal16 DeleteAtom(word) DeleteAtom
72  pascal16 GetAtomName(word ptr word) GetAtomName
73  pascal16 GetAtomHandle(word) GetAtomHandle
74  pascal16 OpenFile(ptr ptr word) OpenFile
75  stub OpenPathName
76  stub DeletePathName
#77 RESERVED1
#78 RESERVED2
#79 RESERVED3
#80 RESERVED4
81  pascal16 _lclose(word) _lclose
82  pascal16 _lread(word segptr word) _lread
83  pascal16 _lcreat(ptr word) _lcreat
84  pascal   _llseek(word long word) _llseek
85  pascal16 _lopen(ptr word) _lopen
86  pascal16 _lwrite(word ptr word) _lwrite
87  pascal16 RESERVED5(ptr ptr) lstrcmp
88  pascal lstrcpy(segptr segptr) lstrcpy
89  pascal lstrcat(segptr segptr) lstrcat
90  pascal16 lstrlen(ptr) lstrlen
91  register InitTask() InitTask
92  pascal16 GetTempDrive(byte) GetTempDrive
93  pascal16 GetCodeHandle(segptr) GetCodeHandle
94  stub DefineHandleTable
95  pascal16 LoadLibrary(ptr) LoadLibrary
96  pascal16 FreeLibrary(word) FreeLibrary
97  pascal16 GetTempFileName(byte ptr word ptr) GetTempFileName
98  stub GetLastDiskChange
99  stub GetLPErrMode
100 stub ValidateCodeSegments
101 stub NoHookDosCall
102 register DOS3Call() DOS3Call
103 register NetBIOSCall() NetBIOSCall
104 stub GetCodeInfo
105 stub GetExeVersion
106 pascal SetSwapAreaSize(word) SetSwapAreaSize
107 pascal16 SetErrorMode(word) SetErrorMode
108 stub SwitchStackTo
109 stub SwitchStackBack
110 pascal16 PatchCodeHandle(word) PatchCodeHandle
111 pascal GlobalWire(word) GlobalWire
112 pascal16 GlobalUnWire(word) GlobalUnWire
113 equate __AHSHIFT 3
114 equate __AHINCR 8
115 pascal OutputDebugString(ptr) OutputDebugString
116 stub InitLib
117 pascal16 OldYield() OldYield
118 register GetTaskQueueDS() GetTaskQueueDS
119 register GetTaskQueueES() GetTaskQueueES
120 stub UndefDynLink
121 pascal16 LocalShrink(word word) LocalShrink
122 pascal16 IsTaskLocked() IsTaskLocked
123 stub KbdRst
124 return EnableKernel 0 0
125 return DisableKernel 0 0
126 stub MemoryFreed
127 pascal16 GetPrivateProfileInt(ptr ptr s_word ptr) GetPrivateProfileInt
128 pascal16 GetPrivateProfileString(ptr ptr ptr ptr word ptr)
             GetPrivateProfileString
129 pascal16 WritePrivateProfileString(ptr ptr ptr ptr)
             WritePrivateProfileString
130 pascal FileCDR(ptr) FileCDR
131 pascal GetDOSEnvironment() GetDOSEnvironment
132 pascal GetWinFlags() GetWinFlags
133 pascal16 GetExePtr(word) GetExePtr
134 pascal16 GetWindowsDirectory(ptr word) GetWindowsDirectory
135 pascal16 GetSystemDirectory(ptr word) GetSystemDirectory
136 pascal16 GetDriveType(byte) GetDriveType
137 pascal FatalAppExit(word ptr) FatalAppExit
138 pascal GetHeapSpaces(word) GetHeapSpaces
139 stub DoSignal
140 stub SetSigHandler
141 stub InitTask1
142 stub GetProfileSectionNames
143 stub GetPrivateProfileSectionNames
144 stub CreateDirectory
145 stub RemoveDirectory
146 stub DeleteFile
147 stub SetLastError
148 stub GetLastError
149 stub GetVersionEx
150 pascal16 DirectedYield(word) DirectedYield
151 stub WinOldApCall
152 pascal16 GetNumTasks() GetNumTasks
154 return GlobalNotify 4 0
155 pascal16 GetTaskDS() GetTaskDS
156 stub LimitEMSPages
157 return GetCurPID 4 0
158 return IsWinOldApTask 2 0
159 stub GlobalHandleNoRIP
160 stub EMSCopy
161 pascal16 LocalCountFree() LocalCountFree
162 pascal16 LocalHeapSize() LocalHeapSize
163 pascal16 GlobalLRUOldest(word) GlobalLRUOldest
164 pascal16 GlobalLRUNewest(word) GlobalLRUNewest
165 stub A20Proc
166 pascal16 WinExec(ptr word) WinExec
167 pascal16 GetExpWinVer(word) GetExpWinVer
168 pascal16 DirectResAlloc(word word word) DirectResAlloc
169 pascal GetFreeSpace(word) GetFreeSpace
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
197 pascal16 GlobalFix(word) GlobalFix
198 pascal16 GlobalUnfix(word) GlobalUnfix
199 pascal16 SetHandleCount(word) SetHandleCount
200 stub ValidateFreeSpaces
201 stub ReplaceInst
202 stub RegisterPtrace
203 stub DebugBreak
204 stub SwapRecording
205 stub CVWBreak
206 pascal16 AllocSelectorArray(word) AllocSelectorArray
207 return IsDBCSLeadByte 2 0
216 pascal   RegEnumKey(long long ptr long) RegEnumKey
217 pascal   RegOpenKey(long ptr ptr) RegOpenKey
218 pascal   RegCreateKey(long ptr ptr) RegCreateKey
219 stub RegDeleteValue
220 pascal   RegCloseKey(long) RegCloseKey
221 pascal   RegSetValue(long ptr long ptr long) RegSetValue
222 stub RegDeleteValue
223 stub RegEnumValue
224 pascal   RegQueryValue(long ptr ptr ptr) RegQueryValue
225 stub RegQueryValueEx
226 stub RegSetValueEx
227 stub RegFlushKey
#228 K228
#229 K229
230 stub GlobalSmartPageLock
231 stub GlobalSmartPageUnlock
232 stub RegLoadKey
233 stub RegUnloadKey
234 stub RegSaveKey
235 stub InvalidateNlsCache
310 pascal16 LocalHandleDelta(word) LocalHandleDelta
311 stub GetSetKernelDosProc
314 stub DebugDefineSegment
315 pascal16 WriteOutProfiles() WriteOutProfiles
316 stub GetFreeMemInfo
318 stub FatalExitHook
319 stub FlushCachedFileHandle
320 pascal16 IsTask(word) IsTask
323 return IsRomModule 2 0
324 stub LogError
325 stub LogParamError
326 return IsRomFile 2 0
#327 K327
328 stub _DebugOutput
#329 K329
#332 stub THHOOK
334 pascal16 IsBadReadPtr(segptr word) IsBadReadPtr
335 pascal16 IsBadWritePtr(segptr word) IsBadWritePtr
336 pascal16 IsBadCodePtr(segptr) IsBadCodePtr
337 pascal16 IsBadStringPtr(segptr word) IsBadStringPtr
338 stub HasGPHandler
339 stub DiagQuery
340 stub DiagOutput
341 stub ToolHelpHook
342 stub __GP
343 stub RegisterWinOldApHook
344 stub GetWinOldApHooks
345 stub IsSharedSelector
346 pascal16 IsBadHugeReadPtr(segptr long) IsBadHugeReadPtr
347 pascal16 IsBadHugeWritePtr(segptr long) IsBadHugeWritePtr
348 pascal16 hmemcpy(ptr ptr long) hmemcpy
349 pascal   _hread(word segptr long) _hread
350 pascal   _hwrite(word ptr long) _hwrite
#351 BUNNY_351
352 stub lstrcatn
353 pascal lstrcpyn(segptr segptr word) WIN16_lstrcpyn
354 stub GetAppCompatFlags
355 pascal16 GetWinDebugInfo(ptr word) GetWinDebugInfo
356 pascal16 SetWinDebugInfo(ptr) SetWinDebugInfo
360 stub OpenFileEx
#361 PIGLET_361
403 pascal16 FarSetOwner(word word) FarSetOwner
404 pascal16 FarGetOwner(word) FarGetOwner
406 stub WritePrivateProfileStruct
407 stub GetPrivateProfileStruct
411 stub GetCurrentDirectory
412 stub SetCurrentDirectory
413 stub FindFirstFile
414 stub FindNextFile
415 stub FindClose
416 stub WritePrivateProfileSection
417 stub WriteProfileSection
418 stub GetPrivateProfileSection
419 stub GetProfileSection
420 stub GetFileAttributes
421 stub SetFileAttributes
422 stub GetDiskFreeSpace
432 stub FileTimeToLocalFileTime
450 pascal16 KERNEL_450() stub_KERNEL_450
491 stub RegisterServiceProcess
513 stub LoadLibraryEx32W
514 stub FreeLibrary32W
515 stub GetProcAddress32W
516 stub GetVDMPointer32W
517 stub CallProc32W
518 stub CallProcEx32W
627 stub IsBadFlatReadWritePtr
