# $Id: kernel.spec,v 1.3 1993/07/04 04:04:21 root Exp root $
#
name	kernel
id	1
length	415

#1 FATALEXIT
#2 EXITKERNEL
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
#26 GLOBALFREEALL
#28 GLOBALMASTERHANDLE
29  return Yield 0 0
30  pascal WaitEvent(word) KERNEL_WaitEvent
#31 POSTEVENT
#32 SETPRIORITY
#33 LOCKCURRENTTASK
34  pascal SetTaskQueue(word word) SetTaskQueue
35  pascal GetTaskQueue(word) GetTaskQueue
36  pascal16 GetCurrentTask() GetCurrentTask
37  pascal16 GetCurrentPDB() GetCurrentPDB
#38 SETTASKSIGNALPROC
41  return EnableDos 0 0
42  return DisableDos 0 0
45  pascal16 LoadModule(ptr ptr) LoadModule
46  pascal16 FreeModule(word) FreeLibrary
47  pascal16 GetModuleHandle(ptr) GetModuleHandle
48  pascal16 GetModuleUsage(word) GetModuleUsage
49  pascal16 GetModuleFileName(word ptr s_word) GetModuleFileName
50  pascal GetProcAddress(word ptr) GetProcAddress
51  pascal MakeProcInstance(segptr word) MakeProcInstance
52  pascal FreeProcInstance(segptr) FreeProcInstance
#53 CALLPROCINSTANCE
#54 pascal16 GETINSTANCEDATA
55  pascal16 Catch(ptr) Catch 
56  pascal Throw(ptr word) Throw
57  pascal16 GetProfileInt(ptr ptr word) GetProfileInt
58  pascal16 GetProfileString(ptr ptr ptr ptr word) GetProfileString
59  pascal16 WriteProfileString(ptr ptr ptr) WriteProfileString
60  pascal16 FindResource(word segptr segptr) FindResource
61  pascal16 LoadResource(word word) LoadResource
62  pascal LockResource(word) WIN16_LockResource
63  pascal16 FreeResource(word) FreeResource
64  pascal16 AccessResource(word word) AccessResource
65  pascal SizeofResource(word word) SizeofResource
66  pascal16 AllocResource(word word long) AllocResource
#67 SETRESOURCEHANDLER
68  pascal16 InitAtomTable(word) InitAtomTable
69  pascal16 FindAtom(ptr) FindAtom
70  pascal16 AddAtom(ptr) AddAtom
71  pascal16 DeleteAtom(word) DeleteAtom
72  pascal16 GetAtomName(word ptr word) GetAtomName
73  pascal16 GetAtomHandle(word) GetAtomHandle
74  pascal16 OpenFile(ptr ptr word) OpenFile
#75 OPENPATHNAME
#76 DELETEPATHNAME
#77 RESERVED1
#78 RESERVED2
#79 RESERVED3
#80 RESERVED4
81  pascal16 _lclose(word) _lclose
82  pascal16 _lread(word ptr word) _lread
83  pascal16 _lcreat(ptr word) _lcreat
84  pascal _llseek(word long word) _llseek
85  pascal16 _lopen(ptr word) _lopen
86  pascal16 _lwrite(word ptr word) _lwrite
#87 RESERVED5
88  pascal lstrcpy(segptr segptr) lstrcpy
89  pascal lstrcat(segptr segptr) lstrcat
90  pascal16 lstrlen(ptr) lstrlen
91  register InitTask() KERNEL_InitTask
92  pascal16 GetTempDrive(byte) GetTempDrive
93 pascal16 GetCodeHandle(ptr) GetCodeHandle
#94 DEFINEHANDLETABLE
95  pascal16 LoadLibrary(ptr) LoadLibrary
96  pascal16 FreeLibrary(word) FreeLibrary
97  pascal16 GetTempFileName(byte ptr word ptr) GetTempFileName
#98 GETLASTDISKCHANGE
#99 GETLPERRMODE
#100 VALIDATECODESEGMENTS
#101 NOHOOKDOSCALL
102 register DOS3Call() DOS3Call
#103 NETBIOSCALL
#104 GETCODEINFO
#105 GETEXEVERSION
106 pascal SetSwapAreaSize(word) SetSwapAreaSize
107 pascal SetErrorMode(word) SetErrorMode
#108 SWITCHSTACKTO
#109 SWITCHSTACKBACK
#110 PATCHCODEHANDLE
111 pascal GlobalWire(word) GlobalWire
112 pascal16 GlobalUnWire(word) GlobalUnWire
113 equate __AHSHIFT 3
114 equate __AHINCR 8
115 pascal OutputDebugString(ptr) OutputDebugString
#116 INITLIB
117 return OldYield 0 0
#118 GETTASKQUEUEDS
#119 GETTASKQUEUEES
#120 UNDEFDYNLINK
121 pascal16 LocalShrink(word word) LocalShrink
#122 ISTASKLOCKED
#123 KBDRST
124 return EnableKernel 0 0
125 return DisableKernel 0 0
#126 MEMORYFREED
127 pascal16 GetPrivateProfileInt(ptr ptr s_word ptr) GetPrivateProfileInt
128 pascal16 GetPrivateProfileString(ptr ptr ptr ptr s_word ptr)
             GetPrivateProfileString
129 pascal16 WritePrivateProfileString(ptr ptr ptr ptr)
             WritePrivateProfileString
