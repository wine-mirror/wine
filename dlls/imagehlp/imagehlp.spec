name	imagehlp
type	win32
init	IMAGEHLP_LibMain

import	kernel32.dll

@ stdcall BindImage(str str str) BindImage
@ stdcall BindImageEx(long str str str ptr) BindImageEx
@ stdcall CheckSumMappedFile(ptr long ptr ptr) CheckSumMappedFile
@ stdcall EnumerateLoadedModules(long ptr ptr) EnumerateLoadedModules
@ stdcall FindDebugInfoFile(str str str) FindDebugInfoFile
@ stdcall FindExecutableImage(str str str) FindExecutableImage
@ stdcall GetImageConfigInformation(ptr ptr) GetImageConfigInformation
@ stdcall GetImageUnusedHeaderBytes(ptr ptr) GetImageUnusedHeaderBytes
@ stdcall GetTimestampForLoadedLibrary(long) GetTimestampForLoadedLibrary
@ stdcall ImageAddCertificate(long ptr ptr) ImageAddCertificate
@ stdcall ImageDirectoryEntryToData(ptr long long ptr) ImageDirectoryEntryToData
@ stdcall ImageEnumerateCertificates(long long ptr ptr long) ImageEnumerateCertificates
@ stdcall ImageGetCertificateData(long long ptr ptr) ImageGetCertificateData
@ stdcall ImageGetCertificateHeader(long long ptr) ImageGetCertificateHeader
@ stdcall ImageGetDigestStream(long long ptr long) ImageGetDigestStream
@ stdcall ImageLoad(str str) ImageLoad
@ stdcall ImageNtHeader(ptr) ImageNtHeader
@ stdcall ImageRemoveCertificate(long long) ImageRemoveCertificate
@ stdcall ImageRvaToSection(ptr ptr long) ImageRvaToSection
@ stdcall ImageRvaToVa(ptr ptr long ptr) ImageRvaToVa
@ stdcall ImageUnload(ptr) ImageUnload
@ stdcall ImagehlpApiVersion() ImagehlpApiVersion
@ stdcall ImagehlpApiVersionEx(ptr) ImagehlpApiVersionEx
@ stdcall MakeSureDirectoryPathExists(str) MakeSureDirectoryPathExists
@ stdcall MapAndLoad(str str ptr long long) MapAndLoad
@ stdcall MapDebugInformation(long str str long) MapDebugInformation
@ stdcall MapFileAndCheckSumA(str ptr ptr) MapFileAndCheckSumA
@ stdcall MapFileAndCheckSumW(wstr ptr ptr) MapFileAndCheckSumW
@ stub  MarkImageAsRunFromSwap
@ stdcall ReBaseImage(str str long long long long ptr ptr ptr ptr long) ReBaseImage
@ stdcall RemovePrivateCvSymbolic(ptr ptr ptr) RemovePrivateCvSymbolic
@ stdcall RemoveRelocations(ptr) RemoveRelocations
@ stdcall SearchTreeForFile(str str str) SearchTreeForFile
@ stdcall SetImageConfigInformation(ptr ptr) SetImageConfigInformation
@ stdcall SplitSymbols(str str str long) SplitSymbols
@ stdcall StackWalk(long long long ptr ptr ptr ptr ptr ptr) StackWalk
@ stdcall SymCleanup(long) SymCleanup
@ stdcall SymEnumerateModules(long ptr ptr) SymEnumerateModules
@ stdcall SymEnumerateSymbols(long long ptr ptr) SymEnumerateSymbols
@ stdcall SymFunctionTableAccess(long long) SymFunctionTableAccess
@ stdcall SymGetModuleBase(long long) SymGetModuleBase
@ stdcall SymGetModuleInfo(long long ptr) SymGetModuleInfo
@ stdcall SymGetOptions() SymGetOptions
@ stdcall SymGetSearchPath(long str long) SymGetSearchPath
@ stdcall SymGetSymFromAddr(long long ptr ptr) SymGetSymFromAddr
@ stdcall SymGetSymFromName(long str ptr) SymGetSymFromName
@ stdcall SymGetSymNext(long ptr) SymGetSymNext
@ stdcall SymGetSymPrev(long ptr) SymGetSymPrev
@ stdcall SymInitialize(long str long) SymInitialize
@ stdcall SymLoadModule(long long str str long long) SymLoadModule
@ stdcall SymRegisterCallback(long ptr ptr) SymRegisterCallback
@ stdcall SymSetOptions(long) SymSetOptions
@ stdcall SymSetSearchPath(long str) SymSetSearchPath
@ stdcall SymUnDName(ptr str long) SymUnDName
@ stdcall SymUnloadModule(long long) SymUnloadModule
@ stdcall TouchFileTimes(long ptr) TouchFileTimes
@ stdcall UnDecorateSymbolName(str str long long) UnDecorateSymbolName
@ stdcall UnMapAndLoad(ptr) UnMapAndLoad
@ stdcall UnmapDebugInformation(ptr) UnmapDebugInformation
@ stdcall UpdateDebugInfoFile(str str str ptr) UpdateDebugInfoFile
@ stdcall UpdateDebugInfoFileEx(str str str ptr long) UpdateDebugInfoFileEx
