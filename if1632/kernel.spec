# $Id: kernel.spec,v 1.3 1993/07/04 04:04:21 root Exp root $
#
name	kernel
id	1
length	415

#1 FATALEXIT
#2 EXITKERNEL
3   pascal GetVersion() GetVersion()
4   pascal16 LocalInit(word word word) WIN16_LocalInit(1 2 3)
5   pascal16 LocalAlloc(word word) WIN16_LocalAlloc(1 2)
6   pascal16 LocalReAlloc(word word word) WIN16_LocalReAlloc(1 2 3)
7   pascal16 LocalFree(word) WIN16_LocalFree(1)
8   pascal LocalLock(word) WIN16_LocalLock(1)
9   pascal16 LocalUnlock(word) WIN16_LocalUnlock(1)
10  pascal LocalSize(word) WIN16_LocalSize(1)
11  pascal16 LocalHandle(word) ReturnArg(1)
12  pascal16 LocalFlags(word) WIN16_LocalFlags(1)
13  pascal16 LocalCompact(word) WIN16_LocalCompact(1)
14  return LocalNotify 4 0
15  pascal16 GlobalAlloc(word long) WIN16_GlobalAlloc(1 2)
16  pascal16 GlobalReAlloc(word long word) GlobalReAlloc(1 2 3)
17  pascal16 GlobalFree(word) GlobalFree(1)
18  pascal GlobalLock(word) GlobalLock(1)
19  pascal16 GlobalUnlock(word) GlobalUnlock(1)
20  pascal GlobalSize(word) GlobalSize(1)
21  pascal GlobalHandle(word) GlobalHandle(1)
22  pascal16 GlobalFlags(word) GlobalFlags(1)
23  pascal16 LockSegment(s_word) KERNEL_LockSegment(1)
24  pascal UnlockSegment(s_word) KERNEL_UnlockSegment(1)
25  pascal GlobalCompact(long) GlobalCompact(1)
#26 GLOBALFREEALL
#28 GLOBALMASTERHANDLE
29  return Yield 0 0
30  pascal WaitEvent(word) KERNEL_WaitEvent(1)
#31 POSTEVENT
#32 SETPRIORITY
#33 LOCKCURRENTTASK
34  pascal SetTaskQueue(word word) SetTaskQueue(1 2)
35  pascal GetTaskQueue(word) GetTaskQueue(1)
36  pascal16 GetCurrentTask() GetCurrentTask()
37  pascal16 GetCurrentPDB() GetCurrentPDB()
#38 SETTASKSIGNALPROC
#41 ENABLEDOS
#42 DISABLEDOS
45  pascal16 LoadModule(ptr ptr) LoadModule(1 2)
46  pascal16 FreeModule(word) FreeLibrary(1)
47  pascal16 GetModuleHandle(ptr) GetModuleHandle(1)
48  pascal16 GetModuleUsage(word) GetModuleUsage(1)
49  pascal16 GetModuleFileName(word ptr s_word) GetModuleFileName(1 2 3)
50  pascal GetProcAddress(word ptr) GetProcAddress(1 2)
51  pascal MakeProcInstance(ptr word) CALLBACK_MakeProcInstance(1 2)
52  pascal FreeProcInstance(ptr) FreeProcInstance(1)
#53 CALLPROCINSTANCE
#54 pascal16 GETINSTANCEDATA
55  pascal16 Catch(ptr) Catch (1)
56  pascal Throw(ptr word) Throw(1 2)
57  pascal16 GetProfileInt(ptr ptr word) GetProfileInt(1 2 3)
58  pascal16 GetProfileString(ptr ptr ptr ptr word) GetProfileString(1 2 3 4 5)
59  pascal16 WriteProfileString(ptr ptr ptr) WriteProfileString(1 2 3)
60  pascal16 FindResource(word ptr ptr) FindResource(1 2 3)
61  pascal16 LoadResource(word word) LoadResource(1 2)
62  pascal LockResource(word) LockResource(1)
63  pascal16 FreeResource(word) FreeResource(1)
64  pascal16 AccessResource(word word) AccessResource(1 2)
65  pascal SizeofResource(word word) SizeofResource(1 2)
66  pascal16 AllocResource(word word long) AllocResource(1 2 3)
#67 SETRESOURCEHANDLER
68  pascal16 InitAtomTable(word) InitAtomTable(1)
69  pascal16 FindAtom(ptr) FindAtom(1)
70  pascal16 AddAtom(ptr) AddAtom(1)
71  pascal16 DeleteAtom(word) DeleteAtom(1)
72  pascal16 GetAtomName(word ptr word) GetAtomName(1 2 3)
73  pascal16 GetAtomHandle(word) GetAtomHandle(1)
74  pascal16 OpenFile(ptr ptr word) OpenFile(1 2 3)
#75 OPENPATHNAME
#76 DELETEPATHNAME
#77 RESERVED1
#78 RESERVED2
#79 RESERVED3
#80 RESERVED4
81  pascal16 _lclose(word) _lclose(1)
82  pascal16 _lread(word ptr word) _lread(1 2 3)
83  pascal16 _lcreat(ptr word) _lcreat(1 2)
84  pascal _llseek(word long word) _llseek(1 2 3)
85  pascal16 _lopen(ptr word) _lopen(1 2)
86  pascal16 _lwrite(word ptr word) _lwrite(1 2 3)
#87 RESERVED5
88  pascal lstrcpy(ptr ptr) lstrcpy(1 2)
89  pascal lstrcat(ptr ptr) lstrcat(1 2)
90  pascal16 lstrlen(ptr) lstrlen(1)
91  register InitTask(word word word word word
		      word word word word word) 
	     KERNEL_InitTask()
