name	imagehlp
type	win32
init	IMAGEHLP_LibMain

  1 stdcall BindImage(str str str) BindImage
  2 stdcall BindImageEx(long str str str ptr) BindImageEx
  3 stdcall CheckSumMappedFile(ptr long ptr ptr) CheckSumMappedFile
  4 stdcall EnumerateLoadedModules(long ptr ptr) EnumerateLoadedModules
  5 stdcall FindDebugInfoFile(str str str) FindDebugInfoFile
  6 stdcall FindExecutableImage(str str str) FindExecutableImage
  7 stdcall GetImageConfigInformation(ptr ptr) GetImageConfigInformation
  8 stdcall GetImageUnusedHeaderBytes(ptr ptr) GetImageUnusedHeaderBytes
  9 stdcall GetTimestampForLoadedLibrary(long) GetTimestampForLoadedLibrary
 10 stdcall ImageAddCertificate(long ptr ptr) ImageAddCertificate
 11 stdcall ImageDirectoryEntryToData(ptr long long ptr) ImageDirectoryEntryToData
 12 stdcall ImageEnumerateCertificates(long long ptr ptr long) ImageEnumerateCertificates
 13 stdcall ImageGetCertificateData(long long ptr ptr) ImageGetCertificateData
 14 stdcall ImageGetCertificateHeader(long long ptr) ImageGetCertificateHeader
 15 stdcall ImageGetDigestStream(long long ptr long) ImageGetDigestStream
 16 stdcall ImageLoad(str str) ImageLoad
 17 stdcall ImageNtHeader(ptr) ImageNtHeader
 18 stdcall ImageRemoveCertificate(long long) ImageRemoveCertificate
 19 stdcall ImageRvaToSection(ptr ptr long) ImageRvaToSection
 20 stdcall ImageRvaToVa(ptr ptr long ptr) ImageRvaToVa
 21 stdcall ImageUnload(ptr) ImageUnload
 22 stdcall ImagehlpApiVersion() ImagehlpApiVersion
 23 stdcall ImagehlpApiVersionEx(ptr) ImagehlpApiVersionEx
 24 stdcall MakeSureDirectoryPathExists(str) MakeSureDirectoryPathExists
 25 stdcall MapAndLoad(str str ptr long long) MapAndLoad
 26 stdcall MapDebugInformation(long str str long) MapDebugInformation
 27 stdcall MapFileAndCheckSumA(str ptr ptr) MapFileAndCheckSumA
 28 stdcall MapFileAndCheckSumW(wstr ptr ptr) MapFileAndCheckSumW
 29 stub  MarkImageAsRunFromSwap
 30 stdcall ReBaseImage(str str long long long long ptr ptr ptr ptr long) ReBaseImage
 31 stdcall RemovePrivateCvSymbolic(ptr ptr ptr) RemovePrivateCvSymbolic
 32 stdcall RemoveRelocations(ptr) RemoveRelocations
 33 stdcall SearchTreeForFile(str str str) SearchTreeForFile
 34 stdcall SetImageConfigInformation(ptr ptr) SetImageConfigInformation
 35 stdcall SplitSymbols(str str str long) SplitSymbols
 36 stdcall StackWalk(long long long ptr ptr ptr ptr ptr ptr) StackWalk
 37 stdcall SymCleanup(long) SymCleanup
 38 stdcall SymEnumerateModules(long ptr ptr) SymEnumerateModules
 39 stdcall SymEnumerateSymbols(long long ptr ptr) SymEnumerateSymbols
 40 stdcall SymFunctionTableAccess(long long) SymFunctionTableAccess
 41 stdcall SymGetModuleBase(long long) SymGetModuleBase
 42 stdcall SymGetModuleInfo(long long ptr) SymGetModuleInfo
 43 stdcall SymGetOptions() SymGetOptions
 44 stdcall SymGetSearchPath(long str long) SymGetSearchPath
 45 stdcall SymGetSymFromAddr(long long ptr ptr) SymGetSymFromAddr
 46 stdcall SymGetSymFromName(long str ptr) SymGetSymFromName
 47 stdcall SymGetSymNext(long ptr) SymGetSymNext
 48 stdcall SymGetSymPrev(long ptr) SymGetSymPrev
 49 stdcall SymInitialize(long str long) SymInitialize
 50 stdcall SymLoadModule(long long str str long long) SymLoadModule
 51 stdcall SymRegisterCallback(long ptr ptr) SymRegisterCallback
 52 stdcall SymSetOptions(long) SymSetOptions
 53 stdcall SymSetSearchPath(long str) SymSetSearchPath
 54 stdcall SymUnDName(ptr str long) SymUnDName
 55 stdcall SymUnloadModule(long long) SymUnloadModule
 56 stdcall TouchFileTimes(long ptr) TouchFileTimes
 57 stdcall UnDecorateSymbolName(str str long long) UnDecorateSymbolName
 58 stdcall UnMapAndLoad(ptr) UnMapAndLoad
 59 stdcall UnmapDebugInformation(ptr) UnmapDebugInformation
 60 stdcall UpdateDebugInfoFile(str str str ptr) UpdateDebugInfoFile
