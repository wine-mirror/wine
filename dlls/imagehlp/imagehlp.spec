@ stdcall BindImage(str str str)
@ stdcall BindImageEx(long str str str ptr)
@ stdcall CheckSumMappedFile(ptr long ptr ptr)
@ stdcall EnumerateLoadedModules(long ptr ptr) dbghelp.EnumerateLoadedModules
@ stub EnumerateLoadedModules64
@ stdcall FindDebugInfoFile(str str str) dbghelp.FindDebugInfoFile
@ stdcall FindDebugInfoFileEx(str str ptr ptr ptr) dbghelp.FindDebugInfoFileEx
@ stdcall FindExecutableImage(str str str) dbghelp.FindExecutableImage
@ stub FindExecutableImageEx
@ stub FindFileInPath
@ stub FindFileInSearchPath
@ stdcall GetImageConfigInformation(ptr ptr)
@ stdcall GetImageUnusedHeaderBytes(ptr ptr)
@ stdcall GetTimestampForLoadedLibrary(long) dbghelp.GetTimestampForLoadedLibrary
@ stdcall ImageAddCertificate(long ptr ptr)
@ stdcall ImageDirectoryEntryToData(ptr long long ptr) ntdll.RtlImageDirectoryEntryToData
@ stub ImageDirectoryEntryToDataEx
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
@ stub ReBaseImage64
@ stdcall RemovePrivateCvSymbolic(ptr ptr ptr)
@ stub RemovePrivateCvSymbolicEx
@ stdcall RemoveRelocations(ptr)
@ stdcall SearchTreeForFile(str str str) dbghelp.SearchTreeForFile
@ stdcall SetImageConfigInformation(ptr ptr)
@ stdcall SplitSymbols(str str str long)
@ stdcall StackWalk(long long long ptr ptr ptr ptr ptr ptr) dbghelp.StackWalk
@ stub StackWalk64
@ stdcall SymCleanup(long) dbghelp.SymCleanup
@ stdcall SymEnumSourceFiles(long long str ptr ptr) dbghelp.SymEnumSourceFiles
@ stub SymEnumSym
@ stdcall SymEnumSymbols(long long str ptr ptr) dbghelp.SymEnumSymbols
@ stdcall SymEnumTypes(long long ptr ptr) dbghelp.SymEnumTypes
@ stdcall SymEnumerateModules(long ptr ptr) dbghelp.SymEnumerateModules
@ stub SymEnumerateModules64
@ stdcall SymEnumerateSymbols(long long ptr ptr) dbghelp.SymEnumerateSymbols
@ stub SymEnumerateSymbols64
@ stub SymEnumerateSymbolsW
@ stub SymEnumerateSymbolsW64
@ stub SymFindFileInPath
@ stdcall SymFromAddr(long long ptr ptr) dbghelp.SymFromAddr
@ stdcall SymFromName(long str ptr) dbghelp.SymFromName
@ stdcall SymFunctionTableAccess(long long) dbghelp.SymFunctionTableAccess
@ stub SymFunctionTableAccess64
@ stdcall SymGetLineFromAddr(long long ptr ptr) dbghelp.SymGetLineFromAddr
@ stub SymGetLineFromAddr64
@ stub SymGetLineFromName
@ stub SymGetLineFromName64
@ stdcall SymGetLineNext(long ptr) dbghelp.SymGetLineNext
@ stub SymGetLineNext64
@ stdcall SymGetLinePrev(long ptr) dbghelp.SymGetLinePrev
@ stub SymGetLinePrev64
@ stdcall SymGetModuleBase(long long) dbghelp.SymGetModuleBase
@ stub SymGetModuleBase64
@ stdcall SymGetModuleInfo(long long ptr) dbghelp.SymGetModuleInfo
@ stub SymGetModuleInfo64
@ stub SymGetModuleInfoW
@ stub SymGetModuleInfoW64
@ stdcall SymGetOptions() dbghelp.SymGetOptions
@ stdcall SymGetSearchPath(long str long) dbghelp.SymGetSearchPath
@ stdcall SymGetSymFromAddr(long long ptr ptr) dbghelp.SymGetSymFromAddr
@ stub SymGetSymFromAddr64
@ stdcall SymGetSymFromName(long str ptr) dbghelp.SymGetSymFromName
@ stub SymGetSymFromName64
@ stdcall SymGetSymNext(long ptr) dbgelp.SymGetSymNext
@ stub SymGetSymNext64
@ stdcall SymGetSymPrev(long ptr) dbgelp.SymGetSymPrev
@ stub SymGetSymPrev64
@ stdcall SymGetTypeFromName(long long str ptr) dbghelp.SymGetTypeFromName
@ stdcall SymGetTypeInfo(long long long long ptr) dbghelp.SymGetTypeInfo
@ stdcall SymInitialize(long str long) dbghelp.SymInitialize
@ stdcall SymLoadModule(long long str str long long) dbghelp.SymLoadModule
@ stub SymLoadModule64
@ stub SymMatchFileName
@ stub SymMatchString
@ stdcall SymRegisterCallback(long ptr ptr) dbghelp.SymRegisterCallback
@ stub SymRegisterCallback64
@ stub SymRegisterFunctionEntryCallback
@ stub SymRegisterFunctionEntryCallback64
@ stdcall SymSetContext(long ptr ptr) dbghelp.SymSetContext
@ stdcall SymSetOptions(long) dbghelp.SymGetOptions
@ stdcall SymSetSearchPath(long str) dbghelp.SymSetSearchPath
@ stdcall SymUnDName(ptr str long) dbghelp.SymUnDName
@ stub SymUnDName64
@ stdcall SymUnloadModule(long long) dbghelp.SymUnloadModule
@ stub SymUnloadModule64
@ stdcall TouchFileTimes(long ptr)
@ stdcall UnDecorateSymbolName(str str long long) dbghelp.UnDecorateSymbolName
@ stdcall UnMapAndLoad(ptr)
@ stdcall UnmapDebugInformation(ptr) dbghelp.UnmapDebugInformation
@ stdcall UpdateDebugInfoFile(str str str ptr)
@ stdcall UpdateDebugInfoFileEx(str str str ptr long)