92  pascal16 GetTempDrive(byte) GetTempDrive(1)
93 pascal16 GetCodeHandle(ptr) GetCodeHandle(1)
#94 DEFINEHANDLETABLE
95  pascal16 LoadLibrary(ptr) LoadLibrary(1)
96  pascal16 FreeLibrary(word) FreeLibrary(1)
97  pascal16 GetTempFileName(byte ptr word ptr) GetTempFileName(1 2 3 4)
#98 GETLASTDISKCHANGE
#99 GETLPERRMODE
#100 VALIDATECODESEGMENTS
#101 NOHOOKDOSCALL
102 register DOS3Call(word word word word word
		      word word word word word) 
	     DOS3Call()
#103 NETBIOSCALL
#104 GETCODEINFO
#105 GETEXEVERSION
106 pascal SetSwapAreaSize(word) SetSwapAreaSize(1)
107 pascal SetErrorMode(word) SetErrorMode(1)
#108 SWITCHSTACKTO
#109 SWITCHSTACKBACK
#110 PATCHCODEHANDLE
111 pascal GlobalWire(word) GlobalLock(1)
112 pascal16 GlobalUnWire(word) GlobalUnlock(1)
113 equate __AHSHIFT 3
114 equate __AHINCR 8
115 pascal OutputDebugString(ptr) OutputDebugString(1)
#116 INITLIB
117 return OldYield 0 0
#118 GETTASKQUEUEDS
#119 GETTASKQUEUEES
#120 UNDEFDYNLINK
121 return LocalShrink 4 0
#122 ISTASKLOCKED
#123 KBDRST
#124 ENABLEKERNEL
#125 DISABLEKERNEL
#126 MEMORYFREED
127 pascal16 GetPrivateProfileInt(ptr ptr s_word ptr)
	     GetPrivateProfileInt(1 2 3 4)
128 pascal16 GetPrivateProfileString(ptr ptr ptr ptr s_word ptr)
	     GetPrivateProfileString(1 2 3 4 5 6)
129 pascal16 WritePrivateProfileString(ptr ptr ptr ptr)
	     WritePrivateProfileString(1 2 3 4)
