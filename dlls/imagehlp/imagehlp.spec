@ stdcall BindImage(str str str)
@ stdcall BindImageEx(long str str str ptr)
@ stdcall CheckSumMappedFile(ptr long ptr ptr)
@ stdcall EnumerateLoadedModules(long ptr ptr) dbghelp.EnumerateLoadedModules
@ stdcall FindDebugInfoFile(str str str) dbghelp.FindDebugInfoFile
@ stdcall FindExecutableImage(str str str) dbghelp.FindExecutableImage
@ stdcall GetImageConfigInformation(ptr ptr)
@ stdcall GetImageUnusedHeaderBytes(ptr ptr)
@ stdcall GetTimestampForLoadedLibrary(long) dbghelp.GetTimestampForLoadedLibrary
@ stdcall ImageAddCertificate(long ptr ptr)
@ stdcall ImageDirectoryEntryToData(ptr long long ptr) ntdll.RtlImageDirectoryEntryToData
@ stdcall ImageEnumerateCertificates(long long ptr ptr long)
@ stdcall ImageGetCertificateData(long long ptr ptr)
@ stdcall ImageGetCertificateHeader(long long ptr)
@ stdcall ImageGetDigestStream(long long ptr long)
@ stdcall ImageLoad(str str)
@ stdcall ImageNtHeader(ptr) ntdll.RtlImageNtHeader
@ stdcall ImageRemoveCertificate(long long)
@ stdcall ImageRvaToSection(ptr ptr long) ntdll.RtlImageRvaToSection
@ stdcall ImageRvaToVa(ptr ptr long ptr) ntdll.RtlImageRvaToVa
@ stdcall ImageUnload(ptr)
@ stdcall ImagehlpApiVersion() dbghelp.ImagehlpApiVersion
@ stdcall ImagehlpApiVersionEx(ptr) dbghelp.ImagehlpApiVersionEx
@ stdcall MakeSureDirectoryPathExists(str) dbghelp.MakeSureDirectoryPathExists
@ stdcall MapAndLoad(str str ptr long long)
@ stdcall MapDebugInformation(long str str long) dbghelp.MapDebugInformation
@ stdcall MapFileAndCheckSumA(str ptr ptr)
@ stdcall MapFileAndCheckSumW(wstr ptr ptr)
@ stub  MarkImageAsRunFromSwap
@ stdcall ReBaseImage(str str long long long long ptr ptr ptr ptr long)
@ stdcall RemovePrivateCvSymbolic(ptr ptr ptr)
@ stdcall RemoveRelocations(ptr)
@ stdcall SearchTreeForFile(str str str) dbghelp.SearchTreeForFile
@ stdcall SetImageConfigInformation(ptr ptr)
@ stdcall SplitSymbols(str str str long)
@ stdcall StackWalk(long long long ptr ptr ptr ptr ptr ptr) dbghelp.StackWalk
@ stdcall SymCleanup(long) dbghelp.SymCleanup
@ stdcall SymEnumerateModules(long ptr ptr) dbghelp.SymEnumerateModules
@ stdcall SymEnumerateSymbols(long long ptr ptr) dbghelp.SymEnumerateSymbols
@ stdcall SymFunctionTableAccess(long long) dbghelp.SymFunctionTableAccess
@ stdcall SymGetModuleBase(long long) dbghelp.SymGetModuleBase
@ stdcall SymGetModuleInfo(long long ptr) dbghelp.SymGetModuleInfo
@ stdcall SymGetOptions() dbghelp.SymGetOptions
@ stdcall SymGetSearchPath(long str long) dbghelp.SymGetSearchPath
@ stdcall SymGetSymFromAddr(long long ptr ptr) dbghelp.SymGetSymFromAddr
@ stdcall SymGetSymFromName(long str ptr) dbghelp.SymGetSymFromName
@ stdcall SymGetSymNext(long ptr) dbgelp.SymGetSymNext
@ stdcall SymGetSymPrev(long ptr) dbgelp.SymGetSymPrev
@ stdcall SymInitialize(long str long) dbghelp.SymInitialize
@ stdcall SymLoadModule(long long str str long long) dbghelp.SymLoadModule
@ stdcall SymRegisterCallback(long ptr ptr) dbghelp.SymRegisterCallback
@ stdcall SymSetOptions(long) dbghelp.SymGetOptions
@ stdcall SymSetSearchPath(long str) dbghelp.SymSetSearchPath
@ stdcall SymUnDName(ptr str long) dbghelp.SymUnDName
@ stdcall SymUnloadModule(long long) dbghelp.SymUnloadModule
@ stdcall TouchFileTimes(long ptr)
@ stdcall UnDecorateSymbolName(str str long long) dbghelp.UnDecorateSymbolName
@ stdcall UnMapAndLoad(ptr)
@ stdcall UnmapDebugInformation(ptr) dbghelp.UnmapDebugInformation
@ stdcall UpdateDebugInfoFile(str str str ptr)
@ stdcall UpdateDebugInfoFileEx(str str str ptr long)