130 pascal FileCDR(ptr) FileCDR
131 pascal GetDOSEnvironment() GetDOSEnvironment
132 pascal GetWinFlags() GetWinFlags
#133 GETEXEPTR
134 pascal16 GetWindowsDirectory(ptr word) GetWindowsDirectory
135 pascal16 GetSystemDirectory(ptr word) GetSystemDirectory
136 pascal16 GetDriveType(byte) GetDriveType
137 pascal FatalAppExit(word ptr) FatalAppExit
138 pascal GetHeapSpaces(word) GetHeapSpaces
#139 DOSIGNAL
#140 SETSIGHANDLER
#141 INITTASK1
150 return DirectedYield 2 0
#151 WINOLDAPCALL
152 pascal16 GetNumTasks() GetNumTasks
154 return GlobalNotify 4 0
#155 GETTASKDS
#156 LIMITEMSPAGES
#157 GETCURPID
#158 ISWINOLDAPTASK
#159 GLOBALHANDLENORIP
#160 EMSCOPY
161 pascal16 LocalCountFree() LocalCountFree
162 pascal16 LocalHeapSize() LocalHeapSize
163 pascal16 GlobalLRUOldest(word) GlobalLRUOldest
164 pascal16 GlobalLRUNewest(word) GlobalLRUNewest
#165 A20PROC
166 pascal16 WinExec(ptr word) WinExec
#167 GETEXPWINVER
#168 DIRECTRESALLOC
169 pascal GetFreeSpace(word) GetFreeSpace
170 pascal16 AllocCStoDSAlias(word) AllocCStoDSAlias
171 pascal16 AllocDStoCSAlias(word) AllocDStoCSAlias
172 pascal16 AllocAlias(word) AllocCStoDSAlias
#173 __ROMBIOS
#174 __A000H
175 pascal16 AllocSelector(word) AllocSelector
176 pascal16 FreeSelector(word) FreeSelector
177 pascal16 PrestoChangoSelector(word word) PrestoChangoSelector
178 equate __WINFLAGS 0x413
#179 __D000H
180 pascal16 LongPtrAdd(long long) LongPtrAdd
#181 __B000H
#182 __B800H
#183 __0000H
184 pascal GlobalDOSAlloc(long) GlobalDOSAlloc
185 pascal16 GlobalDOSFree(word) GlobalDOSFree
186 pascal GetSelectorBase(word) GetSelectorBase
187 pascal16 SetSelectorBase(word long) SetSelectorBase
188 pascal GetSelectorLimit(word) GetSelectorLimit
189 pascal16 SetSelectorLimit(word long) SetSelectorLimit
#190 __E000H
191 pascal16 GlobalPageLock(word) GlobalPageLock
192 pascal16 GlobalPageUnlock(word) GlobalPageUnlock
#193 __0040H
#194 __F000H
#195 __C000H
196 pascal16 SelectorAccessRights(word word word) SelectorAccessRights
197 pascal16 GlobalFix(word) GlobalFix
198 pascal16 GlobalUnfix(word) GlobalUnfix
199 pascal16 SetHandleCount(word) SetHandleCount
#200 VALIDATEFREESPACES
#201 REPLACEINST
#202 REGISTERPTRACE
#203 DEBUGBREAK
#204 SWAPRECORDING
#205 CVWBREAK
206 pascal16 AllocSelectorArray(word) AllocSelectorArray
207 return IsDBCSLeadByte 2 0
310 pascal16 LocalHandleDelta(word) LocalHandleDelta
#311 GETSETKERNELDOSPROC
#314 DEBUGDEFINESEGMENT
315 pascal WriteOutProfiles() sync_profiles
#316 GETFREEMEMINFO
#318 FATALEXITHOOK
#319 FLUSHCACHEDFILEHANDLE
#320 ISTASK
323 return IsRomModule 2 0
#324 LOGERROR
#325 LOGPARAMERROR
326 return IsRomFile 2 0
#327 K327
#328 _DEBUGOUTPUT
#329 K329
#332 THHOOK
334 pascal16 IsBadReadPtr(segptr word) IsBadReadPtr
335 pascal16 IsBadWritePtr(segptr word) IsBadWritePtr
336 pascal16 IsBadCodePtr(segptr) IsBadCodePtr
337 pascal16 IsBadStringPtr(segptr word) IsBadStringPtr
#338 HASGPHANDLER
#339 DIAGQUERY
#340 DIAGOUTPUT
#341 TOOLHELPHOOK
#342 __GP
#343 REGISTERWINOLDAPHOOK
#344 GETWINOLDAPHOOKS
#345 ISSHAREDSELECTOR
346 pascal16 IsBadHugeReadPtr(segptr long) IsBadHugeReadPtr
347 pascal16 IsBadHugeWritePtr(segptr long) IsBadHugeWritePtr
348 pascal hmemcpy(ptr ptr long) hmemcpy
349 pascal16 _hread(word ptr long) _hread
350 pascal16 _hwrite(word ptr long) _hwrite
#351 BUNNY_351
353 pascal lstrcpyn(segptr segptr word) lstrcpyn
#354 GETAPPCOMPATFLAGS
#355 GETWINDEBUGINFO
#356 SETWINDEBUGINFO
403 pascal16 FarSetOwner(word word) FarSetOwner
404 pascal16 FarGetOwner(word) FarGetOwner