130 pascal FileCDR(ptr) FileCDR(1)
131 pascal GetDOSEnvironment() GetDOSEnvironment()
132 pascal GetWinFlags() GetWinFlags()
#133 GETEXEPTR
134 pascal16 GetWindowsDirectory(ptr word) GetWindowsDirectory(1 2)
135 pascal16 GetSystemDirectory(ptr word) GetSystemDirectory(1 2)
136 pascal16 GetDriveType(byte) GetDriveType(1)
137 pascal FatalAppExit(word ptr) FatalAppExit(1 2)
#138 GETHEAPSPACES - This is not correct but may fake out most apps
138 return GetHeapSpaces 2 0x80004000
#139 DOSIGNAL
#140 SETSIGHANDLER
#141 INITTASK1
150 return DirectedYield 2 0
#151 WINOLDAPCALL
152 pascal16 GetNumTasks() GetNumTasks()
154 return GlobalNotify 4 0
#155 GETTASKDS
#156 LIMITEMSPAGES
#157 GETCURPID
#158 ISWINOLDAPTASK
#159 GLOBALHANDLENORIP
#160 EMSCOPY
#161 LOCALCOUNTFREE
#162 LOCALHEAPSIZE
163 pascal16 GlobalLRUOldest(word) ReturnArg(1)
164 pascal16 GlobalLRUNewest(word) ReturnArg(1)
#165 A20PROC
166 pascal16 WinExec(ptr word) WinExec(1 2)
#167 GETEXPWINVER
#168 DIRECTRESALLOC
169 pascal GetFreeSpace(word) GetFreeSpace(1)
170 pascal AllocCStoDSAlias(word) AllocDStoCSAlias(1)
171 pascal AllocDStoCSAlias(word) AllocDStoCSAlias(1)
#172 ALLOCALIAS
#173 __ROMBIOS
#174 __A000H
175 pascal16 AllocSelector(word) AllocSelector(1)
176 pascal16 FreeSelector(word) FreeSelector(1)
177 pascal16 PrestoChangoSelector(word word) PrestoChangoSelector(1 2)
178 equate __WINFLAGS 0x413
#179 __D000H
#180 LONGPTRADD
#181 __B000H
#182 __B800H
#183 __0000H
184 return GlobalDOSAlloc 4 0
185 return GlobalDOSFree 2 0
186 pascal16 GetSelectorBase(word) GetSelectorBase(1)
187 pascal16 SetSelectorBase(word long) SetSelectorBase(1 2)
188 pascal GetSelectorLimit(word) GetSelectorLimit(1)
189 pascal16 SetSelectorLimit(word long) SetSelectorLimit(1 2)
#190 __E000H
191 pascal GlobalPageLock(word) GlobalLock(1)
192 pascal GlobalPageUnlock(word) GlobalUnlock(1)
#193 __0040H
#194 __F000H
#195 __C000H
#196 SELECTORACCESSRIGHTS
197 pascal GlobalFix(word) GlobalLock(1)
198 pascal GlobalUnfix(word) GlobalUnlock(1)
199 pascal16 SetHandleCount(word) SetHandleCount(1)
#200 VALIDATEFREESPACES
#201 REPLACEINST
#202 REGISTERPTRACE
#203 DEBUGBREAK
#204 SWAPRECORDING
#205 CVWBREAK
#206 ALLOCSELECTORARRAY
207 return IsDBCSLeadByte 2 0
310 pascal LocalHandleDelta(word) WIN16_LocalHandleDelta(1)
#311 GETSETKERNELDOSPROC
#314 DEBUGDEFINESEGMENT
315 pascal WriteOutProfiles() sync_profiles()
#316 GETFREEMEMINFO
#318 FATALEXITHOOK
#319 FLUSHCACHEDFILEHANDLE
#320 ISTASK
323 pascal IsRomModule() IsRomModule()
#324 LOGERROR
#325 LOGPARAMERROR
#326 ISROMFILE
#327 K327
#328 _DEBUGOUTPUT
#329 K329
#332 THHOOK
334 pascal IsBadReadPtr(ptr word) IsBadReadPtr(1 2)
335 pascal IsBadWritePtr(ptr word) IsBadWritePtr(1 2)
336 pascal IsBadCodePtr(ptr) IsBadCodePtr(1)
337 pascal IsBadStringPtr(ptr word) IsBadStringPtr(1 2)
#338 HASGPHANDLER
#339 DIAGQUERY
#340 DIAGOUTPUT
#341 TOOLHELPHOOK
#342 __GP
#343 REGISTERWINOLDAPHOOK
#344 GETWINOLDAPHOOKS
#345 ISSHAREDSELECTOR
346 pascal IsBadHugeReadPtr(ptr long) IsBadHugeReadPtr(1 2)
347 pascal IsBadHugeWritePtr(ptr long) IsBadHugeWritePtr(1 2)
348 pascal hmemcpy(ptr ptr long) hmemcpy(1 2 3)
349 pascal16 _hread(word ptr long) _hread(1 2 3)
350 pascal16 _hwrite(word ptr long) _hwrite(1 2 3)
#351 BUNNY_351
353 pascal lstrcpyn(ptr ptr word) lstrcpyn(1 2 3)
#354 GETAPPCOMPATFLAGS
#355 GETWINDEBUGINFO
#356 SETWINDEBUGINFO
#403 K403
#404 K404
