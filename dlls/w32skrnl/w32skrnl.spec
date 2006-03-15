1 stub _kSetEnvironmentVariable@8
2 stub _SzFromImte@4
3 stdcall GetCurrentTask32()
4 stub _DebugPrintf
5 stub _NtCloseSection@8
6 stub _AllocKernelHeap@8
7 stub _SelToFlat
8 stub _kGetExeVersion@4
9 stub _ResizeKernelHeap@12
10 stub _DbgBackTo32
11 stub _kGethInstance@0
12 stub SelOffsetToLinear
13 stub DebugPrintf
14 stdcall _kGetWin32sDirectory@0() GetWin32sDirectory
15 stub _sprintf
16 stub _KIsDBCSLeadByte@4
17 stdcall -i386 W32S_BackTo32() kernel32.W32S_BackTo32
18 stub _KGetDbgFlags32
19 stub SelToFlat
20 stub _FreeKernelHeap@4
21 stub WIN32SYSDLL
#22
23 stub _KSilentException@4
24 stub _NtCreateSection@28
#25 stub _W32sError32@12
26 stub _NtOpenSection@12
27 stub _NtDupSection@4
28 stub _GetSelModName@20
29 stub _FGetDscr@8
30 stdcall _RtlNtStatusToDosError(long) ntdll.RtlNtStatusToDosError #FIXME: not sure
31 stub _NtFlushVirtualMemory@16
32 stub _snprintf
33 stub _NtUnmapViewOfSection@8

#34-149

153 stub _PvAllocVirtMem@16
154 stub SetVirtMemProcess
155 stub _FFreeVirtMem@12
156 stub _FSetVirtProtect@16
157 stub _QueryVirtMem@8
158 stub _FLockVirtMem@8
159 stub _FUnlockVirtMem@8

#160-170

171 stub _HHeapCreateHeap@12
#172
173 stub _FHeapDestroy@4
174 stub _PvAllocHeapMem@12
175 stub _PvResizeHeapMem@16
176 stub _FFreeHeapMem@8

#177-196

197 stub _PpdbGetProcess@0
198 stub _ErcGetError@0
199 stub _SetError@4

#200-223

224 stub _CbSearchPath@24
225 stub _CbSearchBinPath@16

#226-230

231 stub _FFindEnvVar@8

#232-251

252 stub _SzFileFromImte@4
253 stub _ImteFromFileSz@4
254 stub _ImteFromSz@4
255 stub _FGetProcAddr@12
256 stub _KLoadLibrary@8
257 stub _KFreeLibrary@4

#258-271

272 stub _lmaUserBase

#273-325

326 stub _KrnOutputDebugString@4

#327-359

360 stub _RtlRaiseException
361 stub _RtlUnwind
362 stub _RtlUnwind4

#363-401

402 stub _DfhCreateFile@12
403 stub _FCloseFile@4
404 stub _CbWriteFile@12
405 stub _LfoSetFilePos@12
406 stub _CbReadFile@12

#407-499

500 stub _SelOffsetToLinear@4
501 stub _ImteFromHModule@4
502 stub _HModuleFromImte@4
503 stub _ExitPEApp
#504 stdcall _GetThunkBuff@0() _GetThunkBuff
505 stub _Dos3Call@0
506 stub _GetPEInstanceData@0
507 stub _GetThreadTask@4
508 stub _BaseAddrFromImte@4
509 stub _GetTaskId@12
510 stub _KGetProcessDebugPort@4
511 stub _W32S_BackTo32
512 stub _KReadProcessMemory32@20
513 stub _KWriteProcessMemory32@20
514 stub _KGetThreadContext32@8
515 stub _KSetThreadContext32@8
516 stub _KQueryPerformanceCounter@8
#517
518 stub _KGetExitCodeThread32@8
519 stub _KGetExitCodeProcess32@8
520 stub _KGetTaskPpdb32@4
521 stub _KGetThreadPtdb32@4

#522-599

600 stub _GetCurProcFIOData@0
601 stub _KUseObject32@4
602 stub _KUnuseObject32@4
603 stub _KErcGetError32@0
604 stub _KCreateProcessHeap@4
605 stub _KGetSystemInfo@8
606 stub _KGlobalMemStat@4
607 stub _GetHandleFromAddr@4
608 stub _TlsNullSlot@4
609 stub _FWorkingSetSize@8
