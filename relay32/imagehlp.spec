name	imagehlp
type	win32
init	IMAGEHLP_LibMain

  1 stdcall BindImage(str str str) BindImage32
  2 stdcall BindImageEx(long str str str ptr) BindImageEx32
  3 stdcall CheckSumMappedFile(ptr long ptr ptr) CheckSumMappedFile32
  4 stdcall EnumerateLoadedModules(long ptr ptr) EnumerateLoadedModules32
  5 stdcall FindDebugInfoFile(str str str) FindDebugInfoFile32
  6 stdcall FindExecutableImage(str str str) FindExecutableImage32
  7 stdcall GetImageConfigInformation(ptr ptr) GetImageConfigInformation32
  8 stdcall GetImageUnusedHeaderBytes(ptr ptr) GetImageUnusedHeaderBytes32
  9 stdcall GetTimestampForLoadedLibrary(long) GetTimestampForLoadedLibrary32
 10 stdcall ImageAddCertificate(long ptr ptr) ImageAddCertificate32
 11 stdcall ImageDirectoryEntryToData(ptr long long ptr) ImageDirectoryEntryToData32
 12 stdcall ImageEnumerateCertificates(long long ptr ptr long) ImageEnumerateCertificates32
 13 stdcall ImageGetCertificateData(long long ptr ptr) ImageGetCertificateData32
 14 stdcall ImageGetCertificateHeader(long long ptr) ImageGetCertificateHeader32
 15 stdcall ImageGetDigestStream(long long ptr long) ImageGetDigestStream32
 16 stdcall ImageLoad(str str) ImageLoad32
 17 stdcall ImageNtHeader(ptr) ImageNtHeader32
 18 stdcall ImageRemoveCertificate(long long) ImageRemoveCertificate32
 19 stdcall ImageRvaToSection(ptr ptr long) ImageRvaToSection32
 20 stdcall ImageRvaToVa(ptr ptr long ptr) ImageRvaToVa32
 21 stdcall ImageUnload(ptr) ImageUnload32
 22 stdcall ImagehlpApiVersion() ImagehlpApiVersion32
 23 stdcall ImagehlpApiVersionEx(ptr) ImagehlpApiVersionEx32
 24 stdcall MakeSureDirectoryPathExists(str) MakeSureDirectoryPathExists32
 25 stdcall MapAndLoad(str str ptr long long) MapAndLoad32
 26 stdcall MapDebugInformation(long str str long) MapDebugInformation32
 27 stdcall MapFileAndCheckSumA(str ptr ptr) MapFileAndCheckSum32A
 28 stdcall MapFileAndCheckSumW(wstr ptr ptr) MapFileAndCheckSum32W
 29 stub  MarkImageAsRunFromSwap
 30 stdcall ReBaseImage(str str long long long long ptr ptr ptr ptr long) ReBaseImage32
 31 stdcall RemovePrivateCvSymbolic(ptr ptr ptr) RemovePrivateCvSymbolic32
 32 stdcall RemoveRelocations(ptr) RemoveRelocations32
 33 stdcall SearchTreeForFile(str str str) SearchTreeForFile32
 34 stdcall SetImageConfigInformation(ptr ptr) SetImageConfigInformation32
 35 stdcall SplitSymbols(str str str long) SplitSymbols32
 36 stdcall StackWalk(long long long ptr ptr ptr ptr ptr ptr) StackWalk32
 37 stdcall SymCleanup(long) SymCleanup32
 38 stdcall SymEnumerateModules(long ptr ptr) SymEnumerateModules32
 39 stdcall SymEnumerateSymbols(long long ptr ptr) SymEnumerateSymbols32
 40 stdcall SymFunctionTableAccess(long long) SymFunctionTableAccess32
 41 stdcall SymGetModuleBase(long long) SymGetModuleBase32
 42 stdcall SymGetModuleInfo(long long ptr) SymGetModuleInfo32
 43 stdcall SymGetOptions() SymGetOptions32
 44 stdcall SymGetSearchPath(long str long) SymGetSearchPath32
 45 stdcall SymGetSymFromAddr(long long ptr ptr) SymGetSymFromAddr32
 46 stdcall SymGetSymFromName(long str ptr) SymGetSymFromName32
 47 stdcall SymGetSymNext(long ptr) SymGetSymNext32
 48 stdcall SymGetSymPrev(long ptr) SymGetSymPrev32
 49 stdcall SymInitialize(long str long) SymInitialize32
 50 stdcall SymLoadModule(long long str str long long) SymLoadModule32
 51 stdcall SymRegisterCallback(long ptr ptr) SymRegisterCallback32
 52 stdcall SymSetOptions(long) SymSetOptions32
 53 stdcall SymSetSearchPath(long str) SymSetSearchPath32
 54 stdcall SymUnDName(ptr str long) SymUnDName32
 55 stdcall SymUnloadModule(long long) SymUnloadModule32
 56 stdcall TouchFileTimes(long ptr) TouchFileTimes32
 57 stdcall UnDecorateSymbolName(str str long long) UnDecorateSymbolName32
 58 stdcall UnMapAndLoad(ptr) UnMapAndLoad32
 59 stdcall UnmapDebugInformation(ptr) UnmapDebugInformation32
 60 stdcall UpdateDebugInfoFile(str str str ptr) UpdateDebugInfoFile32
